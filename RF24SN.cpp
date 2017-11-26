#include "Arduino.h"
#include "RF24SN.h"

RF24SN::RF24SN(RF24* radio, RF24Network* network, RF24SNConfig* config, messageHandler onMessageHandler){
	//initialize private variables
	_radio = radio;
	_network = network;
	_config = config;
	_onMessageHandler = onMessageHandler;
#ifdef RF24SN_HAS_LEDS
	_ledFlags = 0x00;
#endif
}


void RF24SN::begin(){
#ifdef RF24SN_HAS_LEDS
	pinMode(PIN_LED_TX, OUTPUT);
	pinMode(PIN_LED_RX, OUTPUT);
	digitalWrite(PIN_LED_TX, LOW);
	digitalWrite(PIN_LED_RX, LOW);
#endif

	// Start the radio
	_radio->begin();
	_network->begin(_config->radioChannel, _config->nodeAddress);
	_radio->setDataRate(_config->radioDatarate);
	_radio->setPALevel(_config->radioPaLevel);
	_radio->setAutoAck(false);
}

byte RF24SN::subscribe(const char* topic){
	byte response = RF24SN_RSP_FAILED;
	RF24SNSubscribeRequest sendPacket;
	if(strlen(topic) > RF24SN_TOPIC_LENGTH){
		return RF24SN_RSP_FAILED;
	}
	strcpy(sendPacket.topicName, topic);
	sendPacket.topicName[strlen(topic)] = '\0'; // TODO is this required?
	IF_RF24SN_DEBUG(Serial.print(F("Sub ")); Serial.println(sendPacket.topicName););
	RF24SNSubscribeResponse responsePacket;
	bool gotResponse = sendRequest(_config->baseNodeAddress, RF24SN_SUBSCRIBE, &sendPacket, sizeof(RF24SNSubscribeRequest), &responsePacket, sizeof(RF24SNSubscribeResponse), 5);
	if(gotResponse){
		response = responsePacket.topicId;
	}else{
		IF_RF24SN_DEBUG(Serial.println(F("NO SUBACK")));
	}
	return response;
}

bool RF24SN::publish(uint16_t nodeId, uint8_t sensorId, float value){
	return publish(nodeId, sensorId, value, 1);
}

bool RF24SN::publish(uint16_t nodeId, uint8_t sensorId, float value, int retries){
	RF24SNPacket sendPacket{sensorId, value};
	RF24SNPacket responsePacket;
	bool gotResponse = sendRequest(nodeId, RF24SN_PUBLISH, &sendPacket, sizeof(RF24SNPacket), &responsePacket, sizeof(RF24SNPacket), retries);
	if(!gotResponse){
		IF_RF24SN_DEBUG(Serial.println(F("NO PUBACK")));
	}
	return gotResponse;
}


//send the packet to base, wait for ack-packet received back
bool RF24SN::sendRequest(uint16_t nodeId, uint8_t messageType, const void* requestPacket, uint16_t reqLen, void* responsePacket, uint16_t resLen){

#ifdef RF24SN_HAS_LEDS
	_ledFlags |= LEDF_FLASH_TX;
	updateLeds();
#endif
	bool gotPacket = waitForPacket(getAckType(networkHeader.type), responsePacket, resLen);
	return gotPacket;
}

//send the packet to base, wait for ack-packet received back and check it, optionally resent if ack does not match
bool RF24SN::sendRequest(uint16_t nodeId, uint8_t messageType, const void* requestPacket, uint16_t reqLen, void* responsePacket, uint16_t resLen, int retries){
	bool gotResponse = false;
	//loop until no retires are left or until successfully acked.
	for(int transmission = 1; (transmission <= retries) && !gotResponse ; transmission++){
		gotResponse = sendRequest(nodeId, messageType, requestPacket, reqLen, responsePacket, resLen);
	}
	return gotResponse;
}


uint8_t RF24SN::getAckType(uint8_t request){
	if(request == RF24SN_PUBLISH){
		return RF24SN_PUBACK;
	}
	else if(request == RF24SN_SUBSCRIBE){
		return RF24SN_SUBACK;
	}
	else if(request == RF24SN_PINGREQ){
		return RF24SN_PINGRES;
	}
	return 0;
}


bool RF24SN::handleMessage(bool swallowInvalid){
	RF24NetworkHeader header;
	_network->peek(header);
	if(header.type == RF24SN_PUBLISH){
		RF24SN::handlePublishMessage();
		return true;
	}
	else if(swallowInvalid){
		_network->read(header, NULL, 0);
	}
	return false;
}


void RF24SN::handlePublishMessage(void){
	RF24NetworkHeader header;
	RF24SNMessage message;
	_network->read(header, &message.packet, sizeof(RF24SNPacket));
	message.fromNode = header.from_node;
	message.messageType = header.type;
	_onMessageHandler(message);

	// Send back ack
	delay(100);
	RF24NetworkHeader responseHeader(header.from_node, RF24SN_PUBACK);
	_network->write(responseHeader, NULL, 0);
}


bool RF24SN::waitForPacket(uint8_t type, void* responsePacket, uint16_t resLen){
	//wait until response is available or until timeout
	//the timeout period is random as to minimize repeated collisions.
	unsigned long started_waiting_at = millis();
	bool timeout = false;
	int random_wait_time = random(2000,3000);

	RF24NetworkHeader header;

	while(!timeout){
		// Keep updating the network
		_network->update();

		// Check if there is a packet available
		if(_network->available()){
			_network->peek(header);
#ifdef RF24SN_HAS_LEDS
	_ledFlags |= LEDF_FLASH_RX;
	updateLeds();
#endif
			if(header.type == type){
				_network->read(header, responsePacket, resLen);
				return true;
			}
			else{
				handleMessage(true);
			}
		}

		// Check if we need to give up
		if (millis() - started_waiting_at > random_wait_time ){
			timeout = true;
		}
	}

	return false;
}
void RF24SN::update(void){
	_network->update();
	if(_network->available()){
#ifdef RF24SN_HAS_LEDS
	_ledFlags |= LEDF_FLASH_RX;
	updateLeds();
#endif

		// We might receive a publish message while waiting for a ack
		handleMessage(true);
	}
#ifdef RF24SN_HAS_LEDS
	updateLeds();
#endif
}

#ifdef RF24SN_HAS_LEDS
void RF24SN::updateLeds(void){
	if((_ledFlags & LEDF_FLASH_RX)){
		digitalWrite(PIN_LED_RX, HIGH);
	}
	if((_ledFlags & LEDF_FLASH_TX)){
		digitalWrite(PIN_LED_TX, HIGH);
	}
	if((_ledFlags & LEDF_FLASH_RX) || (_ledFlags & LEDF_FLASH_TX)){
		delay(10);
		_ledFlags &= ~LEDF_FLASH_RX;
		_ledFlags &= ~LEDF_FLASH_TX;
		digitalWrite(PIN_LED_TX, LOW);
		digitalWrite(PIN_LED_RX, LOW);
	}
}
#endif

