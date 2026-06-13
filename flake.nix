{
  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-24.05";
    flake-utils.url = "github:numtide/flake-utils";
  };

  outputs = { self, nixpkgs, flake-utils }:
    flake-utils.lib.eachDefaultSystem (system:
      let
        pkgs = import nixpkgs { inherit system; };
        llvmPackages = pkgs.llvmPackages_18;

        # The Nix build sandbox has no network access, so cmake's
        # FetchContent(tree-sitter) cannot reach github.com at configure time.
        # Pre-fetch the v0.26.9 tarball as a fixed-output derivation (which
        # runs outside the sandbox), unpack it into a source directory, and
        # hand the path to cmake via FETCHCONTENT_SOURCE_DIR_TREE-SITTER so
        # the in-tree FetchContent_Declare just picks up the local source.
        treeSitterTarball = pkgs.fetchurl {
          url = "https://github.com/tree-sitter/tree-sitter/archive/refs/tags/v0.26.9.tar.gz";
          sha256 = "8e14780500933f43d86662fcaa1b0ce99ebe9c220f4680bc929dce09a0e0cfc6";
        };
        treeSitterSrc = pkgs.runCommand "tree-sitter-v0.26.9-src" {} ''
          mkdir -p $out
          tar -xzf ${treeSitterTarball} -C $out --strip-components=1
        '';

        wasmedge_buildInputs = with pkgs; [
          cmake
          git
          llvmPackages.clang-unwrapped
          llvmPackages.lld
          llvmPackages.llvm
          openssl
          pkg-config
          libxml2
          spdlog
        ] ++ pkgs.lib.optionals (system == "x86_64-darwin" || system == "aarch64-darwin") [
          pkgs.darwin.apple_sdk.frameworks.Foundation
        ];
        wasmedge = pkgs.stdenv.mkDerivation {
          name = "wasmedge";
          version = "0.14.0";
          src = ./.;

          buildInputs = wasmedge_buildInputs;
          cmakeFlags = [
            "-DCMAKE_BUILD_TYPE=Debug"
            "-DWASMEDGE_BUILD_PLUGINS=OFF"
            "-DWASMEDGE_BUILD_TESTS=OFF"
            "-DWASMEDGE_USE_LLVM=ON"
            "-DFETCHCONTENT_SOURCE_DIR_TREE-SITTER=${treeSitterSrc}"
          ];
        };
      in with pkgs; rec {
        packages = { wasmedge = wasmedge; };
        packages.default = packages.wasmedge;
        devShells.default = mkShell {
          buildInputs = [
            wasmedge
            ninja
            gcovr
          ] ++ wasmedge_buildInputs;

          LIBCLANG_PATH = "${llvmPackages.libclang.lib}/lib";
        };
      });
}
