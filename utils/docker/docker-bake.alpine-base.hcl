group "default" {
  targets = ["alpine-base"]
}

target "alpine-base" {
  dockerfile = "Dockerfile.alpine-base"
  context    = "utils/docker"

  matrix     = {
    arch = ["amd64", "arm64"]
  }

  name       = "alpine-base-${arch}"
  platforms  = ["linux/${arch}"]
  tags       = ["wasmedge/wasmedge:alpine-base-3.23-${arch}"]
}
