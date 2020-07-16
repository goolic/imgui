#include "pch.h"
// TODO:
// implement clearDisplayBuffer and make use of it
// implement bufferToarr
// implement calc via writing and hitting enter
// implement android version
// implement softraster version
// makeOperand and operateOrContinue need to return "struct operation"
#define STB_SPRINTF_IMPLEMENTATION
#include "stb_sprintf.h"

#define i8 signed __int8
#define i8_MAX 127
#define i8_MIN -128
#define u8 unsigned __int8
#define u8_MAX 255
#define u8_MIN 0
#define i16 signed __int16
#define i16_MAX 32,767
#define i16_MIN -32,768
#define u16 unsigned __int16
#define f64 double
#define strLit const char *

#define arr struct AJMVarray

// dear imgui: standalone example application for DirectX 12
// If you are new to dear imgui, see examples/README.txt and documentation at the top of imgui.cpp.
// FIXME: 64-bit only for now! (Because sizeof(ImTextureId) == sizeof(void*))

// #include "imgui.h"
// #include "imgui_impl_win32.h"
// #include "imgui_impl_dx12.h"
// #include <d3d12.h>
// #include <dxgi1_4.h>
// #include <tchar.h>
// #include <math.h>

// #ifdef _DEBUG
// #define DX12_ENABLE_DEBUG_LAYER
// #endif

// #ifdef DX12_ENABLE_DEBUG_LAYER
// #include <dxgidebug.h>
// #pragma comment(lib, "dxguid.lib")
// #endif

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
static ID3D12Device*                g_pd3dDevice = NULL;
static ID3D12DescriptorHeap*        g_pd3dRtvDescHeap = NULL;
static ID3D12DescriptorHeap*        g_pd3dSrvDescHeap = NULL;
static ID3D12CommandQueue*          g_pd3dCommandQueue = NULL;
static ID3D12GraphicsCommandList*   g_pd3dCommandList = NULL;
static ID3D12Fence*                 g_fence = NULL;
static HANDLE                       g_fenceEvent = NULL;
static UINT64                       g_fenceLastSignaledValue = 0;
static IDXGISwapChain3*             g_pSwapChain = NULL;
static HANDLE                       g_hSwapChainWaitableObject = NULL;
static ID3D12Resource*              g_mainRenderTargetResource[NUM_BACK_BUFFERS] = {};
static D3D12_CPU_DESCRIPTOR_HANDLE  g_mainRenderTargetDescriptor[NUM_BACK_BUFFERS] = {};

char   buf[u8_MAX-1] =  "";
i16 i = 0;
f64 accumulator = 0;
char arrayS[u8_MAX-1] = "";
static char print_buf[1024] = "";
u8 base = 10;

void StyleColorsDarkRed(ImGuiStyle* dst);
void StyleColorsLightGreen(ImGuiStyle* dst);

static __int64                g_Time;
static __int64                g_TicksPerSecond;
static __int64                g_TimeAtStartup;
#ifndef STARTUP_BENCHMARK 
    #define STARTUP_BENCHMARK
    
    static __int64                  g_AppCreationTime;
    static __int64                  g_AppDestructionTime;
    
#endif
//comentar abaixo para encerar o benchmark
#undef STARTUP_BENCHMARK


// Forward declarations of helper functions
int softraster_main(int, char**);
char * arrayToString(arr * theArr);
void setup();
void loop();
int dx12_main();
bool CreateDeviceD3D(HWND hWnd);
void CleanupDeviceD3D();
void CreateRenderTarget();
void CleanupRenderTarget();
void WaitForLastSubmittedFrame();
FrameContext* WaitForNextFrameResources();
void ResizeSwapChain(HWND hWnd, int width, int height);
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

//se o usurari clicar em 1 botao temos de multiplicar o valor 
//pela casa decimal em que estamos, adicionando o valor.
//se clicar 7 operand recebe 7*1, se clicar em seguida
//no 8 operand recebe 7*10 + 8*1 e se por fim clicar em 3,
//recebera 7*100 + 8*10 + 3 * 1
// queremos que primeiro os digitos sejam adicionados
//em um array, e uma vez que a operação fopr selecionada
// os digitos tem que ser popped fo array e receberem
// n zeros e somados ao operand
BETTER_ENUM (OP_ENUM, u8, OP_NONE,
    OP_MULTIPLICATION,
    OP_DIVISION,
    OP_SUM,
    OP_SUBTRACTION,
    OP_EQUALS,
    OP_SOMETHING,
    OP_COMMA)

typedef enum {
    ST_FIRST_OPERAND,
    ST_FIRST_OPER_FRACTION,
    ST_SECOND_OPERAND,
    ST_SECOND_OPER_FRACTION,
    ST_RESULT_OBTAINED
} STATE_ENUM;

arr {
    u8 item[u8_MAX-1] = { 0 };
    u8 size = 0;
    u8 commaPosition;
    bool hasComma;
};

struct operation {
    f64 x = 0;
    f64 y = 0;
    f64 result = 0;
    arr xC;
    arr yC;
    OP_ENUM op;
    STATE_ENUM state;
};

struct charBuffer {
    char * buf = "";
    u16 size = 0;
};

//if maxsize==0 we use u8_MAX as the buffer sizer
void clearDisplayBuffer(struct charBuffer display) {
    for (i=0; i <= display.size; i = i+1){
        display.buf = "\0";
    }
}

void logProgress(struct operation& ops){
    int ret = 0;
    auto xCstr = arrayToString(&ops.xC);
    auto yCstr = arrayToString(&ops.yC);
    // ret = stbsp_sprintf(print_buf, "x:\ty:\txC:\txC.size\ty:\txY.size\n");
    // fwrite(print_buf, sizeof(char), ret, stdout)
    // ret = stbsp_sprintf(print_buf, "%.3f\t%.3f\t%s\t%d\t%s\t%d", ops.x, ops.y, xCstr, ops.xC.size, yCstr, ops.yC.size);
    ret = stbsp_sprintf(print_buf, "\n*NOVO*\nx:  %.3f\ny:  %.3f\nresult  %.3f\nxC: %s\nxC.size %d\nxy: %s\nxY.size %d\n",
                                            ops.x,   ops.y,   ops.result,    xCstr, ops.xC.size,  yCstr, ops.yC.size);
    fwrite(print_buf, sizeof(char), ret, stdout);
}

//@speed we unrool this loop, since we garantee that theArr is 0 initialized
char * arrayToString(arr * theArr) {
    int ch = 0;
    for (i = 0; i != theArr->size; i = i + 1) {
        ch = theArr->item[i] + 48;
        arrayS[i] = (char)ch;
    }
    return arrayS;
}

void refreshOperand(struct operation& ops) {
    accumulator = 0;
    i16 j = 0;
    if (ops.state == ST_FIRST_OPERAND) {
        for (i = ops.xC.size; i >= 0 && j <= ops.xC.size; i = i - 1) {
            accumulator = (accumulator + (f64)(ops.xC.item[i] * pow(base, j)));
            j = j + 1;
        }
        ops.x = accumulator;
    }
    if (ops.state == ST_SECOND_OPERAND) {
        for (i = ops.yC.size; i >= 0 && j <= ops.yC.size; i = i - 1) {
            accumulator = (accumulator + (f64)(ops.yC.item[i] * pow(base, j)));
            j = j + 1;
        }
        ops.y = accumulator;
    }
    
}

void addToDisplay(u8 digit, struct operation& ops) {
    // we convert from an int to an ascii char
    digit = digit + 48;

    //const char* ascii_digit = (const char*)(digit);
    //strncat(buf, ascii_digit,1);
    strncat_s(buf, (const char*)&digit, 1);

    logProgress(ops);
}

f64 makeOperand (u8 digit, struct operation& ops) {
    //pra ficar correto é necessário armazenar os digitos 
    //em um array e fazer pop deles na ordem reversa para 
    //que a unidade de cada casa esteja no lugar certo
    u8 cursor = 0;

    if (ops.xC.size || ops.yC.size < u8_MAX) {//erro, não travar por enquanto


        if (ops.state == ST_FIRST_OPERAND) {
            if (ops.xC.size == 0) {
                ops.xC.item[ops.xC.size] = digit;
                ops.xC.size = ops.xC.size + 1;
                ops.x = digit;//(ops.x + (double)(digit * pow(10, ops.e)));

                addToDisplay(digit, ops);

                return ops.x;
            }
            if (ops.xC.size > 0) {
                ops.xC.item[ops.xC.size] = digit;
                ops.xC.size = ops.xC.size + 1;
                refreshOperand(ops);

                addToDisplay(digit, ops);

                return ops.x;
            }
        }
        if (ops.state == ST_SECOND_OPERAND) {
            if (ops.yC.size == 0) {
                ops.yC.item[ops.yC.size] = digit;
                ops.yC.size = ops.yC.size + 1;
                ops.y = digit;//(ops.x + (double)(digit * pow(10, ops.e)));

                addToDisplay(digit, ops);

                return ops.y;
            }
            if (ops.yC.size > 0) {
                ops.yC.item[ops.yC.size] = digit;
                ops.yC.size = ops.yC.size + 1;
                refreshOperand(ops);

                addToDisplay(digit, ops);

                return ops.y;
            }
        }
        

    }

    //DO NOTHING, error condition
}

void operateOrContinue(OP_ENUM op, struct operation& ops)
{
    //do we need to do comething fancy here?
    //if (op == OP_ENUM::OP_EQUALS._to_integral)

    if (op._to_integral == OP_ENUM::OP_COMMA._to_integral) {//continue ops.state, set ops.xyC.commaPosition to ops.xyC.size
        //@robustness we need to see if the commaposition is not off by one
        if (ops.state == ST_FIRST_OPERAND) ops.xC.commaPosition = ops.xC.size;
        if (ops.state == ST_SECOND_OPERAND) ops.yC.commaPosition = ops.yC.size;
    }
    
    if ((ops.state == ST_FIRST_OPERAND) && (op != OP_ENUM::OP_COMMA._to_integral)) {
        ops.state = ST_SECOND_OPERAND;
        ops.op = op;
    } 
    
    if ((ops.state == ST_SECOND_OPERAND) && (op != OP_ENUM::OP_COMMA._to_integral)) {
        assert(ops.op != OP_ENUM::OP_NONE._to_integral);

        if (op == OP_ENUM::OP_SUM._to_integral || ops.op == OP_ENUM::OP_SUM._to_integral)
        {
            ops.result = ops.x + ops.y;
        }
        if (op == OP_ENUM::OP_SUBTRACTION._to_integral || ops.op == OP_ENUM::OP_SUBTRACTION._to_integral)
        {
            ops.result = ops.x - ops.y;
        }
        if (op == OP_ENUM::OP_MULTIPLICATION._to_integral || ops.op == OP_ENUM::OP_MULTIPLICATION._to_integral)
        {
            ops.result = ops.x * ops.y;
        }
        if (op == OP_ENUM::OP_DIVISION._to_integral || ops.op == OP_ENUM::OP_DIVISION._to_integral)
        {
            ops.result = (double) ops.x/ops.y;
            // ops.result += (double) ops.x%ops.y;
        }
    }
}

// Main code
int main(int, char**) {
    #ifdef STARTUP_BENCHMARK 
    if (!::QueryPerformanceCounter((LARGE_INTEGER*)&g_AppCreationTime))
                return false;
    #endif

    //isso foi meu chuto de como fazer funcionar
    //o codigo original fica no .ino o arduino chama automagicamente setup e loop
    //softraster_main(1,(char**)"args");
    // setup();
    // loop();
    dx12_main();

    return 0;
}

int dx12_main(){
    // Create application window
    WNDCLASSEX wc = { sizeof(WNDCLASSEX), CS_CLASSDC, WndProc, 0L, 0L, GetModuleHandle(NULL), NULL, NULL, NULL, NULL, _T( "BRCalc" ), NULL };
    ::RegisterClassEx(&wc);
    HWND hwnd = ::CreateWindow(wc.lpszClassName, _T("BRCalc"), WS_OVERLAPPEDWINDOW, 100, 100, 400, 500, NULL, NULL, wc.hInstance, NULL);

    // Initialize Direct3D
    if (!CreateDeviceD3D(hwnd))
    {
        CleanupDeviceD3D();
        ::UnregisterClass(wc.lpszClassName, wc.hInstance);
        return 1;
    }

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
    ImGui_ImplWin32_Init(hwnd);
    ImGui_ImplDX12_Init(g_pd3dDevice, NUM_FRAMES_IN_FLIGHT,
        DXGI_FORMAT_R8G8B8A8_UNORM, g_pd3dSrvDescHeap,
        g_pd3dSrvDescHeap->GetCPUDescriptorHandleForHeapStart(),
        g_pd3dSrvDescHeap->GetGPUDescriptorHandleForHeapStart());

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
    auto defaultFont = io.Fonts->AddFontFromFileTTF("../../misc/fonts/Karla-Regular.ttf", 15.f);
    auto calcFont = io.Fonts->AddFontFromFileTTF("../../misc/fonts/SourceCodePro-Black.ttf", 22.f);
    //ImFont* font = io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\ArialUni.ttf", 18.0f, NULL, io.Fonts->GetGlyphRangesJapanese());
    //IM_ASSERT(font != NULL);

    // Our state
    bool show_demo_window = true;
    bool show_another_window = false;
    bool whichtheme = false;

    static char   hint[13] = "0";
    static double result   = 0;
    struct operation op_history [1024];
    static __int16 cursor = 0;


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
        ImGui_ImplDX12_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        // 1. Show the big demo window (Most of the sample code is in ImGui::ShowDemoWindow()! You can browse its code to learn more about Dear ImGui!).
        if (show_demo_window)
            ImGui::ShowDemoWindow(&show_demo_window);

        // 2. Show a simple window that we create ourselves. We use a Begin/End pair to created a named window.
        {
            static float f = 0.0f;
            static int counter = 0;

            ImGui::Begin("Hello, world!");                          // Create a window called "Hello, world!" and append into it.

            ImGui::Text("This is some useful text.");               // Display some text (you can use a format strings too)
            ImGui::Checkbox("Demo Window", &show_demo_window);      // Edit bools storing our window open/close state
            ImGui::Checkbox("Another Window", &show_another_window);

            ImGui::SliderFloat("float", &f, 0.0f, 1.0f);            // Edit 1 float using a slider from 0.0f to 1.0f
            ImGui::ColorEdit3("clear color", (float*)&clear_color); // Edit 3 floats representing a color

            if (ImGui::Button("Button"))                            // Buttons return true when clicked (most widgets return true when edited/activated)
                counter++;
            ImGui::SameLine();
            ImGui::Text("counter = %d", counter);

            ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
            ImGui::End();
        }

        // 3. Show another simple window.
        if (show_another_window)
        {
            ImGui::Begin("Another Window", &show_another_window);   // Pass a pointer to our bool variable (the window will have a closing button that will clear the bool when clicked)
            ImGui::Text("Hello from another window!");
            if (ImGui::Button("Close Me"))
                show_another_window = false;
            ImGui::End();
        }

        // 4. Show a simple window that we create ourselves. We use a Begin/End pair to created a named window.
        {


            //static float result = 0.0f;
            static int counter = 0;

            ImGui::Begin("Do your calcs here !!!");                          // Create a window called "Hello, world!" and append into it.

            ImGui::Text("When adding numbers you press +");               // Display some text (you can use a format strings too)

            
            if (ImGui::Button("Togle Theme")) {
                whichtheme = !whichtheme;
            }

            if (whichtheme) {
                StyleColorsDarkRed(NULL);
            } else {
                StyleColorsLightGreen(NULL);
            }

            //acho que preciso ir em imgui draw e checar se os buttons
            //são da janela xyz, e lá alterar a fonte que pinta essa janela,
            //tem alguem dizendo que tem que alterar a fonte quando beginframe,
            //mas não queremos fazer isso
            ImGui::PushFont(calcFont);


           
            auto bSize = ImVec2(ImGui::GetWindowSize().x*0.1f, ImGui::GetWindowSize().x*0.1f);

            //precisamos que o keyboard esteja aqui por padão mas não monopolize o foco
            //ImGui::SetKeyboardFocusHere();
            //queremos a label em branco e o buf inicial em branco
            //ImGuiInputTextFlags_CharsDecimal
            //ImGuiInputTextFlags_CharsScientific
            //ImGuiInputTextFlags_CharsHexadecimal
            //ImGuiInputTextFlags_EnterReturnsTrue
            //ImGuiInputTextFlags_CallbackHistory
            ImGui::PushItemWidth(160.0f);

            //
            // This is the input in which we display the number 
            //
            ImGui::InputTextWithHint("", hint, buf, IM_ARRAYSIZE(buf));

            if (ImGui::Button("7",   bSize))
                makeOperand(7,op_history[cursor]);
            ImGui::SameLine();
            if (ImGui::Button("8",   bSize)) makeOperand(8,op_history[cursor]); ImGui::SameLine();
            if (ImGui::Button("9",   bSize)) makeOperand(9,op_history[cursor]); ImGui::SameLine();
            if (ImGui::Button("X",   bSize)) operateOrContinue(OP_ENUM::OP_MULTIPLICATION,op_history[cursor]);
            if (ImGui::Button("4",   bSize)) makeOperand(4,op_history[cursor]); ImGui::SameLine();
            if (ImGui::Button("5",   bSize)) makeOperand(5,op_history[cursor]); ImGui::SameLine();
            if (ImGui::Button("6",   bSize)) makeOperand(6,op_history[cursor]); ImGui::SameLine();
            if (ImGui::Button("-",   bSize)) operateOrContinue(OP_ENUM::OP_SUBTRACTION,op_history[cursor]); 
            if (ImGui::Button("1",   bSize)) makeOperand(1,op_history[cursor]); ImGui::SameLine();
            if (ImGui::Button("2",   bSize)) makeOperand(2,op_history[cursor]); ImGui::SameLine();
            if (ImGui::Button("3",   bSize)) makeOperand(3,op_history[cursor]); ImGui::SameLine();
            if (ImGui::Button("+",   bSize)) operateOrContinue(OP_ENUM::OP_SUM,op_history[cursor]);
            if (ImGui::Button("+/-", bSize)) operateOrContinue(OP_ENUM::OP_SOMETHING,op_history[cursor]); ImGui::SameLine();
            if (ImGui::Button("0",   bSize)) makeOperand(0,op_history[cursor]); ImGui::SameLine();
            if (ImGui::Button(",",   bSize)) operateOrContinue(OP_ENUM::OP_COMMA,op_history[cursor]); ImGui::SameLine();
            if (ImGui::Button("=",   bSize)) operateOrContinue(OP_ENUM::OP_EQUALS,op_history[cursor]);

            ImGui::PopFont();

            ImGui::Text("counter = %d", counter);

            ImGui::Text("Application average \n%.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
            ImGui::End();

        }

        // Rendering
        FrameContext* frameCtxt = WaitForNextFrameResources();
        UINT backBufferIdx = g_pSwapChain->GetCurrentBackBufferIndex();
        frameCtxt->CommandAllocator->Reset();

        D3D12_RESOURCE_BARRIER barrier = {};
        barrier.Type                   = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrier.Flags                  = D3D12_RESOURCE_BARRIER_FLAG_NONE;
        barrier.Transition.pResource   = g_mainRenderTargetResource[backBufferIdx];
        barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
        barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
        barrier.Transition.StateAfter  = D3D12_RESOURCE_STATE_RENDER_TARGET;

        g_pd3dCommandList->Reset(frameCtxt->CommandAllocator, NULL);
        g_pd3dCommandList->ResourceBarrier(1, &barrier);
        g_pd3dCommandList->ClearRenderTargetView(g_mainRenderTargetDescriptor[backBufferIdx], (float*)&clear_color, 0, NULL);
        g_pd3dCommandList->OMSetRenderTargets(1, &g_mainRenderTargetDescriptor[backBufferIdx], FALSE, NULL);
        g_pd3dCommandList->SetDescriptorHeaps(1, &g_pd3dSrvDescHeap);
        ImGui::Render();
        ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), g_pd3dCommandList);
        barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
        barrier.Transition.StateAfter  = D3D12_RESOURCE_STATE_PRESENT;
        g_pd3dCommandList->ResourceBarrier(1, &barrier);
        g_pd3dCommandList->Close();

        g_pd3dCommandQueue->ExecuteCommandLists(1, (ID3D12CommandList* const*)&g_pd3dCommandList);

        g_pSwapChain->Present(1, 0); // Present with vsync
        //g_pSwapChain->Present(0, 0); // Present without vsync

        UINT64 fenceValue = g_fenceLastSignaledValue + 1;
        g_pd3dCommandQueue->Signal(g_fence, fenceValue);
        g_fenceLastSignaledValue = fenceValue;
        frameCtxt->FenceValue = fenceValue;

        
        //manter desativado, benchmark de startup, vai encerrar app depois de um único frame
        #ifdef STARTUP_BENCHMARK

            __int64 TicksPerSecond;
            if (!::QueryPerformanceCounter((LARGE_INTEGER*)&g_AppDestructionTime))
                return false;
            if (!::QueryPerformanceFrequency((LARGE_INTEGER*)&TicksPerSecond))
                return false;


            //#include <Synchapi.h>
            //Sleep(1000);

            //WaitForLastSubmittedFrame();
            int ret = 0;

            ret = stbsp_sprintf (print_buf, "time at startup: %lld\n", g_AppCreationTime);
            fwrite(print_buf,  sizeof(char), ret, stdout);

            __int64 current_time = 0;
            ::QueryPerformanceCounter((LARGE_INTEGER*)&current_time);

            ret = stbsp_sprintf (print_buf, "time at shutdown %lld\n", g_AppDestructionTime);
            fwrite(print_buf,  sizeof(char), ret, stdout);

            ret = stbsp_sprintf (print_buf, "time at shutdown %lld\n", TicksPerSecond);
            fwrite(print_buf,  sizeof(char), ret, stdout);

            double calc = (double) (g_AppDestructionTime-g_AppCreationTime)/TicksPerSecond;

            if (TicksPerSecond != 0)
                ret = stbsp_sprintf (print_buf, "exec time at shutdown %f ms\n", calc);
            else
                ret = stbsp_sprintf (print_buf, "to infinity and beyond\n" ); 
            fwrite(print_buf,  sizeof(char), ret, stdout);
        

            ImGui_ImplDX12_Shutdown();
            ImGui_ImplWin32_Shutdown();
            ImGui::DestroyContext();

            CleanupDeviceD3D();
            ::DestroyWindow(hwnd);
            ::UnregisterClass(wc.lpszClassName, wc.hInstance);

            return 0;
        #endif

    }

    WaitForLastSubmittedFrame();
    ImGui_ImplDX12_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    CleanupDeviceD3D();
    ::DestroyWindow(hwnd);
    ::UnregisterClass(wc.lpszClassName, wc.hInstance);
}

//https://github.com/ocornut/imgui/issues/707#issuecomment-636221193
void StyleColorsDarkRed(ImGuiStyle* dst) {
    ImGuiStyle* style = dst ? dst : &ImGui::GetStyle();
    style->FrameRounding = 4.0f;
    style->WindowBorderSize = 0.0f;
    style->PopupBorderSize = 0.0f;
    style->GrabRounding = 4.0f;

    ImVec4* colors = ImGui::GetStyle().Colors;
    colors[ImGuiCol_Text]                   = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
    colors[ImGuiCol_TextDisabled]           = ImVec4(0.73f, 0.75f, 0.74f, 1.00f);
    colors[ImGuiCol_WindowBg]               = ImVec4(0.09f, 0.09f, 0.09f, 0.94f);
    colors[ImGuiCol_ChildBg]                = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_PopupBg]                = ImVec4(0.08f, 0.08f, 0.08f, 0.94f);
    colors[ImGuiCol_Border]                 = ImVec4(0.20f, 0.20f, 0.20f, 0.50f);
    colors[ImGuiCol_BorderShadow]           = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_FrameBg]                = ImVec4(0.71f, 0.39f, 0.39f, 0.54f);
    colors[ImGuiCol_FrameBgHovered]         = ImVec4(0.84f, 0.66f, 0.66f, 0.40f);
    colors[ImGuiCol_FrameBgActive]          = ImVec4(0.84f, 0.66f, 0.66f, 0.67f);
    colors[ImGuiCol_TitleBg]                = ImVec4(0.47f, 0.22f, 0.22f, 0.67f);
    colors[ImGuiCol_TitleBgActive]          = ImVec4(0.47f, 0.22f, 0.22f, 1.00f);
    colors[ImGuiCol_TitleBgCollapsed]       = ImVec4(0.47f, 0.22f, 0.22f, 0.67f);
    colors[ImGuiCol_MenuBarBg]              = ImVec4(0.34f, 0.16f, 0.16f, 1.00f);
    colors[ImGuiCol_ScrollbarBg]            = ImVec4(0.02f, 0.02f, 0.02f, 0.53f);
    colors[ImGuiCol_ScrollbarGrab]          = ImVec4(0.31f, 0.31f, 0.31f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabHovered]   = ImVec4(0.41f, 0.41f, 0.41f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabActive]    = ImVec4(0.51f, 0.51f, 0.51f, 1.00f);
    colors[ImGuiCol_CheckMark]              = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
    colors[ImGuiCol_SliderGrab]             = ImVec4(0.71f, 0.39f, 0.39f, 1.00f);
    colors[ImGuiCol_SliderGrabActive]       = ImVec4(0.84f, 0.66f, 0.66f, 1.00f);
    colors[ImGuiCol_Button]                 = ImVec4(0.47f, 0.22f, 0.22f, 0.65f);
    colors[ImGuiCol_ButtonHovered]          = ImVec4(0.71f, 0.39f, 0.39f, 0.65f);
    colors[ImGuiCol_ButtonActive]           = ImVec4(0.20f, 0.20f, 0.20f, 0.50f);
    colors[ImGuiCol_Header]                 = ImVec4(0.71f, 0.39f, 0.39f, 0.54f);
    colors[ImGuiCol_HeaderHovered]          = ImVec4(0.84f, 0.66f, 0.66f, 0.65f);
    colors[ImGuiCol_HeaderActive]           = ImVec4(0.84f, 0.66f, 0.66f, 0.00f);
    colors[ImGuiCol_Separator]              = ImVec4(0.43f, 0.43f, 0.50f, 0.50f);
    colors[ImGuiCol_SeparatorHovered]       = ImVec4(0.71f, 0.39f, 0.39f, 0.54f);
    colors[ImGuiCol_SeparatorActive]        = ImVec4(0.71f, 0.39f, 0.39f, 0.54f);
    colors[ImGuiCol_ResizeGrip]             = ImVec4(0.71f, 0.39f, 0.39f, 0.54f);
    colors[ImGuiCol_ResizeGripHovered]      = ImVec4(0.84f, 0.66f, 0.66f, 0.66f);
    colors[ImGuiCol_ResizeGripActive]       = ImVec4(0.84f, 0.66f, 0.66f, 0.66f);
    colors[ImGuiCol_Tab]                    = ImVec4(0.71f, 0.39f, 0.39f, 0.54f);
    colors[ImGuiCol_TabHovered]             = ImVec4(0.84f, 0.66f, 0.66f, 0.66f);
    colors[ImGuiCol_TabActive]              = ImVec4(0.84f, 0.66f, 0.66f, 0.66f);
    colors[ImGuiCol_TabUnfocused]           = ImVec4(0.07f, 0.10f, 0.15f, 0.97f);
    colors[ImGuiCol_TabUnfocusedActive]     = ImVec4(0.14f, 0.26f, 0.42f, 1.00f);
    colors[ImGuiCol_PlotLines]              = ImVec4(0.61f, 0.61f, 0.61f, 1.00f);
    colors[ImGuiCol_PlotLinesHovered]       = ImVec4(1.00f, 0.43f, 0.35f, 1.00f);
    colors[ImGuiCol_PlotHistogram]          = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
    colors[ImGuiCol_PlotHistogramHovered]   = ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
    colors[ImGuiCol_TextSelectedBg]         = ImVec4(0.26f, 0.59f, 0.98f, 0.35f);
    colors[ImGuiCol_DragDropTarget]         = ImVec4(1.00f, 1.00f, 0.00f, 0.90f);
    colors[ImGuiCol_NavHighlight]           = ImVec4(0.41f, 0.41f, 0.41f, 1.00f);
    colors[ImGuiCol_NavWindowingHighlight]  = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
    colors[ImGuiCol_NavWindowingDimBg]      = ImVec4(0.80f, 0.80f, 0.80f, 0.20f);
    colors[ImGuiCol_ModalWindowDimBg]       = ImVec4(0.80f, 0.80f, 0.80f, 0.35f);
}

// generic miniDart theme
//https://github.com/ocornut/imgui/issues/707#issuecomment-439117182
void StyleColorsLightGreen(ImGuiStyle* dst) {
    ImGuiStyle* style = dst ? dst : &ImGui::GetStyle();
    ImVec4* colors = style->Colors;

    style->WindowRounding    = 2.0f;             // Radius of window corners rounding. Set to 0.0f to have rectangular windows
    style->ScrollbarRounding = 3.0f;             // Radius of grab corners rounding for scrollbar
    style->GrabRounding      = 2.0f;             // Radius of grabs corners rounding. Set to 0.0f to have rectangular slider grabs.
    style->AntiAliasedLines  = true;
    style->AntiAliasedFill   = true;
    style->WindowRounding    = 2;
    style->ChildRounding     = 2;
    style->ScrollbarSize     = 16;
    style->ScrollbarRounding = 3;
    style->GrabRounding      = 2;
    style->ItemSpacing.x     = 10;
    style->ItemSpacing.y     = 4;
    style->IndentSpacing     = 22;
    style->FramePadding.x    = 6;
    style->FramePadding.y    = 4;
    style->Alpha             = 1.0f;
    style->FrameRounding     = 3.0f;

    colors[ImGuiCol_Text]                   = ImVec4(0.00f, 0.00f, 0.00f, 1.00f);
    colors[ImGuiCol_TextDisabled]          = ImVec4(0.60f, 0.60f, 0.60f, 1.00f);
    colors[ImGuiCol_WindowBg]              = ImVec4(0.86f, 0.86f, 0.86f, 1.00f);
    //colors[ImGuiCol_ChildWindowBg]         = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_ChildBg]                = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_PopupBg]                = ImVec4(0.93f, 0.93f, 0.93f, 0.98f);
    colors[ImGuiCol_Border]                = ImVec4(0.71f, 0.71f, 0.71f, 0.08f);
    colors[ImGuiCol_BorderShadow]          = ImVec4(0.00f, 0.00f, 0.00f, 0.04f);
    colors[ImGuiCol_FrameBg]               = ImVec4(0.71f, 0.71f, 0.71f, 0.55f);
    colors[ImGuiCol_FrameBgHovered]        = ImVec4(0.94f, 0.94f, 0.94f, 0.55f);
    colors[ImGuiCol_FrameBgActive]         = ImVec4(0.71f, 0.78f, 0.69f, 0.98f);
    colors[ImGuiCol_TitleBg]               = ImVec4(0.85f, 0.85f, 0.85f, 1.00f);
    colors[ImGuiCol_TitleBgCollapsed]      = ImVec4(0.82f, 0.78f, 0.78f, 0.51f);
    colors[ImGuiCol_TitleBgActive]         = ImVec4(0.78f, 0.78f, 0.78f, 1.00f);
    colors[ImGuiCol_MenuBarBg]             = ImVec4(0.86f, 0.86f, 0.86f, 1.00f);
    colors[ImGuiCol_ScrollbarBg]           = ImVec4(0.20f, 0.25f, 0.30f, 0.61f);
    colors[ImGuiCol_ScrollbarGrab]         = ImVec4(0.90f, 0.90f, 0.90f, 0.30f);
    colors[ImGuiCol_ScrollbarGrabHovered]  = ImVec4(0.92f, 0.92f, 0.92f, 0.78f);
    colors[ImGuiCol_ScrollbarGrabActive]   = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
    colors[ImGuiCol_CheckMark]             = ImVec4(0.184f, 0.407f, 0.193f, 1.00f);
    colors[ImGuiCol_SliderGrab]            = ImVec4(0.26f, 0.59f, 0.98f, 0.78f);
    colors[ImGuiCol_SliderGrabActive]      = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
    colors[ImGuiCol_Button]                = ImVec4(0.71f, 0.78f, 0.69f, 0.40f);
    colors[ImGuiCol_ButtonHovered]         = ImVec4(0.725f, 0.805f, 0.702f, 1.00f);
    colors[ImGuiCol_ButtonActive]          = ImVec4(0.793f, 0.900f, 0.836f, 1.00f);
    colors[ImGuiCol_Header]                = ImVec4(0.71f, 0.78f, 0.69f, 0.31f);
    colors[ImGuiCol_HeaderHovered]         = ImVec4(0.71f, 0.78f, 0.69f, 0.80f);
    colors[ImGuiCol_HeaderActive]          = ImVec4(0.71f, 0.78f, 0.69f, 1.00f);
//        colors[ImGuiCol_Column]                = ImVec4(0.39f, 0.39f, 0.39f, 1.00f);
//      colors[ImGuiCol_ColumnHovered]         = ImVec4(0.26f, 0.59f, 0.98f, 0.78f);
//    colors[ImGuiCol_ColumnActive]          = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
    colors[ImGuiCol_Separator]              = ImVec4(0.39f, 0.39f, 0.39f, 1.00f);
    colors[ImGuiCol_SeparatorHovered]       = ImVec4(0.14f, 0.44f, 0.80f, 0.78f);
    colors[ImGuiCol_SeparatorActive]        = ImVec4(0.14f, 0.44f, 0.80f, 1.00f);
    colors[ImGuiCol_ResizeGrip]            = ImVec4(1.00f, 1.00f, 1.00f, 0.00f);
    colors[ImGuiCol_ResizeGripHovered]     = ImVec4(0.26f, 0.59f, 0.98f, 0.45f);
    colors[ImGuiCol_ResizeGripActive]      = ImVec4(0.26f, 0.59f, 0.98f, 0.78f);
    colors[ImGuiCol_PlotLines]             = ImVec4(0.39f, 0.39f, 0.39f, 1.00f);
    colors[ImGuiCol_PlotLinesHovered]      = ImVec4(1.00f, 0.43f, 0.35f, 1.00f);
    colors[ImGuiCol_PlotHistogram]         = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
    colors[ImGuiCol_PlotHistogramHovered]  = ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
    colors[ImGuiCol_TextSelectedBg]        = ImVec4(0.26f, 0.59f, 0.98f, 0.35f);
    colors[ImGuiCol_ModalWindowDarkening]  = ImVec4(0.20f, 0.20f, 0.20f, 0.35f);
    colors[ImGuiCol_DragDropTarget]         = ImVec4(0.26f, 0.59f, 0.98f, 0.95f);
    colors[ImGuiCol_NavHighlight]           = colors[ImGuiCol_HeaderHovered];
    colors[ImGuiCol_NavWindowingHighlight]  = ImVec4(0.70f, 0.70f, 0.70f, 0.70f);
}


// Helper functions

bool CreateDeviceD3D(HWND hWnd)
{
    // Setup swap chain
    DXGI_SWAP_CHAIN_DESC1 sd;
    {
        ZeroMemory(&sd, sizeof(sd));
        sd.BufferCount = NUM_BACK_BUFFERS;
        sd.Width = 0;
        sd.Height = 0;
        sd.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        sd.Flags = DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT;
        sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        sd.SampleDesc.Count = 1;
        sd.SampleDesc.Quality = 0;
        sd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
        sd.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
        sd.Scaling = DXGI_SCALING_STRETCH;
        sd.Stereo = FALSE;
    }

#ifdef DX12_ENABLE_DEBUG_LAYER
    ID3D12Debug* pdx12Debug = NULL;
    if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&pdx12Debug))))
    {
        pdx12Debug->EnableDebugLayer();
        pdx12Debug->Release();
    }
#endif

    D3D_FEATURE_LEVEL featureLevel = D3D_FEATURE_LEVEL_11_0;
    if (D3D12CreateDevice(NULL, featureLevel, IID_PPV_ARGS(&g_pd3dDevice)) != S_OK)
        return false;

    {
        D3D12_DESCRIPTOR_HEAP_DESC desc = {};
        desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
        desc.NumDescriptors = NUM_BACK_BUFFERS;
        desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
        desc.NodeMask = 1;
        if (g_pd3dDevice->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&g_pd3dRtvDescHeap)) != S_OK)
            return false;

        SIZE_T rtvDescriptorSize = g_pd3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
        D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = g_pd3dRtvDescHeap->GetCPUDescriptorHandleForHeapStart();
        for (UINT i = 0; i < NUM_BACK_BUFFERS; i++)
        {
            g_mainRenderTargetDescriptor[i] = rtvHandle;
            rtvHandle.ptr += rtvDescriptorSize;
        }
    }

    {
        D3D12_DESCRIPTOR_HEAP_DESC desc = {};
        desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
        desc.NumDescriptors = 1;
        desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
        if (g_pd3dDevice->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&g_pd3dSrvDescHeap)) != S_OK)
            return false;
    }

    {
        D3D12_COMMAND_QUEUE_DESC desc = {};
        desc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
        desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
        desc.NodeMask = 1;
        if (g_pd3dDevice->CreateCommandQueue(&desc, IID_PPV_ARGS(&g_pd3dCommandQueue)) != S_OK)
            return false;
    }

    for (UINT i = 0; i < NUM_FRAMES_IN_FLIGHT; i++)
        if (g_pd3dDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&g_frameContext[i].CommandAllocator)) != S_OK)
            return false;

    if (g_pd3dDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, g_frameContext[0].CommandAllocator, NULL, IID_PPV_ARGS(&g_pd3dCommandList)) != S_OK ||
        g_pd3dCommandList->Close() != S_OK)
        return false;

    if (g_pd3dDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&g_fence)) != S_OK)
        return false;

    g_fenceEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
    if (g_fenceEvent == NULL)
        return false;

    {
        IDXGIFactory4* dxgiFactory = NULL;
        IDXGISwapChain1* swapChain1 = NULL;
        if (CreateDXGIFactory1(IID_PPV_ARGS(&dxgiFactory)) != S_OK ||
            dxgiFactory->CreateSwapChainForHwnd(g_pd3dCommandQueue, hWnd, &sd, NULL, NULL, &swapChain1) != S_OK ||
            swapChain1->QueryInterface(IID_PPV_ARGS(&g_pSwapChain)) != S_OK)
            return false;
        swapChain1->Release();
        dxgiFactory->Release();
        g_pSwapChain->SetMaximumFrameLatency(NUM_BACK_BUFFERS);
        g_hSwapChainWaitableObject = g_pSwapChain->GetFrameLatencyWaitableObject();
    }

    CreateRenderTarget();
    return true;
}

void CleanupDeviceD3D()
{
    CleanupRenderTarget();
    if (g_pSwapChain) { g_pSwapChain->Release(); g_pSwapChain = NULL; }
    if (g_hSwapChainWaitableObject != NULL) { CloseHandle(g_hSwapChainWaitableObject); }
    for (UINT i = 0; i < NUM_FRAMES_IN_FLIGHT; i++)
        if (g_frameContext[i].CommandAllocator) { g_frameContext[i].CommandAllocator->Release(); g_frameContext[i].CommandAllocator = NULL; }
    if (g_pd3dCommandQueue) { g_pd3dCommandQueue->Release(); g_pd3dCommandQueue = NULL; }
    if (g_pd3dCommandList) { g_pd3dCommandList->Release(); g_pd3dCommandList = NULL; }
    if (g_pd3dRtvDescHeap) { g_pd3dRtvDescHeap->Release(); g_pd3dRtvDescHeap = NULL; }
    if (g_pd3dSrvDescHeap) { g_pd3dSrvDescHeap->Release(); g_pd3dSrvDescHeap = NULL; }
    if (g_fence) { g_fence->Release(); g_fence = NULL; }
    if (g_fenceEvent) { CloseHandle(g_fenceEvent); g_fenceEvent = NULL; }
    if (g_pd3dDevice) { g_pd3dDevice->Release(); g_pd3dDevice = NULL; }

#ifdef DX12_ENABLE_DEBUG_LAYER
    IDXGIDebug1* pDebug = NULL;
    if (SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&pDebug))))
    {
        pDebug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_SUMMARY);
        pDebug->Release();
    }
#endif
}

void CreateRenderTarget()
{
    for (UINT i = 0; i < NUM_BACK_BUFFERS; i++)
    {
        ID3D12Resource* pBackBuffer = NULL;
        g_pSwapChain->GetBuffer(i, IID_PPV_ARGS(&pBackBuffer));
        g_pd3dDevice->CreateRenderTargetView(pBackBuffer, NULL, g_mainRenderTargetDescriptor[i]);
        g_mainRenderTargetResource[i] = pBackBuffer;
    }
}

void CleanupRenderTarget()
{
    WaitForLastSubmittedFrame();

    for (UINT i = 0; i < NUM_BACK_BUFFERS; i++)
        if (g_mainRenderTargetResource[i]) { g_mainRenderTargetResource[i]->Release(); g_mainRenderTargetResource[i] = NULL; }
}

void WaitForLastSubmittedFrame()
{
    FrameContext* frameCtxt = &g_frameContext[g_frameIndex % NUM_FRAMES_IN_FLIGHT];

    UINT64 fenceValue = frameCtxt->FenceValue;
    if (fenceValue == 0)
        return; // No fence was signaled

    frameCtxt->FenceValue = 0;
    if (g_fence->GetCompletedValue() >= fenceValue)
        return;

    g_fence->SetEventOnCompletion(fenceValue, g_fenceEvent);
    WaitForSingleObject(g_fenceEvent, INFINITE);
}

FrameContext* WaitForNextFrameResources()
{
    UINT nextFrameIndex = g_frameIndex + 1;
    g_frameIndex = nextFrameIndex;

    HANDLE waitableObjects[] = { g_hSwapChainWaitableObject, NULL };
    DWORD numWaitableObjects = 1;

    FrameContext* frameCtxt = &g_frameContext[nextFrameIndex % NUM_FRAMES_IN_FLIGHT];
    UINT64 fenceValue = frameCtxt->FenceValue;
    if (fenceValue != 0) // means no fence was signaled
    {
        frameCtxt->FenceValue = 0;
        g_fence->SetEventOnCompletion(fenceValue, g_fenceEvent);
        waitableObjects[1] = g_fenceEvent;
        numWaitableObjects = 2;
    }

    WaitForMultipleObjects(numWaitableObjects, waitableObjects, TRUE, INFINITE);

    return frameCtxt;
}

void ResizeSwapChain(HWND hWnd, int width, int height)
{
    DXGI_SWAP_CHAIN_DESC1 sd;
    g_pSwapChain->GetDesc1(&sd);
    sd.Width = width;
    sd.Height = height;

    IDXGIFactory4* dxgiFactory = NULL;
    g_pSwapChain->GetParent(IID_PPV_ARGS(&dxgiFactory));

    g_pSwapChain->Release();
    CloseHandle(g_hSwapChainWaitableObject);

    IDXGISwapChain1* swapChain1 = NULL;
    dxgiFactory->CreateSwapChainForHwnd(g_pd3dCommandQueue, hWnd, &sd, NULL, NULL, &swapChain1);
    swapChain1->QueryInterface(IID_PPV_ARGS(&g_pSwapChain));
    swapChain1->Release();
    dxgiFactory->Release();

    g_pSwapChain->SetMaximumFrameLatency(NUM_BACK_BUFFERS);

    g_hSwapChainWaitableObject = g_pSwapChain->GetFrameLatencyWaitableObject();
    assert(g_hSwapChainWaitableObject != NULL);
}

// Forward declare message handler from imgui_impl_win32.cpp
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// Win32 message handler
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
        return true;

    switch (msg)
    {
    case WM_SIZE:
        if (g_pd3dDevice != NULL && wParam != SIZE_MINIMIZED)
        {
            WaitForLastSubmittedFrame();
            ImGui_ImplDX12_InvalidateDeviceObjects();
            CleanupRenderTarget();
            ResizeSwapChain(hWnd, (UINT)LOWORD(lParam), (UINT)HIWORD(lParam));
            CreateRenderTarget();
            ImGui_ImplDX12_CreateDeviceObjects();
        }
        return 0;
    case WM_SYSCOMMAND:
        if ((wParam & 0xfff0) == SC_KEYMENU) // Disable ALT application menu
            return 0;
        break;
    case WM_DESTROY:
        ::PostQuitMessage(0);
        return 0;
    }
    return ::DefWindowProc(hWnd, msg, wParam, lParam);
}
