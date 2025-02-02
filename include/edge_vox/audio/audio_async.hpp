/*
 * Copyright (c) 2023-2024 GGerganov (Pulled from whisper.cpp project)
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

#ifndef AUDIO_ASYNC_HPP
#define AUDIO_ASYNC_HPP

#include <SDL.h>
#include <SDL_audio.h>

#include <atomic>
#include <cstdint>
#include <mutex>
#include <vector>

//
// SDL Audio capture
//

class audio_async {
public:
    audio_async(int len_ms);
    ~audio_async();

    bool init(int capture_id, int sample_rate);  // Keep old function for compatibility
    bool init(int capture_id, int playback_id, int sample_rate);

    // start capturing audio via the provided SDL callback
    // keep last len_ms seconds of audio in a circular buffer
    bool resume();
    bool pause();
    bool clear();
    bool close();

    // callback handlers to be called by SDL
    void capture_callback(uint8_t* stream, int len);
    void playback_callback(uint8_t* stream, int len);

    // get capture data from the circular buffer
    void get(int ms, std::vector<float>& audio);

    // Playback control
    bool start_playback();
    bool stop_playback();
    bool is_playing() const;
    bool play_audio(const std::vector<float>& audio);
    void clear_playback_buffer();
    size_t get_playback_buffer_size() const;

private:
    SDL_AudioDeviceID m_dev_id_in = 0;
    SDL_AudioDeviceID m_dev_id_out = 0;

    // Configuration
    int m_len_ms = 0;
    int m_sample_rate = 0;

    // State tracking
    std::atomic_bool m_running;
    std::atomic_bool m_playing;

    // Thread synchronization
    std::mutex m_mutex;

    std::vector<float> m_capture_buffer;
    std::vector<float> m_playback_buffer;
    size_t m_capture_buffer_pos = 0;
    size_t m_capture_buffer_len = 0;
};

// Return false if need to quit
bool sdl_poll_events();

#endif /* AUDIO_ASYNC_HPP */