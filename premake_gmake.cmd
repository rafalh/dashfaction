@echo off
call config.cmd

%PREMAKE4% gmake
pause
