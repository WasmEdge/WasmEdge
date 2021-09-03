use std::path::PathBuf;

const WASMEDGE_H: &str = "wasmedge.h";

fn main() {
    let Paths { header, lib_dir } = find_wasmedge();

    let out_file = PathBuf::from(std::env::var("OUT_DIR").unwrap()).join("wasmedge.rs");
    bindgen::builder()
        .header(
            header
                .to_str()
                .unwrap_or_else(|| panic!("`{}` must be a utf-8 path", header.display())),
        )
        .prepend_enum_name(false) // The API already prepends the name.
        .dynamic_link_require_all(true)
        .parse_callbacks(Box::new(bindgen::CargoCallbacks))
        .generate()
        .expect("failed to generate bindings")
        .write_to_file(out_file)
        .expect("failed to write bindings");

    println!("cargo:rustc-env=LD_LIBRARY_PATH={}", lib_dir.display());
    println!("cargo:rustc-link-search={}", lib_dir.display());
    println!("cargo:rustc-link-lib=dylib=wasmedge_c");
}

/// Returns the best guess of the location of wasmedge.h and libwasmedge_c.(dylib|so)
/// The priorities are:
/// 1. The locations specified by `WASMEDGE_INCLUDE_DIR` and `WASMEDGE_LIB_DIR`.
/// 2. `include/` and `lib/` subdirectories of `WASMEDGE_DIR`.
/// 3. The build directory, if this is an in-tree build.
/// 4. The "XDG" local installation dirs: `~/.local/include` and `~/.local/lib`.
/// 5. The global installation dirs: `/usr/include` and `/usr/bin`.
fn find_wasmedge() -> Paths {
    macro_rules! env_path {
        ($env_var:literal) => {
            std::env::var_os($env_var).map(PathBuf::from)
        };
    }

    let mut inc_dir = env_path!("WASMEDGE_INCLUDE_DIR");
    let mut lib_dir = env_path!("WASMEDGE_LIB_DIR");

    if let Some(prefix) = env_path!("WASMEDGE_DIR") {
        inc_dir = inc_dir.or_else(|| Some(prefix.join("include")));
        lib_dir = lib_dir.or_else(|| Some(prefix.join("lib")));
    }

    let build_dir = env_path!("WASMEDGE_SRC_DIR").map(|d| d.join("build"));
    if let Some((build_dir, found_inc_dir)) = contains_wasmedge_h(build_dir, "include/api") {
        inc_dir = inc_dir.or(Some(found_inc_dir));
        lib_dir = lib_dir.or_else(|| Some(build_dir.join("lib/api")));
    }

    let xdg_dir = env_path!("HOME").map(|d| d.join(".local"));
    if let Some((xdg_dir, found_inc_dir)) = contains_wasmedge_h(xdg_dir, "include") {
        inc_dir = inc_dir.or(Some(found_inc_dir));
        lib_dir = lib_dir.or_else(|| Some(xdg_dir.join("lib")));
    }

    Paths {
        header: inc_dir
            .unwrap_or_else(|| PathBuf::from("/usr/include"))
            .join(WASMEDGE_H),
        lib_dir: lib_dir.unwrap_or_else(|| PathBuf::from("/usr/lib")),
    }
}

/// If the WasmEdge header file is found under `base_dir/inc_subdir`, returns
/// `Some((base_dir, base_dir/inc_subdir))`.
fn contains_wasmedge_h(base_dir: Option<PathBuf>, inc_subdir: &str) -> Option<(PathBuf, PathBuf)> {
    base_dir.and_then(|base_dir| {
        let inc_dir = base_dir.join(inc_subdir);
        inc_dir
            .join(WASMEDGE_H)
            .is_file()
            .then(|| (base_dir, inc_dir))
    })
}

#[derive(Debug)]
struct Paths {
    header: PathBuf,
    lib_dir: PathBuf,
}
