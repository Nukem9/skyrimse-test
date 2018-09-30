echo Press any key to clean project directory
pause

Remove-Item -Path .\Build -Recurse
Remove-Item -Path .\x64 -Recurse
Remove-Item -Path .\x86 -Recurse
Remove-Item -Path .\shader_analyzer\obj -Recurse