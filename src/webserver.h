#ifndef WEBSERVER_H

#define WEBSERVER_H

#include <Arduino.h>
#include "flashAppData.h"

bool ICACHE_FLASH_ATTR configureWebServices();
void ICACHE_FLASH_ATTR processWebClientRequests();
bool ICACHE_FLASH_ATTR isAuthenticated(const String &sessionCookieVal = EMPTY_STR);
void ICACHE_FLASH_ATTR redirect(const String &target, const String &contentType);

#endif // WEBSERVER_H
