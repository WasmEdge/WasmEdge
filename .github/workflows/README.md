# CI Workflows

This document has not yet covered all workflows.

## build.yml
```mermaid
flowchart LR
    %% _ is the starting point of everything
    _(( ))-->lint
    lint-->|pass|build
    lint-->|fail|reject(unable to merge)
    build-->source(create source tarball fa:fa-link)
    click source "reusable-create-source-tarball.yml"
    build-->macos(build on macOS fa:fa-link)
    click macos "reusable-build-on-macos.yml"
```
