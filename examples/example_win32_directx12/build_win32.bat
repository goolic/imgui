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
rem if exist brcalc.pch (
rem 	rem use pch
rem 	echo usando pch
rem    cl  %compiler_options% /Yu brcalc.pch main.cpp %debug_options% %link_options%
rem ) else (
rem    rem create pch
rem    echo nao usando pch
rem 	cl %compiler_options%      ..\imgui_impl_dx12.cpp ^
rem                               ..\imgui_impl_win32.cpp ^
rem                               ..\..\imgui.cpp ^
rem                               ..\..\imgui_demo.cpp ^
rem                               ..\..\imgui_draw.cpp ^
rem                               ..\..\imgui_widgets.cpp ^
rem                           /Ycmain.cpp /Fpbrcalc.pch %debug_options% %link_options%
rem )
cl %compiler_options% ..\imgui_impl_dx12.cpp ^
                      ..\imgui_impl_win32.cpp ^
                      ..\..\imgui.cpp ^
                      ..\..\imgui_demo.cpp ^
                      ..\..\imgui_draw.cpp ^
                      ..\..\imgui_widgets.cpp ^
                      main.cpp ^
                      %debug_options% %link_options%

