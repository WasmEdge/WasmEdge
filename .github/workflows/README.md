# CI Workflows

This document has not yet covered all workflows.

## Workflow for `build.yml`

```mermaid
flowchart LR
    %% _ is the starting point of everything
    _(( ))-->lint(lint)
    lint-->|pass|build(build)
    lint-->|fail|reject(unable to merge)
    build-.->source(create source tarball)
    build-.->oss("build on all OS")
    build-.->ext("build plugins on all OS")
```

### macOS

```json
[
  {
    "name": "MacOS 12 (x86_64)",
    "runner": "macos-12",
    "darwin_version": 21
  },
  {
    "name": "MacOS 14 (arm64)",
    "runner": "macos-14",
    "darwin_version": 23
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
    "runner": "linux-arm64-v2",
    "docker_tag": "manylinux2014_aarch64"
  },
  {
    "runner": "ubuntu-latest",
    "docker_tag": "manylinux_2_28_x86_64"
  },
  {
    "runner": "linux-arm64-v2",
    "docker_tag": "manylinux_2_28_aarch64"
  }
]
```

## Calling Structure for Reusable Workflows

```mermaid
flowchart LR
    subgraph "build-extensions.yml"
        b_("build-extensions.yml")-->|reusable-call-linter.yml|l0(("lint pass"))
        l0-->b_ext("reusable-build-extensions.yml")
        b_ext-->b_ext_m("reusable-build-extensions-on-manylinux.yml")
    end
    b("build.yml")-->|reusable-call-linter.yml|l1(("lint pass"))
    l1-->oss("<ul>
      <li>reusable-build-on-alpine-static.yml</li>
      <li>reusable-build-on-android.yml</li>
      <li>reusable-build-on-debian-static.yml</li>
      <li>reusable-build-on-macos.yml</li>
      <li>reusable-build-on-manylinux.yml</li>
      <li>reusable-build-on-ubuntu.yml</li>
      <li>reusable-build-on-windows.yml</li>
      <li>reusable-build-on-windows-msvc.yml</li>
    </ul>")
    subgraph "release.yml"
        rel("release.yml")-->|reusable-call-linter.yml|l2(("lint pass"))
        l2-->oss
        l2-->b_ext
        l2-->src
    end
    l1-->oss_extra("<ul>
      <li>reusable-build-on-debian.yml</li>
      <li>reusable-build-on-fedora.yml</li>
    </ul>")
    l1-->src("reusable-create-source-tarball.yml")
    classDef nostroke stroke:none;
    class l0,l1,l2 nostroke
    classDef relcls stroke:olive;
    class rel,oss,src,b_ext relcls
    classDef bcls stroke:orange;
    class b,oss_extra bcls
```
