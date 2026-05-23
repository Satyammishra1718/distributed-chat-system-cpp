@echo off
title Distributed Chat Server Node 2
echo Launching Chat Server Node 2 on Port 9002...
cd %~dp0\..

if exist build\main_server.exe (
    build\main_server.exe configs\server_2.cfg
) else if exist build\Debug\main_server.exe (
    build\Debug\main_server.exe configs\server_2.cfg
) else if exist build\Release\main_server.exe (
    build\Release\main_server.exe configs\server_2.cfg
) else (
    echo Error: main_server.exe not found! Please build the project using CMake first.
)
pause