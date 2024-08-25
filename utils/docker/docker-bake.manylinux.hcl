target "base" {
  dockerfile = "Dockerfile.manylinux_2_28-base"
  context    = "./utils/docker"
}

target "plugins-base" {
  dockerfile = "./docker/Dockerfile.manylinux_2_28-plugins-deps"
  context    = "./utils"
}

target "x86_64" {
  inherits  = ["base"]
  platforms = ["linux/amd64"]
  tags      = ["wasmedge/wasmedge:manylinux_2_28_x86_64"]
  args      = {
    LLVM_TARGETS = "X86;BPF",
    LLVM_TRIPLE  = "x86_64-pc-linux-gnu"
  }
}

target "x86_64-plugins" {
  inherits  = ["plugins-base"]
  platforms = ["linux/amd64"]
  tags      = ["wasmedge/wasmedge:manylinux_2_28_x86_64-plugins-deps"]
  contexts  = {
    "wasmedge/wasmedge:manylinux_2_28_x86_64"= "target:x86_64"
  }
}

target "aarch64" {
  inherits  = ["base"]
  platforms = ["linux/arm64"]
  tags      = ["wasmedge/wasmedge:manylinux_2_28_aarch64"]
  args      = {
    BASE_IMAGE   = "quay.io/pypa/manylinux_2_28_aarch64",
    LLVM_TARGETS = "AArch64;BPF",
    LLVM_TRIPLE  = "aarch64-redhat-linux-gnu"
  }
}

target "aarch64-plugins" {
  inherits  = ["plugins-base"]
  platforms = ["linux/arm64"]
  tags      = ["wasmedge/wasmedge:manylinux_2_28_aarch64-plugins-deps"]
  contexts  = {
    "wasmedge/wasmedge:manylinux_2_28_aarch64" = "target:aarch64"
  }
  args      = {
    BASE_IMAGE = "wasmedge/wasmedge:manylinux_2_28_aarch64"
  }
}
