#pragma once
// MainFrame - Clon visual de NetAnts 1.25
// Layout original: 
// - Toolbar con iconos 16x16 flat
// - Split: Tree categorias izquierda, ListView detalles derecha
// - StatusBar con animacion y progreso global
// Funcionalidades clonadas: Nuevo, Iniciar, Pausar, Borrar, Propiedades, Opciones, Arriba/Abajo

#include <wx/wx.h>
#include <wx/listctrl.h>
#include <wx/splitter.h>
#include <wx/treectrl.h>
#include <wx/notebook.h>
#include "DownloadManager.h"
#include "OptionsDialog.h"

class MainFrame : public wxFrame {
public:
    MainFrame(DownloadManager& mgr, AppOptions& opts);
    ~MainFrame();

private:
    // UI setup
    void CreateToolbar();
    void CreateStatusBar();
    void CreateLayout();
    void BindEvents();
    void RefreshList(); // llamado desde DownloadManager callback

    // Event handlers
    void OnNew(wxCommandEvent&);
    void OnStart(wxCommandEvent&);
    void OnPause(wxCommandEvent&);
    void OnDelete(wxCommandEvent&);
    void OnProps(wxCommandEvent&);
    void OnOptions(wxCommandEvent&);
    void OnExit(wxCommandEvent&);
    void OnAbout(wxCommandEvent&);
    void OnListDClick(wxListEvent&);
    void OnCategoryChanged(wxTreeEvent&);
    void OnTimer(wxTimerEvent&);
    void OnAntTimer(wxTimerEvent&);

    // Helpers
    void UpdateStatusBar();
    wxString StatusToString(JobStatus s) const;
    int GetSelectedJobIndex() const;

    DownloadManager& mgr_;
    AppOptions& opts_;

    wxSplitterWindow* splitter_ = nullptr;
    wxTreeCtrl* catTree_ = nullptr;
    wxListCtrl* list_ = nullptr;
    wxTimer refreshTimer_;
    wxTimer antTimer_;
    int antFrame_ = 0;
    std::vector<wxBitmap> antFrames_;

    enum {
        ID_NEW = wxID_HIGHEST+1,
        ID_START,
        ID_PAUSE,
        ID_DELETE,
        ID_PROPS,
        ID_OPTIONS,
        ID_ANT_TIMER = 2000
    };

    wxDECLARE_EVENT_TABLE();
};
