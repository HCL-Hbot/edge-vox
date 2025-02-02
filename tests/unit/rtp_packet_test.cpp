#include "net/rtp_packet.hpp"

#include <gtest/gtest.h>

class RtpPacketTest : public ::testing::Test {
protected:
    void SetUp() override {
        packet = std::make_unique<RtpPacket>();
    }

    void TearDown() override {
        packet.reset();
    }

    // Helper to extract header fields from serialized packet
    struct ParsedHeader {
        uint8_t version;
        uint8_t padding;
        uint8_t extension;
        uint8_t csrcCount;
        uint8_t marker;
        uint8_t payloadType;
        uint16_t sequenceNumber;
        uint32_t timestamp;
        uint32_t ssrc;
        std::vector<uint32_t> csrcList;
    };

    ParsedHeader parsePacketHeader(const std::vector<uint8_t>& data) {
        EXPECT_GE(data.size(), 12u);  // Minimum RTP header size

        ParsedHeader header;

        // First byte
        header.version = (data[0] >> 6) & 0x03;
        header.padding = (data[0] >> 5) & 0x01;
        header.extension = (data[0] >> 4) & 0x01;
        header.csrcCount = data[0] & 0x0F;

        // Second byte
        header.marker = (data[1] >> 7) & 0x01;
        header.payloadType = data[1] & 0x7F;

        // Sequence number (network byte order)
        header.sequenceNumber = (data[2] << 8) | data[3];

        // Timestamp
        header.timestamp = (data[4] << 24) | (data[5] << 16) | (data[6] << 8) | data[7];

        // SSRC
        header.ssrc = (data[8] << 24) | (data[9] << 16) | (data[10] << 8) | data[11];

        // CSRC List if present
        for (int i = 0; i < header.csrcCount; i++) {
            size_t offset = 12 + i * 4;
            EXPECT_LE(offset + 4, data.size());
            uint32_t csrc = (data[offset] << 24) | (data[offset + 1] << 16) |
                            (data[offset + 2] << 8) | data[offset + 3];
            header.csrcList.push_back(csrc);
        }

        return header;
    }

    std::vector<uint8_t> extractPayload(const std::vector<uint8_t>& packetData) {
        auto header = parsePacketHeader(packetData);
        size_t headerSize = 12 + header.csrcCount * 4;
        EXPECT_LE(headerSize, packetData.size());
        return std::vector<uint8_t>(packetData.begin() + headerSize, packetData.end());
    }

    std::unique_ptr<RtpPacket> packet;
};

TEST_F(RtpPacketTest, DefaultHeaderInitialization) {
    auto serialized = packet->serialize();
    auto header = parsePacketHeader(serialized);

    // Check RTP version 2
    EXPECT_EQ(header.version, 2);

    // Check default values
    EXPECT_EQ(header.padding, 0);
    EXPECT_EQ(header.extension, 0);
    EXPECT_EQ(header.csrcCount, 0);
    EXPECT_EQ(header.marker, 0);
    EXPECT_EQ(header.payloadType, 11);  // Default audio payload type
    EXPECT_TRUE(header.csrcList.empty());
}

TEST_F(RtpPacketTest, SequenceNumberHandling) {
    auto serialized1 = packet->serialize();
    auto header1 = parsePacketHeader(serialized1);

    packet->incrementSequenceNumber();

    auto serialized2 = packet->serialize();
    auto header2 = parsePacketHeader(serialized2);

    EXPECT_EQ(header2.sequenceNumber, header1.sequenceNumber + 1);
}

TEST_F(RtpPacketTest, SequenceNumberWraparound) {
    // Get initial sequence number
    auto initialSeq = parsePacketHeader(packet->serialize()).sequenceNumber;
    std::cout << "Initial sequence number: " << initialSeq << std::endl;

    // Increment until just before wrap
    uint16_t targetSeq = 0xFFFF;
    uint16_t currentSeq = initialSeq;

    std::cout << "Target sequence number: " << targetSeq << std::endl;

    while (currentSeq < targetSeq) {
        packet->incrementSequenceNumber();
        currentSeq = parsePacketHeader(packet->serialize()).sequenceNumber;
        if (currentSeq % 1000 == 0) {
            std::cout << "Current sequence number: " << currentSeq << std::endl;
        }
    }

    auto beforeWrap = parsePacketHeader(packet->serialize()).sequenceNumber;
    std::cout << "Before wrap: " << beforeWrap << std::endl;

    packet->incrementSequenceNumber();
    auto afterWrap = parsePacketHeader(packet->serialize()).sequenceNumber;
    std::cout << "After wrap: " << afterWrap << std::endl;

    EXPECT_EQ(beforeWrap, 0xFFFF) << "Expected to reach max value before wrap";
    EXPECT_EQ(afterWrap, 0) << "Expected to wrap to 0";
}

TEST_F(RtpPacketTest, TimestampIncrement) {
    auto serialized1 = packet->serialize();
    auto header1 = parsePacketHeader(serialized1);

    uint32_t sampleIncrement = 480;  // 10ms at 48kHz
    packet->incrementTimestamp(sampleIncrement);

    auto serialized2 = packet->serialize();
    auto header2 = parsePacketHeader(serialized2);

    EXPECT_EQ(header2.timestamp, header1.timestamp + sampleIncrement);
}

TEST_F(RtpPacketTest, MarkerBitHandling) {
    EXPECT_FALSE(parsePacketHeader(packet->serialize()).marker);

    packet->setMarker(true);
    EXPECT_TRUE(parsePacketHeader(packet->serialize()).marker);

    packet->setMarker(false);
    EXPECT_FALSE(parsePacketHeader(packet->serialize()).marker);
}

TEST_F(RtpPacketTest, CsrcListManagement) {
    std::vector<uint32_t> csrcs = {0x12345678, 0x87654321};

    for (auto csrc : csrcs) {
        packet->addCsrc(csrc);
    }

    auto header = parsePacketHeader(packet->serialize());

    EXPECT_EQ(header.csrcCount, csrcs.size());
    EXPECT_EQ(header.csrcList, csrcs);
}

TEST_F(RtpPacketTest, MaximumCsrcCount) {
    // Try to add more than maximum allowed CSRCs (15)
    for (int i = 0; i < 20; ++i) {
        packet->addCsrc(i);
    }

    auto header = parsePacketHeader(packet->serialize());
    EXPECT_LE(header.csrcCount, 15);
}

TEST_F(RtpPacketTest, PayloadHandling) {
    // Test empty payload
    packet->setPayload({});
    EXPECT_TRUE(extractPayload(packet->serialize()).empty());

    // Test normal payload
    std::vector<uint8_t> testPayload = {1, 2, 3, 4, 5};
    packet->setPayload(testPayload);
    EXPECT_EQ(extractPayload(packet->serialize()), testPayload);

    // Test large payload
    std::vector<uint8_t> largePayload(1000, 0x42);
    packet->setPayload(largePayload);
    EXPECT_EQ(extractPayload(packet->serialize()), largePayload);
}

TEST_F(RtpPacketTest, HeaderConstancy) {
    // The SSRC should remain constant for the lifetime of the packet
    auto ssrc1 = parsePacketHeader(packet->serialize()).ssrc;
    auto ssrc2 = parsePacketHeader(packet->serialize()).ssrc;
    EXPECT_EQ(ssrc1, ssrc2);
}

TEST_F(RtpPacketTest, PacketSerialization) {
    packet->setMarker(true);
    packet->addCsrc(0x12345678);
    std::vector<uint8_t> payload = {0xAA, 0xBB, 0xCC};
    packet->setPayload(payload);

    auto serialized = packet->serialize();

    // Verify minimum packet size
    EXPECT_GE(serialized.size(), 12u + 4u);  // Header + 1 CSRC

    auto header = parsePacketHeader(serialized);
    auto extractedPayload = extractPayload(serialized);

    EXPECT_TRUE(header.marker);
    EXPECT_EQ(header.csrcCount, 1);
    EXPECT_EQ(header.csrcList[0], 0x12345678);
    EXPECT_EQ(extractedPayload, payload);
}

TEST_F(RtpPacketTest, NetworkByteOrder) {
    // Test that multi-byte fields are in network byte order (big-endian)
    uint16_t seqNum = 0x1234;
    uint32_t timestamp = 0x12345678;

    // Set sequence number through multiple increments
    while (parsePacketHeader(packet->serialize()).sequenceNumber != seqNum) {
        packet->incrementSequenceNumber();
    }

    // Set timestamp
    packet->incrementTimestamp(timestamp);

    auto serialized = packet->serialize();

    // Check sequence number bytes
    EXPECT_EQ(serialized[2], 0x12);
    EXPECT_EQ(serialized[3], 0x34);

    // Check timestamp bytes
    EXPECT_EQ(serialized[4], 0x12);
    EXPECT_EQ(serialized[5], 0x34);
    EXPECT_EQ(serialized[6], 0x56);
    EXPECT_EQ(serialized[7], 0x78);
}