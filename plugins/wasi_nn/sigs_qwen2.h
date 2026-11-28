/* Sgs trade secret */
/* Copyright (c) [2019~2025] Sgs Technology Ltd.
All rights reserved.

Unless otherwise stipulated in writing, any and all information contained
herein regardless in any format shall remain the sole proprietary of
Sgs and be kept in strict confidence
(Sgs Confidential Information) by the recipient.
Any unauthorized act including without limitation unauthorized disclosure,
copying, use, reproduction, sale, distribution, modification, disassembling,
reverse engineering and compiling of the contents of Sgs Confidential
Information is unlawful and strictly prohibited. Sgs hereby reserves the
rights to any and all damages, losses, costs and expenses resulting therefrom.
*/

#ifndef QWEN2_FOR_CAUSAL_LM_H_
#define QWEN2_FOR_CAUSAL_LM_H_

#include "model.h"

namespace sgsmz {

class Qwen2ForCausalLM : public Model {
public:
    explicit Qwen2ForCausalLM(const std::string &model_dir);
    ~Qwen2ForCausalLM() override;

    Qwen2ForCausalLM(const Qwen2ForCausalLM &) = delete;
    Qwen2ForCausalLM &operator=(const Qwen2ForCausalLM &) = delete;
    Qwen2ForCausalLM(Qwen2ForCausalLM &&) = delete;
    Qwen2ForCausalLM &operator=(Qwen2ForCausalLM &&) = delete;

protected:
    void load_weights(const std::string &model_dir) override;
    std::vector<std::pair<std::string, std::string>> kv_pairs() const override;
    std::unique_ptr<Decoder> create_decoder(const std::string &model_dir) override;
    int prepare_model_io(const std::vector<int> &input_ids, int position, bool is_decoder) override;

private:
    void prepare_hidden_in(const std::vector<int> &input_ids);
    void prepare_causal_mask(int size);
    void prepare_cos_sin(int size);

    Buffer cos_pos_;
    Buffer sin_pos_;
    int pos_ = 0;
};

} // namespace sgsmz

#endif // QWEN2_FOR_CAUSAL_LM_H_
