#include <Windows.h>
#include <d2d1.h>
#include <dwrite.h>
#include <wincodec.h>
#include <iostream>
#include <fstream>
#include <windows.h>
#include <shlobj.h>
#include "elzip.hpp"

HWND hwnd;
HFONT hFont;

ID2D1Factory* pD2DFactory = nullptr;
ID2D1HwndRenderTarget* pRT = nullptr;
ID2D1SolidColorBrush* pBrush = nullptr;
IDWriteFactory* pDWriteFactory = nullptr;

int windowWidth = 275;
int windowHeight = 120;

const wchar_t *status;

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

void install() {



    const std::string url = "https://cdn.flarial.net/launcher/latest.zip";
    std::string zipPath;
    std::string extractPath;

    const char* expandedPath = std::getenv("APPDATA");
    zipPath = std::string(expandedPath) + "/Flarial/latest.zip";
    extractPath = std::string(expandedPath) + "/Flarial";

    std::filesystem::path directoryPath = extractPath;

    if (std::filesystem::exists(directoryPath)) {
        std::filesystem::remove_all(directoryPath);
    }
    std::filesystem::create_directory(directoryPath);

    // Download the 7z file
    status = L"Downloading Files... This might take some time.";
    RECT rect;
    GetClientRect(hwnd, &rect);
    RedrawWindow(hwnd, &rect, NULL, RDW_INVALIDATE | RDW_ERASE);

    std::cout << "Downlopading" << std::endl;

    HRESULT hr = URLDownloadToFileA(NULL, url.c_str(), zipPath.c_str(), 0, NULL);
    if (FAILED(hr)) {
        status = L"Failed to download the files.";
        RedrawWindow(hwnd, &rect, NULL, RDW_INVALIDATE | RDW_ERASE);
        std::cout << "fail" << std::endl;
        return;
    }

    status = L"Extracting Zip Files";
    std::cout << "extracting" << std::endl;
    RedrawWindow(hwnd, &rect, NULL, RDW_INVALIDATE | RDW_ERASE);
    elz::extractZip(zipPath, extractPath);

    std::filesystem::remove(zipPath);

    status = L"Done! Closing..";
    std::cout << "Done" << std::endl;
    RedrawWindow(hwnd, &rect, NULL, RDW_INVALIDATE | RDW_ERASE);

    std::string allahPath = extractPath + "/Flarial.Launcher.exe";
    ShellExecuteA(NULL, "open", extractPath.c_str(), NULL, NULL, SW_SHOWDEFAULT);
    ShellExecuteA(NULL, "open", allahPath.c_str(), NULL, NULL, SW_SHOWDEFAULT);

    // Create Start Menu shortcut
    std::string shortcutPath;
    PWSTR appDataPath;
    if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_RoamingAppData, 0, NULL, &appDataPath))) {
        std::wstring wAppDataPath(appDataPath);
        shortcutPath = std::string(wAppDataPath.begin(), wAppDataPath.end()) + "\\Microsoft\\Windows\\Start Menu\\Programs\\Flarial.Lnk";
        CoTaskMemFree(appDataPath);
    }
    if (!shortcutPath.empty()) {
        CoInitialize(NULL);
        IShellLink* shellLink;
        HRESULT hr = CoCreateInstance(CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER, IID_IShellLink, (LPVOID*)&shellLink);
        if (SUCCEEDED(hr)) {
            shellLink->SetPath((extractPath + "\\Flarial.Launcher.exe").c_str());
            shellLink->SetWorkingDirectory(extractPath.c_str());
            IPersistFile* persistFile;
            hr = shellLink->QueryInterface(IID_IPersistFile, (LPVOID*)&persistFile);
            if (SUCCEEDED(hr)) {
                std::wstring wideExePath1(shortcutPath.begin(), shortcutPath.end());
                LPCOLESTR oleExePat1 = wideExePath1.c_str();
                persistFile->Save(oleExePat1, TRUE);
                persistFile->Release();
            }
            shellLink->Release();
        }
        CoUninitialize();
    }

    if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_RoamingAppData, 0, NULL, &appDataPath))) {
        std::wstring wAppDataPath(appDataPath);
        shortcutPath = std::string(wAppDataPath.begin(), wAppDataPath.end()) + "\\Microsoft\\Windows\\Start Menu\\Programs\\Flarial Minimal.Lnk";
        CoTaskMemFree(appDataPath);
    }
    if (!shortcutPath.empty()) {
        CoInitialize(NULL);
        IShellLink* shellLink;
        HRESULT hr = CoCreateInstance(CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER, IID_IShellLink, (LPVOID*)&shellLink);
        if (SUCCEEDED(hr)) {
            shellLink->SetPath((extractPath + "\\Flarial.Minimal.exe").c_str());
            shellLink->SetWorkingDirectory(extractPath.c_str());
            IPersistFile* persistFile;
            hr = shellLink->QueryInterface(IID_IPersistFile, (LPVOID*)&persistFile);
            if (SUCCEEDED(hr)) {
                std::wstring wideExePath1(shortcutPath.begin(), shortcutPath.end());
                LPCOLESTR oleExePat1 = wideExePath1.c_str();
                persistFile->Save(oleExePat1, TRUE);
                persistFile->Release();
            }
            shellLink->Release();
        }
        CoUninitialize();
    }
}


int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow)
{


    WNDCLASS wc = {};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = "Flarial Installer";
    wc.hbrBackground = CreateSolidBrush(RGB(19, 14, 15));

    RegisterClass(&wc);

    HICON hIcon = LoadIcon(NULL, IDI_APPLICATION);
    SendMessage(hwnd, WM_SETICON, ICON_SMALL, (LPARAM)hIcon);
    SendMessage(hwnd, WM_SETICON, ICON_BIG, (LPARAM)hIcon);


    hwnd = CreateWindowExW(0, L"Flarial Installer", L"Flarial Installer", WS_OVERLAPPED,
        100, 100, windowWidth, windowHeight, nullptr, nullptr, hInstance, nullptr);

    if (hwnd == nullptr)
    {
        return 0;
    }

    hFont = CreateFontW(16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
        CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Space Grotesk");

    SendMessage(hwnd, WM_SETFONT, (WPARAM)hFont, TRUE);

    std::thread statusThread([]() {
        install();
    });
    statusThread.detach();

    ShowWindow(hwnd, nCmdShow);

    D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &pD2DFactory);

    RECT rc;
    GetClientRect(hwnd, &rc);
    D2D1_SIZE_U size = D2D1::SizeU(rc.right - rc.left, rc.bottom - rc.top);

    pD2DFactory->CreateHwndRenderTarget(D2D1::RenderTargetProperties(), D2D1::HwndRenderTargetProperties(hwnd, size), &pRT);

    MSG msg;
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }



    if (hFont)
    {
        DeleteObject(hFont);
    }

    if (pBrush)
    {
        pBrush->Release();
    }

    if (pDWriteFactory)
    {
        pDWriteFactory->Release();
    }

    if (pRT)
    {
        pRT->Release();
    }

    if (pD2DFactory)
    {
        pD2DFactory->Release();
    }

    return 0;
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_PAINT:
    {
        pRT->BeginDraw();
        pRT->Clear(D2D1::ColorF(19.0f / 255.0f, 14.0f / 255.0f, 15.0f / 255.0f));

        if (!pBrush)
        {
            pRT->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::White), &pBrush);
        }

        DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory), reinterpret_cast<IUnknown**>(&pDWriteFactory));

        IDWriteTextFormat* pTextFormat;
        pDWriteFactory->CreateTextFormat(L"Space Grotesk", nullptr, DWRITE_FONT_WEIGHT_NORMAL,
            DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL, 16, L"en-us", &pTextFormat);

            float width = 200.0f;
            float height = 40.0f;

            float x = 25.0f;
            float y = 15.0f;
            D2D1_RECT_F textRect = D2D1::RectF(x, y, x + width, y + height);

            pBrush->SetColor(D2D1::ColorF(D2D1::ColorF::White));
            DWRITE_TEXT_ALIGNMENT textAlignment = DWRITE_TEXT_ALIGNMENT_CENTER;
            DWRITE_PARAGRAPH_ALIGNMENT paragraphAlignment = DWRITE_PARAGRAPH_ALIGNMENT_CENTER;
            pTextFormat->SetTextAlignment(textAlignment);
            pTextFormat->SetParagraphAlignment(paragraphAlignment);



        pRT->DrawText(status, wcsnlen_s(status, 200), pTextFormat, textRect, pBrush);

        pTextFormat->Release();

        pRT->EndDraw();

        ValidateRect(hwnd, NULL);

        if(status == L"Done! Closing..") {
            Sleep(1200);
            DestroyWindow(hwnd);
        }

        return 0;
    }
    case WM_DESTROY:
    {
        PostQuitMessage(0);
        return 0;
    }
    }

    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}