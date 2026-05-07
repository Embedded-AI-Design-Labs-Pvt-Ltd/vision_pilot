# Camera Subscriber Module

## I. Overview

This camera subscriber module is a ROS2-based middleware component that does these following:

1. Subscribing and listening to an exposed ROS2 topic (for example, `sensor_msgs/image`) from any ROS2 image source (simulators like CARLA, or hardware cameras, rosbag playback, etc.).
2. Transforming received image messages into OpenCV format `cv::Mat` objects for processing.
3. Maintaining a thread-safe queue with time-critical buffer ti ensure low-latency processing.

Other features:

- Stream status tracking which confirms whether the ROS2 image message stream has been started.
- Validation of received frames, whether they are available and have been read appropriately.
- Able to handle multiple encodings, supporting multiple image formats like RGB, Mono, etc.
- Also some nice statistics of subscription health (frame reception, drop, error metrics etc.).

