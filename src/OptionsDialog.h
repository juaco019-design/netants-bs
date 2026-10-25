#pragma once
#include <wx/wx.h>
#include <wx/propdlg.h>
#include <wx/spinctrl.h>

// Opciones - Replica las opciones clave de NetAnts sin ads/obsoleto
struct AppOptions {
    int maxConcurrent = 3;          // descargas simultaneas
    int maxSegments = 4;            // por defecto
    int64_t speedLimitKBps = 0;     // 0 = ilimitado
    bool useProxy = false;
    wxString proxyHost;
    int proxyPort = 8080;
    wxString proxyUser;
    wxString proxyPass;
    wxString userAgent = "NetAntsReborn/2.0";
    wxString defaultDir;
    bool autoStartQueue = true;
    bool confirmDelete = true;
};

class OptionsDialog : public wxDialog {
public:
    OptionsDialog(wxWindow* parent, AppOptions& opts);
private:
    void OnOk(wxCommandEvent&);
    AppOptions& opts_;
    wxSpinCtrl* spConcurrent;
    wxSpinCtrl* spSegments;
    wxSpinCtrl* spSpeed;
    wxCheckBox* chkProxy;
    wxTextCtrl* txtProxyHost;
    wxSpinCtrl* spProxyPort;
    wxTextCtrl* txtProxyUser;
    wxTextCtrl* txtProxyPass;
    wxTextCtrl* txtDir;

    wxDECLARE_EVENT_TABLE();
};
