# Overview

A lightweight C++ client library for real-time voice interaction with AI models running on edge servers. edge-vox enables bidirectional audio streaming and seamless integration with speech processing models like Whisper, LLaMA, and text-to-speech systems.

The library provides a robust C++ interface for:
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

## Prerequisites
<!-- 
Install Mosquitto MQTT Broker on Ubuntu 24.04

```console
sudo apt install -y mosquitto
```  -->

<!-- Install and Test the Mosquitto Clients

```console
sudo apt install -y mosquitto-clients
``` -->

Install Mosquitto dependencies

```bash
sudo apt-get install libmosquitto-dev pkg-config
```

For the tests to work, you'll need to install Google Test:

```bash
sudo apt-get install libgtest-dev
```

Install SDL2 development libraries:

```bash
sudo apt-get install libsdl2-dev
```

Or SDL3?

## Architecture

Key aspects of the library structure:

1. PIMPL Pattern: The Client class uses the PIMPL (Pointer to Implementation) idiom to hide implementation details and provide ABI stability.

2. Modular Design: The code is split into logical components:
    - core: Main client interface
    - audio: Audio processing functionality
    - net: Network-related code (RTP, control messages)

3. Configuration: Separate config structures for audio and streaming settings


The client implementation ties together all the components:

AudioProcessor for handling audio capture
RtpStreamer for sending audio data
ControlClient for MQTT communication

Key features of the implementation:

Thread-safe operations using atomic flags
RAII-style resource management
Error handling and state validation
Configuration management
Callback-based status updates

## API
The library API consists of the following functions:
<!-- ```cpp
namespace CLFML::LOWWI {

/**
 * @brief The user-provided struct which provides the classifier model settings
 * @param phrase The model identifier which get's passed into the callback function when triggered.
 * @param model_path The classifier model path
 * @param cbfunc Function pointer to a callback function that get's called when wakeword is triggered
 * @param cb_arg Additional function argument that get's passed in the callback when wakeword is triggered
 *               (void pointer)
 * @param refractory The negative feedback on activation, when activated this factor makes the debouncing work :)
 *                   Increasing it gives a higher negative bounty, thus dampening any further activations.
 *                   (Default = 20)
 *
 * @param threshold The threshold determines whether model confidence is worth acting on (default = 0.5f)
 * @param min_activations Number of activations the model should have to be considered detected
 *                       (Default = 5, but depends on how well the model is trained and how easy to detect)
 *                       (It's like a debouncing system)
 */
struct Lowwi_word_t
{
    std::string phrase = "";
    std::filesystem::path model_path = std::filesystem::path("");
    std::function<void(Lowwi_ctx_t, std::shared_ptr<void>)> cbfunc = nullptr;
    std::shared_ptr<void> cb_arg = nullptr;
    int refractory = 20;
    float threshold = 0.5f;
    uint8_t min_activations = 5;
    uint8_t debug = false;
};

/**
* @brief Add new wakeword to detection runtime
* @param lowwi_word Struct with the properties 
*                   of the to be added wakeword
*/
void Lowwi::add_wakeword(const Lowwi_word_t& lowwi_word);

/**
* @brief Remove wakeword from detection runtime
* @param model_path Model path of the to be removed wakeword
*/
void remove_wakeword(std::filesystem::path model_path);

/**
* @brief Runs wakeword detection runtime on audio samples
* @param audio_samples Audio samples to parse
*/
void Lowwi::run(const std::vector<float> &audio_samples);
}
``` -->
The comments above the functions describe fairly well what each function does. Here some additional notes;

## CMake integration
This project uses CMake for generating the build files. The CMakeLists is configured as follows:

### Target(s)
TODO
<!-- The main target defined in the CMakeLists is the `Lowwi` target. **As this will not be the only library released under the CLFML organisation, we chose to namespace it and call it `CLFML::Lowwi`**. 

Other targets which are defined in the CMake files of this project are the Unit tests. -->


### Configuration options
TODO
<!-- Some of the configuration options which can be used to generate the CMake project are:

- `CLFML_FACE_DETECTOR_BUILD_EXAMPLE_PROJECTS`; Build example projects (fragment & mic demo) (Default=ON, *only when project is not part of other project) -->


### Integrating it into your own project
Here are some CMake snippets which indicate how this project might be included into your own CMake project.

TODO

<!-- !!! example "Automatically fetching from GitHub"
    CPU only:
    ```cmake
    include(FetchContent)

    FetchContent_Declare(
     Lowwi
     GIT_REPOSITORY https://github.com/CLFML/lowwi.git
     GIT_TAG        main
    )
    FetchContent_MakeAvailable(Lowwi)

    ...

    target_link_libraries(YOUR_MAIN_EXECUTABLE_NAME CLFML::Lowwi)
    ```

!!! example "Manually using add_subdirectory"
    First make sure that this library is cloned into the project directory!
        CPU only:
    ```cmake
    add_subdirectory(lowwi)
    ...

    target_link_libraries(YOUR_MAIN_EXECUTABLE_NAME CLFML::Lowwi)
    ``` -->
