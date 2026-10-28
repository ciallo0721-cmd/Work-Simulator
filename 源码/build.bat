@echo off
chcp 65001 >nul 2>nul
setlocal
cd /d "%~dp0"

echo ============================================
echo   打工模拟器 v5.0  TUI 版  构建脚本
echo ============================================
echo.
echo 注意：本版本必须用 UTF-8 编译。
echo       绝对不要加 -fexec-charset=GBK ，
echo       否则会报 Illegal byte sequence 并连带
echo       满屏 "tui_stream_reset was not declared".
echo.

set "GXX="
for %%P in (g++.exe) do if not defined GXX if not "%%~$PATH:P"=="" set "GXX=%%~$PATH:P"
if not defined GXX if exist "G:\EmoScan Pro\toolchain\w64devkit\bin\g++.exe" set "GXX=G:\EmoScan Pro\toolchain\w64devkit\bin\g++.exe"
if not defined GXX if exist "C:\msys64\mingw64\bin\g++.exe" set "GXX=C:\msys64\mingw64\bin\g++.exe"
if not defined GXX if exist "C:\MinGW\bin\g++.exe" set "GXX=C:\MinGW\bin\g++.exe"

if not defined GXX (
    echo [错误] 找不到 g++。请安装 MinGW-w64，或把 g++.exe 所在目录加进 PATH。
    echo.
    pause
    exit /b 1
)

echo 编译器: %GXX%
echo.
"%GXX%" -O2 -std=c++14 -o "打工模拟器.exe" "打工模拟器.cpp" -lm
if errorlevel 1 (
    echo.
    echo [编译失败] 请把上面的错误贴出来
    pause
    exit /b 1
)

echo.
echo [编译成功] 打工模拟器.exe
echo.
pause
