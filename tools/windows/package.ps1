# Packages CMake's explicit program/runtime list, never data, port_data, saves or ROMs.
param([Parameter(Mandatory)][string]$BuildDirectory,
      [Parameter(Mandatory)][string]$Destination, [switch]$AllowDebug)
$ErrorActionPreference = 'Stop'
. "$PSScriptRoot/common.ps1"
$build = Resolve-CheckoutPath $BuildDirectory
$archive = Resolve-CheckoutPath $Destination
& "$PSScriptRoot/check-runtime.ps1" -BuildDirectory $build
$manifest = Join-Path $build 'windows-runtime-files.txt'
$names = @(Get-Content -LiteralPath $manifest | Where-Object { $_.Trim() })
if ('Play.exe' -notin $names -or 'darkchronicle.exe' -notin $names -or 'dc2data.exe' -notin $names -or 'vulkan-1.dll' -notin $names) {
    throw 'The runtime manifest is incomplete. Reconfigure and build first.'
}
$paths = foreach ($name in $names) {
    if ($name -ne [IO.Path]::GetFileName($name) -or $name -notmatch '\.(exe|dll)$') {
        throw "Unexpected runtime manifest entry: $name"
    }
    $path = Join-Path $build $name
    if (!(Test-Path -LiteralPath $path -PathType Leaf)) { throw "Missing runtime file: $path" }
    $path
}
$source = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../..'))
$dirty = @(git -C $source status --porcelain --untracked-files=normal)
if ($LASTEXITCODE) { throw 'Cannot inspect the source checkout.' }
if ($dirty.Count) { throw 'Commit the candidate before packaging so build-info.json identifies a clean source revision.' }
$revision = git -C $source rev-parse HEAD
if ($LASTEXITCODE) { throw 'Cannot identify the source revision.' }
$pin = git -C $source rev-parse HEAD:tools/kazarov
if ($LASTEXITCODE) { throw 'Cannot identify the Kazarov revision.' }
$configuredSource = (Select-String -LiteralPath (Join-Path $build 'CMakeCache.txt') -Pattern '^CMAKE_HOME_DIRECTORY:[^=]+=(.+)$' |
    Select-Object -First 1).Matches[0].Groups[1].Value
if ([IO.Path]::GetFullPath($configuredSource) -ne $source) { throw 'The build directory belongs to a different source checkout.' }
$identityPath = Join-Path $build 'windows-build-identity.json'
if (!(Test-Path -LiteralPath $identityPath)) { throw 'Run tools/windows/build.ps1 from the clean candidate before packaging.' }
$identity = Get-Content -LiteralPath $identityPath -Raw | ConvertFrom-Json
if ($identity.build_type -notin @('Release', 'RelWithDebInfo') -and
    !($AllowDebug -and $identity.build_type -eq 'Debug')) {
    throw 'Portable candidates require Release or RelWithDebInfo. Use -AllowDebug only for CI diagnostics.'
}
if ($identity.source_dirty -or $identity.source_revision -ne $revision.Trim() -or $identity.kazarov_revision -ne $pin.Trim()) {
    throw 'The build does not identify this clean candidate. Run tools/windows/build.ps1 again before packaging.'
}
foreach ($path in $paths) {
    $name = [IO.Path]::GetFileName($path)
    if ((Get-FileHash -LiteralPath $path -Algorithm SHA256).Hash.ToLowerInvariant() -ne $identity.files_sha256.$name) {
        throw "The built file changed after its identity was recorded: $name"
    }
}
$stage = Join-Path ([IO.Path]::GetTempPath()) ('chronicletwo-package-' + [Guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Path $stage | Out-Null
try {
    foreach ($path in $paths) { Copy-Item -LiteralPath $path -Destination $stage }
    Copy-Item -LiteralPath (Join-Path $source 'tools/portable/README.txt') -Destination $stage
    Copy-Item -LiteralPath $identityPath -Destination (Join-Path $stage 'build-info.json')
    $parent = Split-Path $archive
    if (!(Test-Path -LiteralPath $parent)) { New-Item -ItemType Directory -Path $parent | Out-Null }
    Compress-Archive -Path (Join-Path $stage '*') -DestinationPath $archive -Force
} finally {
    $resolvedStage = [IO.Path]::GetFullPath($stage)
    $resolvedTemp = [IO.Path]::GetFullPath([IO.Path]::GetTempPath())
    if (!$resolvedStage.StartsWith($resolvedTemp, [StringComparison]::OrdinalIgnoreCase) -or
        [IO.Path]::GetFileName($resolvedStage) -notlike 'chronicletwo-package-*') { throw 'Unexpected package staging directory.' }
    Remove-Item -LiteralPath $resolvedStage -Recurse -Force
}
Write-Output "Packaged $($paths.Count) program/runtime files plus player instructions and build identity in $archive (no retail assets or saves)."
