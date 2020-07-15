#include "pch.h"

// #ifdef _DEBUG
#define DX12_ENABLE_DEBUG_LAYER
// #endif

#ifdef DX12_ENABLE_DEBUG_LAYER
#include <dxgidebug.h>
#pragma comment(lib, "dxguid.lib")
#endif
#include "imgui_impl_softraster.h"
#include "../misc/softraster/softraster.h"


const uint8_t TFTLED = 32;
const uint8_t TFTRST = 33;
const uint8_t TFTRS  = 27;
const uint8_t TFTCS  = 15;
const uint8_t TFTCLK = 14;
const uint8_t TFTSDI = 13;

#define TFTX 220
#define TFTY 176

texture_color16_t screen;
texture_alpha8_t fontAtlas;

//TFT_22_ILI9225 tft = TFT_22_ILI9225(TFTRST, TFTRS, TFTCS, TFTLED, 128);
//SPIClass tftspi(HSPI);

unsigned long drawTime;
unsigned long renderTime;
unsigned long rasterTime;

ImGuiContext *context;

void setup()
{
    // Serial.begin(115200);

    // tft.begin(tftspi);
    // tft.setFont(Terminal6x8);
    // tft.setOrientation(3);
    // digitalWrite(TFTLED, HIGH);

    context = ImGui::CreateContext();

    ImGui_ImplSoftraster_Init(&screen);

    ImGuiStyle& style = ImGui::GetStyle();
    style.AntiAliasedLines = false;
    style.AntiAliasedFill = false;
    style.WindowRounding = 0.0f;

    ImGuiIO& io = ImGui::GetIO();
    io.Fonts->Flags |= ImFontAtlasFlags_NoPowerOfTwoHeight | ImFontAtlasFlags_NoMouseCursors;

    uint8_t* pixels;
    int width, height;
    io.Fonts->GetTexDataAsAlpha8(&pixels, &width, &height);
    fontAtlas.init(width, height, (alpha8_t*)pixels);
    io.Fonts->TexID = &fontAtlas;

    screen.init(TFTX, TFTY);
}

float f = 0.0f;
unsigned long time = 0;

void loop()
{
    ImGuiIO& io = ImGui::GetIO();
    io.DeltaTime = 1.0f/60.0f;

    // io.MousePos = mouse_pos;
    // io.MouseDown[0] = mouse_button_0;
    // io.MouseDown[1] = mouse_button_1;

    /* [0.0f - 1.0f] */
    io.NavInputs[ImGuiNavInput_Activate]    = 0.0f; // activate / open / toggle / tweak value       // e.g. Circle (PS4), A (Xbox), B (Switch), Space (Keyboard)
    io.NavInputs[ImGuiNavInput_Cancel]      = 0.0f; // cancel / close / exit                        // e.g. Cross    (PS4), B (Xbox), A (Switch), Escape (Keyboard)
    io.NavInputs[ImGuiNavInput_Input]       = 0.0f; // text input / on-screen keyboard              // e.g. Triang.(PS4), Y (Xbox), X (Switch), Return (Keyboard)
    io.NavInputs[ImGuiNavInput_Menu]        = 0.0f; // tap: toggle menu / hold: focus, move, resize // e.g. Square (PS4), X (Xbox), Y (Switch), Alt (Keyboard)
    io.NavInputs[ImGuiNavInput_DpadLeft]    = 0.0f; // move / tweak / resize window (w/ PadMenu)    // e.g. D-pad Left/Right/Up/Down (Gamepads), Arrow keys (Keyboard)
    io.NavInputs[ImGuiNavInput_DpadRight]   = 0.0f;
    io.NavInputs[ImGuiNavInput_DpadUp]      = 0.0f;
    io.NavInputs[ImGuiNavInput_DpadDown]    = 0.0f;
    io.NavInputs[ImGuiNavInput_TweakSlow]   = 0.0f; // slower tweaks                                // e.g. L1 or L2 (PS4), LB or LT (Xbox), L or ZL (Switch)
    io.NavInputs[ImGuiNavInput_TweakFast]   = 0.0f; // faster tweaks                                // e.g. R1 or R2 (PS4), RB or RT (Xbox), R or ZL (Switch)

    ImGui_ImplSoftraster_NewFrame();
    ImGui::NewFrame();
    ImGui::SetWindowPos(ImVec2(0.0, 0.0));
    ImGui::SetWindowSize(ImVec2(TFTX, TFTY));

    f += 0.05;
    if(f > 1.0f) f = 0.0f;

    // unsigned int deltaTime = millis() - time;
    unsigned int deltaTime = 0 - time;
    time += deltaTime;

    deltaTime -= (drawTime + renderTime + rasterTime);

    ImGui::Text("SPI screen draw time %d ms", drawTime);
    ImGui::Text("Render time %d ms", renderTime);
    ImGui::Text("Raster time %d ms", rasterTime);
    ImGui::Text("Remaining time %d ms", deltaTime);
    ImGui::SliderFloat("SliderFloat", &f, 0.0f, 1.0f);

    // renderTime = millis();
    ImGui::Render();
    // renderTime = millis() - renderTime;

    // rasterTime = millis();
    ImGui_ImplSoftraster_RenderDrawData(ImGui::GetDrawData());
    // rasterTime = millis() - rasterTime;

    // drawTime = millis();
    //tft.drawBitmap(0, 0, (uint16_t*)screen.pixels, screen.w, screen.h);
    // drawTime = millis() - drawTime;

}

struct FrameContext
{
    ID3D12CommandAllocator* CommandAllocator;
    UINT64                  FenceValue;
};

// Data
static int const                    NUM_FRAMES_IN_FLIGHT = 3;
static FrameContext                 g_frameContext[NUM_FRAMES_IN_FLIGHT] = {};
static UINT                         g_frameIndex = 0;

static int const                    NUM_BACK_BUFFERS = 3;
static ID3D12Device* g_pd3dDevice = NULL;
static ID3D12DescriptorHeap* g_pd3dRtvDescHeap = NULL;
static ID3D12DescriptorHeap* g_pd3dSrvDescHeap = NULL;
static ID3D12CommandQueue* g_pd3dCommandQueue = NULL;
static ID3D12GraphicsCommandList* g_pd3dCommandList = NULL;
static ID3D12Fence* g_fence = NULL;
static HANDLE                       g_fenceEvent = NULL;
static UINT64                       g_fenceLastSignaledValue = 0;
static IDXGISwapChain3* g_pSwapChain = NULL;
static HANDLE                       g_hSwapChainWaitableObject = NULL;
static ID3D12Resource* g_mainRenderTargetResource[NUM_BACK_BUFFERS] = {};
static D3D12_CPU_DESCRIPTOR_HANDLE  g_mainRenderTargetDescriptor[NUM_BACK_BUFFERS] = {};


void StyleColorsDarkRed(ImGuiStyle* dst);
void StyleColorsLightGreen(ImGuiStyle* dst);

static __int64                g_Time;
static __int64                g_TicksPerSecond;
static __int64                g_TimeAtStartup;

// Forward declarations of helper functions
bool CreateDeviceD3D(HWND hWnd);
void CleanupDeviceD3D();
void CreateRenderTarget();
void CleanupRenderTarget();
void WaitForLastSubmittedFrame();
FrameContext* WaitForNextFrameResources();
void ResizeSwapChain(HWND hWnd, int width, int height);
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);


texture_base_t *Screen = nullptr;

bool ImGui_ImplSoftraster_Init(texture_base_t *screen)
{
    if (screen != nullptr)
    {
        Screen = screen;
        return true;
    }
    return false;
}

void ImGui_ImplSoftraster_Shutdown()
{
    Screen = nullptr;
}

void ImGui_ImplSoftraster_NewFrame()
{
    if (Screen == nullptr) return;

    ImGuiIO& io = ImGui::GetIO();
    io.DisplaySize.x = Screen->w;
    io.DisplaySize.y = Screen->h;
}


// Main code
int softraster_main(int, char**) {

    // Create application window
    WNDCLASSEX wc = { sizeof(WNDCLASSEX), CS_CLASSDC, WndProc, 0L, 0L, GetModuleHandle(NULL), NULL, NULL, NULL, NULL, _T("BRCalc"), NULL };
    ::RegisterClassEx(&wc);
    HWND hwnd = ::CreateWindow(wc.lpszClassName, _T("BRCalc"), WS_OVERLAPPEDWINDOW, 100, 100, 400, 500, NULL, NULL, wc.hInstance, NULL);
    texture_base_t* tela = nullptr;

    // Show the window
    ::ShowWindow(hwnd, SW_SHOWDEFAULT);
    ::UpdateWindow(hwnd);


    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    //ImGui::StyleColorsClassic();

    // Setup Platform/Renderer bindings
    ImGui_ImplSoftraster_Init(tela);
    ImGui_ImplSoftraster_NewFrame();
    //ImGui_ImplWin32_Init(hwnd);
    //ImGui_ImplDX12_Init(g_pd3dDevice, NUM_FRAMES_IN_FLIGHT,
    //    DXGI_FORMAT_R8G8B8A8_UNORM, g_pd3dSrvDescHeap,
    //    g_pd3dSrvDescHeap->GetCPUDescriptorHandleForHeapStart(),
    //    g_pd3dSrvDescHeap->GetGPUDescriptorHandleForHeapStart());

    // Load Fonts
    // - If no fonts are loaded, dear imgui will use the default font. You can also load multiple fonts and use ImGui::PushFont()/PopFont() to select them.
    // - AddFontFromFileTTF() will return the ImFont* so you can store it if you need to select the font among multiple.
    // - If the file cannot be loaded, the function will return NULL. Please handle those errors in your application (e.g. use an assertion, or display an error and quit).
    // - The fonts will be rasterized at a given size (w/ oversampling) and stored into a texture when calling ImFontAtlas::Build()/GetTexDataAsXXXX(), which ImGui_ImplXXXX_NewFrame below will call.
    // - Read 'docs/FONTS.md' for more instructions and details.
    // - Remember that in C/C++ if you want to include a backslash \ in a string literal you need to write a double backslash \\ !
    //io.Fonts->AddFontDefault();
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Roboto-Medium.ttf", 16.0f);
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Cousine-Regular.ttf", 15.0f);
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/DroidSans.ttf", 16.0f);
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/ProggyTiny.ttf", 10.0f);
    ImFont* defaultFont = io.Fonts->AddFontFromFileTTF("../../misc/fonts/Karla-Regular.ttf", 15.f);
    //ImFont* calcFont = io.Fonts->AddFontFromFileTTF("../../misc/fonts/SourceCodePro-Black.ttf", 22.f);
    //ImFont* font = io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\ArialUni.ttf", 18.0f, NULL, io.Fonts->GetGlyphRangesJapanese());
    //IM_ASSERT(font != NULL);

    // Our state
    bool show_demo_window = true;
    bool show_another_window = false;
    bool whichtheme = false;

    static char   hint[13] = "0";
    static double result = 0;
    static __int16 cursor = 0;
    static char print_buf[1024] = "";


    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

    // Main loop
    MSG msg;
    ZeroMemory(&msg, sizeof(msg));
    while (msg.message != WM_QUIT)
    {
        // Poll and handle messages (inputs, window resize, etc.)
        // You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
        // - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application.
        // - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application.
        // Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
        if (::PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE))
        {
            ::TranslateMessage(&msg);
            ::DispatchMessage(&msg);
            continue;
        }

        // Start the Dear ImGui frame
        //ImGui_ImplDX12_NewFrame();
        ImGui_ImplSoftraster_NewFrame();
        ImGui_ImplWin32_NewFrame();
        
        ImGui::NewFrame();

        // 1. Show the big demo window (Most of the sample code is in ImGui::ShowDemoWindow()! You can browse its code to learn more about Dear ImGui!).
        if (show_demo_window)
            ImGui::ShowDemoWindow(&show_demo_window);

    }
    return 0;
}
void ImGui_ImplSoftraster_RenderDrawData(ImDrawData* draw_data)
{
    if (Screen == nullptr) return;

    Screen->clear();

    switch (Screen->type)
    {
    case texture_type_t::ALPHA8:
        renderDrawLists<int32_t>(draw_data, *reinterpret_cast<texture_alpha8_t*>(Screen));
        break;

    case texture_type_t::VALUE8:
        renderDrawLists<int32_t>(draw_data, *reinterpret_cast<texture_value8_t*>(Screen));
        break;

    case texture_type_t::COLOR16:
        renderDrawLists<int32_t>(draw_data, *reinterpret_cast<texture_color16_t*>(Screen));
        break;

    case texture_type_t::COLOR24:
        renderDrawLists<int32_t>(draw_data, *reinterpret_cast<texture_color24_t*>(Screen));
        break;

    case texture_type_t::COLOR32:
        renderDrawLists<int32_t>(draw_data, *reinterpret_cast<texture_color32_t*>(Screen));
        break;

    default: return;
    }
}

