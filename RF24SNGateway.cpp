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

}
