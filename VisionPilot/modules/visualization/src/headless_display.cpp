#include <visualization/headless_display.hpp>

namespace visualization
{
    bool HeadlessDisplay::render_frame(const cv::Mat& /*display_frame*/)
    {
        return true;
    }

    bool HeadlessDisplay::stop()
    {
        return true;
    }
}
