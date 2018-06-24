#include "RF24SNGateway.h"
#include "RF24SN.h"

RF24SNGateway::RF24SNGateway(RF24* radio, RF24Network* network, RF24SNConfig* config, messageHandler onMessageHandler, subsribeHandler onSubsribeHandler):RF24SN(radio, network, config, onMessageHandler){
	_onSubsribeHandler = onSubsribeHandler;
}

void RF24SNGateway::begin(void){
	RF24SN::begin();
}

void RF24SNGateway::update(void){
	RF24SN::update();
	RF24SNGateway::checkInactiveClients();
}

void RF24SNGateway::checkInactiveClients(void){
	// Check clients
	if(RF24SN::hasTimedout(lastInactiveCheck, RF24SN_CLIENT_INACTIVE_DELAY)){
		IF_RF24SN_DEBUG(
			Serial.print(F("Chk i/a "));
			Serial.println(lastInactiveCheck);
		);
		lastInactiveCheck = millis();
		for(byte clientIndex = 0 ; clientIndex < RF24SN_MAX_CLIENTS; clientIndex++){
			if(clients[clientIndex].clientId != RF24SN_CLIENT_EMPTY_ID
				&& RF24SN::hasTimedout(clients[clientIndex].lastActivity, RF24SN_CLIENT_INACTIVE_TIMEOUT)){

				IF_RF24SN_DEBUG(
					Serial.print(F("Clnt t/o: "));
					Serial.println(clients[clientIndex].clientId, DEC);
				);
				RF24SNGateway::resetClient(clientIndex);
			}
		}
	}
}

void RF24SNGateway::resetClient(byte clientIndex){
	IF_RF24SN_DEBUG(
		Serial.print(F("rst clnt : "));
		Serial.println(clientIndex, DEC);
	);
	for(int topicIndex = 0 ; topicIndex < RF24SN_MAX_CLIENT_TOPICS; topicIndex++){
		clients[clientIndex].topics[topicIndex].topicName[0] = '\0';
	}
	clients[clientIndex].clientId = RF24SN_CLIENT_EMPTY_ID;
	clients[clientIndex].topicCount = 0;
}

void RF24SNGateway::resetClients(void){
	for(int clientIndex = 0 ; clientIndex < RF24SN_MAX_CLIENTS; clientIndex++){
		RF24SNGateway::resetClient(clientIndex);
	}
}

byte RF24SNGateway::findClient(uint16_t clientId){
	byte clientIndex = RF24SN_CLIENT_NOT_FOUND_IDX;
	for(byte idx = 0 ; idx < RF24SN_MAX_CLIENTS; idx++){
		if(clients[idx].clientId == clientId){
			clientIndex = idx;
			break;
		}
	}
	return clientIndex;
}

byte RF24SNGateway::registerClient(uint16_t clientId){
	IF_RF24SN_DEBUG(Serial.println(F("Clnt reg : ")););
	byte clientIndex = RF24SN_CLIENT_NOT_FOUND_IDX;
	for(byte idx = 0 ; idx < RF24SN_MAX_CLIENTS; idx++){
		// See if we've got a free slot
		if(clients[idx].clientId == RF24SN_CLIENT_EMPTY_ID){
			clients[idx].clientId = clientId;
			clientIndex = idx;
			break;
		}
	}

	if(clientIndex == RF24SN_CLIENT_NOT_FOUND_IDX){
		IF_RF24SN_DEBUG(Serial.println(F("Clnt mx")););
	}
	else {
		IF_RF24SN_DEBUG(
			Serial.print(F("Clnt reg : "));
			Serial.println(clientIndex, DEC);
		);
	}
	return clientIndex;
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

	// Try and find existing client
	byte clientIndex = RF24SNGateway::findClient(header.from_node);

	IF_RF24SN_DEBUG(
		Serial.print(F("Clnt fnd: "));
		Serial.println(clientIndex, DEC);
	);

	// register new client
	if(clientIndex == RF24SN_CLIENT_NOT_FOUND_IDX){
		clientIndex = RF24SNGateway::registerClient(header.from_node);

		// If clientIdx is still -1 there is no more space for clients
		if(clientIndex == -1){
			return;
		}
	}

	// Update the last time we got a request from the client
	clients[clientIndex].lastActivity = millis();

	// Id of the topic to return to the client
	byte topicId = 0;

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

void RF24SNGateway::updateClientActivity(uint16_t clientId){
	// Try and find existing client
	byte clientIndex = RF24SNGateway::findClient(clientId);
	if(clientIndex != RF24SN_CLIENT_NOT_FOUND_IDX){
		clients[clientIndex].lastActivity = millis();
	}
}

bool RF24SNGateway::handleMessage(bool swallowInvalid){
	RF24NetworkHeader header;
	_network->peek(header);
	updateClientActivity(header.from_node);

	bool handled = RF24SN::handleMessage(false);

	if(!handled){
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
	for(int clientIndex = 0 ; clientIndex < RF24SN_MAX_CLIENTS; clientIndex++){
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
