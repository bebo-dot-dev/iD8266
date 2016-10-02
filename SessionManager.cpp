/*
 * sessionManager.cpp
 *
 *  Created on: 28 Feb 2016
 *      Author: joe
 */

#include "SessionManager.h"
#include "flashAppData.h"

SessionManager SessionMngr;

String ICACHE_FLASH_ATTR SessionManager::getSessionId() {
	byte uuidNumber[16];
	ESP8266TrueRandom.uuid(uuidNumber);
	return ESP8266TrueRandom.uuidToString(uuidNumber);
}

String ICACHE_FLASH_ATTR SessionManager::addWebSession()
{
	byte freeSlotPos = 0;
	bool hasFreeSlot = false;
	webSession *sessionSlot;

	for (freeSlotPos = 0; freeSlotPos < MAX_WEB_SESSIONS; freeSlotPos++) {

		sessionSlot = &appConfigData.webSessions[freeSlotPos];

		if ((sessionSlot->sessionId == 0) ||
			(sessionSlot-> sessionMillis < millis()) ||
			(abs(sessionSlot-> sessionMillis - millis()) > SESSION_LENGTH))
		{
			hasFreeSlot = true;
			break;
		}
		yield();
	}

	if (hasFreeSlot)
	{
		strncpy(sessionSlot->sessionId, getSessionId().c_str(), SESSION_UUID_LENGTH);
		sessionSlot->sessionMillis = millis() + SESSION_LENGTH;
		return sessionSlot->sessionId;
	}
	return EMPTY_STR;
}

bool ICACHE_FLASH_ATTR SessionManager::webSessionExists(String sessionId) {

  webSession *s;
  if (!sessionId.equals(EMPTY_STR)) {
	  for (int i = 0; i < MAX_WEB_SESSIONS; i++)
	  {
		s = &appConfigData.webSessions[i];
		if (strcmp(s->sessionId, sessionId.c_str()) == 0) {

		  if (s->sessionMillis >= millis()) {
			//session exists and is still active so implement the sliding window
			s->sessionMillis = millis() + SESSION_LENGTH;
			return true;
		  } else {
			//session exists but it has expired so free up the slot
			  strncpy(s->sessionId, EMPTY_STR, SESSION_UUID_LENGTH);
			s->sessionMillis = 0;
			return false;
		  }
		}
		yield();
	  }
  }
  return false;

}

bool ICACHE_FLASH_ATTR SessionManager::freeSession(String sessionId) {

  webSession *s;
  for (int i = 0; i < MAX_WEB_SESSIONS; i++)
  {
    s = &appConfigData.webSessions[i];
    if (strcmp(s->sessionId, sessionId.c_str()) == 0) {
        //session exists so free up the slot
    	strncpy(s->sessionId, EMPTY_STR, SESSION_UUID_LENGTH);
        s->sessionMillis = 0;
        return true;
    }
    yield();
  }
  return false;

}

