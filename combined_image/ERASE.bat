@echo off
SET COMBINED_DIR=%cd%
cd C:/Program Files/STMicroelectronics/STM32Cube/STM32CubeProgrammer/bin
call STM32_Programmer_CLI.exe -c port=swd -e all
cd %COMBINED_DIR%
