# Windows portable playtest

The portable ZIP contains `Play.exe`, `darkchronicle.exe`, `dc2data.exe` and their
runtime DLLs. The player extracts the whole ZIP into a writable directory and
opens `Play.exe`. No developer toolchain is required on the player's machine.

## First launch

The launcher offers a single compatible ISO found directly beside `Play.exe`,
or opens a Windows file picker through **Choose ISO**. The path field also accepts
a pasted ISO path, so setup can proceed without the dialog. It checks the ISO
filesystem, PAL boot identifier `SCES_511.90`, boot executable and required
game archives before extraction. This is a format/region check, not a retail
disc hash certification. The extractor performs the archive conversion and
staged publication.

**Prepare and play** runs the bundled extractor and then the bundled game.
Preparation is a single initial task; the progress indicator shows that work
is ongoing. Extraction errors leave setup available for correction and retry.
The player can open the setup log. Closing the setup window during extraction
does not interrupt publication.

The launcher uses its executable's directory rather than the caller's working
directory. `--run-dir <package>/userdata` puts the data trees, version marker and
save root under `userdata/`. It does not adopt or copy an existing AppData save
root. The game keeps its own existing save/configuration behavior within the
explicit portable root. A lock prevents another launcher from using that same
root while extraction or gameplay is running.
The game retains that lock if the launcher closes unexpectedly; it can finish
its session normally. Starting the game directly bypasses the launcher lock.

Current extracted data skips setup even when the ISO is no longer present.
The launcher checks the same versioned manifests and publisher marker as
the game's first-run path; this check does not hash every asset on every boot.
Use `dc2data check userdata/data` for a complete extraction audit.

## Updating and moving

Close the game, replace the package's program files, and retain `userdata/`.
The ZIP contains no `userdata` entries, so a normal extraction cannot replace
player progress. Back up that directory before experimental upgrades.
An incompatible asset version prompts for extraction again without deleting
saves or settings. A remembered ISO inside the package uses a relative path.

Move the entire package, including `userdata`, while the game is closed.
The new launcher location becomes the portable root on the next launch.
There is no automatic update or branch following; each package names its
source revision and pinned Kazarov revision in `build-info.json`.

## Building a candidate

On this fork, keep `master` synchronized with upstream. See [fork hygiene](FORK.md).
Build launcher changes
on their feature branch; a testing package does not require merging a pending
gameplay PR into the mirror.

```powershell
./tools/windows/configure.ps1 -Root .kazarov/windows `
    -BuildDirectory port/build/windows-portable -BuildType RelWithDebInfo -Tests
./tools/windows/build.ps1 -Root .kazarov/windows `
    -BuildDirectory port/build/windows-portable -Tests
ctest --test-dir port/build/windows-portable --output-on-failure -j4
./tools/windows/package.ps1 -BuildDirectory port/build/windows-portable `
    -Destination chronicletwo-windows-portable.zip
```

Acquire the pinned dependencies with `tools/windows/acquire.ps1` first if they
are absent. Packaging refuses a dirty source checkout and audits all bundled
PE imports. `PortableLauncher` exercises first run, ISO rejection, extraction
failure, relocation, updates, argument quoting and concurrent launches with
isolated synthetic disc and process fixtures. It does not qualify real gameplay.

The existing **PC builds** workflow can be dispatched on this feature branch.
Its manual Windows build uses RelWithDebInfo; normal CI keeps its Debug build.
Automatic Windows CI tests without uploading a package. A manual dispatch also
keeps the Windows package unpublished by default; only the explicit
`publish_windows_artifact` input packages and uploads it. CI artifacts on a
public repository are public downloads, so enable that input only when ready
for public binary distribution. The existing Linux and macOS jobs retain their
upstream artifact behavior. No workflow in this change creates a GitHub Release.
Packaging requires Release or RelWithDebInfo by default; `-AllowDebug` is an
explicit developer option for local diagnostics.

For unattended extraction verification, `Play.exe --no-ui --iso <path>
--prepare-only` prepares the portable directory without launching the game.
`--check-iso <path>` checks a disc without creating userdata. Game replay
arguments may follow `--`; the portable data/save arguments cannot be replaced.
`Play.exe --iso <path>` also supplies a disc without using the picker. The setup
field accepts quoted paths from Explorer's **Copy as path**. Setup and
game logs each retain one prior run as `setup.previous.log` and `game.previous.log`.

## Release gate: PortableBeta.Ready

Publish the first Windows beta Release when all of these hold:

- The candidate source SHA and upstream Kazarov pin are recorded, and the
  packaged binaries are built from that source.
- Windows CPU and local GPU gates pass, plus a real-ISO extraction audit,
  first launch, repeat launch, folder relocation, and save/load smoke test.
- At least two selected testers on separate compatible machines can choose
  their ISO, reach gameplay, save, close and continue without developer help.
- Wrong-region input, damaged input and extraction failures have actionable
  recovery, and updating the package preserves their progress.
- Binary-distribution permission for the project code and complete dependency
  licensing/notices have been settled. The upstream source snapshot has no
  project LICENSE file; public source visibility alone does not settle this.
- The notes identify known gameplay limits and the actual candidate features.
  A timing PR is included only in an explicitly identified timing candidate or
  after it has landed upstream; PAL content remains required in either case.

The appropriate first audience is a small technical playtest. Broader campaign
qualification belongs to a later game release. Keep each distributed build's
identity fixed, and retain a recovery copy even when replacing its download.
Before sharing even a private ZIP with another person, settle code redistribution
permission and ship the dependency licences/notices; privacy does not remove that
requirement. The current package is for local technical testing.
