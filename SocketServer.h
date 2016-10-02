/*
 * SocketServer.h
 *
 *  Created on: 27 Mar 2016
 *      Author: joe
 */

#ifndef SOCKETSERVER_H_
#define SOCKETSERVER_H_

#include "/home/joe/git/arduinoWebSockets/src/WebSocketsServer.h"
#include "GPIOManager.h"

class SocketServer {
public:
	ICACHE_FLASH_ATTR SocketServer(uint16_t socketServerPort);
	ICACHE_FLASH_ATTR virtual ~SocketServer();

	static bool ICACHE_FLASH_ATTR broadcastGPIOChange(gpioType type, uint8_t pinIdx);

	static void ICACHE_FLASH_ATTR handleRequests();
private:
	static bool ICACHE_FLASH_ATTR validateHttpHeader(String headerName, String headerValue);
};

#endif /* SOCKETSERVER_H_ */
