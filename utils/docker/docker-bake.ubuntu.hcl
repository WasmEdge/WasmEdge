group "default" {
  targets = [
    "cuda",
    "final"
  ]
}

group "latest" {
  targets = [
    "base-2404-clang",
  ]
}

group "focal" {
  targets = [
    "base-2004-clang",
    "base-2004-gcc",
    "plugins-2004-clang",
    "plugins-2004-gcc",
  ]
}

group "jammy" {
  targets = [
    "base-2204-clang",
    "base-2204-gcc",
    "plugins-2204-clang",
    "plugins-2204-gcc",
  ]
}

group "noble" {
  targets = [
    "base-2404-clang",
    "base-2404-gcc",
    "plugins-2404-clang",
    "plugins-2404-gcc",
  ]
}

function "no-dot" {
  params = [ubuntu]
  result = replace(ubuntu, ".", "")
}

function "major" {
  params = [ubuntu]
  result = regex("^[[:digit:]]+", ubuntu)
}

function "tags-latest" {
  params = [target, ubuntu, toolchain]
  result = target == "base" && ubuntu == "24.04" && toolchain == "clang" ? "latest" : ""
}

function "tags-latest-backports" {
  params = [target, ubuntu, toolchain]
  result = ubuntu == "24.04" ? join("-", compact([
    "ubuntu",
    "build",
    toolchain,
    target == "plugins" ? "plugins-deps" : "",
  ])) : ""
}

function "tags-backports" {
  params = [target, ubuntu, toolchain]
  result = join("-", compact([
    "ubuntu",
    ubuntu,
    "build",
    toolchain,
    target == "plugins" ? "plugins-deps" : "",
  ]))
}

function "tags-simplified" {
  params = [target, ubuntu, toolchain]
  result = toolchain == "clang" ? join("-", compact([
    "ubuntu",
    ubuntu,
    target == "plugins" ? "plugins" : "",
  ])) : ""
}

function "tags" {
  params = [target, ubuntu, toolchain]
  result = [for tag in compact([
    tags-latest(target, ubuntu, toolchain),
    tags-latest-backports(target, ubuntu, toolchain),
    tags-backports(target, ubuntu, toolchain),
    tags-simplified(target, ubuntu, toolchain),
  ]) : "wasmedge/wasmedge:${tag}"]
}

target "base" {
  dockerfile = "Dockerfile.ubuntu-base"
  context    = "./utils/docker"

  matrix     = {
    ubuntu = ["20.04", "22.04", "24.04"]
  }

  name       = "base-${no-dot(ubuntu)}"
  tags       = ["local/tmp:base-${ubuntu}"]
  args       = {
    UBUNTU_VER = major(ubuntu)
  }
}

target "plugins" {
  dockerfile = "./docker/Dockerfile.ubuntu-plugins-deps"
  context    = "./utils"

  matrix     = {
    ubuntu = ["20.04", "22.04", "24.04"]
  }

  name       = "plugins-${no-dot(ubuntu)}"
  contexts   = {
    "local/tmp:base-${ubuntu}" = "target:base-${no-dot(ubuntu)}"
  }
  tags       = ["local/tmp:plugins-${ubuntu}"]
  args       = {
    BASE_IMAGE = "local/tmp:base-${ubuntu}"
    UBUNTU_VER = major(ubuntu)
  }
}

target "final" {
  matrix     = {
    parent = ["base", "plugins"]
    ubuntu = ["20.04", "22.04", "24.04"]
    toolchain = ["clang", "gcc"]
  }

  dockerfile = "Dockerfile.ubuntu-env"
  context    = "./utils/docker"

  name       = "${parent}-${no-dot(ubuntu)}-${toolchain}"
  contexts   = {
    "local/tmp:${parent}-${ubuntu}" = "target:${parent}-${no-dot(ubuntu)}"
  }
  tags       = tags(parent, ubuntu, toolchain)
  args       = {
    BASE_IMAGE = "local/tmp:${parent}-${ubuntu}"
    TOOLCHAIN  = toolchain
  }
}

target "cuda" {
  dockerfile = "Dockerfile.ubuntu-cuda"
  context    = "./utils/docker"

  matrix     = {
    cuda = ["11.3", "12.0"]
  }

  name       = "base-2004-gcc-cuda${major(cuda)}"
  contexts   = {
    "wasmedge/wasmedge:ubuntu-20.04-build-gcc" = "target:base-2004-gcc"
  }
  tags       = ["wasmedge/wasmedge:ubuntu-20.04-build-gcc-cuda${major(cuda)}"]
  args       = {
    BASE_IMAGE = "wasmedge/wasmedge:ubuntu-20.04-build-gcc"
    NVCC_VER   = replace(cuda, ".", "-")
  }
}

# TODO: Refactor with multi-arch image
target "base-2004-clang-aarch64" {
  inherits   = ["base-2004"]
  contexts   = {
    "local/tmp:base-2004" = "target:base-2004"
  }
  tags       = [for tag in tags("base", "20.04", "clang") : "${tag}-aarch64"]
  platforms = ["linux/arm64"]
}
