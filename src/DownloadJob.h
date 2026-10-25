#pragma once
// DownloadJob - Unidad basica de descarga, modernizacion de la logica de NetAnts 1.x
// NetAnts original: C, WinInet, segmentos fijos, .ant files
// Nuevo: libcurl multi-handle, segmentos dinamicos, resume, FTP+HTTP, progreso thread-safe

#include <string>
#include <vector>
#include <atomic>
#include <mutex>
#include <chrono>
#include <functional>
#include <curl/curl.h>
#include <nlohmann/json.hpp>

enum class JobStatus {
    Queued = 0,
    Downloading,
    Paused,
    Completed,
    Error,
    Merging
};

struct Segment {
    int64_t start = 0;
    int64_t end = -1; // -1 = hasta el final
    int64_t downloaded = 0;
    std::string tempFile; // archivo temporal por segmento
    bool completed = false;
};

struct JobStats {
    double speedBps = 0;          // bytes/sec instant
    double avgSpeedBps = 0;
    int64_t totalDownloaded = 0;
    int remainingSec = -1;
    int activeConnections = 0;
};

class DownloadJob {
public:
    // Callbacks
    using ProgressCb = std::function<void(const std::string& id)>;

    DownloadJob() = default;
    DownloadJob(std::string url, std::string saveDir, int numSegments = 4);

    // No copy, move ok
    DownloadJob(const DownloadJob&) = delete;
    DownloadJob& operator=(const DownloadJob&) = delete;
    DownloadJob(DownloadJob&&) = default;
    DownloadJob& operator=(DownloadJob&&) = default;

    // Getters
    const std::string& GetId() const { return id_; }
    const std::string& GetUrl() const { return url_; }
    const std::string& GetFileName() const { return fileName_; }
    const std::string& GetSaveDir() const { return saveDir_; }
    std::string GetFullPath() const;
    JobStatus GetStatus() const { return status_.load(); }
    int64_t GetTotalSize() const { return totalSize_; }
    int64_t GetDownloaded() const;
    double GetProgress() const; // 0..1
    JobStats GetStats() const;
    std::string GetError() const { std::lock_guard<std::mutex> l(mtx_); return lastError_; }
    int GetNumSegments() const { return numSegments_; }
    std::vector<Segment> GetSegments() const { std::lock_guard<std::mutex> l(mtx_); return segments_; }

    // Control
    void Pause();
    void Resume();
    void Cancel();

    // Core - ejecutado por DownloadManager en worker thread
    // Devuelve true si completo
    bool Execute(CURLM* multi, ProgressCb progressCb);

    // Persistencia JSON (.job) - reemplaza .ant/.job binario original
    nlohmann::json ToJson() const;
    static DownloadJob FromJson(const nlohmann::json& j);

    // Config
    void SetFileName(const std::string& name) { fileName_ = name; }
    void SetSaveDir(const std::string& dir) { saveDir_ = dir; }
    void SetNumSegments(int n) { numSegments_ = std::clamp(n,1,16); }
    void SetStatus(JobStatus s) { status_ = s; }

private:
    std::string id_; // uuid corto
    std::string url_;
    std::string fileName_;
    std::string saveDir_;
    std::string referer_;
    std::string userAgent_ = "NetAntsReborn/2.0 (Windows; libcurl)";

    std::atomic<JobStatus> status_{JobStatus::Queued};
    int64_t totalSize_ = -1; // desconocido hasta HEAD
    int numSegments_ = 4;

    std::vector<Segment> segments_;
    mutable std::mutex mtx_;
    std::string lastError_;

    // Medicion velocidad
    std::chrono::steady_clock::time_point startTime_;
    std::atomic<int64_t> downloaded_{0};
    std::atomic<double> speed_{0};

    // Interno
    bool FetchFileSize(CURL* easy);
    bool InitSegments();
    static size_t WriteCallback(void* ptr, size_t size, size_t nmemb, void* userdata);
    static int ProgressCallback(void* clientp, curl_off_t dltotal, curl_off_t dlnow, curl_off_t, curl_off_t);
    std::string GenerateId();
    bool MergeSegments();

    // Para multi-handle por segmento
    struct TransferCtx {
        DownloadJob* job;
        Segment* seg;
        FILE* fp = nullptr;
        int64_t segDownloaded = 0;
    };
};
