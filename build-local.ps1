param([ValidateSet('Release', 'Debug')][string]$Configuration = 'Release')
$ErrorActionPreference = 'Stop'
$root = $PSScriptRoot
$atl = Join-Path $root '.build-deps\atl-v143'
if (-not (Test-Path -LiteralPath "$atl\include\atlbase.h")) {
    throw "Local ATL dependency missing: $atl"
}
$vswhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
$vs = & $vswhere -latest -products '*' -requires Microsoft.Component.MSBuild -property installationPath
if (-not $vs) { throw 'MSBuild installation not found.' }
$msbuild = Join-Path $vs 'MSBuild\Current\Bin\MSBuild.exe'
$savedCL = $env:CL
$savedLINK = $env:LINK
$savedTemp = $env:TEMP
$savedTmp = $env:TMP
try {
    $env:CL = "/I`"$atl\include`" $savedCL"
    $env:LINK = "/LIBPATH:`"$atl\lib\x64`" $savedLINK"
    $tempDir = Join-Path $root '.build-deps\temp'
    New-Item -ItemType Directory -Path $tempDir -Force | Out-Null
    $env:TEMP = $tempDir
    $env:TMP = $tempDir
    # Separate executable output preserves the existing working release.
    $output = Join-Path $root "x64\Local$Configuration\"
    $ling = [IO.Path]::GetFullPath((Join-Path $root '..\Ling')) + '\'
    $libs = Join-Path $root ".build-deps\libs\$Configuration\"
    foreach ($project in @('yoga\yoga.vcxproj', 'Ling.vcxproj')) {
        & $msbuild (Join-Path $ling $project) "/p:Configuration=$Configuration" /p:Platform=x64 /p:PlatformToolset=v143 "/p:SolutionDir=$ling" "/p:OutDir=$libs" /m /v:minimal /nologo
        if ($LASTEXITCODE -ne 0) { throw "Dependency build failed: $project" }
    }
    $props = Join-Path $root '.build-deps\local-link.props'
    @"
<Project xmlns="http://schemas.microsoft.com/developer/msbuild/2003">
  <ItemDefinitionGroup><Link><AdditionalLibraryDirectories>$libs;%(AdditionalLibraryDirectories)</AdditionalLibraryDirectories></Link></ItemDefinitionGroup>
</Project>
"@ | Set-Content -LiteralPath $props -Encoding utf8
    & $msbuild (Join-Path $root 'ScreenCapture.slnx') "/p:Configuration=$Configuration" /p:Platform=x64 /p:PlatformToolset=v143 "/p:OutDir=$output" "/p:ForceImportBeforeCppTargets=$props" /m /v:minimal /nologo
    if ($LASTEXITCODE -ne 0) { throw "Build failed with exit code $LASTEXITCODE" }
}
finally {
    $env:CL = $savedCL
    $env:LINK = $savedLINK
    $env:TEMP = $savedTemp
    $env:TMP = $savedTmp
}
