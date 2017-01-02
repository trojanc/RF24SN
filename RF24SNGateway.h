/*
  RF24SN.h - Alternative network library on top of RF24 library and nRF24l01 radio modules
  Created by Vaclav Synacek, 2014.
  Released under MIT license
 */

#ifndef RF24SNGateway_h
#define RF24SNGateway_h

#include "RF24SN.h"

class RF24SNGateway : public RF24SN {
public:
	RF24SNGateway(RF24* radio, RF24Network* network, RF24SNConfig* config, messageHandler onMessageHandler);

protected:
	void handleSubscribe(void);
};

#endif
