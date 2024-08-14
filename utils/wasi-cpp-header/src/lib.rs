// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

mod cpp_header;

use anyhow::{anyhow, Result};
pub use cpp_header::to_cpp_header;
use std::path::Path;
use witx::{load, WitxError};

pub fn generate<P: AsRef<Path>>(inputs: &[P]) -> Result<String> {
    // TODO: drop the anyhow! part once witx switches to anyhow.
    let doc = load(&inputs).map_err(|e| match e {
        WitxError::Parse(err) => anyhow!(err),
        WitxError::Validation(err) => anyhow!(err),
        e => anyhow!(e.to_string()),
    })?;

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
