#include "DownloadJob.h"
#include "Utils.h"
#include <random>
#include <fstream>
#include <filesystem>
#include <cpr/cpr.h> // para HEAD rapido
#include <wx/log.h>

namespace fs = std::filesystem;

// ================= IMPLEMENTACION DownloadJob =================

DownloadJob::DownloadJob(std::string url, std::string saveDir, int numSegments)
    : url_(std::move(url)), saveDir_(std::move(saveDir)), numSegments_(numSegments)
{
    id_ = GenerateId();
    fileName_ = Utils::FileNameFromUrl(url_);
    status_ = JobStatus::Queued;
    startTime_ = std::chrono::steady_clock::now();
}

std::string DownloadJob::GenerateId() {
    static std::mt19937_64 rng(std::random_device{}());
    static std::uniform_int_distribution<uint64_t> dist;
    uint64_t v = dist(rng);
    char buf[17];
    snprintf(buf, sizeof(buf), "%016llx", (unsigned long long)v);
    return std::string(buf, 8); // 8 hex corto
}

std::string DownloadJob::GetFullPath() const {
    fs::path p = fs::path(saveDir_) / fileName_;
    return p.string();
}

int64_t DownloadJob::GetDownloaded() const {
    return downloaded_.load();
}

double DownloadJob::GetProgress() const {
    if (totalSize_ <= 0) return 0.0;
    double d = (double)GetDownloaded() / (double)totalSize_;
    return std::clamp(d, 0.0, 1.0);
}

JobStats DownloadJob::GetStats() const {
    JobStats s;
    s.totalDownloaded = GetDownloaded();
    auto now = std::chrono::steady_clock::now();
    double elapsed = std::chrono::duration<double>(now - startTime_).count();
    if (elapsed > 0) s.avgSpeedBps = s.totalDownloaded / elapsed;
    s.speedBps = speed_.load();
    if (s.speedBps > 1) s.remainingSec = int((totalSize_ - s.totalDownloaded) / s.speedBps);
    else s.remainingSec = -1;
    s.activeConnections = numSegments_;
    return s;
}

void DownloadJob::Pause() { status_ = JobStatus::Paused; }
void DownloadJob::Resume() { if(status_==JobStatus::Paused) status_=JobStatus::Queued; }
void DownloadJob::Cancel() { status_ = JobStatus::Error; lastError_="Cancelado por usuario"; }

bool DownloadJob::FetchFileSize(CURL* easy) {
    // Intento 1: HEAD con libcurl (soporta FTP y HTTP)
    curl_easy_reset(easy);
    curl_easy_setopt(easy, CURLOPT_URL, url_.c_str());
    curl_easy_setopt(easy, CURLOPT_NOBODY, 1L);
    curl_easy_setopt(easy, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(easy, CURLOPT_USERAGENT, userAgent_.c_str());
    curl_easy_setopt(easy, CURLOPT_CONNECTTIMEOUT, 15L);
    curl_easy_setopt(easy, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(easy, CURLOPT_SSL_VERIFYHOST, 0L);
    if (Utils::IsFtpUrl(url_)) {
        // FTP: usa SIZE command via CURLOPT_NOBODY + header
        curl_easy_setopt(easy, CURLOPT_FTPLISTONLY, 0L);
    }
    CURLcode res = curl_easy_perform(easy);
    if (res == CURLE_OK) {
        curl_off_t cl = -1;
        curl_easy_getinfo(easy, CURLINFO_CONTENT_LENGTH_DOWNLOAD_T, &cl);
        if (cl > 0) { totalSize_ = (int64_t)cl; return true; }
        // FTP fallback: intenta via cpr / curl info
        double cl2 = 0;
        curl_easy_getinfo(easy, CURLINFO_CONTENT_LENGTH_DOWNLOAD, &cl2);
        if (cl2 > 0) { totalSize_ = (int64_t)cl2; return true; }
    }
    // Si no se pudo obtener, asumimos desconocido pero permitimos descarga single-thread
    totalSize_ = -1;
    return false;
}

bool DownloadJob::InitSegments() {
    std::lock_guard<std::mutex> l(mtx_);
    segments_.clear();
    if (totalSize_ <= 0) {
        // Tamaño desconocido -> 1 segmento hasta final
        Segment s;
        s.start = 0; s.end = -1; s.downloaded = 0;
        s.tempFile = GetFullPath() + ".part0";
        segments_.push_back(std::move(s));
        numSegments_ = 1;
        return true;
    }
    int64_t part = totalSize_ / numSegments_;
    for (int i = 0; i < numSegments_; ++i) {
        Segment s;
        s.start = i * part;
        s.end = (i == numSegments_-1) ? totalSize_-1 : (i+1)*part -1;
        s.downloaded = 0;
        s.tempFile = GetFullPath() + ".part" + std::to_string(i);
        // Resume: si .part existe, toma su tamaño
        try {
            if (fs::exists(s.tempFile)) {
                s.downloaded = (int64_t)fs::file_size(s.tempFile);
                downloaded_ += s.downloaded;
            }
        } catch(...) {}
        segments_.push_back(std::move(s));
    }
    return true;
}

size_t DownloadJob::WriteCallback(void* ptr, size_t size, size_t nmemb, void* userdata) {
    TransferCtx* ctx = (TransferCtx*)userdata;
    size_t total = size * nmemb;
    if (!ctx->fp) return 0;
    size_t written = fwrite(ptr, 1, total, ctx->fp);
    if (written) {
        ctx->segDownloaded += written;
        ctx->job->downloaded_ += written;
        ctx->seg->downloaded += written;
        // Actualiza velocidad aproximada via progress callback real
    }
    return written;
}

int DownloadJob::ProgressCallback(void* clientp, curl_off_t dltotal, curl_off_t dlnow, curl_off_t, curl_off_t) {
    TransferCtx* ctx = (TransferCtx*)clientp;
    if (ctx->job->GetStatus() == JobStatus::Paused) return 1; // aborta transfer
    // calcula speed (libcurl no lo da aqui, lo estimamos afuera)
    return 0;
}

bool DownloadJob::Execute(CURLM* multi, ProgressCb progressCb) {
    if (status_ == JobStatus::Paused || status_ == JobStatus::Completed) return false;
    status_ = JobStatus::Downloading;
    startTime_ = std::chrono::steady_clock::now();
    lastError_.clear();

    CURL* easySize = curl_easy_init();
    FetchFileSize(easySize);
    curl_easy_cleanup(easySize);

    InitSegments();

    // Crea handles por segmento
    std::vector<CURL*> easies;
    std::vector<TransferCtx> ctxs(segments_.size());

    for (size_t i = 0; i < segments_.size(); ++i) {
        auto& seg = segments_[i];
        if (seg.completed) continue;
        CURL* e = curl_easy_init();
        ctxs[i].job = this;
        ctxs[i].seg = &seg;
        ctxs[i].segDownloaded = seg.downloaded;

        // Abrir archivo temporal en append
        FILE* fp = fopen(seg.tempFile.c_str(), "ab");
        if (!fp) { lastError_ = "No se pudo abrir " + seg.tempFile; status_=JobStatus::Error; return false; }
        ctxs[i].fp = fp;

        curl_easy_setopt(e, CURLOPT_URL, url_.c_str());
        curl_easy_setopt(e, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(e, CURLOPT_WRITEDATA, &ctxs[i]);
        curl_easy_setopt(e, CURLOPT_XFERINFOFUNCTION, ProgressCallback);
        curl_easy_setopt(e, CURLOPT_XFERINFODATA, &ctxs[i]);
        curl_easy_setopt(e, CURLOPT_NOPROGRESS, 0L);
        curl_easy_setopt(e, CURLOPT_FOLLOWLOCATION, 1L);
        curl_easy_setopt(e, CURLOPT_USERAGENT, userAgent_.c_str());
        curl_easy_setopt(e, CURLOPT_CONNECTTIMEOUT, 20L);
        curl_easy_setopt(e, CURLOPT_LOW_SPEED_LIMIT, 1L);
        curl_easy_setopt(e, CURLOPT_LOW_SPEED_TIME, 60L);
        curl_easy_setopt(e, CURLOPT_SSL_VERIFYPEER, 0L);
        curl_easy_setopt(e, CURLOPT_SSL_VERIFYHOST, 0L);

        if (totalSize_ > 0) {
            // Range: bytes=start+downloaded - end
            int64_t from = seg.start + seg.downloaded;
            int64_t to = seg.end;
            if (from <= to) {
                std::string range = std::to_string(from) + "-" + std::to_string(to);
                curl_easy_setopt(e, CURLOPT_RANGE, range.c_str());
            }
        } else {
            // resume desde lo descargado
            if (seg.downloaded > 0) {
                curl_easy_setopt(e, CURLOPT_RESUME_FROM_LARGE, (curl_off_t)seg.downloaded);
            }
        }

        if (Utils::IsFtpUrl(url_)) {
            curl_easy_setopt(e, CURLOPT_FTP_USE_EPSV, 1L);
        }

        curl_multi_add_handle(multi, e);
        easies.push_back(e);
    }

    // Loop multi
    int stillRunning = 0;
    auto lastProgress = std::chrono::steady_clock::now();
    int64_t lastBytes = downloaded_.load();
    do {
        CURLMcode mc = curl_multi_perform(multi, &stillRunning);
        if (mc != CURLM_OK) break;

        // Espera I/O (optimizado)
        int numfds = 0;
        curl_multi_wait(multi, nullptr, 0, 200, &numfds);

        // Chequea si usuario pauso
        if (status_ == JobStatus::Paused) {
            break;
        }

        // Calcula velocidad cada 500ms
        auto now = std::chrono::steady_clock::now();
        if (std::chrono::duration_cast<std::chrono::milliseconds>(now-lastProgress).count() > 500) {
            int64_t cur = downloaded_.load();
            double elapsed = std::chrono::duration<double>(now-lastProgress).count();
            if (elapsed > 0) speed_ = (cur - lastBytes) / elapsed;
            lastBytes = cur;
            lastProgress = now;
            if (progressCb) progressCb(id_);
        }

        // Chequea mensajes
        CURLMsg* msg;
        int msgsLeft;
        while ((msg = curl_multi_info_read(multi, &msgsLeft))) {
            if (msg->msg == CURLMSG_DONE) {
                if (msg->data.result != CURLE_OK) {
                    // Log pero no falla todo inmediatamente - NetAnts reintenta
                    lastError_ = curl_easy_strerror(msg->data.result);
                    // wxLogMessage("Segment error: %s", lastError_);
                }
            }
        }
    } while (stillRunning);

    // Limpieza handles
    for (auto e : easies) {
        curl_multi_remove_handle(multi, e);
        curl_easy_cleanup(e);
    }
    for (auto& c : ctxs) if (c.fp) fclose(c.fp);

    if (status_ == JobStatus::Paused) return false;

    // Verifica si todos segmentos completados
    bool allDone = true;
    if (totalSize_ > 0) {
        if (GetDownloaded() < totalSize_) allDone = false;
    } else {
        // tamaño desconocido: si no hubo error, asumimos done
        allDone = lastError_.empty();
    }

    if (allDone) {
        status_ = JobStatus::Merging;
        if (progressCb) progressCb(id_);
        if (MergeSegments()) {
            status_ = JobStatus::Completed;
            speed_ = 0;
            if (progressCb) progressCb(id_);
            return true;
        } else {
            status_ = JobStatus::Error;
            lastError_ = "Fallo al fusionar segmentos";
            return false;
        }
    } else {
        if (status_ != JobStatus::Error) status_ = JobStatus::Paused;
        return false;
    }
}

bool DownloadJob::MergeSegments() {
    try {
        fs::path finalPath = GetFullPath();
        Utils::EnsureDir(finalPath.parent_path());
        std::ofstream out(finalPath, std::ios::binary | std::ios::trunc);
        if (!out) return false;
        for (auto& seg : segments_) {
            std::ifstream in(seg.tempFile, std::ios::binary);
            if (!in) continue;
            out << in.rdbuf();
            in.close();
            fs::remove(seg.tempFile);
        }
        out.close();
        // Limpia estado
        segments_.clear();
        return true;
    } catch (std::exception& e) {
        lastError_ = e.what();
        return false;
    }
}

// JSON .job moderno - reemplaza binario original con ads
nlohmann::json DownloadJob::ToJson() const {
    nlohmann::json j;
    j["id"] = id_;
    j["url"] = url_;
    j["fileName"] = fileName_;
    j["saveDir"] = saveDir_;
    j["totalSize"] = totalSize_;
    j["downloaded"] = GetDownloaded();
    j["status"] = (int)status_.load();
    j["numSegments"] = numSegments_;
    j["referer"] = referer_;
    j["userAgent"] = userAgent_;
    nlohmann::json segs = nlohmann::json::array();
    std::lock_guard<std::mutex> l(mtx_);
    for (auto& s : segments_) {
        nlohmann::json sj;
        sj["start"] = s.start;
        sj["end"] = s.end;
        sj["downloaded"] = s.downloaded;
        sj["tempFile"] = s.tempFile;
        sj["completed"] = s.completed;
        segs.push_back(sj);
    }
    j["segments"] = segs;
    j["lastError"] = lastError_;
    return j;
}

DownloadJob DownloadJob::FromJson(const nlohmann::json& j) {
    DownloadJob job;
    job.id_ = j.value("id", job.GenerateId());
    job.url_ = j.value("url", "");
    job.fileName_ = j.value("fileName", Utils::FileNameFromUrl(job.url_));
    job.saveDir_ = j.value("saveDir", "");
    job.totalSize_ = j.value("totalSize", -1LL);
    job.downloaded_ = j.value("downloaded", 0LL);
    job.status_ = (JobStatus)j.value("status", (int)JobStatus::Queued);
    job.numSegments_ = j.value("numSegments", 4);
    job.referer_ = j.value("referer", "");
    job.userAgent_ = j.value("userAgent", "NetAntsReborn/2.0");
    job.lastError_ = j.value("lastError", "");
    if (j.contains("segments") && j["segments"].is_array()) {
        for (auto& sj : j["segments"]) {
            Segment s;
            s.start = sj.value("start", 0LL);
            s.end = sj.value("end", -1LL);
            s.downloaded = sj.value("downloaded", 0LL);
            s.tempFile = sj.value("tempFile", "");
            s.completed = sj.value("completed", false);
            job.segments_.push_back(s);
        }
    }
    return job;
}
