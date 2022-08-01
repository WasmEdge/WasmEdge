# CI Workflows

This document has not yet covered all workflows.

```mermaid
flowchart LR
    %% _ is the starting point of everything
    _(( ))-->lint(linter.yml)

    subgraph PR
        direction LR
        lint-->|pass|build(build.yml)
        build-.->|call|_build((list of files fa:fa-link))
        click _build "#Reusable-YMLs"
    end

    lint-->|fail|reject(unable to merge)
```

## Reusable YMLs
- [reusable-create-source-tarball.yml](reusable-create-source-tarball.yml)
- [reusable-build-on-macos.yml](reusable-build-on-macos.yml)
