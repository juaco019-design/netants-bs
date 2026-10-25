#pragma once
// DownloadManager - Core multihilo, reemplazo de la logica de NetAnts original
// Original: WinInet, thread por segmento con mensajes WM_
// Nuevo: curl_multi + thread pool, cola prioritaria, scheduler, limite velocidad global

#include "DownloadJob.h"
#include "JobStorage.h"
#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <unordered_map>
#include <atomic>
#include <functional>

class DownloadManager {
public:
    using UiCallback = std::function<void()>; // notifica UI para refrescar

    explicit DownloadManager(std::filesystem::path dataPath);
    ~DownloadManager();

    // Lifecycle
    void Start();
    void Stop();

    // Jobs
    std::string AddJob(std::string url, std::string saveDir, int segments = 4);
    void PauseJob(const std::string& id);
    void ResumeJob(const std::string& id);
    void RemoveJob(const std::string& id, bool deleteFile = false);
    std::vector<DownloadJob*> GetAllJobs(); // thread-safe snapshot pointers? devolvemos copia
    DownloadJob* FindJob(const std::string& id);

    // Snapshot para UI (copia segura)
    struct JobSnapshot {
        std::string id;
        std::string url;
        std::string fileName;
        std::string saveDir;
        JobStatus status;
        int64_t totalSize;
        int64_t downloaded;
        double progress;
        JobStats stats;
        std::string error;
    };
    std::vector<JobSnapshot> GetSnapshots();

    void SetUiCallback(UiCallback cb) { uiCb_ = std::move(cb); }
    void SetMaxConcurrent(int n) { maxConcurrent_ = n; }
    void SetGlobalSpeedLimit(int64_t bps) { speedLimitBps_ = bps; }

private:
    void WorkerLoop();
    void NotifyUi();

    std::filesystem::path dataPath_;
    std::unique_ptr<JobStorage> storage_;

    std::unordered_map<std::string, std::unique_ptr<DownloadJob>> jobs_;
    mutable std::mutex jobsMtx_;

    std::queue<std::string> queue_;
    std::mutex queueMtx_;
    std::condition_variable queueCv_;

    std::thread worker_;
    std::atomic<bool> running_{false};
    int maxConcurrent_ = 3; // simultaneas
    std::atomic<int64_t> speedLimitBps_{0};

    UiCallback uiCb_;

    // curl multi global
    CURLM* multi_ = nullptr;
};
