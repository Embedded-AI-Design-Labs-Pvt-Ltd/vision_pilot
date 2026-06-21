// VisionPilot — preprocess → inference → fusion → display
#include <config/vision_pilot_config.hpp>
#include <debug/debug_draw.hpp>
#include <engine/onnx_engine.hpp>
#include <image_preprocessing/image_preprocessor.hpp>
#include <logging/logger.hpp>
#include <models/inference.hpp>
#include <planning/planning.hpp>
#include <visualization/visualization.hpp>

#include <camera_interface/frame_source.hpp>
#ifdef ENABLE_WEBRTC
#include <visualization/visualization_to_webrtc.hpp>
#endif

#ifdef ENABLE_ROS2_INTERFACE
#include <vehicle_state_subscriber/ros2_to_can.hpp>
#include <control_cmd_publisher/cmd_to_ros2.hpp>
#endif

#include <chrono>
#include <memory>
#include <thread>

namespace ve = visionpilot::engine;
namespace vm = visionpilot::models;
namespace vd = visionpilot::debug;

int main(int argc, char **argv) {
    // ── 1. Config ─────────────────────────────────────────────────────────────
    const std::string cfg_path = resolve_vision_pilot_config_path(argc, argv);
    if (cfg_path.empty()) {
        VP_ERROR("No config — cp config/vision_pilot.conf.example config/vision_pilot.conf");
        return 1;
    }

    VisionPilotConfig cfg;
    try { cfg = load_vision_pilot_config(cfg_path); } catch (const std::exception &e) {
        VP_ERROR("Config: %s", e.what());
        return 1;
    }

    // ── 2. Pipeline (preprocess + ONNX + inference/fusion) ────────────────────
    ImagePreprocessor preprocessor;
    ve::OnnxEngine engine(cfg.engine);
    vm::InferencePipeline pipeline(engine, cfg.inference);

    Planner planner(cfg.speed_limit, cfg.Lf);

    vd::init_wheel_assets(cfg.wheel_dir);
    vd::init_homography();

    // ── 3. Display output ─────────────────────────────────────────────────────
    bool show_window = true;
#ifdef ENABLE_WEBRTC
    std::unique_ptr<visualization::WebRTCStreamer> webrtc;
    for (int i = 1; i < argc; ++i) {
        if (std::string(argv[i]) == "--webrtc") show_window = false;
        if (std::string(argv[i]) == "--webrtc-port" && i + 1 < argc) {
            webrtc = std::make_unique<visualization::WebRTCStreamer>();
            if (!webrtc->init(static_cast<uint16_t>(std::stoi(argv[++i])))) return 1;
        }
    }
#endif

    // ── 4. Frame source (video / V4L2 / ROS2) ───────────────────────────────
    auto source = camera_interface::open_frame_source(cfg.source);
    if (!source || !source->is_device_open()) {
        VP_ERROR("Cannot open frame source");
        return 1;
    }

    // ── 5. Middleware (optional ROS2 + shmem) ────────────────────────────────
    //
    // When built with -DENABLE_ROS2_INTERFACE=ON:
    //   • VehicleStateSubscriber  subscribes /vehicle/speed + /vehicle/steering_angle
    //     and writes to POSIX shmem /vp_state_shm
    //   • ControlCmdPublisher     publishes /vehicle/steering_cmd + /vehicle/throttle_cmd
    //     and writes to POSIX shmem /vp_ctrl_shm
    //
    // Both channels work independently — a CARLA bridge, CAN writer, or any other
    // process can pick up the shmem without a ROS2 dependency.
    //
#ifdef ENABLE_ROS2_INTERFACE
    vp_middleware::VehicleStateSubscriber state_sub;
    vp_middleware::ControlCmdPublisher    cmd_pub;
    VP_INFO("ROS2 middleware enabled — vehicle state subscriber + control cmd publisher running");
#endif

    const cv::Size net_size(vm::AutoDrive::NET_W, vm::AutoDrive::NET_H);
    const std::string label = source_label(cfg.source);
    cv::Mat frame, warped, resized;

    // ── 6. Main loop ────────────────────────────────────────────────────────
    while (true) {
        auto [ok, frame] = source->get_latest_frame();
        if (!ok || frame.empty()) {
            if (cfg.source.mode == SourceMode::Video && !cfg.source.video_loop) break;
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
            continue;
        }

        preprocessor.preprocess(frame, warped, resized, net_size);

        if (const auto r = pipeline.process(warped)) {
            pipeline.latency().print();
            vd::annotate_frame(warped, vd::debug_view_from(
                                   *r, label, cfg.wheel_dir));

            // ── Plan (lateral + longitudinal) ──────────────────────────────
            double cte          = r->lateral.cte_m;
            double epsi         = r->lateral.yaw_rad;
            double kappa        = r->lateral.curvature;
            double cipo_dist    = r->cipo.distance_m;
            bool   has_cipo     = r->cipo.cipo_raw_found;
            double cipo_v       = has_cipo ? r->cipo.velocity_ms : cfg.speed_limit;

            // Ego speed: prefer live vehicle state when ROS2 middleware is active
            double ego_v = 0.0;
#ifdef ENABLE_ROS2_INTERFACE
            if (state_sub.is_valid()) {
                ego_v = static_cast<double>(state_sub.get_state().speed_ms);
            }
#endif

            auto [acceleration, deltas] = planner.compute_plan(
                cte, epsi, kappa, ego_v, has_cipo, cipo_v, cipo_dist);

            // First element of deltas is the tyre angle to apply now
            const float tyre_angle_rad  = deltas.empty() ? 0.0f : static_cast<float>(deltas[0]);
            const float accel_ms2       = static_cast<float>(acceleration);

            VP_INFO("plan: tyre=%.4f rad  accel=%.3f m/s²", tyre_angle_rad, accel_ms2);

#ifdef ENABLE_ROS2_INTERFACE
            cmd_pub.publish(tyre_angle_rad, accel_ms2);
#endif
        }

        if (show_window) visualization::render_frame(warped, "VisionPilot", {});
#ifdef ENABLE_WEBRTC
        if (webrtc) webrtc->push_frame(warped);
#endif
    }

    visualization::close_windows();
    return 0;
}
