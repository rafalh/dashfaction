@echo off
call config.cmd

%PREMAKE5% gmake
pause
