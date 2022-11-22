//! Defines WasmEdge VmDock and Param types.

use crate::{
    error::{InstanceError, WasmEdgeError},
    params, Memory, Vm, WasmEdgeResult, WasmVal, WasmValue,
};
use num_derive::FromPrimitive;
use num_traits::FromPrimitive;
use std::any::Any;

/// Extends a [Vm](crate::Vm) instance by supporting function arguments of Rust built-in types.
#[derive(Debug)]
pub struct VmDock {
    pub(crate) vm: Box<Vm>, // Can't use Arc because vm can be get_mut after cloned for hostfunc
}
impl VmDock {
    /// Creates a new [VmDock] to be associated with the given [Vm](crate::Vm).
    ///
    /// # Arguments
    ///
    /// * `vm` - The [Vm] instance to be extended.
    ///
    pub fn new(vm: Vm) -> Self {
        VmDock { vm: Box::new(vm) }
    }

    /// Runs an exported WASM function registered in a named or active module.
    ///
    /// # Arguments
    ///
    /// * `func_name` - The name of the exported WASM function to run.
    ///
    /// * `params` - The parameter values passed to the exported WASM function.
    ///
    /// # Error
    ///
    /// If fail to run the WASM function, then an error is returned.
    pub fn run_func(
        &self,
        func_name: impl AsRef<str>,
        params: Vec<Param>,
    ) -> WasmEdgeResult<Result<Vec<Box<dyn Any + Send + Sync>>, String>> {
        let inputs_count = params.len() as i32;

        // allocate new frame for passing pointers
        let pointer_of_pointers = match self.alloc(None, params!(inputs_count * 4 * 2)) {
            Ok(res) => res[0].to_i32(),
            Err(err) => {
                return Err(err);
            }
        };

        let mut memory = self.vm.active_module()?.memory("memory").ok_or_else(|| {
            WasmEdgeError::Instance(InstanceError::NotFoundMem("memory".to_string()))
        })?;

        for (pos, param) in params.iter().enumerate() {
            let sr = param.settle(self, &mut memory);
            let (pointer, length_of_input) = match sr {
                Ok(r) => (r.0, r.1),
                Err(e) => {
                    println!("run_wasm error: {}", func_name.as_ref());
                    return Err(e);
                }
            };

            memory.write(
                pointer.to_le_bytes(),
                pointer_of_pointers as u32 + pos as u32 * 4 * 2,
            )?;
            memory.write(
                length_of_input.to_le_bytes(),
                pointer_of_pointers as u32 + pos as u32 * 4 * 2 + 4,
            )?;
        }

        // call host function
        let rets = self
            .vm
            .run_func(None, func_name, params!(pointer_of_pointers, inputs_count))?;

        let rvec = memory.read(rets[0].to_i32() as u32, 9)?;
        self.free(None, params!(rets[0].to_i32(), 9))?;

        let flag = rvec[0];
        let ret_pointer = i32::from_le_bytes(rvec[1..5].try_into().unwrap());
        let ret_len = i32::from_le_bytes(rvec[5..9].try_into().unwrap());

        match flag {
            0 => Ok(Ok(self.parse_result(ret_pointer, ret_len)?)),
            _ => Ok(Err(self.parse_error(ret_pointer, ret_len)?)),
        }
    }

    fn parse_error(&self, ret_pointer: i32, ret_len: i32) -> WasmEdgeResult<String> {
        let memory = self.vm.active_module()?.memory("memory").ok_or_else(|| {
            WasmEdgeError::Operation(
                "[wasmedge-sdk::VmDock] Not found memory instance named 'memory'".to_string(),
            )
        })?;
        let err_bytes = memory.read(ret_pointer as u32, ret_len as u32)?;
        self.free(None, params!(ret_pointer, ret_len))?;
        Ok(String::from_utf8(err_bytes).map_err(WasmEdgeError::FromUtf8)?)
    }

    fn parse_result(
        &self,
        ret_pointer: i32,
        ret_len: i32,
    ) -> WasmEdgeResult<Vec<Box<dyn Any + Send + Sync>>> {
        let size = ret_len as usize;
        let memory = self.vm.active_module().unwrap().memory("memory").unwrap();
        let p_data = memory
            .read(ret_pointer as u32, size as u32 * 3 * 4)
            .unwrap();
        self.free(None, params!(ret_pointer, size as i32 * 3 * 4))?;

        let mut p_values = vec![0; size * 3];
        for i in 0..size * 3 {
            p_values[i] = i32::from_le_bytes(p_data[i * 4..(i + 1) * 4].try_into().unwrap());
        }

        let mut results: Vec<Box<dyn Any + Send + Sync>> = Vec::with_capacity(size);
        for i in 0..size {
            let bytes = memory
                .read(p_values[i * 3] as u32, p_values[i * 3 + 2] as u32)
                .unwrap();
            self.free(None, params!(p_values[i * 3], p_values[i * 3 + 2]))?;
            match FromPrimitive::from_i32(p_values[i * 3 + 1]) {
                Some(RetTypes::U8) => {
                    results.push(Box::new(bytes[0]));
                }
                Some(RetTypes::I8) => {
                    results.push(Box::new(bytes[0] as i8));
                }
                Some(RetTypes::U16) => {
                    let v = u16::from_le_bytes(bytes.try_into().unwrap());
                    results.push(Box::new(v));
                }
                Some(RetTypes::I16) => {
                    let v = i16::from_le_bytes(bytes.try_into().unwrap());
                    results.push(Box::new(v));
                }
                Some(RetTypes::U32) => {
                    let v = u32::from_le_bytes(bytes.try_into().unwrap());
                    results.push(Box::new(v));
                }
                Some(RetTypes::I32) => {
                    let v = i32::from_le_bytes(bytes.try_into().unwrap());
                    results.push(Box::new(v));
                }
                Some(RetTypes::U64) => {
                    let v = u64::from_le_bytes(bytes.try_into().unwrap());
                    results.push(Box::new(v));
                }
                Some(RetTypes::I64) => {
                    let v = i64::from_le_bytes(bytes.try_into().unwrap());
                    results.push(Box::new(v));
                }
                Some(RetTypes::F32) => {
                    let v = f32::from_le_bytes(bytes.try_into().unwrap());
                    results.push(Box::new(v));
                }
                Some(RetTypes::F64) => {
                    let v = f64::from_le_bytes(bytes.try_into().unwrap());
                    results.push(Box::new(v));
                }
                Some(RetTypes::Bool) => {
                    results.push(Box::new(bytes[0] == 1_u8));
                }
                Some(RetTypes::Char) => {
                    let v = u32::from_le_bytes(bytes.try_into().unwrap());
                    results.push(Box::new(char::from_u32(v)));
                }
                Some(RetTypes::U8Array) => {
                    let len = bytes.len();
                    let mut v = vec![0; len];
                    v[..len].copy_from_slice(&bytes[..len]);
                    results.push(Box::new(v));
                }
                Some(RetTypes::I8Array) => {
                    let len = bytes.len();
                    let mut v = vec![0; len];
                    for i in 0..len {
                        v[i] = bytes[i] as i8;
                    }
                    results.push(Box::new(v));
                }
                Some(RetTypes::U16Array) => {
                    let len = bytes.len() / 2;
                    let mut v = vec![0; len];
                    for i in 0..len {
                        v[i] = u16::from_le_bytes(bytes[i * 2..(i + 1) * 2].try_into().unwrap());
                    }
                    results.push(Box::new(v));
                }
                Some(RetTypes::I16Array) => {
                    let len = bytes.len() / 2;
                    let mut v = vec![0; len];
                    for i in 0..len {
                        v[i] = i16::from_le_bytes(bytes[i * 2..(i + 1) * 2].try_into().unwrap());
                    }
                    results.push(Box::new(v));
                }
                Some(RetTypes::U32Array) => {
                    let len = bytes.len() / 4;
                    let mut v = vec![0; len];
                    for i in 0..len {
                        v[i] = u32::from_le_bytes(bytes[i * 4..(i + 1) * 4].try_into().unwrap());
                    }
                    results.push(Box::new(v));
                }
                Some(RetTypes::I32Array) => {
                    let len = bytes.len() / 4;
                    let mut v = vec![0; len];
                    for i in 0..len {
                        v[i] = i32::from_le_bytes(bytes[i * 4..(i + 1) * 4].try_into().unwrap());
                    }
                    results.push(Box::new(v));
                }
                Some(RetTypes::U64Array) => {
                    let len = bytes.len() / 8;
                    let mut v = vec![0; len];
                    for i in 0..len {
                        v[i] = u64::from_le_bytes(bytes[i * 8..(i + 1) * 8].try_into().unwrap());
                    }
                    results.push(Box::new(v));
                }
                Some(RetTypes::I64Array) => {
                    let len = bytes.len() / 8;
                    let mut v = vec![0; len];
                    for i in 0..len {
                        v[i] = i64::from_le_bytes(bytes[i * 8..(i + 1) * 8].try_into().unwrap());
                    }
                    results.push(Box::new(v));
                }
                Some(RetTypes::String) => {
                    results.push(Box::new(String::from_utf8(bytes).unwrap()));
                }
                None => {}
            }
        }

        Ok(results)
    }

    fn alloc(
        &self,
        mod_name: Option<&str>,
        args: impl IntoIterator<Item = WasmValue>,
    ) -> WasmEdgeResult<Vec<WasmValue>> {
        self.vm.run_func(mod_name, "allocate", args)
    }

    fn free(
        &self,
        mod_name: Option<&str>,
        args: impl IntoIterator<Item = WasmValue>,
    ) -> WasmEdgeResult<Vec<WasmValue>> {
        self.vm.run_func(mod_name, "deallocate", args)
    }
}
unsafe impl Send for VmDock {}
unsafe impl Sync for VmDock {}

/// Defines a type container that wraps a value of Rust built-in type.
#[derive(Debug)]
pub enum Param<'a> {
    I8(i8),
    U8(u8),
    I16(i16),
    U16(u16),
    I32(i32),
    U32(u32),
    I64(i64),
    U64(u64),
    F32(f32),
    F64(f64),
    Bool(bool),
    VecI8(&'a Vec<i8>),
    VecU8(&'a Vec<u8>),
    VecI16(&'a Vec<i16>),
    VecU16(&'a Vec<u16>),
    VecI32(&'a Vec<i32>),
    VecU32(&'a Vec<u32>),
    VecI64(&'a Vec<i64>),
    VecU64(&'a Vec<u64>),
    String(&'a str),
}
impl<'a> Param<'a> {
    fn settle(&self, vm: &VmDock, mem: &mut Memory) -> WasmEdgeResult<(i32, i32)> {
        match self {
            Param::I8(v) => {
                let length = 1;
                let pointer = Param::allocate(vm, length)?;
                mem.write(vec![*v as u8], pointer as u32)?;
                Ok((pointer, length))
            }
            Param::U8(v) => {
                let length = 1;
                let pointer = Param::allocate(vm, length)?;
                mem.write(vec![*v], pointer as u32)?;
                Ok((pointer, length))
            }
            Param::I16(v) => {
                let length = 1;
                let pointer = Param::allocate(vm, length * 2)?;
                let bytes = v.to_le_bytes();
                mem.write(bytes, pointer as u32)?;
                Ok((pointer, length))
            }
            Param::U16(v) => {
                let length = 2;
                let pointer = Param::allocate(vm, length * 2)?;
                let bytes = v.to_le_bytes();
                mem.write(bytes, pointer as u32)?;
                Ok((pointer, length))
            }
            Param::I32(v) => {
                let length = 1;
                let pointer = Param::allocate(vm, length * 4)?;
                let bytes = v.to_le_bytes();
                mem.write(bytes, pointer as u32)?;
                Ok((pointer, length))
            }
            Param::U32(v) => {
                let length = 1;
                let pointer = Param::allocate(vm, length * 4)?;
                let bytes = v.to_le_bytes();
                mem.write(bytes, pointer as u32)?;
                Ok((pointer, length))
            }
            Param::I64(v) => {
                let length = 1;
                let pointer = Param::allocate(vm, length * 8)?;
                let bytes = v.to_le_bytes();
                mem.write(bytes, pointer as u32)?;
                Ok((pointer, length))
            }
            Param::U64(v) => {
                let length = 1;
                let pointer = Param::allocate(vm, length * 8)?;
                let bytes = v.to_le_bytes();
                mem.write(bytes, pointer as u32)?;
                Ok((pointer, length))
            }
            Param::F32(v) => {
                let length = 1;
                let pointer = Param::allocate(vm, length * 4)?;
                let bytes = v.to_le_bytes();
                mem.write(bytes, pointer as u32)?;
                Ok((pointer, length))
            }
            Param::F64(v) => {
                let length = 1;
                let pointer = Param::allocate(vm, length * 8)?;
                let bytes = v.to_le_bytes();
                mem.write(bytes, pointer as u32)?;
                Ok((pointer, length))
            }
            Param::Bool(v) => {
                let length = 1;
                let pointer = Param::allocate(vm, length)?;
                let byte: u8 = match v {
                    true => 1,
                    false => 0,
                };
                mem.write(vec![byte], pointer as u32)?;
                Ok((pointer, length))
            }
            Param::VecI8(v) => {
                let length = v.len() as i32;
                let pointer = Param::allocate(vm, length)?;
                let mut bytes = vec![0; length as usize];
                for (pos, iv) in v.iter().enumerate() {
                    bytes[pos] = *iv as u8;
                }
                mem.write(bytes, pointer as u32)?;
                Ok((pointer, length))
            }
            Param::VecU8(v) => {
                let length = v.len() as i32;
                let pointer = Param::allocate(vm, length)?;
                let mut bytes = vec![0; length as usize];
                for (pos, iv) in v.iter().enumerate() {
                    bytes[pos] = *iv;
                }
                mem.write(bytes, pointer as u32)?;
                Ok((pointer, length))
            }
            Param::VecI16(v) => {
                let length = v.len() as i32;
                let pointer = Param::allocate(vm, length * 2)?;
                let mut bytes = vec![0; length as usize * 2];
                for (pos, iv) in v.iter().enumerate() {
                    let b = iv.to_le_bytes();
                    for i in 0..2 {
                        bytes[pos * 2 + i] = b[i];
                    }
                }
                mem.write(bytes, pointer as u32)?;
                Ok((pointer, length))
            }
            Param::VecU16(v) => {
                let length = v.len() as i32;
                let pointer = Param::allocate(vm, length * 2)?;
                let mut bytes = vec![0; length as usize * 2];
                for (pos, iv) in v.iter().enumerate() {
                    let b = iv.to_le_bytes();
                    for i in 0..2 {
                        bytes[pos * 2 + i] = b[i];
                    }
                }
                mem.write(bytes, pointer as u32)?;
                Ok((pointer, length))
            }
            Param::VecI32(v) => {
                let length = v.len() as i32;
                let pointer = Param::allocate(vm, length * 4)?;
                let mut bytes = vec![0; length as usize * 4];
                for (pos, iv) in v.iter().enumerate() {
                    let b = iv.to_le_bytes();
                    for i in 0..4 {
                        bytes[pos * 4 + i] = b[i];
                    }
                }
                mem.write(bytes, pointer as u32)?;
                Ok((pointer, length))
            }
            Param::VecU32(v) => {
                let length = v.len() as i32;
                let pointer = Param::allocate(vm, length * 4)?;
                let mut bytes = vec![0; length as usize * 4];
                for (pos, iv) in v.iter().enumerate() {
                    let b = iv.to_le_bytes();
                    for i in 0..4 {
                        bytes[pos * 4 + i] = b[i];
                    }
                }
                mem.write(bytes, pointer as u32)?;
                Ok((pointer, length))
            }
            Param::VecI64(v) => {
                let length = v.len() as i32;
                let pointer = Param::allocate(vm, length * 8)?;
                let mut bytes = vec![0; length as usize * 8];
                for (pos, iv) in v.iter().enumerate() {
                    let b = iv.to_le_bytes();
                    for i in 0..8 {
                        bytes[pos * 8 + i] = b[i];
                    }
                }
                mem.write(bytes, pointer as u32)?;
                Ok((pointer, length))
            }
            Param::VecU64(v) => {
                let length = v.len() as i32;
                let pointer = Param::allocate(vm, length * 8)?;
                let mut bytes = vec![0; length as usize * 8];
                for (pos, iv) in v.iter().enumerate() {
                    let b = iv.to_le_bytes();
                    for i in 0..8 {
                        bytes[pos * 8 + i] = b[i];
                    }
                }
                mem.write(bytes, pointer as u32)?;
                Ok((pointer, length))
            }
            Param::String(v) => {
                let bytes = v.as_bytes().to_vec();
                let length = bytes.len() as i32;
                let pointer = Param::allocate(vm, length)?;
                mem.write(bytes, pointer as u32)?;
                Ok((pointer, length))
            }
        }
    }

    fn allocate(dock: &VmDock, size: i32) -> WasmEdgeResult<i32> {
        let res = dock.vm.run_func(None, "allocate", params!(size))?;

        Ok(res[0].to_i32())
    }
}

#[derive(FromPrimitive)]
enum RetTypes {
    U8 = 1,
    I8 = 2,
    U16 = 3,
    I16 = 4,
    U32 = 5,
    I32 = 6,
    U64 = 7,
    I64 = 8,
    F32 = 9,
    F64 = 10,
    Bool = 11,
    Char = 12,
    U8Array = 21,
    I8Array = 22,
    U16Array = 23,
    I16Array = 24,
    U32Array = 25,
    I32Array = 26,
    U64Array = 27,
    I64Array = 28,
    String = 31,
}
