// To ensure reproducible builds, pin to a specific vcpkg commit
$VCPKG_COMMIT_SHA = "7528c4d525419a418e8e0046f6650b833ad75fd7";

Function Vcpkg_Install([string]$VCPKG_TRIPLET)
{
	$env:VCPKG_DEFAULT_TRIPLET = "$VCPKG_TRIPLET";
	.\vcpkg install physfs harfbuzz libiconv libogg libtheora libvorbis libpng openal-soft sdl2 glew freetype gettext zlib;
}

git clone https://github.com/Microsoft/vcpkg.git;
pushd vcpkg;
git reset --hard $VCPKG_COMMIT_SHA;
.\bootstrap-vcpkg.bat;

If ((Test-Path env:APPVEYOR))
{
	// On AppVeyor builds, get the WZ_VC_TARGET_PLATFORMNAME environment var (options: Win32, x64)
	If ($env:WZ_VC_TARGET_PLATFORMNAME -eq "x64")
	{
		Vcpkg_Install "x64-windows";
	}
	ElseIf ($env:WZ_VC_TARGET_PLATFORMNAME -eq "Win32")
	{
		Vcpkg_Install "x86-windows";
	}
	Else
	{
		Write-Error "Unsupported WZ_VC_TARGET_PLATFORMNAME";
	}
}
Else
{
	// Default to x86-windows triplet
	Vcpkg_Install "x86-windows";
}

popd;
