use thiserror::Error;

#[derive(Error, Debug)]
pub enum TlsError {
    #[error("{0}")]
    Tls(#[from] rustls::Error),
    #[error("{0}")]
    IO(#[from] std::io::Error),
    #[error("ParamError")]
    ParamError,
}

impl TlsError {
    pub fn error_code(&self) -> i32 {
        match self {
            TlsError::ParamError => -1,
            TlsError::Tls(tls_err) => match tls_err {
                rustls::Error::InappropriateMessage { .. } => -2,
                rustls::Error::InappropriateHandshakeMessage { .. } => -3,
                rustls::Error::CorruptMessage => -4,
                rustls::Error::CorruptMessagePayload(_) => -5,
                rustls::Error::NoCertificatesPresented => -6,
                rustls::Error::UnsupportedNameType => -7,
                rustls::Error::DecryptError => -8,
                rustls::Error::EncryptError => -9,
                rustls::Error::PeerIncompatibleError(_) => -10,
                rustls::Error::PeerMisbehavedError(_) => -11,
                rustls::Error::AlertReceived(_) => -12,
                rustls::Error::InvalidCertificateEncoding => -13,
                rustls::Error::InvalidCertificateSignatureType => -14,
                rustls::Error::InvalidCertificateSignature => -15,
                rustls::Error::InvalidCertificateData(_) => -16,
                rustls::Error::InvalidSct(_) => -17,
                rustls::Error::General(_) => -18,
                rustls::Error::FailedToGetCurrentTime => -19,
                rustls::Error::FailedToGetRandomBytes => -20,
                rustls::Error::HandshakeNotComplete => -21,
                rustls::Error::PeerSentOversizedRecord => -22,
                rustls::Error::NoApplicationProtocol => -23,
                rustls::Error::BadMaxFragmentSize => -24,
            },
            TlsError::IO(io_err) if io_err.kind() == std::io::ErrorKind::WouldBlock => -25,
            TlsError::IO(_) => -26,
        }
    }
}

#[repr(C)]
pub struct TlsIoState {
    tls_bytes_to_write: u32,
    plaintext_bytes_to_read: u32,
    peer_has_closed: bool,
}

impl From<rustls::IoState> for TlsIoState {
    fn from(value: rustls::IoState) -> Self {
        TlsIoState {
            tls_bytes_to_write: value.tls_bytes_to_write() as u32,
            plaintext_bytes_to_read: value.plaintext_bytes_to_read() as u32,
            peer_has_closed: value.peer_has_closed(),
        }
    }
}

mod tls_client {
    use std::{
        io::{Read, Write},
        sync::Arc,
    };

    use bytes::{Buf, BufMut};
    use rustls::{OwnedTrustAnchor, RootCertStore};

    use crate::TlsError;
    use crate::TlsIoState;

    pub struct Ctx {
        pub client_configs: Vec<Option<Arc<rustls::ClientConfig>>>,
        pub client_codec: Vec<Option<ClientCodec>>,
    }

    impl Ctx {
        pub fn new() -> Ctx {
            let mut root_store = RootCertStore::empty();
            root_store.add_server_trust_anchors(webpki_roots::TLS_SERVER_ROOTS.0.iter().map(
                |ta| {
                    OwnedTrustAnchor::from_subject_spki_name_constraints(
                        ta.subject,
                        ta.spki,
                        ta.name_constraints,
                    )
                },
            ));
            let config = rustls::ClientConfig::builder()
                .with_safe_defaults()
                .with_root_certificates(root_store)
                .with_no_client_auth();

            Ctx {
                client_configs: vec![Some(Arc::new(config))],
                client_codec: Vec::with_capacity(1024),
            }
        }

        pub fn default_client_config(&mut self) -> usize {
            0
        }

        pub fn new_codec(
            &mut self,
            server_name: &str,
            config_id: usize,
        ) -> Result<usize, TlsError> {
            let config = self
                .client_configs
                .get(config_id)
                .ok_or(TlsError::ParamError)?
                .clone()
                .ok_or(TlsError::ParamError)?;

            let name = server_name.try_into().map_err(|_| TlsError::ParamError)?;
            let new_codec = rustls::ClientConnection::new(config, name)?;
            let new_codec = ClientCodec(new_codec);

            if let Some((id, item)) = self
                .client_codec
                .iter_mut()
                .enumerate()
                .find(|(_, item)| item.is_none())
            {
                debug_assert!(item.is_none());
                let _ = item.insert(new_codec);
                Ok(id)
            } else {
                let id = self.client_codec.len();
                self.client_codec.push(Some(new_codec));
                Ok(id)
            }
        }

        pub fn delete_codec(&mut self, codec_id: usize) {
            if let Some(codec) = self.client_codec.get_mut(codec_id) {
                let _ = codec.take();
            }
        }
    }

    #[derive(Debug)]
    pub struct ClientCodec(pub rustls::ClientConnection);

    impl ClientCodec {
        pub fn is_handshaking(&self) -> bool {
            self.0.is_handshaking()
        }

        pub fn process_new_packets(&mut self) -> Result<TlsIoState, TlsError> {
            Ok(self.0.process_new_packets()?.into())
        }

        pub fn send_close_notify(&mut self) {
            self.0.send_close_notify();
        }

        pub fn write_raw(&mut self, raw_buf: &[u8]) -> Result<usize, TlsError> {
            let conn = &mut self.0;
            Ok(conn.writer().write(raw_buf)?)
        }

        pub fn write_tls(&mut self, tls_buf: &mut [u8]) -> Result<usize, TlsError> {
            let conn = &mut self.0;
            Ok(conn.write_tls(&mut tls_buf.writer())?)
        }

        pub fn read_raw(&mut self, raw_buf: &mut [u8]) -> Result<usize, TlsError> {
            let conn = &mut self.0;
            Ok(conn.reader().read(raw_buf)?)
        }

        pub fn read_tls(&mut self, tls_buf: &[u8]) -> Result<usize, TlsError> {
            let conn = &mut self.0;
            Ok(conn.read_tls(&mut tls_buf.reader())?)
        }
    }

    #[cfg(test)]
    mod tls_client_test {
        use super::*;
        #[test]
        fn test_ctx() {
            let mut ctx = Ctx::new();
            let config_id = ctx.default_client_config();
            assert_eq!(config_id, 0);

            let codec_id_0 = ctx.new_codec("httpbin.org", config_id).unwrap();
            assert_eq!(codec_id_0, 0);
            let codec_id_1 = ctx.new_codec("httpbin.org", config_id).unwrap();
            assert_eq!(codec_id_1, 1);
            ctx.delete_codec(codec_id_0);
            println!("{:?}", ctx.client_codec);
            let codec_id_0 = ctx.new_codec("httpbin.org", config_id).unwrap();
            assert_eq!(codec_id_0, 0);
        }
    }
}

mod wasmedge_client_plugin {

    use wasmedge_plugin_sdk::{
        error::CoreError,
        memory::Memory,
        module::{PluginModule, SyncInstanceRef},
        types::{ValType, WasmVal},
    };

    use crate::{tls_client::*, TlsError};

    macro_rules! match_value {
        ($expression:expr, $t:path, $error:expr) => {
            match $expression {
                $t(v) => v,
                _ => return Err($error),
            }
        };
    }

    fn default_config(
        _inst: &mut SyncInstanceRef,
        _memory: &mut Memory,
        ctx: &mut Ctx,
        _args: Vec<WasmVal>,
    ) -> Result<Vec<WasmVal>, CoreError> {
        let config_id = ctx.default_client_config();
        Ok(vec![WasmVal::I32(config_id as i32)])
    }

    fn new_client_codec(
        _inst: &mut SyncInstanceRef,
        memory: &mut Memory,
        ctx: &mut Ctx,
        args: Vec<WasmVal>,
    ) -> Result<Vec<WasmVal>, CoreError> {
        #[inline]
        fn new_client_codec_inner(
            memory: &mut Memory,
            ctx: &mut Ctx,
            args: Vec<WasmVal>,
        ) -> Result<WasmVal, TlsError> {
            let config_id = args[0].clone();
            let server_ptr = args[1].clone();
            let server_len = args[2].clone();

            if let (WasmVal::I32(config_id), WasmVal::I32(server_ptr), WasmVal::I32(server_len)) =
                (config_id, server_ptr, server_len)
            {
                let server_name = memory.data_pointer(server_ptr as usize, server_len as usize);
                let server_name = server_name
                    .and_then(|bs| std::str::from_utf8(bs).ok())
                    .ok_or(TlsError::ParamError)?;
                let r = ctx.new_codec(server_name, config_id as usize)?;
                Ok(WasmVal::I32(r as i32))
            } else {
                Err(TlsError::ParamError)
            }
        }
        match new_client_codec_inner(memory, ctx, args) {
            Ok(ok) => Ok(vec![ok]),
            Err(e) => Ok(vec![WasmVal::I32(e.error_code())]),
        }
    }

    fn is_handshaking(
        _inst: &mut SyncInstanceRef,
        memory: &mut Memory,
        ctx: &mut Ctx,
        args: Vec<WasmVal>,
    ) -> Result<Vec<WasmVal>, CoreError> {
        #[inline]
        fn is_handshaking_inner(
            _memory: &mut Memory,
            ctx: &mut Ctx,
            args: Vec<WasmVal>,
        ) -> Result<WasmVal, TlsError> {
            let codec_id = match_value!(args[0].clone(), WasmVal::I32, TlsError::ParamError);
            let codec = ctx
                .client_codec
                .get(codec_id as usize)
                .ok_or(TlsError::ParamError)?
                .as_ref()
                .ok_or(TlsError::ParamError)?;

            if codec.is_handshaking() {
                Ok(WasmVal::I32(1))
            } else {
                Ok(WasmVal::I32(0))
            }
        }

        match is_handshaking_inner(memory, ctx, args) {
            Ok(ok) => Ok(vec![ok]),
            Err(e) => Ok(vec![WasmVal::I32(e.error_code())]),
        }
    }

    fn wants(
        _inst: &mut SyncInstanceRef,
        memory: &mut Memory,
        ctx: &mut Ctx,
        args: Vec<WasmVal>,
    ) -> Result<Vec<WasmVal>, CoreError> {
        #[inline]
        fn wants_inner(
            _memory: &mut Memory,
            ctx: &mut Ctx,
            args: Vec<WasmVal>,
        ) -> Result<WasmVal, TlsError> {
            let codec_id = match_value!(args[0].clone(), WasmVal::I32, TlsError::ParamError);
            let codec = ctx
                .client_codec
                .get(codec_id as usize)
                .ok_or(TlsError::ParamError)?
                .as_ref()
                .ok_or(TlsError::ParamError)?;
            match (codec.0.wants_write(), codec.0.wants_read()) {
                (true, true) => Ok(WasmVal::I32(0b11)),
                (true, false) => Ok(WasmVal::I32(0b10)),
                (false, true) => Ok(WasmVal::I32(0b01)),
                (false, false) => Ok(WasmVal::I32(0)),
            }
        }

        match wants_inner(memory, ctx, args) {
            Ok(ok) => Ok(vec![ok]),
            Err(e) => Ok(vec![WasmVal::I32(e.error_code())]),
        }
    }

    fn delete_codec(
        _inst: &mut SyncInstanceRef,
        memory: &mut Memory,
        ctx: &mut Ctx,
        args: Vec<WasmVal>,
    ) -> Result<Vec<WasmVal>, CoreError> {
        #[inline]
        fn delete_codec_inner(
            _memory: &mut Memory,
            ctx: &mut Ctx,
            args: Vec<WasmVal>,
        ) -> Result<WasmVal, TlsError> {
            let codec_id = match_value!(args[0].clone(), WasmVal::I32, TlsError::ParamError);
            ctx.delete_codec(codec_id as usize);
            Ok(WasmVal::I32(0))
        }

        match delete_codec_inner(memory, ctx, args) {
            Ok(ok) => Ok(vec![ok]),
            Err(e) => Ok(vec![WasmVal::I32(e.error_code())]),
        }
    }

    fn process_new_packets(
        _inst: &mut SyncInstanceRef,
        memory: &mut Memory,
        ctx: &mut Ctx,
        args: Vec<WasmVal>,
    ) -> Result<Vec<WasmVal>, CoreError> {
        #[inline]
        fn process_new_packets_inner(
            memory: &mut Memory,
            ctx: &mut Ctx,
            args: Vec<WasmVal>,
        ) -> Result<WasmVal, TlsError> {
            let codec_id = match_value!(args[0].clone(), WasmVal::I32, TlsError::ParamError);
            let result_ptr = match_value!(args[1].clone(), WasmVal::I32, TlsError::ParamError);

            let codec = ctx
                .client_codec
                .get_mut(codec_id as usize)
                .ok_or(TlsError::ParamError)?
                .as_mut()
                .ok_or(TlsError::ParamError)?;
            let io_state = codec.process_new_packets()?;

            memory
                .write_data((result_ptr as usize).into(), io_state)
                .ok_or(TlsError::ParamError)?;

            Ok(WasmVal::I32(0 as i32))
        }
        match process_new_packets_inner(memory, ctx, args) {
            Ok(ok) => Ok(vec![ok]),
            Err(e) => Ok(vec![WasmVal::I32(e.error_code())]),
        }
    }

    fn send_close_notify(
        _inst: &mut SyncInstanceRef,
        memory: &mut Memory,
        ctx: &mut Ctx,
        args: Vec<WasmVal>,
    ) -> Result<Vec<WasmVal>, CoreError> {
        #[inline]
        fn send_close_notify_inner(
            _memory: &mut Memory,
            ctx: &mut Ctx,
            args: Vec<WasmVal>,
        ) -> Result<WasmVal, TlsError> {
            let codec_id = match_value!(args[0].clone(), WasmVal::I32, TlsError::ParamError);
            let codec = ctx
                .client_codec
                .get_mut(codec_id as usize)
                .ok_or(TlsError::ParamError)?
                .as_mut()
                .ok_or(TlsError::ParamError)?;
            codec.send_close_notify();
            Ok(WasmVal::I32(0))
        }

        match send_close_notify_inner(memory, ctx, args) {
            Ok(ok) => Ok(vec![ok]),
            Err(e) => Ok(vec![WasmVal::I32(e.error_code())]),
        }
    }

    fn write_raw(
        _inst: &mut SyncInstanceRef,
        memory: &mut Memory,
        ctx: &mut Ctx,
        args: Vec<WasmVal>,
    ) -> Result<Vec<WasmVal>, CoreError> {
        #[inline]
        fn write_raw_inner(
            memory: &mut Memory,
            ctx: &mut Ctx,
            args: Vec<WasmVal>,
        ) -> Result<WasmVal, TlsError> {
            let codec_id = match_value!(args[0].clone(), WasmVal::I32, TlsError::ParamError);
            let raw_buf_ptr = match_value!(args[1].clone(), WasmVal::I32, TlsError::ParamError);
            let raw_len = match_value!(args[2].clone(), WasmVal::I32, TlsError::ParamError);

            let codec = ctx
                .client_codec
                .get_mut(codec_id as usize)
                .ok_or(TlsError::ParamError)?
                .as_mut()
                .ok_or(TlsError::ParamError)?;

            let raw_buf = memory
                .data_pointer(raw_buf_ptr as usize, raw_len as usize)
                .ok_or(TlsError::ParamError)?;

            let n = codec.write_raw(raw_buf)?;
            Ok(WasmVal::I32(n as i32))
        }
        match write_raw_inner(memory, ctx, args) {
            Ok(ok) => Ok(vec![ok]),
            Err(e) => Ok(vec![WasmVal::I32(e.error_code())]),
        }
    }

    fn write_tls(
        _inst: &mut SyncInstanceRef,
        memory: &mut Memory,
        ctx: &mut Ctx,
        args: Vec<WasmVal>,
    ) -> Result<Vec<WasmVal>, CoreError> {
        #[inline]
        fn write_tls_inner(
            memory: &mut Memory,
            ctx: &mut Ctx,
            args: Vec<WasmVal>,
        ) -> Result<WasmVal, TlsError> {
            let codec_id = match_value!(args[0].clone(), WasmVal::I32, TlsError::ParamError);
            let tls_buf_ptr = match_value!(args[1].clone(), WasmVal::I32, TlsError::ParamError);
            let tls_len = match_value!(args[2].clone(), WasmVal::I32, TlsError::ParamError);

            let codec = ctx
                .client_codec
                .get_mut(codec_id as usize)
                .ok_or(TlsError::ParamError)?
                .as_mut()
                .ok_or(TlsError::ParamError)?;

            let raw_buf = memory
                .data_pointer_mut(tls_buf_ptr as usize, tls_len as usize)
                .ok_or(TlsError::ParamError)?;

            let n = codec.write_tls(raw_buf)?;
            Ok(WasmVal::I32(n as i32))
        }
        match write_tls_inner(memory, ctx, args) {
            Ok(ok) => Ok(vec![ok]),
            Err(e) => Ok(vec![WasmVal::I32(e.error_code())]),
        }
    }

    fn read_raw(
        _inst: &mut SyncInstanceRef,
        memory: &mut Memory,
        ctx: &mut Ctx,
        args: Vec<WasmVal>,
    ) -> Result<Vec<WasmVal>, CoreError> {
        #[inline]
        fn read_raw_inner(
            memory: &mut Memory,
            ctx: &mut Ctx,
            args: Vec<WasmVal>,
        ) -> Result<WasmVal, TlsError> {
            let codec_id = match_value!(args[0].clone(), WasmVal::I32, TlsError::ParamError);
            let raw_buf_ptr = match_value!(args[1].clone(), WasmVal::I32, TlsError::ParamError);
            let raw_len = match_value!(args[2].clone(), WasmVal::I32, TlsError::ParamError);

            let codec = ctx
                .client_codec
                .get_mut(codec_id as usize)
                .ok_or(TlsError::ParamError)?
                .as_mut()
                .ok_or(TlsError::ParamError)?;

            let raw_buf = memory
                .data_pointer_mut(raw_buf_ptr as usize, raw_len as usize)
                .ok_or(TlsError::ParamError)?;

            let n = codec.read_raw(raw_buf);
            let n = n?;
            Ok(WasmVal::I32(n as i32))
        }
        match read_raw_inner(memory, ctx, args) {
            Ok(ok) => Ok(vec![ok]),
            Err(e) => Ok(vec![WasmVal::I32(e.error_code())]),
        }
    }

    fn read_tls(
        _inst: &mut SyncInstanceRef,
        memory: &mut Memory,
        ctx: &mut Ctx,
        args: Vec<WasmVal>,
    ) -> Result<Vec<WasmVal>, CoreError> {
        #[inline]
        fn read_tls_inner(
            memory: &mut Memory,
            ctx: &mut Ctx,
            args: Vec<WasmVal>,
        ) -> Result<WasmVal, TlsError> {
            let codec_id = match_value!(args[0].clone(), WasmVal::I32, TlsError::ParamError);
            let tls_buf_ptr = match_value!(args[1].clone(), WasmVal::I32, TlsError::ParamError);
            let tls_len = match_value!(args[2].clone(), WasmVal::I32, TlsError::ParamError);

            let codec = ctx
                .client_codec
                .get_mut(codec_id as usize)
                .ok_or(TlsError::ParamError)?
                .as_mut()
                .ok_or(TlsError::ParamError)?;

            let raw_buf = memory
                .data_pointer(tls_buf_ptr as usize, tls_len as usize)
                .ok_or(TlsError::ParamError)?;

            let n = codec.read_tls(raw_buf)?;
            Ok(WasmVal::I32(n as i32))
        }
        match read_tls_inner(memory, ctx, args) {
            Ok(ok) => Ok(vec![ok]),
            Err(e) => Ok(vec![WasmVal::I32(e.error_code())]),
        }
    }

    pub fn create_module() -> PluginModule<Ctx> {
        let mut module = PluginModule::create("rustls_client", Ctx::new()).unwrap();
        module
            .add_func(
                "default_config",
                (vec![], vec![ValType::I32]),
                default_config,
            )
            .unwrap();

        module
            .add_func(
                "new_codec",
                (
                    vec![ValType::I32, ValType::I32, ValType::I32],
                    vec![ValType::I32],
                ),
                new_client_codec,
            )
            .unwrap();

        module
            .add_func(
                "codec_is_handshaking",
                (vec![ValType::I32], vec![ValType::I32]),
                is_handshaking,
            )
            .unwrap();

        module
            .add_func(
                "codec_wants",
                (vec![ValType::I32], vec![ValType::I32]),
                wants,
            )
            .unwrap();

        module
            .add_func(
                "delete_codec",
                (vec![ValType::I32], vec![ValType::I32]),
                delete_codec,
            )
            .unwrap();

        module
            .add_func(
                "send_close_notify",
                (vec![ValType::I32], vec![ValType::I32]),
                send_close_notify,
            )
            .unwrap();

        module
            .add_func(
                "process_new_packets",
                (vec![ValType::I32, ValType::I32], vec![ValType::I32]),
                process_new_packets,
            )
            .unwrap();

        module
            .add_func(
                "write_raw",
                (
                    vec![
                        ValType::I32, //codec_id
                        ValType::I32, // buf
                        ValType::I32, // buf_len
                    ],
                    vec![ValType::I32],
                ),
                write_raw,
            )
            .unwrap();

        module
            .add_func(
                "write_tls",
                (
                    vec![
                        ValType::I32, //codec_id
                        ValType::I32, // buf
                        ValType::I32, // buf_len
                    ],
                    vec![ValType::I32],
                ),
                write_tls,
            )
            .unwrap();

        module
            .add_func(
                "read_raw",
                (
                    vec![
                        ValType::I32, //codec_id
                        ValType::I32, // buf
                        ValType::I32, // buf_len
                    ],
                    vec![ValType::I32],
                ),
                read_raw,
            )
            .unwrap();

        module
            .add_func(
                "read_tls",
                (
                    vec![
                        ValType::I32, //codec_id
                        ValType::I32, // buf
                        ValType::I32, // buf_len
                    ],
                    vec![ValType::I32],
                ),
                read_tls,
            )
            .unwrap();

        module
    }
}

use wasmedge_client_plugin::create_module;

wasmedge_plugin_sdk::plugin::register_plugin!(
    plugin_name="rustls",
    plugin_description="rustls plugin",
    version=(0,0,1,0),
    modules=[
        {"rustls_client","rustls client module",create_module}
    ]
);
