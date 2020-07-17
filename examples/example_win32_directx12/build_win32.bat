@echo off
@REM Build for Visual Studio compiler. Run your copy of vcvars32.bat or vcvarsall.bat to setup command-line compiler.
mkdir debug

rem setar o dir padrao do cl e das sdks pra não precisar rodar o vc antes
rem original 
@REM cl /nologo /Zi /MD /I .. /I ..\.. /I "%WindowsSdkDir%Include\um" /I "%WindowsSdkDir%Include\shared" /D UNICODE /D _UNICODE *.cpp ..\imgui_impl_dx12.cpp ..\imgui_impl_win32.cpp ..\..\imgui*.cpp /FeDebug/example_win32_directx12.exe /FoDebug/ /link d3d12.lib d3dcompiler.lib dxgi.lib

rem expandindo *
rem cl /nologo /Zi /MD /I .. /I ..\.. /I "%WindowsSdkDir%Include\um" /I "%WindowsSdkDir%Include\shared" /D UNICODE /D _UNICODE main.cpp  ..\imgui_impl_dx12.cpp ..\imgui_impl_win32.cpp ..\..\imgui.cpp ..\..\imgui_demo.cpp ..\..\imgui_draw.cpp ..\..\imgui_widgets.cpp  /FeDebug/example_win32_directx12.exe /FoDebug/ /link d3d12.lib d3dcompiler.lib dxgi.lib

rem
rem user configs
rem

set release=false
set usepch=true
set imguidir=Z:
set pchfile=%imguidir%\examples\example_win32_directx12\debug\pch.pch

set release_compiler_options=/MT /Zi /Fdrelease\brcalczi.pdb /Ferelease\brcalc.exe /Forelease\
set debug_compiler_options=/MTd /Zi /Fddebug\brcalczi.pdb /Fedebug\brcalc.exe /Fodebug\
set compiler_options=/nologo /MP /D UNICODE /D _UNICODE
set includes=/I %imguidir%\ /I %imguidir%\examples /I %imguidir%\examples\example_win32_directx12 /I "%WindowsSdkDir%Include\um" /I "%WindowsSdkDir%Include\shared" 

rem set release_link_options=libcpmt.lib libcmt.lib libvcruntime.lib libucrtd.lib 
rem set debug_link_options=libcpmtd.lib libcmtd.lib libvcruntimed.lib libucrtd.lib 
set d3d_link_deps=d3d12.lib d3dcompiler.lib dxgi.lib
set imgui_link_deps=%imguidir%\examples\example_win32_directx12\debug\imgui_impl_dx12.obj ^
              	  %imguidir%\examples\example_win32_directx12\debug\imgui_impl_win32.obj ^
              	  %imguidir%\examples\example_win32_directx12\debug\imgui.obj ^
              	  %imguidir%\examples\example_win32_directx12\debug\imgui_demo.obj ^
              	  %imguidir%\examples\example_win32_directx12\debug\imgui_draw.obj ^
              	  %imguidir%\examples\example_win32_directx12\debug\imgui_widgets.obj
set link_options=/link %d3d_link_deps% %imgui_link_deps% /incremental /verbose:incr /debug:fastlink

set minimal_files=%imguidir%\examples\imgui_impl_dx12.cpp ^
              	  %imguidir%\examples\imgui_impl_win32.cpp ^
              	  %imguidir%\imgui.cpp ^
              	  %imguidir%\imgui_demo.cpp ^
              	  %imguidir%\imgui_draw.cpp ^
              	  %imguidir%\imgui_widgets.cpp

rem lets set the debug or release modes
if %release% equ false (
	set compiler_options=%compiler_options% %includes% %debug_compiler_options%
	set link_options=%link_options% %debug_link_options%
)
if %release% equ true (
	set compiler_options=%compiler_options% %includes% %release_compiler_options%
	set link_options=%link_options% %release_link_options%
)


if %usepch% equ false (
	rem
	rem echo full compile
	rem
	cl %compiler_options% ^
					%minimal_files% ^
              		%imguidir%\examples\example_win32_directx12\main.cpp ^
              		%imguidir%\examples\example_win32_directx12\imgui_impl_softraster.cpp ^
              		%link_options%

) 
if %usepch% equ true (
			rem
			rem echo deciding to use or create pch
			rem

			rem echo !!!!
			rem echo !!!! Deletando pch
			rem echo !!!! 
			rem del %pchfile%


			if exist  %pchfile%  (
				rem
				rem echo use pch
				rem

				cl %compiler_options%	^
								/Yupch.h /Fp%pchfile% ^
								%imguidir%\examples\example_win32_directx12\main.cpp ^
			                    %link_options%

			) 	else (
					rem 
					rem echo create pch
					rem

					cl %compiler_options% /c ^
										/Ycpch.h /Fp%pchfile% ^
										%minimal_files%
										rem %imguidir%\examples\example_win32_directx12\pch.cpp

					)
	)
)


