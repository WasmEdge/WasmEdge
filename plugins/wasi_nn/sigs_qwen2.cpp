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

#include "sigs_qwen2.h"
#include "causal_decoder.h"
#include "npy.h"
#include "registry.h"
#include <algorithm>
#include <cstring>
#include <stdexcept>

namespace sgsmz {

REGISTER_ARCHITECTURE("Qwen2ForCausalLM", Qwen2ForCausalLM);

Qwen2ForCausalLM::Qwen2ForCausalLM(const std::string &model_dir) { init(model_dir); }
Qwen2ForCausalLM::~Qwen2ForCausalLM() = default;

void Qwen2ForCausalLM::load_weights(const std::string &model_dir) {
    auto load_npy = [&](const std::string &file) {
        NpyArray npy = NpyArray::load(model_dir + "/" + file);
        Buffer buf(npy.get_size());
        npy.dump(buf);
        return buf;
    };
    this->embedding_ = load_npy(this->config_.emb_file);
    if (!this->config_.cos_file.empty()) {
        this->cos_pos_ = load_npy(this->config_.cos_file);
        this->sin_pos_ = load_npy(this->config_.sin_file);
    }
}

std::vector<std::pair<std::string, std::string>> Qwen2ForCausalLM::kv_pairs() const {
    return {{"past_key_in", "past_key_out"}, {"past_value_in", "past_value_out"}};
}

std::unique_ptr<Decoder> Qwen2ForCausalLM::create_decoder(const std::string &model_dir) {
    return std::make_unique<CausalDecoder>(model_dir, this->config_.decoder_model_path);
}

int Qwen2ForCausalLM::prepare_model_io(const std::vector<int> &input_ids, int position,
                                       bool is_decoder) {
    (void)is_decoder;
    this->pos_ = position;
    int size = static_cast<int>(input_ids.size());
    prepare_hidden_in(input_ids);
    prepare_causal_mask(size);
    prepare_cos_sin(size);
    return 0;
}

void Qwen2ForCausalLM::prepare_hidden_in(const std::vector<int> &input_ids) {
    auto &invoke = this->decoder_->invoke_param();
    int dim = this->decoder_->hidden_dim();
    Buffer &hidden = this->decoder_->hidden_in();
    int elem_size = (hidden.get_format() == MI_IPU_FORMAT_FP32) ? static_cast<int>(sizeof(float))
                                                                : static_cast<int>(sizeof(int16_t));

    if (input_ids.size() == 1) {
        int token_id = input_ids[0];
        ptrdiff_t off = static_cast<ptrdiff_t>(token_id) * dim * elem_size;
        invoke.stInvokeParam.astArrayTensors[0].ptTensorData[0] =
            static_cast<char *>(this->embedding_.get_addr()) + off;
        invoke.stInvokeParam.astArrayTensors[0].phyTensorAddr[0] =
            this->embedding_.get_phy_addr() + static_cast<MI_PHY>(off);
        return;
    }

    size_t written = input_ids.size() * static_cast<size_t>(dim) * static_cast<size_t>(elem_size);
    if (written < hidden.get_size()) {
        std::memset(static_cast<char *>(hidden.get_addr()) + written, 0,
                    hidden.get_size() - written);
    }
    for (size_t i = 0; i < input_ids.size(); ++i) {
        int token_id = input_ids[i];
        std::memcpy(static_cast<char *>(hidden.get_addr()) + i * dim * elem_size,
                    static_cast<const char *>(this->embedding_.get_addr()) +
                        static_cast<ptrdiff_t>(token_id) * dim * elem_size,
                    static_cast<size_t>(dim) * elem_size);
    }
    this->decoder_->hidden_in().sync_cache();
    invoke.stInvokeParam.astArrayTensors[0].ptTensorData[0] = hidden.get_addr();
    invoke.stInvokeParam.astArrayTensors[0].phyTensorAddr[0] = hidden.get_phy_addr();
}

void Qwen2ForCausalLM::prepare_causal_mask(int size) {
    auto *causal_decoder = dynamic_cast<CausalDecoder *>(this->decoder_.get());
    if (causal_decoder == nullptr) {
        throw std::runtime_error("Decoder is not a CausalDecoder");
    }
    auto &invoke = this->decoder_->invoke_param();
    Buffer &mask = causal_decoder->attention_mask();
    int mask_step = static_cast<int>(mask.get_shape().back());
    int mask_rows = static_cast<int>(mask.get_shape()[2]);
    constexpr float k_causal_mask_fill = -10000.0F;
    float *data = static_cast<float *>(mask.get_addr());
    for (int i = 0; i < mask_rows * mask_step; ++i) {
        data[i] = k_causal_mask_fill;
    }
    for (int i = 0; i < size; ++i) {
        for (int j = 0; j <= this->pos_ + i && j < mask_step; ++j) {
            data[i * mask_step + j] = 0.0F;
        }
    }
    causal_decoder->attention_mask().sync_cache();
    invoke.stInvokeParam.astArrayTensors[1].ptTensorData[0] = mask.get_addr();
    invoke.stInvokeParam.astArrayTensors[1].phyTensorAddr[0] = mask.get_phy_addr();
}

void Qwen2ForCausalLM::prepare_cos_sin(int size) {
    (void)size;
    auto *causal_decoder = dynamic_cast<CausalDecoder *>(this->decoder_.get());
    if (causal_decoder == nullptr) {
        throw std::runtime_error("Decoder is not a CausalDecoder");
    }
    auto &invoke = this->decoder_->invoke_param();
    int fp16_size = static_cast<int>(sizeof(int16_t));
    int cos_step = static_cast<int>(causal_decoder->cos_step());
    ptrdiff_t cos_off = static_cast<ptrdiff_t>(this->pos_) * cos_step * fp16_size;
    invoke.stInvokeParam.astArrayTensors[2].ptTensorData[0] =
        static_cast<char *>(this->cos_pos_.get_addr()) + cos_off;
    invoke.stInvokeParam.astArrayTensors[2].phyTensorAddr[0] =
        this->cos_pos_.get_phy_addr() + static_cast<MI_PHY>(cos_off);
    int sin_step = static_cast<int>(causal_decoder->sin_step());
    ptrdiff_t sin_off = static_cast<ptrdiff_t>(this->pos_) * sin_step * fp16_size;
    invoke.stInvokeParam.astArrayTensors[3].ptTensorData[0] =
        static_cast<char *>(this->sin_pos_.get_addr()) + sin_off;
    invoke.stInvokeParam.astArrayTensors[3].phyTensorAddr[0] =
        this->sin_pos_.get_phy_addr() + static_cast<MI_PHY>(sin_off);
}

} // namespace sgsmz
