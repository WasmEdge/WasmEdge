// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#pragma once

#include "llmc_env.h"

extern "C" {

struct GPT2;
struct Tokenizer;
struct DataLoader;

GPT2 *gpt2_create(const char *checkpoint_path);

void gpt2_destroy(GPT2 *model);

DataLoader *dataloader_create(const char *filename_pattern, size_t B, size_t T,
                              int process_rank, int num_processes,
                              int should_shuffle);
void dataloader_destroy(DataLoader *loader);

Tokenizer *tokenizer_create(const char *filename);

void tokenizer_destroy(Tokenizer *tokenizer);

void gpt2_train(GPT2 *model, DataLoader *train_loader, DataLoader *val_loader,
                Tokenizer *tokenizer, int B, int T, float lr, int epoch);
}
