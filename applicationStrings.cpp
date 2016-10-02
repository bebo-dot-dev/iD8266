/*
 * applicationStrings.cpp
 *
 *  Created on: 14 May 2016
 *      Author: joe
 */

#include "applicationStrings.h"

#define ENTRY(enumVal, str) const char CLI_STR_ ## enumVal[] PROGMEM = str;
STRING_TABLE
#undef ENTRY

const char* const appStrings[] PROGMEM = {
#define ENTRY(enumVal, str) CLI_STR_ ## enumVal,
		STRING_TABLE
#undef ENTRY
};

static char appStrBuffer[70];

String getAppStr(appStrType strType) {

	strcpy_P(appStrBuffer, (char*)pgm_read_dword(&(appStrings[strType])));
	return String(appStrBuffer);
}


