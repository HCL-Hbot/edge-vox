#pragma once

#include <audio_common_msgs/msg/audio_data.hpp>
#include <memory>
#include <rclcpp/rclcpp.hpp>

#include "edge_vox/audio/audio_async.hpp"

namespace edgevox_ros {

class AudioCaptureNode : public rclcpp::Node {
public:
    explicit AudioCaptureNode(const rclcpp::NodeOptions& options = rclcpp::NodeOptions());
    ~AudioCaptureNode();

private:
    void publish_audio();

    std::unique_ptr<audio_async> audio_capture_;
    rclcpp::Publisher<audio_common_msgs::msg::AudioData>::SharedPtr audio_pub_;
    rclcpp::TimerBase::SharedPtr timer_;
};

}  // namespace edgevox_ros