#include "FileSystem.h"
#include <Windows.h>
#include <filesystem>
#include <algorithm>
#include "StringUtility.h"

namespace fs = std::filesystem;

std::string FileSystem::projectRoot_ = "";
std::string FileSystem::engineRoot_ = "";
std::string FileSystem::exePath_ = "";

void FileSystem::Initialize() {
    wchar_t path[MAX_PATH];
    GetModuleFileNameW(NULL, path, MAX_PATH);
    fs::path exeFsPath = path;
    exePath_ = ConvertString(exeFsPath.wstring());
    fs::path exeDir = exeFsPath.parent_path();

    // 1. Traverse up to find 'project' dir (Development Environment)
    fs::path current = exeDir;
    while (current.has_parent_path() && current != current.parent_path()) {
        if (fs::exists(current / "project" / "Application_solo")) {
            projectRoot_ = ConvertString((current / "project" / "Application_solo").wstring());
            engineRoot_ = ConvertString((current / "project" / "IrufemiEngine").wstring());
            break;
        }
        if (fs::exists(current / "project" / "Application_team")) {
            projectRoot_ = ConvertString((current / "project" / "Application_team").wstring());
            engineRoot_ = ConvertString((current / "project" / "IrufemiEngine").wstring());
            break;
        }
        // fallback if it's already inside project
        if (current.filename() == "Application_solo" || current.filename() == "Application_team") {
            projectRoot_ = ConvertString(current.wstring());
            engineRoot_ = ConvertString((current.parent_path() / "IrufemiEngine").wstring());
            break;
        }
        current = current.parent_path();
    }

    // 2. If not found, assume distributed build
    if (projectRoot_.empty()) {
        projectRoot_ = ConvertString(exeDir.wstring());
        engineRoot_ = ConvertString(exeDir.wstring());
    }

    // Convert backslashes to forward slashes for consistency
    std::replace(projectRoot_.begin(), projectRoot_.end(), '\\', '/');
    std::replace(engineRoot_.begin(), engineRoot_.end(), '\\', '/');
}

std::string FileSystem::GetProjectRoot() {
    return projectRoot_;
}

std::string FileSystem::GetEngineRoot() {
    return engineRoot_;
}

std::string FileSystem::GetResourcePath(const std::string& relativePath) {
    std::string path = relativePath;
    // Remove leading "./" or "/" if present
    if (path.find("./") == 0) path = path.substr(2);
    if (path.find("/") == 0) path = path.substr(1);

    if (path.find("resources") != 0) {
        path = "resources/" + path;
    }

    std::string fullPath = projectRoot_ + "/" + path;
    std::replace(fullPath.begin(), fullPath.end(), '\\', '/');
    return fullPath;
}

std::string FileSystem::GetLogPath() {
    std::string path = projectRoot_ + "/Logs";
    std::replace(path.begin(), path.end(), '\\', '/');
    return path;
}

std::string FileSystem::GetDumpPath() {
    std::string path = projectRoot_ + "/Dumps";
    std::replace(path.begin(), path.end(), '\\', '/');
    return path;
}

std::string FileSystem::GetExePath() {
    return exePath_;
}
