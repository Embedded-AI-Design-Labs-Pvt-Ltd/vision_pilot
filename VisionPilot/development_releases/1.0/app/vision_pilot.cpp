// VisionPilot — preprocess → inference → fusion → display
#include <config/vision_pilot_config.hpp>
#include <debug/debug_draw.hpp>
#include <engine/onnx_engine.hpp>
#include <image_preprocessing/image_preprocessor.hpp>
#include <logging/logger.hpp>
#include <models/inference.hpp>
#include <visualization/visualization.hpp>

#include <camera_interface/camera_interface.hpp>
#include <camera_interface/v4l2_camera_interface.hpp>
#ifdef ENABLE_WEBRTC
#include <visualization/visualization_to_webrtc.hpp>
#endif
#ifdef ENABLE_ROS2_INTERFACE
#include <camera_subscriber/ros2_to_opencv.hpp>
#endif

#include <chrono>
#include <memory>
#include <opencv2/videoio.hpp>
#include <thread>

namespace ve = visionpilot::engine;
namespace vm = visionpilot::models;
namespace vd = visionpilot::debug;

struct DisplayOutput {
    bool show_local_preview = true;
#ifdef ENABLE_WEBRTC
    std::unique_ptr<visualization::WebRTCStreamer> webrtc;
#endif
};

static cv::Mat preprocess(const ImagePreprocessor& p, const cv::Mat& frame)
{
    cv::Mat warped, resized;
    p.preprocess(frame, warped, resized, cv::Size(vm::AutoDrive::NET_W, vm::AutoDrive::NET_H));
    return warped;
}

static vd::DebugView to_view(const vm::InferenceFrameResult& r, const std::string& label,
                             const VisionPilotConfig& cfg)
{
    return {r.frame_id, r.wall_ms, r.pre_ms, r.ad_ms, r.as_ms, r.asp_ms, label,
            r.auto_drive, r.auto_steer, r.auto_speed, r.cipo, r.lateral, {}, cfg.wheel_dir,
            cfg.homography_path};
}

static void present(cv::Mat& prep, DisplayOutput& out)
{
    if (out.show_local_preview) visualization::render_frame(prep, "VisionPilot", {});
#ifdef ENABLE_WEBRTC
    if (out.webrtc) out.webrtc->push_frame(prep);
#endif
}

static void tick(cv::Mat& prep, vm::InferencePipeline& pipe, const std::string& label,
                 const VisionPilotConfig& cfg, DisplayOutput& out)
{
    if (const auto r = pipe.process(prep)) {
        if (r->frame_id % 30 == 0) pipe.latency().print();
        vd::annotate_frame(prep, to_view(*r, label, cfg));
    }
    present(prep, out);
}

static int run_video(vm::InferencePipeline& pipe, const VisionPilotConfig& cfg,
                     const ImagePreprocessor& prep, DisplayOutput& out)
{
    cv::VideoCapture cap(cfg.source.video_path);
    if (!cap.isOpened()) { VP_ERROR("Cannot open video: %s", cfg.source.video_path.c_str()); return 1; }

    const double fps = cap.get(cv::CAP_PROP_FPS);
    const auto period = (cfg.source.video_realtime && fps > 1.0)
                            ? std::chrono::duration<double>(1.0 / fps)
                            : std::chrono::duration<double>(0);
    cv::Mat frame;
    for (;;) {
        const auto t0 = std::chrono::steady_clock::now();
        if (!cap.read(frame) || frame.empty()) {
            if (!cfg.source.video_loop) break;
            cap.set(cv::CAP_PROP_POS_FRAMES, 0);
            pipe.reset();
            continue;
        }
        cv::Mat p = preprocess(prep, frame);
        tick(p, pipe, "video", cfg, out);
        if (period.count() > 0) {
            const auto rem = period - (std::chrono::steady_clock::now() - t0);
            if (rem.count() > 0) std::this_thread::sleep_for(rem);
        }
    }
    visualization::close_windows();
    return 0;
}

static void run_live(vm::InferencePipeline& pipe, const ImagePreprocessor& prep,
                     camera_interface::CameraInterface& cam, const std::string& label,
                     const VisionPilotConfig& cfg, DisplayOutput& out)
{
    for (;;) {
        auto [ok, frame] = cam.get_latest_frame();
        if (!ok || frame.empty()) { std::this_thread::sleep_for(std::chrono::milliseconds(5)); continue; }
        cv::Mat p = preprocess(prep, frame);
        tick(p, pipe, label, cfg, out);
    }
}

int main(int argc, char** argv)
{
    const std::string cfg_path = resolve_vision_pilot_config_path(argc, argv);
    if (cfg_path.empty()) { VP_ERROR("No config — cp config/vision_pilot.conf.example config/vision_pilot.conf"); return 1; }

    VisionPilotConfig cfg;
    try { cfg = load_vision_pilot_config(cfg_path); }
    catch (const std::exception& e) { VP_ERROR("Config: %s", e.what()); return 1; }

    ImagePreprocessor preprocessor;
    ve::OnnxEngine engine(cfg.engine_cfg);
    vm::InferencePipeline pipeline(engine, {cfg.autodrive_model, cfg.autosteer_model, cfg.autospeed_model,
                                            cfg.homography_path, cfg.fusion_debug});
    vd::init_wheel_assets(cfg.wheel_dir);
    vd::init_homography(cfg.homography_path);

    DisplayOutput display;
#ifdef ENABLE_WEBRTC
    for (int i = 1; i < argc; ++i) {
        if (std::string(argv[i]) == "--webrtc") display.show_local_preview = false;
        if (std::string(argv[i]) == "--webrtc-port" && i + 1 < argc) {
            display.webrtc = std::make_unique<visualization::WebRTCStreamer>();
            if (!display.webrtc->init(static_cast<uint16_t>(std::stoi(argv[++i])))) return 1;
        }
    }
#endif

    switch (cfg.source.mode) {
        case SourceMode::Video:
            return run_video(pipeline, cfg, preprocessor, display);
        case SourceMode::Ros2:
#ifdef ENABLE_ROS2_INTERFACE
            { camera_interface::ROS2ImageSubscriber sub(cfg.source.ros2_topic);
              run_live(pipeline, preprocessor, sub, cfg.source.ros2_topic, cfg, display); }
#else
            VP_ERROR("ROS2 requested but ENABLE_ROS2_INTERFACE=OFF");
#endif
            break;
        case SourceMode::V4l2:
            { camera_interface::V4L2CameraInterface cam(cfg.source.v4l2_device,
                                                          static_cast<uint32_t>(cfg.source.v4l2_fps));
              run_live(pipeline, preprocessor, cam, cfg.source.v4l2_device, cfg, display); }
            break;
    }
    return 0;
}
