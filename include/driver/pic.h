#pragma once

#define I8259_MASTER_CMD 0x20  // Master (IRQs 0-7)
#define I8259_MASTER_IMR 0x21

#define I8259_SLAVE_CMD 0xA0   // Slave  (IRQs 8-15)
#define I8259_SLAVE_IMR 0xA1