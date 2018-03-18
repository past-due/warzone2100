# To ensure reproducible builds, pin to a specific vcpkg commit
$VCPKG_COMMIT_SHA = "7528c4d525419a418e8e0046f6650b833ad75fd7";

Function Vcpkg_Install([string]$VCPKG_TRIPLET)
{
	If (-not ([string]::IsNullOrEmpty($VCPKG_TRIPLET)))
	{
		$env:VCPKG_DEFAULT_TRIPLET = "$VCPKG_TRIPLET";
	}
	.\vcpkg install physfs harfbuzz libiconv libogg libtheora libvorbis libpng openal-soft sdl2 glew freetype gettext zlib;
}

If ( -not (Test-Path (Join-Path (pwd) vcpkg\.git) -PathType Container) )
{
	# Clone the vcpkg repo
	git clone -q https://github.com/Microsoft/vcpkg.git;
}
Else
{
	# On CI (for example), the vcpkg directory may have been cached and restored
	Write-Output "Skipping git clone for vcpkg (local copy already exists)";
}
pushd vcpkg;
git reset --hard $VCPKG_COMMIT_SHA;
.\bootstrap-vcpkg.bat;

If ((Test-Path env:APPVEYOR))
{
	# On AppVeyor builds, get the WZ_VC_TARGET_PLATFORMNAME environment var (options: Win32, x64)
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
	# Default to default triplet (this is the vcpkg default, unless the user defines %VCPKG_DEFAULT_TRIPLET%)
	Vcpkg_Install "";
}

popd;
