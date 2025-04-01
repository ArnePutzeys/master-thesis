#ifndef SGX_STEP_PF_ABSTRACTIONS_H
#define SGX_STEP_PF_ABSTRACTIONS_H

#include <stddef.h>
#include "pf.h"

void register_fault_handler_IDT(fault_handler_t cb);

#endif
