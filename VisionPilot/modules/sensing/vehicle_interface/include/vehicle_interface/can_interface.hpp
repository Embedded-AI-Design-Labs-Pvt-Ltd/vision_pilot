#ifndef VISIONPILOT_CAN_INTERFACE_HPP
#define VISIONPILOT_CAN_INTERFACE_HPP

#include <vehicle_interface/vehicle_interface.hpp>

#include <atomic>
#include <mutex>
#include <string>
#include <thread>

/**
 * Vehicle gateway over TCP (default 0.0.0.0:59000).
 * Speaks the ASCII protocol in platforms/protocol/vp_vehicle_gateway.md
 * so Arduino / ESP32 / Zephyr gateways can exchange SPEED / CMD frames.
 *
 * Environment overrides:
 *   VP_VEHICLE_GATEWAY_HOST  (default 0.0.0.0)
 *   VP_VEHICLE_GATEWAY_PORT  (default 59000)
 *   VP_VEHICLE_GATEWAY_DISABLE=1  → no-op stub (legacy behavior)
 */
class CanInterface : public VehicleInterface
{
public:
    CanInterface();
    ~CanInterface() override;

    double read() override;
    void write(double steering, double acceleration) override;

private:
    void server_loop();
    void client_loop(int client_fd);
    static void set_nonblocking(int fd);

    std::string host_;
    int port_{59000};
    bool enabled_{true};

    std::atomic<bool> running_{false};
    std::thread server_thread_;

    mutable std::mutex mu_;
    double speed_mps_{0.0};
    double steer_cmd_{0.0};
    double accel_cmd_{0.0};
    int client_fd_{-1};
};

#endif //VISIONPILOT_CAN_INTERFACE_HPP
