;; SPDX-License-Identifier: Apache-2.0
;; SPDX-FileCopyrightText: 2019-2024 Second State INC

(module
  (import "module_wasm_name" "get_string" (func $get_string (param i32 i32 i32)))
  (import "wasi_snapshot_preview1" "fd_write" (func $fd_write (param i32 i32 i32 i32) (result i32)))
  (import "wasi_snapshot_preview1" "proc_exit" (func $proc_exit (param i32)))
  (memory 1 1)
  (func (export "_start")
    ;; address layout
    ;; 8 -> fd_write result
    ;; 16 -> ciovec array with one element
    ;;   16 -> pointer to buffer
    ;;   20 -> buffer size
    ;; 24 -> buffer with size 64
    ;; call get_string
    ;; 24 -> pointer to buffer
    ;; 64 -> buffer size
    ;; 20 -> pointer to written
    (call $get_string (i32.const 24) (i32.const 64) (i32.const 20))
    ;; set ciovec buf
    (i32.store (i32.const 16) (i32.const 24))
    ;; call fd_write
    ;; 1 -> stdout
    ;; 16 -> pointer to ciovec
    ;; 1 -> ciovec count
    ;; 8 -> pointer to result
    (drop (call $fd_write (i32.const 1) (i32.const 16) (i32.const 1) (i32.const 8)))
    ;; call proc_exit
    ;; 0 -> successful exit
    (call $proc_exit (i32.const 0))
  )
)
