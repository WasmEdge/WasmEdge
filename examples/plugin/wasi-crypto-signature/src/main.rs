use wasi_crypto_example::{
    decode, encode,
    signatures::{array_output_len, array_output_pull, signature_export, signature_import},
};

fn main() {
    let code = "9D92E9FDCA3DDF2E1DDCA1E3B7A79A250B6E4AFFCABF5F9FF4D960B152AB8300E9EB978BD3DA89C42BBFE5A2C2AEB0AF1DD178FB4BCD0833B587D118F59BBB4D";
    let encoded = encode(code);

    let handle = signature_import("ECDSA_K256_SHA256".into(), &encoded, 0).unwrap();

    let export_sig_handle = signature_export(handle, 0).unwrap();

    let export_sig_size = array_output_len(export_sig_handle).unwrap();

    let mut export_sig: Vec<u8> = vec![0; export_sig_size as usize];
    let _ = array_output_pull(export_sig_handle, &mut export_sig).unwrap();

    assert_eq!(encoded, export_sig);
    dbg!(decode(encoded));
    dbg!(decode(export_sig));
}
