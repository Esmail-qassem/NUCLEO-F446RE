@echo off
SET COMBINED_DIR=%cd%
cd ..\BM\Build\
Call m -j
cd %COMBINED_DIR%
cd ..\BTLD\Build\
Call m -j
cd %COMBINED_DIR%
cd ..\APP\Build
Call m -j
cd %COMBINED_DIR%
srec_cat C:\NUCLEO-F446RE\BM\Tools\BM.bin -Binary -offset 0x08000000 C:\NUCLEO-F446RE\BTLD\Tools\BTLD.bin -Binary -offset 0x08004000 C:\NUCLEO-F446RE\APP\Tools\application.bin -Binary -offset 0x08008000 -o full_image.hex -Intel
echo hex generated
