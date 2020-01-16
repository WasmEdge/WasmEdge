/* EVMC: Ethereum Client-VM Connector API.
 * Copyright 2016-2019 The EVMC Authors.
 * Licensed under the Apache License, Version 2.0.
 */

#include <evmc/evmc.h>

#if __cplusplus
extern "C" {
#endif

struct evmc_context *
example_host_create_context(struct evmc_tx_context tx_context);

void example_host_destroy_context(struct evmc_context *context);

#if __cplusplus
}
#endif
