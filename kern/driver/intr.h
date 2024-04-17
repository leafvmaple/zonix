#pragma once

#include "def.h"

void intr_enable(void);
void intr_disable(void);

void pic_setmask(uint16_t mask);
void pic_enable(unsigned int irq);