#include "JobStorage.h"
#include "Utils.h"
#include <fstream>
#include <nlohmann/json.hpp>

JobStorage::JobStorage(std::filesystem::path baseDir) : baseDir_(std::move(baseDir)) {
    Utils::EnsureDir(baseDir_);
}

std::filesystem::path JobStorage::JobPath(const std::string& id) const {
    return baseDir_ / (id + ".job");
}

bool JobStorage::Save(const DownloadJob& job) {
    try {
        auto path = JobPath(job.GetId());
        auto j = job.ToJson();
        // Escritura atomica: escribe .tmp y renombra
        auto tmp = path; tmp += ".tmp";
        std::ofstream out(tmp);
        out << j.dump(4);
        out.close();
        std::filesystem::rename(tmp, path);
        return true;
    } catch (...) { return false; }
}

bool JobStorage::Remove(const std::string& id) {
    try {
        auto p = JobPath(id);
        if (std::filesystem::exists(p)) std::filesystem::remove(p);
        return true;
    } catch(...) { return false; }
}

std::vector<DownloadJob> JobStorage::LoadAll() {
    std::vector<DownloadJob> jobs;
    try {
        for (auto& entry : std::filesystem::directory_iterator(baseDir_)) {
            if (entry.path().extension() == ".job") {
                std::ifstream in(entry.path());
                if (!in) continue;
                nlohmann::json j;
                in >> j;
                auto job = DownloadJob::FromJson(j);
                // Si estaba Downloading, pasa a Queued al recargar (crash recovery)
                if (job.GetStatus() == JobStatus::Downloading || job.GetStatus() == JobStatus::Merging) {
                    job.SetStatus(JobStatus::Queued);
                }
                jobs.push_back(std::move(job));
            }
        }
    } catch(...) {}
    return jobs;
}
