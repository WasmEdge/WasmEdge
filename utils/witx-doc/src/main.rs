// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

use std::{
    ffi::OsStr,
    fs,
    io::{self, Write},
    path::PathBuf,
    process,
};
use witx::{self, Documentation};

fn main() -> io::Result<()> {
    let arg1 = std::env::args().nth(1).expect("args 1 = witx root-dir");

    let dirname = PathBuf::from(arg1);
    let sources = fs::read_dir(dirname.as_path())?
        .filter_map(|res| {
            res.map_or(None, |i| {
                let p = i.path();
                match p.extension().and_then(OsStr::to_str) {
                    Some("witx") => Some(p),
                    _ => {
                        println!("skipped {:?}", p);
                        None
                    }
                }
            })
        })
        .collect::<Vec<_>>();
    println!("sources: {:?}", sources);

    match witx::load(&sources) {
        Ok(doc) => {
            let docfile = dirname.join("docs.md");
            println!("docfile: {:?}", docfile);
            let mut file = fs::File::create(docfile).expect("create output file");
            file.write_all(doc.to_md().as_bytes())
                .expect("write output file");
        }
        Err(e) => {
            eprintln!("{}", e.report());
            process::exit(7);
        }
    };
    Ok(())
}
