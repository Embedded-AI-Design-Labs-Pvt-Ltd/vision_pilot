#ifndef VISIONPILOT_HEADLESS_DISPLAY_HPP
#define VISIONPILOT_HEADLESS_DISPLAY_HPP

#include <visualization/visual_interface.hpp>

namespace visualization
{
    // No-op display for Docker / CI / virtual-hardware demos without X11.
    class HeadlessDisplay : public VisualInterface
    {
    public:
        HeadlessDisplay() = default;
        ~HeadlessDisplay() override = default;

        bool render_frame(const cv::Mat& display_frame) override;
        bool stop() override;
    };
}

#endif
