#include "AddUrlDialog.h"
#include "Utils.h"
#include <wx/dirdlg.h>
#include <wx/spinctrl.h>
#include <wx/valtext.h>

wxBEGIN_EVENT_TABLE(AddUrlDialog, wxDialog)
    EVT_BUTTON(wxID_OK, AddUrlDialog::OnOk)
    EVT_BUTTON(wxID_OPEN, AddUrlDialog::OnBrowse)
wxEND_EVENT_TABLE()

AddUrlDialog::AddUrlDialog(wxWindow* parent)
    : wxDialog(parent, wxID_ANY, "Agregar descarga - NetAnts Reborn", wxDefaultPosition, wxSize(560,280), wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER)
{
    auto* mainSizer = new wxBoxSizer(wxVERTICAL);

    // URL
    mainSizer->Add(new wxStaticText(this, wxID_ANY, "URL (HTTP / FTP):"), 0, wxLEFT|wxTOP, 12);
    txtUrl = new wxTextCtrl(this, wxID_ANY, "", wxDefaultPosition, wxDefaultSize, wxTE_PROCESS_ENTER);
    txtUrl->SetHint("https://example.com/archivo.zip  o  ftp://ftp.example.com/file.iso");
    mainSizer->Add(txtUrl, 0, wxEXPAND|wxLEFT|wxRIGHT|wxTOP, 12);

    // Directorio
    mainSizer->Add(new wxStaticText(this, wxID_ANY, "Guardar en:"), 0, wxLEFT|wxTOP, 12);
    auto* dirSizer = new wxBoxSizer(wxHORIZONTAL);
    wxString defaultDir = wxString(Utils::GetDataPath().string()) + "\\Downloads";
    txtDir = new wxTextCtrl(this, wxID_ANY, defaultDir);
    dirSizer->Add(txtDir, 1, wxEXPAND|wxRIGHT, 6);
    dirSizer->Add(new wxButton(this, wxID_OPEN, "Examinar..."), 0);
    mainSizer->Add(dirSizer, 0, wxEXPAND|wxLEFT|wxRIGHT|wxTOP, 12);

    // Segmentos
    auto* segSizer = new wxBoxSizer(wxHORIZONTAL);
    segSizer->Add(new wxStaticText(this, wxID_ANY, "Conexiones (segmentos):"), 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 8);
    spinSeg = new wxSpinCtrl(this, wxID_ANY, "4", wxDefaultPosition, wxSize(80,-1), wxSP_ARROW_KEYS, 1, 16, 4);
    segSizer->Add(spinSeg, 0);
    segSizer->Add(new wxStaticText(this, wxID_ANY, "  (NetAnts clasico usaba 4-5)"), 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 12);
    mainSizer->Add(segSizer, 0, wxLEFT|wxRIGHT|wxTOP, 12);

    mainSizer->AddStretchSpacer(1);

    // Botones
    auto* btnSizer = new wxStdDialogButtonSizer();
    btnSizer->AddButton(new wxButton(this, wxID_OK, "Descargar"));
    btnSizer->AddButton(new wxButton(this, wxID_CANCEL, "Cancelar"));
    btnSizer->Realize();
    mainSizer->Add(btnSizer, 0, wxEXPAND|wxALL, 12);

    SetSizer(mainSizer);
    txtUrl->SetFocus();
}

void AddUrlDialog::OnBrowse(wxCommandEvent&) {
    wxDirDialog dlg(this, "Seleccione carpeta de descarga", txtDir->GetValue());
    if (dlg.ShowModal()==wxID_OK) txtDir->SetValue(dlg.GetPath());
}

void AddUrlDialog::OnOk(wxCommandEvent&) {
    wxString url = txtUrl->GetValue().Trim().Trim(false);
    if (url.IsEmpty() || !Utils::IsValidUrl(url.ToStdString())) {
        wxMessageBox("URL invalida. Debe empezar con http:// https:// o ftp://", "Error", wxICON_ERROR);
        return;
    }
    wxString dir = txtDir->GetValue();
    if (dir.IsEmpty()) {
        wxMessageBox("Seleccione carpeta de destino", "Error", wxICON_ERROR);
        return;
    }
    url_ = url;
    saveDir_ = dir;
    segments_ = spinSeg->GetValue();
    EndModal(wxID_OK);
}
