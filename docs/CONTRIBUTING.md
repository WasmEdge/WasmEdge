# Contributing Guide

Please refer to the detailed [Contribution Guide] in the [WasmEdge/docs] repo.

[Contribution Guide]: https://github.com/WasmEdge/docs/blob/main/docs/contribute/contribute.md
[WasmEdge/docs]: https://github.com/WasmEdge/docs

## Coding Style

You must follow these coding style guidelines:

- C++17 standard
- Follow LLVM coding style (enforced via `.clang-format` and `.clang-tidy`)
  - 2-space indentation, no tabs; UTF-8 encoding; LF line endings
  - CamelCase for classes, functions, variables, and parameters
- All files must end with a newline and have no trailing whitespace. Run `lineguard` to check
- Do not add inline comments explaining the change

## Commit Messages

In short, all commit messages should follow these standards:

0. [Conventional Commits](https://www.conventionalcommits.org/en/v1.0.0/)
0. [Developer Certificate of Origin (DCO)](https://probot.github.io/apps/dco/)

You can sign off your commit with Git's `-s` option or by appending the sign-off text to your commit message.
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
docs: update Contribution Guide

Signed-off-by: Alice Chen <alice.chen@example.com>
```

## AI Assistance Disclosure

WasmEdge allows contributors to use AI tools for assistance. If you use AI assistance, you must disclose it in your commit messages and pull request descriptions.

However, the following practices are prohibited:

- Using AI tools to generate code without human review and understanding.
- Using automated AI agents to submit commits or pull requests without human oversight.

### How to disclose in commit messages

If you are using AI tools, use the `Assisted-by:` trailer to disclose AI assistance.

The `Assisted-by:` trailer should be placed before `Signed-off-by:` to indicate that AI assistance was used, with the human sign-off as the final confirmation.

```
<type>: <description>

Assisted-by: <name of AI tool or service>
Signed-off-by: Your Name <email@example.com>
```

Example: `Assisted-by: Claude (Anthropic)`. Use the tool's commercial name, optionally with the model version.

For pull requests, include a statement like:

> This contribution was developed with assistance from [AI tool name].

## CI Checks

The CI workflows will be approved to run only if:

1. The DCO check passes.
2. Commit messages follow the Conventional Commits standard.
3. The pull request title follows the Conventional Commits standard.
4. The local tests have been run, and evidence (a screenshot or logs) is provided in the PR description.
