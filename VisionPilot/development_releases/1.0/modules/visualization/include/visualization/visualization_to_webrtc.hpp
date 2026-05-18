//
// Created by atanasko on 1.5.26.
// Developed by TranHuuNhatHuy on 18.5.26.
//

#ifndef VISIONPILOT_VISUALIZATION_TO_WEBRTC_H
#define VISIONPILOT_VISUALIZATION_TO_WEBRTC_H

#include <opencv2/opencv.hpp>
#include <cstdint>
#include <memory>
#include <string>


namespace visualization {

    class VisualizationToWebRTC {
    public:
        VisualizationToWebRTC();
        ~VisualizationToWebRTC();

        void sendFrame(const cv::Mat& frame);
        void setWebRTCConfig(const std::string& config);

    private:
        // Internal implementation details (e.g., WebRTC connection, encoding, etc.)
        struct Impl;
        std::unique_ptr<Impl> impl_;
    };

}


#endif //VISIONPILOT_VISUALIZATION_TO_WEBRTC_H
