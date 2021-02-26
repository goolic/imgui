#include "pch.h"
// TODO:

// operands deve ser

//Vamos fazer essa porra fazer calculos básicos e passar testes. 
//Chegando nesse ponto a gente se preocupa em deixar mais bonito e sofisticado.

// OP ENUM has to be an element of ops

// when result is achieved we need to ++ cursor and do a new operation

// assembleOperator and operateOrContinue need to return "struct operation"????

// we need to query the window size and use it as an input to SetNextWindowSize

// make a new windows handler and put the imgui stuff inside it.

// we need to stop using adddigittodisplay and alway print x, y or result to the display buffer

// implement calc via writing and hitting enter

// implement android version

// implement softraster version
// http://git.2f30.org/fortify-headers/file/README.html
// investigate fuzzers
#define STB_SPRINTF_IMPLEMENTATION
#include "stb_sprintf.h"

#define GB_IMPLEMENTATION
//#define GB_PLATFORM experimental
#include "gb.h"
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

//
// UI Data structs
struct FrameContext
{
    ID3D12CommandAllocator* CommandAllocator;
    UINT64                  FenceValue;
};

//
//BRCalc Data structs
#define BUFFER_MAX 1024
#define ARR_MAX 32

BETTER_ENUM (OPERATION, u8,
    NONE,
    MULTIPLICATION,
    DIVISION,
    SUM,
    SUBTRACTION,
    EQUALS,
    SOMETHING,
    COMMA_FOUND,
    COMMA_SET)
#define OP OPERATION

BETTER_ENUM (STATE, u8,
    FIRST_OPERAND,
    FIRST_OPER_FRACTION,
    SECOND_OPERAND,
    SECOND_OPER_FRACTION,
    RESULT_OBTAINED)
#define ST STATE

BETTER_ENUM (BASE, u8,
    NO_BASE,
    BINARY=2,
    OCTAL=8,
    DECIMAL=10,
    HEXADECIMAL=16)

arr {
    u8 item[ARR_MAX] = { 0 };
    u8 size = 0;
    u8 commaPosition;
    bool hasComma;
};

struct operand {
    u64 listOfDigits[ARR_MAX-1] = { 0 };
    f64 value;
    u64 size = 0;
    u64 commaPosition;
    bool hasComma;
};

struct historyNew {
    f64 resultBuffer = 0.0;
    BASE base= BASE::DECIMAL;
    OP op = OP::NONE;
    bool readingNewOperand = true;
    struct operand* operands; //[ARR_MAX-1] = { 0 };
};

struct operation {
    f64 x = 0;
    f64 y = 0;
    f64 result = 0;
    BASE base= BASE::DECIMAL;
    arr xC;
    arr yC;
    OP op = OP::NONE;
    ST state = ST::FIRST_OPERAND;
};

//
//BENCHMARK DATA
static __int64                g_Time;
static __int64                g_TicksPerSecond;
static __int64                g_TimeAtStartup;

//
// Data for the DX backend
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

//
// BRCalc data
static  gbAllocator all = gb_heap_allocator();
static char   buf[U8_MAX-1] =  "";
static f64 accumulator = 0;
static char arrayS[U8_MAX-1] = "";
static gbString print_buf = gb_string_make_length(all, "", BUFFER_MAX);
static struct historyNew* history_of_operands = (struct historyNew*) gb_alloc((all), ((ARR_MAX-1)*gb_size_of(struct historyNew)));

static int ret;


#ifndef STARTUP_BENCHMARK 
    #define STARTUP_BENCHMARK
    
    static __int64                  g_AppCreationTime;
    static __int64                  g_AppDestructionTime;
    
#endif
//The line bellow deactivates this benchmark
#undef STARTUP_BENCHMARK

//
//Test the hell out of things !
#ifndef TEST_BRCALC 
    #define TEST_BRCALC

    void test_just_one_time();
#endif

    

// Forward declarations of helper functions
int softraster_main(int, char**);
char* arrayToString(arr * theArr);
char const* decimalSeparator = (char const*)',';
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

//imGui style
void StyleColorsDarkRed(ImGuiStyle* dst);
void StyleColorsLightGreen(ImGuiStyle* dst);




void addToDisplay(u8 digit, gbString& display) {
    //convert from an int to an ascii char
    digit = digit + 48;

    //TODO: kill this and use only refreshDisplay

    display = gb_string_append_length(display, (const char*) &digit, 1);
}

void refreshDisplay(double number, gbString& display) {
    //TODO: switch so that %d and %f are both possible
    //isize gb_snprintf(char *str, isize n, char const *fmt, ...) {
    gb_snprintf(display, gb_size_of(display), "%f", number);
}


void logProgress(const char* log){
    ret = 0;
    ret = stbsp_sprintf(print_buf, "%s", log);
    fwrite(print_buf, sizeof(char), ret, stdout);
}

void logProgress(struct operation& ops){
    ret = 0;
    auto xCstr = arrayToString(&ops.xC);
    auto yCstr = arrayToString(&ops.yC);
    // ret = stbsp_sprintf(print_buf, "x:\ty:\txC:\txC.size\ty:\txY.size\n");
    // fwrite(print_buf, sizeof(char), ret, stdout)
    // ret = stbsp_sprintf(print_buf, "%.3f\t%.3f\t%s\t%d\t%s\t%d", ops.x, ops.y, xCstr, ops.xC.size, yCstr, ops.yC.size);
    ret = stbsp_sprintf(print_buf, "\n*NOVO*\nx:  %.3f\ny:  %.3f\nop:  %s\nresult  %.3f\nxC: %s\nxC.size %d\nxy: %s\nxY.size %d\n",
                                            ops.x, ops.y,ops.op._to_string(), ops.result,    xCstr, ops.xC.size,  yCstr, ops.yC.size);
    fwrite(print_buf, sizeof(char), ret, stdout);
}

//@speed we unrool this loop, since we garantee that theArr is 0 initialized
char * arrayToString(arr * theArr) {
    static u16 i = 0;
    int ch = 0;
    for (i = 0; i != theArr->size; i = i + 1) {
        ch = theArr->item[i] + 48;
        arrayS[i] = (char)ch;
    }
    return arrayS;
}

// The objective of this function is to assemble an operator whenever there are multiple digits
f64 assembleOperator (u8 digit, struct operation& ops, gbString& display) {
    //pra ficar correto é necessário armazenar os digitos 
    //em um array e fazer pop deles na ordem reversa para 
    //que a unidade de cada casa esteja no lugar certo

    GB_ASSERT_MSG(ops.xC.size < ARR_MAX-1 || ops.yC.size < ARR_MAX-1, "Number of elements can't be higher than ARR_MAX");

    //se o usuario clicar em 1 botao temos de multiplicar o valor 
    //pela casa decimal em que estamos, adicionando o valor.
    //se clicar 7 operand recebe 7*1, se clicar em seguida
    //no 8 operand recebe 7*10 + 8*1 e se por fim clicar em 3,
    //recebera 7*100 + 8*10 + 3 * 1
    // queremos que primeiro os digitos sejam adicionados
    //em um array, e uma vez que a operação for selecionada
    // os digitos tem que ser popped do array e receberem
    // n zeros e somados ao operand
    {
        u16 cursor = 0;
        accumulator = 0;
        u16 powOrder = 0;
        u8 powStopCondition = 0;

        if (ops.state == +ST::FIRST_OPERAND) {
            cursor = ops.xC.size-1;
            powStopCondition = ops.xC.size;
        }
        if (ops.state == +ST::SECOND_OPERAND) {
            powStopCondition = ops.yC.size;
            cursor = ops.yC.size-1;
        }
            
        while(true) {
            accumulator = (accumulator + (f64)(ops.xC.item[cursor] * pow(ops.base, powOrder)));

            if (cursor == 0) break;
            if (powOrder == powStopCondition) break;

            powOrder = powOrder + 1;
            cursor = cursor - 1;
        }

        if (ops.state == +ST::FIRST_OPERAND)
            ops.x = accumulator;

        if (ops.state == +ST::SECOND_OPERAND)
            ops.y = accumulator;
    }

    if (digit == 5)
        printf("stop here\n");

    if (ops.state == +ST::FIRST_OPERAND) {
        ops.xC.item[ops.xC.size] = digit;
        ops.xC.size = ops.xC.size + 1;
        // ops.x = (ops.x + (double)(digit * pow(10, ops.e)));

        gb_string_clear(display);
        display = gb_string_append_fmt(display, "%f", ops.x);
        logProgress(ops);

        return ops.x;
    }
    if (ops.state == +ST::SECOND_OPERAND) {
        ops.yC.item[ops.yC.size] = digit;
        ops.yC.size = ops.yC.size + 1;
        // ops.y = (ops.x + (double)(digit * pow(10, ops.e)));

        gb_string_clear(display);
        display = gb_string_append_fmt(display, "%f", ops.y);
        logProgress(ops);

        return ops.y;
    }
        
    return NULL;
}

//if its a comma we save the current position else we do the operation or go to the second operand
void operateOrContinue(OP op, struct operation& ops, gbString& display) {

    if (op == +OP::COMMA_FOUND) {//continue ops.state, set ops.xyC.commaPosition to ops.xyC.size
        //@robustness we need to see if the commaposition is not off by one
        if (ops.state == +ST::FIRST_OPERAND) {
                ops.xC.commaPosition = ops.xC.size;
                display = gb_string_append_length(display, decimalSeparator, 1);
                op = +OP::COMMA_SET;
                logProgress(ops);
            }
        if (ops.state == +ST::SECOND_OPERAND) {
            ops.yC.commaPosition = ops.yC.size;
            display = gb_string_append_length(display, decimalSeparator, 1);
            op = +OP::COMMA_SET;
            logProgress(ops);
            }
        
    }
    
    if ((ops.state == +ST::SECOND_OPERAND) && (op != +OP::COMMA_FOUND)) {
        assert(ops.op != +OP::NONE);

        if (op == +OP::SUM || ops.op == +OP::SUM)
        {
            ops.result = ops.x + ops.y;
            gb_string_clear(display);
            display = gb_string_append_fmt(display, "%f", ops.result);
            ops.state = ST::RESULT_OBTAINED;
            logProgress(ops);
        }
        if (op == +OP::SUBTRACTION || ops.op == +OP::SUBTRACTION)
        {
            ops.result = ops.x - ops.y;
            gb_string_clear(display);
            display = gb_string_append_fmt(display, "%f", ops.result);
            ops.state = ST::RESULT_OBTAINED;
            logProgress(ops);
        }
        if (op == +OP::MULTIPLICATION || ops.op == +OP::MULTIPLICATION)
        {
            ops.result = ops.x * ops.y;
            gb_string_clear(display);
            display = gb_string_append_fmt(display, "%f", ops.result);
            ops.state = ST::RESULT_OBTAINED;
            logProgress(ops);
        }
        if (op == +OP::DIVISION || ops.op == +OP::DIVISION)
        {
            ops.result = (double) ops.x/ops.y;
            gb_string_clear(display);
            display = gb_string_append_fmt(display, "%f", ops.result);
            ops.state = ST::RESULT_OBTAINED;
            // ops.result += (double) ops.x%ops.y;
            logProgress(ops);
        }
        if (op == +OP::EQUALS || ops.state != +ST::RESULT_OBTAINED)
        {
            display = gb_string_append_fmt(display, "%f", ops.result);
            // ops.result += (double) ops.x%ops.y;
            ops.state = ST::RESULT_OBTAINED;
            logProgress(ops);
        }
    }

    if ((ops.state == +ST::FIRST_OPERAND) && (op != +OP::COMMA_FOUND)) {
        ops.state = +ST::SECOND_OPERAND;
        ops.op = op;
        logProgress("cá ixtou!\n");
        gb_string_clear(display);
        addToDisplay(0, display);
    }

    
}

// Main code
int main(int, char**) {
    #ifdef STARTUP_BENCHMARK 
    if (!::QueryPerformanceCounter((LARGE_INTEGER*)&g_AppCreationTime))
                return false;
    #endif

    //isso foi meu chute de como fazer funcionar a versão android !
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


    //No borders:
    // //find border thickness
    //https://stackoverflow.com/a/39735058
    //https://docs.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-setwindowlongptra
    //
    //     SetRectEmpty(&border_thickness);
    //     if (GetWindowLongPtr(hwnd, GWL_STYLE) & WS_THICKFRAME)
    //     {
    //         AdjustWindowRectEx(&border_thickness, GetWindowLongPtr(hwnd, GWL_STYLE) & ~WS_CAPTION, FALSE, NULL);
    //         border_thickness.left *= -1;
    //         border_thickness.top *= -1;
    //     }
    //     else if (GetWindowLongPtr(hwnd, GWL_STYLE) & WS_BORDER)
    //     {
    //         SetRect(&border_thickness, 1, 1, 1, 1);
    //     }

    //     MARGINS margins = { 0 };
    //     DwmExtendFrameIntoClientArea(hwnd, &margins);
    //     SetWindowPos(hwnd, NULL, 0, 0, 0, 0, SWP_SHOWWINDOW | SWP_NOMOVE | SWP_NOSIZE | SWP_FRAMECHANGED)

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
    //ImFont* font = io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\ArialUni.ttf", 18.0f, NULL, io.Fonts->GetGlyphRangesJapanese());

    auto defaultFont = io.Fonts->AddFontFromFileTTF("fonts/Karla-Regular.ttf", 15.f);
    auto calcFont = io.Fonts->AddFontFromFileTTF("fonts/SourceCodePro-Black.ttf", 22.f);
    IM_ASSERT(defaultFont != NULL);
    IM_ASSERT(calcFont != NULL);
    
    // Our state
    bool show_demo_window = true;
    bool show_another_window = false;
    bool whichtheme = false;

    gbString display = gb_string_make(all, "");
    addToDisplay(0,display);

    static double result   = 0;
    struct operation history [1024];// = new();
    static __int16 cursor = 0;


    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

#ifdef TEST_BRCALC
    test_just_one_time();
#endif

    // Main loop
    MSG msg;
    ZeroMemory(&msg, sizeof(msg));//SecureZeroMemory https://docs.microsoft.com/en-us/previous-versions/windows/desktop/legacy/aa366877(v=vs.85)
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
            
            void          SetNextWindowPos(const ImVec2& pos, ImGuiCond cond = 0, const ImVec2& pivot = ImVec2(0, 0)); // set next window position. call before Begin(). use pivot=(0.5f,0.5f) to center on given point, etc.
            void          SetNextWindowSize(const ImVec2& size, ImGuiCond cond = 0);                  // set next window size. set axis to 0.0f to force an auto-fit on this axis. call before Begin()
    
            static RECT rect;
            static i32 width;
            static i32 height;

            if(GetWindowRect(hwnd, &rect))
            {
                width = rect.right - rect.left;
                height = rect.bottom - rect.top;
            }

            ImGui::SetNextWindowPos(ImVec2(0,0),0,ImVec2(0,0));
            ImGui::SetNextWindowSize(ImVec2((f32)width,(f32)height),0);

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
            ImGui::InputTextWithHint("", display, display, gb_strlen(display));

            if (ImGui::Button("7",   bSize))
                assembleOperator(7, history[cursor], display);
            ImGui::SameLine();

            if (ImGui::Button("8",   bSize)) 
                assembleOperator(8, history[cursor], display);
            ImGui::SameLine();

            if (ImGui::Button("9",   bSize)) 
                assembleOperator(9, history[cursor], display);
            ImGui::SameLine();

            if (ImGui::Button("X",   bSize)) 
                operateOrContinue(OP::MULTIPLICATION,history[cursor], display);

            if (ImGui::Button("4",   bSize)) 
                assembleOperator(4,history[cursor], display); 
            ImGui::SameLine();
            
            if (ImGui::Button("5",   bSize)) 
                assembleOperator(5,history[cursor], display); 
            ImGui::SameLine();
            
            if (ImGui::Button("6",   bSize)) 
                assembleOperator(6,history[cursor], display); 
            ImGui::SameLine();
            
            if (ImGui::Button("-",   bSize)) 
                operateOrContinue(OP::SUBTRACTION,history[cursor], display); 
            
            if (ImGui::Button("1",   bSize)) 
                assembleOperator(1,history[cursor], display); 
            ImGui::SameLine();
            
            if (ImGui::Button("2",   bSize)) 
                assembleOperator(2,history[cursor], display); 
            ImGui::SameLine();
            
            if (ImGui::Button("3",   bSize)) 
                assembleOperator(3,history[cursor], display); 
            ImGui::SameLine();
            
            if (ImGui::Button("+",   bSize)) 
                operateOrContinue(OP::SUM, history[cursor], display);
            
            if (ImGui::Button("+/-", bSize)) 
                operateOrContinue(OP::SOMETHING, history[cursor], display); 
            ImGui::SameLine();
            
            if (ImGui::Button("0",   bSize)) 
                assembleOperator(0, history[cursor], display); 
            ImGui::SameLine();
            
            if (ImGui::Button(",",   bSize)) 
                operateOrContinue(OP::COMMA_FOUND, history[cursor], display); 
            ImGui::SameLine();
            
            if (ImGui::Button("=",   bSize)) 
                operateOrContinue(OP::EQUALS, history[cursor], display);

            if (ImGui::Button("Binary",   bSize))
                history[cursor].base = +BASE::BINARY;
            ImGui::SameLine();

            if (ImGui::Button("Octal",   bSize))
                history[cursor].base = +BASE::OCTAL;
            ImGui::SameLine();

            if (ImGui::Button("Decimal",   bSize))
                history[cursor].base = +BASE::DECIMAL;
            ImGui::SameLine();

            if (ImGui::Button("Hexadecimal",   bSize))
                history[cursor].base = +BASE::HEXADECIMAL;



            ImGui::PopFont();

            ImGui::Text("counter = %d", counter);

            ImGui::Text("Application average \n%.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
            ImGui::End();

        }

        //*******************
        //*                 *
        //**** Rendering ****
        //*                 *
        //*******************
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

        
        //benchmark de startup, vai encerrar app depois de um único frame
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

    }//end of main loop



    WaitForLastSubmittedFrame();
    ImGui_ImplDX12_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    CleanupDeviceD3D();
    ::DestroyWindow(hwnd);
    ::UnregisterClass(wc.lpszClassName, wc.hInstance);

    return 0;
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
        for (u32 i = 0; i < NUM_BACK_BUFFERS; i++)
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

    for (u32 i = 0; i < NUM_FRAMES_IN_FLIGHT; i++)
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
    for (u32 i = 0; i < NUM_FRAMES_IN_FLIGHT; i++)
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
    for (u32 i = 0; i < NUM_BACK_BUFFERS; i++)
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

    for (u32 i = 0; i < NUM_BACK_BUFFERS; i++)
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

#ifdef TEST_BRCALC
void test_just_one_time() {

        struct operation history [1024];// = new();
        static __int16 cursor = 0;
        gbString display = gb_string_make(all, "");



        // assembleOperator(3,history[cursor], display); 
        // operateOrContinue(OP::SUM, history[cursor], display);
        // assembleOperator(3,history[cursor], display);
        // operateOrContinue(OP::EQUALS, history[cursor], display);

        assembleOperator(3,history[cursor], display); 
        operateOrContinue(OP::SUM, history[cursor], display);
        assembleOperator(4,history[cursor], display);
        operateOrContinue(OP::SUM, history[cursor], display);
        assembleOperator(5,history[cursor], display);
        assembleOperator(5,history[cursor], display);
        assembleOperator(5,history[cursor], display);
        operateOrContinue(OP::EQUALS, history[cursor], display);


        // assembleOperator(3,history[cursor], display); 
        // operateOrContinue(OP::MULTIPLICATION, history[cursor], display);
        // assembleOperator(3,history[cursor], display); 

        // assembleOperator(3,history[cursor], display); 
        // operateOrContinue(OP::DIVISION, history[cursor], display);
        // assembleOperator(3,history[cursor], display);

        // assembleOperator(3,history[cursor], display); 
        // operateOrContinue(OP::SUBTRACTION, history[cursor], display);
        // assembleOperator(3,history[cursor], display); 
    }
#endif

#undef TEST_BRCALC