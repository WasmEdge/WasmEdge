use functions::{AsyncHostFn, HostFn};
use wasi::snapshots::{
    common::{
        error::Errno,
        memory::WasmPtr,
        types::{__wasi_ciovec_t, __wasi_size_t},
    },
    preview_1 as p, WasiCtx,
};
use wasmedge_async_wasi as wasi;

// use wasmedge_sdk::{Vm, WasmEdgeResult};
use crate::{AsImport, CallingFrame, FuncType, Memory, WasmValue};
use wasmedge_types::{error::HostFuncError, ValType, WasmEdgeResult};

// use crate::values::WasmVal;

use crate::instance::function::functions;
// mod values;

fn to_wasm_return(r: Result<(), Errno>) -> Vec<WasmValue> {
    let code = if let Err(e) = r { e.0 } else { 0 };
    vec![WasmValue::from_i32(code as i32)]
}

impl wasmedge_async_wasi::snapshots::common::memory::Memory for Memory {
    fn get_data<'a, T: Sized>(&'a self, offset: WasmPtr<T>) -> Result<&'a T, Errno> {
        unsafe {
            let r = std::mem::size_of::<T>();
            let ptr = self
                .data_pointer(offset.0 as u32, r as u32)
                .map_err(|_| Errno::__WASI_ERRNO_FAULT)? as *const u8;
            Ok(ptr.cast::<T>().as_ref().unwrap())
        }
    }

    fn get_slice<'a, T: Sized>(&'a self, offset: WasmPtr<T>, len: usize) -> Result<&'a [T], Errno> {
        unsafe {
            let r = std::mem::size_of::<T>() * len;
            let ptr = self
                .data_pointer(offset.0 as u32, r as u32)
                .map_err(|_| Errno::__WASI_ERRNO_FAULT)? as *const u8
                as *const T;
            Ok(std::slice::from_raw_parts(ptr, len))
        }
    }

    fn get_iovec<'a>(
        &'a self,
        iovec_ptr: WasmPtr<__wasi_ciovec_t>,
        iovec_len: __wasi_size_t,
    ) -> Result<Vec<std::io::IoSlice<'a>>, Errno> {
        unsafe {
            let iovec = self.get_slice(iovec_ptr, iovec_len as usize)?.to_vec();
            let mut result = Vec::with_capacity(iovec.len());
            for i in iovec {
                let ptr = self
                    .data_pointer(i.buf, i.buf_len)
                    .map_err(|_| Errno::__WASI_ERRNO_FAULT)? as *const u8;
                let s = std::io::IoSlice::new(std::slice::from_raw_parts(ptr, i.buf_len as usize));
                result.push(s);
            }
            Ok(result)
        }
    }

    fn mut_data<'a, T: Sized>(&'a mut self, offset: WasmPtr<T>) -> Result<&'a mut T, Errno> {
        unsafe {
            let r = std::mem::size_of::<T>();
            let ptr = self
                .data_pointer_mut(offset.0 as u32, r as u32)
                .map_err(|_| Errno::__WASI_ERRNO_FAULT)? as *mut u8;
            Ok(ptr.cast::<T>().as_mut().unwrap())
        }
    }

    fn mut_slice<'a, T: Sized>(
        &'a mut self,
        offset: WasmPtr<T>,
        len: usize,
    ) -> Result<&'a mut [T], Errno> {
        unsafe {
            let r = std::mem::size_of::<T>() * len;
            let ptr = self
                .data_pointer_mut(offset.0 as u32, r as u32)
                .map_err(|_| Errno::__WASI_ERRNO_FAULT)? as *mut u8 as *mut T;
            Ok(std::slice::from_raw_parts_mut(ptr, len))
        }
    }

    fn mut_iovec<'a>(
        &'a mut self,
        iovec_ptr: WasmPtr<wasmedge_async_wasi::snapshots::env::wasi_types::__wasi_iovec_t>,
        iovec_len: wasmedge_async_wasi::snapshots::env::wasi_types::__wasi_size_t,
    ) -> Result<Vec<std::io::IoSliceMut<'a>>, Errno> {
        unsafe {
            let iovec = self.get_slice(iovec_ptr, iovec_len as usize)?.to_vec();
            let mut result = Vec::with_capacity(iovec.len());
            for i in iovec {
                let ptr = self
                    .data_pointer_mut(i.buf, i.buf_len)
                    .map_err(|_| Errno::__WASI_ERRNO_FAULT)? as *mut u8;
                let s = std::io::IoSliceMut::new(std::slice::from_raw_parts_mut(
                    ptr,
                    i.buf_len as usize,
                ));
                result.push(s);
            }
            Ok(result)
        }
    }

    fn write_data<'a, T: Sized>(&'a mut self, offset: WasmPtr<T>, data: T) -> Result<(), Errno> {
        let p = self.mut_data(offset)?;
        *p = data;
        Ok(())
    }
}

pub fn args_get(
    frame: &mut CallingFrame,
    wasi_ctx: &mut WasiCtx,
    args: Vec<WasmValue>,
) -> std::result::Result<Vec<WasmValue>, HostFuncError> {
    let mut mem = frame.memory_mut(0).ok_or(HostFuncError::Runtime(0x88))?;

    if let Some([argv, argv_buf]) = args.get(0..2) {
        let argv = argv.to_i32() as usize;
        let argv_buf = argv_buf.to_i32() as usize;
        Ok(to_wasm_return(p::args_get(
            wasi_ctx,
            &mut mem,
            WasmPtr::from(argv),
            WasmPtr::from(argv_buf),
        )))
    } else {
        Err(HostFuncError::Runtime(0x83))
    }
}

pub fn args_sizes_get(
    frame: &mut CallingFrame,
    wasi_ctx: &mut WasiCtx,
    args: Vec<WasmValue>,
) -> std::result::Result<Vec<WasmValue>, HostFuncError> {
    let mut mem = frame.memory_mut(0).ok_or(HostFuncError::Runtime(0x88))?;

    if let Some([argc, argv_buf_size]) = args.get(0..2) {
        let argc = argc.to_i32() as usize;
        let argv_buf_size = argv_buf_size.to_i32() as usize;
        Ok(to_wasm_return(p::args_sizes_get(
            wasi_ctx,
            &mut mem,
            WasmPtr::from(argc),
            WasmPtr::from(argv_buf_size),
        )))
    } else {
        Err(HostFuncError::Runtime(0x83))
    }
}

pub fn environ_get(
    frame: &mut CallingFrame,
    wasi_ctx: &mut WasiCtx,
    args: Vec<WasmValue>,
) -> std::result::Result<Vec<WasmValue>, HostFuncError> {
    let mut mem = frame.memory_mut(0).ok_or(HostFuncError::Runtime(0x88))?;

    if let Some([p1, p2]) = args.get(0..2) {
        let environ = p1.to_i32() as usize;
        let environ_buf = p2.to_i32() as usize;
        Ok(to_wasm_return(p::environ_get(
            wasi_ctx,
            &mut mem,
            WasmPtr::from(environ),
            WasmPtr::from(environ_buf),
        )))
    } else {
        Err(HostFuncError::Runtime(0x83))
    }
}

pub fn environ_sizes_get(
    frame: &mut CallingFrame,
    wasi_ctx: &mut WasiCtx,
    args: Vec<WasmValue>,
) -> std::result::Result<Vec<WasmValue>, HostFuncError> {
    let mut mem = frame.memory_mut(0).ok_or(HostFuncError::Runtime(0x88))?;

    if let Some([p1, p2]) = args.get(0..2) {
        let environ_count = p1.to_i32() as usize;
        let environ_buf_size = p2.to_i32() as usize;
        Ok(to_wasm_return(p::environ_sizes_get(
            wasi_ctx,
            &mut mem,
            WasmPtr::from(environ_count),
            WasmPtr::from(environ_buf_size),
        )))
    } else {
        Err(HostFuncError::Runtime(0x83))
    }
}

pub fn clock_res_get(
    frame: &mut CallingFrame,
    wasi_ctx: &mut WasiCtx,
    args: Vec<WasmValue>,
) -> std::result::Result<Vec<WasmValue>, HostFuncError> {
    let mut mem = frame.memory_mut(0).ok_or(HostFuncError::Runtime(0x88))?;

    if let Some([p1, p2]) = args.get(0..2) {
        let clock_id = p1.to_i32() as u32;
        let resolution_ptr = p2.to_i32() as usize;
        Ok(to_wasm_return(p::clock_res_get(
            wasi_ctx,
            &mut mem,
            clock_id,
            WasmPtr::from(resolution_ptr),
        )))
    } else {
        Err(HostFuncError::Runtime(0x83))
    }
}

pub fn clock_time_get(
    frame: &mut CallingFrame,
    wasi_ctx: &mut WasiCtx,
    args: Vec<WasmValue>,
) -> std::result::Result<Vec<WasmValue>, HostFuncError> {
    let mut mem = frame.memory_mut(0).ok_or(HostFuncError::Runtime(0x88))?;

    if let Some([p1, p2, p3]) = args.get(0..3) {
        let clock_id = p1.to_i32() as u32;
        let precision = p2.to_i64() as u64;
        let time_ptr = p3.to_i32() as usize;

        Ok(to_wasm_return(p::clock_time_get(
            wasi_ctx,
            &mut mem,
            clock_id,
            precision,
            WasmPtr::from(time_ptr),
        )))
    } else {
        Err(HostFuncError::Runtime(0x83))
    }
}

pub fn random_get(
    frame: &mut CallingFrame,
    wasi_ctx: &mut WasiCtx,
    args: Vec<WasmValue>,
) -> std::result::Result<Vec<WasmValue>, HostFuncError> {
    let mut mem = frame.memory_mut(0).ok_or(HostFuncError::Runtime(0x88))?;

    if let Some([p1, p2]) = args.get(0..2) {
        let buf = p1.to_i32() as usize;
        let buf_len = p2.to_i32() as u32;

        Ok(to_wasm_return(p::random_get(
            wasi_ctx,
            &mut mem,
            WasmPtr::from(buf),
            buf_len,
        )))
    } else {
        Err(HostFuncError::Runtime(0x83))
    }
}

pub fn fd_prestat_get(
    frame: &mut CallingFrame,
    wasi_ctx: &mut WasiCtx,
    args: Vec<WasmValue>,
) -> std::result::Result<Vec<WasmValue>, HostFuncError> {
    let mut mem = frame.memory_mut(0).ok_or(HostFuncError::Runtime(0x88))?;

    // if let Some([WasmVal::I32(p1), WasmVal::I32(p2)]) = args.get(0..2) {
    if let Some([p1, p2]) = args.get(0..2) {
        let fd = p1.to_i32();
        let prestat_ptr = p2.to_i32() as usize;

        Ok(to_wasm_return(p::fd_prestat_get(
            wasi_ctx,
            &mut mem,
            fd,
            WasmPtr::from(prestat_ptr),
        )))
    } else {
        Err(HostFuncError::Runtime(0x83))
    }
}

pub fn fd_prestat_dir_name(
    frame: &mut CallingFrame,
    wasi_ctx: &mut WasiCtx,
    args: Vec<WasmValue>,
) -> std::result::Result<Vec<WasmValue>, HostFuncError> {
    let mut mem = frame.memory_mut(0).ok_or(HostFuncError::Runtime(0x88))?;

    if let Some([p1, p2, p3]) = args.get(0..3) {
        let fd = p1.to_i32();
        let path_buf_ptr = p2.to_i32() as usize;
        let path_max_len = p3.to_i32() as u32;

        Ok(to_wasm_return(p::fd_prestat_dir_name(
            wasi_ctx,
            &mut mem,
            fd,
            WasmPtr::from(path_buf_ptr),
            path_max_len,
        )))
    } else {
        Err(HostFuncError::Runtime(0x83))
    }
}

pub fn fd_renumber(
    frame: &mut CallingFrame,
    wasi_ctx: &mut WasiCtx,
    args: Vec<WasmValue>,
) -> std::result::Result<Vec<WasmValue>, HostFuncError> {
    let mut mem = frame.memory_mut(0).ok_or(HostFuncError::Runtime(0x88))?;

    if let Some([p1, p2]) = args.get(0..2) {
        let from = p1.to_i32();
        let to = p2.to_i32();

        Ok(to_wasm_return(p::fd_renumber(wasi_ctx, &mut mem, from, to)))
    } else {
        Err(HostFuncError::Runtime(0x83))
    }
}

pub fn fd_advise(
    frame: &mut CallingFrame,
    wasi_ctx: &mut WasiCtx,
    args: Vec<WasmValue>,
) -> std::result::Result<Vec<WasmValue>, HostFuncError> {
    let mut mem = frame.memory_mut(0).ok_or(HostFuncError::Runtime(0x88))?;

    if let Some([p1, p2, p3, p4]) = args.get(0..4) {
        let fd = p1.to_i32();
        let offset = p2.to_i64() as u64;
        let len = p3.to_i64() as u64;
        let advice = p4.to_i32() as u8;

        Ok(to_wasm_return(p::fd_advise(
            wasi_ctx, &mut mem, fd, offset, len, advice,
        )))
    } else {
        Err(HostFuncError::Runtime(0x83))
    }
}

pub fn fd_allocate(
    frame: &mut CallingFrame,
    wasi_ctx: &mut WasiCtx,
    args: Vec<WasmValue>,
) -> std::result::Result<Vec<WasmValue>, HostFuncError> {
    let mut mem = frame.memory_mut(0).ok_or(HostFuncError::Runtime(0x88))?;

    if let Some([p1, p2, p3]) = args.get(0..3) {
        let fd = p1.to_i32();
        let offset = p2.to_i64() as u64;
        let len = p3.to_i64() as u64;

        Ok(to_wasm_return(p::fd_allocate(
            wasi_ctx, &mut mem, fd, offset, len,
        )))
    } else {
        Err(HostFuncError::Runtime(0x83))
    }
}

pub fn fd_close(
    frame: &mut CallingFrame,
    wasi_ctx: &mut WasiCtx,
    args: Vec<WasmValue>,
) -> std::result::Result<Vec<WasmValue>, HostFuncError> {
    let mut mem = frame.memory_mut(0).ok_or(HostFuncError::Runtime(0x88))?;

    if let Some([p1]) = args.get(0..1) {
        let fd = p1.to_i32();

        Ok(to_wasm_return(p::fd_close(wasi_ctx, &mut mem, fd)))
    } else {
        Err(HostFuncError::Runtime(0x83))
    }
}

pub fn fd_seek(
    frame: &mut CallingFrame,
    wasi_ctx: &mut WasiCtx,
    args: Vec<WasmValue>,
) -> std::result::Result<Vec<WasmValue>, HostFuncError> {
    let mut mem = frame.memory_mut(0).ok_or(HostFuncError::Runtime(0x88))?;

    if let Some([p1, p2, p3, p4]) = args.get(0..4) {
        let fd = p1.to_i32();
        let offset = p2.to_i64();
        let whence = p3.to_i32() as u8;
        let newoffset_ptr = p4.to_i32() as usize;

        Ok(to_wasm_return(p::fd_seek(
            wasi_ctx,
            &mut mem,
            fd,
            offset,
            whence,
            WasmPtr::from(newoffset_ptr),
        )))
    } else {
        Err(HostFuncError::Runtime(0x83))
    }
}

pub fn fd_sync(
    frame: &mut CallingFrame,
    wasi_ctx: &mut WasiCtx,
    args: Vec<WasmValue>,
) -> std::result::Result<Vec<WasmValue>, HostFuncError> {
    let mut mem = frame.memory_mut(0).ok_or(HostFuncError::Runtime(0x88))?;

    if let Some([p1]) = args.get(0..1) {
        let fd = p1.to_i32();

        Ok(to_wasm_return(p::fd_sync(wasi_ctx, &mut mem, fd)))
    } else {
        Err(HostFuncError::Runtime(0x83))
    }
}

pub fn fd_datasync(
    frame: &mut CallingFrame,
    wasi_ctx: &mut WasiCtx,
    args: Vec<WasmValue>,
) -> std::result::Result<Vec<WasmValue>, HostFuncError> {
    let mut mem = frame.memory_mut(0).ok_or(HostFuncError::Runtime(0x88))?;

    if let Some([p1]) = args.get(0..1) {
        let fd = p1.to_i32();

        Ok(to_wasm_return(p::fd_datasync(wasi_ctx, &mut mem, fd)))
    } else {
        Err(HostFuncError::Runtime(0x83))
    }
}

pub fn fd_tell(
    frame: &mut CallingFrame,
    wasi_ctx: &mut WasiCtx,
    args: Vec<WasmValue>,
) -> std::result::Result<Vec<WasmValue>, HostFuncError> {
    let mut mem = frame.memory_mut(0).ok_or(HostFuncError::Runtime(0x88))?;

    // if let Some([WasmVal::I32(p1), WasmVal::I32(p2)]) = args.get(0..2) {
    if let Some([p1, p2]) = args.get(0..2) {
        let fd = p1.to_i32();
        let offset = p2.to_i32() as usize;

        Ok(to_wasm_return(p::fd_tell(
            wasi_ctx,
            &mut mem,
            fd,
            WasmPtr::from(offset),
        )))
    } else {
        Err(HostFuncError::Runtime(0x83))
    }
}

pub fn fd_fdstat_get(
    frame: &mut CallingFrame,
    wasi_ctx: &mut WasiCtx,
    args: Vec<WasmValue>,
) -> std::result::Result<Vec<WasmValue>, HostFuncError> {
    let mut mem = frame.memory_mut(0).ok_or(HostFuncError::Runtime(0x88))?;

    if let Some([p1, p2]) = args.get(0..2) {
        let fd = p1.to_i32();
        let buf_ptr = p2.to_i32() as usize;

        Ok(to_wasm_return(p::fd_fdstat_get(
            wasi_ctx,
            &mut mem,
            fd,
            WasmPtr::from(buf_ptr),
        )))
    } else {
        Err(HostFuncError::Runtime(0x83))
    }
}

pub fn fd_fdstat_set_flags(
    frame: &mut CallingFrame,
    wasi_ctx: &mut WasiCtx,
    args: Vec<WasmValue>,
) -> std::result::Result<Vec<WasmValue>, HostFuncError> {
    let mut mem = frame.memory_mut(0).ok_or(HostFuncError::Runtime(0x88))?;

    if let Some([p1, p2]) = args.get(0..2) {
        let fd = p1.to_i32();
        let flags = p2.to_i32() as u16;

        Ok(to_wasm_return(p::fd_fdstat_set_flags(
            wasi_ctx, &mut mem, fd, flags,
        )))
    } else {
        Err(HostFuncError::Runtime(0x83))
    }
}

pub fn fd_fdstat_set_rights(
    frame: &mut CallingFrame,
    wasi_ctx: &mut WasiCtx,
    args: Vec<WasmValue>,
) -> std::result::Result<Vec<WasmValue>, HostFuncError> {
    let mut mem = frame.memory_mut(0).ok_or(HostFuncError::Runtime(0x88))?;

    if let Some([p1, p2, p3]) = args.get(0..3) {
        let fd = p1.to_i32();
        let fs_rights_base = p2.to_i64() as u64;
        let fs_rights_inheriting = p3.to_i64() as u64;

        Ok(to_wasm_return(p::fd_fdstat_set_rights(
            wasi_ctx,
            &mut mem,
            fd,
            fs_rights_base,
            fs_rights_inheriting,
        )))
    } else {
        Err(HostFuncError::Runtime(0x83))
    }
}

pub fn fd_filestat_get(
    frame: &mut CallingFrame,
    wasi_ctx: &mut WasiCtx,
    args: Vec<WasmValue>,
) -> std::result::Result<Vec<WasmValue>, HostFuncError> {
    let mut mem = frame.memory_mut(0).ok_or(HostFuncError::Runtime(0x88))?;

    if let Some([p1, p2]) = args.get(0..2) {
        let fd = p1.to_i32();
        let buf = p2.to_i32() as usize;

        Ok(to_wasm_return(p::fd_filestat_get(
            wasi_ctx,
            &mut mem,
            fd,
            WasmPtr::from(buf),
        )))
    } else {
        Err(HostFuncError::Runtime(0x83))
    }
}

pub fn fd_filestat_set_size(
    frame: &mut CallingFrame,
    wasi_ctx: &mut WasiCtx,
    args: Vec<WasmValue>,
) -> std::result::Result<Vec<WasmValue>, HostFuncError> {
    let mut mem = frame.memory_mut(0).ok_or(HostFuncError::Runtime(0x88))?;

    if let Some([p1, p2]) = args.get(0..2) {
        let fd = p1.to_i32();
        let buf = p2.to_i32() as usize;

        Ok(to_wasm_return(p::fd_filestat_get(
            wasi_ctx,
            &mut mem,
            fd,
            WasmPtr::from(buf),
        )))
    } else {
        Err(HostFuncError::Runtime(0x83))
    }
}

pub fn fd_filestat_set_times(
    frame: &mut CallingFrame,
    wasi_ctx: &mut WasiCtx,
    args: Vec<WasmValue>,
) -> std::result::Result<Vec<WasmValue>, HostFuncError> {
    let mut mem = frame.memory_mut(0).ok_or(HostFuncError::Runtime(0x88))?;

    if let Some([p1, p2, p3, p4]) = args.get(0..4) {
        let fd = p1.to_i32();
        let st_atim = p2.to_i64() as u64;
        let st_mtim = p3.to_i64() as u64;
        let fst_flags = p4.to_i32() as u16;

        Ok(to_wasm_return(p::fd_filestat_set_times(
            wasi_ctx, &mut mem, fd, st_atim, st_mtim, fst_flags,
        )))
    } else {
        Err(HostFuncError::Runtime(0x83))
    }
}

pub fn fd_read(
    frame: &mut CallingFrame,
    wasi_ctx: &mut WasiCtx,
    args: Vec<WasmValue>,
) -> std::result::Result<Vec<WasmValue>, HostFuncError> {
    let mut mem = frame.memory_mut(0).ok_or(HostFuncError::Runtime(0x88))?;

    if let Some([p1, p2, p3, p4]) = args.get(0..4) {
        let fd = p1.to_i32();
        let iovs = p2.to_i32() as usize;
        let iovs_len = p3.to_i32() as u32;
        let nread = p4.to_i32() as usize;

        Ok(to_wasm_return(p::fd_read(
            wasi_ctx,
            &mut mem,
            fd,
            WasmPtr::from(iovs),
            iovs_len,
            WasmPtr::from(nread),
        )))
    } else {
        Err(HostFuncError::Runtime(0x83))
    }
}

pub fn fd_pread(
    frame: &mut CallingFrame,
    wasi_ctx: &mut WasiCtx,
    args: Vec<WasmValue>,
) -> std::result::Result<Vec<WasmValue>, HostFuncError> {
    let mut mem = frame.memory_mut(0).ok_or(HostFuncError::Runtime(0x88))?;

    if let Some([p1, p2, p3, p4, p5]) = args.get(0..5) {
        let fd = p1.to_i32();
        let iovs = p2.to_i32() as usize;
        let iovs_len = p3.to_i32() as u32;
        let offset = p4.to_i64() as u64;
        let nread = p5.to_i32() as usize;

        Ok(to_wasm_return(p::fd_pread(
            wasi_ctx,
            &mut mem,
            fd,
            WasmPtr::from(iovs),
            iovs_len,
            offset,
            WasmPtr::from(nread),
        )))
    } else {
        Err(HostFuncError::Runtime(0x83))
    }
}

pub fn fd_write(
    frame: &mut CallingFrame,
    wasi_ctx: &mut WasiCtx,
    args: Vec<WasmValue>,
) -> std::result::Result<Vec<WasmValue>, HostFuncError> {
    let mut mem = frame.memory_mut(0).ok_or(HostFuncError::Runtime(0x88))?;

    if let Some([p1, p2, p3, p4]) = args.get(0..4) {
        let fd = p1.to_i32();
        let iovs = p2.to_i32() as usize;
        let iovs_len = p3.to_i32() as u32;
        let nwritten = p4.to_i32() as usize;

        Ok(to_wasm_return(p::fd_write(
            wasi_ctx,
            &mut mem,
            fd,
            WasmPtr::from(iovs),
            iovs_len,
            WasmPtr::from(nwritten),
        )))
    } else {
        Err(HostFuncError::Runtime(0x83))
    }
}

pub fn fd_pwrite(
    frame: &mut CallingFrame,
    wasi_ctx: &mut WasiCtx,
    args: Vec<WasmValue>,
) -> std::result::Result<Vec<WasmValue>, HostFuncError> {
    let mut mem = frame.memory_mut(0).ok_or(HostFuncError::Runtime(0x88))?;

    if let Some([p1, p2, p3, p4, p5]) = args.get(0..5) {
        let fd = p1.to_i32();
        let iovs = p2.to_i32() as usize;
        let iovs_len = p3.to_i32() as u32;
        let offset = p4.to_i64() as u64;
        let nwritten = p5.to_i32() as usize;

        Ok(to_wasm_return(p::fd_pwrite(
            wasi_ctx,
            &mut mem,
            fd,
            WasmPtr::from(iovs),
            iovs_len,
            offset,
            WasmPtr::from(nwritten),
        )))
    } else {
        Err(HostFuncError::Runtime(0x83))
    }
}

pub fn fd_readdir(
    frame: &mut CallingFrame,
    wasi_ctx: &mut WasiCtx,
    args: Vec<WasmValue>,
) -> std::result::Result<Vec<WasmValue>, HostFuncError> {
    let mut mem = frame.memory_mut(0).ok_or(HostFuncError::Runtime(0x88))?;

    if let Some([p1, p2, p3, p4, p5]) = args.get(0..5) {
        let fd = p1.to_i32();
        let buf = p2.to_i32() as usize;
        let buf_len = p3.to_i32() as u32;
        let cookie = p4.to_i64() as u64;
        let bufused_ptr = p5.to_i32() as usize;

        Ok(to_wasm_return(p::fd_readdir(
            wasi_ctx,
            &mut mem,
            fd,
            WasmPtr::from(buf),
            buf_len,
            cookie,
            WasmPtr::from(bufused_ptr),
        )))
    } else {
        Err(HostFuncError::Runtime(0x83))
    }
}

pub fn path_create_directory(
    frame: &mut CallingFrame,
    wasi_ctx: &mut WasiCtx,
    args: Vec<WasmValue>,
) -> std::result::Result<Vec<WasmValue>, HostFuncError> {
    let mut mem = frame.memory_mut(0).ok_or(HostFuncError::Runtime(0x88))?;

    if let Some([p1, p2, p3]) = args.get(0..3) {
        let dirfd = p1.to_i32();
        let path_ptr = p2.to_i32() as usize;
        let path_len = p3.to_i32() as u32;

        Ok(to_wasm_return(p::path_create_directory(
            wasi_ctx,
            &mut mem,
            dirfd,
            WasmPtr::from(path_ptr),
            path_len,
        )))
    } else {
        Err(HostFuncError::Runtime(0x83))
    }
}

pub fn path_filestat_get(
    frame: &mut CallingFrame,
    wasi_ctx: &mut WasiCtx,
    args: Vec<WasmValue>,
) -> std::result::Result<Vec<WasmValue>, HostFuncError> {
    let mut mem = frame.memory_mut(0).ok_or(HostFuncError::Runtime(0x88))?;

    if let Some([p1, p2, p3, p4, p5]) = args.get(0..5) {
        let fd = p1.to_i32();
        let flags = p2.to_i32() as u32;
        let path_ptr = p3.to_i32() as usize;
        let path_len = p4.to_i32() as u32;
        let file_stat_ptr = p5.to_i32() as usize;

        Ok(to_wasm_return(p::path_filestat_get(
            wasi_ctx,
            &mut mem,
            fd,
            flags,
            WasmPtr::from(path_ptr),
            path_len,
            WasmPtr::from(file_stat_ptr),
        )))
    } else {
        Err(HostFuncError::Runtime(0x83))
    }
}

pub fn path_filestat_set_times(
    _frame: &mut CallingFrame,
    _wasi_ctx: &mut WasiCtx,
    _args: Vec<WasmValue>,
) -> std::result::Result<Vec<WasmValue>, HostFuncError> {
    Ok(vec![WasmValue::from_i32(
        Errno::__WASI_ERRNO_NOSYS.0 as i32,
    )])
}

pub fn path_link(
    _frame: &mut CallingFrame,
    _wasi_ctx: &mut WasiCtx,
    _args: Vec<WasmValue>,
) -> std::result::Result<Vec<WasmValue>, HostFuncError> {
    Ok(vec![WasmValue::from_i32(
        Errno::__WASI_ERRNO_NOSYS.0 as i32,
    )])
}

pub fn path_open(
    frame: &mut CallingFrame,
    wasi_ctx: &mut WasiCtx,
    args: Vec<WasmValue>,
) -> std::result::Result<Vec<WasmValue>, HostFuncError> {
    let mut mem = frame.memory_mut(0).ok_or(HostFuncError::Runtime(0x88))?;

    if let Some([p1, p2, p3, p4, p5, p6, p7, p8, p9]) = args.get(0..9) {
        let dirfd = p1.to_i32();
        let dirflags = p2.to_i32() as u32;
        let path = p3.to_i32() as usize;
        let path_len = p4.to_i32() as u32;
        let o_flags = p5.to_i32() as u16;
        let fs_rights_base = p6.to_i64() as u64;
        let fs_rights_inheriting = p7.to_i64() as u64;
        let fs_flags = p8.to_i32() as u16;
        let fd_ptr = p9.to_i32() as usize;

        Ok(to_wasm_return(p::path_open(
            wasi_ctx,
            &mut mem,
            dirfd,
            dirflags,
            WasmPtr::from(path),
            path_len,
            o_flags,
            fs_rights_base,
            fs_rights_inheriting,
            fs_flags,
            WasmPtr::from(fd_ptr),
        )))
    } else {
        Err(HostFuncError::Runtime(0x83))
    }
}

pub fn path_readlink(
    _frame: &mut CallingFrame,
    _wasi_ctx: &mut WasiCtx,
    _args: Vec<WasmValue>,
) -> std::result::Result<Vec<WasmValue>, HostFuncError> {
    Ok(vec![WasmValue::from_i32(
        Errno::__WASI_ERRNO_NOSYS.0 as i32,
    )])
}

pub fn path_remove_directory(
    frame: &mut CallingFrame,
    wasi_ctx: &mut WasiCtx,
    args: Vec<WasmValue>,
) -> std::result::Result<Vec<WasmValue>, HostFuncError> {
    let mut mem = frame.memory_mut(0).ok_or(HostFuncError::Runtime(0x88))?;

    if let Some([p1, p2, p3]) = args.get(0..3) {
        let fd = p1.to_i32();
        let path_ptr = p2.to_i32() as usize;
        let path_len = p3.to_i32() as u32;

        Ok(to_wasm_return(p::path_remove_directory(
            wasi_ctx,
            &mut mem,
            fd,
            WasmPtr::from(path_ptr),
            path_len,
        )))
    } else {
        Err(HostFuncError::Runtime(0x83))
    }
}

pub fn path_rename(
    frame: &mut CallingFrame,
    wasi_ctx: &mut WasiCtx,
    args: Vec<WasmValue>,
) -> std::result::Result<Vec<WasmValue>, HostFuncError> {
    let mut mem = frame.memory_mut(0).ok_or(HostFuncError::Runtime(0x88))?;

    if let Some([p1, p2, p3, p4, p5, p6]) = args.get(0..6) {
        let old_fd = p1.to_i32();
        let old_path = p2.to_i32() as usize;
        let old_path_len = p3.to_i32() as u32;
        let new_fd = p4.to_i32();
        let new_path = p5.to_i32() as usize;
        let new_path_len = p6.to_i32() as u32;

        Ok(to_wasm_return(p::path_rename(
            wasi_ctx,
            &mut mem,
            old_fd,
            WasmPtr::from(old_path),
            old_path_len,
            new_fd,
            WasmPtr::from(new_path),
            new_path_len,
        )))
    } else {
        Err(HostFuncError::Runtime(0x83))
    }
}

pub fn path_symlink(
    _frame: &mut CallingFrame,
    _wasi_ctx: &mut WasiCtx,
    _args: Vec<WasmValue>,
) -> std::result::Result<Vec<WasmValue>, HostFuncError> {
    Ok(vec![WasmValue::from_i32(
        Errno::__WASI_ERRNO_NOSYS.0 as i32,
    )])
}

pub fn path_unlink_file(
    frame: &mut CallingFrame,
    wasi_ctx: &mut WasiCtx,
    args: Vec<WasmValue>,
) -> std::result::Result<Vec<WasmValue>, HostFuncError> {
    let mut mem = frame.memory_mut(0).ok_or(HostFuncError::Runtime(0x88))?;

    if let Some([p1, p2, p3]) = args.get(0..3) {
        let fd = p1.to_i32();
        let path_ptr = p2.to_i32() as usize;
        let path_len = p3.to_i32() as u32;

        Ok(to_wasm_return(p::path_unlink_file(
            wasi_ctx,
            &mut mem,
            fd,
            WasmPtr::from(path_ptr),
            path_len,
        )))
    } else {
        Err(HostFuncError::Runtime(0x83))
    }
}

pub fn proc_exit(
    frame: &mut CallingFrame,
    wasi_ctx: &mut WasiCtx,
    args: Vec<WasmValue>,
) -> std::result::Result<Vec<WasmValue>, HostFuncError> {
    let mut mem = frame.memory_mut(0).ok_or(HostFuncError::Runtime(0x88))?;

    if let Some([p1]) = args.get(0..1) {
        let code = p1.to_i32() as u32;
        p::proc_exit(wasi_ctx, &mut mem, code);
        Err(HostFuncError::Runtime(0x01))
    } else {
        Err(HostFuncError::Runtime(0x83))
    }
}

pub fn proc_raise(
    _frame: &mut CallingFrame,
    _wasi_ctx: &mut WasiCtx,
    _args: Vec<WasmValue>,
) -> std::result::Result<Vec<WasmValue>, HostFuncError> {
    Ok(vec![WasmValue::from_i32(
        Errno::__WASI_ERRNO_NOSYS.0 as i32,
    )])
}

// todo: ld asyncify yield
pub fn sched_yield(
    _frame: &mut CallingFrame,
    _wasi_ctx: &mut WasiCtx,
    _args: Vec<WasmValue>,
) -> std::result::Result<Vec<WasmValue>, HostFuncError> {
    Ok(vec![WasmValue::from_i32(
        Errno::__WASI_ERRNO_NOSYS.0 as i32,
    )])
}

//socket

pub fn sock_open(
    frame: &mut CallingFrame,
    wasi_ctx: &mut WasiCtx,
    args: Vec<WasmValue>,
) -> std::result::Result<Vec<WasmValue>, HostFuncError> {
    let mut mem = frame.memory_mut(0).ok_or(HostFuncError::Runtime(0x88))?;

    if let Some([p1, p2, p3]) = args.get(0..3) {
        let af = p1.to_i32() as u8;
        let ty = p2.to_i32() as u8;
        let ro_fd_ptr = p3.to_i32() as usize;

        Ok(to_wasm_return(p::async_socket::sock_open(
            wasi_ctx,
            &mut mem,
            af,
            ty,
            WasmPtr::from(ro_fd_ptr),
        )))
    } else {
        Err(HostFuncError::Runtime(0x83))
    }
}

pub fn sock_bind(
    frame: &mut CallingFrame,
    wasi_ctx: &mut WasiCtx,
    args: Vec<WasmValue>,
) -> std::result::Result<Vec<WasmValue>, HostFuncError> {
    let mut mem = frame.memory_mut(0).ok_or(HostFuncError::Runtime(0x88))?;

    if let Some([p1, p2, p3]) = args.get(0..3) {
        let fd = p1.to_i32();
        let addr_ptr = p2.to_i32() as usize;
        let port = p3.to_i32() as u32;
        Ok(to_wasm_return(p::async_socket::sock_bind(
            wasi_ctx,
            &mut mem,
            fd,
            WasmPtr::from(addr_ptr),
            port,
        )))
    } else {
        Err(HostFuncError::Runtime(0x83))
    }
}

pub fn sock_listen(
    frame: &mut CallingFrame,
    wasi_ctx: &mut WasiCtx,
    args: Vec<WasmValue>,
) -> std::result::Result<Vec<WasmValue>, HostFuncError> {
    let mut mem = frame.memory_mut(0).ok_or(HostFuncError::Runtime(0x88))?;

    if let Some([p1, p2]) = args.get(0..2) {
        let fd = p1.to_i32();
        let backlog = p2.to_i32() as u32;

        Ok(to_wasm_return(p::async_socket::sock_listen(
            wasi_ctx, &mut mem, fd, backlog,
        )))
    } else {
        Err(HostFuncError::Runtime(0x83))
    }
}

pub type ResultFuture =
    Box<dyn std::future::Future<Output = Result<Vec<WasmValue>, HostFuncError>> + Send>;

pub fn sock_accept(
    frame: CallingFrame,
    wasi_ctx: &'static mut WasiCtx,
    args: Vec<WasmValue>,
) -> ResultFuture {
    Box::new(async move {
        let mut mem = frame.memory_mut(0).ok_or(HostFuncError::Runtime(0x88))?;

        if let Some([p1, p2]) = args.get(0..2) {
            let fd = p1.to_i32();
            let ro_fd_ptr = p2.to_i32() as usize;

            Ok(to_wasm_return(
                p::async_socket::sock_accept(wasi_ctx, &mut mem, fd, WasmPtr::from(ro_fd_ptr))
                    .await,
            ))
        } else {
            Err(HostFuncError::Runtime(0x83))
        }
    })
}

pub fn sock_connect(
    frame: CallingFrame,
    wasi_ctx: &'static mut WasiCtx,
    args: Vec<WasmValue>,
) -> ResultFuture {
    Box::new(async move {
        let mut mem = frame.memory_mut(0).ok_or(HostFuncError::Runtime(0x88))?;

        if let Some([p1, p2, p3]) = args.get(0..3) {
            let fd = p1.to_i32();
            let addr_ptr = p2.to_i32() as usize;
            let port = p3.to_i32() as u32;

            Ok(to_wasm_return(
                p::async_socket::sock_connect(
                    wasi_ctx,
                    &mut mem,
                    fd,
                    WasmPtr::from(addr_ptr),
                    port,
                )
                .await,
            ))
        } else {
            Err(HostFuncError::Runtime(0x83))
        }
    })
}

pub fn sock_recv(
    frame: CallingFrame,
    wasi_ctx: &'static mut WasiCtx,
    args: Vec<WasmValue>,
) -> ResultFuture {
    Box::new(async move {
        let mut mem = frame.memory_mut(0).ok_or(HostFuncError::Runtime(0x88))?;

        if let Some([p1, p2, p3, p4, p5, p6]) = args.get(0..6) {
            let fd = p1.to_i32();
            let buf_ptr = p2.to_i32() as usize;
            let buf_len = p3.to_i32() as u32;
            let flags = p4.to_i32() as u16;
            let ro_data_len_ptr = p5.to_i32() as usize;
            let ro_flags_ptr = p6.to_i32() as usize;

            Ok(to_wasm_return(
                p::async_socket::sock_recv(
                    wasi_ctx,
                    &mut mem,
                    fd,
                    WasmPtr::from(buf_ptr),
                    buf_len,
                    flags,
                    WasmPtr::from(ro_data_len_ptr),
                    WasmPtr::from(ro_flags_ptr),
                )
                .await,
            ))
        } else {
            Err(HostFuncError::Runtime(0x83))
        }
    })
}

pub fn sock_recv_from(
    frame: CallingFrame,
    wasi_ctx: &'static mut WasiCtx,
    args: Vec<WasmValue>,
) -> ResultFuture {
    Box::new(async move {
        let mut mem = frame.memory_mut(0).ok_or(HostFuncError::Runtime(0x88))?;

        if let Some([p1, p2, p3, p4, p5, p6, p7, p8]) = args.get(0..8) {
            let fd = p1.to_i32();
            let buf_ptr = p2.to_i32() as usize;
            let buf_len = p3.to_i32() as u32;
            let wasi_addr_ptr = p4.to_i32() as usize;
            let flags = p5.to_i32() as u16;
            let port_ptr = p6.to_i32() as usize;
            let ro_data_len_ptr = p7.to_i32() as usize;
            let ro_flags_ptr = p8.to_i32() as usize;

            Ok(to_wasm_return(
                p::async_socket::sock_recv_from(
                    wasi_ctx,
                    &mut mem,
                    fd,
                    WasmPtr::from(buf_ptr),
                    buf_len,
                    WasmPtr::from(wasi_addr_ptr),
                    flags,
                    WasmPtr::from(port_ptr),
                    WasmPtr::from(ro_data_len_ptr),
                    WasmPtr::from(ro_flags_ptr),
                )
                .await,
            ))
        } else {
            Err(HostFuncError::Runtime(0x83))
        }
    })
}

pub fn sock_send(
    frame: CallingFrame,
    wasi_ctx: &'static mut WasiCtx,
    args: Vec<WasmValue>,
) -> ResultFuture {
    Box::new(async move {
        let mut mem = frame.memory_mut(0).ok_or(HostFuncError::Runtime(0x88))?;

        if let Some([p1, p2, p3, p4, p5]) = args.get(0..5) {
            let fd = p1.to_i32();
            let buf_ptr = p2.to_i32() as usize;
            let buf_len = p3.to_i32() as u32;
            let flags = p4.to_i32() as u16;
            let send_len_ptr = p5.to_i32() as usize;

            Ok(to_wasm_return(
                p::async_socket::sock_send(
                    wasi_ctx,
                    &mut mem,
                    fd,
                    WasmPtr::from(buf_ptr),
                    buf_len,
                    flags,
                    WasmPtr::from(send_len_ptr),
                )
                .await,
            ))
        } else {
            Err(HostFuncError::Runtime(0x83))
        }
    })
}

pub fn sock_send_to(
    frame: CallingFrame,
    wasi_ctx: &'static mut WasiCtx,
    args: Vec<WasmValue>,
) -> ResultFuture {
    Box::new(async move {
        let mut mem = frame.memory_mut(0).ok_or(HostFuncError::Runtime(0x88))?;

        if let Some([p1, p2, p3, p4, p5, p6, p7]) = args.get(0..7) {
            let fd = p1.to_i32();
            let buf_ptr = p2.to_i32() as usize;
            let buf_len = p3.to_i32() as u32;
            let wasi_addr_ptr = p4.to_i32() as usize;
            let port = p5.to_i32() as u32;
            let flags = p6.to_i32() as u16;
            let send_len_ptr = p7.to_i32() as usize;

            Ok(to_wasm_return(
                p::async_socket::sock_send_to(
                    wasi_ctx,
                    &mut mem,
                    fd,
                    WasmPtr::from(buf_ptr),
                    buf_len,
                    WasmPtr::from(wasi_addr_ptr),
                    port,
                    flags,
                    WasmPtr::from(send_len_ptr),
                )
                .await,
            ))
        } else {
            Err(HostFuncError::Runtime(0x83))
        }
    })
}

pub fn sock_shutdown(
    frame: &mut CallingFrame,
    wasi_ctx: &mut WasiCtx,
    args: Vec<WasmValue>,
) -> std::result::Result<Vec<WasmValue>, HostFuncError> {
    let mut mem = frame.memory_mut(0).ok_or(HostFuncError::Runtime(0x88))?;

    if let Some([p1, p2]) = args.get(0..2) {
        let fd = p1.to_i32();
        let how = p2.to_i32() as u8;
        Ok(to_wasm_return(p::async_socket::sock_shutdown(
            wasi_ctx, &mut mem, fd, how,
        )))
    } else {
        Err(HostFuncError::Runtime(0x83))
    }
}

pub fn sock_getpeeraddr(
    frame: &mut CallingFrame,
    wasi_ctx: &mut WasiCtx,
    args: Vec<WasmValue>,
) -> std::result::Result<Vec<WasmValue>, HostFuncError> {
    let mut mem = frame.memory_mut(0).ok_or(HostFuncError::Runtime(0x88))?;

    if let Some([p1, p2, p3, p4]) = args.get(0..4) {
        let fd = p1.to_i32();
        let wasi_addr_ptr = p2.to_i32() as usize;
        let addr_type = p3.to_i32() as usize;
        let port_ptr = p4.to_i32() as usize;
        Ok(to_wasm_return(p::async_socket::sock_getpeeraddr(
            wasi_ctx,
            &mut mem,
            fd,
            WasmPtr::from(wasi_addr_ptr),
            WasmPtr::from(addr_type),
            WasmPtr::from(port_ptr),
        )))
    } else {
        Err(HostFuncError::Runtime(0x83))
    }
}

pub fn sock_getlocaladdr(
    frame: &mut CallingFrame,
    wasi_ctx: &mut WasiCtx,
    args: Vec<WasmValue>,
) -> std::result::Result<Vec<WasmValue>, HostFuncError> {
    let mut mem = frame.memory_mut(0).ok_or(HostFuncError::Runtime(0x88))?;

    if let Some([p1, p2, p3, p4]) = args.get(0..4) {
        let fd = p1.to_i32();
        let wasi_addr_ptr = p2.to_i32() as usize;
        let addr_type = p3.to_i32() as usize;
        let port_ptr = p4.to_i32() as usize;
        Ok(to_wasm_return(p::async_socket::sock_getlocaladdr(
            wasi_ctx,
            &mut mem,
            fd,
            WasmPtr::from(wasi_addr_ptr),
            WasmPtr::from(addr_type),
            WasmPtr::from(port_ptr),
        )))
    } else {
        Err(HostFuncError::Runtime(0x83))
    }
}

pub fn sock_getsockopt(
    frame: &mut CallingFrame,
    wasi_ctx: &mut WasiCtx,
    args: Vec<WasmValue>,
) -> std::result::Result<Vec<WasmValue>, HostFuncError> {
    let mut mem = frame.memory_mut(0).ok_or(HostFuncError::Runtime(0x88))?;

    if let Some([p1, p2, p3, p4, p5]) = args.get(0..5) {
        let fd = p1.to_i32();
        let level = p2.to_i32() as u32;
        let name = p3.to_i32() as u32;
        let flag = p4.to_i32() as usize;
        let flag_size_ptr = p5.to_i32() as usize;
        Ok(to_wasm_return(p::async_socket::sock_getsockopt(
            wasi_ctx,
            &mut mem,
            fd,
            level,
            name,
            WasmPtr::from(flag),
            WasmPtr::from(flag_size_ptr),
        )))
    } else {
        Err(HostFuncError::Runtime(0x83))
    }
}

pub fn sock_setsockopt(
    frame: &mut CallingFrame,
    wasi_ctx: &mut WasiCtx,
    args: Vec<WasmValue>,
) -> std::result::Result<Vec<WasmValue>, HostFuncError> {
    let mut mem = frame.memory_mut(0).ok_or(HostFuncError::Runtime(0x88))?;

    if let Some([p1, p2, p3, p4, p5]) = args.get(0..5) {
        let fd = p1.to_i32();
        let level = p2.to_i32() as u32;
        let name = p3.to_i32() as u32;
        let flag = p4.to_i32() as usize;
        let flag_size = p5.to_i32() as u32;
        Ok(to_wasm_return(p::async_socket::sock_setsockopt(
            wasi_ctx,
            &mut mem,
            fd,
            level,
            name,
            WasmPtr::from(flag),
            flag_size,
        )))
    } else {
        Err(HostFuncError::Runtime(0x83))
    }
}

pub fn poll_oneoff(
    frame: CallingFrame,
    wasi_ctx: &'static mut WasiCtx,
    args: Vec<WasmValue>,
) -> ResultFuture {
    Box::new(async move {
        let mut mem = frame.memory_mut(0).ok_or(HostFuncError::Runtime(0x88))?;

        if let Some([p1, p2, p3, p4]) = args.get(0..4) {
            let in_ptr = p1.to_i32() as usize;
            let out_ptr = p2.to_i32() as usize;
            let nsubscriptions = p3.to_i32() as u32;
            let revents_num_ptr = p4.to_i32() as usize;

            Ok(to_wasm_return(
                p::async_poll::poll_oneoff(
                    wasi_ctx,
                    &mut mem,
                    WasmPtr::from(in_ptr),
                    WasmPtr::from(out_ptr),
                    nsubscriptions,
                    WasmPtr::from(revents_num_ptr),
                )
                .await,
            ))
        } else {
            Err(HostFuncError::Runtime(0x83))
        }
    })
}

pub fn sock_lookup_ip(
    frame: CallingFrame,
    wasi_ctx: &'static mut WasiCtx,
    args: Vec<WasmValue>,
) -> ResultFuture {
    Box::new(async move {
        let mut mem = frame.memory_mut(0).ok_or(HostFuncError::Runtime(0x88))?;

        if let Some([p1, p2, p3, p4, p5, p6]) = args.get(0..6) {
            let host_name_ptr = p1.to_i32() as usize;
            let host_name_len = p2.to_i32() as u32;
            let lookup_type = p3.to_i32() as u8;
            let addr_buf = p4.to_i32() as usize;
            let addr_buf_max_len = p5.to_i32() as u32;
            let raddr_num_ptr = p6.to_i32() as usize;
            Ok(to_wasm_return(
                p::async_socket::sock_lookup_ip(
                    wasi_ctx,
                    &mut mem,
                    WasmPtr::from(host_name_ptr),
                    host_name_len,
                    lookup_type,
                    WasmPtr::from(addr_buf),
                    addr_buf_max_len,
                    WasmPtr::from(raddr_num_ptr),
                )
                .await,
            ))
        } else {
            Err(HostFuncError::Runtime(0x83))
        }
    })
}

pub enum WasiFunc<T: 'static> {
    SyncFn(String, (Vec<ValType>, Vec<ValType>), HostFn<T>),
    AsyncFn(String, (Vec<ValType>, Vec<ValType>), AsyncHostFn<T>),
}

pub fn wasi_impls() -> Vec<WasiFunc<WasiCtx>> {
    macro_rules! sync_fn {
        ($name:expr,$ty:expr,$f:ident) => {
            WasiFunc::SyncFn($name.into(), $ty, $f)
        };
    }
    macro_rules! async_fn {
        ($name:expr,$ty:expr,$f:ident) => {
            WasiFunc::AsyncFn($name.into(), $ty, $f)
        };
    }
    let impls = vec![
        sync_fn!(
            "args_get",
            (vec![ValType::I32, ValType::I32], vec![ValType::I32]),
            args_get
        ),
        sync_fn!(
            "args_sizes_get",
            (vec![ValType::I32, ValType::I32], vec![ValType::I32]),
            args_sizes_get
        ),
        sync_fn!(
            "environ_get",
            (vec![ValType::I32, ValType::I32], vec![ValType::I32]),
            environ_get
        ),
        sync_fn!(
            "environ_sizes_get",
            (vec![ValType::I32, ValType::I32], vec![ValType::I32]),
            environ_sizes_get
        ),
        sync_fn!(
            "clock_res_get",
            (vec![ValType::I32, ValType::I64], vec![ValType::I32]),
            clock_res_get
        ),
        sync_fn!(
            "clock_time_get",
            (
                vec![ValType::I32, ValType::I64, ValType::I32],
                vec![ValType::I32],
            ),
            clock_time_get
        ),
        sync_fn!(
            "random_get",
            (vec![ValType::I32, ValType::I32], vec![ValType::I32]),
            random_get
        ),
        sync_fn!(
            "fd_prestat_get",
            (vec![ValType::I32, ValType::I32], vec![ValType::I32]),
            fd_prestat_get
        ),
        sync_fn!(
            "fd_prestat_dir_name",
            (
                vec![ValType::I32, ValType::I32, ValType::I32],
                vec![ValType::I32],
            ),
            fd_prestat_dir_name
        ),
        sync_fn!(
            "fd_renumber",
            (vec![ValType::I32, ValType::I32], vec![ValType::I32]),
            fd_renumber
        ),
        sync_fn!(
            "fd_advise",
            (
                vec![ValType::I32, ValType::I64, ValType::I64, ValType::I32],
                vec![ValType::I32],
            ),
            fd_advise
        ),
        sync_fn!(
            "fd_allocate",
            (
                vec![ValType::I32, ValType::I64, ValType::I64],
                vec![ValType::I32],
            ),
            fd_allocate
        ),
        sync_fn!(
            "fd_close",
            (vec![ValType::I32], vec![ValType::I32]),
            fd_close
        ),
        sync_fn!(
            "fd_seek",
            (
                vec![ValType::I32, ValType::I64, ValType::I32, ValType::I32],
                vec![ValType::I32],
            ),
            fd_seek
        ),
        sync_fn!("fd_sync", (vec![ValType::I32], vec![ValType::I32]), fd_sync),
        sync_fn!(
            "fd_datasync",
            (vec![ValType::I32], vec![ValType::I32]),
            fd_datasync
        ),
        sync_fn!(
            "fd_tell",
            (vec![ValType::I32, ValType::I32], vec![ValType::I32]),
            fd_tell
        ),
        sync_fn!(
            "fd_fdstat_get",
            (vec![ValType::I32, ValType::I32], vec![ValType::I32]),
            fd_fdstat_get
        ),
        sync_fn!(
            "fd_fdstat_set_flags",
            (vec![ValType::I32, ValType::I32], vec![ValType::I32]),
            fd_fdstat_set_flags
        ),
        sync_fn!(
            "fd_fdstat_set_rights",
            (
                vec![ValType::I32, ValType::I64, ValType::I64],
                vec![ValType::I32],
            ),
            fd_fdstat_set_rights
        ),
        sync_fn!(
            "fd_filestat_get",
            (vec![ValType::I32, ValType::I32], vec![ValType::I32]),
            fd_filestat_get
        ),
        sync_fn!(
            "fd_filestat_set_size",
            (vec![ValType::I32, ValType::I32], vec![ValType::I32]),
            fd_filestat_set_size
        ),
        sync_fn!(
            "fd_filestat_set_times",
            (
                vec![ValType::I32, ValType::I64, ValType::I64, ValType::I32],
                vec![ValType::I32],
            ),
            fd_filestat_set_times
        ),
        sync_fn!(
            "fd_read",
            (
                vec![ValType::I32, ValType::I32, ValType::I32, ValType::I32],
                vec![ValType::I32],
            ),
            fd_read
        ),
        sync_fn!(
            "fd_pread",
            (
                vec![
                    ValType::I32,
                    ValType::I32,
                    ValType::I32,
                    ValType::I64,
                    ValType::I32,
                ],
                vec![ValType::I32],
            ),
            fd_pread
        ),
        sync_fn!(
            "fd_write",
            (
                vec![ValType::I32, ValType::I32, ValType::I32, ValType::I32],
                vec![ValType::I32],
            ),
            fd_write
        ),
        sync_fn!(
            "fd_pwrite",
            (
                vec![
                    ValType::I32,
                    ValType::I32,
                    ValType::I32,
                    ValType::I64,
                    ValType::I32,
                ],
                vec![ValType::I32],
            ),
            fd_pwrite
        ),
        sync_fn!(
            "fd_readdir",
            (
                vec![
                    ValType::I32,
                    ValType::I32,
                    ValType::I32,
                    ValType::I64,
                    ValType::I32,
                ],
                vec![ValType::I32],
            ),
            fd_readdir
        ),
        sync_fn!(
            "path_create_directory",
            (
                vec![ValType::I32, ValType::I32, ValType::I32],
                vec![ValType::I32],
            ),
            path_create_directory
        ),
        sync_fn!(
            "path_filestat_get",
            (
                vec![
                    ValType::I32,
                    ValType::I32,
                    ValType::I32,
                    ValType::I32,
                    ValType::I32,
                ],
                vec![ValType::I32],
            ),
            path_filestat_get
        ),
        sync_fn!(
            "path_filestat_set_times",
            (
                vec![
                    ValType::I32,
                    ValType::I32,
                    ValType::I32,
                    ValType::I32,
                    ValType::I64,
                    ValType::I64,
                    ValType::I32,
                ],
                vec![ValType::I32],
            ),
            path_filestat_set_times
        ),
        sync_fn!(
            "path_link",
            (
                vec![
                    ValType::I32,
                    ValType::I32,
                    ValType::I32,
                    ValType::I32,
                    ValType::I32,
                    ValType::I32,
                    ValType::I32,
                ],
                vec![ValType::I32],
            ),
            path_link
        ),
        sync_fn!(
            "path_open",
            (
                vec![
                    ValType::I32,
                    ValType::I32,
                    ValType::I32,
                    ValType::I32,
                    ValType::I32,
                    ValType::I64,
                    ValType::I64,
                    ValType::I32,
                    ValType::I32,
                ],
                vec![ValType::I32],
            ),
            path_open
        ),
        sync_fn!(
            "path_readlink",
            (
                vec![
                    ValType::I32,
                    ValType::I32,
                    ValType::I32,
                    ValType::I32,
                    ValType::I32,
                    ValType::I32,
                ],
                vec![ValType::I32],
            ),
            path_readlink
        ),
        sync_fn!(
            "path_remove_directory",
            (
                vec![ValType::I32, ValType::I32, ValType::I32],
                vec![ValType::I32],
            ),
            path_remove_directory
        ),
        sync_fn!(
            "path_rename",
            (
                vec![
                    ValType::I32,
                    ValType::I32,
                    ValType::I32,
                    ValType::I32,
                    ValType::I32,
                    ValType::I32,
                ],
                vec![ValType::I32],
            ),
            path_rename
        ),
        sync_fn!(
            "path_rename",
            (
                vec![
                    ValType::I32,
                    ValType::I32,
                    ValType::I32,
                    ValType::I32,
                    ValType::I32,
                ],
                vec![ValType::I32],
            ),
            path_rename
        ),
        sync_fn!(
            "path_unlink_file",
            (
                vec![ValType::I32, ValType::I32, ValType::I32],
                vec![ValType::I32],
            ),
            path_unlink_file
        ),
        sync_fn!("proc_exit", (vec![ValType::I32], vec![]), proc_exit),
        sync_fn!(
            "proc_raise",
            (vec![ValType::I32], vec![ValType::I32]),
            proc_raise
        ),
        sync_fn!("sched_yield", (vec![], vec![ValType::I32]), sched_yield),
        sync_fn!(
            "sock_open",
            (
                vec![ValType::I32, ValType::I32, ValType::I32],
                vec![ValType::I32],
            ),
            sock_open
        ),
        sync_fn!(
            "sock_bind",
            (
                vec![ValType::I32, ValType::I32, ValType::I32],
                vec![ValType::I32],
            ),
            sock_bind
        ),
        sync_fn!(
            "sock_listen",
            (vec![ValType::I32, ValType::I32], vec![ValType::I32]),
            sock_listen
        ),
        async_fn!(
            "sock_accept",
            (vec![ValType::I32, ValType::I32], vec![ValType::I32]),
            sock_accept
        ),
        async_fn!(
            "sock_connect",
            (
                vec![ValType::I32, ValType::I32, ValType::I32],
                vec![ValType::I32],
            ),
            sock_connect
        ),
        async_fn!(
            "sock_recv",
            (
                vec![
                    ValType::I32,
                    ValType::I32,
                    ValType::I32,
                    ValType::I32,
                    ValType::I32,
                    ValType::I32,
                ],
                vec![ValType::I32],
            ),
            sock_recv
        ),
        async_fn!(
            "sock_recv_from",
            (
                vec![
                    ValType::I32,
                    ValType::I32,
                    ValType::I32,
                    ValType::I32,
                    ValType::I32,
                    ValType::I32,
                    ValType::I32,
                    ValType::I32,
                ],
                vec![ValType::I32],
            ),
            sock_recv_from
        ),
        async_fn!(
            "sock_send",
            (
                vec![
                    ValType::I32,
                    ValType::I32,
                    ValType::I32,
                    ValType::I32,
                    ValType::I32,
                ],
                vec![ValType::I32],
            ),
            sock_send
        ),
        async_fn!(
            "sock_send_to",
            (
                vec![
                    ValType::I32,
                    ValType::I32,
                    ValType::I32,
                    ValType::I32,
                    ValType::I32,
                    ValType::I32,
                    ValType::I32,
                ],
                vec![ValType::I32],
            ),
            sock_send_to
        ),
        sync_fn!(
            "sock_shutdown",
            (vec![ValType::I32, ValType::I32], vec![ValType::I32]),
            sock_shutdown
        ),
        sync_fn!(
            "sock_getpeeraddr",
            (
                vec![ValType::I32, ValType::I32, ValType::I32, ValType::I32],
                vec![ValType::I32],
            ),
            sock_getpeeraddr
        ),
        sync_fn!(
            "sock_getlocaladdr",
            (
                vec![ValType::I32, ValType::I32, ValType::I32, ValType::I32],
                vec![ValType::I32],
            ),
            sock_getlocaladdr
        ),
        sync_fn!(
            "sock_getsockopt",
            (
                vec![
                    ValType::I32,
                    ValType::I32,
                    ValType::I32,
                    ValType::I32,
                    ValType::I32,
                ],
                vec![ValType::I32],
            ),
            sock_getlocaladdr
        ),
        sync_fn!(
            "sock_setsockopt",
            (
                vec![
                    ValType::I32,
                    ValType::I32,
                    ValType::I32,
                    ValType::I32,
                    ValType::I32,
                ],
                vec![ValType::I32],
            ),
            sock_setsockopt
        ),
        async_fn!(
            "poll_oneoff",
            (
                vec![ValType::I32, ValType::I32, ValType::I32, ValType::I32],
                vec![ValType::I32],
            ),
            poll_oneoff
        ),
        async_fn!(
            "sock_lookup_ip",
            (
                vec![
                    ValType::I32,
                    ValType::I32,
                    ValType::I32,
                    ValType::I32,
                    ValType::I32,
                    ValType::I32,
                ],
                vec![ValType::I32],
            ),
            sock_lookup_ip
        ),
    ];
    impls
}

// * define WasiVm

pub struct WasiVm<VM> {
    vm: VM,
    wasi_ctx: Box<WasiCtx>,
}

impl<VM> WasiVm<VM>
where
    VM: AsRef<Vm> + AsMut<Vm>,
{
    pub fn create(
        mut vm: VM,
        mut wasi_ctx: Box<WasiCtx>,
        wasi_fns: Vec<WasiFunc<WasiCtx>>,
    ) -> WasmEdgeResult<Self> {
        let data = wasi_ctx.as_mut();
        let mut import_object = wasmedge_sys::ImportModule::create("wasi_snapshot_preview1")?;
        for wasi_fn in wasi_fns {
            match wasi_fn {
                WasiFunc::SyncFn(name, ty, real_fn) => {
                    let fn_ty = FuncType::create(ty.0, ty.1)?;
                    let func = unsafe { functions::new_sync_function(&fn_ty, real_fn, data, 0) }?;
                    import_object.add_func(name, func);
                }
                WasiFunc::AsyncFn(name, ty, real_fn) => {
                    let fn_ty = FuncType::create(ty.0, ty.1)?;
                    let func = unsafe { functions::new_async_function(&fn_ty, real_fn, data, 0) }?;
                    import_object.add_func(name, func);
                }
            }
        }
        vm.as_mut()
            .register_import_module_(wasmedge_sys::ImportObject::Import(import_object).into())?;
        Ok(Self { vm, wasi_ctx })
    }

    pub fn unpack(self) -> (VM, Box<WasiCtx>) {
        (self.vm, self.wasi_ctx)
    }
}

impl<VM: AsRef<Vm>> AsRef<Vm> for WasiVm<VM> {
    fn as_ref(&self) -> &Vm {
        self.vm.as_ref()
    }
}

impl<VM: AsMut<Vm>> AsMut<Vm> for WasiVm<VM> {
    fn as_mut(&mut self) -> &mut Vm {
        self.vm.as_mut()
    }
}

#[cfg(test)]
mod test {
    use super::*;
    use wasmedge_sdk::VmBuilder;

    #[tokio::test]
    pub async fn test_vm() {
        async fn tick() {
            let mut i = 0;
            loop {
                println!("i={i}");
                tokio::time::sleep(std::time::Duration::from_millis(500)).await;
                i += 1;
            }
        }

        let vm = Box::new(VmBuilder::default().build().unwrap());
        let wasi_funcs = wasi_impls();
        let mut wasi_ctx = Box::new(WasiCtx::new());
        wasi_ctx.push_arg("abc".into());
        wasi_ctx.push_env("a", "1");
        let mut wasi_vm = WasiVm::create(vm, wasi_ctx, wasi_funcs).unwrap();

        tokio::spawn(tick());
        let r = wasi_vm
            .as_mut()
            .run_func_from_file_async("hello.wasm", "_start", [])
            .await
            .unwrap();

        println!("{r:?}");
    }
}
