@echo off
echo Starting MAME with Hybrid Debugger (Win32 UI + TCP)...
echo ==========================================
echo Game: Quiz ^& Dragons (qadjr)
echo Debugger: Windows UI + TCP on localhost:12345
echo ==========================================
echo.
echo The debugger UI will appear AND the TCP server will listen.
echo Start Claude Code in the mame project folder to connect.
echo.
cd /d "H:\_DEV\mame"
cps1test.exe qadjr -rompath "H:\_DEV\qadjr-builder\rom" -debug -debugger windows -debugger_port 12345 -verbose -log
pause
