# Requires .NET 4.5
$MethodDefinition =
@'
    [DllImport("kernel32.dll", CharSet = CharSet.Unicode, SetLastError = true)]
    public static extern long WritePrivateProfileString(string Section, string Key, string Value, string FilePath);
'@
$WPPS = Add-Type -MemberDefinition $MethodDefinition -Name "Win32WPPS" -Namespace Win32Functions -PassThru

[Reflection.Assembly]::LoadWithPartialName("System.IO.Compression.FileSystem")
$Compression = [System.IO.Compression.CompressionLevel]::Optimal


#
# General release
#

mkdir "Build"
copy "skyrim64_test.ini" "Build\skyrim64_test.ini"

# Generate commit hash and shove it into the .ini
$commitId = (git rev-parse --short HEAD)
$WPPS::WritePrivateProfileString("Version", "CommitId", $commitId, $pwd.Path + "\Build\skyrim64_test.ini")

cd "x64\Release"
copy "winhttp.dll" "..\..\Build\winhttp.dll"
copy "tbb.dll" "..\..\Build\tbb.dll"
copy "tbbmalloc.dll" "..\..\Build\tbbmalloc.dll"

cd ..
cd ..
[System.IO.Compression.ZipFile]::CreateFromDirectory("Build", "CK64Fixes Release X.zip", $Compression, $false) # Don't include base dir

#
# FaceFXWrapper
#
mkdir "Tools"
mkdir "Tools\Audio"

$ReadMeInfo = @" 
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
# FonixData.cdx is a proprietary file required for FaceFX. You can obtain FonixData
# from the Fallout 3, Fallout 4, or Skyrim LE CreationKit files. It must be placed
# in '<SKYRIM_DIR>\Data\Sound\Voice\Processing'.
#
"@

$ReadMeInfo | Out-File -FilePath "Tools\Audio\README.txt" -Encoding ASCII

cd "x86\Release"
copy "FaceFXWrapper.exe" "..\..\Tools\Audio\FaceFXWrapper.exe"
cd ..
cd ..
[System.IO.Compression.ZipFile]::CreateFromDirectory("Tools", "FaceFXWrapper X.zip", $Compression, $true) # Include base dir
Remove-Item -Path "Tools" -Recurse