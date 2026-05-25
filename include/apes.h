#ifndef APES_H
#define APES_H

#include "types.h"

void* female_ape_thread(void* arg);
void* male_ape_thread(void* arg);
void* baby_ape_thread(void* arg);

#endif
