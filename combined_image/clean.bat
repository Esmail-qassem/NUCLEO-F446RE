@echo off
SET COMBINED_DIR=%cd%
cd ..\BM\Build\
Call m clean
cd %COMBINED_DIR%
cd ..\BTLD\Build\
Call m clean
cd %COMBINED_DIR%
cd ..\APP\Build
Call m clean
cd %COMBINED_DIR%

