# Contributing Guide

Please refer to the detailed [Contribution Guide] in the [WasmEdge/docs] repo.

[Contribution Guide]: https://github.com/WasmEdge/docs/blob/main/docs/contribute/contribute.md
[WasmEdge/docs]: https://github.com/WasmEdge/docs

## Commit Messages

In short, all commit messages should follow the standards:

0. [Conventional Commit](https://www.conventionalcommits.org/en/v1.0.0/)
0. [Developer Certificate of Origin (DCO)](https://probot.github.io/apps/dco/)

You can sign-off your commit with Git `-s` option, or by appending the sign-off text to your commit message.
The sign-off must match the Git user and email associated with the commit.

A valid commit should look like this:

```
<type>: <short description of the change>

<optional detailed description>

Signed-off-by: Your Name <your.email@example.com>
```

See [@commitlint/@config-conventional] for allowed `<type>` values.

[@commitlint/@config-conventional]: https://github.com/conventional-changelog/commitlint/tree/master/%40commitlint/config-conventional

### Example

```
docs: updates Contribution Guide

Signed-off-by: Alice Chen <alice.chen@example.com>
```

## CI Checks

The CI workflows will only be approved to execute if:

1. DCO check is passed.
2. Commit messages follow the Conventional Commit standards.
3. Pull Request title follows the Conventional Commit standards.
4. Local tests have been run and evidence (screenshot or logs) is provided in the PR description.
