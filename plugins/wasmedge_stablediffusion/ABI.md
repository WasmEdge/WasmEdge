# WasmEdge Stable Diffusion Plug-in: Guest ABI

This document is the contract between the `wasmedge_stablediffusion` plug-in
and a WebAssembly guest. It describes ABI version 2 (plug-in version 0.5.0).
The plug-in generates images with stable-diffusion.cpp ("the library").
`DESIGN.md` describes the internals and the duties of the embedder.

## 1. The module

- The host module name is `wasmedge_stablediffusion`. It has six functions.
- Every parameter and result is an `i32`, read as an unsigned 32-bit number.
- A pointer is an offset into memory 0 of the calling module.
- Every multi-byte value in that memory is little-endian.
- Every call is synchronous. `generate` returns when the images are complete.

Call `abi_version()` first. It returns 2. If the plug-in is not installed,
WasmEdge registers a stand-in module: its `abi_version` returns 0, and its
other functions return 1 (`InvalidArgument`) and do nothing. If
`abi_version()` does not return 2, do not call the other functions.

## 2. Functions

| Function | Parameters | Result |
|---|---|---|
| `abi_version` | none | 2 |
| `session_open` | `config_ptr, config_len, session_ptr` | an error code |
| `session_close` | `session` | an error code |
| `generate` | `session, request_ptr, request_len, blobs_ptr, blobs_len, count_ptr` | an error code |
| `result_info` | `session, index, format, info_ptr` | an error code |
| `result_read` | `session, index, format, buf_ptr, buf_len, written_ptr` | an error code |

- `config` and `request` are UTF-8 JSON objects (section 4).
- `blobs` is a table of input images (section 7.1).
- `format` is 0 for PNG and 1 for raw RGB (section 7.2).
- Section 3 lists the error codes.

### 2.1 Pointers and lengths

- A pointer and a length name the bytes `[ptr, ptr + len)`. The range must be
  inside the memory, also when `len` is 0. If it is not, the call returns
  `MissingMemory`.
- If there is nothing to point to, pass 0 as the pointer.
- The plug-in writes fixed-size values at `session_ptr` (4 bytes),
  `count_ptr` (4 bytes), `info_ptr` (16 bytes), and `written_ptr` (4 bytes).
  The same rule applies.
- No alignment is necessary.
- A failed call writes nothing. The only exception: `result_read` with
  `BufferTooSmall` writes the necessary size at `written_ptr`.
- If the calling module has no memory, every function that uses memory
  returns `MissingMemory`.

### 2.2 Session ids

- A session id is an opaque `u32`. 0 is a valid id.
- An id is valid only in the module instance that created it. Each VM has
  its own module instance.
- A module instance never uses an id twice. A closed id always returns
  `NotFound`. After 4,294,967,295 ids, `session_open` returns `RuntimeError`.
- A module instance has 4 sessions at most. A session keeps its models in
  memory from its first `generate` until it closes. Close the sessions that
  you do not use.

### 2.3 `session_open(config_ptr, config_len, session_ptr)`

Reads the configuration (`config_len` bytes of JSON at `config_ptr`, section
4.2) and resolves its files (section 6). Then it creates the library context
and writes the session id as a `u32` at `session_ptr`.

- With 4 sessions, the call returns `Busy` and reads nothing. A
  `session_open` in progress on another thread counts as a session.
- With a `config_len` above 65,536, the call returns `InvalidArgument` and
  reads nothing.
- The call checks `session_ptr` before it parses the configuration.
- A failed call creates no session and writes nothing.
- The library loads lazily. `session_open` returns `LoadFailed` for a file
  that does not resolve (section 6.1), an embedding that breaks section 6.4,
  a model that the library does not recognize, and an upscaler that does not
  load. The first `generate` reads the weights and can also return
  `LoadFailed`.

Codes: `Success`, `InvalidArgument`, `InvalidEncoding`, `MissingMemory`,
`Busy`, `LoadFailed`, `NotReadOnly`, `RuntimeError`.

### 2.4 `session_close(session)`

Removes the id immediately. Every later call with this id returns `NotFound`.

- `session_close` never waits for a call on the same session. A call in
  progress completes, and the plug-in frees the models and results when that
  call returns. Until then, the session is one of the 4 of section 2.2.
- If no call uses the session, `session_close` frees the models before it
  returns. For this, it can wait for a generation of a different session
  (section 2.8).

Codes: `Success`, and `NotFound` for an unknown or closed id.

### 2.5 `generate(session, request_ptr, request_len, blobs_ptr, blobs_len, count_ptr)`

Reads the request (`request_len` bytes of JSON at `request_ptr`, section 4.3)
and the blob table (`blobs_len` entries at `blobs_ptr`, section 7.1). Then it
generates the images, replaces the results of the session, and writes the
image count as a `u32` at `count_ptr`.

**Phase 1: checks.** A failure returns its code and does not change the
session: the previous results stay. The checks run in this order:

1. `request_len` is 65,536 or less.
2. The request is inside the memory.
3. `blobs_len` is 8 or less.
4. The blob table and every blob are inside the memory.
5. `count_ptr` is inside the memory.
6. The request obeys section 4.
7. The session exists.
8. The session can serve the request (LoRAs, `control_image`,
   `upscale_repeats`).
9. The LoRA paths resolve (section 6).
10. The input images that the request uses decode (section 7.1).

Thus, on an unknown session, an invalid request returns its own code, but an
image that does not decode returns `NotFound`.

**Phase 2: generation.** The session drops its previous results. Then the
plug-in:

1. refuses a prompt that breaks section 4.4 (`InvalidArgument`);
2. refuses a LoRA file that breaks section 6.4 (`LoadFailed`);
3. runs the library and upscales.

On success, the session holds the new results, and the plug-in writes the
count. On failure, the session holds no results.

Codes: `Success`, `InvalidArgument`, `InvalidEncoding`, `MissingMemory`,
`NotFound`, `LoadFailed`, `NotReadOnly`, `RuntimeError`.

### 2.6 `result_info(session, index, format, info_ptr)`

Writes four `u32` at `info_ptr`: the width, the height, the channel count
(always 3), and the size. The size is the number of bytes that `result_read`
writes for this index and format. The plug-in encodes a PNG at the first
request and keeps it.

- A `format` other than 0 or 1 is `InvalidArgument`.
- An index with no result is `NotFound`. This applies to every index if the
  session has no results or is poisoned.
- A failed PNG encoding is `RuntimeError`.

Codes: `Success`, `InvalidArgument`, `MissingMemory`, `NotFound`,
`RuntimeError`.

### 2.7 `result_read(session, index, format, buf_ptr, buf_len, written_ptr)`

Copies one result to `buf_ptr` and writes its size at `written_ptr`.

- The buffer `[buf_ptr, buf_ptr + buf_len)` and `written_ptr` must be inside
  the memory, for every result size.
- The buffer and the four bytes at `written_ptr` must not overlap
  (`InvalidArgument`). An empty buffer overlaps nothing.
- If `buf_len` is smaller than the result, the call returns `BufferTooSmall`,
  writes the result size at `written_ptr`, and does not change the buffer.
  This is not a fault: the host logs it at `debug`. To get the size, call
  `result_read` with a `buf_len` of 0.
- `format` and `index` have the same checks as in `result_info`.

Codes: `Success`, `InvalidArgument`, `MissingMemory`, `BufferTooSmall`,
`NotFound`, `RuntimeError`.

### 2.8 Calls from several threads

- `generate`, `result_info`, and `result_read` on one session run one at a
  time. A second call waits. No call on a session returns `Busy`.
- A `generate` that waits decodes its input images after the wait (check 10
  of section 2.5). Thus only one call of a session holds decoded images.
- Only one library call runs at a time in the process, for all sessions and
  all VMs. Thus generations run one after the other, and `session_open` can
  wait for a generation.
- Every operation that frees the models of a session waits for a generation
  in progress: the last call on a closed session, `session_close` on a
  session that no call uses, and the destruction of a module instance.
- `result_info` and `result_read` do not call the library. They wait only for
  calls on their own session.

### 2.9 Session states

A session is `Ready` (with or without results) or `Poisoned`. A session
becomes `Poisoned` when a library failure leaves the library state unknown.
No known input causes this. A poisoned session keeps its models until it
closes. Close it and open a new session.

| Call | From | Code | Afterwards |
|---|---|---|---|
| `session_open` succeeds | | `Success` | a new session, `Ready`, no results |
| `session_open` fails | | any other code | no session |
| `generate` fails in phase 1 | `Ready`, `Poisoned` | `InvalidArgument`, `InvalidEncoding`, `MissingMemory`, `LoadFailed`, `NotReadOnly` | unchanged; the previous results stay |
| `generate` succeeds | `Ready` | `Success` | `Ready` with the new results |
| `generate` fails in phase 2 | `Ready` | `InvalidArgument`, `LoadFailed`, `RuntimeError` | `Ready`, no results |
| `generate` leaves the library state unknown | `Ready` | `RuntimeError` | `Poisoned`, no results |
| `generate` passes phase 1 | `Poisoned` | `RuntimeError` | `Poisoned` |
| `result_info`, `result_read` | `Ready` with results | `Success`, `BufferTooSmall`, or an error of section 2.6 or 2.7 | unchanged |
| `result_info`, `result_read` | `Ready` without results, `Poisoned` | `NotFound`; `InvalidArgument` for a bad `format` | unchanged |
| `session_close` | any | `Success` | closed |
| any call with an unknown or closed id | | `NotFound` | |

`InvalidArgument` and `LoadFailed` can come from both phases. Thus the code
does not tell if the previous results stayed. Read the results that you need
before the next `generate`, or call `result_info` after it.

When a module instance is destroyed (with its VM), the plug-in closes all its
sessions.

## 3. Error codes

| Code | Name | Meaning |
|---|---|---|
| 0 | `Success` | The call completed. |
| 1 | `InvalidArgument` | A text above 65,536 bytes, or more than 8 blobs. A JSON value that breaks section 4. A request that the session cannot serve (section 4.3). An input image above the limits of section 7.1. A prompt that breaks section 4.4. A `format` other than 0 or 1. A value that the library refuses. |
| 2 | `InvalidEncoding` | The text is not UTF-8 or not a JSON object, or the JSON parser refuses it (section 4.1). An input image is not a PNG or JPEG, or does not decode. |
| 3 | `MissingMemory` | The calling module has no memory, or a range is outside it (section 2.1). |
| 4 | `Busy` | `session_open` only: the module instance already has 4 sessions (section 2.3). |
| 5 | `RuntimeError` | The library failed. The session is poisoned (section 2.9). A PNG encoding failed. The module instance has no more session ids. The plug-in could not create its backend, or its names do not agree with the library (a build defect of the host). |
| 6 | `BufferTooSmall` | `result_read` only: the buffer is smaller than the result. The necessary size is at `written_ptr`. |
| 7 | `NotFound` | No open session has this id, or no result has this index. |
| 8 | `LoadFailed` | A path does not resolve to one regular file that the guest can read (section 6.1). A LoRA or embedding breaks section 6.4. The library cannot load a file, or does not recognize it as a model. |
| 9 | `NotReadOnly` | A path names a file that the guest could change, or the plug-in cannot tell (section 6.2). The embedder must correct this. |

The host logs every failure in one line with the function and the code. If a
rule refused something, a second line gives the reason. Prompt text never
goes to the host log.

## 4. Configuration and request

### 4.1 Rules for both texts

- The text has 65,536 bytes at most. It is valid UTF-8 and one JSON object.
- These are `InvalidArgument`, also in nested objects: an unknown key, a
  repeated key, a missing required key, and a value of the wrong type.
- A string must not contain NUL (`\u0000`).
- A path is a string that is not empty (section 6).
- An *integer* is a JSON number with no fraction and no exponent: `20`, not
  `20.0` or `2e1`.
- A *number* is any JSON number: `7`, `7.5`, and `7.5e0`.
- A *boolean* is `true` or `false`.
- If the JSON parser refuses the text, the code is `InvalidEncoding`. This
  includes two cases of valid JSON: a number too large for a 64-bit float
  (such as `1e400`), and more than 1,024 nesting levels. An integer that does
  not fit in 64 bits is `InvalidArgument`.

### 4.2 Session configuration

| Key | Type | Rule | When omitted |
|---|---|---|---|
| `model_path` | path | the model file | required unless `diffusion_model_path` is given |
| `diffusion_model_path` | path | a diffusion model in a separate file, for models that come in parts | |
| `clip_l_path`, `clip_g_path`, `t5xxl_path` | path | text encoders in separate files | |
| `vae_path` | path | a VAE that replaces the VAE of the model | |
| `taesd_path` | path | a TAESD decoder | |
| `control_net_path` | path | a ControlNet; the session then accepts `control_image` | |
| `upscaler_model_path` | path | an upscaler model, loaded by `session_open`; the session then accepts `upscale_repeats` | |
| `embeddings` | array of `{"name": string, "path": path}` | 64 entries at most; `name` is not empty; each file obeys section 6.4 | no embeddings |
| `n_threads` | integer | 1 to 1024; a value above the hardware thread count is reduced to that count | the number of physical cores |
| `weight_type` | string | a name of section 5; with a quantized type (all except `f32`, `f16`, `bf16`), the session accepts no LoRAs (section 6.4) | the library chooses |
| `rng` | string | a name of section 5 | the library default |
| `flash_attention` | boolean | `true` enables flash attention | off |
| `device` | string | `auto`: the library chooses; `cpu`: everything runs on the CPU | `auto` |
| `cpu_modules` | array of strings | names of section 5, each once at most; these parts stay on the CPU when `device` is `auto`; ignored when `device` is `cpu` | none |

### 4.3 Generation request

| Key | Type | Rule | When omitted |
|---|---|---|---|
| `prompt` | string | required; can be empty; see section 4.4 | |
| `negative_prompt` | string | see section 4.4 | empty |
| `width`, `height` | integer | 64 to 8192, a multiple of 64; `width * height` is 33,554,432 (2^25) at most | 512 |
| `seed` | integer | a signed 64-bit integer; with a negative seed, the library uses a random seed | random |
| `steps` | integer | 1 to 1000; some schedulers need 2 or more (below) | 20 |
| `cfg_scale` | number | 0 to 1000: the classifier-free guidance scale | the library default |
| `guidance` | number | 0 to 1000: the distilled guidance, for models that use it | the library default |
| `control_strength` | number | 0 to 100: the strength of the ControlNet | the library default |
| `strength` | number | 0 to 1: the image-to-image strength, used with `init_image` | the library default |
| `sampler`, `scheduler` | string | names of section 5 | the library chooses |
| `clip_skip` | integer | 1 to 12: the layer of the CLIP text encoder that gives the output, counted from the last layer (1) | the model default |
| `batch_count` | integer | 1 to 16: the number of images | 1 |
| `init_image` | `{"blob": n}` | the input image for image-to-image | none |
| `mask_image` | `{"blob": n}` | an inpainting mask; needs `init_image` | none |
| `control_image` | `{"blob": n}` | the input of the ControlNet; needs a session with `control_net_path` | none |
| `loras` | array of `{"path": path, "multiplier": number}` | 16 entries at most; `multiplier` is -100 to 100, and 1 when omitted; needs a session whose `weight_type` is not a quantized type; see section 6.4 | none |
| `vae_tiling` | boolean | `true` decodes the image in tiles | off |
| `upscale_repeats` | integer | 1 to 3; needs a session with `upscaler_model_path`; each upscaled side is 8192 at most, and the upscaled size is 33,554,432 pixels at most | no upscaling |

- An image object has exactly one key, `blob`: an integer below `blobs_len`.
- LoRAs apply only to the request that names them.
- With the scheduler `karras`, `exponential`, or `gits`, `steps` must be 2 or
  more (`InvalidArgument`).
- If the request names no `scheduler`, the library chooses one. On a model
  with EDM-V prediction (some SDXL-family models), it chooses `exponential`,
  so `steps` must also be 2 or more. The plug-in cannot check this. A request
  with one step then fails in phase 2 with `RuntimeError`.

### 4.4 Prompts and embeddings

- `prompt` and `negative_prompt` together have 1,500 bytes at most.
- Prompt text cannot name files. A prompt that contains the bytes
  `<embedding:` or `<image:` is `InvalidArgument`, also after a backslash.
- To use an embedding, configure it in the session (`embeddings`) and write
  its name in a prompt. The library matches the name as a whole word, in any
  letter case. Text directly before a name can prevent the match.
- Each mention of an embedding adds one token for each row of its file. The
  two prompts of a request can mention 1,024 rows at most
  (`InvalidArgument`). The plug-in counts every whole-word occurrence of a
  configured name, in any letter case, also between `<` and `>`. The row
  count of a file is the first dimension of its largest two-dimensional
  tensor, or 1.
- LoRAs come only from `loras`. `<lora:...>` in a prompt is plain text.

## 5. Name tables

Names are case-sensitive. They belong to this ABI: the plug-in maps each name
to the spelling of the library.

| Key | Names |
|---|---|
| `sampler` (21) | `euler`, `euler_a`, `heun`, `dpm2`, `dpm++2s_a`, `dpm++2m`, `dpm++2mv2`, `ipndm`, `ipndm_v`, `lcm`, `ddim_trailing`, `tcd`, `res_multistep`, `res_2s`, `er_sde`, `euler_cfg_pp`, `euler_a_cfg_pp`, `euler_ge`, `dpm++2m_sde`, `dpm++2m_sde_bt`, `lms` |
| `scheduler` (17) | `discrete`, `karras`, `exponential`, `ays`, `gits`, `sgm_uniform`, `simple`, `smoothstep`, `kl_optimal`, `lcm`, `bong_tangent`, `ltx2`, `logit_normal`, `flux2`, `flux`, `beta`, `llada_image` |
| `weight_type` (13) | `f32`, `f16`, `bf16`, `q8_0`, `q6_k`, `q5_k`, `q5_1`, `q5_0`, `q4_k`, `q4_1`, `q4_0`, `q3_k`, `q2_k` |
| `rng` (3) | `std_default`, `cuda`, `cpu` |
| `device` (2) | `auto`, `cpu` |
| `cpu_modules` (3) | `text_encoder`, `vae`, `control_net` |

The sampler `tcd` always runs with an `eta` of 0. The ABI has no key for
`eta`.

## 6. Files

### 6.1 Guest paths

Every path in the configuration and the request is a guest path, such as
`/models/x`. The plug-in resolves it through the WASI environment of the
calling module, as for the file calls of the guest.

- The calling module must have a WASI environment. It has one when it imports
  from `wasi_snapshot_preview1`. Without one, every path is `LoadFailed`.
- The plug-in matches the path against the preopen names of the guest
  (`--dir guest:host`):
  - It first drops every leading `/` and `./`. Thus `/models/x`, `models/x`,
    and `./models/x` all match a preopen named `/models`.
  - The longest preopen name that ends at a component boundary wins. A
    preopen named `.` or `/` matches every path.
  - For two preopens with the same name, the higher descriptor number wins.
    This is the later `--dir`, unless the guest renumbered them.
  - A preopen that the guest closed does not match.
- Write a plain path: `/models/x` or `models/x`. A relative path starts at
  the initial working directory of the guest. A path that starts with `./`
  can name a different file for wasi-libc, which drops one leading `./` at
  most.
- WASI opens the remaining path inside the preopen. It follows `..` and
  symbolic links, but never out of the preopen.
- The path must name one regular file that the guest can read. These are
  `LoadFailed`: a path that matches no preopen, a directory, a path that ends
  in `/`, a file that does not exist, and a model that is a directory or a
  set of shard files.

The plug-in resolves the files of the configuration at `session_open` and the
LoRA files at each `generate`. It keeps no descriptor and makes no copy. The
library opens the file by its host path, which the guest never sees.

### 6.2 Read-only files and `NotReadOnly`

The plug-in uses a file only if the guest could never change it. It decides
this from the start state of the WASI environment. If the guest later closes
a writable preopen, drops its rights, or renumbers it, the files below it are
still refused.

A file is `NotReadOnly` in each of these cases:

1. The file is inside a directory that the guest got writable (a `--dir`
   without `:readonly`). This includes a read-only preopen inside a writable
   preopen, and one directory preopened both writable and read-only. On
   Linux, it also includes the same files mounted at a second place inside a
   writable preopen.
2. The file is the standard output or standard error of the guest. After the
   guest closes such a stream, the plug-in cannot tell which file it was.
   Then it refuses a file that changed after the module instance of the
   plug-in was created.
3. A writable preopen is on the file system of the file, or (on Linux) has a
   mount of that file system at or below it, and one of these is true:
   - the file has more than one name (a hard link);
   - the file changed after the module instance of the plug-in was created.
4. On Linux, a writable preopen is on an automounted (`autofs`) file system
   or has an automount point at or below it. Then every file is refused.
5. The plug-in cannot find out if the guest could change the file. For
   example, it cannot read the mount table.

Details for cases 2 and 3:

- The `wasmedge` tool creates the module instance when it starts.
- A change up to 20 ms before that time counts as after it. On a file system
  that keeps whole seconds, a change in the same second counts as after it.
- "Changed" means that the change time (`st_ctime`) moved. A write, a rename,
  a link or unlink, and a change of mode, owner, or times all move it.
- On macOS, the system volume and the data volume of the startup disk count
  as one file system.

The host log gives the guest path, the preopen or stream that causes the
refusal, and the reason. The embedder applies one of these remedies:

- Preopen the model directories with `:readonly`, outside every writable
  preopen: `--dir /models:/data/sd:readonly`. Put the files in place as
  copies (not hard links) before the VM is created.
- Or keep the writable directories of the guest on a different file system
  than the models. On Linux, a tmpfs is sufficient. On macOS, use a separate
  volume, such as an APFS volume or a disk image.
- After you change a file on a file system that the guest can write, create
  the VM again (restart the `wasmedge` tool).

The plug-in cannot use a file that the guest wrote, such as a LoRA that the
guest downloaded.

### 6.3 What to ask the embedder for

- A `:readonly` preopen for each directory with model, LoRA, or embedding
  files, as in section 6.2.
- For a model that is quantized in its file: only LoRA files made for that
  model. A LoRA that does not fit such a model can end the host process
  (section 6.4).
- Embedding files whose host paths contain no backslash, `<`, or `>`. The
  host path is the real location of the file, which the guest does not see.
  `session_open` refuses such an embedding with `LoadFailed`.

`DESIGN.md` lists the other duties of the embedder.

### 6.4 LoRA and embedding files

The plug-in reads the header of each LoRA and embedding file before the
library gets its path. If the file breaks these rules, the plug-in returns
`LoadFailed` and does not call the library.

**Both file types** must be safetensors files:

- The file starts with an 8-byte little-endian header length. The length is 1
  to 8,388,608 (8 MiB) and does not go past the end of the file.
- The header is a JSON object with no repeated key.
- `__metadata__`, if present, is an object of strings with no repeated key.
- Every other key is a tensor: an object with no repeated key, a `dtype` of
  `F16`, `BF16`, or `F32`, and a `shape` of integers that are 1 or more.
- No tensor is larger than the file (its element count times 2 or 4 bytes).
- All tensors together are not larger than the data after the header.

The library checks the tensor offsets and the data. It reports a damaged file
as `LoadFailed`.

**A LoRA** must also be of the plain LoRA family, in the kohya or the PEFT
spelling:

- Every tensor name ends in `.lora_down.weight`, `.lora_up.weight`,
  `.lora_A.weight`, `.lora_B.weight`, or `.alpha`, with text before the
  suffix.
- A down tensor (`lora_down`, `lora_A`) has the shape `[r, in]` or
  `[r, in, kh, kw]`. An up tensor (`lora_up`, `lora_B`) has the shape
  `[out, r]` or `[out, r, 1, 1]`.
- Each name before the suffix has exactly one down tensor and one up tensor,
  in the same spelling and with the same `r`. It has one alpha at most, with
  one element. An alpha must have its pair.
- A four-dimensional up tensor has an `out` of 2 or more, or an `r` of 1. A
  four-dimensional down tensor has an `r` or `in` of 2 or more, or a `kh` of
  1.
- The file has one pair or more.
- `__metadata__` has no `adapter_config`.

The plug-in refuses all other files, such as LoHa, LoKr, Tucker
(`lora_mid`), and full-difference files.

If no tensor of a LoRA matches a weight of the model, the request fails in
phase 2 with `RuntimeError`.

**How a session applies LoRAs** depends on its weights:

| Weights of the session | LoRAs |
|---|---|
| `weight_type` is a quantized type | Not accepted: a request with `loras` is `InvalidArgument`. To use LoRAs, omit `weight_type` or use `f32`, `f16`, or `bf16`. |
| known to be plain (not quantized, not FP8) | Merged into the weights for the request. The library skips a pair that does not fit its layer and writes a warning to the host log. |
| all other sessions: a model that is quantized in its file (a GGUF file, or a safetensors file with quantized or FP8 tensors) | The library chooses the method. A LoRA made for the model of the session works. **A LoRA whose tensors do not fit their layers, such as a LoRA for a different base model, can end the host process.** The plug-in cannot check the fit. Use only LoRAs made for the model of the session. |

The weights are "known to be plain" when each model file of the session
(`model_path`, `diffusion_model_path`, the three text encoders, `vae_path`,
`taesd_path`, `control_net_path`) is a safetensors file with a readable
header of 8 MiB at most, no tensor name that ends in `.comfy_quant`, and no
tensor of the type `F8_E4M3` or `F8_E5M2`.

**An embedding** must also:

- have 16,777,216 bytes (16 MiB) at most;
- have one tensor or more, each with one or two dimensions;
- have a host path with no backslash, `<`, or `>` (section 6.3).

The library checks the model, VAE, text encoder, ControlNet, and upscaler
files. It reports damage as `LoadFailed`.

### 6.5 Models on the CUDA and Metal assets

The CUDA assets and the macOS arm64 asset are built without the ggml
extensions of the library. On these assets, the library:

- refuses checkpoints with INT8 or packed INT4 weights (the ComfyUI formats);
- refuses GGUF files with FP8 tensors;
- loads FP8 safetensors weights as F16, which uses twice the memory;
- writes three warning lines to the host log for each session.

Models in F32, F16, BF16, and the GGML quantizations are not affected.

## 7. Images

### 7.1 Input images

`blobs_ptr` points to a table of `blobs_len` entries. Each entry has 8 bytes,
`{ptr: u32, len: u32}`, and names the bytes of one encoded image in the
memory of the calling module. `blobs_len` is 8 at most. If it is 0, pass 0 as
`blobs_ptr`. Every entry must be inside the memory, also if the request does
not use it.

A request refers to a blob by its index: `{"blob": n}`. One blob can serve
more than one key. The plug-in decodes only the blobs that a key refers to:

- A blob must be a PNG or a JPEG of 2,147,483,647 bytes at most. Other data,
  and data that does not decode, is `InvalidEncoding`.
- The plug-in reads the header first. An image with a side above 8192 or with
  more than 33,554,432 pixels is `InvalidArgument` and is not decoded.
- The decoder can use memory in proportion to the size in the header. Data
  that needs more is `InvalidEncoding`.
- `init_image` and `control_image` are decoded to three channels (RGB), and
  `mask_image` to one channel (gray).

If an image has a different size than the request, the library fits it to the
request.

### 7.2 Results

After a successful `generate`, the session holds `count` images at the
indexes 0 to `count - 1`. They stay until the next `generate` enters phase 2,
until `session_close`, or until the module instance is destroyed. Each result
has three channels of 8 bits. `result_info` reports its width and height,
which include the upscaling.

| `format` | Bytes |
|---|---|
| 0 | a PNG file: 8-bit RGB, no alpha |
| 1 | raw RGB: `width * height * 3` bytes, rows from top to bottom, each pixel red, green, blue |

To read a result, call `result_info` for its size, then call `result_read`
with a buffer of that size.

## 8. A worked example

The embedder keeps the model in `/data/sd` on the host and gives the guest a
second, writable directory for its output:

```sh
wasmedge --dir /models:/data/sd:readonly --dir /out:/srv/sd-out app.wasm
```

The guest sees the model as `/models/sd-v1-5.safetensors`. If `/srv/sd-out`
is on the same file system as `/data/sd`, the model must also have one name
and must be unchanged since `wasmedge` started (section 6.2).

The guest, in C, built for `wasm32-wasip1`:

```c
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SD(name)                                                               \
  __attribute__((import_module("wasmedge_stablediffusion"),                    \
                 import_name(name)))

SD("abi_version") uint32_t sd_abi_version(void);
SD("session_open")
uint32_t sd_session_open(const char *config, uint32_t config_len,
                         uint32_t *session);
SD("session_close") uint32_t sd_session_close(uint32_t session);
SD("generate")
uint32_t sd_generate(uint32_t session, const char *request,
                     uint32_t request_len, const uint32_t *blobs,
                     uint32_t blobs_len, uint32_t *count);
SD("result_info")
uint32_t sd_result_info(uint32_t session, uint32_t index, uint32_t format,
                        uint32_t info[4]);
SD("result_read")
uint32_t sd_result_read(uint32_t session, uint32_t index, uint32_t format,
                        uint8_t *buf, uint32_t buf_len, uint32_t *written);

enum { FORMAT_PNG = 0, FORMAT_RGB8 = 1 };

int main(void) {
  if (sd_abi_version() != 2) {
    fprintf(stderr, "the Stable Diffusion plug-in is not installed\n");
    return 1;
  }

  const char *config = "{\"model_path\":\"/models/sd-v1-5.safetensors\"}";
  uint32_t session;
  uint32_t err = sd_session_open(config, (uint32_t)strlen(config), &session);
  if (err != 0) {
    fprintf(stderr, "session_open failed with %u\n", err);
    return 1;
  }

  const char *request = "{\"prompt\":\"a lighthouse at dawn, oil painting\","
                        "\"width\":512,\"height\":512,\"steps\":20,"
                        "\"seed\":42,\"batch_count\":2}";
  uint32_t count = 0;
  err = sd_generate(session, request, (uint32_t)strlen(request), NULL, 0,
                    &count);
  if (err != 0) {
    fprintf(stderr, "generate failed with %u\n", err);
    sd_session_close(session);
    return 1;
  }

  for (uint32_t i = 0; i < count; ++i) {
    uint32_t info[4]; /* width, height, channels, size */
    if (sd_result_info(session, i, FORMAT_PNG, info) != 0) {
      break;
    }
    uint8_t *png = malloc(info[3]);
    uint32_t written = 0;
    if (png == NULL ||
        sd_result_read(session, i, FORMAT_PNG, png, info[3], &written) != 0) {
      free(png);
      break;
    }
    char name[32];
    snprintf(name, sizeof name, "/out/image-%u.png", (unsigned)i);
    FILE *file = fopen(name, "wb");
    if (file != NULL) {
      fwrite(png, 1, written, file);
      fclose(file);
    }
    free(png);
  }

  sd_session_close(session);
  return 0;
}
```

To use a result as the input of image-to-image, keep its bytes, put them in
the blob table, and name the blob in the request. In this example, `png` and
`written` hold a result from the loop above, before `free`:

```c
  uint32_t blobs[2] = {(uint32_t)(uintptr_t)png, written}; /* ptr, len */
  const char *again = "{\"prompt\":\"the same lighthouse at night\","
                      "\"init_image\":{\"blob\":0},\"strength\":0.6}";
  err = sd_generate(session, again, (uint32_t)strlen(again), blobs, 1, &count);
```

This second `generate` replaces the results of the session.

## 9. Changes from the 0.4 ABI

The 0.4 ABI is removed. A guest built for it does not instantiate, and there
is no compatibility layer. The error codes 0 to 5 keep their numbers. The
codes 6 to 9 and `abi_version` are new.

| Removed | Replacement |
|---|---|
| `create_context`, `text_to_image`, and `image_to_image`, with 33, 35, and 40 positional parameters | `session_open` and `generate`, with one JSON text each |
| numbers for sampler, schedule, weight type, and RNG | the names of section 5 |
| `path:` image inputs and the output path | images cross as bytes (section 7); a guest reads and writes its own files through WASI |
| host paths for model, LoRA, and embedding files | guest paths, resolved through the WASI preopens (section 6) |
| the PNG written into a caller-sized buffer during the generation | results stay in the session; a small buffer no longer costs a generation |
| `convert` | none; the command-line tool of the library converts models |
| canny preprocessing, PhotoMaker (ID images, embedding directory, style ratio, input normalization), skip-layer guidance, and the decode-only VAE flag | none; the pinned library has no public parameter for them |
| the LoRA and embedding directories, and LoRA tags in prompts | `loras` per request and `embeddings` per session |
| the upscaler path of each generation call | `upscaler_model_path` per session and `upscale_repeats` per request |
| the flags that kept the text encoder, the VAE, or the ControlNet on the CPU | `device` and `cpu_modules` |
| two sessions for one model, one for text-to-image and one for image-to-image | one session serves both |

Other differences:

- A file that the guest could change is refused with `NotReadOnly`. Thus
  `--dir .:.` alone does not make a model in the working directory usable.
- Sizes must be multiples of 64 within the limits of section 4.3.
- The library fits an input, mask, or control image of a different size.
- A failed generation leaves the session usable.
- The plug-in frees the models when a session closes or its module instance
  is destroyed.
- The output of the library goes to the WasmEdge log, not to stdout.
- Calls on one session run one at a time, and so do the library calls of all
  sessions.
