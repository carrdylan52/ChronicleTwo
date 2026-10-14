#requires -Version 7.0
param([string]$Root, [string]$BuildDirectory, [switch]$Tests,
      [ValidateRange(1, 65536)][int]$Jobs = 12)
$ErrorActionPreference = 'Stop'
. "$PSScriptRoot/common.ps1"
$root = Resolve-WindowsRoot $Root
$build = Resolve-WindowsBuild $root $BuildDirectory
$cmake = Find-WindowsTool cmake 'C:/Program Files/CMake/bin/cmake.exe'
$rust = "$root/deps/rust/1.99.0-gnullvm/bin"
$llvm = "$root/deps/llvm/llvm-mingw-20260922-ucrt-x86_64/bin"
$env:CARGO_HOME = "$root/deps/rust/cargo-home"
$env:PATH = "$rust;$llvm;$env:PATH"
$targets = @('darkchronicle', 'dc2data', 'chronicle_play')
if ($Tests) { $targets += @('darkchronicle_tests', 'dc2data_bridge_test', 'ct_portable_fixture') }
$log = Join-Path $build 'build.log'
Write-Host "kazarov: building $build ($Jobs jobs; log: $log)"
$timer = [Diagnostics.Stopwatch]::StartNew()
# Keep the native invocation in PowerShell's foreground pipeline so Ctrl+C reaches
# CMake/Ninja. Tee-Object streams each line and preserves the complete diagnostics.
$PSNativeCommandUseErrorActionPreference = $false
try {
    # PowerShell 7.0/7.1 treat redirected native stderr as PowerShell errors.
    # Keep compiler diagnostics non-terminating while log-write failures still stop.
    $ErrorActionPreference = 'Continue'
    & $cmake --build $build --target $targets -j $Jobs 2>&1 | Tee-Object -FilePath $log -ErrorAction Stop
    $code = $LASTEXITCODE
} finally {
    $ErrorActionPreference = 'Stop'
}
Write-Host ("kazarov: build finished in {0:N1}s (exit {1})" -f $timer.Elapsed.TotalSeconds, $code)
if ($code) { exit $code }
# CMake's runtime dependency targets populate the output for both scripted and direct builds.
$manifest = Join-Path $build 'windows-runtime-files.txt'
if (Test-Path -LiteralPath $manifest) {
    $source = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../..'))
    $revision = git -C $source rev-parse HEAD
    if ($LASTEXITCODE) { throw 'Cannot identify the built source revision.' }
    $pin = git -C $source rev-parse HEAD:tools/kazarov
    if ($LASTEXITCODE) { throw 'Cannot identify the built Kazarov revision.' }
    $dirty = @(git -C $source status --porcelain --untracked-files=normal)
    if ($LASTEXITCODE) { throw 'Cannot inspect the built source checkout.' }
    $type = (Select-String -LiteralPath (Join-Path $build 'CMakeCache.txt') -Pattern '^CMAKE_BUILD_TYPE:[^=]+=(.+)$' |
        Select-Object -First 1).Matches[0].Groups[1].Value
    $hashes = @{}
    foreach ($name in (Get-Content -LiteralPath $manifest | Where-Object { $_.Trim() })) {
        $hashes[$name] = (Get-FileHash -LiteralPath (Join-Path $build $name) -Algorithm SHA256).Hash.ToLowerInvariant()
    }
    [ordered]@{ schema_version = 1; source_revision = $revision.Trim(); kazarov_revision = $pin.Trim();
        source_dirty = [bool]$dirty.Count; build_type = $type; platform = 'windows-x64'; files_sha256 = $hashes } |
        ConvertTo-Json -Depth 4 | Set-Content -LiteralPath (Join-Path $build 'windows-build-identity.json') -Encoding utf8NoBOM
}
exit 0
