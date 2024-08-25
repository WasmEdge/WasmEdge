// SPDX-License-Identifier: GPL-2.0 OR BSD-3-Clause
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#define SEC(name) __attribute__((section(name), used))
#define __uint(name, val) int(*name)[val]

#define __u64 unsigned long long
#define __u32 unsigned int

#define u32 __u32

#define BPF_MAP_TYPE_RINGBUF ((u32)27)

char LICENSE[] SEC("license") = "Dual BSD/GPL";
static void *(*bpf_ringbuf_reserve)(void *ringbuf, __u64 size,
                                    __u64 flags) = (void *)131;

static void (*bpf_ringbuf_submit)(void *data, __u64 flags) = (void *)132;

struct {
  __uint(type, BPF_MAP_TYPE_RINGBUF);
  __uint(max_entries, 256 * 1024);
} rb SEC(".maps");

SEC("tp/sched/sched_process_exec")
int handle_exec(void *ctx) {
  u32 send_data;
  send_data = 0xABCD1234;

  /* reserve sample from BPF ringbuf */
  u32 *e = bpf_ringbuf_reserve(&rb, sizeof(send_data), 0);
  if (!e)
    return 0;
  *e = send_data;
  /* successfully submit it to user-space for post-processing */
  bpf_ringbuf_submit(e, 0);
  return 0;
}
