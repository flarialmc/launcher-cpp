#include <Windows.h>
#include <d2d1.h>
#include <dwrite.h>
#include <wincodec.h>
#include <string>
#include <filesystem>
#include <Aclapi.h>
#include <Sddl.h>
#include <fstream>
#include <TlHelp32.h>
#include <thread>

#include <discord_rpc.h>

namespace fs = std::filesystem;


HWND hwnd;
HFONT hFont;

ID2D1Factory* pD2DFactory = nullptr;
ID2D1HwndRenderTarget* pRT = nullptr;
ID2D1SolidColorBrush* pBrush = nullptr;
IDWriteFactory* pDWriteFactory = nullptr;

bool isMouseOverButton = false;
int windowWidth = 275;
int windowHeight = 120;

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
void WaitForModules(const std::string& processName, int moduleCount);
void updateStatus();
static int64_t eptime = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count();
std::string currIp = "looool";


DWORD GetProcessIdByName(const std::string& processName) {
    DWORD processId = 0;
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);

    if (snapshot != INVALID_HANDLE_VALUE) {
        PROCESSENTRY32 processEntry;
        processEntry.dwSize = sizeof(PROCESSENTRY32);

        if (Process32First(snapshot, &processEntry)) {
            do {
                std::string currentProcessName = processEntry.szExeFile;
                if (currentProcessName == processName) {
                    processId = processEntry.th32ProcessID;
                    break;
                }
            } while (Process32Next(snapshot, &processEntry));
        }

        CloseHandle(snapshot);
    }

    return processId;
}

void WaitForModules(const std::string& processName, int moduleCount) {

    DWORD processId = GetProcessIdByName(processName);
    if (processId == 0) {
        return;
    }

    HANDLE hProc = OpenProcess(PROCESS_ALL_ACCESS, 0, processId);
    if (hProc == NULL) {
        return;
    }

    bool modulesLoaded = false;
    while (!modulesLoaded) {
        MODULEENTRY32 moduleEntry;
        moduleEntry.dwSize = sizeof(MODULEENTRY32);

        HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, processId);
        if (snapshot != INVALID_HANDLE_VALUE) {
            if (Module32First(snapshot, &moduleEntry)) {
                int count = 0;
                do {
                    count++;
                } while (Module32Next(snapshot, &moduleEntry) && count < moduleCount);
                if (count >= moduleCount) {
                    modulesLoaded = true;
                }
            }
            CloseHandle(snapshot);
        }

        if (!modulesLoaded) {
            Sleep(100);
        }
    }

    CloseHandle(hProc);
}

int performInjection(DWORD procId, std::string dllPath)

{
    HANDLE hProc = OpenProcess(PROCESS_ALL_ACCESS, 0, procId);



    if (hProc && hProc != INVALID_HANDLE_VALUE)
    {
        void* loc = VirtualAllocEx(hProc, 0, MAX_PATH, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);

        WriteProcessMemory(hProc, loc, dllPath.c_str(), dllPath.length() + 1, 0); // length * 2 for bytes + 2 for end string

        HANDLE hThread = CreateRemoteThread(hProc, 0, 0, (LPTHREAD_START_ROUTINE)LoadLibraryA, loc, 0, 0); // using LoadLibraryW instead of LoadLibraryA to allow wchar

        if (hThread)
        {
            CloseHandle(hThread);
        }
    }
    if (hProc)
    {
        CloseHandle(hProc);
    }
    return 0;
}

void SetAccessControl(std::string ExecutableName, const wchar_t* AccessString)
{
    PSECURITY_DESCRIPTOR SecurityDescriptor = nullptr;
    EXPLICIT_ACCESSW ExplicitAccess = { 0 };

    ACL* AccessControlCurrent = nullptr;
    ACL* AccessControlNew = nullptr;

    SECURITY_INFORMATION SecurityInfo = DACL_SECURITY_INFORMATION;
    PSID SecurityIdentifier = nullptr;

    if (
        GetNamedSecurityInfoA(
            ExecutableName.c_str(),
            SE_FILE_OBJECT,
            DACL_SECURITY_INFORMATION,
            nullptr,
            nullptr,
            &AccessControlCurrent,
            nullptr,
            &SecurityDescriptor
        ) == ERROR_SUCCESS
        )
    {
        ConvertStringSidToSidW(AccessString, &SecurityIdentifier);
        if (SecurityIdentifier != nullptr)
        {
            ExplicitAccess.grfAccessPermissions = GENERIC_READ | GENERIC_EXECUTE;
            ExplicitAccess.grfAccessMode = SET_ACCESS;
            ExplicitAccess.grfInheritance = SUB_CONTAINERS_AND_OBJECTS_INHERIT;
            ExplicitAccess.Trustee.TrusteeForm = TRUSTEE_IS_SID;
            ExplicitAccess.Trustee.TrusteeType = TRUSTEE_IS_WELL_KNOWN_GROUP;
            ExplicitAccess.Trustee.ptstrName = reinterpret_cast<wchar_t*>(SecurityIdentifier);

            if (
                SetEntriesInAclW(
                    1,
                    &ExplicitAccess,
                    AccessControlCurrent,
                    &AccessControlNew
                ) == ERROR_SUCCESS
                )
            {
                SetNamedSecurityInfoA(
                        (char*)ExecutableName.c_str(),
                    SE_FILE_OBJECT,
                    SecurityInfo,
                    nullptr,
                    nullptr,
                    AccessControlNew,
                    nullptr
                );
            }
        }
    }
    if (SecurityDescriptor)
    {
        LocalFree(reinterpret_cast<HLOCAL>(SecurityDescriptor));
    }
    if (AccessControlNew)
    {
        LocalFree(reinterpret_cast<HLOCAL>(AccessControlNew));
    }

}

HRESULT LoadBitmapFromFile(ID2D1RenderTarget* pRenderTarget, PCWSTR uri, ID2D1Bitmap** ppBitmap)
{
    IWICImagingFactory* pIWICFactory = nullptr;
    IWICBitmapDecoder* pDecoder = nullptr;
    IWICBitmapFrameDecode* pSource = nullptr;
    IWICStream* pStream = nullptr;
    IWICFormatConverter* pConverter = nullptr;

    HRESULT hr = CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pIWICFactory));
    if (FAILED(hr))
    {
        return hr;
    }

    hr = pIWICFactory->CreateDecoderFromFilename(uri, nullptr, GENERIC_READ, WICDecodeMetadataCacheOnLoad, &pDecoder);
    if (FAILED(hr))
    {
        pIWICFactory->Release();
        return hr;
    }

    hr = pDecoder->GetFrame(0, &pSource);
    if (FAILED(hr))
    {
        pIWICFactory->Release();
        pDecoder->Release();
        return hr;
    }

    hr = pIWICFactory->CreateFormatConverter(&pConverter);
    if (FAILED(hr))
    {
        pIWICFactory->Release();
        pDecoder->Release();
        pSource->Release();
        return hr;
    }

    hr = pConverter->Initialize(pSource, GUID_WICPixelFormat32bppPBGRA, WICBitmapDitherTypeNone, nullptr, 0.0, WICBitmapPaletteTypeMedianCut);
    if (FAILED(hr))
    {
        pIWICFactory->Release();
        pDecoder->Release();
        pSource->Release();
        pConverter->Release();
        return hr;
    }

    hr = pRenderTarget->CreateBitmapFromWicBitmap(pConverter, nullptr, ppBitmap);
    if (FAILED(hr))
    {
        pIWICFactory->Release();
        pDecoder->Release();
        pSource->Release();
        pConverter->Release();
        return hr;
    }

    pIWICFactory->Release();
    pDecoder->Release();
    pSource->Release();
    pConverter->Release();

    return hr;
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow)
{


    WNDCLASS wc = {};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = "Flarial Minimal";
    wc.hbrBackground = CreateSolidBrush(RGB(19, 14, 15));

    RegisterClass(&wc);

    HICON hIcon = LoadIcon(NULL, IDI_APPLICATION);
    SendMessage(hwnd, WM_SETICON, ICON_SMALL, (LPARAM)hIcon);
    SendMessage(hwnd, WM_SETICON, ICON_BIG, (LPARAM)hIcon);

    fs::path exePath(fs::current_path());
    std::string exeDirectory = exePath.string();
    std::string folderPath = exeDirectory + "/Resources";

    if (!fs::exists(folderPath)) {
        fs::create_directory(folderPath);
    }

    std::string logoPath = folderPath + "/logo.png";


    std::string url = "https://cdn-c6f.pages.dev/assets/logo.png";
    URLDownloadToFileA(nullptr, url.c_str(), logoPath.c_str(), 0, nullptr);


    hwnd = CreateWindowExW(0, L"Flarial Minimal", L"Flarial Minimal", WS_OVERLAPPED,
        100, 100, windowWidth, windowHeight, nullptr, nullptr, hInstance, nullptr);

    if (hwnd == nullptr)
    {
        return 0;
    }

    hFont = CreateFontW(16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
        CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Space Grotesk");

    SendMessage(hwnd, WM_SETFONT, (WPARAM)hFont, TRUE);
 

    ShowWindow(hwnd, nCmdShow);

    D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &pD2DFactory);

    RECT rc;
    GetClientRect(hwnd, &rc);
    D2D1_SIZE_U size = D2D1::SizeU(rc.right - rc.left, rc.bottom - rc.top);

    pD2DFactory->CreateHwndRenderTarget(D2D1::RenderTargetProperties(), D2D1::HwndRenderTargetProperties(hwnd, size), &pRT);


    std::thread statusThread([]() {
        while (true) {
            updateStatus();
            Sleep(5000);
        }
        });
    statusThread.detach();

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

std::string readIp() {

    std::string filePath = std::string(getenv("LOCALAPPDATA")) + "\\Packages\\Microsoft.MinecraftUWP_8wekyb3d8bbwe\\RoamingState\\Flarial\\serverip.txt";
    const char* filePathW = filePath.c_str();
    std::ifstream file(filePathW);

    std::string fileContent;

    if (file.is_open()) {

        std::string line;
        while (std::getline(file, line)) {
            fileContent += line + "\n";
        }
    }

    
    file.close();
    file.seekg(0);

    return fileContent;
}

void doStuffWithIp(DiscordRichPresence& presence, const std::string& ip) {

    presence.details = "HACKED LMAOO";
}

void updateStatus() {

        std::string ip = readIp();




        if (currIp != ip) {

            DiscordRichPresence discordPresence;
            DiscordEventHandlers handlers;
            memset(&handlers, 0, sizeof(handlers));
            Discord_Initialize("1067854754518151168", &handlers, 1, nullptr);

            memset(&discordPresence, 0, sizeof(discordPresence));


            discordPresence.largeImageKey = "flarialbig";
            doStuffWithIp(discordPresence, ip);
            discordPresence.startTimestamp = eptime;
            Discord_UpdatePresence(&discordPresence);

            currIp = ip;
        }
    
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

        ID2D1Bitmap* pLogoBitmap = nullptr;
        HRESULT hr = LoadBitmapFromFile(pRT, L"Resources/logo.png", &pLogoBitmap);
        if (SUCCEEDED(hr))
        {

            D2D1_SIZE_F originalLogoSize = pLogoBitmap->GetSize();
            float scaleFactor = 0.45f;
            D2D1_SIZE_F targetLogoSize = D2D1::SizeF(originalLogoSize.width * scaleFactor, originalLogoSize.height * scaleFactor);

            float buttonWidth = 150.0f;
            float buttonHeight = 40.0f;

            float logoY = (windowHeight - targetLogoSize.height) / 2.0f;

            float logoX = 10.0f;
            float buttonX = logoX + 75.0f;
            float buttonY = windowHeight / 2.0f - buttonHeight / 2.0f;

            logoY -= 16.0f;
            logoX -= 22.0f;
            buttonY -= 16.0f;

            pRT->DrawBitmap(pLogoBitmap, D2D1::RectF(logoX, logoY, logoX + targetLogoSize.width, logoY + targetLogoSize.height));

            D2D1_RECT_F buttonRect = D2D1::RectF(buttonX, buttonY, buttonX + buttonWidth, buttonY + buttonHeight);

            if (isMouseOverButton)
            {
                pBrush->SetColor(D2D1::ColorF(255.0f / 255.0f, 35.0f / 255.0f, 55.0f / 255.0f));
            }
            else
            {
                pBrush->SetColor(D2D1::ColorF(42.0f / 255.0f, 33.0f / 255.0f, 34.0f / 255.0f));
            }

            D2D1_ROUNDED_RECT roundedRect = D2D1::RoundedRect(buttonRect, 10.0f, 10.0f);
            pRT->FillRoundedRectangle(roundedRect, pBrush);

            pBrush->SetColor(D2D1::ColorF(D2D1::ColorF::White));
            D2D1_RECT_F textRect = buttonRect;
            DWRITE_TEXT_ALIGNMENT textAlignment = DWRITE_TEXT_ALIGNMENT_CENTER;
            DWRITE_PARAGRAPH_ALIGNMENT paragraphAlignment = DWRITE_PARAGRAPH_ALIGNMENT_CENTER;
            pTextFormat->SetTextAlignment(textAlignment);
            pTextFormat->SetParagraphAlignment(paragraphAlignment);
            pRT->DrawText(L"Launch", 6, pTextFormat, textRect, pBrush);

            pLogoBitmap->Release();
        }

        pTextFormat->Release();

        pRT->EndDraw();

        ValidateRect(hwnd, NULL);
        return 0;
    }
    case WM_MOUSEMOVE:
    {
        int xPos = LOWORD(lParam);
        int yPos = HIWORD(lParam);

        if (xPos >= 80 && xPos <= 230 && yPos >= 20 && yPos <= 75)
        {
            isMouseOverButton = true;
        }
        else
        {
            isMouseOverButton = false;
        }

        InvalidateRect(hwnd, NULL, FALSE);
        return 0;
    }
    case WM_LBUTTONDOWN:
    {
        if (isMouseOverButton)
        {
            if (wParam == MK_LBUTTON)
            {

                std::thread statusThread([&hwnd]() {


                wchar_t currentExePath[MAX_PATH];
                GetModuleFileNameW(nullptr, currentExePath, MAX_PATH);

                fs::path exePath(currentExePath);
                std::string exeDirectory = exePath.parent_path().string();
                std::string latestDllPath = fs::path(exeDirectory).append(L"latest.dll").string();

                std::string url = "https://cdn-c6f.pages.dev/dll/latest.dll";

                HRESULT hr = URLDownloadToFileA(nullptr, url.c_str(), latestDllPath.c_str(), 0, nullptr);
                if (FAILED(hr))
                {
                    MessageBoxW(hwnd, L"Failed to download the DLL.", L"Error", MB_ICONERROR);
                    return 0;
                }


                SetAccessControl(latestDllPath, L"S-1-15-2-1");

                ShellExecuteW(hwnd, L"open", L"minecraft://", nullptr, nullptr, SW_SHOWNORMAL);



                WaitForModules("Minecraft.Windows.exe", 160);
                performInjection(GetProcessIdByName("Minecraft.Windows.exe"), fs::path(exeDirectory).append("latest.dll").string());

                });

                statusThread.detach();
            }
        }
        return 0;
    }
    case WM_SETCURSOR:
    {
        if (isMouseOverButton)
        {
            SetCursor(LoadCursor(nullptr, IDC_HAND));
            return TRUE;
        }
        else
        {
            SetCursor(LoadCursor(nullptr, IDC_ARROW));
            return TRUE;
        }

        break;
    }
    case WM_DESTROY:
    {
        PostQuitMessage(0);
        return 0;
    }
    }

    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}