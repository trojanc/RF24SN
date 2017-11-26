#include "RF24SNGateway.h"
#include "RF24SN.h"

RF24SNGateway::RF24SNGateway(RF24* radio, RF24Network* network, RF24SNConfig* config, messageHandler onMessageHandler, subsribeHandler onSubsribeHandler):RF24SN(radio, network, config, onMessageHandler){
	_onSubsribeHandler = onSubsribeHandler;
}

void RF24SNGateway::begin(void){
	RF24SN::begin();
}

void RF24SNGateway::resetSubscriptions(void)
{
	for(int clientIndex = 0 ; clientIndex < clientCount; clientIndex++){
		for(int topicIndex = 0 ; topicIndex < RF24SN_MAX_CLIENT_TOPICS; topicIndex++){
			clients[clientIndex].topics[topicIndex].topicName[0] = '\0';
		}
		clients[clientIndex].clientId = 0;
		clients[clientIndex].topicCount = 0;
	}
	clientCount = 0;
}

void RF24SNGateway::handleSubscribe(void)
{
	RF24NetworkHeader header;
	RF24SNSubscribeRequest subscribeRequest;

	// Read the full request
	_network->read(header, &subscribeRequest, sizeof(RF24SNSubscribeRequest));

	IF_RF24SN_DEBUG(
		Serial.print(F("Sbcr "));
		Serial.print(header.from_node);
		Serial.print(F(" - "));
		Serial.println(subscribeRequest.topicName);
	);

	// Index of this client in the list of current clients
	int clientIndex = 0;

	// Flag if we have found the user yet
	bool found = false;

	// Id of the topic to return to the client
	byte topicId = 0;

	// Try and find existing client
	IF_RF24SN_DEBUG(Serial.print(F("Clnts"));Serial.println(clientCount, DEC););
	for( ; clientIndex < clientCount; clientIndex++){
		if(clients[clientIndex].clientId == header.from_node){
			found = true;
			break;
		}
	}
	IF_RF24SN_DEBUG(
		Serial.print(F("Clnt fnd: "));
		Serial.print(found);
		Serial.print(F(" : "));
		Serial.println(clientCount, DEC);
	);
	// register new client
	if(!found){
		if(clientCount < RF24SN_MAX_CLIENTS){
			clientCount++;
			clients[clientIndex].clientId = header.from_node;
			IF_RF24SN_DEBUG(
				Serial.print(F("Clnt reg : "));
				Serial.println(clientIndex, DEC);
				Serial.print(F("Clnt cnt : "));
				Serial.println(clientCount, DEC);
			);
		}else{
			IF_RF24SN_DEBUG(Serial.println(F("Clnt mx")););
			return;
		}
	}

	// Index of the topic for the current client
	int topicIndex = 0;

	// Flag if we have found the topic
	bool foundTopic = false;

	// Try and find existing topic
	for( ; topicIndex < clients[clientIndex].topicCount; topicIndex++){
		if(strcmp(clients[clientIndex].topics[topicIndex].topicName, subscribeRequest.topicName) == 0){
			foundTopic = true;
			break;
		}
	}
	IF_RF24SN_DEBUG(
		Serial.print(F("tpc fnd: "));
		Serial.println(foundTopic);
	);
	// Register new topic
	if(!foundTopic){
		if(clients[clientIndex].topicCount < RF24SN_MAX_CLIENT_TOPICS){
			// Subscribe to the new topic
			bool subscribed = _onSubsribeHandler(subscribeRequest.topicName);
			if(subscribed){
				clients[clientIndex].topicCount = clients[clientIndex].topicCount + 1;
				strcpy(clients[clientIndex].topics[topicIndex].topicName, subscribeRequest.topicName);
				clients[clientIndex].topics[topicIndex].topicId = topicIndex+1;
				topicId = clients[clientIndex].topics[topicIndex].topicId;
				IF_RF24SN_DEBUG(
					Serial.print(F("Tpc reg : "));
					Serial.println(topicId, DEC);
				);
			}
			else{
				IF_RF24SN_DEBUG(
					Serial.println(F("Tpc reg f"));
				);
				return; // TODO implement below
				//				RF24NetworkHeader responseHeader(header.from_node, RF24SN_SUBNACK);
				//				_network->write(responseHeader, NULL, 0);
			}
		}else{
			IF_RF24SN_DEBUG(Serial.println(F("tpc mx")););
			// TODO implement below
			//			RF24NetworkHeader responseHeader(header.from_node, RF24SN_SUBNACK);
			//			_network->write(responseHeader, NULL, 0);
			return;
		}
	}
	// We already have this topic
	else{
		topicId = clients[clientIndex].topics[topicIndex].topicId;
	}
	IF_RF24SN_DEBUG(
		Serial.print(F("tpc id: "));
		Serial.println(topicId);
	);
	// Send back ack
	delay(100);
	RF24SNSubscribeResponse response;
	response.topicId = topicId;
	RF24NetworkHeader responseHeader(header.from_node, RF24SN_SUBACK);
	_network->write(responseHeader, &response, sizeof(RF24SNSubscribeResponse));
}

bool RF24SNGateway::handleMessage(bool swallowInvalid){
	bool handled = RF24SN::handleMessage(false);
	if(!handled){
		RF24NetworkHeader header;
		_network->peek(header);
		if(header.type == RF24SN_SUBSCRIBE){
			RF24SNGateway::handleSubscribe();
			handled = true;
		}
		else if(swallowInvalid){
			RF24NetworkHeader requestHeader;
			_network->read(requestHeader, NULL, 0);
		}
	}
	return handled;
}

bool RF24SNGateway::checkSubscription(const char* topic, float value){
	IF_RF24SN_DEBUG(
		Serial.print(F("chk sbr "));
		Serial.println(topic);
	);
	bool hasClient = false;
	for(int clientIndex = 0 ; clientIndex < clientCount; clientIndex++){
		for(int topicIndex = 0 ; topicIndex < clients[clientIndex].topicCount; topicIndex++){
			if(strcmp(clients[clientIndex].topics[topicIndex].topicName, topic) == 0){
				IF_RF24SN_DEBUG(
					Serial.print(F("fwd sbr "));
					Serial.print(clients[clientIndex].clientId, DEC);
					Serial.print(F(" tpc "));
					Serial.println(clients[clientIndex].topics[topicIndex].topicId, DEC);
					Serial.print(F(" cid "));
					Serial.println(clients[clientIndex].clientId, DEC);
				);
				RF24SNPacket requestPacket{clients[clientIndex].topics[topicIndex].topicId, value};
				sendRequest(clients[clientIndex].clientId, RF24SN_PUBLISH, &requestPacket, sizeof(RF24SNPacket), NULL, 0);
				hasClient = true;
			}
		}
	}
	return hasClient;
}
