#include <Windows.h>
#include <d2d1.h>
#include <dwrite.h>
#include <wincodec.h>
#include <iostream>
#include <shlobj.h>
#include <Shlwapi.h>
#include <codecvt>
#include "elzip.hpp"

HWND hwnd;
HFONT hFont;

ID2D1Factory* pD2DFactory = nullptr;
ID2D1HwndRenderTarget* pRT = nullptr;
ID2D1SolidColorBrush* pBrush = nullptr;
IDWriteFactory* pDWriteFactory = nullptr;

int windowWidth = 275;
int windowHeight = 120;
int progress = 0;

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

class MyBindStatusCallback : public IBindStatusCallback {
public:
    static inline const wchar_t* status;
    // Constructor
    MyBindStatusCallback(HWND hwnd) : hwnd(hwnd) {}

    // IUnknown methods (not fully implemented for brevity)
    STDMETHODIMP QueryInterface(REFIID riid, void** ppvObject) {
        if (riid == IID_IUnknown || riid == IID_IBindStatusCallback) {
            *ppvObject = static_cast<IBindStatusCallback*>(this);
            return S_OK;
        }
        return E_NOINTERFACE;
    }

    STDMETHODIMP_(ULONG) AddRef() { return 1; }
    STDMETHODIMP_(ULONG) Release() { return 1; }

    // IBindStatusCallback methods
    STDMETHODIMP OnStartBinding(DWORD dwReserved, IBinding* pib) { return E_NOTIMPL; }
    STDMETHODIMP GetPriority(LONG* pnPriority) { return E_NOTIMPL; }
    STDMETHODIMP OnLowResource(DWORD reserved) { return E_NOTIMPL; }
    STDMETHODIMP OnProgress(ULONG ulProgress, ULONG ulProgressMax, ULONG ulStatusCode, LPCWSTR szStatusText) {

        if(ulProgressMax != 0) {

            // Calculate download percentage
            int progress = (ulProgress * 100) / ulProgressMax;

            std::string str = "Downloading Files... This might take some time. " + std::to_string(progress) + "%";
            std::wstring wstr(str.begin(), str.end());

            // Update status text and redraw the window
            status = wstr.c_str();
            RECT rect;
            GetClientRect(hwnd, &rect);
            RedrawWindow(hwnd, &rect, NULL, RDW_INVALIDATE | RDW_ERASE);

            // Print the download percentage
            std::cout << "Download progress: " << progress << "%" << std::endl;

        }

        return S_OK;
    }

    STDMETHODIMP OnStopBinding(HRESULT hresult, LPCWSTR szError) { return E_NOTIMPL; }
    STDMETHODIMP GetBindInfo(DWORD* grfBINDF, BINDINFO* pbindinfo) { return E_NOTIMPL; }
    STDMETHODIMP OnDataAvailable(DWORD grfBSCF, DWORD dwSize, FORMATETC* pformatetc, STGMEDIUM* pstgmed) { return E_NOTIMPL; }
    STDMETHODIMP OnObjectAvailable(REFIID riid, IUnknown* punk) { return E_NOTIMPL; }

private:
    HWND hwnd;
};


void install() {



    std::string url = "https://github.com/flarialmc/newcdn/releases/download/amazing/latest.zip";
    std::string zipPath;
    std::string extractPath;

    char* expandedPath = nullptr;
    size_t requiredSize;
    _dupenv_s(&expandedPath, &requiredSize, "APPDATA");

    zipPath = std::string(expandedPath) + "/Flarial/latest.zip";
    extractPath = std::string(expandedPath) + "\\Flarial";

    free(expandedPath);

    std::filesystem::path directoryPath = extractPath;

    if (std::filesystem::exists(directoryPath)) {
        std::filesystem::remove_all(directoryPath);
    }
    std::filesystem::create_directory(directoryPath);

    // Download the 7z file
    std::string str = "Downloading Files... This might take some time. " + std::to_string(progress) + "%";
    std::wstring wstr(str.begin(), str.end());

    MyBindStatusCallback::status = wstr.c_str();

    RECT rect;
    GetClientRect(hwnd, &rect);

    std::cout << "Downlopading" << std::endl;

    std::thread statusThread([]() {

        while(true) {

            RECT rect;
            GetClientRect(hwnd, &rect);
            RedrawWindow(hwnd, &rect, NULL, RDW_INVALIDATE | RDW_ERASE);
            Sleep(1000);

        }

    });
    statusThread.detach();

    auto* pBindStatusCallback = new MyBindStatusCallback(GetDesktopWindow());

    HRESULT hr = URLDownloadToFileA(NULL, url.c_str(), zipPath.c_str(), 0,
                                    pBindStatusCallback);



    if (FAILED(hr)) {
        MyBindStatusCallback::status = L"Failed to download the files.";
        RedrawWindow(hwnd, &rect, NULL, RDW_INVALIDATE | RDW_ERASE);
        std::cout << "fail" << std::endl;
        return;
    }

    MyBindStatusCallback::status = L"Extracting Zip Files";
    std::cout << "extracting" << std::endl;
    RedrawWindow(hwnd, &rect, NULL, RDW_INVALIDATE | RDW_ERASE);
    elz::extractZip(zipPath, extractPath);

    std::filesystem::remove(zipPath);

    MyBindStatusCallback::status = L"Done! Closing..";
    std::cout << "Done" << std::endl;
    RedrawWindow(hwnd, &rect, NULL, RDW_INVALIDATE | RDW_ERASE);

    ShellExecuteA(NULL, "open", extractPath.c_str(), NULL, NULL, SW_SHOWDEFAULT);

    // Create Start Menu shortcut
    std::string targetFilePath = extractPath + "\\Flarial.Launcher.exe";
    std::string shortcutFolderPath = "C:\\ProgramData\\Microsoft\\Windows\\Start Menu\\";
    std::string shortcutFileName = "Flarial.lnk";

    std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
    std::wstring wShortcutFolderPath = converter.from_bytes(shortcutFolderPath);
    std::wstring wShortcutFileName = converter.from_bytes(shortcutFileName);
    WCHAR shortcutPath[MAX_PATH];
    PathCombineW(shortcutPath, wShortcutFolderPath.c_str(), wShortcutFileName.c_str());

    CoInitialize(NULL);
    IShellLink* pShellLink;
    CoCreateInstance(CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER, IID_IShellLink, (LPVOID*)&pShellLink);


    pShellLink->SetPath(targetFilePath.c_str());

    IPersistFile* pPersistFile;
    pShellLink->QueryInterface(IID_IPersistFile, (LPVOID*)&pPersistFile);
    pPersistFile->Save(shortcutPath, TRUE);

    pPersistFile->Release();
    pShellLink->Release();
    CoUninitialize();

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



        pRT->DrawText(MyBindStatusCallback::status, wcsnlen_s(MyBindStatusCallback::status, 200), pTextFormat, textRect, pBrush);

        pTextFormat->Release();

        pRT->EndDraw();

        ValidateRect(hwnd, NULL);

        if(MyBindStatusCallback::status == L"Done! Closing..") {
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