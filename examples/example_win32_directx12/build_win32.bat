@echo on
@REM Build for Visual Studio compiler. Run your copy of vcvars32.bat or vcvarsall.bat to setup command-line compiler.

rem
rem initialization 
rem

set link_options=
set pchfile=
set build_dependent_compile_options=
set build_dependent_link_options=
set compiler=
set linker=
set minimal_files=
set compile_options=
set includes=
set d3d_link_deps=
set imgui_link_deps=
set buildDir=
set imguidir=
set release=
set clean=
set usepch=

rem
rem user configs
rem

set clean=true
set usepch=false
set release=false
set imguidir=..\..

rem
rem default configs
rem

set compile_options=/c /GS /W4 /Zc:wchar_t /Gm- /Od /Zc:inline /fp:precise /errorReport:prompt
set compile_options=%compile_options% /WX- /Zc:forScope /RTC1 /Gd /std:c++14 /EHsc /diagnostics:column
set compile_options=%compile_options% /Zi /nologo /MP /D UNICODE /D _UNICODE

set includes=/I %imguidir%\ /I %imguidir%\examples /I %imguidir%\examples\example_win32_directx12
set includes=%includes% /I "%WindowsSdkDir%Include\um" /I "%WindowsSdkDir%Include\shared"

set d3d_link_deps=d3d12.lib d3dcompiler.lib dxgi.lib

set link_options=%d3d_link_deps% /nologo /incremental /verbose:incr /debug:fastlink /MACHINE:X64

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
	set buildDir=Debug

	set build_dependent_compile_options=/MTd
	rem  libcmtd.lib libcpmtd.lib libvcruntimed.lib
	set build_dependent_link_options=libucrtd.lib 
)

rem RELEASE
if %release% equ true (
	set buildDir=Release

	set build_dependent_compile_options=/MT
	set build_dependent_link_options=libcpmt.lib libcmt.lib libvcruntime.lib libucrtd.lib
)

rem
rem We need buildDir for this step
rem

rem Replace examples\ with "" 
set imgui_link_deps=%minimal_files:examples\=%

rem Replace cpp with obj 
set imgui_link_deps=%imgui_link_deps:cpp=obj%

rem We need some extra magic here
setlocal enabledelayedexpansion

rem Replace ..\..\imgui with %buildDir%\imgui
set imgui_link_deps=%imgui_link_deps:..\..\imgui=!buildDir!\imgui%
setlocal disabledelayedexpansion

set imgui_link_deps=%buildDir%\main.obj %imgui_link_deps%

rem
rem Finish setting options
rem

set compile_options=%includes% %compile_options% /Fo"%buildDir%\\" /Fd%buildDir%\\brcalczi.pdb

set link_options= %imgui_link_deps% %build_dependent_link_options% %link_options% /OUT:"%buildDir%\brcalc.exe"

set pchfile=%buildDir%\pch.pch

rem
rem Clean Output Dir and remake it
rem 

if %clean% equ true (
	rem echo !!!!
	rem echo !!!! Deletando buildDir
	rem echo !!!! 
	rem echo buildDir:%buildDir%

	rem call del /s/q %buildDir%\
	rem call rmdir /s/q %buildDir%\
)
if not exist %buildDir% (
	call mkdir %buildDir%\
)

rem FULL COMPILE
if %usepch% equ false (
	set compile_options=%minimal_files% ^
%imguidir%\examples\example_win32_directx12\main.cpp ^
%compile_options%
rem %imguidir%\examples\example_win32_directx12\imgui_impl_softraster.cpp

	echo !!!
	echo !!!
	echo !!!
	echo FULL COMPILE
)

rem CREATE PCH
if %usepch% equ false (
	if exist  %pchfile%  (

	set compile_options=%minimal_files% ^
	%compile_options% ^
/Ycpch.h /Fp%pchfile%


	echo !!!
	echo !!!
	echo !!!
	echo CREATE PCH
	)
)

rem USE PCH
if %usepch% equ true (
	if exist  %pchfile%  (

		set compile_options=%imguidir%\examples\example_win32_directx12\main.cpp ^
/Yupch.h /Fp%pchfile% ^
%compile_options%


	echo !!!
	echo !!!
	echo !!!
	echo USE PCH
	)
)

rem NOW COMPILE
call %compiler% %compile_options%

rem NOW LINK
call %linker% %link_options%