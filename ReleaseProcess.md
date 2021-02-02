## How to create a release

### Write Changelog
 - [ ] Make sure every changes are written in the changelog.
 - [ ] Create a commit on ``release/major.minor.patch`` branch that updates the ``README.md``, ``CMakeLists.txt``, and ``Changelog.md`` with the correct version number and the release date.
 - [ ] Copy the changelog of this version to ``.CurrentChangelog.md``. (Our release CI will take this file as the release notes)
 - [ ] Create a pull request, make sure the CI are all passed, and merge it.

### Create the Release
 - [ ] Use git tag to create a new release tag ``major.minor.patch``. And push this tag to GitHub.
 - [ ] Wait for the CI builds and pushes the release binaries and release notes to the GitHub release page.
