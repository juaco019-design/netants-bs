#pragma once
#include <wx/wx.h>
#include "DownloadManager.h"
#include "OptionsDialog.h"

// App principal - reemplaza WinMain obsoleto del dump RecStudio (L00403710)
class NetAntsApp : public wxApp {
public:
    virtual bool OnInit() override;
    virtual int OnExit() override;
private:
    std::unique_ptr<DownloadManager> mgr_;
    AppOptions opts_;
};
