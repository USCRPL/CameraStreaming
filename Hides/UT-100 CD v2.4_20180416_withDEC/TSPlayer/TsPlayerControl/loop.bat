:loop
TsPlayerControl.exe setfile "F:\ts\test.ts"
rem wait for TSPlayer loading and analyzing the file
TsPlayerControl.exe wait 5000

TsPlayerControl.exe set 6 533000 1 1 3 1 0
TsPlayerControl.exe setvbr 0
TsPlayerControl.exe setfilerate 8952942
TsPlayerControl.exe run
rem wait for the whole TS file transmission is done
TsPlayerControl.exe wait 62000

TsPlayerControl.exe stop
rem schedule the next play time
TsPlayerControl.exe waittill 2014:06:25:17:27:25

TsPlayerControl.exe setfile "F:\ts\TS_473000KHz_6M_20130520_1042_1.ts"
TsPlayerControl.exe wait 5000

TsPlayerControl.exe set 6 473000 2 0 2 0 0
TsPlayerControl.exe setvbr 1
TsPlayerControl.exe run
TsPlayerControl.exe wait 62000

TsPlayerControl.exe stop
TsPlayerControl.exe wait 2000

goto loop
