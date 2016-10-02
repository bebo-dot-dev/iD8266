#ifndef WEBSERVER_H

#define WEBSERVER_H

#include <Arduino.h>
#include "flashAppData.h"

bool ICACHE_FLASH_ATTR configureWebServices();
void ICACHE_FLASH_ATTR processWebClientRequest();
bool ICACHE_FLASH_ATTR isAuthenticated(String sessionCookieVal = EMPTY_STR);

#endif // WEBSERVER_H
