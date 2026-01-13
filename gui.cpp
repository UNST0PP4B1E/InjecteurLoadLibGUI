#include "injector.h"

#include <Windowsx.h>

#include <psapi.h>
#include <shellapi.h>

#include <d3d11.h>
#include <tchar.h>

#include <WICTextureLoader.h>

#include <imgui.h>
#include <imgui_impl_dx11.h>
#include <imgui_impl_win32.h>

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "DirectXTK.lib")


static ID3D11Device* g_pd3dDevice = nullptr;
static ID3D11DeviceContext* g_pd3dDeviceContext = nullptr;
static IDXGISwapChain* g_pSwapChain = nullptr;
static bool                     g_SwapChainOccluded = false;
static UINT                     g_ResizeWidth = 0, g_ResizeHeight = 0;
static ID3D11RenderTargetView* g_mainRenderTargetView = nullptr;

// Forward declarations of helper functions
bool CreateDeviceD3D(HWND hWnd);
void CleanupDeviceD3D();
void CreateRenderTarget();
void CleanupRenderTarget();

LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

ID3D11ShaderResourceView* LoadTextureFromFileDX11(const wchar_t* filename)
{
    ID3D11ShaderResourceView* texture = nullptr;
    HRESULT hr = DirectX::CreateWICTextureFromFile(g_pd3dDevice, g_pd3dDeviceContext, filename, nullptr, &texture);

    if (FAILED(hr))
    {
        MessageBoxW(0, L"Erreur lors du chargement de l'image", filename, MB_OK);
        return nullptr;
    }
    return texture;
}

void SetupImGuiStyle()
{
    // Hazy Dark style by kaitabuchi314 from ImThemes
    ImGuiStyle& style = ImGui::GetStyle();

    style.Alpha = 1.0f;
    style.DisabledAlpha = 0.6f;
    style.WindowPadding = ImVec2(5.5f, 8.3f);
    style.WindowRounding = 20.0f;
    style.WindowBorderSize = 3.0f;
    style.WindowMinSize = ImVec2(32.0f, 32.0f);
    style.WindowTitleAlign = ImVec2(0.0f, 0.5f);
    style.WindowMenuButtonPosition = ImGuiDir_Left;
    style.ChildRounding = 3.2f;
    style.ChildBorderSize = 1.0f;
    style.PopupRounding = 2.7f;
    style.PopupBorderSize = 1.0f;
    style.FramePadding = ImVec2(4.0f, 3.0f);
    style.FrameRounding = 5.0f;
    style.FrameBorderSize = 0.0f;
    style.ItemSpacing = ImVec2(8.0f, 4.0f);
    style.ItemInnerSpacing = ImVec2(12.0f, 12.0f);
    style.CellPadding = ImVec2(4.0f, 2.0f);
    style.IndentSpacing = 21.0f;
    style.ColumnsMinSpacing = 6.0f;
    style.ScrollbarSize = 14.0f;
    style.ScrollbarRounding = 9.0f;
    style.GrabMinSize = 10.0f;
    style.GrabRounding = 3.2f;
    style.TabRounding = 3.5f;
    style.TabBorderSize = 3.0f;
    style.ColorButtonPosition = ImGuiDir_Right;
    style.ButtonTextAlign = ImVec2(0.5f, 0.5f);
    style.SelectableTextAlign = ImVec2(0.0f, 0.0f);

    style.Colors[ImGuiCol_Text] = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
    style.Colors[ImGuiCol_TextDisabled] = ImVec4(0.49803922f, 0.49803922f, 0.49803922f, 1.0f);
    style.Colors[ImGuiCol_WindowBg] = ImVec4(31.0f / 255, 30.0f / 255, 30.0f / 255, 0.94f);
    style.Colors[ImGuiCol_ChildBg] = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
    style.Colors[ImGuiCol_PopupBg] = ImVec4(0.078431375f, 0.078431375f, 0.078431375f, 0.94f);
    style.Colors[ImGuiCol_Border] = ImVec4(97.0f / 255, 23.0f / 255, 23.0f / 255, 0.94f);
    style.Colors[ImGuiCol_BorderShadow] = ImVec4(31.0f / 255, 30.0f / 255, 30.0f / 255, 0.94f);
    style.Colors[ImGuiCol_FrameBg] = ImVec4(0.13725491f, 0.17254902f, 0.22745098f, 0.54f);
    style.Colors[ImGuiCol_FrameBgHovered] = ImVec4(0.21176471f, 0.25490198f, 0.3019608f, 0.4f);
    style.Colors[ImGuiCol_FrameBgActive] = ImVec4(0.043137256f, 0.047058824f, 0.047058824f, 0.67f);
    style.Colors[ImGuiCol_TitleBg] = ImVec4(31.0f / 255, 30.0f / 255, 30.0f / 255, 0.94f);
    style.Colors[ImGuiCol_TitleBgActive] = ImVec4(0.078431375f, 0.08235294f, 0.09019608f, 1.0f);
    style.Colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.0f, 0.0f, 0.0f, 0.51f);
    style.Colors[ImGuiCol_MenuBarBg] = ImVec4(31.0f / 255, 30.0f / 255, 30.0f / 255, 0.94f);
    style.Colors[ImGuiCol_ScrollbarBg] = ImVec4(0.019607844f, 0.019607844f, 0.019607844f, 0.53f);
    style.Colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.30980393f, 0.30980393f, 0.30980393f, 1.0f);
    style.Colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.40784314f, 0.40784314f, 0.40784314f, 1.0f);
    style.Colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.50980395f, 0.50980395f, 0.50980395f, 1.0f);
    style.Colors[ImGuiCol_CheckMark] = ImVec4(0.7176471f, 0.78431374f, 0.84313726f, 1.0f);
    style.Colors[ImGuiCol_SliderGrab] = ImVec4(0.47843137f, 0.5254902f, 0.57254905f, 1.0f);
    style.Colors[ImGuiCol_SliderGrabActive] = ImVec4(0.2901961f, 0.31764707f, 0.3529412f, 1.0f);
    style.Colors[ImGuiCol_Button] = ImVec4(158.0f / 255, 19.0f / 255, 42.0f / 255, 0.4f);
    style.Colors[ImGuiCol_ButtonHovered] = ImVec4(102.0f / 255, 15.0f / 255, 30.0f / 255, 1.0f);
    style.Colors[ImGuiCol_ButtonActive] = ImVec4(0.078431375f, 0.08627451f, 0.09019608f, 1.0f);
    style.Colors[ImGuiCol_Header] = ImVec4(31.0f / 255, 30.0f / 255, 30.0f / 255, 0.94f);
    style.Colors[ImGuiCol_HeaderHovered] = ImVec4(110.0f / 255, 110.0f / 255, 110.0f / 255, 1);
    style.Colors[ImGuiCol_HeaderActive] = ImVec4(31.0f / 255, 30.0f / 255, 30.0f / 255, 0.94f);
    style.Colors[ImGuiCol_Separator] = ImVec4(0.42745098f, 0.42745098f, 0.49803922f, 0.5f);
    style.Colors[ImGuiCol_SeparatorHovered] = ImVec4(0.23921569f, 0.3254902f, 0.42352942f, 0.78f);
    style.Colors[ImGuiCol_SeparatorActive] = ImVec4(0.27450982f, 0.38039216f, 0.49803922f, 1.0f);
    style.Colors[ImGuiCol_ResizeGrip] = ImVec4(0.2901961f, 0.32941177f, 0.3764706f, 0.2f);
    style.Colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.23921569f, 0.29803923f, 0.36862746f, 0.67f);
    style.Colors[ImGuiCol_ResizeGripActive] = ImVec4(0.16470589f, 0.1764706f, 0.1882353f, 0.95f);
    style.Colors[ImGuiCol_Tab] = ImVec4(0.11764706f, 0.1254902f, 0.13333334f, 0.862f);
    style.Colors[ImGuiCol_TabHovered] = ImVec4(0.32941177f, 0.40784314f, 0.5019608f, 0.8f);
    style.Colors[ImGuiCol_TabActive] = ImVec4(0.24313726f, 0.24705882f, 0.25490198f, 1.0f);
    style.Colors[ImGuiCol_TabUnfocused] = ImVec4(0.06666667f, 0.101960786f, 0.14509805f, 0.9724f);
    style.Colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.13333334f, 0.25882354f, 0.42352942f, 1.0f);
    style.Colors[ImGuiCol_PlotLines] = ImVec4(0.60784316f, 0.60784316f, 0.60784316f, 1.0f);
    style.Colors[ImGuiCol_PlotLinesHovered] = ImVec4(1.0f, 0.42745098f, 0.34901962f, 1.0f);
    style.Colors[ImGuiCol_PlotHistogram] = ImVec4(0.8980392f, 0.69803923f, 0.0f, 1.0f);
    style.Colors[ImGuiCol_PlotHistogramHovered] = ImVec4(1.0f, 0.6f, 0.0f, 1.0f);
    style.Colors[ImGuiCol_TableHeaderBg] = ImVec4(0.1882353f, 0.1882353f, 0.2f, 1.0f);
    style.Colors[ImGuiCol_TableBorderStrong] = ImVec4(0.30980393f, 0.30980393f, 0.34901962f, 1.0f);
    style.Colors[ImGuiCol_TableBorderLight] = ImVec4(0.22745098f, 0.22745098f, 0.24705882f, 1.0f);
    style.Colors[ImGuiCol_TableRowBg] = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
    style.Colors[ImGuiCol_TableRowBgAlt] = ImVec4(1.0f, 1.0f, 1.0f, 0.06f);
    style.Colors[ImGuiCol_TextSelectedBg] = ImVec4(0.25882354f, 0.5882353f, 0.9764706f, 0.35f);
    style.Colors[ImGuiCol_DragDropTarget] = ImVec4(1.0f, 1.0f, 0.0f, 0.9f);
    style.Colors[ImGuiCol_NavHighlight] = ImVec4(0.25882354f, 0.5882353f, 0.9764706f, 1.0f);
    style.Colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.0f, 1.0f, 1.0f, 0.7f);
    style.Colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.8f, 0.8f, 0.8f, 0.2f);
    style.Colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.8f, 0.8f, 0.8f, 0.35f);
}

// Main code
int WINAPI WinMain(
    HINSTANCE hInstance,
    HINSTANCE hPrevInstance,
    LPSTR     lpCmdLine,
    int       nShowCmd
)
{
    // Make process DPI aware and obtain main monitor scale
    ImGui_ImplWin32_EnableDpiAwareness();
    float main_scale = ImGui_ImplWin32_GetDpiScaleForMonitor(::MonitorFromPoint(POINT{ 0, 0 }, MONITOR_DEFAULTTOPRIMARY));

    WNDCLASSEXW wc = { 
        sizeof(wc),
        CS_CLASSDC,
        WndProc,
        0L, 0L,
        GetModuleHandle(nullptr),
        nullptr, nullptr, nullptr, nullptr,
        L"ImGui Example",
        nullptr 
    };

    ::RegisterClassExW(&wc);
    HWND hwnd = ::CreateWindowEx(0, wc.lpszClassName, L"", WS_POPUP, 100, 100, 575, 645, nullptr, nullptr, wc.hInstance, nullptr);
    SetLayeredWindowAttributes(hwnd, RGB(0, 0, 0), 0, ULW_COLORKEY);


    HRGN hRgn = CreateRoundRectRgn(0, 0, 575, 645, 30, 30);
    SetWindowRgn(hwnd, hRgn, TRUE);

    // Initialize Direct3D
    if (!CreateDeviceD3D(hwnd))
    {
        CleanupDeviceD3D();
        ::UnregisterClassW(wc.lpszClassName, wc.hInstance);
        return 1;
    }

    // Show the window
    ::ShowWindow(hwnd, SW_SHOWDEFAULT);
    ::UpdateWindow(hwnd);

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    //ImGui::StyleColorsLight();

    // Setup scaling
    ImGuiStyle& style = ImGui::GetStyle();
    style.ScaleAllSizes(main_scale);        // Bake a fixed style scale. (until we have a solution for dynamic style scaling, changing this requires resetting Style + calling this again)
    style.FontScaleDpi = main_scale;        // Set initial font scale. (using io.ConfigDpiScaleFonts=true makes this unnecessary. We leave both here for documentation purpose)

    // Setup Platform/Renderer backends
    ImGui_ImplWin32_Init(hwnd);
    ImGui_ImplDX11_Init(g_pd3dDevice, g_pd3dDeviceContext);

    ID3D11ShaderResourceView* ExitImageTexture = LoadTextureFromFileDX11(L"assets/image1.png");
    ID3D11ShaderResourceView* SuccesImageTexture = LoadTextureFromFileDX11(L"assets/succes.png");
    ID3D11ShaderResourceView* DiscordImageTexture = LoadTextureFromFileDX11(L"assets/discord.png");
    ID3D11ShaderResourceView* InjectMeImageTexture = LoadTextureFromFileDX11(L"assets/InjectMe.png");
    ID3D11ShaderResourceView* ProcessOnImageTexture = LoadTextureFromFileDX11(L"assets/ProcessOn.png");
    ID3D11ShaderResourceView* FileOnImageTexture = LoadTextureFromFileDX11(L"assets/FileOn.png");
    ID3D11ShaderResourceView* SelectImageTexture = LoadTextureFromFileDX11(L"assets/Select.png");
    ID3D11ShaderResourceView* InjecterImageTexture = LoadTextureFromFileDX11(L"assets/Injecter.png");

    ImGuiWindowFlags imguiFlag = ImGuiWindowFlags_NoCollapse
        | ImGuiWindowFlags_NoResize
        | ImGuiWindowFlags_NoCollapse
        | ImGuiWindowFlags_NoTitleBar
        | ImGuiWindowFlags_NoMove;

    bool Tab1 = true;
    bool Tab2 = false;

    OPENFILENAMEA OpenFile;
    char fileBuffer[MAX_PATH] = { 0 };
    ZeroMemory(&OpenFile, sizeof(OpenFile));
    OpenFile.lStructSize = sizeof(OPENFILENAMEA);
    OpenFile.hwndOwner = hwnd;
    OpenFile.hInstance = hInstance;
    OpenFile.lpstrFilter = "DLL Files (*.dll)\0*.dll\0All Files (*.*)\0*.*\0\0";
    OpenFile.lpstrCustomFilter = nullptr;
    OpenFile.nMaxCustFilter = 0;
    OpenFile.nFilterIndex = 1;
    OpenFile.lpstrFile = fileBuffer;
    OpenFile.nMaxFile = MAX_PATH;
    OpenFile.lpstrFileTitle = nullptr;
    OpenFile.nMaxFileTitle = 0;
    OpenFile.lpstrInitialDir = nullptr;
    OpenFile.lpstrTitle = "Select a DLL";
    OpenFile.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_EXPLORER | OFN_NOCHANGEDIR;

    bool ProcessTabOpen = true;
    bool FileTabOpen = false;

    int searchedCount = 0;
    char buffer[200] = "";
    char ProcessList[1024][MAX_PATH] = { 0 };
    char* ProcessListSearched[1024] = { nullptr };
    char* ProcessSearched = nullptr;
    DWORD ProcIDChoice = NULL;
    int ProcessesListSize = 0;
    DWORD processIDs[1024], bytesReturned;

    if (!EnumProcesses(processIDs, sizeof(processIDs), &bytesReturned)) {
        MessageBoxA(hwnd, "Error EnumProc", "debug", MB_OK);
        return 1;
    }

    int count = bytesReturned / sizeof(DWORD);

    for (int i = 0; i < count; i++) {
        DWORD pid = processIDs[i];
        if (pid == 0) continue;

        HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pid);
        if (!hProcess) continue;

        HMODULE hMod;
        DWORD needed;
        char name[MAX_PATH] = "<inconnu>";

        if (EnumProcessModules(hProcess, &hMod, sizeof(hMod), &needed)) {
            GetModuleBaseNameA(hProcess, hMod, name, MAX_PATH);
        }

        strcpy(ProcessList[i], name);
        //MessageBoxA(hwnd, name, "debug", MB_OK);

        CloseHandle(hProcess);
    }

    // Main loop
    bool done = false;
    while (!done)
    {
        MSG msg;
        while (::PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE))
        {
            ::TranslateMessage(&msg);
            ::DispatchMessage(&msg);
            if (msg.message == WM_QUIT)
                done = true;
        }
        if (done)
            break;

        // Handle window being minimized or screen locked
        if (g_SwapChainOccluded && g_pSwapChain->Present(0, DXGI_PRESENT_TEST) == DXGI_STATUS_OCCLUDED)
        {
            ::Sleep(10);
            continue;
        }
        g_SwapChainOccluded = false;

        // Handle window resize (we don't resize directly in the WM_SIZE handler)
        if (g_ResizeWidth != 0 && g_ResizeHeight != 0)
        {
            CleanupRenderTarget();
            g_pSwapChain->ResizeBuffers(0, g_ResizeWidth, g_ResizeHeight, DXGI_FORMAT_UNKNOWN, 0);
            g_ResizeWidth = g_ResizeHeight = 0;
            CreateRenderTarget();
        }

        // Start the Dear ImGui frame
        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();


        SetupImGuiStyle();
        ImGui::SetNextWindowPos(ImVec2(0, 0));
        ImGui::SetNextWindowSize(ImVec2(575, 645), 0);
        ImGui::Begin("##TestWindows", nullptr, imguiFlag);

        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0, 0, 0, 0));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0, 0, 0, 0));


        ImGui::SetCursorPos(ImVec2(190, 50));
        ImGui::Image(InjecterImageTexture, ImVec2(200, 50));

        ImGui::SetCursorPos(ImVec2(500, 15));
        if (ImGui::ImageButton("ExitButton", ExitImageTexture, ImVec2(50, 50))) {
            break;
        }

        ImGui::SetCursorPos(ImVec2(20, 15));
        if (ImGui::ImageButton("DiscordButton", DiscordImageTexture, ImVec2(58, 59))) {
            ShellExecuteA(NULL, "open", "https://discord.gg/J6BBFFAuhH", nullptr, nullptr, SW_SHOWNORMAL);
        }

        ImGui::SetWindowFontScale(2.0f);
        ImGui::SetCursorPos(ImVec2(130, 150));
        if (ImGui::ImageButton("InjectMeButton", InjectMeImageTexture, ImVec2(300, 132))) {

            if (ProcIDChoice != NULL) {
                if (fileBuffer[0] != 0) {
                    Injecter(ProcIDChoice, fileBuffer);
                }
                else {
                    MessageBoxA(hwnd, "Error : Please select a DLL", "debug", MB_OK);
                }
            }
            else {
                MessageBoxA(hwnd, "Error : Please select a Process", "debug", MB_OK);
            }
        }

        ImGui::SetWindowFontScale(2.0f);
        ImGui::SetCursorPos(ImVec2(10, 350));
        if (ImGui::Button("##Process", ImVec2(170, 40))) {
            ProcessTabOpen = true;
            FileTabOpen = false;
        }
        if (ProcessTabOpen) {
            ImGui::SetCursorPos(ImVec2(10, 350));
            ImGui::Image(ProcessOnImageTexture, ImVec2(550, 272));

            ImGui::SetCursorPos(ImVec2(20, 393));
            ImGui::SetWindowFontScale(1.0f);
            ImGui::InputText("Search", buffer, IM_ARRAYSIZE(buffer));

            ImGui::SetCursorPos(ImVec2(20, 425));
            ImGui::BeginChild("Div1", ImVec2(530, 170), false);
            for (int i = 0; i < count; i++)
            {
                if (!(buffer[0] == '\0')) {
                    ProcessSearched = strstr(ProcessList[i], buffer);
                    if (ProcessSearched != nullptr) {
                        ProcessListSearched[searchedCount] = ProcessSearched;

                        char TempName[100] = { 0 };
                        sprintf(TempName, " %10d : %s", processIDs[i], ProcessListSearched[searchedCount]);

                        bool selected = (ProcIDChoice == i);
                        if (ImGui::Selectable(TempName, selected)) {
                            ProcIDChoice = processIDs[i];
                        }
                        if (selected) {
                            ImGui::SetItemDefaultFocus();
                        }
                        searchedCount++;
                    }
                }
                else {
                    char TempName[100] = { 0 };
                    sprintf(TempName, " %10d  :  %s", processIDs[i], ProcessList[i]);
                    if (strcmp(ProcessList[i], "") != 0) {
                        bool selected = (ProcIDChoice == i);
                        if (ImGui::Selectable(TempName, selected)) {
                            ProcIDChoice = processIDs[i];
                        }
                        if (selected) {
                            ImGui::SetItemDefaultFocus();
                        }
                    }

                }
            }
            ImGui::EndChild();
            searchedCount = 0;
        }


        ImGui::SetCursorPos(ImVec2(185, 350));
        ImGui::SetWindowFontScale(2.0f);
        if (ImGui::Button("##File", ImVec2(170, 40))) {
            ProcessTabOpen = false;
            FileTabOpen = true;
        }
        if (FileTabOpen) {
            ImGui::SetCursorPos(ImVec2(10, 350));
            ImGui::Image(FileOnImageTexture, ImVec2(550, 272));
            ImGui::SetCursorPos(ImVec2(210, 470));
            ImGui::SetWindowFontScale(1.75f);
            ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 12.0f);
            if (fileBuffer[0] != 0) {
                ImGui::SetCursorPos(ImVec2(230, 460));
                ImGui::Image(SuccesImageTexture, ImVec2(100, 100));
            }
            else {
                if (ImGui::ImageButton("Select", SelectImageTexture, ImVec2(150, 75))) {
                    GetOpenFileNameA(&OpenFile);
                }
            }
            ImGui::PopStyleVar();
        }
        ImGui::SetWindowFontScale(1.0f);


        ImGui::PopStyleColor(3);

        ImGui::End();

        // Rendering
        ImGui::Render();
        const float clear_color_with_alpha[4] = { 0.0f, 0.0f , 0.0f , 0.0f };
        g_pd3dDeviceContext->OMSetRenderTargets(1, &g_mainRenderTargetView, nullptr);
        g_pd3dDeviceContext->ClearRenderTargetView(g_mainRenderTargetView, clear_color_with_alpha);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

        // Present
        HRESULT hr = g_pSwapChain->Present(1, 0);   // Present with vsync
        //HRESULT hr = g_pSwapChain->Present(0, 0); // Present without vsync
        g_SwapChainOccluded = (hr == DXGI_STATUS_OCCLUDED);
    }

    // Cleanup
    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    CleanupDeviceD3D();
    ::DestroyWindow(hwnd);
    ::UnregisterClassW(wc.lpszClassName, wc.hInstance);

    return 0;
}

bool CreateDeviceD3D(HWND hWnd)
{
    // Setup swap chain
    DXGI_SWAP_CHAIN_DESC sd;
    ZeroMemory(&sd, sizeof(sd));
    sd.BufferCount = 2;
    sd.BufferDesc.Width = 0;
    sd.BufferDesc.Height = 0;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.RefreshRate.Numerator = 60;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = hWnd;
    sd.SampleDesc.Count = 1;
    sd.SampleDesc.Quality = 0;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    UINT createDeviceFlags = 0;
    //createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
    D3D_FEATURE_LEVEL featureLevel;
    const D3D_FEATURE_LEVEL featureLevelArray[2] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0, };
    HRESULT res = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, createDeviceFlags, featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext);
    if (res == DXGI_ERROR_UNSUPPORTED) // Try high-performance WARP software driver if hardware is not available.
        res = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_WARP, nullptr, createDeviceFlags, featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext);
    if (res != S_OK)
        return false;

    CreateRenderTarget();
    return true;
}

void CleanupDeviceD3D()
{
    CleanupRenderTarget();
    if (g_pSwapChain) { g_pSwapChain->Release(); g_pSwapChain = nullptr; }
    if (g_pd3dDeviceContext) { g_pd3dDeviceContext->Release(); g_pd3dDeviceContext = nullptr; }
    if (g_pd3dDevice) { g_pd3dDevice->Release(); g_pd3dDevice = nullptr; }
}

void CreateRenderTarget()
{
    ID3D11Texture2D* pBackBuffer;
    g_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
    g_pd3dDevice->CreateRenderTargetView(pBackBuffer, nullptr, &g_mainRenderTargetView);
    pBackBuffer->Release();
}

void CleanupRenderTarget()
{
    if (g_mainRenderTargetView) { g_mainRenderTargetView->Release(); g_mainRenderTargetView = nullptr; }
}

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
        return true;

    switch (msg)
    {
        case WM_NCHITTEST:
        {
            POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
            ScreenToClient(hWnd, &pt);

            if (pt.y < 30)
                return HTCAPTION; // permet de déplacer la fenêtre

            return HTCLIENT;
        }
        case WM_SIZE: {
            if (wParam == SIZE_MINIMIZED)
                return 0;
            g_ResizeWidth = (UINT)LOWORD(lParam);
            g_ResizeHeight = (UINT)HIWORD(lParam);
            return 0;
        }
        case WM_SYSCOMMAND: {
            if ((wParam & 0xfff0) == SC_KEYMENU) // Disable ALT application menu
                return 0;
            break;
        }
        case WM_DESTROY: {
            ::PostQuitMessage(0);
            return 0;
        }
    }
    return ::DefWindowProcW(hWnd, msg, wParam, lParam);
}
