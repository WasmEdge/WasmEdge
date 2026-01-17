group "default" {
  targets = ["alpine-base"]
}

target "alpine-base" {
  dockerfile = "Dockerfile.alpine-base"
  context    = "utils/docker"
  # Build for both major architectures at once
  platforms  = ["linux/amd64", "linux/arm64"]
  # Tag it clearly so we can use it later
  tags       = ["wasmedge/wasmedge:alpine-base-3.23"]
}
