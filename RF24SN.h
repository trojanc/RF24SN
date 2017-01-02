/*
  RF24SN.h - Alternative network library on top of RF24 library and nRF24l01 radio modules
  Created by Vaclav Synacek, 2014.
  Released under MIT license
 */

#ifndef RF24SN_h
#define RF24SN_h

#include <Arduino.h>
#include <stdint.h>
#include "RF24.h"
#include "RF24Network.h"

//#define PIN_LED_TX 4
//#define PIN_LED_RX 5

#if defined(PIN_LED_TX) && defined(PIN_LED_RX)
#define RF24SN_HAS_LEDS
#define LEDF_FLASH_TX 0x01
#define LEDF_FLASH_RX 0x02
#endif

// Max length for a topic
#ifndef RF24SN_TOPIC_LENGTH
#define RF24SN_TOPIC_LENGTH 20
#endif

typedef enum {
	RF24SN_PUBLISH = 0x0C, // Publish / Receive data
	RF24SN_PUBACK = 0x0D,
	RF24SN_SUBSCRIBE = 0x12,
	RF24SN_SUBACK = 0x13,
	RF24SN_PINGREQ = 0x16,
	RF24SN_PINGRES = 0x17
} MsgTypes;

struct __attribute__((__packed__))  RF24SNSubscribeRequest{
	char topicName[RF24SN_TOPIC_LENGTH]; // name of the topic to subscribe
};

struct __attribute__((__packed__))  RF24SNSubscribeResponse{
	byte topicId;		// Id of the topic for publish requests
};

struct __attribute__((__packed__))  RF24SNPacket{
	uint8_t sensorId;    //sensor id
	float value;         //sensor reading
};

struct __attribute__((__packed__)) RF24SNMessage{
	uint8_t messageType;		// Message Type
	uint8_t fromNode;			// Node that sent the message
	RF24SNPacket packet;		//sensor reading
};


typedef struct {
	uint16_t baseNodeAddress; 		// Node where the gateway is
	uint16_t nodeAddress; 			// Address of this node
	rf24_datarate_e radioDatarate;	// Data rate for node
	uint8_t radioPaLevel;			// PA level for radio
	uint8_t radioChannel;			// Channel to use for radio
} RF24SNConfig;

typedef void (*messageHandler)(RF24SNMessage&);

class RF24SN
{
public:
	RF24SN(RF24* radio, RF24Network* network, RF24SNConfig* config, messageHandler onMessageHandler);
	void begin(void);
	bool publish(uint8_t sensorId, float value);
	bool publish(uint8_t sensorId, float value, int retries);

	/**
	 * Subscribes for a topic
	 * Returns the id of the topic which will be used for published messages
	 *
	 * returns -127 if failed
	 */
	byte subscribe(const char* topic);
	void update(void);

protected:
	RF24* _radio;
	RF24Network* _network;
	RF24SNConfig* _config;
	messageHandler _onMessageHandler;
	uint8_t getAckType(uint8_t request);

	/**
	 * Sends a request to the broker
	 * @param messageType The message type to send in the header
	 * @param requestPacket Optional request data to send with
	 * @param responsePacket Optional pointer to where reponse data must be saved
	 *
	 * @return True if the expected response is received in time
	 */
	bool sendRequest(uint8_t messageType, const void* requestPacket, uint16_t reqLen, const void* responsePacket, uint16_t resLen);
	bool sendRequest(uint8_t messageType, const void* requestPacket, uint16_t reqLen, const void* responsePacket, uint16_t resLen, int retries);
	bool waitForPacket(uint8_t type, const void* responsePacket, uint16_t resLen);

	/**
	 * Handles any incomming message
	 */
	virtual bool handleMessage(bool swallowInvalid = true);
	/**
	 * Handle a published message
	 */
	void handlePublishMessage(void);

private:

#ifdef RF24SN_HAS_LEDS
	byte _ledFlags;
	void updateLeds(void);
#endif
};

#endif
