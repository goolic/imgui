@echo off
@REM Build for Visual Studio compiler. Run your copy of vcvars32.bat or vcvarsall.bat to setup command-line compiler.

rem
rem user configs
rem

set clean=true
set usepch=false
set release=false
set imguidir=Z:

rem
rem initialization 
rem

set link_options=
set pchfile=
set debug_compile_options=
set debug_link_options=
set release_compile_options=
set release_link_options=
set compiler=
set linker=


rem
rem default configs
rem

set compile_options=/c /GS /W4 /Zc:wchar_t /Gm- /Od /Zc:inline /fp:precise /errorReport:prompt 
set compile_options=%compile_options% /WX- /Zc:forScope /RTC1 /Gd /std:c++14 /EHsc /diagnostics:column 
set compile_options=%compile_options% /Zi /nologo /MP /D UNICODE /D _UNICODE 

set includes=/I %imguidir%\ /I %imguidir%\examples /I %imguidir%\examples\example_win32_directx12 
set includes=%includes% /I "%WindowsSdkDir%Include\um" /I "%WindowsSdkDir%Include\shared" 

set d3d_link_deps=d3d12.lib d3dcompiler.lib dxgi.lib
set imgui_link_deps=%buildDir%\imgui_impl_dx12.obj ^
%buildDir%\imgui_impl_win32.obj ^
%buildDir%\imgui.obj ^
%buildDir%\imgui_demo.obj ^
%buildDir%\imgui_draw.obj ^
%buildDir%\imgui_widgets.obj

set link_options=/nologo /incremental /verbose:incr /debug:fastlink /DEBUG %d3d_link_deps% %imgui_link_deps% 

set minimal_files=%imguidir%\examples\imgui_impl_dx12.cpp ^
%imguidir%\examples\imgui_impl_win32.cpp ^
%imguidir%\imgui.cpp ^
%imguidir%\imgui_demo.cpp ^
%imguidir%\imgui_draw.cpp ^
%imguidir%\imgui_widgets.cpp

set compiler=cl
set linker=link

rem
rem lets set the debug or release modes
rem

rem DEBUG
if %release% equ false (
	set buildDir=%imguidir%\examples\example_win32_directx12\Debug
	set pchfile=%buildDir%\pch.pch

	set debug_compile_options=/MTd /Fd%buildDir%\\brcalczi.pdb /Fe%buildDir%\brcalc.exe /Fo"%buildDir%\"
	set debug_link_options=libcpmtd.lib libcmtd.lib libvcruntimed.lib libucrtd.lib 
	
	set compile_options=%compile_options% %includes% %debug_compile_options%
	set link_options=%link_options% %debug_link_options%

	echo !!!
	echo !!!
	echo !!!
	echo DENTRO DO DEBUG

	echo %compile_options%
)

	echo !!!
	echo !!!
	echo !!!
	echo FORA  DO DEBUG

	echo %compile_options%

rem RELEASE
if %release% equ true (
	set buildDir=%imguidir%\examples\example_win32_directx12\Release
	set pchfile=%buildDir%\pch.pch

	set release_compile_options=/MT /FdRelease\brcalczi.pdb /FeRelease\brcalc.exe /Fo"%buildDir%\"
	set release_link_options=libcpmt.lib libcmt.lib libvcruntime.lib libucrtd.lib 

	set compile_options=%compile_options% %includes% %release_compile_options%
	set link_options=%link_options% %release_link_options%
)



rem
rem Clean Output Dir and remake it
rem 

if %clean% equ true (
	echo !!!!
	echo !!!! Deletando pch
	echo !!!! 
	echo buildDir:%buildDir%

	rem call del /s/q %buildDir%\
	rem call rmdir /s/q %buildDir%\
)
if not exist %buildDir% (
	call mkdir %buildDir%\
)

rem FULL COMPILE
if %usepch% equ false (
	set compile_options=%compile_options% ^
%minimal_files% ^
%imguidir%\examples\example_win32_directx12\main.cpp
rem %imguidir%\examples\example_win32_directx12\imgui_impl_softraster.cpp
)

rem CREATE PCH
if %usepch% equ false (
	if exist  %pchfile%  (

	set compile_options=%compile_options% ^
/Ycpch.h /Fp%pchfile% ^
%minimal_files%
	)
)

rem USE PCH
if %usepch% equ true (
	if exist  %pchfile%  (

		set compile_options=%compile_options%	^
/Yupch.h /Fp%pchfile% ^
%imguidir%\examples\example_win32_directx12\main.cpp

	)
)

@echo on
rem NOW COMPILE
call %compiler% %compile_options%

rem NOW LINK
call %linker% %link_options%