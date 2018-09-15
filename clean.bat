@echo off

echo Press any key to clean project directory
pause > NUL
@echo on

rmdir /S /Q .\x64
rmdir /S /Q .\Dependencies\x64
rmdir /S /Q .\shader_analyzer\obj
rmdir /S /Q .\skyrim64_test\x64
rmdir /S /Q .\skyrim64_tls_mt\x64