#include "edgevox_ros/audio_capture_node.hpp"

namespace edgevox_ros {

AudioCaptureNode::AudioCaptureNode(const rclcpp::NodeOptions& options)
    : Node("audio_capture_node", options) {
    // Declare parameters
    this->declare_parameter("sample_rate", 16000);
    this->declare_parameter("buffer_ms", 30);
    this->declare_parameter("publish_rate", 100.0);  // Hz

    // Get parameters
    int sample_rate = this->get_parameter("sample_rate").as_int();
    int buffer_ms = this->get_parameter("buffer_ms").as_int();
    double publish_rate = this->get_parameter("publish_rate").as_double();

    // Initialize audio capture
    audio_capture_ = std::make_unique<audio_async>(buffer_ms);
    if (!audio_capture_->init(-1, sample_rate)) {
        RCLCPP_ERROR(this->get_logger(), "Failed to initialize audio capture");
        return;
    }

    // Create publisher
    audio_pub_ = this->create_publisher<audio_common_msgs::msg::AudioData>("audio_raw", 10);

    // Create timer for regular audio publishing
    timer_ = this->create_wall_timer(std::chrono::duration<double>(1.0 / publish_rate),
                                     std::bind(&AudioCaptureNode::publish_audio, this));

    // Start audio capture
    if (!audio_capture_->resume()) {
        RCLCPP_ERROR(this->get_logger(), "Failed to start audio capture");
        return;
    }

    RCLCPP_INFO(this->get_logger(), "Audio capture node initialized");
}

AudioCaptureNode::~AudioCaptureNode() {
    if (audio_capture_) {
        audio_capture_->pause();
    }
}

void AudioCaptureNode::publish_audio() {
    if (!audio_capture_)
        return;

    std::vector<float> audio_data;
    audio_capture_->get(this->get_parameter("buffer_ms").as_int(), audio_data);

    if (!audio_data.empty()) {
        auto msg = std::make_unique<audio_common_msgs::msg::AudioData>();

        // Convert float samples to bytes (16-bit PCM)
        msg->data.reserve(audio_data.size() * 2);
        for (float sample : audio_data) {
            int16_t pcm = static_cast<int16_t>(sample * 32767.0f);
            msg->data.push_back(static_cast<uint8_t>(pcm & 0xFF));
            msg->data.push_back(static_cast<uint8_t>((pcm >> 8) & 0xFF));
        }

        audio_pub_->publish(std::move(msg));
    }
}

}  // namespace edgevox_ros

#include "rclcpp_components/register_node_macro.hpp"
RCLCPP_COMPONENTS_REGISTER_NODE(edgevox_ros::AudioCaptureNode)