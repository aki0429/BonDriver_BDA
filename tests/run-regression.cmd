@echo off
rem Synthetic regression tests (no tuner needed). Run from an x64 Native Tools Command Prompt.
rem Default toolset is v142. With another Visual Studio set PLATFORM_TOOLSET first (e.g. v145).
setlocal
cd /d "%~dp0.."
if "%PLATFORM_TOOLSET%"=="" set PLATFORM_TOOLSET=v142
if not exist build\test mkdir build\test
msbuild projects\BaseClasses\BaseClasses.vcxproj /m /nologo /v:minimal /p:Configuration=Release_Static /p:Platform=x64 /p:PlatformToolset=%PLATFORM_TOOLSET% /p:OutDir=%CD%\build\test\lib\ /p:IntDir=%CD%\build\test\obj\base\
if errorlevel 1 exit /b %errorlevel%
cl /nologo /EHsc /std:c++17 /MT /DUNICODE /D_UNICODE /FIstdio.h /FIshare.h /FIcstring /Icommon /IBonDriver_BDA /I3rdParties\Windows-classic-samples\Samples\Win7Samples\multimedia\directshow\baseclasses tests\stream_regression.cpp BonDriver_BDA\TS_BUFF.cpp BonDriver_BDA\tswriter.cpp /Fobuild\test\ /Febuild\test\stream_regression.exe /link /LIBPATH:build\test\lib strmbase.lib strmiids.lib ole32.lib winmm.lib uuid.lib user32.lib oleaut32.lib
if errorlevel 1 exit /b %errorlevel%
build\test\stream_regression.exe
