/*
 * sessionManager.h
 *
 *  Created on: 28 Feb 2016
 *      Author: joe
 */

#ifndef SESSIONMANAGER_H_
#define SESSIONMANAGER_H_

#include <Arduino.h>
#include "/home/joe/git/ESP8266TrueRandom/ESP8266TrueRandom.h"

#define MAX_WEB_SESSIONS 10
#define SESSION_KEY_LENGTH 10 //the length of the session cookie key 'sessionId='
#define SESSION_UUID_LENGTH 37 //the length of a session uuid + \0 e.g. '9c474b30-48c0-41bb-b54e-d84585c1022b\0'
#define SESSION_LENGTH /*msecs*/ ((1000 * 60) * 5) //5 mins

struct webSession {
	char sessionId[SESSION_UUID_LENGTH];
	unsigned long sessionMillis;
} __attribute__ ((__packed__));

class SessionManager {
public:
	String ICACHE_FLASH_ATTR addWebSession();
	bool ICACHE_FLASH_ATTR webSessionExists(String sessionId);
	bool ICACHE_FLASH_ATTR freeSession(String sessionId);
private:
	String ICACHE_FLASH_ATTR getSessionId();
};

extern SessionManager SessionMngr;

#endif /* SESSIONMANAGER_H_ */
