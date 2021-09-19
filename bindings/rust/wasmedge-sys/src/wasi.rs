
use core::default::Default;
use std::os::raw::c_char;

#[derive(Debug, Default)]
pub struct ImportObjInitParams {
    pub(crate) args: Vec<*const c_char>,
    pub(crate) args_len: u32,
    pub(crate) envs: Vec<*const c_char>,
    pub(crate) envs_len: u32,
    pub(crate) dirs: Vec<*const c_char>,
    pub(crate) dirs_len: u32,
    pub(crate) preopens: Vec<*const c_char>,
    pub(crate) preopens_len: u32,
}

impl<'a> ImportObjInitParams {
    pub(crate) fn new() -> Self{
        Self::default()
    }
}
