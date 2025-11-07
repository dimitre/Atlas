// https://art-net.org.uk/how-it-works/streaming-packets/artdmx-packet-definition/
#include "ofxNetwork.h"

struct artuniverse {
	static constexpr size_t ARTNET_HEADER_SIZE = 18;
	static constexpr size_t DMX_DATA_SIZE = 512;
	static constexpr size_t PACKET_SIZE = ARTNET_HEADER_SIZE + DMX_DATA_SIZE;
	static constexpr int ARTNET_PORT = 6454;

	uint8_t dataPacket[PACKET_SIZE];
	uint16_t universe;
	uint8_t sequence = 0;
	ofxUDPManager udp;
	std::string ip;
	bool isSetup = false;

	// Constructor with IP parameter
	artuniverse(uint16_t u, const std::string & targetIp = "127.0.0.1")
		: universe(u)
		, ip(targetIp) {

		ofxUDPSettings settings;
		settings.ttl = 1;
		settings.blocking = false;
		settings.sendTo(ip.c_str(), ARTNET_PORT);
		isSetup = udp.Setup(settings);

		if (!isSetup) {
			// ofLogError("artuniverse") << "Failed to setup UDP connection to " << ip;
			// ofLogError("artuniverse") << "Failed to setup UDP connection to " << ip;
		}

		cout << "Artnet connecting to " << ip << endl;
		initPacket(u);
	}

	~artuniverse() {
		udp.Close();
	}

	// Disable copy (UDP socket shouldn't be copied)
	artuniverse(const artuniverse &) = delete;
	artuniverse & operator=(const artuniverse &) = delete;

private:
	void initPacket(uint16_t u) {
		// Art-Net ID (7 bytes + null terminator)
		memcpy(dataPacket, "Art-Net", 8);

		// OpCode (0x5000 for OpDmx) - LITTLE ENDIAN
		dataPacket[8] = 0x00;
		dataPacket[9] = 0x50;

		// Protocol version (14 = 0x000E) - BIG ENDIAN
		dataPacket[10] = 0x00;
		dataPacket[11] = 0x0E;

		// Sequence (0 = disable sequencing)
		dataPacket[12] = 0;

		// Physical port
		dataPacket[13] = 0;

		// Universe (15-bit)
		setUniverse(u);

		// Data length (512) - BIG ENDIAN
		dataPacket[16] = 0x02;
		dataPacket[17] = 0x00;

		// Initialize DMX data to zero
		memset(&dataPacket[ARTNET_HEADER_SIZE], 0, DMX_DATA_SIZE);
	}

public:
	void setUniverse(uint16_t u) {
		universe = u & 0x7FFF;
		dataPacket[14] = universe & 0xFF;
		dataPacket[15] = (universe >> 8) & 0x7F;
	}

	void setChannel(uint16_t channel, uint8_t value) {
		if (channel < DMX_DATA_SIZE) {
			dataPacket[ARTNET_HEADER_SIZE + channel] = value;
		}
	}

	void setChannels(uint16_t startChannel, const uint8_t * data, size_t length) {
		size_t copyLength = std::min(length, DMX_DATA_SIZE - startChannel);
		if (copyLength > 0) {
			memcpy(&dataPacket[ARTNET_HEADER_SIZE + startChannel], data, copyLength);
		}
	}

	void incrementSequence() {
		sequence = (sequence + 1) % 256;
		dataPacket[12] = sequence;
	}

	void send() {
		if (isSetup && udp.HasSocket()) {
			incrementSequence();
			udp.Send((const char *)dataPacket, PACKET_SIZE);
		}
	}

	bool ready() const { return isSetup; }
};
