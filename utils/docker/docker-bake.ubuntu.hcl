group "default" {
  targets = [
    "cuda",
    "clang",
    "gcc"
  ]
}

group "latest" {
  targets = [
    "base-2204-clang",
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
  result = target == "base" && ubuntu == "22.04" && toolchain == "clang" ? "latest" : ""
}

function "tags-latest-backports" {
  params = [target, ubuntu, toolchain]
  result = ubuntu == "22.04" ? join("-", compact([
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
  result = target == "base" && toolchain == "clang" ? "ubuntu-${ubuntu}" : ""
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
    ubuntu = ["20.04", "22.04"]
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
    ubuntu = ["20.04", "22.04"]
  }

  inherits   = ["base-${no-dot(ubuntu)}"]
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

target "clang" {
  matrix     = {
    parent = ["base", "plugins"]
    ubuntu = ["20.04", "22.04"]
  }

  inherits   = ["${parent}-${no-dot(ubuntu)}"]
  name       = "${parent}-${no-dot(ubuntu)}-clang"
  contexts   = {
    "local/tmp:${parent}-${ubuntu}" = "target:${parent}-${no-dot(ubuntu)}"
  }
  tags       = tags(parent, ubuntu, "clang")
}

target "gcc" {
  dockerfile = "Dockerfile.ubuntu-gcc"
  context    = "./utils/docker"

  matrix     = {
    parent = ["base", "plugins"]
    ubuntu = ["20.04", "22.04"]
  }

  inherits   = ["${parent}-${no-dot(ubuntu)}"]
  name       = "${parent}-${no-dot(ubuntu)}-gcc"
  contexts   = {
    "local/tmp:${parent}-${ubuntu}" = "target:${parent}-${no-dot(ubuntu)}"
  }
  tags       = tags(parent, ubuntu, "gcc")
  args       = {
    BASE_IMAGE = "local/tmp:${parent}-${ubuntu}"
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
