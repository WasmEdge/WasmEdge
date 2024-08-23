group "default" {
  targets = [
    "clang",
    "clang-plugins",
    "gcc",
    "gcc-plugins"
  ]
}

target "base" {
  dockerfile = "Dockerfile.ubuntu-base"
  context    = "./utils/docker"
}

target "plugins-base" {
  dockerfile = "./docker/Dockerfile.ubuntu-plugins-deps"
  context    = "./utils"
}

target "clang" {
  inherits = ["base"]
  tags     = [
    "wasmedge/wasmedge:latest",
    "wasmedge/wasmedge:ubuntu-build-clang"
  ]
}

target "clang-plugins" {
  inherits = ["plugins-base"]
  tags     = ["wasmedge/wasmedge:ubuntu-build-clang-plugins-deps"]
  contexts = {
    "wasmedge/wasmedge:ubuntu-build-clang" = "target:base"
  }
  args     = {
    BASE_IMAGE = "wasmedge/wasmedge:ubuntu-build-clang"
  }
}

target "gcc" {
  inherits = ["base"]
  tags     = ["wasmedge/wasmedge:ubuntu-build-gcc"]
  args     = {
    TOOLCHAIN = "gcc"
  }
}

target "gcc-plugins" {
  inherits = ["plugins-base"]
  tags     = ["wasmedge/wasmedge:ubuntu-build-gcc-plugins-deps"]
  contexts = {
    "wasmedge/wasmedge:ubuntu-build-gcc" = "target:base"
  }
  args     = {
    BASE_IMAGE = "wasmedge/wasmedge:ubuntu-build-gcc"
  }
}
