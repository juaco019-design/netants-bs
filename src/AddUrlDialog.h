#pragma once
#include <wx/wx.h>
#include <wx/dialog.h>

// Dialogo Agregar URL - Clon de NetAnts "Add" dialog
class AddUrlDialog : public wxDialog {
public:
    AddUrlDialog(wxWindow* parent);

    wxString GetUrl() const { return url_; }
    wxString GetSaveDir() const { return saveDir_; }
    int GetSegments() const { return segments_; }

private:
    void OnBrowse(wxCommandEvent&);
    void OnOk(wxCommandEvent&);

    wxTextCtrl* txtUrl = nullptr;
    wxTextCtrl* txtDir = nullptr;
    wxSpinCtrl* spinSeg = nullptr;

    wxString url_;
    wxString saveDir_;
    int segments_ = 4;

    wxDECLARE_EVENT_TABLE();
};
