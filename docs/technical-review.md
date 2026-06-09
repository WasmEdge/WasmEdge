# General Technical Review - [WasmEdge] / [Incubation]

- **Project:** WasmEdge
- **Project Version:** 0.16.1
- **Website:** https://wasmedge.org/
- **Date Updated:** 2026-03-25
- **Template Version:** v1.0
- **Description:** WasmEdge is a lightweight, high-performance, and extensible WebAssembly runtime.

## Day 0 - Planning Phase

### Scope

  * Describe the roadmap process, how scope is determined for mid to long term features, as well as how the roadmap maps back to current contributions and maintainer ladder?
    * The WasmEdge roadmap and the process can be found at the [document](https://github.com/WasmEdge/WasmEdge/blob/master/docs/ROADMAP.md).
    * WasmEdge maintainers will create a roadmap discussion issue every quarter, and welcome everyone to add roadmap entries with the planned timeline. We'll assign the roadmap entries to the owners, or refer to the [code owner list](https://github.com/WasmEdge/WasmEdge/blob/master/.github/CODEOWNERS) to determine the roadmap owner.
    * For contributions, the WasmEdge project maintains a governance structure with roles including community participants, contributors, reviewers, committers, and maintainers, as described in the [governance document](https://github.com/WasmEdge/WasmEdge/blob/master/docs/GOVERNANCE.md) and the [contributor ladder](https://github.com/WasmEdge/WasmEdge/blob/master/docs/CONTRIBUTOR_LADDER.md). Contributors who demonstrate sustained, high-quality contributions can be nominated for committer or maintainer roles.
  * Describe the target persona or user(s) for the project?
    * Cloud native developers: Use WasmEdge as a lightweight and cross-platform cloud native runtime
    * AI developers: Use WasmEdge as a lightweight and cross-platform LLM runtime
    * Blockchain developers: Use WasmEdge as the smart contract engine
    * Application developers: Use WasmEdge to develop microservices
  * Explain the primary use case for the project. What additional use cases are supported by the project?
    * The primary use case is to use WasmEdge as a lightweight, high-performance, and extensible WebAssembly runtime. Use cases include:
      * LLM inference
      * Embedded runtime
      * Serverless runtime
      * Application runtime
  * Explain which use cases have been identified as unsupported by the project.
    * WasmEdge is a runtime to execute WebAssembly programs with extensions. Therefore, we expect the inputs are WASM files, not the host languages before compilation. Tasks such as compiling programs into WASM, injecting code into WASM, or combining multiple WASM files are identified as toolchain tasks.
  * Describe the intended types of organizations who would benefit from adopting this project. (i.e. financial services, any software manufacturer, organizations providing platform engineering services)?
    * Organizations that benefit from WasmEdge include cloud infrastructure providers (e.g., Docker, Kubernetes distributions), edge computing platforms (e.g., eKuiper, KubeEdge), AI/LLM application providers (e.g., Gaia), blockchain platforms, IoT device manufacturers, and any software vendor seeking lightweight, portable, and sandboxed execution of user-contributed or third-party code.
  * Please describe any completed end user research and link to any reports.
    * Current case studies are documented in [the user list](https://wasmedge.org/docs/contribute/users), including WikiFunctions, LF-Edge/eKuiper, crun, KubeEdge, Docker, Gaia, and 5miles. Formal user research reports are planned as the project matures toward v1.0.

### Usability

  * How should the target personas interact with your project?
    * The target personas interact with WasmEdge via the WasmEdge CLI or the WasmEdge C API.
  * Describe the user experience (UX) and user interface (UI) of the project.
    * WasmEdge provides the [CLI tools](https://wasmedge.org/docs/start/build-and-run/cli) for the user interface. As a WASM runtime, users can interact with WasmEdge via the commands including the input files and options easily.
    * For developers who want to bind WasmEdge into their host applications, WasmEdge provides the C API header and shared libraries with documents. Developers can refer to the API docs to invoke the WasmEdge APIs for good user experience.
  * Describe how this project integrates with other projects in a production environment.
    * Docker: [Docker integrated WasmEdge](https://wasmedge.org/docs/start/getting-started/quick_start_docker) as a WASM runtime with the `containerd` image store. Users can install a docker image via the Docker Desktop to run their WASM files.
    * crun: After installation of `crun` and WasmEdge, users can use [crun to deploy WASM images](https://wasmedge.org/docs/develop/deploy/oci-runtime/crun/). `crun` integrates with WasmEdge via the C APIs, providing an easier way to deploy WASM images on Fedora platforms.
    * podman: Furthermore, after installing `crun-wasm` and `podman`, users can [use the podman tool](https://developers.redhat.com/articles/2023/12/06/unlock-webassembly-workloads-podman-macos-and-windows#using_podman_engine_with_wasm_) to create and manage the containerized WASM workloads.
    * Gaia: Use WasmEdge as the cross-platform LLM runtime
    * 5miles: Use Kubernetes and WasmEdge to host whisper models

### Design

  * Explain the design principles and best practices the project is following.
    * WasmEdge mainly follows the [LLVM coding standards](https://llvm.org/docs/CodingStandards.html). The coding style is enforced by `clang-format`, and the naming style and comment formatting follow the LLVM style. For the API designs, although they differ, WasmEdge mainly follows the [WASM C API](https://github.com/WebAssembly/wasm-c-api) usages.
  * Outline or link to the project’s architecture requirements? Describe how they differ for Proof of Concept, Development, Test and Production environments, as applicable.
    * WasmEdge uses [GitHub CI](https://github.com/WasmEdge/WasmEdge/blob/master/.github/workflows/build.yml) to build, test, and release the project. We currently support Windows, MacOS, and Linux platforms. Developers can refer to the CI `.yml` files to set up their environments on their platforms or use the [docker image](https://wasmedge.org/docs/contribute/source/docker).
    * As a standalone runtime library, WasmEdge does not differentiate between Proof of Concept, Development, Test, and Production environments. The same binary and configuration can be used across all environments. Environment-specific behavior is controlled by the caller's configuration (e.g., enabling debug core dumps in development, disabling them in production).
  * Define any specific service dependencies the project relies on in the cluster.
    *  If users install WasmEdge from the installer, the only requirement is the `glibc` version on Linux platforms. Currently, WasmEdge requires `glibc` version `2.28` or above.
  * Describe how the project implements Identity and Access Management.
    * By default, WasmEdge will be installed under the `$HOME` directory by the installer, and can be accessed by the owner of the `$HOME` directory. Users can also install WasmEdge under the system directory so that all users can access it.
    * From a runtime perspective, WasmEdge implements capability-based access control through WASI. WASM programs have no access to host resources by default. Access to the filesystem, environment variables, and sockets must be explicitly granted via CLI options (e.g., `--dir`, `--env`) or host function registration via the C API. This follows the principle of least privilege.
  * Describe how the project has addressed sovereignty.
    * WasmEdge is a runtime to instantiate and execute the WebAssembly programs provided by users with parameters, configurations, and input data. By installing WasmEdge in a local environment, WasmEdge returns data sovereignty to individual users.
  * Describe any compliance requirements addressed by the project.
    * Currently, WasmEdge does not claim compliance with specific security standards (e.g., SOC2, FIPS). The project maintains a [OpenSSF Best Practices](https://bestpractices.coreinfrastructure.org/projects/5059) badge and uses [FOSSA](https://fossa.com/) for license compliance scanning. An SBOM in SPDX format is published with each release. As the project matures toward v1.0, compliance with relevant standards will be evaluated.
  * Describe the project’s High Availability requirements.
    * For users who use WasmEdge directly, no matter by API or CLI, the mean time to repair is much shorter than the mean time to failure for the following reasons:
      * As a WASM runtime, WasmEdge passed the WASM test suites to cover the most cases of WASM instructions. Furthermore, fuzzing tests help the WasmEdge project to handle more edge cases to prevent failure.
      * As a lightweight runtime, users just need to restart WasmEdge quickly to recover from failure
    * For users who use WasmEdge integrated in containers such as `crun`, `docker`, `containerd`, or `Kubernetes`, the High Availability requirements are handled by the containers.
  * Describe the project’s resource requirements, including CPU, Network and Memory.
    * WasmEdge is designed as a lightweight runtime with minimal resource requirements:
      * CPU: WasmEdge can run on a wide range of architectures including x86\_64, aarch64, RISC-V, and s390x. The AOT compiler defaults to O3 optimization for best performance. No minimum CPU requirement beyond what is needed to run the host application.
      * Memory: The default WASM memory page size is 65,536 bytes, and memory instances are limited to 65,536 pages (up to 4 GB). The WasmEdge runtime itself has a small memory footprint suitable for edge and embedded environments. Users can configure the memory page limit.
      * Network: WasmEdge has no network requirements by default. Network access is only available when the WASM program uses WASI socket interfaces.
  * Describe the project’s storage requirements, including its use of ephemeral and/or persistent storage.
    * WasmEdge primarily uses ephemeral storage during execution. No persistent storage is required for normal operation. The installation artifacts (binaries, headers, shared libraries, and plug-ins) are stored in the installation path (default `$HOME/.wasmedge/`). AOT-compiled WASM files can be optionally saved to disk by the user. WasmEdge does not create temporary files, caches, or databases during runtime execution.
  * Please outline the project’s API Design:
    * Describe the project’s API topology and conventions
      * WasmEdge provides APIs and structure definitions for users to control the workflow of WASM execution and maintain the resources. Every structure and API has a `WasmEdge_` prefix in its name.
      * For the structures of handling resources, the structure names end with `Context`, and users should use the APIs to maintain the resource life cycles.
      * The APIs whose names end with `Create` are used for creating the `Context` resources, and the APIs whose names end with `Delete` are for releasing the corresponding resources.
    * Describe the project defaults
      * The defaults for WASM AOT compiler configuration:
        * The optimization level of the AOT compiler defaults to `O3`.
        * The output format defaults to universal WASM, which embeds generated code in a WASM custom section.
        * Interruptible mode defaults to `false` to accelerate WASM execution.
      * The defaults for WASM runtime:
        * The memory instance page size is 65536 bytes.
        * The memory page limit is 65536 pages.
        * JIT mode default to `false`.
        * Core dump for debugging defaults to `false`.
        * All standard WASM proposals (up to and including WASM 3.0) are enabled by default. Experimental or unimplemented proposals (e.g., Annotations) are disabled. Users can disable individual proposals via CLI flags or API configuration.
      * The defaults for plug-ins:
        * The plug-in search path is computed relative to the WasmEdge library installation location. For local installations, this is typically `$HOME/.wasmedge/plugin`. For system installations (under `/usr`), the path resolves to the system library directory (e.g., `/usr/lib/wasmedge`).
        * Users can override the plug-in search path by setting the `WASMEDGE_PLUGIN_PATH` environment variable.
    * Outline any additional configurations from default to make reasonable use of the project
      * When executing WasmEdge via the CLI or APIs, the following can be additionally configured:
        * For the AOT compiler configuration, interruptible mode can be enabled for asynchronous calls.
        * The cost measuring, time measuring, and instruction counting can be enabled for statistics.
        * Users can set the memory page limit.
        * Users can enable or disable individual WASM proposals via CLI flags or API configuration.
    * Describe any new or changed API types and calls - including to cloud providers - that will result from this project being enabled and used
      * The new APIs will be announced in the release notes of the new versions. Furthermore, the documents will publish the [upgrade guide](https://wasmedge.org/docs/embed/c/reference/upgrade_to_0.14.0) to the new versions.
      * The deprecated APIs will be marked as `deprecated` in the WasmEdge header with the comments. WasmEdge will keep maintaining these APIs for at least 2 minor releases.
    * Describe compatibility of any new or changed APIs with API servers, including the Kubernetes API server
      * WasmEdge does not directly interact with the Kubernetes API server. It does not define CRDs, admission webhooks, or API server extensions. Instead, WasmEdge integrates with Kubernetes through standard OCI container runtime interfaces (e.g., via `crun`, `containerd`, Docker). As a result, WasmEdge is compatible with any Kubernetes version that supports OCI-compliant container runtimes.
    * Describe versioning of any new or changed APIs, including how breaking changes are handled
      *  WasmEdge maintains the [API versioning with shared library version](https://wasmedge.org/docs/embed/c/reference/latest#abi-compatibility). If there are breaking changes of APIs, the `SOVERSION` will be changed.
  * Describe the project’s release processes, including major, minor and patch releases.
    * WasmEdge will update the patch release every quarter, or sooner if important issues need to be fixed. WasmEdge will follow the [release process guide](https://wasmedge.org/docs/contribute/release) to publish the `alpha`, `beta`, and `rc` pre-release for testing. If there are new APIs or API changes, the next version will update the minor release number. After the `1.0.0` version in the future, WasmEdge will update the major version number if there are API breaking changes.

### Installation

  * Describe how the project is installed and initialized, e.g. a minimal install with a few lines of code or does it require more complex integration and configuration?
    * WasmEdge can be installed with a single command on all major platforms (e.g., `brew install wasmedge` on macOS, `apt install wasmedge` on Debian/Ubuntu, or the universal installer script). No complex integration or configuration is required for basic usage. See the [Day 1 - Project Installation and Configuration](#project-installation-and-configuration) section for the full platform list and installer options.
  * How does an adopter test and validate the installation?
    * After installation, users can use the command `wasmedge -v` to check the installed version. The message such as `wasmedge version 0.16.1` will show on the terminal if the installation succeeded.

### Security

  * Please provide a link to the project’s cloud native [security self assessment](https://tag-security.cncf.io/community/assessments/).
    * The WasmEdge [security self-assessment](https://github.com/WasmEdge/WasmEdge/blob/master/docs/self-assessment.md) is available in the project repository. The assessment stage is currently marked as incomplete and serves as the foundation for the CNCF TAG-Security joint-assessment required for incubation.
  * Please review the [Cloud Native Security Tenets](https://github.com/cncf/tag-security/blob/main/community/resources/security-whitepaper/secure-defaults-cloud-native-8.md) from TAG Security.
    * How are you satisfying the tenets of cloud native security projects?
      * **Least privilege by default**: WebAssembly programs run in a sandboxed environment with no access to host OS resources unless explicitly granted. JIT mode, core dumps, and debug features are all disabled by default.
      * **Defense in depth**: WasmEdge employs multiple layers of security — bytecode validation before execution (Validator), memory isolation via the WASM linear memory model, and capability-based access control for filesystem and network resources via WASI.
      * **Secure defaults**: Optional runtime features (JIT, interruptible execution, core dump) are disabled by default. Filesystem access requires explicit `--dir` configuration with optional `readonly` flag.
    * Describe how each of the cloud native principles apply to your project.
      * **Zero trust**: WASM bytecode is validated against the WebAssembly specification before execution. No bytecode is trusted by default.
      * **Immutability**: WebAssembly modules are immutable once compiled. The AOT compiler produces deterministic output.
      * **Minimal attack surface**: As a standalone runtime, WasmEdge has no network listeners, no default filesystem access, and no ambient authority.
    * How do you recommend users alter security defaults in order to "loosen" the security of the project? Please link to any documentation the project has written concerning these use cases.
      * Enable filesystem access via `--dir guest_path:host_path` (or with `:readonly` suffix for read-only access). Documentation: [WASI CLI options](https://wasmedge.org/docs/start/build-and-run/cli).
      * Disable specific WASM proposals via CLI flags (e.g., `--disable-simd`) or API configuration if not needed.
      * Enable JIT mode or interruptible execution for specific use cases.
  * Security Hygiene
    * Please describe the frameworks, practices and procedures the project uses to maintain the basic health and security of the project.
      * **Static analysis**: [CodeQL](https://github.com/WasmEdge/WasmEdge/blob/master/.github/workflows/codeql-analysis.yml) runs on every push to master and every pull request. Weekly [Infer](https://github.com/WasmEdge/WasmEdge/blob/master/.github/workflows/static-code-analysis.yml) static analysis is also performed.
      * **Fuzzing**: WasmEdge participates in [Google OSS-Fuzz](https://github.com/google/oss-fuzz/tree/master/projects/wasmedge) for continuous fuzzing. Additional component fuzzing tests are integrated into the CI pipeline.
      * **Dependency management**: GitHub Actions dependencies are pinned to specific commit hashes (immutable references). [Dependabot](https://github.com/WasmEdge/WasmEdge/blob/master/.github/dependabot.yml) is enabled for weekly automated dependency updates. [step-security/harden-runner](https://github.com/step-security/harden-runner) is used across CI workflows for egress auditing.
      * **Code review**: All pull requests require maintainer approval before merge. Contributors must sign off on commits. CI must pass before merge.
      * **SBOM**: A software bill of materials in [SPDX format](https://github.com/WasmEdge/WasmEdge/blob/master/LICENSE.spdx) is generated and published with each release. WasmEdge also publishes the SBOM of each released version under the release pages.
    * Describe how the project has evaluated which features will be a security risk to users if they are not maintained by the project?
      * Features evaluated as security risks if unmaintained include the WASI host function implementations (filesystem, sockets, environment variables) and any third-party plug-in interfaces, as these form the boundary between the WASM sandbox and the host OS.
  * Cloud Native Threat Modeling
    * Explain the least minimal privileges required by the project and reasons for additional privileges.
      * WasmEdge requires no special OS privileges to run. It executes as a regular user-space process. No root access, no capabilities, and no seccomp profile modifications are needed. Additional privileges are only required if the user chooses to mount and interact with a system path (e.g., `/usr/local`), which requires write access to that directory.
    * Describe how the project is handling certificate rotation and mitigates any issues with certificates.
      * WasmEdge does not manage TLS certificates or establish persistent network connections by default. Certificate management is the responsibility of the host application or orchestration layer integrating WasmEdge.
    * Describe how the project is following and implementing [secure software supply chain best practices](https://project.linuxfoundation.org/hubfs/CNCF_SSCP_v1.pdf)
      * WasmEdge follows supply chain best practices including:
        * SBOM generation in SPDX format published with each release
        * GitHub Actions pinned to immutable commit hashes
        * Dependabot for automated dependency updates
        * step-security/harden-runner for CI workflow hardening
        * CodeQL and Infer for automated vulnerability detection
        * [OpenSSF Best Practices](https://bestpractices.coreinfrastructure.org/projects/5059) badge
        * [FOSSA](https://fossa.com/) for license compliance
        * 90-day vulnerability disclosure timeline with dedicated security contacts ([embargo policy](https://github.com/WasmEdge/WasmEdge/blob/master/docs/embargo-policy.md))

## Day 1 - Installation and Deployment Phase

### Project Installation and Configuration

 * WasmEdge can be installed on most popular platforms.
   * On Windows platforms, WasmEdge can be installed by `winget`: `winget install wasmedge`
   * On MacOS platforms, WasmEdge can be installed by `HomeBrew`: `brew install wasmedge`
   * On Fedora and Red Hat Linux, WasmEdge can be installed by `dnf install wasmedge`
   * On Ubuntu and Debian, WasmEdge can be installed by `apt install wasmedge`
   * For general Linux and MacOS platforms, users can install WasmEdge by the WasmEdge installer via the command `curl -sSf https://raw.githubusercontent.com/WasmEdge/WasmEdge/master/utils/install.sh | bash` in terminal. Or use [wasmedgeup](https://github.com/WasmEdge/wasmedgeup) CLI binary to install WasmEdge with the command `wasmedgeup install`.
 * WasmEdge configuration can be done by the [WasmEdge installer](https://wasmedge.org/docs/contribute/installer) with options.
   * `-v`: Specify the WasmEdge version to install.
   * `-p`: Specify the installation path. For example, use `-p /usr/local` to install WasmEdge under the system path.
   * `--plugins`: Install WasmEdge with plug-ins. For example, use `--plugins wasi_crypto:0.14.1` to install the plug-in with a specific version.
   * `--arch`: Forcibly specify the architecture of WasmEdge to install: `x86_64` or `aarch64`.
   * `--dist`: Forcibly specify the target distribution of Linux to install: `ubuntu20.04` or `manylinux2_28`.

### Project Enablement and Rollback

  * How can this project be enabled or disabled in a live cluster? Please describe any downtime required of the control plane or nodes.
    * It depends on how live clusters use WasmEdge. For clusters that use WasmEdge via CLI, the WasmEdge process will only be enabled during the invocation. However, for clusters that link WasmEdge via API, users should turn off the operators running WasmEdge only when they want to upgrade or reinstall the WasmEdge versions.
  * Describe how enabling the project changes any default behavior of the cluster or running workloads.
    * Enabling WasmEdge does not change any default behavior of the cluster or running workloads. WasmEdge is a standalone runtime that executes WebAssembly programs provided by users. It does not modify the underlying cluster behavior or workload execution unless users explicitly choose to run their workloads with WasmEdge as the runtime.
  * Describe how the project tests enablement and disablement.
    * WasmEdge tests enablement and disablement through CI workflows that validate the installer and uninstaller scripts across supported platforms. The CI tests ensure that the minimum and maximum supported versions can be installed and that the uninstaller correctly removes all WasmEdge resources.
  * How does the project clean up any resources created, including CRDs?
    * For users who installed WasmEdge by the installer, all created resources, downloaded plug-ins, and libraries and headers are all in the installation path. Users can delete the installation path (usually in `$HOME/.wasmedge`) to clean up all resources, or use the [uninstaller](https://wasmedge.org/docs/start/install#uninstall) command: `bash <(curl -sSf https://raw.githubusercontent.com/WasmEdge/WasmEdge/master/utils/uninstall.sh)`.

### Rollout, Upgrade and Rollback Planning

  * How does the project intend to provide and maintain compatibility with infrastructure and orchestration management tools like Kubernetes and with what frequency?
    * WasmEdge expects to release at least a new version each quarter. If there are important features or breaking changes, the minor version number will be increased. Otherwise, the patch number will be increased. Before the `1.0.0` version, the latest 2 minor versions will be long-term supported versions, and the fixing patches will keep updating to these versions.
  * Describe how the project handles rollback procedures.
    * If there are important issues or bugs after publishing a release version, WasmEdge doesn't rollback the version, but publishes a new release version to fix the issues or bugs. For example, if an issue should be fixed quickly after `0.15.0` is announced, then WasmEdge will publish the `0.15.1` version to fix that issue.
  * How can a rollout or rollback fail? Describe any impact to already running workloads.
    * The rollout or rollback will occur only when users update their WasmEdge version. To install or reinstall WasmEdge with a new version, users should stop their running workloads.
  * Describe any specific metrics that should inform a rollback.
    * Issues or bugs in new features not detected during the `alpha` or `beta` stages.
    * Unexpected breaking changes in a `patch` version that should be non-breaking.
    * Security issues discovered in a newly released feature.
  * Explain how upgrades and rollbacks were tested and how the upgrade-\>downgrade-\>upgrade path was tested.
    * WasmEdge has CI to test the installer to ensure the minimum and maximum supported versions can be installed. On the other hand, WasmEdge publishes a new patch version if a rollback is needed. This means that if the CI passed the testing of the newest WasmEdge version installation, the upgrades and rollback paths are tested.
  * Explain how the project informs users of deprecations and removals of features and APIs.
    * In WasmEdge API headers, the deprecated APIs are marked with comments to notify users. The removals of features and APIs are also listed in the release notes of versions.
    * WasmEdge CLI will show the deprecated warning messages to notify users when they are using some features that are planned for removal. Once the cooldown period is finished, we will then remove these features.
  * Explain how the project permits utilization of alpha and beta capabilities as part of a rollout.
    * When a new feature is added to WasmEdge, new options or APIs are supplied. Even though it's an alpha or beta feature, users can use those new options to enable them after updating the WasmEdge version.
