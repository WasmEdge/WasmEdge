group "default" {
  targets = [
    "x86_64",
    "aarch64"
  ]
}

target "base" {
  dockerfile = "Dockerfile.ci-image-base"
  context    = "./utils/docker"
}

target "x86_64" {
  inherits  = ["base"]
  platforms = ["linux/amd64"]
  tags      = [
    "wasmedge/wasmedge:ci-image-base",
    "wasmedge/wasmedge:ci-image-base_x86_64"
  ]
}

target "aarch64" {
  inherits  = ["base"]
  platforms = ["linux/arm64"]
  tags      = ["wasmedge/wasmedge:ci-image-base_aarch64"]
}
