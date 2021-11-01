## How to create a release

### Write Changelog
 - [ ] Make sure every change is written in the changelog.
 - [ ] Make sure the ``Changelog.md`` has the correct version number and the release date.
 - [ ] Copy the changelog of this version to ``.CurrentChangelog.md``. (Our release CI will take this file as the release notes)
 - [ ] Create a pull request, make sure the CI are all passed, and merge it.

### Create the Pre-Release
 - [ ] Use git tag to create a new release tag ``major.minor.patch-rc.version``. And push this tag to GitHub.
 - [ ] Wait for the CI builds and pushes the release binaries and release notes to the GitHub release page.
 - [ ] Check the ``Pre-release`` checkbox and publish the pre-release.

### Create the Official Release
 - [ ] Make sure the tests of features and the extensions are all passed, and issues are fixed.
 - [ ] Make sure the ``Changelog.md`` and ``.CurrentChangelog.md`` have the correct version number and the release date.
 - [ ] Use git tag to create a new release tag ``major.minor.patch``. And push this tag to GitHub.
 - [ ] Wait for the CI builds and pushes the release binaries and release notes to the GitHub release page.
 - [ ] Publish the release.