#pragma once

struct EdgeVoxAudioConfig {
    uint32_t sample_rate{48000};
    uint16_t channels{1};
    uint16_t bits_per_sample{16};
    uint32_t buffer_ms{30};
};