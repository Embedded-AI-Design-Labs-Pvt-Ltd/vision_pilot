#ifndef VISIONPILOT_FILE_INTERFACE_HPP
#define VISIONPILOT_FILE_INTERFACE_HPP

#include <memory>
#include <string>
#include <vector>
#include <vehicle_interface/vehicle_interface.hpp>

class CanInterface;

class FileInterface : public VehicleInterface
{
public:
    FileInterface(const std::string& filename);
    ~FileInterface() override;

    double read() override;
    void write(double steering, double acceleration) override;

private:
    std::vector<double> speeds_;
    int frame_cnt_ = 0;
    // Optional TCP vehicle gateway tee for virtual MCU demos
    std::unique_ptr<CanInterface> gateway_;
};

#endif //VISIONPILOT_FILE_INTERFACE_HPP
