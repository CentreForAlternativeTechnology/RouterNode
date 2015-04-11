#ifndef __ARANDOM_H__
#define __ARANDOM_H__

/* Analog pin with long wire connected to it which provides entropy */
#define RAND_PIN A7

uint32_t arandom();

#endif