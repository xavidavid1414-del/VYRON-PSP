#ifndef PSP_NID_H
#define PSP_NID_H
#include <stdbool.h>
#include <stdint.h>
#define PSP_NID_MAX_ENTRIES 128u
typedef struct { uint32_t nid; uint32_t syscall_code; bool active; } PSP_NID_Entry;
void psp_nid_reset(void);
bool psp_nid_register(uint32_t nid, uint32_t syscall_code);
bool psp_nid_unregister(uint32_t nid);
bool psp_nid_resolve(uint32_t nid, uint32_t *syscall_code);
#endif
