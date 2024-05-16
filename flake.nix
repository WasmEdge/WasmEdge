{
  inputs = {
    nixpkgs.url = "nixpkgs/nixpkgs-unstable";
    rust-overlay.url = "github:oxalica/rust-overlay";
    flake-utils.url = "github:numtide/flake-utils";
  };

  outputs = { self, rust-overlay, nixpkgs, flake-utils }:
    flake-utils.lib.eachDefaultSystem (system:
      let
        overlays = [ (import rust-overlay) ];
        pkgs = import nixpkgs { inherit system overlays; };
        rust = pkgs.rust-bin.stable.latest.default.override {
          extensions = [ "rust-src" ];
          targets = [ "wasm32-wasi" "wasm32-unknown-unknown" ];
        };
        llvmPackages = pkgs.llvmPackages_16;

        runRustSysTest = pkgs.writeShellScriptBin "run-rust-sys-test" ''
          cd bindings/rust/
          export WASMEDGE_DIR="$(pwd)/../../"
          export WASMEDGE_BUILD_DIR="$(pwd)/../../build"
          export LD_LIBRARY_PATH="$(pwd)/../../build/lib/api"
          cargo test -p wasmedge-sys --examples -- --nocapture
        '';
        runRustSysExample = pkgs.writeShellScriptBin "run-rust-sys-example" ''
          cd bindings/rust/
          export WASMEDGE_DIR="$(pwd)/../../"
          export WASMEDGE_BUILD_DIR="$(pwd)/../../build"
          export LD_LIBRARY_PATH="$(pwd)/../../build/lib/api"
          cargo run -p wasmedge-sys --example $1
        '';

        wasmedge = pkgs.stdenv.mkDerivation {
          name = "wasmedge";
          version = "0.12.1";
          src = ./.;

          buildInputs = with pkgs; [
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
          configurePhase = ''
            cmake -Bbuild \
              -DCMAKE_BUILD_TYPE=Debug \
              -DWASMEDGE_BUILD_PLUGINS=OFF \
              -DWASMEDGE_BUILD_TESTS=OFF \
              -DWASMEDGE_USE_LLVM=ON \
              .
          '';
          buildPhase = ''
            cmake --build build -j
          '';
          installPhase = ''
            cd build
            cmake --install . --prefix $out
          '';
        };
      in with pkgs; rec {
        packages = { wasmedge = wasmedge; };
        packages.default = packages.wasmedge;
        devShells.default = mkShell {
          buildInputs = [
            wasmedge

            ninja
            rust
            gcovr

            runRustSysTest
            runRustSysExample
          ];

          LIBCLANG_PATH = "${llvmPackages.libclang.lib}/lib";
        };
      });
}
