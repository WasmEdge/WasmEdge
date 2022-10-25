# CI Workflows

This document has not yet covered all workflows.

## build.yml
```mermaid
flowchart LR
    %% _ is the starting point of everything
    _(( ))-->lint(lint)
    lint-->|pass|build(build)
    lint-->|fail|reject(unable to merge)
    build-.->source(create source tarball)
    build-.->oss("<ul>
      <li>build on macOS</li>
      <li>build on manylinux</li>
      <li>build on Windows</li>
      <li>build on Android</li>
      <li>build on Fedora</li>
    </ul>")
```

### macOS
```json
[
  {
    "name": "MacOS 11 (x86_64)",
    "runner": "macos-11",
    "darwin_version": 20
  },
  {
    "name": "MacOS 12 (x86_64)",
    "runner": "macos-12",
    "darwin_version": 21
  }
]
```

### manylinux
```json
[
  {
    "runner": "ubuntu-latest",
    "docker_tag": "manylinux2014_x86_64"
  },
  {
    "runner": "linux-arm64",
    "docker_tag": "manylinux2014_aarch64"
  }
]
```
