@echo off
SET COMBINED_DIR=%cd%
cd C:/Program Files/STMicroelectronics/STM32Cube/STM32CubeProgrammer/bin
call STM32_Programmer_CLI.exe -c port=swd -d %COMBINED_DIR%/full_image.hex 0x08000000 -v -rst
cd %COMBINED_DIR%
