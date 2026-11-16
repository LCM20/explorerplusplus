@echo off
setlocal
set "MSBUILD=C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\MSBuild\Current\Bin\MSBuild.exe"
cd /d "%~dp0"
set "SLNDIR=%CD%\Explorer++/"
set "VCPKGINST=%CD:\=/%/vcpkg_installed/"

echo === Building Explorer++ Release x64 (VS2022 / v143) ===
"%MSBUILD%" "Explorer++\Explorer++\Explorer++.vcxproj" /p:Configuration=Release /p:Platform=x64 /p:SolutionDir="%SLNDIR%" /p:VcpkgInstalledDir="%VCPKGINST%" /m /v:m
if errorlevel 1 goto fail

echo === Building ZH_CN translation Release x64 ===
"%MSBUILD%" "Translations\Explorer++ZH_CN\Explorer++ZH_CN.vcxproj" /p:Configuration=Release /p:Platform=x64 /p:SolutionDir="%SLNDIR%" /m /v:m
if errorlevel 1 goto fail

echo === Copying ZH_CN translation DLL next to the exe ===
copy /y "Explorer++\x64\Release\Explorer++ZH_CN.dll" "Explorer++\Explorer++\x64\Release\" >nul

echo.
echo === BUILD SUCCEEDED ===
echo EXE: %CD%\Explorer++\Explorer++\x64\Release\Explorer++.exe
goto end

:fail
echo.
echo === BUILD FAILED ===

:end
pause
