# Fork and release hygiene

Keep `master` as an upstream mirror. Work on one named feature branch per change.
Keep an open PR's branch until it is merged or deliberately closed; a separate
launcher branch should not silently include pending timing changes. For a build
that combines unmerged features, use a named integration branch and document
every included SHA. Test that combined source again.

Before syncing, inspect the actual remotes, fetch upstream, and compare both
sides. Fast-forward a clean mirror. If the fork's default branch contains unique
work, preserve it on another branch before deciding how to reconcile it; do not
force-sync over it. The default branch name here is `master`.

Branch cleanup is separate from making a release. Inventory old branches by
their unique commits, associated PRs and dependent worktrees. Keep active PRs,
unmerged work and bases of other branches. Back up candidates and propose a
small, explicit deletion list for review. A squash-merged branch may still show
unique commits, so ancestry alone cannot prove its changes are redundant.
Deleting branches is not the same as `git gc`, and aggressive garbage collection
is unnecessary for preparing a portable package.

Use an exact source commit and pinned Kazarov revision for each package. Build
from a clean checkout, keep its build identity and ZIP digest, and retain the
validation receipts outside the public source tree. Do not package the build
directory wholesale: the runtime manifest is the whitelist. Retail ISO files,
extracted assets, player saves, local paths and crash dumps stay out of Git and
release assets.

Start with a locally built package for selected technical testers. Actions
artifacts and published prereleases on a public fork are public distribution.
A GitHub draft is preparation for collaborators, not a private player download.
Publishing a release can be undone on GitHub, but cannot recall copies already
downloaded. Immutable releases additionally lock published assets and prevent
reuse of their tag after deletion. Use a new version for the next candidate,
mark an older build as superseded, and explain save/data compatibility rather
than silently replacing its files.

The launcher/build candidate has its own validation; gameplay readiness is a
separate gate. See [PortableBeta.Ready](PORTABLE.md#release-gate-portablebetaready)
for when a public Windows beta is appropriate. Branch tidiness does not replace
ISO setup, real save/load and independent-machine testing.

GitHub references: [branch deletion and restoration](https://docs.github.com/en/repositories/configuring-branches-and-merges-in-your-repository/managing-branches-in-your-repository/deleting-and-restoring-branches-in-a-pull-request),
[managing releases](https://docs.github.com/en/repositories/releasing-projects-on-github/managing-releases-in-a-repository),
[immutable releases](https://docs.github.com/en/code-security/concepts/supply-chain-security/immutable-releases).
