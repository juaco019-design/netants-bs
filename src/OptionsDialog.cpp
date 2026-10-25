#include "OptionsDialog.h"
#include <wx/spinctrl.h>
#include <wx/dirdlg.h>
#include "Utils.h"

wxBEGIN_EVENT_TABLE(OptionsDialog, wxDialog)
    EVT_BUTTON(wxID_OK, OptionsDialog::OnOk)
wxEND_EVENT_TABLE()

OptionsDialog::OptionsDialog(wxWindow* parent, AppOptions& opts)
    : wxDialog(parent, wxID_ANY, "Opciones - NetAnts Reborn", wxDefaultPosition, wxSize(520,460)), opts_(opts)
{
    auto* top = new wxBoxSizer(wxVERTICAL);
    auto* nb = new wxNotebook(this, wxID_ANY);

    // Pagina General
    auto* pageGeneral = new wxPanel(nb);
    auto* gSizer = new wxBoxSizer(wxVERTICAL);
    auto* fg = new wxFlexGridSizer(2,2,8,12);
    fg->Add(new wxStaticText(pageGeneral, wxID_ANY, "Descargas simultaneas:"),0,wxALIGN_CENTER_VERTICAL);
    spConcurrent = new wxSpinCtrl(pageGeneral, wxID_ANY, wxString::Format("%d", opts.maxConcurrent), wxDefaultPosition, wxSize(90,-1), wxSP_ARROW_KEYS, 1, 10, opts.maxConcurrent);
    fg->Add(spConcurrent);

    fg->Add(new wxStaticText(pageGeneral, wxID_ANY, "Segmentos por descarga:"),0,wxALIGN_CENTER_VERTICAL);
    spSegments = new wxSpinCtrl(pageGeneral, wxID_ANY, wxString::Format("%d", opts.maxSegments), wxDefaultPosition, wxSize(90,-1), wxSP_ARROW_KEYS, 1,16, opts.maxSegments);
    fg->Add(spSegments);

    fg->Add(new wxStaticText(pageGeneral, wxID_ANY, "Limite velocidad (KB/s, 0=ilimitado):"),0,wxALIGN_CENTER_VERTICAL);
    spSpeed = new wxSpinCtrl(pageGeneral, wxID_ANY, wxString::Format("%lld", opts.speedLimitKBps), wxDefaultPosition, wxSize(120,-1), wxSP_ARROW_KEYS, 0, 1000000, opts.speedLimitKBps);
    fg->Add(spSpeed);

    gSizer->Add(fg,0,wxALL,12);
    // Default dir
    gSizer->Add(new wxStaticText(pageGeneral, wxID_ANY, "Carpeta por defecto:"),0,wxLEFT|wxTOP,12);
    auto* dirRow = new wxBoxSizer(wxHORIZONTAL);
    txtDir = new wxTextCtrl(pageGeneral, wxID_ANY, opts.defaultDir.IsEmpty() ? wxString(Utils::GetDataPath().string())+"\\Downloads" : opts.defaultDir);
    dirRow->Add(txtDir,1,wxEXPAND|wxRIGHT,6);
    dirRow->Add(new wxButton(pageGeneral, wxID_ANY, "Examinar"),0);
    gSizer->Add(dirRow,0,wxEXPAND|wxLEFT|wxRIGHT,12);
    pageGeneral->SetSizer(gSizer);

    // Pagina Proxy
    auto* pageProxy = new wxPanel(nb);
    auto* pSizer = new wxBoxSizer(wxVERTICAL);
    chkProxy = new wxCheckBox(pageProxy, wxID_ANY, "Usar Proxy HTTP/FTP");
    chkProxy->SetValue(opts.useProxy);
    pSizer->Add(chkProxy,0,wxALL,12);
    auto* fg2 = new wxFlexGridSizer(2,2,8,12);
    fg2->Add(new wxStaticText(pageProxy, wxID_ANY, "Host:"),0,wxALIGN_CENTER_VERTICAL);
    txtProxyHost = new wxTextCtrl(pageProxy, wxID_ANY, opts.proxyHost);
    fg2->Add(txtProxyHost,1,wxEXPAND);
    fg2->Add(new wxStaticText(pageProxy, wxID_ANY, "Puerto:"),0,wxALIGN_CENTER_VERTICAL);
    spProxyPort = new wxSpinCtrl(pageProxy, wxID_ANY, wxString::Format("%d", opts.proxyPort), wxDefaultPosition, wxSize(90,-1), wxSP_ARROW_KEYS, 1,65535, opts.proxyPort);
    fg2->Add(spProxyPort);
    fg2->Add(new wxStaticText(pageProxy, wxID_ANY, "Usuario:"),0,wxALIGN_CENTER_VERTICAL);
    txtProxyUser = new wxTextCtrl(pageProxy, wxID_ANY, opts.proxyUser);
    fg2->Add(txtProxyUser,1,wxEXPAND);
    fg2->Add(new wxStaticText(pageProxy, wxID_ANY, "Clave:"),0,wxALIGN_CENTER_VERTICAL);
    txtProxyPass = new wxTextCtrl(pageProxy, wxID_ANY, opts.proxyPass, wxDefaultPosition, wxDefaultSize, wxTE_PASSWORD);
    fg2->Add(txtProxyPass,1,wxEXPAND);
    pSizer->Add(fg2,0,wxEXPAND|wxALL,12);
    pageProxy->SetSizer(pSizer);

    nb->AddPage(pageGeneral, "General");
    nb->AddPage(pageProxy, "Proxy");

    top->Add(nb,1,wxEXPAND|wxALL,8);
    auto* btns = new wxStdDialogButtonSizer();
    btns->AddButton(new wxButton(this, wxID_OK, "Aceptar"));
    btns->AddButton(new wxButton(this, wxID_CANCEL, "Cancelar"));
    btns->Realize();
    top->Add(btns,0,wxEXPAND|wxALL,8);
    SetSizer(top);
}

void OptionsDialog::OnOk(wxCommandEvent&) {
    opts_.maxConcurrent = spConcurrent->GetValue();
    opts_.maxSegments = spSegments->GetValue();
    opts_.speedLimitKBps = spSpeed->GetValue();
    opts_.useProxy = chkProxy->GetValue();
    opts_.proxyHost = txtProxyHost->GetValue();
    opts_.proxyPort = spProxyPort->GetValue();
    opts_.proxyUser = txtProxyUser->GetValue();
    opts_.proxyPass = txtProxyPass->GetValue();
    opts_.defaultDir = txtDir->GetValue();
    EndModal(wxID_OK);
}
