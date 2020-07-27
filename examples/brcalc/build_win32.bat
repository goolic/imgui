@echo off
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
set create_pch_compile_options=

rem
rem user configs
rem

set clean=false
set usepch=true
set release=false
set imguidir=..\..

rem
rem default configs
rem

set compile_options=/c /GS /W4 /Zc:wchar_t /Gm- /Zc:inline /fp:precise /errorReport:prompt
set compile_options=%compile_options% /WX- /Zc:forScope /Gd /std:c++14 /EHsc /diagnostics:column
set compile_options=%compile_options% /Zi /FS /nologo /D UNICODE /D _UNICODE

set includes=/I %imguidir%\ /I %imguidir%\examples /I %imguidir%\examples\brcalc
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
	set buildDir=debug

	set build_dependent_compile_options=/MTd /Od /RTC1
	rem  libcmtd.lib libcpmtd.lib libvcruntimed.lib
	set build_dependent_link_options=libucrtd.lib 
)

rem RELEASE
if %release% equ true (
	set buildDir=release

	set build_dependent_compile_options=/MT /O1
	rem libcpmt.lib libcmt.lib libvcruntime.lib 
	set build_dependent_link_options=libucrtd.lib
)

set pchfile=%buildDir%\pch.pch
set compile_options=%includes% %compile_options% %build_dependent_compile_options% /Fo"%buildDir%\\" /Fd%buildDir%\\brcalczi.pdb

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


rem
rem Set compile options per type of build
rem 

rem FULL COMPILE
if %usepch% equ false (
	set compile_options=%minimal_files% ^
%imguidir%\examples\brcalc\main.cpp ^
%compile_options%
rem %imguidir%\examples\brcalc\imgui_impl_softraster.cpp
)

rem CREATE PCH
rem if %usepch% equ true (
rem 	if not exist %pchfile%  (
rem 		rem pre-create pch file because microsoft is stupid
rem 		copy /y nul %pchfile%
rem 		)
rem 	)
if %usepch% equ true (
	if not exist %pchfile%  (

		call %compiler% /Ycpch.h /Fp"%pchfile%" %minimal_files% %compile_options%
	)
)

rem USE PCH
if %usepch% equ true (
	if exist %pchfile% (

		set compile_options=%imguidir%\examples\brcalc\main.cpp ^
/Yupch.h /Fp%pchfile% ^
%compile_options%
	)
	if not exist %pchfile% (echo NEED TO CREATE PCH FIRST)
)

rem
rem Set the obj inputs to the linker
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

set link_options= %imgui_link_deps% %build_dependent_link_options% %link_options% /OUT:"%buildDir%\brcalc.exe"

rem NOW COMPILE
call %compiler% /MP %compile_options%

rem NOW LINK
call %linker% %link_options%