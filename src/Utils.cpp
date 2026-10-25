#include "Utils.h"
#include <wx/filename.h>
#include <regex>
#include <sstream>
#include <iomanip>

namespace Utils {

wxString FormatBytes(int64_t bytes) {
    const double kb = 1024.0;
    const double mb = kb * 1024.0;
    const double gb = mb * 1024.0;
    if (bytes < kb) return wxString::Format("%lld B", bytes);
    if (bytes < mb) return wxString::Format("%.2f KB", bytes / kb);
    if (bytes < gb) return wxString::Format("%.2f MB", bytes / mb);
    return wxString::Format("%.2f GB", bytes / gb);
}

wxString FormatSpeed(double bps) {
    if (bps <= 0) return "0 B/s";
    return FormatBytes((int64_t)bps) + "/s";
}

wxString FormatTimeLeft(double seconds) {
    if (seconds <= 0 || seconds > 86400*7) return "--:--:--";
    int h = (int)seconds / 3600;
    int m = ((int)seconds % 3600) / 60;
    int s = (int)seconds % 60;
    return wxString::Format("%02d:%02d:%02d", h, m, s);
}

std::string FileNameFromUrl(const std::string& url) {
    try {
        auto pos = url.find_last_of('/');
        std::string name = (pos == std::string::npos) ? url : url.substr(pos+1);
        auto q = name.find('?');
        if (q != std::string::npos) name = name.substr(0,q);
        if (name.empty()) name = "index.html";
        // Sanitiza
        std::regex bad("[<>:\"/\\\\|?*]");
        name = std::regex_replace(name, bad, "_");
        return name;
    } catch(...) { return "download.bin"; }
}

std::filesystem::path GetDataPath() {
    wxString home = wxStandardPaths::Get().GetUserDataDir(); // %APPDATA%/NetAntsReborn en Win
    std::filesystem::path p = std::string(home.ToStdString());
    return p;
}

void EnsureDir(const std::filesystem::path& p) {
    std::error_code ec;
    std::filesystem::create_directories(p, ec);
}

bool IsValidUrl(const std::string& url) {
    static std::regex re(R"(^(https?|ftp)://[^\s/$.?#].[^\s]*$)", std::regex::icase);
    return std::regex_match(url, re);
}

bool IsFtpUrl(const std::string& url) {
    if (url.size() < 4) return false;
    std::string l = url; std::transform(l.begin(), l.end(), l.begin(), ::tolower);
    return l.rfind("ftp://",0) == 0;
}

} // ns
