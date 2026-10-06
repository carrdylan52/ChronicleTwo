# Chronicle Two

Chronicle Two is a decompilation project (and eventual port) of Dark
Chronicle/Dark Cloud 2 for the PlayStation 2.

# Building and running

1. Clone the repository with `git clone --recurse-submodules https://github.com/TheMoonPeople/ChronicleTwo.git`
2. Place the PAL retail build named `Dark Chronicle (PAL).iso` in the `rom/pal/` folder at the root of the project.
3. Run `build.sh`.

`build.sh` builds the game.
`run.sh` builds the disc image and boots it in PCSX2.

# Progress reporting

Each push to `master` generates an objdiff report for the PAL executable. The
`Progress report` GitHub Actions workflow uploads it as the `pal_report`
artifact, which [decomp.dev](https://decomp.dev/) reads after the repository is
registered at [decomp.dev/manage/new](https://decomp.dev/manage/new). The
workflow also posts a summary to Discord's `#progress` channel. A separate
workflow posts commits from `master` to Discord, independently of the build.

Configure these GitHub Actions settings for the repository:

| Setting | Type | Purpose |
| --- | --- | --- |
| `PRIVATE_REPOSITORY` | Variable | `owner/name` of the private asset repository. |
| `PRIVATE_REPO_TOKEN` | Secret | Fine-grained GitHub token with read-only Contents access to the private repository. |
| `DISCORD_WEBHOOK_URL` | Secret | Webhook for the `#progress` channel. |
| `DISCORD_COMMITS_WEBHOOK_URL` | Secret | Webhook for the commit channel. |

The private repository must contain
`rom/pal/extracted/iso/SCES_511.90` from the PAL disc. Git LFS may store this
file; CI fetches only this path and verifies it against
`rom/pal/extracted.sha256`. CI does not need the ISO or the other extracted
files. To use a local private checkout for a progress build, place it at
`.private/` and run `scripts/host/overlay_private.sh .private`, then `build.sh`.
Do not commit retail files to this repository.

Progress reports require private access, so pull request runs do not build a
report. Pushes to `master` and manual workflow runs on `master` do. If a
Discord webhook is absent, its notification job skips delivery; the progress
artifact is still uploaded when private access is configured.
