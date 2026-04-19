{
  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-24.05";
    flake-utils.url = "github:numtide/flake-utils";
    # tree-sitter runtime, fetched by the evaluator so the sealed build sandbox
    # gets it via FETCHCONTENT_SOURCE_DIR instead of cloning at configure time.
    # Keep the rev in sync with lib/wat/CMakeLists.txt.
    tree-sitter-src = {
      url = "github:tree-sitter/tree-sitter/v0.26.6";
      flake = false;
    };
  };

  outputs = { self, nixpkgs, flake-utils, tree-sitter-src }:
    flake-utils.lib.eachDefaultSystem (system:
      let
        pkgs = import nixpkgs { inherit system; };
        llvmPackages = pkgs.llvmPackages_18;

        wasmedge_buildInputs = with pkgs; [
          cmake
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
            "-DFETCHCONTENT_SOURCE_DIR_TREESITTER=${tree-sitter-src}"
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
