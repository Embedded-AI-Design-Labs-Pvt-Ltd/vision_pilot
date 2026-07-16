#include <iostream>
#include <stdexcept>
#include <fstream>
#include <cstdlib>
#include <cstring>

#include <vehicle_interface/file_interface.hpp>
#include <vehicle_interface/can_interface.hpp>

FileInterface::FileInterface(const std::string& filename)
{
    std::ifstream file(filename);
    if (file.is_open())
    {
        std::string line;
        while (std::getline(file, line))
        {
            if (line.find_first_not_of(" \t\r\n") == std::string::npos)
            {
                continue;
            }
            try
            {
                double value = std::stod(line);
                speeds_.push_back(value);
            }
            catch (const std::exception& e)
            {
                std::cerr << "Warning: skipping invalid line: \"" << line
                    << "\" (" << e.what() << ")" << std::endl;
            }
        }

        file.close();
    }

    // Tee actuator commands to the TCP vehicle gateway so a virtual MCU/ECU
    // (or real Arduino/ESP32/Zephyr board) can demonstrate closed-loop I/O
    // while ego speed still comes from the recorded/synthetic speed file.
    const char *tee = std::getenv("VP_TEE_VEHICLE_GATEWAY");
    if (tee && (std::strcmp(tee, "1") == 0 || std::strcmp(tee, "true") == 0))
    {
        gateway_ = std::make_unique<CanInterface>();
    }
}

FileInterface::~FileInterface() = default;

double FileInterface::read()
{
    if (speeds_.empty())
    {
        throw std::runtime_error("FileInterface: no speeds loaded");
    }

    if (frame_cnt_ >= static_cast<int>(speeds_.size()))
    {
        throw std::runtime_error("FileInterface: read() called past end of speeds data");
    }

    return speeds_[frame_cnt_++];
}

void FileInterface::write(double steering, double acceleration)
{
    if (gateway_)
    {
        gateway_->write(steering, acceleration);
    }
}
