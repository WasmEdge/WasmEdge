use std::{env, path::PathBuf, process::Command};

const WASMEDGE_H: &str = "wasmedge.h";

macro_rules! env_path {
    ($env_var:literal) => {
        std::env::var_os($env_var).map(PathBuf::from)
    };
}

#[derive(Debug)]
struct Paths {
    header: PathBuf,
    lib_dir: PathBuf,
    inc_dir: PathBuf,
}

fn main() {
    #[cfg(not(feature = "standalone"))]
    let Paths {
        header,
        lib_dir,
        inc_dir,
    } = find_wasmedge()
        .or_else(build_wasmedge)
        .expect("should be dependency paths");

    #[cfg(feature = "standalone")]
    let Paths {
        header,
        lib_dir,
        inc_dir,
    } = build_wasmedge().expect("should be dependency paths");

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
fn find_wasmedge() -> Option<Paths> {
    // search in the env variables: WASMEDGE_INCLUDE_DIR, WASMEDGE_LIB_DIR
    let inc_dir = env_path!("WASMEDGE_INCLUDE_DIR");
    let lib_dir = env_path!("WASMEDGE_LIB_DIR");
    if let Some(inc_dir) = inc_dir {
        if let Some(lib_dir) = lib_dir {
            let header = inc_dir.join("wasmedge");
            let header = header.join(WASMEDGE_H);
            if inc_dir.exists() && lib_dir.exists() && header.exists() {
                println!(
                    "cargo:warning=[wasmedge-sys] Use WASMEDGE_INCLUDE_DIR: {:?}",
                    inc_dir
                );
                println!(
                    "cargo:warning=[wasmedge-sys] Use WASMEDGE_LIB_DIR: {:?}",
                    lib_dir
                );
                return Some(Paths {
                    header,
                    lib_dir,
                    inc_dir,
                });
            }
        }
    }

    // search in the env variable: WASMEDGE_BUILD_DIR
    let build_dir = env_path!("WASMEDGE_BUILD_DIR");
    if let Some(build_dir) = build_dir {
        if build_dir.exists() {
            // WASMEDGE_INCLUDE_DIR
            let inc_dir = build_dir.join("include");
            let inc_dir = inc_dir.join("api");
            // header
            let header = inc_dir.join("wasmedge");
            let header = header.join(WASMEDGE_H);
            // WASMEDGE_LIB_DIR
            let lib_dir = if build_dir.join("lib64").exists() {
                build_dir.join("lib64")
            } else {
                build_dir.join("lib")
            };
            let lib_dir = lib_dir.join("api");

            if inc_dir.exists() && lib_dir.exists() && header.exists() {
                println!(
                    "cargo:warning=[wasmedge-sys] Use WASMEDGE_BUILD_DIR: {:?}",
                    build_dir
                );
                return Some(Paths {
                    header,
                    lib_dir,
                    inc_dir,
                });
            }
        }
    }

    // search in xdg
    let xdg_dir = env_path!("HOME").map(|d| d.join(".local"));
    if let Some(xdg_dir) = xdg_dir {
        if xdg_dir.exists() {
            let inc_dir = xdg_dir.join("include");
            let header = inc_dir.join(WASMEDGE_H);
            let lib_dir = if xdg_dir.join("lib64").exists() {
                xdg_dir.join("lib64")
            } else {
                xdg_dir.join("lib")
            };
            if inc_dir.exists() && lib_dir.exists() && header.exists() {
                println!("cargo:warning=[wasmedge-sys] Use xdg path: {:?}", xdg_dir);
                return Some(Paths {
                    header,
                    lib_dir,
                    inc_dir,
                });
            }
        }
    }

    // search in the official docker container
    let docker_dir = env_path!("HOME").map(|d| d.join(".wasmedge"));
    if let Some(docker_dir) = docker_dir {
        if docker_dir.exists() {
            // WASMEDGE_INCLUDE_DIR
            let inc_dir = docker_dir.join("include");
            // header
            let header = inc_dir.join("wasmedge");
            let header = header.join(WASMEDGE_H);
            // WASMEDGE_LIB_DIR
            let lib_dir = if docker_dir.join("lib64").exists() {
                docker_dir.join("lib64")
            } else {
                docker_dir.join("lib")
            };

            if inc_dir.exists() && lib_dir.exists() && header.exists() {
                println!(
                    "cargo:warning=[wasmedge-sys] Use docker env path: {:?}",
                    docker_dir
                );
                return Some(Paths {
                    header,
                    lib_dir,
                    inc_dir,
                });
            }
        }
    }

    None
}

fn build_wasmedge() -> Option<Paths> {
    let target_os = env::var("CARGO_CFG_TARGET_OS").unwrap();
    println!("cargo:warning=[wasmedge-sys] TARGET_OS: {}", target_os);

    match target_os.as_str() {
        "linux" => Some(build_linux()),
        "macos" => {
            let output = Command::new("git")
                .args(&["rev-parse", "HEAD"])
                .output()
                .unwrap();
            let git_hash = String::from_utf8(output.stdout)
                .expect("fail to get the git hash for current build");
            Some(build_macos(git_hash))
        }
        _ => None,
    }
}

fn build_macos(hash: String) -> Paths {
    let out_dir = env::var("OUT_DIR").expect("fail to get the out dir");

    let wasmedge_src = format!("{}/wasmedge", &out_dir);
    Command::new("git")
        .args(&["init", &wasmedge_src])
        .output()
        .expect("fail to init wasmedge project");

    Command::new("git")
        .current_dir(&wasmedge_src)
        .args(&[
            "remote",
            "add",
            "origin",
            "https://github.com/WasmEdge/WasmEdge.git",
        ])
        .output()
        .expect("fail to add wasmedge upstream");

    Command::new("git")
        .current_dir(&wasmedge_src)
        .args(&["fetch", "--tags", "origin", hash.trim()])
        .output()
        .expect("fail to fetch the commit");

    Command::new("git")
        .current_dir(&wasmedge_src)
        .args(&["checkout", "FETCH_HEAD"])
        .output()
        .expect("fail to checkout the commit");

    Command::new("cmake")
        .current_dir(&wasmedge_src)
        .args(&[
            "-Bbuild",
            "-GNinja",
            "-DCMAKE_BUILD_TYPE=Release",
            "-DWASMEDGE_BUILD_TESTS=ON",
            #[cfg(not(feature = "aot"))]
            "-DWASMEDGE_BUILD_AOT_RUNTIME=OFF",
            &format!("-DCMAKE_INSTALL_PREFIX={}", &out_dir),
            ".",
        ])
        .output()
        .expect("fail to cmake setup wasmedge project");

    Command::new("cmake")
        .current_dir(&wasmedge_src)
        .args(&["--build", "build"])
        .output()
        .expect("fail to cmake build wasmedge project");

    Command::new("ninja")
        .current_dir(&format!("{}/build", &wasmedge_src))
        .args(&["install"])
        .output()
        .expect("fail to ninja build wasmedge project");

    let lib_dir = if std::path::Path::new(&format!("{}/lib64", &out_dir)).exists() {
        format!("{}/lib64", &out_dir)
    } else {
        format!("{}/lib", &out_dir)
    };

    Paths {
        header: format!("{}/include/wasmedge/wasmedge.h", &out_dir).into(),
        lib_dir: lib_dir.into(),
        inc_dir: format!("{}/include", &out_dir).into(),
    }
}

fn build_linux() -> Paths {
    #[cfg(feature = "standalone")]
    println!("cargo:warning=[wasmedge-sys] standalone");

    #[cfg(not(feature = "standalone"))]
    println!("cargo:warning=[wasmedge-sys] not_standalone");

    let out_dir = env_path!("OUT_DIR").expect("fail to find the OUT_DIR env variable");

    let wasmedge_dir =
        env_path!("WASMEDGE_DIR").expect("fail to find the WASMEDGE_DIR env variable");

    // create build_dir
    #[cfg(not(feature = "standalone"))]
    let build_dir = out_dir.join("build");
    #[cfg(feature = "standalone")]
    let build_dir = wasmedge_dir.join("build");
    if !build_dir.exists() {
        std::fs::create_dir(&build_dir).expect("fail to create build_dir");
    }
    let build_dir_str = build_dir.to_str().unwrap();

    Command::new("cmake")
        .current_dir(&build_dir_str)
        .args([
            "-DCMAKE_BUILD_TYPE=Release",
            "-DWASMEDGE_BUILD_TESTS=ON",
            #[cfg(not(feature = "aot"))]
            "-DWASMEDGE_BUILD_AOT_RUNTIME=OFF",
            wasmedge_dir.to_str().unwrap(),
        ])
        .output()
        .expect("fail to cmake setup wasmedge project");

    Command::new("make")
        .current_dir(&build_dir_str)
        .arg("-j")
        .output()
        .expect("fail to compile wasmedge project");

    // WASMEDGE_INCLUDE_DIR
    let inc_dir = build_dir.join("include");
    assert!(inc_dir.exists());
    let inc_dir = inc_dir.join("api");
    assert!(inc_dir.exists());
    println!(
        "cargo:warning=[wasmedge-sys] WASMEDGE_INCLUDE_DIR: {}",
        inc_dir.to_str().unwrap()
    );

    // WASMEDGE_LIB_DIR
    let lib_dir = if build_dir.join("lib64").exists() {
        build_dir.join("lib64")
    } else {
        build_dir.join("lib")
    };
    let lib_dir = lib_dir.join("api");
    assert!(lib_dir.exists());
    println!(
        "cargo:warning=[wasmedge-sys] WASMEDGE_LIB_DIR: {}",
        lib_dir.to_str().unwrap()
    );

    // Path to wasmedge.h
    let header = inc_dir.join("wasmedge");
    assert!(header.exists());
    let header = header.join(WASMEDGE_H);
    assert!(header.exists());
    println!(
        "cargo:warning=[wasmedge-sys] header path: {}",
        header.to_str().unwrap()
    );

    Paths {
        inc_dir,
        header,
        lib_dir,
    }
}
