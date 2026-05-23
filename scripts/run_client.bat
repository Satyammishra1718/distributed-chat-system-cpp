@echo off
title Distributed Chat Client
echo Launching Chat Client...
cd %~dp0\..

if exist build\main_client.exe (
    build\main_client.exe
) else if exist build\Debug\main_client.exe (
    build\Debug\main_client.exe
) else if exist build\Release\main_client.exe (
    build\Release\main_client.exe
) else (
    echo Error: main_client.exe not found! Please build the project using CMake first.
)
pause