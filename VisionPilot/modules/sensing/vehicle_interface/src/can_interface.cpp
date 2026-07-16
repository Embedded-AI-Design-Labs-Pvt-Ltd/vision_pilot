#include <vehicle_interface/can_interface.hpp>

#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cerrno>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace {

const char *env_or(const char *key, const char *fallback)
{
    const char *v = std::getenv(key);
    return (v && *v) ? v : fallback;
}

} // namespace

CanInterface::CanInterface()
{
    host_ = env_or("VP_VEHICLE_GATEWAY_HOST", "0.0.0.0");
    port_ = std::atoi(env_or("VP_VEHICLE_GATEWAY_PORT", "59000"));
    const char *disable = std::getenv("VP_VEHICLE_GATEWAY_DISABLE");
    enabled_ = !(disable && (std::strcmp(disable, "1") == 0 || std::strcmp(disable, "true") == 0));

    if (!enabled_) {
        return;
    }

    running_ = true;
    server_thread_ = std::thread([this]() { server_loop(); });
}

CanInterface::~CanInterface()
{
    running_ = false;
    {
        std::lock_guard<std::mutex> lock(mu_);
        if (client_fd_ >= 0) {
            ::shutdown(client_fd_, SHUT_RDWR);
            ::close(client_fd_);
            client_fd_ = -1;
        }
    }
    if (server_thread_.joinable()) {
        server_thread_.join();
    }
}

double CanInterface::read()
{
    std::lock_guard<std::mutex> lock(mu_);
    return speed_mps_;
}

void CanInterface::write(double steering, double acceleration)
{
    std::lock_guard<std::mutex> lock(mu_);
    steer_cmd_ = steering;
    accel_cmd_ = acceleration;
    if (client_fd_ < 0) {
        return;
    }
    char buf[128];
    const int n = std::snprintf(buf, sizeof(buf), "CMD %.6f %.6f\n", steering, acceleration);
    if (n > 0) {
        (void)::send(client_fd_, buf, static_cast<size_t>(n), MSG_NOSIGNAL);
    }
}

void CanInterface::set_nonblocking(int fd)
{
    const int flags = fcntl(fd, F_GETFL, 0);
    if (flags >= 0) {
        fcntl(fd, F_SETFL, flags | O_NONBLOCK);
    }
}

void CanInterface::server_loop()
{
    const int server_fd = ::socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        return;
    }

    int yes = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(static_cast<uint16_t>(port_));
    if (::inet_pton(AF_INET, host_.c_str(), &addr.sin_addr) != 1) {
        addr.sin_addr.s_addr = INADDR_ANY;
    }

    if (::bind(server_fd, reinterpret_cast<sockaddr *>(&addr), sizeof(addr)) < 0 ||
        ::listen(server_fd, 1) < 0) {
        ::close(server_fd);
        return;
    }

    set_nonblocking(server_fd);

    while (running_) {
        const int client = ::accept(server_fd, nullptr, nullptr);
        if (client < 0) {
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            continue;
        }
        {
            std::lock_guard<std::mutex> lock(mu_);
            if (client_fd_ >= 0) {
                ::close(client_fd_);
            }
            client_fd_ = client;
        }
        client_loop(client);
        {
            std::lock_guard<std::mutex> lock(mu_);
            if (client_fd_ == client) {
                client_fd_ = -1;
            }
        }
        ::close(client);
    }

    ::close(server_fd);
}

void CanInterface::client_loop(int client_fd)
{
    set_nonblocking(client_fd);
    std::string pending;
    pending.reserve(256);
    char buf[256];

    while (running_) {
        const ssize_t n = ::recv(client_fd, buf, sizeof(buf), 0);
        if (n > 0) {
            pending.append(buf, static_cast<size_t>(n));
            size_t pos;
            while ((pos = pending.find('\n')) != std::string::npos) {
                std::string line = pending.substr(0, pos);
                pending.erase(0, pos + 1);
                if (!line.empty() && line.back() == '\r') {
                    line.pop_back();
                }
                if (line.rfind("SPEED ", 0) == 0) {
                    const double v = std::atof(line.c_str() + 6);
                    std::lock_guard<std::mutex> lock(mu_);
                    speed_mps_ = v;
                } else if (line == "PING") {
                    const char *pong = "PONG\n";
                    (void)::send(client_fd, pong, 5, MSG_NOSIGNAL);
                }
            }
        } else if (n == 0) {
            break;
        } else if (errno != EAGAIN && errno != EWOULDBLOCK) {
            break;
        }

        // Push latest actuator command periodically
        double steer = 0.0;
        double accel = 0.0;
        {
            std::lock_guard<std::mutex> lock(mu_);
            steer = steer_cmd_;
            accel = accel_cmd_;
        }
        char out[128];
        const int m = std::snprintf(out, sizeof(out), "CMD %.6f %.6f\n", steer, accel);
        if (m > 0) {
            const ssize_t sent = ::send(client_fd, out, static_cast<size_t>(m), MSG_NOSIGNAL);
            if (sent < 0 && errno != EAGAIN && errno != EWOULDBLOCK) {
                break;
            }
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
}
