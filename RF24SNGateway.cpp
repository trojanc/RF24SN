/*
  RF24SN.cpp - Alternative network library on top of RF24 library and nRF24l01 radio modules
  Created by Vaclav Synacek, 2014.
  Released under MIT license
 */

#include "RF24SNGateway.h"
#include "RF24SN.h"

RF24SNGateway::RF24SNGateway(RF24* radio, RF24Network* network, RF24SNConfig* config, messageHandler onMessageHandler, subsribeHandler onSubsribeHandler):RF24SN(radio, network, config, onMessageHandler)
{
	_onSubsribeHandler = onSubsribeHandler;
}

void RF24SNGateway::begin(void){
	RF24SN::begin();
	for(int clientIndex = 0 ; clientIndex < RF24SN_MAX_CLIENTS; clientIndex++){
		for(int topicIndex = 0 ; topicIndex < RF24SN_MAX_CLIENT_TOPICS; topicIndex++){
			clients[clientIndex].topics[topicIndex].topicName[0] = '\0';
		}
	}
}

void RF24SNGateway::handleSubscribe(void){
	RF24NetworkHeader header;
	RF24SNSubscribeRequest subscribeRequest;
	RF24SNMessage message;

	_network->read(header, &subscribeRequest, sizeof(RF24SNSubscribeRequest));

	int clientIndex = 0;
	bool found = false;
	byte topicId = 0; // Id of the topic to return to the client
	// Try and find existing client
	for( ; clientIndex < clientCount; clientIndex++){
		if(clients[clientIndex].clientId == header.from_node){
			found = true;
			break;
		}
	}

	// register new client
	if(!found){
		if(clientCount < RF24SN_MAX_CLIENTS){
			clientCount++;
			clients[clientIndex].clientId = header.from_node;
		}else{
			Serial.println(F("Clnt mx"));
			return;
		}
	}


	// Try and find existing topic
	int topicIndex = 0;
	bool foundTopic = false;
	for( ; topicIndex < clients[clientIndex].topicCount; topicIndex++){
		String topicString = String(clients[clientIndex].topics[topicIndex].topicName);
		if(topicString == subscribeRequest.topicName){
			foundTopic = true;
			break;
		}
	}
	// Register new topic
	if(!foundTopic){
		if(clients[clientIndex].topicCount < RF24SN_MAX_CLIENT_TOPICS){
			bool subcribed = _onSubsribeHandler(subscribeRequest.topicName);
			clients[clientIndex].topicCount = clients[clientIndex].topicCount + 1;
			clients[clientIndex].topics[topicIndex].topicName = String(subscribeRequest.topicName);
			clients[clientIndex].topics[topicIndex].topicId = topicIndex+1;
			topicId = topicIndex;


		}else{
			Serial.println(F("tpc mx"));
			return;
		}
	}
	// We already have this topic
	else{
		topicId = clients[clientIndex].topics[topicIndex].topicId;
	}
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
	bool hasClient = false;
	for(int clientIndex = 0 ; clientIndex < clientCount; clientIndex++){
		for(int topicIndex = 0 ; topicIndex < clients[clientIndex].topicCount; topicIndex++){
			if(clients[clientIndex].topics[topicIndex].topicName  == topic){
				RF24SNPacket requestPacket{36, value};
				sendRequest(clients[clientIndex].clientId, RF24SN_PUBLISH, &requestPacket, sizeof(RF24SNPacket), NULL, 0);
				hasClient = true;
			}
		}
	}
	return hasClient;
}
