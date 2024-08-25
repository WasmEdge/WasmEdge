// SPDX-License-Identifier: GPL-2.0 OR BSD-3-Clause
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#define SEC(name) __attribute__((section(name), used))
#define __uint(name, val) int(*name)[val]
#define __type(name, val) typeof(val) *name

#define __u64 unsigned long long
#define __u32 unsigned int

#define u32 __u32
#define u64 __u64

#define BPF_MAP_TYPE_HASH ((u32)1)
struct {
  __uint(type, BPF_MAP_TYPE_HASH);
  __uint(max_entries, 16);
  __type(key, u32);
  __type(value, u64);
} test_map SEC(".maps");
static void *(*bpf_map_lookup_elem)(void *map, const void *key) = (void *)1;
static long (*bpf_map_update_elem)(void *map, const void *key,
                                   const void *value, __u64 flags) = (void *)2;

static const u32 INDICATING_KEY = 0xABCD;
static const u32 ADD_VALUE_1_KEY = 0xCDEF;
static const u32 ADD_VALUE_2_KEY = 0x1234;
static const u32 RESULT_VALUE_KEY = 0x7890;
SEC("tp_btf/sched_wakeup")
int sched_wakeup(void *ctx) {
  // Use an element with key `0xABCD` to indicate that the userspace program
  // already set values of the add values.
  if (!bpf_map_lookup_elem(&test_map, &INDICATING_KEY)) {
    return 0;
  }
  // Read the two add values from the map
  u64 *val1, *val2;
  val1 = bpf_map_lookup_elem(&test_map, &ADD_VALUE_1_KEY);
  val2 = bpf_map_lookup_elem(&test_map, &ADD_VALUE_2_KEY);
  if (!val1 || !val2)
    return 0;
  u64 result = (u64)(*val1) + (u64)(*val2);
  // Store the result
  bpf_map_update_elem(&test_map, &RESULT_VALUE_KEY, &result, 0);
  return 0;
}

char LICENSE[] SEC("license") = "Dual BSD/GPL";
