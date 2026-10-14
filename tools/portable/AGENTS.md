# Portable Windows launcher

`Play.exe` is a Windows-native setup and launch front end. It uses the bundled
`dc2data` and game executables; it does not compile the game on a player's PC.
Keep the shared Kazarov pin and PS2 code untouched for launcher-only changes.

## Validation

Build with `tools/windows/configure.ps1 -BuildType RelWithDebInfo -Tests` and
`tools/windows/build.ps1 -Tests`. Run `ctest -R '^PortableLauncher$'` from the
configured build directory. These fixtures exercise launcher failures, storage,
argument quoting, relocation, concurrency, crash-log retention and child lifetime.

On an interactive Windows desktop, run `test_ui.py --launcher <Play.exe>
--fixture <ct_portable_fixture.exe> --output <new directory>` for native picker
selection, cancellation, Escape, owner-close protection and paste-path recovery.
It owns only its spawned process and fixture directory; do not automate other apps.

Before handing out a candidate, commit, rebuild, run the full applicable Windows
CPU/GPU gate, and audit a real PAL extraction and relocated launch. Do not commit
disc images, converted game assets, saves, local receipts or crash dumps.

## Packaging and release

`tools/windows/package.ps1` requires clean source, matching build identity, audited
runtime imports and an optimized build. The ZIP must contain only the explicitly
listed programs/runtimes, player instructions and build identity. No userdata.
`-AllowDebug` is for CI diagnostics. CI on a public fork is publicly accessible.

See `docs/PORTABLE.md` for `PortableBeta.Ready`. A locally validated package does
not qualify a campaign, save/load on real data, other machines or distribution
rights. No release publishing or automatic updating is part of this launcher.
