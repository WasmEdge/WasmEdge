// SPDX-License-Identifier: Apache-2.0
mod cpp_header;

use anyhow::{anyhow, Result};
pub use cpp_header::to_cpp_header;
use std::fs;
use std::io;
use std::path::{Path, PathBuf};
use witx::load;

pub fn generate<P: AsRef<Path>>(inputs: &[P]) -> Result<String> {
    // TODO: drop the anyhow! part once witx switches to anyhow.
    let doc = load(&inputs).map_err(|e| anyhow!(e.to_string()))?;

    let inputs_str = &inputs
        .iter()
        .map(|p| {
            p.as_ref()
                .file_name()
                .unwrap()
                .to_str()
                .unwrap()
                .to_string()
        })
        .collect::<Vec<_>>()
        .join(", ");

    Ok(to_cpp_header(&doc, &inputs_str))
}

pub fn snapshot_witx_files() -> Result<Vec<PathBuf>> {
    let snapshot_dir = PathBuf::from(env!("CARGO_MANIFEST_DIR")).join("WASI/phases/snapshot/witx");
    let mut inputs = fs::read_dir(snapshot_dir)?
        .map(|res| res.map(|e| e.path()))
        .collect::<Result<Vec<_>, io::Error>>()?;

    inputs.sort();
    Ok(inputs)
}

pub fn wasi_api_header_path() -> PathBuf {
    PathBuf::from(env!("CARGO_MANIFEST_DIR"))
        .join("../../libc-bottom-half/headers/public/wasi/api.h")
}
