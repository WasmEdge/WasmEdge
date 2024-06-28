# Release Policy

## Regular Release of WasmEdge

WasmEdge releases updates regularly at the end of each quarter. In addition, bug-fix hotfix releases are published as needed.

### Alpha Pre-Release for Feature Preview

- WasmEdge may publish an alpha pre-release to preview important features when necessary.
- An alpha pre-release is intended solely for feature preview and may not correspond to the upcoming official release version number. For example, even if `0.17.2-alpha.1` is pre-released, the upcoming official release may be `0.18.0` rather than `0.17.2`.

### Beta Pre-Release

- A beta pre-release is for fixing and stabilizing features when needed.
- The release version number is finalized at this stage.
- No features introducing breaking changes will be merged into the `master` branch after the beta pre-release.

### RC Pre-Release

- A release candidate (RC) pre-release is dedicated to bug fixing once all features are stable.
- The API documentation is updated throughout this period.

### Official Release

- After publishing the release, close the GitHub issues associated with that version.
- Ensure the WasmEdge documentation and installer are updated to reflect the latest official release version.

## Release Steps

WasmEdge maintainers can follow the steps below to release a new version.

### Changelog

1. Make sure every change is recorded in the changelog.

2. Make sure `Changelog.md` has the correct version number and release date.

3. Copy the changelog for this version into `.CurrentChangelog.md`. The CI takes this file as the release note.

4. Record the contributor list with the command:

   ```bash
   # Take 0.17.0 as the previous version for example.
   git log --pretty="%an" 0.17.0..master
   ```

5. Commit the change with the message `docs(changelog): update the changelog for the 0.17.0 release` and create the PR.

### Push Tag

1. Create a local tag. The version has the format `major.minor.patch`, with or without an `-alpha.version`, `-beta.version`, or `-rc.version` suffix.

   ```bash
   # Take 0.14.0-alpha.2 as the version for example.
   git tag -a 0.14.0-alpha.2 -m "0.14.0-alpha.2 pre-release"
   ```

2. Push the tag.

   ```bash
   # Take 0.14.0-alpha.2 as the version for example.
   git push origin 0.14.0-alpha.2
   ```

3. Wait for the CI to finish.

4. Publish the release on the GitHub release page. Remember to set the `Pre-release` checkbox correctly for release or pre-release versions.

## Backward Support

WasmEdge primarily releases updates for the latest `minor` version.
Within the same `minor` version, the API remains stable without breaking changes.
In the event of a security issue or critical bug, fixes are applied to the two most recent `minor` versions.
For example, if the current latest release is WasmEdge `0.17.0`, critical fixes are provided for the `0.17.x` and `0.16.x` versions.
