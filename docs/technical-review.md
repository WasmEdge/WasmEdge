# General Technical Review - [WasmEdge] / [Incubation]

- **Project:** WasmEdge
- **Project Version:** 0.15.0
- **Website:** https://wasmedge.org/
- **Date Updated:** 2025-10-16
- **Template Version:** v1.0
- **Description:** WasmEdge is a lightweight, high-performance, and extensible WebAssembly runtime.



## Day 0 - Planning Phase

### Scope

  * Describe the roadmap process, how scope is determined for mid to long term features, as well as how the roadmap maps back to current contributions and maintainer ladder?
    * The WasmEDge roadmap can be find at https://github.com/WasmEdge/WasmEdge/blob/master/docs/ROADMAP.md#current-roadmap and the process is described in the [document](https://github.com/WasmEdge/WasmEdge/blob/master/docs/ROADMAP.md).
    * WasmEdge maintainers will create a roadmap discussion issue in every quarter of years, and welcome everyone to add roadmap entries with the planned timeline. We'll assign the roadmap entries to the owners, or refer to the [code owner list](https://github.com/WasmEdge/WasmEdge/blob/master/.github/CODEOWNERS) to determine the roadmap owner.
  * Describe the target persona or user(s) for the project?
    * Cloud native developers: Use WasmEdge as a lightweight and cross-platform cloud native runtime without cold start time
    * AI developers: use WasmEdge as a lightweight and cross-platform LLM runtime
    * Blockchain developer: Use WasmEdge as the smart contract engine
    * Application deverloper: use WasmEdge to developer microservices
  * Explain the primary use case for the project. What additional use cases are supported by the project?
    * The primary use case is to use WasmEdge as a lightweight, high-performance, and extensible WebAssembly runtime. It can be included
      * LLM inference
      * Embedded runtime
      * Severless runtime
      * apllication runtime 
  * Explain which use cases have been identified as unsupported by the project.
    *  WasmEdge is a runtime to execute WebAssembly programs with extentions. Therefore, we expected the inputs are WASM files, not the host languages before compilation. For the tasks about compiling programs into WASM, injecting codes into WASM, or combinding multiple WASM files, are identified as the toolchain's tasks. 
  * Describe the intended types of organizations who would benefit from adopting this project. (i.e. financial services, any software manufacturer, organizations providing platform engineering services)?  
  * Please describe any completed end user research and link to any reports.
    * Current case studies are documented in [the user list](https://wasmedge.org/docs/contribute/users), including WikiFunctions, LF Edge eKuiper, crun, KubeEdge, Docker, Gaia, and 5miles. Formal user research reports are planned as the project matures toward v1.0.



### Usability

  * How should the target personas interact with your project?
    * The target personas interact with WasmEdge is via the WasmEdge CLI or the WasmEdge C API.
  * Describe the user experience (UX) and user interface (UI) of the project.
    * WasmEdge provides the [CLI tools](https://wasmedge.org/docs/start/build-and-run/cli) for the user interface. As a WASM runtime, users can interact with WasmEdge via the commands including the input files and options easily.
    * For developers who want to bind WasmEdge into their host applications, WasmEdge provides the C API header and shared libraries with documents. Developers can refer to the API docs to invoke the WasmEdge APIs for good user experience.
  * Describe how this project integrates with other projects in a production environment.
    * Docker: [Docker integrated WasmEdge](https://wasmedge.org/docs/start/getting-started/quick_start_docker) as one of a WASM runtime with the `containerd` image store. Users can install a docker image via the Docker Desktop to run their WASM files. 
    * crun: After installation of `crun` and WasmEdge, users can use [crun to deploy WASM images](https://wasmedge.org/docs/develop/deploy/oci-runtime/crun/). The `crun` interacts WasmEdge with the C APIs, providing the easier way to deploy WASM images on the fedora platforms.
    * podman: Furthermore with installing `crun-wasm` and `podman`, users can [use the podman tool](https://wasmedge.org/docs/zh-tw/start/getting-started/quick_start_redhat/) to create and manage the containeriz
    * Gaia: Use WasmEdge as the cross-platform LLM runtime
    * 5miles: use Kubernetes and WasmEdge to host whisper models

### Design

  * Explain the design principles and best practices the project is following.
    * WasmEdge mainly follow the [LLVM coding standards](https://llvm.org/docs/CodingStandards.html). The coding style is decorated by the `clang-format`, as well as the naming style and comment formatting follow the LLVM style. For the API designs, although it's different, WasmEdge mainly follow the [WASM C API](https://github.com/WebAssembly/wasm-c-api) usages. 
  * Outline or link to the project’s architecture requirements? Describe how they differ for Proof of Concept, Development, Test and Production environments, as applicable.
    * WasmEdge uses [GitHub CI](https://github.com/WasmEdge/WasmEdge/blob/master/.github/workflows/build.yml) to build, test, and release the project. We currently support Windows, MacOS, and Linux platforms. Developers can refer to the CI `.yml` files to setup their environments on their platforms or use the [docker image](https://wasmedge.org/docs/contribute/source/docker). 
  * Define any specific service dependencies the project relies on in the cluster.
    *  If users install WasmEdge from the installer, the only requirement is the `glibc` version on Linux platforms and the `python` for the installer using. Currently, WasmEdge requires `glibc` version over `2.28`.
  * Describe how the project implements Identity and Access Management.
    * By default, WasmEdge will be installed under the `$Home` directory by the installer, and can be accessed by the owner of `$Home` directory. Users can also install WasmEdge under the system directory so that all users can access to it.
  * Describe how the project has addressed sovereignty.
    * WasmEdge is a runtime to instantiate and execute the WebAssembly programs provided by users with parameters, configurations, and input data. With installing WasmEdge in local environment, WasmEdge returns the data soverignty to the user individuals.
  * Describe any compliance requirements addressed by the project.
  *   
  * Describe the project’s High Availability requirements.
    * For users use WasmEdge directly no matter by API or CLI, the mean time to repair is much shorter than the mean time to failure because of the reasons:
      * As a WASM runtime, WasmEdge passed the WASM test suites to cover the most cases of WASM instructions. Furthermore, fuzzing tests help the WasmEdge project to handle more edge cases to prevent from failure.
      * As a light-weight runtime, users just need to restart WasmEdge quickly to repair from failure
    * For users use WasmEdge integrated in containers such as `crun`, `docker`, `containerd`, or `Kubernetes`, the High Availability requirements are handled by the containers.
  * Describe the project’s resource requirements, including CPU, Network and Memory.
    *  
  * Describe the project’s storage requirements, including its use of ephemeral and/or persistent storage.
    * 
  * Please outline the project’s API Design:  
    * Describe the project’s API topology and conventions
      * WasmEdge provides APIs and structure definitions for users to control the workflow of WASM exection and maintain the resources. Every structures and APIs has `WasmEdge_` prefix in their names.
      * For the structures of handling resources, the structure names are ended with `Context`, and users should use the APIs to maintain the resource life cycles.
      * The APIs named ended with `Create` are used for creating the `Context` resources, and the APIs named ended with `Delete` are for releasing the corresponding resources.
    * Describe the project defaults
      * There are the defaults about WASM AOT compiler configuration:
        * The optimization level of AOT compiler would be `O3`.
        * The output format would be universal WASM, which added generated code in WASM custom section.
        * The generic binary would be `false` to support the new CPU architectures.
        * The interruptible would be `false` to accelerate the WASM execution.
      * There are the defaults about WASM runtime:
        * The page size of memory instance is 65536 bytes.
        * The limitation of memory page is 65536 pages.
        * The JIT mode and force-interpreter mode would be `false`.
        * The core-dump for debugging would be `false`.
        * The WASM proposals not merged into standard would all be disabled.
      * There are the defaults about plug-ins:
        * The plug-in searching path would be at `$HOME/.wasmedge/plugin` if the WasmEdge is installed locally.
        * The plug-in searching path would be at `$LIBRARY_PATH/wasmedge` if the WasmEdge is installed under the system path.
    * Outline any additional configurations from default to make reasonable use of the project
      * When executing WasmEdge via the CLI or APIs, the following can be additionally configured:
        * For the AOT compiler configuration, the interruptible can be enabled for asynchronous calls.
        * The cost measuring, time measuring, and instruction counting can be enabled for statistics.
        * Users can set the limitation page size of memory instances.
        * Users can turn on the supported WASM proposals and turn off the existing proposals.
    * Describe any new or changed API types and calls \- including to cloud providers \- that will result from this project being enabled and used
      * The new APIs will be announced in the release notes of the new versions. Furthermore, the documents will publish the [upgrade guide](https://wasmedge.org/docs/embed/c/reference/upgrade_to_0.14.0) to the new versions.
      * The deprecated APIs will be marked as `deprecated` in the WasmEdge header with the comments. WasmEdge will keep maintaining these APIs at least 2 minor releases.   
    * Describe compatibility of any new or changed APIs with API servers, including the Kubernetes API server
      * 
    * Describe versioning of any new or changed APIs, including how breaking changes are handled
      *  WasmEdge maintains the [API versioning with shared library version](https://wasmedge.org/docs/embed/c/reference/latest#abi-compatibility). If there are breaking changes of APIs, the `SOVERSION` will be changed. 
  * Describe the project’s release processes, including major, minor and patch releases.
    * WasmEdge will update the patch release in every quarter of year or shorter if something important issues should be fixed. WasmEdge will follow the [release process guide](https://wasmedge.org/docs/contribute/release) to publish the `alpha`, `beta`, and `rc` pre-release for testing. If there are new APIs or API changes, the next version will update the minor release number. After the `1.0.0` version in the future, WasmEdge will update the major version number if the API breaking changes. 

### Installation

  * Describe how the project is installed and initialized, e.g. a minimal install with a few lines of code or does it require more complex integration and configuration?
    * On Windows platforms, WasmEdge can be installed by `Chocolatey`: `choco install wasmedge`
    * On MacOS platforms, WasmEdge can be installed by `HomeBrew`: `brew install wasmedge`
    * On Fedora and Red Hat Linux, WasmEdge can be installed by `dnf install wasmedge`
    * On Ubuntu and Debian, WasmEdge can be installed by `apt install wasmedge`
    * For general Linux or MacOS platforms, users can install WasmEdge via the command `curl -sSf https://raw.githubusercontent.com/WasmEdge/WasmEdge/master/utils/install.sh | bash` in terminal.
  * How does an adopter test and validate the installation?
    * After installation, users can use the command `wasmedge -v` to check the installed version. The message such as `wasmedge version 0.15.0` will show on the terminal if the installation succeeded.

### Security

  * Please provide a link to the project’s cloud native [security self assessment](https://tag-security.cncf.io/community/assessments/).  
  * Please review the [Cloud Native Security Tenets](https://github.com/cncf/tag-security/blob/main/community/resources/security-whitepaper/secure-defaults-cloud-native-8.md) from TAG Security.  
    * How are you satisfying the tenets of cloud native security projects?  
    * Describe how each of the cloud native principles apply to your project.  
    * How do you recommend users alter security defaults in order to "loosen" the security of the project? Please link to any documentation the project has written concerning these use cases.  
  * Security Hygiene  
    * Please describe the frameworks, practices and procedures the project uses to maintain the basic health and security of the project.   
    * Describe how the project has evaluated which features will be a security risk to users if they are not maintained by the project?  
  * Cloud Native Threat Modeling  
    * Explain the least minimal privileges required by the project and reasons for additional privileges.  
    * Describe how the project is handling certificate rotation and mitigates any issues with certificates.  
    * Describe how the project is following and implementing [secure software supply chain best practices](https://project.linuxfoundation.org/hubfs/CNCF\_SSCP\_v1.pdf) 

## Day 1 \- Installation and Deployment Phase

### Project Installation and Configuration

 * WasmEdge can be installed on most popular platforms.
   * On Windows platforms, WasmEdge can be installed by `Chocolatey`: `choco install wasmedge`
   * On MacOS platforms, WasmEdge can be installed by `HomeBrew`: `brew install wasmedge`
   * On Fedora and Red Hat Linux, WasmEdge can be installed by `dnf install wasmedge`
   * On Ubuntu and Debian, WasmEdge can be installed by `apt install wasmedge`
   * For general Linux and MacOS platforms, users can install WasmEdge by the WasmEdge installer via the command `curl -sSf https://raw.githubusercontent.com/WasmEdge/WasmEdge/master/utils/install.sh | bash` in terminal.
 * WasmEdge configuration can be done by the [WasmEdge installer](https://wasmedge.org/docs/contribute/installer) with options.
   * `-v`: Specify the WasmEdge version to install.
   * `-p`: Specify the installation path. For example, use `-p /usr/local` to install WasmEdge under the system path.
   * `--plugins`: Install WasmEdge with plug-ins. For example, use `--plugins wasi_crypto:0.14.1` to install the plug-in with specific version.
   * `--machine` or `--arch`: Forcibly specify the architecture of WasmEdge to install: `x86_64` or `aarch64`.
   * `--dist`: Forcibly specify the target distribution of Linux to install: `ubuntu20.04` or `manylinux2_28`.  * Describe what project installation and configuration look like.

### Project Enablement and Rollback

  * How can this project be enabled or disabled in a live cluster? Please describe any downtime required of the control plane or nodes.
    * It depends on how live clusters use WasmEdge. For the clusters use WasmEdge via CLI, the WasmEdge process will only be enabled during the invocation. However, for the clusters link WasmEdge via API, users should turn off the clusters if and only if they want to upgrade or reinstall the WasmEdge versions.     
  * Describe how enabling the project changes any default behavior of the cluster or running workloads.
    *  WasmEdge is a runtime to instantiate and execute the WebAssembly programs provided by users with parameters, configurations, and input data. When running WASM programs, the data accessing will only occor on provided parameters by the cluster or running workloads.
  * Describe how the project tests enablement and disablement.
    * On Windows platforms who installed WasmEdge `Chocolatey`, users can use the command `choco uninstall wasmedge`.
    * On MacOS platforms who installed WasmEdge by `HomeBrew`, users can use the command `brew uninstall wasmedge`.
    * On Fedora and Red Hat Linux who installed WasmEdge by `dnf`, users can use the command `dnf remove wasmedge`.
    * On Ubuntu and Debian who installed WasmEdge by `apt`, users can use the command `apt remove wasmedge`.
  * How does the project clean up any resources created, including CRDs?
    * For users who installed WasmEdge by the installer, all created resources, downloaded plug-ins, and libraries and headers are all in the installation path. Users can delete the installation path (usually in `$HOME/.wasmedge`) to clean up all resources, or use the [uninstaller](https://wasmedge.org/docs/start/install#uninstall) command: `bash <(curl -sSf https://raw.githubusercontent.com/WasmEdge/WasmEdge/master/utils/uninstall.sh)`.

### Rollout, Upgrade and Rollback Planning

  * How does the project intend to provide and maintain compatibility with infrastructure and orchestration management tools like Kubernetes and with what frequency?
    * WasmEdge expects to release at least a new verion in each quarter of years. If there are important features or breaking changes, the minor version number will be increased. Otherwise, the patch number will be increased. Before the `1.0.0` version, the latest 2 minor version number will be long-term supported versions, and the fixing patches will keep updating to these versions. 
  * Describe how the project handles rollback procedures.
    * If there are important issues or bugs after publishing a release version, WasmEdge doesn't rollback the version, but publishes a new release version to fix the issues or bugs. For example, if an issue should be fixed quickly after the `0.15.0` announced, then WasmEdge will publish the `0.15.1` version to fixing that issue.
  * How can a rollout or rollback fail? Describe any impact to already running workloads.
    * The rollout or rollback will occur only when users updating their WasmEdge version. To install or reinstall the WasmEdge with new version, users should stop their running workloads.
  * Describe any specific metrics that should inform a rollback. 
    * First situation is that the issues or bugs of new features not detected during the `alpha` or `beta` stages.
    * Second situation is that unexpected breaking changes occur in a non-breaking change `patch` version.
    * Security issues occur in a new released feature.
  * Explain how upgrades and rollbacks were tested and how the upgrade-\>downgrade-\>upgrade path was tested.
    * WasmEdge has CI to test the installer to ensure the minimum and maximum supported versions can be installed. On the other hand, WasmEdge publishes a new patch version if a rollback is needed. This means that if the CI passed the testing of the newest WasmEdge version installation, the upgrades and rollback paths are tested.
  * Explain how the project informs users of deprecations and removals of features and APIs.
    *  In WasmEdge API headers, the deprecated APIs are marked with the comments to notice the users. And also, the removals of features and APIs are listed out in the release notes of versions.
  * Explain how the project permits utilization of alpha and beta capabilities as part of a rollout.
    * When a new feature added into WasmEdge, new options or APIs are supplied. Even though its an alpha or beta feature, users can use that new options to enable them after updating the WasmEdge version.

