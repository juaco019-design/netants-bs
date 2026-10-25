#include "App.h"
#include "MainFrame.h"
#include "Utils.h"
#include <curl/curl.h>
#include <wx/stdpaths.h>

bool NetAntsApp::OnInit() {
    if (!wxApp::OnInit()) return false;

    // Inicializa libcurl global (reemplaza OleInitialize obsoleto del dump)
    curl_global_init(CURL_GLOBAL_ALL);

    // Asegura directorios
    auto dataPath = Utils::GetDataPath();
    Utils::EnsureDir(dataPath);
    Utils::EnsureDir(dataPath / "jobs");
    Utils::EnsureDir(dataPath / "Downloads");

    // Opciones por defecto (carga desde JSON si existe)
    opts_.defaultDir = wxString((dataPath / "Downloads").string());
    opts_.maxConcurrent = 3;
    opts_.maxSegments = 4;

    // Crea manager
    mgr_ = std::make_unique<DownloadManager>(dataPath);
    mgr_->SetMaxConcurrent(opts_.maxConcurrent);
    mgr_->Start();

    // Frame principal (clon interfaz NetAnts)
    auto* frame = new MainFrame(*mgr_, opts_);
    frame->Show(true);
    SetTopWindow(frame);

    wxLogMessage("NetAnts Reborn iniciado. DataPath=%s", dataPath.string());
    return true;
}

int NetAntsApp::OnExit() {
    if (mgr_) {
        mgr_->Stop();
        mgr_.reset();
    }
    curl_global_cleanup();
    return wxApp::OnExit();
}
