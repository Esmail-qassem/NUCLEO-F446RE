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
srec_cat C:\STM_BLUEPILL\ST32\BM\Tools\BM.bin -Binary -offset 0x08000000 C:\STM_BLUEPILL\ST32\BTLD\Tools\BTLD.bin -Binary -offset 0x08001400 C:\STM_BLUEPILL\ST32\APP\Tools\application.bin -Binary -offset 0x08002800 -o full_image.hex -Intel
echo hex generated
