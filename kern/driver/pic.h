#pragma once

#include "def.h"

void pic_setmask(uint16_t mask);
void pic_enable(unsigned int irq);

void pic_init(void);