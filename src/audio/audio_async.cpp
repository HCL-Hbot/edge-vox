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

#include "edge_vox/audio/audio_async.hpp"

audio_async::audio_async(int len_ms) {
    m_len_ms = len_ms;

    m_running = false;
}

audio_async::~audio_async() {
    if (m_dev_id_in) {
        SDL_CloseAudioDevice(m_dev_id_in);
    }
    if (m_dev_id_out) {
        SDL_CloseAudioDevice(m_dev_id_out);
    }
}

bool audio_async::init(int capture_id, int sample_rate) {
    // Call new init function with default playback device (-1)
    return init(capture_id, -1, sample_rate);
}

bool audio_async::init(int capture_id, int playback_id, int sample_rate) {
    SDL_LogSetPriority(SDL_LOG_CATEGORY_APPLICATION, SDL_LOG_PRIORITY_INFO);

    if (SDL_Init(SDL_INIT_AUDIO) < 0) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Couldn't initialize SDL: %s\n", SDL_GetError());
        return false;
    }

    SDL_SetHintWithPriority(SDL_HINT_AUDIO_RESAMPLING_MODE, "medium", SDL_HINT_OVERRIDE);

    // List capture devices
    {
        int nDevices = SDL_GetNumAudioDevices(SDL_TRUE);
        fprintf(stderr, "%s: found %d capture devices:\n", __func__, nDevices);
        for (int i = 0; i < nDevices; i++) {
            fprintf(stderr, "%s:    - Capture device #%d: '%s'\n", __func__, i,
                    SDL_GetAudioDeviceName(i, SDL_TRUE));
        }
    }

    // List playback devices
    {
        int nDevices = SDL_GetNumAudioDevices(SDL_FALSE);
        fprintf(stderr, "%s: found %d playback devices:\n", __func__, nDevices);
        for (int i = 0; i < nDevices; i++) {
            fprintf(stderr, "%s:    - Playback device #%d: '%s'\n", __func__, i,
                    SDL_GetAudioDeviceName(i, SDL_FALSE));
        }
    }

    SDL_AudioSpec capture_spec_requested;
    SDL_AudioSpec capture_spec_obtained;
    SDL_AudioSpec playback_spec_requested;
    SDL_AudioSpec playback_spec_obtained;

    SDL_zero(capture_spec_requested);
    SDL_zero(capture_spec_obtained);
    SDL_zero(playback_spec_requested);
    SDL_zero(playback_spec_obtained);

    // Configure capture spec
    capture_spec_requested.freq = sample_rate;
    capture_spec_requested.format = AUDIO_F32;
    capture_spec_requested.channels = 1;
    capture_spec_requested.samples = 1024;
    capture_spec_requested.callback = [](void *userdata, uint8_t *stream, int len) {
        audio_async *audio = (audio_async *)userdata;
        audio->capture_callback(stream, len);
    };
    capture_spec_requested.userdata = this;

    // Configure playback spec
    playback_spec_requested.freq = sample_rate;
    playback_spec_requested.format = AUDIO_F32;
    playback_spec_requested.channels = 1;
    playback_spec_requested.samples = 1024;
    playback_spec_requested.callback = [](void *userdata, uint8_t *stream, int len) {
        audio_async *audio = (audio_async *)userdata;
        audio->playback_callback(stream, len);
    };
    playback_spec_requested.userdata = this;

    // Open capture device
    if (capture_id >= 0) {
        fprintf(stderr, "%s: attempt to open capture device %d : '%s' ...\n", __func__, capture_id,
                SDL_GetAudioDeviceName(capture_id, SDL_TRUE));
        m_dev_id_in = SDL_OpenAudioDevice(SDL_GetAudioDeviceName(capture_id, SDL_TRUE), SDL_TRUE,
                                          &capture_spec_requested, &capture_spec_obtained, 0);
    } else {
        fprintf(stderr, "%s: attempt to open default capture device ...\n", __func__);
        m_dev_id_in = SDL_OpenAudioDevice(nullptr, SDL_TRUE, &capture_spec_requested,
                                          &capture_spec_obtained, 0);
    }

    if (!m_dev_id_in) {
        fprintf(stderr, "%s: couldn't open an audio device for capture: %s!\n", __func__,
                SDL_GetError());
        return false;
    }

    // Open playback device
    if (playback_id >= 0) {
        fprintf(stderr, "%s: attempt to open playback device %d : '%s' ...\n", __func__,
                playback_id, SDL_GetAudioDeviceName(playback_id, SDL_FALSE));
        m_dev_id_out =
            SDL_OpenAudioDevice(SDL_GetAudioDeviceName(playback_id, SDL_FALSE), SDL_FALSE,
                                &playback_spec_requested, &playback_spec_obtained, 0);
    } else {
        fprintf(stderr, "%s: attempt to open default playback device ...\n", __func__);
        m_dev_id_out = SDL_OpenAudioDevice(nullptr, SDL_FALSE, &playback_spec_requested,
                                           &playback_spec_obtained, 0);
    }

    if (!m_dev_id_out) {
        fprintf(stderr, "%s: couldn't open an audio device for playback: %s!\n", __func__,
                SDL_GetError());
        // Close capture device since we failed
        if (m_dev_id_in) {
            SDL_CloseAudioDevice(m_dev_id_in);
            m_dev_id_in = 0;
        }
        return false;
    }

    m_sample_rate = capture_spec_obtained.freq;
    m_capture_buffer.resize((m_sample_rate * m_len_ms) / 1000);
    m_playback_buffer.reserve(m_sample_rate);  // Reserve 1 second worth of samples

    return true;
}

bool audio_async::resume() {
    bool success = true;

    if (m_dev_id_in) {
        SDL_PauseAudioDevice(m_dev_id_in, 0);
    }

    if (m_dev_id_out) {
        SDL_PauseAudioDevice(m_dev_id_out, 0);
    }

    if (!m_dev_id_in && !m_dev_id_out) {
        fprintf(stderr, "%s: no audio devices available\n", __func__);
        success = false;
    }

    if (m_running && success) {
        fprintf(stderr, "%s: devices already running\n", __func__);
        // Still return true since this isn't a failure
    }

    m_running = true;
    return success;
}

bool audio_async::pause() {
    bool success = true;

    if (m_dev_id_in) {
        SDL_PauseAudioDevice(m_dev_id_in, 1);
    }

    if (m_dev_id_out) {
        SDL_PauseAudioDevice(m_dev_id_out, 1);
    }

    if (!m_dev_id_in && !m_dev_id_out) {
        fprintf(stderr, "%s: no audio devices available\n", __func__);
        success = false;
    }

    if (!m_running && success) {
        fprintf(stderr, "%s: devices already paused\n", __func__);
        // Still return true since this isn't a failure
    }

    m_running = false;
    m_playing = false;
    return success;
}

bool audio_async::clear() {
    if (!m_dev_id_in) {
        fprintf(stderr, "%s: no audio device to clear!\n", __func__);
        return false;
    }

    if (!m_running) {
        fprintf(stderr, "%s: not running!\n", __func__);
        return false;
    }

    {
        std::lock_guard<std::mutex> lock(m_mutex);

        m_capture_buffer_pos = 0;
        m_capture_buffer_len = 0;
    }

    return true;
}

bool audio_async::close() {
    if (!m_dev_id_in && !m_dev_id_out) {
        fprintf(stderr, "%s: no audio devices to close!\n", __func__);
        return false;
    }

    if (m_dev_id_in) {
        SDL_CloseAudioDevice(m_dev_id_in);
        m_dev_id_in = 0;
    }

    if (m_dev_id_out) {
        SDL_CloseAudioDevice(m_dev_id_out);
        m_dev_id_out = 0;
    }

    SDL_QuitSubSystem(SDL_INIT_AUDIO);
    SDL_Quit();

    return true;
}

// callback to be called by SDL
void audio_async::capture_callback(uint8_t *stream, int len) {
    if (!m_running) {
        return;
    }

    size_t n_samples = len / sizeof(float);

    if (n_samples > m_capture_buffer.size()) {
        n_samples = m_capture_buffer.size();

        stream += (len - (n_samples * sizeof(float)));
    }

    {
        std::lock_guard<std::mutex> lock(m_mutex);

        if (m_capture_buffer_pos + n_samples > m_capture_buffer.size()) {
            const size_t n0 = m_capture_buffer.size() - m_capture_buffer_pos;

            memcpy(&m_capture_buffer[m_capture_buffer_pos], stream, n0 * sizeof(float));
            memcpy(&m_capture_buffer[0], stream + n0 * sizeof(float),
                   (n_samples - n0) * sizeof(float));

            m_capture_buffer_pos = (m_capture_buffer_pos + n_samples) % m_capture_buffer.size();
            m_capture_buffer_len = m_capture_buffer.size();
        } else {
            memcpy(&m_capture_buffer[m_capture_buffer_pos], stream, n_samples * sizeof(float));

            m_capture_buffer_pos = (m_capture_buffer_pos + n_samples) % m_capture_buffer.size();
            m_capture_buffer_len =
                std::min(m_capture_buffer_len + n_samples, m_capture_buffer.size());
        }
    }
}

void audio_async::get(int ms, std::vector<float> &result) {
    if (!m_dev_id_in) {
        fprintf(stderr, "%s: no audio device to get audio from!\n", __func__);
        return;
    }

    if (!m_running) {
        fprintf(stderr, "%s: not running!\n", __func__);
        return;
    }

    result.clear();

    {
        std::lock_guard<std::mutex> lock(m_mutex);

        if (ms <= 0) {
            ms = m_len_ms;
        }

        size_t n_samples = (m_sample_rate * ms) / 1000;
        if (n_samples > m_capture_buffer_len) {
            n_samples = m_capture_buffer_len;
        }

        result.resize(n_samples);

        signed long long s0 = m_capture_buffer_pos - n_samples;
        if (s0 < 0) {
            s0 += m_capture_buffer.size();
        }

        if (s0 + n_samples > m_capture_buffer.size()) {
            const size_t n0 = m_capture_buffer.size() - s0;

            memcpy(result.data(), &m_capture_buffer[s0], n0 * sizeof(float));
            memcpy(&result[n0], &m_capture_buffer[0], (n_samples - n0) * sizeof(float));
        } else {
            memcpy(result.data(), &m_capture_buffer[s0], n_samples * sizeof(float));
        }
    }
}

void audio_async::playback_callback(uint8_t *stream, int len) {
    if (!m_running || m_playback_buffer.empty()) {
        // Fill with silence if no data
        memset(stream, 0, len);
        return;
    }

    std::lock_guard<std::mutex> lock(m_mutex);
    size_t samples_needed = len / sizeof(float);
    size_t samples_to_copy = std::min(samples_needed, m_playback_buffer.size());

    // Copy available samples
    memcpy(stream, m_playback_buffer.data(), samples_to_copy * sizeof(float));

    // Fill remaining with silence if we run out of data
    if (samples_to_copy < samples_needed) {
        memset(stream + (samples_to_copy * sizeof(float)), 0,
               (samples_needed - samples_to_copy) * sizeof(float));
    }

    // Remove used samples from buffer
    m_playback_buffer.erase(m_playback_buffer.begin(), m_playback_buffer.begin() + samples_to_copy);
}

bool audio_async::play_audio(const std::vector<float> &audio) {
    if (!m_dev_id_out) {
        return false;
    }

    std::lock_guard<std::mutex> lock(m_mutex);
    m_playback_buffer.insert(m_playback_buffer.end(), audio.begin(), audio.end());
    return true;
}

bool audio_async::start_playback() {
    if (!m_dev_id_out) {
        fprintf(stderr, "%s: no playback device available\n", __func__);
        return false;
    }

    SDL_PauseAudioDevice(m_dev_id_out, 0);
    m_playing = true;
    return true;
}

bool audio_async::stop_playback() {
    if (!m_dev_id_out) {
        fprintf(stderr, "%s: no playback device available\n", __func__);
        return false;
    }

    SDL_PauseAudioDevice(m_dev_id_out, 1);
    m_playing = false;
    return true;
}

bool audio_async::is_playing() const {
    return m_dev_id_out != 0 && m_playing;
}