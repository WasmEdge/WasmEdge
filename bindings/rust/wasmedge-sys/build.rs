use std::path::PathBuf;
#[cfg(all(
    feature = "standalone",
    target_family = "unix",
    not(feature = "static")
))]
use std::{env, process::Command};

const WASMEDGE_H: &str = "wasmedge.h";
#[cfg(all(
    feature = "standalone",
    target_family = "unix",
    not(feature = "static")
))]
const WASMEDGE_RELEASE_VERSION: &str = "0.12.0";

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

fn main() -> Result<(), Box<dyn std::error::Error>> {
    find_wasmedge()
}

#[cfg(not(feature = "static"))]
fn find_wasmedge() -> Result<(), Box<dyn std::error::Error>> {
    #[cfg(all(feature = "standalone", target_family = "unix"))]
    install_libwasmedge();

    let Paths {
        header,
        lib_dir,
        inc_dir,
    } = find_libwasmedge().expect(
        "[wasmedge-sys] Failed to locate the required header and/or library file. Please reference the link: https://wasmedge.org/book/en/embed/rust.html",
    );

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
    println!("cargo:rustc-link-lib=dylib=wasmedge");

    Ok(())
}

#[cfg(all(feature = "static", target_os = "linux"))]
fn find_wasmedge() -> Result<(), Box<dyn std::error::Error>> {
    let Paths {
        header,
        lib_dir,
        inc_dir,
    } = find_libwasmedge().expect(
        "[wasmedge-sys] Failed to locate the required header and/or library file. Please reference the link: https://wasmedge.org/book/en/embed/rust.html",
    );

    let libdir_path_str = lib_dir
        .to_str()
        .unwrap_or_else(|| panic!("`{}` must be a utf-8 path", lib_dir.display()));
    println!(
        "cargo:warning=[wasmedge-sys] libdir_path_str: {:?}",
        libdir_path_str
    );

    let incdir_path_str = inc_dir
        .to_str()
        .unwrap_or_else(|| panic!("`{}` must be a utf-8 path", inc_dir.display()));
    println!(
        "cargo:warning=[wasmedge-sys] incdir_path_str: {:?}",
        incdir_path_str
    );

    let header_path_str = header
        .to_str()
        .unwrap_or_else(|| panic!("`{}` must be a utf-8 path", header.display()));
    println!(
        "cargo:warning=[wasmedge-sys] header_path_str: {:?}",
        header_path_str
    );

    // Tell cargo to look for shared libraries in the specified directory
    println!("cargo:rustc-link-search=native={}", libdir_path_str);
    println!("cargo:rustc-link-search=/lib/x86_64-linux-gnu/");

    // Tell cargo to tell rustc to link our `wasmedge` library. Cargo will
    // automatically know it must look for a `libwasmedge.a` file.
    println!("cargo:rustc-link-lib=static=wasmedge");
    println!("cargo:rustc-link-lib=rt");
    println!("cargo:rustc-link-lib=dl");
    println!("cargo:rustc-link-lib=pthread");
    println!("cargo:rustc-link-lib=m");
    println!("cargo:rustc-link-lib=stdc++");

    // Tell cargo to invalidate the built crate whenever the header changes.
    println!("cargo:rerun-if-changed={}", header_path_str);

    let out_file = PathBuf::from(std::env::var("OUT_DIR").unwrap()).join("wasmedge.rs");
    bindgen::builder()
        .header(header_path_str)
        .clang_arg(format!("-I{}", incdir_path_str))
        .clang_arg(format!("-L{}", libdir_path_str))
        .prepend_enum_name(false) // The API already prepends the name.
        .parse_callbacks(Box::new(bindgen::CargoCallbacks))
        .generate()
        .expect("failed to generate bindings")
        .write_to_file(out_file)
        .expect("failed to write bindings");

    Ok(())
}

/// Check header and Returns the location of wasmedge.h and libwasmedge.a
#[cfg(all(feature = "static", target_os = "linux"))]
fn find_libwasmedge() -> Option<Paths> {
    // search in the env variables: WASMEDGE_INCLUDE_DIR, WASMEDGE_LIB_DIR
    let inc_dir = env_path!("WASMEDGE_INCLUDE_DIR");
    let lib_dir = env_path!("WASMEDGE_LIB_DIR");
    if let Some(inc_dir) = inc_dir {
        if let Some(lib_dir) = lib_dir {
            let header = inc_dir.join("wasmedge");
            let header = header.join(WASMEDGE_H);
            if inc_dir.exists() && lib_dir.exists() && header.exists() {
                println!(
                    "cargo:warning=[wasmedge-sys] libwasmedge found in {}",
                    lib_dir.to_str().unwrap()
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

        if build_dir.exists() && inc_dir.exists() && lib_dir.exists() && header.exists() {
            println!(
                "cargo:warning=[wasmedge-sys] libwasmedge found in {}",
                lib_dir.to_str().unwrap()
            );
            return Some(Paths {
                header,
                lib_dir,
                inc_dir,
            });
        }
    }

    // search in /usr/local/
    let inc_dir = PathBuf::from("/usr/local/include");
    let lib_dir = if PathBuf::from("/usr/local/lib64").exists() {
        PathBuf::from("/usr/local/lib64")
    } else {
        PathBuf::from("/usr/local/lib")
    };
    let header = inc_dir.join("wasmedge").join(WASMEDGE_H);
    if inc_dir.join("wasmedge").exists() && lib_dir.exists() && header.exists() {
        println!(
            "cargo:warning=[wasmedge-sys] libwasmedge found in {}",
            lib_dir.to_str().unwrap()
        );
        return Some(Paths {
            header,
            inc_dir,
            lib_dir,
        });
    }

    // search in the official docker container
    let default_dir = env_path!("HOME").map(|d| d.join(".wasmedge"));
    if let Some(default_dir) = default_dir {
        // WASMEDGE_INCLUDE_DIR
        let inc_dir = default_dir.join("include");
        // header
        let header = inc_dir.join("wasmedge");
        let header = header.join(WASMEDGE_H);

        // WASMEDGE_LIB_DIR
        let lib_dir = if default_dir.join("lib64").exists() {
            default_dir.join("lib64")
        } else {
            default_dir.join("lib")
        };

        if default_dir.exists() && inc_dir.exists() && lib_dir.exists() && header.exists() {
            println!(
                "cargo:warning=[wasmedge-sys] libwasmedge found in {}",
                lib_dir.to_str().unwrap(),
            );
            return Some(Paths {
                header,
                lib_dir,
                inc_dir,
            });
        }
    }

    // search in xdg
    let xdg_dir = env_path!("HOME").map(|d| d.join(".local"));
    if let Some(xdg_dir) = xdg_dir {
        let inc_dir = xdg_dir.join("include");
        let header = inc_dir.join(WASMEDGE_H);
        let lib_dir = if xdg_dir.join("lib64").exists() {
            xdg_dir.join("lib64")
        } else {
            xdg_dir.join("lib")
        };

        if xdg_dir.exists() && inc_dir.exists() && lib_dir.exists() && header.exists() {
            println!(
                "cargo:warning=[wasmedge-sys] libwasmedge found in {}",
                lib_dir.to_str().unwrap()
            );
            return Some(Paths {
                header,
                lib_dir,
                inc_dir,
            });
        }
    }

    println!("cargo:warning=[wasmedge-sys] Failed to locate lib_dir, include_dir, or header.",);

    None
}

/// Check header and Returns the location of wasmedge.h and libwasmedge_c.(dylib|so)
#[cfg(not(feature = "static"))]
fn find_libwasmedge() -> Option<Paths> {
    // search in the env variables: WASMEDGE_INCLUDE_DIR, WASMEDGE_LIB_DIR
    let inc_dir = env_path!("WASMEDGE_INCLUDE_DIR");
    let lib_dir = env_path!("WASMEDGE_LIB_DIR");
    if let Some(inc_dir) = inc_dir {
        if let Some(lib_dir) = lib_dir {
            let header = inc_dir.join("wasmedge");
            let header = header.join(WASMEDGE_H);
            if inc_dir.exists() && lib_dir.exists() && header.exists() {
                println!(
                    "cargo:warning=[wasmedge-sys] libwasmedge found in {}",
                    lib_dir.to_str().unwrap()
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

        if build_dir.exists() && inc_dir.exists() && lib_dir.exists() && header.exists() {
            println!(
                "cargo:warning=[wasmedge-sys] libwasmedge found in {}",
                lib_dir.to_str().unwrap()
            );
            return Some(Paths {
                header,
                lib_dir,
                inc_dir,
            });
        }
    }

    // search in /usr/local/
    let inc_dir = PathBuf::from("/usr/local/include");
    let lib_dir = if PathBuf::from("/usr/local/lib64").exists() {
        PathBuf::from("/usr/local/lib64")
    } else {
        PathBuf::from("/usr/local/lib")
    };
    let header = inc_dir.join("wasmedge").join(WASMEDGE_H);
    if inc_dir.join("wasmedge").exists() && lib_dir.exists() && header.exists() {
        println!(
            "cargo:warning=[wasmedge-sys] libwasmedge found in {}",
            lib_dir.to_str().unwrap()
        );
        return Some(Paths {
            header,
            inc_dir,
            lib_dir,
        });
    }

    // search in the official docker container
    let default_dir = env_path!("HOME").map(|d| d.join(".wasmedge"));
    if let Some(default_dir) = default_dir {
        // WASMEDGE_INCLUDE_DIR
        let inc_dir = default_dir.join("include");
        // header
        let header = inc_dir.join("wasmedge");
        let header = header.join(WASMEDGE_H);

        // WASMEDGE_LIB_DIR
        let lib_dir = if default_dir.join("lib64").exists() {
            default_dir.join("lib64")
        } else {
            default_dir.join("lib")
        };

        if default_dir.exists() && inc_dir.exists() && lib_dir.exists() && header.exists() {
            println!(
                "cargo:warning=[wasmedge-sys] libwasmedge found in {}",
                lib_dir.to_str().unwrap(),
            );
            return Some(Paths {
                header,
                lib_dir,
                inc_dir,
            });
        }
    }

    // search in xdg
    let xdg_dir = env_path!("HOME").map(|d| d.join(".local"));
    if let Some(xdg_dir) = xdg_dir {
        let inc_dir = xdg_dir.join("include");
        let header = inc_dir.join(WASMEDGE_H);
        let lib_dir = if xdg_dir.join("lib64").exists() {
            xdg_dir.join("lib64")
        } else {
            xdg_dir.join("lib")
        };

        if xdg_dir.exists() && inc_dir.exists() && lib_dir.exists() && header.exists() {
            println!(
                "cargo:warning=[wasmedge-sys] libwasmedge found in {}",
                lib_dir.to_str().unwrap()
            );
            return Some(Paths {
                header,
                lib_dir,
                inc_dir,
            });
        }
    }

    println!("cargo:warning=[wasmedge-sys] Failed to locate lib_dir, include_dir, or header.",);

    None
}

#[cfg(all(
    feature = "standalone",
    not(feature = "static"),
    target_family = "unix"
))]
fn install_libwasmedge() {
    let out_dir = env::var("OUT_DIR").expect("[wasmedge-sys] Failed to get OUT_DIR");
    println!("cargo:warning=[wasmedge-sys] OUT_DIR: {}", &out_dir);

    let output = Command::new("wget")
        .current_dir(&out_dir)
        .arg("https://raw.githubusercontent.com/WasmEdge/WasmEdge/master/utils/install.sh")
        .output()
        .expect("[wasmedge-sys] Failed to download libwasmedge installation script");
    println!(
        "cargo:warning=[wasmedge-sys] Download libwasmedge installation script: {:?}",
        output
    );

    let output = Command::new("bash")
        .current_dir(&out_dir)
        .args(["install.sh", "-v", WASMEDGE_RELEASE_VERSION])
        .output()
        .expect("[wasmedge-sys] Failed to run libwasmedge installation script");
    println!(
        "cargo:warning=[wasmedge-sys] Run libwasmedge installation script: {:?}",
        output
    );

    let output = Command::new("/bin/bash")
        .arg("-c")
        .arg("source $HOME/.wasmedge/env")
        .output()
        .expect("[wasmedge-sys] Failed to source the env");
    println!("cargo:warning=[wasmedge-sys] source the env: {:?}", output);
}
