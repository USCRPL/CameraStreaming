@echo off
rem Builds engine\ut120_engine.exe with the MSYS2 UCRT64 compiler.
rem engine\itesdk\ is ITE's IT913x demodulator API, copied unchanged from the
rem Linux SDK on the UT-120 CD (it913x_linux_v18.05.21.1), plus a small shim header.
setlocal
set PATH=C:\msys64\ucrt64\bin;%PATH%
cd /d "%~dp0engine"
if not exist obj mkdir obj
for %%f in (itesdk\*.c) do gcc -std=gnu99 -O2 -w -Iitesdk -c %%f -o obj\%%~nf.o || exit /b 1
gcc -std=gnu99 -O2 -Wall -Iitesdk -c ut120_hw.c -o obj\ut120_hw.o || exit /b 1
g++ -std=c++17 -O2 -Wall -static -o ut120_engine.exe ut120_engine.cpp obj\*.o -lwinusb -lsetupapi
