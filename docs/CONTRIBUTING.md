# Contributing Guide

Thank you for your interest in contributing to WasmEdge. This guide describes how to report issues, how to submit pull requests, and the commit and CI standards the project enforces.

For the broader onboarding documentation, please also refer to the detailed [Contribution Guide] in the [WasmEdge/docs] repo.

[Contribution Guide]: https://wasmedge.org/docs/contribute/overview
[WasmEdge/docs]: https://github.com/WasmEdge/docs

## Contribution Workflow

At a high level, a contribution flows through these steps:

1. **Open an issue** — report a bug or propose a feature, and reach agreement with the maintainers. See [Issues](#issues).
2. **Prepare your change** — keep each pull request focused on one logical change and follow the [Commit Messages](#commit-messages) standards.
3. **Open a pull request** — link the issue and meet the [pull request requirements](#requirements). See [Pull Requests](#pull-requests).
4. **Pass review and CI** — obtain an approval from the area owner and pass all [CI checks](#ci-checks).
5. **Merge** — the area owner merges the approved change.

## Issues

The GitHub issue tracker is for reporting bugs and requesting features in WasmEdge. It is not a support forum.

### Before Opening an Issue

- Search the [existing issues](https://github.com/WasmEdge/WasmEdge/issues) first to avoid duplicates. If an issue already exists, add your information there instead of opening a new one.
- For simple questions, ask the community on [Discord](https://discord.gg/U4B5sFTkFc).
- For advanced or unanswered questions that need a maintainer's response, use [GitHub Discussions](https://github.com/WasmEdge/WasmEdge/discussions).
- Blank issues are disabled. Please pick the template that matches your case.

### Choosing a Template

| Template | Use for | Title prefix | Auto label |
| --- | --- | --- | --- |
| Bug Report | Something is not working as expected. | `bug:` | `bug` |
| Feature Request | A new feature or idea for WasmEdge. | `feat:` | `enhancement` |
| General Issue | Anything that does not fit the other templates. | `question:` | `question` |

The community, mentorship, and event templates (community meeting, GSoC, LFX mentorship, mentoring workspace) are also available for those specific purposes.

### Labels

- **Type labels** (`bug`, `enhancement`, `question`) are applied automatically based on the template you choose.
- **Component labels** (for example, `c-Executor`, `c-Loader`, `c-Validator`, `WASI-NN`, `binding-rust`, `documentation`) mark the affected area of the project. They mirror the source-tree layout and are applied during triage.

### Triage and Lifecycle

A maintainer triages incoming issues by:

1. Confirming the report and requesting more information if needed.
2. Adding the relevant component labels and, when appropriate, assigning an owner.
3. Linking the issue to the pull request that addresses it.

An issue is closed when the corresponding fix is merged, when it is invalid or cannot be reproduced, or when it is a duplicate of another issue. A closed issue can be reopened if new information becomes available.

### Stale Issues

An issue with no activity for 30 days is marked `stale`. If it remains inactive for another 14 days, it is closed. A stale or closed issue can be reopened at any time when there is new information to add.

## Pull Requests

### Open an Issue First

Every non-trivial pull request must reference an issue so that the change can be tracked and discussed before review:

- **For a newly discovered bug**, open a [Bug Report](https://github.com/WasmEdge/WasmEdge/issues/new?template=bug_report.yml) first, including the reproduction steps and, where possible, a failing test case. The pull request should then add that test case and the fix, and link back to the issue.
- **For a new feature**, open a [Feature Request](https://github.com/WasmEdge/WasmEdge/issues/new?template=feature_request.yml) first and reach an agreement with the maintainers through discussion. A pull request that implements a feature which has not been discussed, or which is not part of the project's future plan (see the [Roadmap](./ROADMAP.md)), may be closed.

Keep each pull request focused on a single logical change. Unrelated changes should be submitted as separate pull requests.

### Requirements

Before submitting, make sure your pull request meets the checklist in the [pull request template](../.github/PULL_REQUEST_TEMPLATE.md):

- **DCO sign-off**: every commit is signed off (`git commit -s`).
- **Conventional Commits**: both the commit messages and the pull request title follow the [Conventional Commit](https://www.conventionalcommits.org/en/v1.0.0/) standards. See [Commit Messages](#commit-messages) for details.
- **Code style**: the C++ code is formatted with `clang-format` (per the project's `.clang-format`) and passes the other linters, including `lineguard` (trailing whitespace and final newline) and `codespell` (typos).
- **Test evidence**: local tests have been run, with a screenshot or logs provided in the description.

See [CI Checks](#ci-checks) for the conditions under which CI workflows are approved. A pull request that does not meet these requirements may be closed automatically.

### Review and Approval

- Each pull request requires at least one approval from a Committer or Maintainer who owns the affected area, as defined in the [CODEOWNERS](../.github/CODEOWNERS) file and the [Contributor Ladder](./CONTRIBUTOR_LADDER.md).
- Address review feedback by pushing follow-up commits to the same branch. Reviewers re-review after the requested changes are made.
- A pull request that introduces a breaking change follows the timing rules in the [Release Policy](./RELEASE_POLICY.md): such changes are not merged after a beta pre-release.

### Merge

Once a pull request is approved and all CI checks pass, the owner of the affected area merges it. WasmEdge keeps a linear history on the `master` branch, so use **squash and merge** or **rebase and merge**; merge commits are not used. The merged commit (or commits, when rebasing) must keep its Conventional Commit message and DCO sign-off.

### Stale Pull Requests

A pull request with no activity for 30 days is marked `stale`. If it remains inactive for another 14 days, it may be closed. A closed pull request can be reopened when work resumes.

## Commit Messages

Every commit message must follow two standards:

- **[Conventional Commit]** — defines the message format.
- **[Developer Certificate of Origin (DCO)]** — certifies that you wrote, or have the right to submit, the change, declared with a sign-off line.

[Conventional Commit]: https://www.conventionalcommits.org/en/v1.0.0/
[Developer Certificate of Origin (DCO)]: https://probot.github.io/apps/dco/

A complete commit message looks like this:

```text
<type>: <short description of the change>

<optional detailed description>

Signed-off-by: Your Name <your.email@example.com>
```

### Format

Write the header in the Conventional Commit form `<type>: <short description>`, optionally followed by a detailed body. The allowed `<type>` values are `feat`, `fix`, `docs`, `style`, `refactor`, `perf`, `test`, `chore`, and `ci`; see [@commitlint/@config-conventional] for the full specification.

[@commitlint/@config-conventional]: https://github.com/conventional-changelog/commitlint/tree/master/%40commitlint/config-conventional

### Sign-off

Sign off your commit with Git's `-s` option, or by appending the sign-off text manually. The sign-off must match the Git user name and email associated with the commit. For example:

```text
docs: updates Contribution Guide

Signed-off-by: Alice Chen <alice.chen@example.com>
```

### AI Assistance Disclosure

If a commit was developed with the help of an AI tool, disclose it with an `Assisted-by:` trailer.
Place the `Assisted-by:` trailer before the `Signed-off-by:` line, so that the human sign-off remains the final confirmation of the change:

```text
<type>: <short description of the change>

<optional detailed description>

Assisted-by: <name of AI tool or service>
Signed-off-by: Your Name <your.email@example.com>
```

Use the tool's commercial name, optionally with its model version, for example `Assisted-by: Claude (Anthropic)`.

For a pull request developed with AI assistance, also include a statement in the description, such as:

> This contribution was developed with assistance from [AI tool name].

## CI Checks

The CI workflows are only approved to run when all of the following pass:

1. The DCO check passes.
2. The commit messages follow the Conventional Commit standards.
3. The pull request title follows the Conventional Commit standards.
4. The lint checks pass, including `clang-format` (C++ code style), `lineguard` (trailing whitespace and final newline), and `codespell` (typos).
5. Local tests have been run, with evidence (a screenshot or logs) provided in the pull request description.
