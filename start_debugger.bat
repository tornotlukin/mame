@echo off
echo Starting MAME with Remote Debugger...
echo ==========================================
echo Game: Quiz ^& Dragons (qadjr)
echo Debugger: Remote TCP on localhost:12345
echo ==========================================
echo.
echo Waiting for LLM connection on port 12345...
echo Start Claude Code in the mame project folder to connect.
echo.
cd /d "H:\_DEV\mame"
cps1test.exe qadjr -rompath "H:\_DEV\qadjr-builder\rom" -debug -debugger remote -debugger_port 12345
pause
