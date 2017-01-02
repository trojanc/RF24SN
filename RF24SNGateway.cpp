/*
  RF24SN.cpp - Alternative network library on top of RF24 library and nRF24l01 radio modules
  Created by Vaclav Synacek, 2014.
  Released under MIT license
 */

#include "RF24SNGateway.h"
#include "RF24SN.h"

RF24SNGateway::RF24SNGateway(RF24* radio, RF24Network* network, RF24SNConfig* config, messageHandler onMessageHandler):RF24SN(radio, network, config, onMessageHandler)
{
}

void RF24SNGateway::handleSubscribe(void){
	RF24NetworkHeader header;
	RF24SNSubscribeRequest subscribeRequest;
	RF24SNMessage message;

	_network->read(header, &subscribeRequest, sizeof(RF24SNSubscribeRequest));
	// Send back ack
	delay(100);
	RF24SNSubscribeResponse response;
	response.topicId = 35;
	RF24NetworkHeader responseHeader(header.from_node, RF24SN_SUBACK);
	_network->write(responseHeader, &response, sizeof(RF24SNSubscribeResponse));
}


bool RF24SNGateway::handleMessage(bool swallowInvalid){
	bool handled = RF24SN::handleMessage(false);
	 if(!handled){
		RF24NetworkHeader header;
		uint16_t size = _network->peek(header);
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
