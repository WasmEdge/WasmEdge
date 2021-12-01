use std::path::PathBuf;

const WASMEDGE_H: &str = "wasmedge.h";

fn main() {
    let Paths {
        header,
        lib_dir,
        inc_dir,
    } = find_wasmedge().expect("wasmedge header not found");

    let out_file = PathBuf::from(std::env::var("OUT_DIR").unwrap()).join("wasmedge.rs");
    bindgen::builder()
        .header(
            header
                .to_str()
                .unwrap_or_else(|| panic!("`{}` must be a utf-8 path", header.display())),
        )
        .clang_arg(format!("-I{}", inc_dir.as_path().display()))
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

/// Check header and Returns the location of wasmedge.h and libwasmedge_c.(dylib|so)
/// The priorities are:
/// 1. The locations specified by `WASMEDGE_INCLUDE_DIR` and `WASMEDGE_LIB_DIR`.
/// 2. The build directory, if this is an in-tree build.
/// 3. The "XDG" local installation dirs: `~/.local/include` and `~/.local/lib`.
/// 4. The global installation dirs: `/usr/include` and `/usr/bin`.
/// 5. Backward compatiable the path berfore 0.9
fn find_wasmedge() -> Option<Paths> {
    macro_rules! env_path {
        ($env_var:literal) => {
            std::env::var_os($env_var).map(PathBuf::from)
        };
    }

    let mut inc_dir = env_path!("WASMEDGE_INCLUDE_DIR");
    let mut lib_dir = env_path!("WASMEDGE_LIB_DIR");

    let build_dir = env_path!("WASMEDGE_BUILD_DIR");
    if let Some((build_dir, found_inc_dir)) = contains_wasmedge_h(build_dir, "include/api/wasmedge")
    {
        inc_dir = inc_dir.or(Some(found_inc_dir));
        lib_dir = lib_dir.or_else(|| Some(build_dir.join("lib/api")));
    }

    let xdg_dir = env_path!("HOME").map(|d| d.join(".local"));
    if let Some((xdg_dir, found_inc_dir)) = contains_wasmedge_h(xdg_dir, "include") {
        inc_dir = inc_dir.or(Some(found_inc_dir));
        lib_dir = lib_dir.or_else(|| Some(xdg_dir.join("lib")));
    }

    let header = inc_dir
        .clone()
        .unwrap_or_else(|| PathBuf::from("/usr/include/wasmedge"))
        .join(WASMEDGE_H);

    if header.exists() {
        Some(Paths {
            header,
            lib_dir: lib_dir.unwrap_or_else(|| PathBuf::from("/usr/lib")),
            inc_dir: inc_dir
                .unwrap_or_else(|| PathBuf::from("/usr/include/wasmedge"))
                .parent()
                .unwrap()
                .to_path_buf(),
        })
    } else if PathBuf::from("/usr/include").join(WASMEDGE_H).exists() {
        // check the header path of old version
        Some(Paths {
            header: PathBuf::from("/usr/include").join(WASMEDGE_H),
            lib_dir: PathBuf::from("/usr/lib"),
            inc_dir: PathBuf::from("/usr/include"),
        })
    } else {
        None
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
    inc_dir: PathBuf,
}
