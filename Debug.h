#ifndef __DEBUG_H__
#define __DEBUG_H__

#define DEBUG
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
#define LOG(x) Serial.print(x)
#include "Arduino.h"
#endif

#define FREEMEM() LOG(F("Free memory: ")); LOG(freeMemory()); LOG(F("\r\n"));

#else
#define LOG(x)
#endif

int freeMemory();

#endif
