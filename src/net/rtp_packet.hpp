#pragma once

#include <chrono>
#include <cstdint>
#include <random>
#include <vector>

namespace {
// RTP version (2 bits): This implementation uses RTP version 2
constexpr uint8_t RTP_VERSION = 2;

// Default values for RTP
constexpr uint8_t AUDIO_PAYLOAD_TYPE = 11;  // Dynamic payload type for audio
constexpr uint32_t SAMPLING_RATE = 48000;   // Default sampling rate
}  // namespace

class RtpPacket {
public:
    struct Header {
        // First byte
        uint8_t version : 2;    // Protocol version (2)
        uint8_t padding : 1;    // Padding flag
        uint8_t extension : 1;  // Extension flag
        uint8_t csrcCount : 4;  // CSRC count

        // Second byte
        uint8_t marker : 1;       // Marker bit
        uint8_t payloadType : 7;  // Payload type

        // Remaining fixed header fields
        uint16_t sequenceNumber;  // Sequence number
        uint32_t timestamp;       // Timestamp
        uint32_t ssrc;            // Synchronization source identifier

        // Optional CSRC list
        std::vector<uint32_t> csrcList;
    };

    RtpPacket() {
        initHeader();
    }

    void setPayload(const std::vector<uint8_t>& data) {
        payload_ = data;
    }

    std::vector<uint8_t> serialize() const {
        std::vector<uint8_t> packet;

        // Reserve space for the header and payload
        packet.reserve(12 + header_.csrcList.size() * 4 + payload_.size());

        // First byte
        uint8_t firstByte = (header_.version << 6) | (header_.padding << 5) |
                            (header_.extension << 4) | (header_.csrcCount & 0x0F);
        packet.push_back(firstByte);

        // Second byte
        uint8_t secondByte = (header_.marker << 7) | (header_.payloadType & 0x7F);
        packet.push_back(secondByte);

        // Sequence number (network byte order)
        packet.push_back((header_.sequenceNumber >> 8) & 0xFF);
        packet.push_back(header_.sequenceNumber & 0xFF);

        // Timestamp (network byte order)
        for (int i = 3; i >= 0; --i) {
            packet.push_back((header_.timestamp >> (i * 8)) & 0xFF);
        }

        // SSRC (network byte order)
        for (int i = 3; i >= 0; --i) {
            packet.push_back((header_.ssrc >> (i * 8)) & 0xFF);
        }

        // Add CSRC list if present
        for (uint32_t csrc : header_.csrcList) {
            for (int i = 3; i >= 0; --i) {
                packet.push_back((csrc >> (i * 8)) & 0xFF);
            }
        }

        // Add payload
        packet.insert(packet.end(), payload_.begin(), payload_.end());

        return packet;
    }

    void incrementSequenceNumber() {
        header_.sequenceNumber++;
    }

    void incrementTimestamp(uint32_t samples) {
        header_.timestamp += samples;
    }

    void setMarker(bool marker) {
        header_.marker = marker;
    }

    void addCsrc(uint32_t csrc) {
        header_.csrcList.push_back(csrc);
        header_.csrcCount = static_cast<uint8_t>(header_.csrcList.size());
    }

    const Header& getHeader() const {
        return header_;
    }
    const std::vector<uint8_t>& getPayload() const {
        return payload_;
    }

private:
    void initHeader() {
        header_.version = RTP_VERSION;
        header_.padding = 0;
        header_.extension = 0;
        header_.csrcCount = 0;
        header_.marker = 0;
        header_.payloadType = AUDIO_PAYLOAD_TYPE;
        header_.sequenceNumber = generateInitialSequenceNumber();
        header_.timestamp = 0;
        header_.ssrc = generateSSRC();
    }

    uint16_t generateInitialSequenceNumber() {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<uint16_t> dis;
        return dis(gen);
    }

    uint32_t generateSSRC() {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<uint32_t> dis;
        return dis(gen);
    }

    Header header_;
    std::vector<uint8_t> payload_;
};