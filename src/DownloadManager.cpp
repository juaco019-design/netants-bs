#include "DownloadManager.h"
#include "Utils.h"
#include <wx/log.h>

DownloadManager::DownloadManager(std::filesystem::path dataPath)
    : dataPath_(std::move(dataPath))
{
    auto jobsDir = dataPath_ / "jobs";
    storage_ = std::make_unique<JobStorage>(jobsDir);
    multi_ = curl_multi_init();
    curl_multi_setopt(multi_, CURLMOPT_MAXCONNECTS, 20L);
    // Cargar jobs persistidos
    auto loaded = storage_->LoadAll();
    for (auto& j : loaded) {
        auto id = j.GetId();
        jobs_[id] = std::make_unique<DownloadJob>(std::move(j));
        if (jobs_[id]->GetStatus() == JobStatus::Queued) {
            queue_.push(id);
        }
    }
}

DownloadManager::~DownloadManager() {
    Stop();
    if (multi_) curl_multi_cleanup(multi_);
}

void DownloadManager::Start() {
    if (running_) return;
    running_ = true;
    worker_ = std::thread(&DownloadManager::WorkerLoop, this);
}

void DownloadManager::Stop() {
    if (!running_) return;
    running_ = false;
    queueCv_.notify_all();
    if (worker_.joinable()) worker_.join();
    // Guarda todo
    std::lock_guard<std::mutex> l(jobsMtx_);
    for (auto& [id,job] : jobs_) storage_->Save(*job);
}

std::string DownloadManager::AddJob(std::string url, std::string saveDir, int segments) {
    if (!Utils::IsValidUrl(url)) return "";
    if (saveDir.empty()) saveDir = (Utils::GetDataPath() / "Downloads").string();
    Utils::EnsureDir(saveDir);
    auto job = std::make_unique<DownloadJob>(url, saveDir, segments);
    auto id = job->GetId();
    {
        std::lock_guard<std::mutex> l(jobsMtx_);
        storage_->Save(*job);
        jobs_[id] = std::move(job);
    }
    {
        std::lock_guard<std::mutex> l(queueMtx_);
        queue_.push(id);
    }
    queueCv_.notify_one();
    NotifyUi();
    return id;
}

void DownloadManager::PauseJob(const std::string& id) {
    std::lock_guard<std::mutex> l(jobsMtx_);
    auto it = jobs_.find(id);
    if (it != jobs_.end()) { it->second->Pause(); storage_->Save(*it->second); }
    NotifyUi();
}

void DownloadManager::ResumeJob(const std::string& id) {
    {
        std::lock_guard<std::mutex> l(jobsMtx_);
        auto it = jobs_.find(id);
        if (it != jobs_.end()) { it->second->Resume(); storage_->Save(*it->second); }
    }
    {
        std::lock_guard<std::mutex> l(queueMtx_);
        queue_.push(id);
    }
    queueCv_.notify_one();
    NotifyUi();
}

void DownloadManager::RemoveJob(const std::string& id, bool deleteFile) {
    std::unique_ptr<DownloadJob> toDel;
    {
        std::lock_guard<std::mutex> l(jobsMtx_);
        auto it = jobs_.find(id);
        if (it != jobs_.end()) {
            toDel = std::move(it->second);
            jobs_.erase(it);
        }
    }
    if (toDel) {
        if (deleteFile) {
            try { std::filesystem::remove(toDel->GetFullPath()); } catch(...) {}
        }
        storage_->Remove(id);
    }
    NotifyUi();
}

DownloadJob* DownloadManager::FindJob(const std::string& id) {
    std::lock_guard<std::mutex> l(jobsMtx_);
    auto it = jobs_.find(id);
    return it==jobs_.end()?nullptr:it->second.get();
}

std::vector<DownloadManager::JobSnapshot> DownloadManager::GetSnapshots() {
    std::vector<JobSnapshot> out;
    std::lock_guard<std::mutex> l(jobsMtx_);
    for (auto& [id,job] : jobs_) {
        JobSnapshot s;
        s.id = job->GetId();
        s.url = job->GetUrl();
        s.fileName = job->GetFileName();
        s.saveDir = job->GetSaveDir();
        s.status = job->GetStatus();
        s.totalSize = job->GetTotalSize();
        s.downloaded = job->GetDownloaded();
        s.progress = job->GetProgress();
        s.stats = job->GetStats();
        s.error = job->GetError();
        out.push_back(std::move(s));
    }
    return out;
}

std::vector<DownloadJob*> DownloadManager::GetAllJobs() {
    std::vector<DownloadJob*> out;
    std::lock_guard<std::mutex> l(jobsMtx_);
    for (auto& [id,job] : jobs_) out.push_back(job.get());
    return out;
}

void DownloadManager::NotifyUi() {
    if (uiCb_) uiCb_();
}

void DownloadManager::WorkerLoop() {
    wxLogMessage("DownloadManager worker iniciado, maxConcurrent=%d", maxConcurrent_);
    int active = 0;
    while (running_) {
        std::string jobId;
        {
            std::unique_lock<std::mutex> l(queueMtx_);
            if (queue_.empty()) {
                queueCv_.wait_for(l, std::chrono::milliseconds(500), [&]{ return !queue_.empty() || !running_; });
            }
            if (!running_) break;
            if (!queue_.empty() && active < maxConcurrent_) {
                jobId = queue_.front(); queue_.pop();
                ++active;
            }
        }
        if (jobId.empty()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            continue;
        }

        // Obtener job ptr
        DownloadJob* jobPtr = nullptr;
        {
            std::lock_guard<std::mutex> l(jobsMtx_);
            auto it = jobs_.find(jobId);
            if (it == jobs_.end()) { --active; continue; }
            jobPtr = it->second.get();
        }

        // Ejecutar (bloqueante pero en worker)
        bool completed = jobPtr->Execute(multi_, [&](const std::string& id){
            // progreso intermedio
            {
                std::lock_guard<std::mutex> l(jobsMtx_);
                storage_->Save(*jobPtr);
            }
            NotifyUi();
        });

        {
            std::lock_guard<std::mutex> l(jobsMtx_);
            storage_->Save(*jobPtr);
        }
        NotifyUi();
        --active;

        // Si pausado, no reencolar automaticamente; si error, puede reencolar con backoff (futuro)
        if (!completed && jobPtr->GetStatus()==JobStatus::Queued) {
            std::lock_guard<std::mutex> l(queueMtx_);
            queue_.push(jobId);
            ++active;
        }
    }
}
