@echo off
@REM Build for Visual Studio compiler. Run your copy of vcvars32.bat or vcvarsall.bat to setup command-line compiler.
mkdir Debug

rem setar o dir padrao do cl e das sdks pra não precisar rodar o vc antes
rem original 
@REM cl /nologo /Zi /MD /I .. /I ..\.. /I "%WindowsSdkDir%Include\um" /I "%WindowsSdkDir%Include\shared" /D UNICODE /D _UNICODE *.cpp ..\imgui_impl_dx12.cpp ..\imgui_impl_win32.cpp ..\..\imgui*.cpp /FeDebug/example_win32_directx12.exe /FoDebug/ /link d3d12.lib d3dcompiler.lib dxgi.lib

rem expandindo *
rem cl /nologo /Zi /MD /I .. /I ..\.. /I "%WindowsSdkDir%Include\um" /I "%WindowsSdkDir%Include\shared" /D UNICODE /D _UNICODE main.cpp  ..\imgui_impl_dx12.cpp ..\imgui_impl_win32.cpp ..\..\imgui.cpp ..\..\imgui_demo.cpp ..\..\imgui_draw.cpp ..\..\imgui_widgets.cpp  /FeDebug/example_win32_directx12.exe /FoDebug/ /link d3d12.lib d3dcompiler.lib dxgi.lib

set compiler_options=/nologo /Zi /MD /I .. /I ..\.. /I "%WindowsSdkDir%Include\um" /I "%WindowsSdkDir%Include\shared" /D UNICODE /D _UNICODE
@REM set debug_options=
set debug_options=/FeDebug/example_win32_directx12.exe /FoDebug/
set link_options=/link d3d12.lib d3dcompiler.lib dxgi.lib
rem if exist mainnnn.pch (
rem 	rem use pch
rem 	echo usando pch
rem    cl  %compiler_options% /Yu main.pch main.cpp %debug_options% %link_options%
rem ) else (
rem    rem create pch
rem    echo nao usando pch
rem    cl %compiler_options%  /Yc pch.cpp ^
rem                           %debug_options% %link_options% /ENTRY:mainCRTStartup 
rem )
cl %compiler_options% ..\imgui_impl_dx12.cpp ^
                      ..\imgui_impl_win32.cpp ^
                       ..\..\imgui.cpp ^
                       ..\..\imgui_demo.cpp ^
                      ..\..\imgui_draw.cpp ^
                      ..\..\imgui_widgets.cpp ^
                      main.cpp ^
                      %debug_options% %link_options%

