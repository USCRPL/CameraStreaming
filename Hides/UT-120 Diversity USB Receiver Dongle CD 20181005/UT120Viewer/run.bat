@echo off
rem Starts UT-120 Viewer. Optional argument: a .ts recording to replay.
cd /d "%~dp0"
if not exist .venv\Scripts\python.exe (
    py -3.12 -m venv .venv
    .venv\Scripts\python -m pip install -r requirements.txt
)
if not exist engine\ut120_engine.exe call build_engine.bat
start "" .venv\Scripts\pythonw.exe ut120_viewer.py %*
