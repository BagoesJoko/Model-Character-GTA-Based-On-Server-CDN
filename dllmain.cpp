#include "pch.h"

void Log(const std::string& msg) {
    std::ofstream log("modloader\\moddownloader\\debug.log", std::ios::app);
    log << msg << std::endl;
}

bool DownloadFile(const std::wstring& url, const std::wstring& path) {
    // Hapus file lama biar tidak corrupt/duplicate
    if (PathFileExistsW(path.c_str())) {
        DeleteFileW(path.c_str());
    }

    HRESULT hr = URLDownloadToFileW(nullptr, url.c_str(), path.c_str(), 0, nullptr);
    if (FAILED(hr)) {
        std::wstring wmsg = L"Download gagal: " + url;
        MessageBoxW(nullptr, wmsg.c_str(), L"Error", MB_OK | MB_ICONERROR);

        std::string msg(url.begin(), url.end());
        Log("FAILED -> " + msg);
        return false;
    }

    std::string msg(url.begin(), url.end());
    Log("SUCCESS -> " + msg);
    return true;
}

DWORD WINAPI MainThread(LPVOID lpParam) {
    std::wstring folder = L"modloader\\moddownloader";

    // bikin folder
    std::filesystem::create_directories(folder);

    // 1. Download manifest.json
    std::wstring manifestPath = folder + L"\\manifest.json";
    DownloadFile(
        L"https://raw.githubusercontent.com/BagoesJoko/modskindev/main/manifest.json",
        manifestPath
    );

    // 2. Parse manifest.json
    std::ifstream f(manifestPath);
    if (!f.is_open()) {
        Log("Manifest tidak bisa dibuka!");
        return 0;
    }

    json manifest;
    try {
        f >> manifest;
    }
    catch (std::exception& e) {
        Log(std::string("JSON parse error: ") + e.what());
        return 0;
    }
    f.close();

    // 3. Loop file list
    for (auto& file : manifest["files"]) {
        std::string name = file["name"];
        std::string url = file["url"];

        std::wstring wname(name.begin(), name.end());
        std::wstring wurl(url.begin(), url.end());

        std::wstring savePath = folder + L"\\" + wname;

        DownloadFile(wurl, savePath);
    }

    // 4. Hapus manifest setelah selesai
    DeleteFileW(manifestPath.c_str());

    return 0;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
    if (ul_reason_for_call == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(hModule); // biar ringan
        CreateThread(nullptr, 0, MainThread, nullptr, 0, nullptr);
    }
    return TRUE;
}