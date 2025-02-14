# edge-vox
A lightweight C++ client library for real-time voice interaction with AI models running on edge servers. edge-vox enables bidirectional audio streaming and seamless integration with speech processing models like Whisper, LLaMA, and text-to-speech systems.

Extended documentation can be found [here](https://hcl-hbot.github.io/edge-vox)


## Overview

edge-vox provides a robust C++ interface for:
- Real-time audio streaming to and from edge servers
- Integration with speech-to-text (Whisper), language models (LLaMA), and text-to-speech services
- Low-latency voice chat capabilities optimized for edge computing
- Efficient binary protocols for audio transmission
- Cross-platform support for voice-enabled applications

**Key Features**
- Bidirectional audio streaming with configurable quality settings
- Asynchronous communication with edge AI services
- Built-in support for common audio formats and codecs
- Minimal dependencies and small footprint
- Comprehensive error handling and connection management
- Simple API for voice chat applications
- Creates ROS2 bindings as a separate component to keep the core EdgeVox library independent


## TODO

### Building
- [ ] Dowbload and build https://github.com/ros-drivers/audio_common.git as part of cmake
- [ ] Fix error: [ 33%] Building CXX object CMakeFiles/audio_capture_node.dir/src/audio_capture_node.cpp.o
/home/jeroen/ros2_ws/src/edgevox_ros/src/audio_capture_node.cpp:70:10: fatal error: rclcpp_components/register_node_macro.hpp: No such file or directory
   70 | #include "rclcpp_components/register_node_macro.hpp"
      |          ^~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
compilation terminated.


### ROS2
- [X] AudioCaptureNode
- [ ] VADNode 
- [ ] WakeWordNode
- [ ] ConversationManagerNode
- [ ] StreamTranscriptionNode


### Testing
- [ ] How to unit test audio_capture_node?


### Examples
- [ ] Create basic streaming example
- [ ] Create full chat application example


### Documentation
- [ ] usage_with_linux.md
- [ ] ROS_demo.md
- [ ] demo.md
- [ ] overview.md


### Clean up
- [ ] remove mosquitto stuff

