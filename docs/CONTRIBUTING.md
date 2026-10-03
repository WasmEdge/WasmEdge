# Contributing Guide

Thank you for your interest in contributing to WasmEdge. This guide describes the coding style, the commit message standards, and the issue and pull request rules that the project enforces.

Please also refer to the detailed [Contribution Guide] in the [WasmEdge/docs] repo.

[Contribution Guide]: https://github.com/WasmEdge/docs/blob/main/docs/contribute/contribute.md
[WasmEdge/docs]: https://github.com/WasmEdge/docs

At a high level, a contribution flows through these steps:

1. **Open an issue** — report a bug or propose a feature with the matching [issue template](#issues), and reach agreement with the maintainers.
2. **Claim the issue** — comment on it and wait for a maintainer to assign it to you. See [Issue assignment](#issue-assignment).
3. **Prepare your change** — follow the [Coding Style](#coding-style) and the [Commit Messages](#commit-messages) standards.
4. **Open a pull request** — link the issue and meet the [pull request rules](#pull-requests).
5. **Pass review and CI** — obtain an approval from the owner of the affected area and pass all [CI checks](#ci-checks).

## Coding Style

You must follow these coding style guidelines:

- C++17 standard
- Follow LLVM coding style (enforced via `.clang-format` and `.clang-tidy`)
  - 2-space indentation, no tabs; UTF-8 encoding; LF line endings
  - CamelCase for classes, functions, variables, and parameters
- All files must end with a newline and have no trailing whitespace
- Do not add inline comments explaining the change

Before submitting, check your change locally with `clang-format` (formatting), `lineguard` (trailing whitespace and final newline), and `codespell` (typos). All three also run in CI, and `clang-format` gates the build workflows: a formatting error stops the builds before they start.

## Commit Messages

Every commit message must follow two standards:

- **[Conventional Commits]** — defines the message format. Both the commit messages and the pull request title are validated by the `commitlint` workflow.
- **[Developer Certificate of Origin (DCO)]** — certifies that you wrote, or have the right to submit, the change, declared with a sign-off line.

[Conventional Commits]: https://www.conventionalcommits.org/en/v1.0.0/
[Developer Certificate of Origin (DCO)]: https://probot.github.io/apps/dco/

A complete commit message looks like this:

```text
<type>: <short description of the change>

<optional detailed description>

Signed-off-by: Your Name <your.email@example.com>
```

### Format

Write the header in the Conventional Commit form `<type>: <short description>`, optionally followed by a detailed body. The allowed `<type>` values are `feat`, `fix`, `docs`, `style`, `refactor`, `perf`, `test`, `chore`, and `ci`; see [@commitlint/@config-conventional] for the full specification. You can validate your messages locally by running `commitlint` before pushing.

[@commitlint/@config-conventional]: https://github.com/conventional-changelog/commitlint/tree/master/%40commitlint/config-conventional

### Sign-off

Sign off your commit with Git's `-s` option, or by appending the sign-off text manually. The sign-off must match the Git user name and email associated with the commit. For example:

```text
docs: update Contribution Guide

Signed-off-by: Alice Chen <alice.chen@example.com>
```

### AI Assistance Disclosure

WasmEdge allows contributors to use AI tools for assistance, provided that the assistance is disclosed. The following practices are prohibited:

- Using AI tools to generate code without human review and understanding.
- Using automated AI agents to submit commits, issues, or pull requests without human oversight.

If a commit was developed with the help of an AI tool, disclose it with an `Assisted-by:` trailer. Place the trailer before the `Signed-off-by:` line, so that the human sign-off remains the final confirmation of the change:

```text
<type>: <short description of the change>

<optional detailed description>

Assisted-by: <name of AI tool or service>
Signed-off-by: Your Name <your.email@example.com>
```

Use the tool's commercial name, optionally with its model version, for example `Assisted-by: Claude (Anthropic)`.

For a pull request developed with AI assistance, also state it in the description, such as:

> This contribution was developed with assistance from [AI tool name].

## Issues

The GitHub issue tracker is for reporting bugs and requesting features in WasmEdge. It is not a support forum. Blank issues are disabled, so please pick the template that matches your case.

- For simple questions, ask the community on [Discord](https://discord.gg/U4B5sFTkFc).
- For advanced or unanswered questions that need a maintainer's answer, use [GitHub Discussions](https://github.com/WasmEdge/WasmEdge/discussions).
- Search the [existing issues](https://github.com/WasmEdge/WasmEdge/issues) first. If an issue already exists, add your information there instead of opening a new one.

### Main issue templates

| Template | Use for | Title prefix | Auto label |
| --- | --- | --- | --- |
| [Bug Report](https://github.com/WasmEdge/WasmEdge/issues/new?template=bug_report.yml) | Something is not working as expected. | `bug:` | `bug` |
| [Feature Request](https://github.com/WasmEdge/WasmEdge/issues/new?template=feature_request.yml) | A new feature or idea for WasmEdge. | `feat:` | `enhancement` |
| [Performance Issue](https://github.com/WasmEdge/WasmEdge/issues/new?template=performance_issue.yml) | Something works, but is slower or uses more resources than expected. | `perf:` | `performance` |
| Good First Issue | An onboarding good first issue for the newcomers. Maintainers only. | `<type>:` | `good first issue` |
| [Question](https://github.com/WasmEdge/WasmEdge/issues/new?template=question.yml) | Q&A that does not fit the other templates. | `question:` | `question` |

Beyond the automatic type label above, maintainers add **component labels** (for example, `c-Executor`, `c-Loader`, `c-Validator`, `WASI-NN`, `binding-rust`, or `documentation`) during triage to mark the affected area of the project.

### Community and mentorship issue templates

These templates serve specific community programs rather than the development of WasmEdge itself:

- **Community Meeting** — propose and discuss topics for the monthly community meeting.
- **Mentorship Project Idea** — submit a project idea for GSoC or LFX Mentorship. Maintainers only.
- **Mentorship Workspace** — mentees submit a project workspace for the mentoring programs, including OSPP, GSoC, and LFX Mentorship.

### Issue assignment

To keep the work visible and avoid duplicated effort, WasmEdge assigns issues under these rules:

1. **One issue at a time** — a contributor can be the assignee of only one issue at any given time.
2. **Claim it first** — comment on the issue to state that you want to work on it.
3. **Wait for the assignment** — you become the assignee once a maintainer assigns the issue to you.
4. **Inactivity releases the issue** — if an assigned issue shows no progress for **7 days** and another contributor claims it, the maintainers may transfer the assignment to that new contributor. A comment describing your progress, or a draft pull request, counts as progress.
5. **The assignee has review priority** — while an issue has an assignee, their pull request is the one that gets reviewed. A pull request for the same issue from anyone else is marked as duplicated and closed.
6. **Good first issues are for newcomers** — only first-time contributors may claim an issue labeled `good first issue`.

## Pull Requests

### Pull request types

Every type must have its corresponding issue opened first, and the pull request links back to it:

| Type | Prerequisite issue | Type-specific requirements |
| --- | --- | --- |
| `fix:` Bug fix | Bug Report | Add a regression test that fails without the fix and passes with it; describe the root cause in the description. |
| `feat:` Feature | Feature Request, agreed with the maintainers first | Add tests covering the new behavior, and update the relevant documentation. |
| `perf:` Performance | Performance Issue | Provide reproducible before/after benchmarks for the same workload and environment; confirm existing tests still pass, so that behavior is unchanged. |

A feature that has not been discussed, or that is not part of the project's future plan (see the [Roadmap](./ROADMAP.md)), may be closed. Keep each pull request focused on a single logical change, and submit unrelated changes separately.

### Checklist

Before submitting, make sure your pull request meets the checklist in the [pull request template](../.github/PULL_REQUEST_TEMPLATE.md):

1. **Linked issue** — the description links back to the prerequisite issue.
2. **DCO sign-off** — every commit is signed off (`git commit -s`).
3. **Conventional Commits** — both the commit messages and the pull request title follow the standard.
4. **Code style** — the change follows the [Coding Style](#coding-style) guidelines and passes its lint checks.
5. **Test evidence** — local tests have been run, with a screenshot or logs in the description.

If you used AI-assisted tools, two more items apply:

- **AI disclosure** — the `Assisted-by:` trailer is in the commits, and the assistance is noted in the description. See [AI Assistance Disclosure](#ai-assistance-disclosure).
- **Human-in-the-loop** — every commit, issue, and pull request has been verified by you before submission.

A pull request that does not meet these requirements may be closed automatically.

### One ready-to-review pull request at a time

Each contributor may have only **one** pull request in the ready-for-review state at a time. Open any further pull request as a **draft**, and mark it ready for review only after the previous one is merged or closed. This keeps the review queue short and the feedback loop fast.

### Review and merge

- Each pull request requires at least one approval from a Committer or Maintainer who owns the affected area, as defined in the [CODEOWNERS](../.github/CODEOWNERS) file and the [Contributor Ladder](./CONTRIBUTOR_LADDER.md).
- Address review feedback by pushing follow-up commits to the same branch. Reviewers re-review after the requested changes are made.
- A pull request that introduces a breaking change follows the timing rules in the [Release Policy](./RELEASE_POLICY.md): such changes are not merged after a beta pre-release.
- Once approved and all [CI checks](#ci-checks) pass, the owner of the affected area merges the pull request. WasmEdge keeps a linear history on the `master` branch, so use **squash and merge** or **rebase and merge**; merge commits are not used. The merged commit, or commits when rebasing, must keep its Conventional Commit message and DCO sign-off.

## CI Checks

The CI workflows do not start on their own. A maintainer approves the run once your pull request meets the [checklist](#checklist), so a pull request that is missing a sign-off, a Conventional Commit message, or its test evidence will not be given CI time.

Which workflows then run depends on the paths your change touches; a few run on every pull request, and a check that is absent was simply not triggered by your change. For the full trigger rules, what each workflow covers, and how to interpret a failure, see the [CI Workflows reference](../.github/workflows/README.md).

> **Tip:** To run the CI jobs before asking for a review, open a pull request against your own fork to trigger the workflows there.
