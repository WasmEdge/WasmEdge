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
    // #[cfg(not(feature = "standalone"))]
    let paths = find_wasmedge().expect(
        "[wasmedge-sys] (build script) failed to locate the required header and/or library file.",
    );

    // #[cfg(feature = "standalone")]
    // let paths = build_wasmedge();

    // if let Some(paths) = paths {
    let Paths {
        header,
        lib_dir,
        inc_dir,
    } = paths;

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
    // }
}

/// Check header and Returns the location of wasmedge.h and libwasmedge_c.(dylib|so)
#[cfg(not(feature = "standalone"))]
fn find_wasmedge() -> Option<Paths> {
    // ! debug
    let curr_dir = std::env::current_dir().unwrap();
    println!(
        "cargo:warning=[wasmedge-sys] current dir: {}",
        curr_dir.display()
    );
    let env_var = std::env::var("WASMEDGE_BUILD_DIR");
    match env_var {
        Ok(path) => {
            println!("cargo:warning=[wasmedge-sys] WASMEDGE_BUILD_DIR: {}", path);
        }
        Err(_) => {
            println!("cargo:warning=[wasmedge-sys] Not found WASMEDGE_BUILD_DIR");
        }
    }
    let env_plugin = std::env::var("WASMEDGE_PLUGIN_PATH");
    match env_plugin {
        Ok(path) => {
            println!(
                "cargo:warning=[wasmedge-sys] WASMEDGE_PLUGIN_PATH: {}",
                path
            );
        }
        Err(_) => {
            println!("cargo:warning=[wasmedge-sys] Not found WASMEDGE_PLUGIN_PATH");
        }
    }

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

#[cfg(feature = "standalone")]
fn build_wasmedge() -> Option<Paths> {
    let target_os = env::var("CARGO_CFG_TARGET_OS").unwrap();
    println!("cargo:warning=[wasmedge-sys] TARGET_OS: {}", target_os);

    let output = Command::new("git")
        .args(["rev-parse", "HEAD"])
        .output()
        .unwrap();

    let git_hash = String::from_utf8(output.stdout)
        .expect("[wasmedge-sys] fail to get the git hash for current build");
    let git_hash = git_hash.trim().trim_end_matches('\n');

    let out_dir = env_path!("OUT_DIR").expect("[wasmedge-sys] fail to get the OUT_DIR");
    let wasmedge_dir = out_dir.join("wasmedge");
    if !wasmedge_dir.exists() {
        std::fs::create_dir(&wasmedge_dir).unwrap_or_else(|_| {
            panic!(
                "[wasmedge-sys] fail to create wasmedge_dir: {:?}",
                &wasmedge_dir
            )
        });
    }
    let wasmedge_dir_str = wasmedge_dir
        .to_str()
        .expect("[wasmedge-sys] fail to convert PathBuf to str");

    let output = Command::new("git")
        .args(&["init", wasmedge_dir_str])
        .output()
        .expect("[wasmedge-sys] fail to init wasmedge project");
    println!("cargo:warning=[wasmedge-sys] git init output: {:?}", output);

    let output = Command::new("git")
        .current_dir(&wasmedge_dir)
        .args(&[
            "remote",
            "add",
            "origin",
            "https://github.com/WasmEdge/WasmEdge.git",
        ])
        .output()
        .expect("[wasmedge-sys] fail to add wasmedge upstream");
    println!(
        "cargo:warning=[wasmedge-sys] git remote add output: {:?}",
        output
    );

    let output = Command::new("git")
        .current_dir(&wasmedge_dir)
        .args(["fetch", "origin", git_hash])
        .output()
        .expect("[wasmedge-sys] fail to fetch a commit using its hash");
    println!(
        "cargo:warning=[wasmedge-sys] git fetch output: {:?}",
        output
    );

    let output = Command::new("git")
        .current_dir(&wasmedge_dir)
        .args(["checkout", "FETCH_HEAD"])
        .output()
        .expect("[wasmedge-sys] fail to reset repository to the commit");
    println!(
        "cargo:warning=[wasmedge-sys] git checkout output: {:?}",
        output
    );

    match target_os.as_str() {
        "linux" => Some(build_linux(&wasmedge_dir)),
        "macos" => Some(build_macos(&wasmedge_dir)),
        "windows" => Some(build_windows(&wasmedge_dir)),
        _ => None,
    }
}

#[cfg(feature = "standalone")]
fn build_macos(wasmedge_dir: impl AsRef<Path>) -> Paths {
    let status = Command::new("cmake")
        .current_dir(wasmedge_dir.as_ref())
        .args([
            "-Bbuild",
            "-GNinja",
            "-DCMAKE_BUILD_TYPE=Release",
            "-DWASMEDGE_BUILD_TESTS=ON",
            #[cfg(not(feature = "aot"))]
            "-DWASMEDGE_BUILD_AOT_RUNTIME=OFF",
            ".",
        ])
        .status()
        .expect("fail to cmake setup wasmedge project");
    println!("cargo:warning=[wasmedge-sys] cmake status: {:?}", status);

    let status = Command::new("cmake")
        .current_dir(wasmedge_dir.as_ref())
        .args(["--build", "build"])
        .status()
        .expect("[wasmedge-sys] fail to cmake build wasmedge project");
    println!(
        "cargo:warning=[wasmedge-sys] cmake build status: {:?}",
        status
    );

    // create build_dir
    let build_dir = wasmedge_dir.as_ref().join("build");
    assert!(build_dir.exists());

    // WASMEDGE_INCLUDE_DIR
    let mut inc_dir = build_dir.join("include");
    assert!(inc_dir.exists());
    if inc_dir.join("api").exists() {
        inc_dir = inc_dir.join("api");
    }
    assert!(inc_dir.join("wasmedge").join("wasmedge.h").exists());
    println!(
        "cargo:warning=[wasmedge-sys] WASMEDGE_INCLUDE_DIR: {}",
        inc_dir.to_str().unwrap()
    );

    // WASMEDGE_LIB_DIR
    let mut lib_dir = if build_dir.join("lib64").exists() {
        build_dir.join("lib64")
    } else {
        build_dir.join("lib")
    };
    if lib_dir.join("api").exists() {
        lib_dir = lib_dir.join("api");
    }
    assert!(lib_dir.join("libwasmedge_c.dylib").exists());
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

    // Path to plugins
    let plugin_dir = build_dir.join("plugins");
    assert!(plugin_dir.exists());
    let wasmedge_process_plugin_dir = plugin_dir.join("wasmedge_process");
    assert!(wasmedge_process_plugin_dir.exists());
    assert!(wasmedge_process_plugin_dir
        .join("libwasmedgePluginWasmEdgeProcess.dylib")
        .exists());
    println!(
        "cargo:rustc-env=WASMEDGE_PLUGIN_PATH={}",
        wasmedge_process_plugin_dir.to_str().unwrap()
    );
    println!(
        "cargo:warning=[wasmedge-sys] WASMEDGE_PLUGIN_PATH: {}",
        wasmedge_process_plugin_dir.to_str().unwrap()
    );

    Paths {
        inc_dir,
        header,
        lib_dir,
    }
}

#[cfg(feature = "standalone")]
fn build_linux(wasmedge_dir: impl AsRef<Path>) -> Paths {
    let out_dir = env_path!("OUT_DIR").expect("[wasmedge-sys] fail to get the OUT_DIR.");

    #[cfg(feature = "aot")]
    let dst = cmake::Config::new(&wasmedge_dir)
        .profile("Release")
        .define("WASMEDGE_BUILD_TESTS", "ON")
        .very_verbose(true)
        .build();
    #[cfg(not(feature = "aot"))]
    let dst = cmake::Config::new(&wasmedge_dir)
        .profile("Release")
        .define("WASMEDGE_BUILD_TESTS", "ON")
        .define("WASMEDGE_BUILD_AOT_RUNTIME", "OFF")
        .very_verbose(true)
        .build();
    println!("cargo:warning=[wasmedge-sys] cmake build dir: {:?}", dst);

    // WASMEDGE_INCLUDE_DIR
    let inc_dir = out_dir.join("include");
    assert!(inc_dir.exists());
    assert!(inc_dir.join("wasmedge").exists());
    println!(
        "cargo:warning=[wasmedge-sys] WASMEDGE_INCLUDE_DIR: {}",
        inc_dir.to_str().unwrap()
    );

    // WASMEDGE_LIB_DIR
    let lib_dir = if out_dir.join("lib64").exists() {
        out_dir.join("lib64")
    } else {
        out_dir.join("lib")
    };
    assert!(lib_dir.join("libwasmedge_c.so").exists());
    println!(
        "cargo:warning=[wasmedge-sys] WASMEDGE_LIB_DIR: {}",
        lib_dir.to_str().unwrap()
    );

    // Path to wasmedge.h
    let header = inc_dir.join("wasmedge").join(WASMEDGE_H);
    assert!(header.exists());
    println!(
        "cargo:warning=[wasmedge-sys] header path: {}",
        header.to_str().unwrap()
    );

    // Path to plugins
    let plugin_dir = lib_dir.join("wasmedge");
    assert!(plugin_dir.exists());
    assert!(plugin_dir
        .join("libwasmedgePluginWasmEdgeProcess.so")
        .exists());
    std::env::set_var("WASMEDGE_PLUGIN_PATH", plugin_dir.as_os_str());
    assert!(env_path!("WASMEDGE_PLUGIN_PATH").is_some());
    println!(
        "cargo:warning=[wasmedge-sys] WASMEDGE_PLUGIN_PATH: {}",
        plugin_dir.to_str().unwrap()
    );

    Paths {
        inc_dir,
        header,
        lib_dir,
    }
}

#[cfg(feature = "standalone")]
fn build_windows(wasmedge_dir: impl AsRef<Path>) -> Paths {
    // D:\a\WasmEdge-1\WasmEdge-1\bindings\rust\target\debug\build\wasmedge-sys-b6cafb6f182352aa\out\wasmedge
    println!(
        "cargo:warning=[wasmedge-sys] wasmedge_dir: {}",
        wasmedge_dir.as_ref().display()
    );

    // D:\a\WasmEdge-1\WasmEdge-1\bindings\rust\wasmedge-sys
    let curr_dir = env::current_dir().unwrap();
    println!(
        "cargo:warning=[wasmedge-sys] current dir: {}",
        curr_dir.display()
    );
    let root_dir = curr_dir
        .parent()
        .unwrap()
        .parent()
        .unwrap()
        .parent()
        .unwrap();
    println!(
        "cargo:warning=[wasmedge-sys] root_dir: {}",
        root_dir.display()
    );
    // list_dir(&root_dir, "root_dir");

    // set the path to the llvm dir
    let llvm_dir = root_dir
        .join("LLVM-13.0.1-win64")
        .join("LLVM-13.0.1-win64")
        .join("lib")
        .join("cmake")
        .join("llvm");
    assert!(llvm_dir.exists());
    println!(
        "cargo:warning=[wasmedge-sys] llvm_dir: {}",
        llvm_dir.display()
    );
    let llvm_dir_s = format!("-DLLVM_DIR={}", llvm_dir.to_str().unwrap());
    println!("cargo:warning=[wasmedge-sys] -DLLVM_DIR: {}", &llvm_dir_s);

    // // set environment variables: CC and CXX
    // env::set_var("CC", "clang-cl");
    // env::set_var("CXX", "clang-cl");

    let output = Command::new("cmake")
        .current_dir(wasmedge_dir.as_ref())
        .args([
            "-Bbuild",
            "-GNinja",
            "-DCMAKE_BUILD_TYPE=Release",
            #[cfg(not(feature = "aot"))]
            "-DWASMEDGE_BUILD_AOT_RUNTIME=OFF",
            r#"-DCMAKE_SYSTEM_VERSION="10.0.19041.0""#,
            "-DCMAKE_MSVC_RUNTIME_LIBRARY=MultiThreadedDLL",
            llvm_dir_s.as_str(),
            "-DWASMEDGE_BUILD_TESTS=ON",
            r#"-DWASMEDGE_BUILD_PACKAGE="ZIP""#,
            ".",
        ])
        .output()
        .expect("[wasmedge-sys] fail to cmake setup wasmedge project");
    println!(
        "cargo:warning=[wasmedge-sys] cmake -Bbuild -GNinja ...: {:?}",
        output,
    );

    let output = Command::new("cmake")
        .current_dir(wasmedge_dir.as_ref())
        .args(["--build", "build"])
        .output()
        .expect("[wasmedge-sys] fail to cmake build wasmedge project");
    println!(
        "cargo:warning=[wasmedge-sys] cmake --build build: {:?}",
        output
    );

    // create build_dir
    let build_dir = wasmedge_dir.as_ref().join("build");

    // WASMEDGE_INCLUDE_DIR
    let mut inc_dir = build_dir.join("include");
    assert!(inc_dir.exists());
    if inc_dir.join("api").exists() {
        inc_dir = inc_dir.join("api");
    }
    println!(
        "cargo:warning=[wasmedge-sys] WASMEDGE_INCLUDE_DIR: {}",
        inc_dir.to_str().unwrap()
    );

    // WASMEDGE_LIB_DIR
    let mut lib_dir = if build_dir.join("lib64").exists() {
        build_dir.join("lib64")
    } else {
        build_dir.join("lib")
    };
    if lib_dir.join("api").exists() {
        lib_dir = lib_dir.join("api");
    }
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

    // Path to plugins
    let plugin_dir = build_dir.join("wasmedge");
    assert!(plugin_dir.exists());
    std::env::set_var("WASMEDGE_PLUGIN_PATH", plugin_dir.as_os_str());
    assert!(env_path!("WASMEDGE_PLUGIN_PATH").is_some());
    println!(
        "cargo:warning=[wasmedge-sys] WASMEDGE_PLUGIN_PATH: {}",
        plugin_dir.to_str().unwrap()
    );

    Paths {
        inc_dir,
        header,
        lib_dir,
    }
}

// fn list_dir(dir: &Path, tag: &str) {
//     if dir.is_dir() {
//         for entry in std::fs::read_dir(dir).expect("[wasmedge-sys] Fail to read dir.") {
//             let entry = entry.expect("[wasmedge-sys] Invalid dir entry.");
//             let path = entry.path();
//             if path.is_dir() {
//                 println!(
//                     "cargo:warning=[wasmedge-sys] sub-dirs name of {}: {}",
//                     tag,
//                     path.to_str().unwrap()
//                 );
//             } else {
//                 println!(
//                     "cargo:warning=[wasmedge-sys] Found file in {}: {}",
//                     tag,
//                     path.to_str().unwrap()
//                 );
//             }
//         }
//     }
// }

// fn print_dir(dir: &Path) {
//     for entry in WalkDir::new(dir) {
//         println!("{}", entry.expect("invalid entry").path().display());
//     }
// }
