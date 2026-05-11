#pragma once

#include "models/model_types.hpp"
#include "models/onnx_session.hpp"

#include <onnxruntime_cxx_api.h>

#include <memory>
#include <string>
#include <vector>

namespace visionpilot::models {

// ─── Interface ────────────────────────────────────────────────────────────────
// Abstract base for the AutoSteer single-frame path prediction model.
//
// Preprocessing contract (caller's responsibility before calling infer()):
//   • Resize frame to NET_W × NET_H (1024 × 512)
//   • Convert BGR → RGB
//   • Scale pixel values to [0, 1]  (NO ImageNet normalisation)
//   • Layout: CHW float32, size = 3 × NET_H × NET_W = CHW_SIZE elements
//
// Output interpretation:
//   xp       — (2, 64) ego-path x-coordinates in image space
//   h_vector — (2, 64) homography-related vectors
class AutoSteerBase {
public:
    virtual ~AutoSteerBase() = default;

    // image_chw : float32 CHW buffer, CHW_SIZE elements, RGB, [0, 1].
    virtual AutoSteerOutput infer(const float* image_chw) = 0;
};

// ─── ONNX Runtime implementation ─────────────────────────────────────────────
class AutoSteerOnnx final : public AutoSteerBase {
public:
    static constexpr int NET_H    = 512;
    static constexpr int NET_W    = 1024;
    static constexpr int CHW_SIZE = 3 * NET_H * NET_W;  // 1 572 864

    explicit AutoSteerOnnx(const OnnxSessionConfig& cfg);
    ~AutoSteerOnnx() override = default;

    AutoSteerOutput infer(const float* image_chw) override;

private:
    Ort::Env                       env_;
    std::unique_ptr<Ort::Session>  session_;
    Ort::MemoryInfo                mem_info_;

    std::vector<std::string>  in_name_strs_;
    std::vector<const char*>  in_names_;

    std::vector<std::string>  out_name_strs_;
    std::vector<const char*>  out_names_;

    std::vector<int64_t>  input_shape_;  // {1, 3, NET_H, NET_W}
};

}  // namespace visionpilot::models
