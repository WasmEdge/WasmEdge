# Release Policy

## Regular Release of WasmEdge

WasmEdge will release updates regularly at the end of February, April, June, August, October, and December each year. Additionally, bug hotfix releases will be published as needed.

### Prepare for the New Release

- At the beginning of a new release cycle, create a GitHub project for the new version.
- Use this project to manage the process of merging pull requests (PRs) and closing issues.

### Alpha Pre-Release for Feature Preview

- WasmEdge may publish an alpha pre-release to preview important features if necessary.
- The alpha pre-release is solely for feature preview purposes and may not be associated with the upcoming official release version number.

### Beta Pre-Release

Approximately two weeks before the scheduled regular release date, a beta pre-release will be published.

- The release version number will be finalized.
- No features causing breaking changes will be merged into the master branch after the beta pre-release.

### RC Pre-Release

Approximately one week before the scheduled regular release date, a RC pre-release will be published.

- This pre-release is dedicated to bug fixing; no new features will be merged into the master branch after the RC pre-release.
- The API documentation will be updated throughout this period.

### Official Release

- After publishing the release, close the GitHub project associated with that version.
- Ensure that the WasmEdge documentation and installer are updated to reflect the latest official release version.

## Release Steps

WasmEdge maintainers can refer and follow the steps to release a new version.

### Changelog

1. Make sure every change is written in the changelog.

2. Make sure the `Changelog.md` has the correct version number and the release date.

3. Copy the changelog of this version to `.CurrentChangelog.md`. The CI will take this file as the release note.

4. Record the contributor lists by the command:

   ```bash
   # Take the 0.14.0 as the previous version for example.
   git log --pretty="%an" 0.14.0..master
   ```

5. Commit the change with message `[Changelog] Update the changelog for the 0.14.0 release.` and create the PR.

### Push Tag

1. Create a local tag. The version has the format as `major.minor.patch` with or without `-alpha.version`, `-beta.version`, or `-rc.version` suffix.

   ```bash
   # Take the 0.14.0-alpha.2 as the version for example.
   git tag -a 0.14.0-alpha.2 -m "0.14.0-alpha.2 pre-release"
   ```

2. Push the tag.

   ```bash
   # Take the 0.14.0-alpha.2 as the version for example.
   git push origin 0.14.0-alpha.2
   ```

3. Wait for the CI.

4. Publish the release in the Github release page. Remind to check the `Pre-release` checkbox status for the release or pre-release versions.

## Backward Supporting

WasmEdge primarily releases updates for the latest `minor` versions.
Within the same `minor` version, the API remains stable without breaking changes.
In the event of a security issue or critical bug, fixes will be applied to the two most recent `minor` versions.
For instance, if the current latest release is WasmEdge `0.14.0`, critical fixes will be provided for versions `0.14.x` and `0.13.x`.
