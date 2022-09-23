mod ffi {

    #[link(wasm_import_module = "wasi_ephemeral_crypto_signatures")]
    extern "C" {
        pub fn signature_import(
            AlgPtr: *const u8,
            AlgLen: u32,
            EncodedPtr: *const u8,
            EncodedLen: u32,
            Encoding: u16,
            SigHandlePtr: *mut u32,
        ) -> i32;
        pub fn signature_export(
            SigHandle: u32,
            Encoding: u32,
            ArrayOutputHandlePtr: *mut u32,
        ) -> i32;
    }

    #[link(wasm_import_module = "wasi_ephemeral_crypto_common")]
    extern "C" {
        pub fn array_output_len(ArrayOutputHandle: u32, SizePtr: *mut u32) -> i32;
        pub fn array_output_pull(
            ArrayOutputHandle: u32,
            BufPtr: *mut u8,
            BufLen: u32,
            SizePtr: *mut u32,
        ) -> i32;
    }
}

pub fn signature_import<S: AsRef<[u8]>>(
    alg: String,
    encoded: S,
    encoding: u16,
) -> Result<u32, i32> {
    let alg_size = alg.len() as u32;
    let encoded_size = encoded.as_ref().len() as u32;
    let mut sig_handle: u32 = 0;
    unsafe {
        let res = ffi::signature_import(
            alg.as_ptr(),
            alg_size,
            encoded.as_ref().as_ptr(),
            encoded_size,
            encoding,
            &mut sig_handle,
        );
        if res == 0 {
            Ok(sig_handle)
        } else {
            Err(res)
        }
    }
}

pub fn signature_export(sig_handle: u32, encoding: u16) -> Result<u32, i32> {
    let mut output_handle: u32 = 0;
    unsafe {
        let res = ffi::signature_export(sig_handle, encoding as u32, &mut output_handle);
        if res == 0 {
            Ok(output_handle)
        } else {
            Err(res)
        }
    }
}

pub fn array_output_len(output_handle: u32) -> Result<u32, i32> {
    let mut length: u32 = 0;
    unsafe {
        let res = ffi::array_output_len(output_handle, &mut length);
        if res == 0 {
            Ok(length)
        } else {
            Err(res)
        }
    }
}

pub fn array_output_pull(output_handle: u32, buf: &mut Vec<u8>) -> Result<u32, i32> {
    let mut length: u32 = 0;
    unsafe {
        let res = ffi::array_output_pull(output_handle, buf.as_mut_ptr(), 64, &mut length);
        if res == 0 {
            Ok(length)
        } else {
            Err(res)
        }
    }
}
