#pragma once
#include <wx/wx.h>
#include <string>
#include <filesystem>

// Utilidades generales - reemplazo de codigo obsoleto WinAPI del dump RecStudio
namespace Utils {

    // Formatea bytes a KB/MB/GB human readable
    wxString FormatBytes(int64_t bytes);

    // Formatea velocidad
    wxString FormatSpeed(double bytesPerSec);

    // Formatea tiempo restante
    wxString FormatTimeLeft(double seconds);

    // Extrae nombre de archivo de URL
    std::string FileNameFromUrl(const std::string& url);

    // Path de datos usuario: %APPDATA%/NetAntsReborn
    std::filesystem::path GetDataPath();

    // Asegura directorio existe
    void EnsureDir(const std::filesystem::path& p);

    // Valida URL (HTTP/FTP)
    bool IsValidUrl(const std::string& url);

    // Detecta FTP vs HTTP
    bool IsFtpUrl(const std::string& url);
}
