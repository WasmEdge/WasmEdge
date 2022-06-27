// use std::{
//     env,
//     path::{Path, PathBuf},
//     process::Command,
// };
// use walkdir::WalkDir;
use std::path::PathBuf;

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
    let Paths {
        header,
        lib_dir,
        inc_dir,
    } = find_wasmedge().expect(
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
    } else {
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
            } else {
                println!(
                    "cargo:warning=[wasmedge-sys] WASMEDGE_BUILD_DIR: {:?} does not exist",
                    build_dir
                );
            }
        } else {
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
                } else {
                    println!(
                        "cargo:warning=[wasmedge-sys] xdg path: {:?} does not exist",
                        xdg_dir
                    );
                }
            } else {
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
                    } else {
                        println!(
                            "cargo:warning=[wasmedge-sys] docker env path: {:?} does not exist",
                            docker_dir
                        );
                    }
                }
            }
        }
    }

    println!("cargo:warning=[wasmedge-sys] Failed to locate lib_dir, include_dir, or header.",);

    None
}
