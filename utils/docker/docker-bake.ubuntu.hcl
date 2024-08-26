group "default" {
  targets = [
    "base",
    "latest",
    "plugins"
  ]
}

function "name" {
  params = [toolchain, ubuntu]
  result = "${toolchain}-ubuntu${replace(ubuntu, ".", "")}"
}

function "tag" {
  params = [toolchain, ubuntu]
  result = equal(ubuntu, "22.04") ? "ubuntu-build-${toolchain}" : "ubuntu-${ubuntu}-build-${toolchain}"
}

variable "matrix" {
  default = {
    toolchain = ["clang", "gcc"]
    ubuntu    = ["20.04", "22.04"]
  }
}

target "base" {
  matrix     = matrix
  name       = name(toolchain, ubuntu)

  dockerfile = "Dockerfile.ubuntu-base"
  context    = "./utils/docker"

  tags       = ["wasmedge/wasmedge:${tag(toolchain, ubuntu)}"]
  args       = {
    TOOLCHAIN = toolchain
  }
}

target "plugins" {
  matrix     = matrix
  name       = "${name(toolchain, ubuntu)}-plugins"

  dockerfile = "./docker/Dockerfile.ubuntu-plugins-deps"
  context    = "./utils"

  contexts   = {
    "wasmedge/wasmedge:${tag(toolchain, ubuntu)}" = "target:${name(toolchain, ubuntu)}"
  }

  tags       = ["wasmedge/wasmedge:${tag(toolchain, ubuntu)}-plugins-deps"]
  args       = {
    BASE_IMAGE = "wasmedge/wasmedge:${tag(toolchain, ubuntu)}"
  }
}

target "latest" {
  matrix     = {
    toolchain = ["clang"]
    ubuntu    = ["22.04"]
  }
  inherits = ["${name(toolchain, ubuntu)}"]
  contexts = {
    "wasmedge/wasmedge:${tag(toolchain, ubuntu)}" = "target:${name(toolchain, ubuntu)}"
  }
  tags     = ["wasmedge/wasmedge:latest"]
}
