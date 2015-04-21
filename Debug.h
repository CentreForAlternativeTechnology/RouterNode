#ifndef __DEBUG_H__
#define __DEBUG_H__

#ifdef DEBUG

#ifdef LINUX
#include <string>
#include <ostream>
#include <iostream>
#include <cstring>
#include <stdint.h>
#define LOG(x) std::cout << (x)
#ifndef F
#define F(x) x
#endif
#else
#include "Arduino.h"
#define LOG(x) Serial.print(x);
#define DEBUG_INIT Serial.begin(115200)
#endif

#define FREEMEM() LOG(F("Free memory: ")); LOG(freeMemory()); LOG(F("\r\n"));

#else
#define LOG(x)
#define DEBUG_INIT
#endif

int freeMemory();

#endif
