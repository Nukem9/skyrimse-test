# Menu is at the end of the file

function Clean-ProjectDirectory
{
Remove-Item -Path .\Build -Recurse
Remove-Item -Path .\x64 -Recurse
Remove-Item -Path .\x86 -Recurse
Remove-Item -Path .\shader_analyzer\obj -Recurse
}

function Make-GeneralRelease
{
[Reflection.Assembly]::LoadWithPartialName("System.IO.Compression.FileSystem")
$compression = [System.IO.Compression.CompressionLevel]::Optimal

#
# DLLs
#
mkdir "Build"
copy "skyrim64_test.ini" "Build\skyrim64_test.ini"

cd "x64\Release"
copy "SSE_winhttp.dll" "..\..\Build\winhttp.dll"
copy "tbb.dll" "..\..\Build\tbb.dll"
copy "tbbmalloc.dll" "..\..\Build\tbbmalloc.dll"

cd ..
cd ..
[System.IO.Compression.ZipFile]::CreateFromDirectory("Build", "CK64Fixes Release X.zip", $compression, $false) # Don't include base dir

#
# FaceFXWrapper
#
mkdir "Tools"
mkdir "Tools\Audio"

$readMeInfo = @" 
#
# NOTICE
#
# While FaceFXWrapper does produce LIP files for the 64-bit CreationKit, it
# has not been validated for correctness. Use at your own risk.
#
# 'FaceFXWrapper.exe' must be in the directory '<SKYRIM_DIR>\Tools\Audio'.
# 
# REQUIREMENTS
#
# FonixData.cdf is a proprietary file required for FaceFX. You can obtain FonixData
# from the Fallout 3, Fallout 4, or Skyrim LE CreationKit files. It must be placed
# in '<SKYRIM_DIR>\Data\Sound\Voice\Processing'.
#
"@

$readMeInfo | Out-File -FilePath "Tools\Audio\README.txt" -Encoding ASCII

cd "x86\Release"
copy "FaceFXWrapper.exe" "..\..\Tools\Audio\FaceFXWrapper.exe"
cd ..
cd ..
[System.IO.Compression.ZipFile]::CreateFromDirectory("Tools", "FaceFXWrapper X.zip", $compression, $true) # Include base dir
Remove-Item -Path "Tools" -Recurse
}

function Write-VersionFile
{
$versionFileInfo = @" 
#pragma once

#define VER_CURRENT_COMMIT_ID "<COMMITID>"
#define VER_CURRENT_DATE "<DATE>"
"@

$commitId = (git rev-parse --short HEAD).ToUpper()
$currDate = (Get-Date)

$versionFileInfo = $versionFileInfo -Replace "<COMMITID>", $commitId
$versionFileInfo = $versionFileInfo -Replace "<DATE>", $currDate

$targetDir = "skyrim64_test\src\"

if (!(Test-Path -Path $targetDir)) {
    $targetDir = "src\"
}

$versionFileInfo | Out-File -FilePath ($targetDir + "version_info.h") -Encoding ASCII
}

# Check for params passed on the command line
$input = $args[0]

if ([string]::IsNullOrWhiteSpace($input)) {
    Write-Host "==================================="
    Write-Host "1: Clean project directory"
    Write-Host "2: Create release build archives"
    Write-Host "3: Write version file"
    Write-Host "==================================="

    $input = Read-Host "Selection"
}

switch ($input)
{
    '1' {
        cls
        Clean-ProjectDirectory
    } '2' {
        cls
        Make-GeneralRelease
    } '3' {
        cls
        Write-VersionFile
    }
}

exit