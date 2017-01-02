/*
  RF24SN.h - Alternative network library on top of RF24 library and nRF24l01 radio modules
  Created by Vaclav Synacek, 2014.
  Released under MIT license
 */

#ifndef RF24SNGateway_h
#define RF24SNGateway_h

#include "RF24SN.h"

// Maximum number of clients that can be registered
#ifndef RF24SN_MAX_CLIENTS
#define RF24SN_MAX_CLIENTS 5
#endif

// Maximum number of topics a client can register for
#ifndef RF24SN_MAX_CLIENT_TOPICS
#define RF24SN_MAX_CLIENT_TOPICS 5
#endif


struct RF24SNTopicRegistration {
	// Name of the topic on the MQTT protocal
	char* topicName;
	// ID of the topic on the RF24SN protocal
	uint8_t topicId;
};

struct RF24SNClient{
	uint16_t clientId;
	RF24SNTopicRegistration topics[RF24SN_MAX_CLIENT_TOPICS];
	// Number of topics that the client has registered
	byte topicIndex;
};

class RF24SNGateway : public RF24SN {
public:
	RF24SNGateway(RF24* radio, RF24Network* network, RF24SNConfig* config, messageHandler onMessageHandler);

protected:

	RF24SNClient clients[RF24SN_MAX_CLIENTS];

	/**
	 * Number of clients that has registered
	 */
	byte clientCount = 0;

	/**
	 * Handle a subscribe request
	 */
	void handleSubscribe(void);

	/**
	 * Handles any incomming message
	 */
	bool handleMessage(bool swallowInvalid = true);
};

#endif
