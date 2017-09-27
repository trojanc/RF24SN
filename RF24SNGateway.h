#ifndef RF24SNGateway_h
#define RF24SNGateway_h

#include "RF24SN.h"

// Maximum number of clients that can be registered
#ifndef RF24SN_MAX_CLIENTS
#define RF24SN_MAX_CLIENTS 2
#endif

// Maximum number of topics a client can register for
#ifndef RF24SN_MAX_CLIENT_TOPICS
#define RF24SN_MAX_CLIENT_TOPICS 2
#endif

/**
 * A struct representing a topic that has been registered
 */
struct RF24SNTopicRegistration {
	/**
	 * Name of the topic on the MQTT protocol
	 */
	char topicName[RF24SN_TOPIC_LENGTH];

	/**
	 * ID of the topic on the RF24SN protocal
	 */
	uint8_t topicId = 0;
};

/**
 * A struct representing a registered client
 */
struct RF24SNClient{
	/**
	 * ID of the client
	 */
	uint16_t clientId = 0;

	/**
	 * Array of registered topics for this client
	 */
	RF24SNTopicRegistration topics[RF24SN_MAX_CLIENT_TOPICS];

	/**
	 * Number of topics that the client has registered
	 */
	byte topicCount = 0;
};

/**
 * Called when a client topic need to be subscribed
 */
typedef bool (*subsribeHandler)(const char* topic);

/**
 * A class representing a gateway on a RF24SN Network
 */
class RF24SNGateway : public RF24SN {
public:

	/**
	 * Creates a new instance of the gateway
	 */
	RF24SNGateway(RF24* radio, RF24Network* network, RF24SNConfig* config, messageHandler onMessageHandler, subsribeHandler onSubsribeHandler);

	/**
	 * Begin the gatewat
	 */
	void begin(void);

	/**
	 * Check which clients are subscribed to the topic, and forward the value to them
	 */
	bool checkSubscription(const char* topic, float value);
protected:

	/**
	 * Handler when a SN client requests to subscribe for a topic
	 */
	subsribeHandler _onSubsribeHandler;

	/**
	 * Reference to the clients connected to this gateway
	 */
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
