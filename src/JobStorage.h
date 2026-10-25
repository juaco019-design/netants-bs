#pragma once
// JobStorage - Gestion de archivos .job en JSON (reemplazo de .ant binario + registry)
// Usa nlohmann/json para persistencia moderna, atomica y portable

#include "DownloadJob.h"
#include <filesystem>
#include <vector>

class JobStorage {
public:
    explicit JobStorage(std::filesystem::path baseDir);

    // Carga todos los .job
    std::vector<DownloadJob> LoadAll();

    // Guarda un job (sobrescribe)
    bool Save(const DownloadJob& job);

    // Borra .job
    bool Remove(const std::string& id);

    // Path del archivo .job
    std::filesystem::path JobPath(const std::string& id) const;

private:
    std::filesystem::path baseDir_;
};
