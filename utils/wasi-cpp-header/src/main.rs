// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#[macro_use]
extern crate clap;

use anyhow::Result;
use clap::{Arg, SubCommand};
use std::fs::File;
use std::io::Write;
use std::path::Path;
use wasi_cpp_header::generate;

pub fn run<P: AsRef<Path>, Q: AsRef<Path>>(inputs: &[P], output: Q) -> Result<()> {
    let cpp_header = generate(inputs)?;
    let mut file = File::create(output)?;
    file.write_all(cpp_header.as_bytes())?;
    Ok(())
}

fn main() -> Result<()> {
    let matches = app_from_crate!()
        .setting(clap::AppSettings::SubcommandRequiredElseHelp)
        .subcommand(
            SubCommand::with_name("generate")
                .arg(Arg::with_name("inputs").required(true).multiple(true))
                .arg(
                    Arg::with_name("output")
                        .short("o")
                        .long("output")
                        .takes_value(true)
                        .required(true),
                ),
        )
        .subcommand(
            SubCommand::with_name("generate-api").about("generate api.hpp from current snapshot"),
        )
        .get_matches();

    if let Some(generate) = matches.subcommand_matches("generate") {
        let inputs = generate
            .values_of("inputs")
            .expect("required inputs arg")
            .collect::<Vec<_>>();
        let output = generate.value_of("output").expect("required output arg");
        run(&inputs, &output)?;
    } else {
        unreachable!("a subcommand must be provided");
    }

    Ok(())
}
