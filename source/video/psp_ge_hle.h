#ifndef PSP_GE_HLE_H
#define PSP_GE_HLE_H

#include <stdbool.h>
#include <stdint.h>
#include "psp_ge.h"

#define PSP_GE_HLE_MAX_LISTS 16u

typedef struct {
    uint32_t id;
    uint32_t address;
    bool active;
    bool completed;
    bool succeeded;
} PSP_GE_HLE_List;

typedef struct {
    PSP_GE *ge;
    PSP_GE_HLE_List lists[PSP_GE_HLE_MAX_LISTS];
    bool initialized;
} PSP_GE_HLE;

void psp_ge_hle_init(PSP_GE_HLE *service, PSP_GE *ge);
void psp_ge_hle_reset(PSP_GE_HLE *service);
bool psp_ge_hle_register(PSP_GE_HLE *service);
void psp_ge_hle_unregister(void);

#endif
