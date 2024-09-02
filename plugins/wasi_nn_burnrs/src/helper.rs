#[macro_export]
macro_rules! get_slice {
    ($memory:expr, $ptr:expr, $length:expr, $ty:ty) => {{
        let raw_bytes = $memory
            .data_pointer($ptr as usize, $length as usize)
            .expect("Failed to get data pointer");
        bytemuck::cast_slice::<u8, $ty>(raw_bytes)
    }};
}

pub use get_slice;
