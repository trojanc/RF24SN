/*
  RF24SN.cpp - Alternative network library on top of RF24 library and nRF24l01 radio modules
  Created by Vaclav Synacek, 2014.
  Released under MIT license
 */

#include "Arduino.h"
#include "RF24SN.h"

RF24SN::RF24SN(RF24* radio, RF24Network* network, RF24SNConfig* config, messageHandler onMessageHandler)
{
	//initialize private variables
	_radio = radio;
	_network = network;
	_config = config;
	_onMessageHandler = onMessageHandler;
#ifdef RF24SN_HAS_LEDS
	_ledFlags = 0x00;
#endif
}


//initialize underlaying RF24 driver
void RF24SN::begin()
{

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
	_radio->printDetails();
}

bool RF24SN::publish(uint8_t sensorId, float value){
	return publish(sensorId, value, 1);
}

bool RF24SN::publish(uint8_t sensorId, float value, int retries)
{
	RF24SNPacket sendPacket{sensorId, value};
	RF24SNPacket responsePacket;
	bool gotResponse = sendRequest(RF24SN_PUBLISH, &sendPacket, &responsePacket, retries);
	if(!gotResponse){
		Serial.println(F("NO PUBACK"));
	}
	return gotResponse;
}


//send the packet to base, wait for ack-packet received back
bool RF24SN::sendRequest(uint8_t messageType, RF24SNPacket* requestPacket, RF24SNPacket* responsePacket)
{

	RF24NetworkHeader networkHeader(_config->baseNodeAddress, messageType);
	//this will be returned at the end. if no ack packet comes back, then "no packet" packet will be returned
	_network->write(networkHeader, requestPacket, sizeof(RF24SNPacket));
#ifdef RF24SN_HAS_LEDS
	_ledFlags |= LEDF_FLASH_TX;
	updateLeds();
#endif
	bool gotPacket = waitForPacket(getAckType(networkHeader.type), responsePacket);
	return gotPacket;
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
//send the packet to base, wait for ack-packet received back and check it, optionally resent if ack does not match
bool RF24SN::sendRequest(uint8_t messageType, RF24SNPacket* requestPacket, RF24SNPacket* responsePacket, int retries)
{
	bool gotResponse = false;
	//loop until no retires are left or until successfully acked.
	for(int transmission = 1; (transmission <= retries) && !gotResponse ; transmission++)
	{
		gotResponse = sendRequest(messageType, requestPacket, responsePacket);
	}

	return gotResponse;
}
void RF24SN::handleIncommingMessage(void){
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


bool RF24SN::waitForPacket(uint8_t type, RF24SNPacket* responsePacket)
{
	//wait until response is awailable or until timeout
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
			// We might receive a publish message while waiting for a ack
			if(header.type == RF24SN_PUBLISH){
				RF24SN::handleIncommingMessage();
			}
			else if(header.type == type){
				_network->read(header, responsePacket, sizeof(RF24SNPacket));
				return true;
			}
			else{
				_network->read(header, NULL, 0);
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
		RF24NetworkHeader header;
		_network->peek(header);

		// We might receive a publish message while waiting for a ack
		if(header.type == RF24SN_PUBLISH){
			RF24SN::handleIncommingMessage();
		}
		else{
			_network->read(header, NULL, 0);
			Serial.print("Inv MSG");
			Serial.println(header.type);
		}
	}
#ifdef RF24SN_HAS_LEDS
	updateLeds();
#endif
}

//utility method to pretty print packet contents
void RF24SN::printPacketDetails(RF24SNPacket packet){
	Serial.print("\tsensorId ");
	Serial.println(packet.sensorId);
	Serial.print("\tvalue ");
	Serial.println(packet.value);
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

