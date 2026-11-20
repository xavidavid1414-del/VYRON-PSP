#include "psp_memory_hle.h"
#include "../cpu/psp_syscall.h"
#include "../loader/psp_nid.h"
#define SC_ALLOC 3u
#define SC_FREE 4u
#define SC_HEAD 5u
#define NID_ALLOC 0x237DBD4Fu
#define NID_FREE 0xB6D61D02u
#define NID_HEAD 0x9D9A5BA1u
static PSP_MemoryBlocks *active;
static bool alloc_call(PSP_CPU *c,PSP_Memory *m,uint32_t code) {
 if(!c||!m||!active||code!=SC_ALLOC)return false;
 uint32_t address=psp_memory_read32(m,c->gpr[29]+16u); c->gpr[2]=psp_memory_block_alloc(active,c->gpr[4],c->gpr[6],c->gpr[7],address); return true;
}
static bool free_call(PSP_CPU *c,PSP_Memory *m,uint32_t code) {
 (void)m;if(!c||!active||code!=SC_FREE)return false;
 c->gpr[2]=psp_memory_block_free(active,c->gpr[4])?0u:0x80020190u;return true;
}
static bool head_call(PSP_CPU *c,PSP_Memory *m,uint32_t code) {
 (void)m;if(!c||!active||code!=SC_HEAD)return false;
 uint32_t a=psp_memory_block_head(active,c->gpr[4]);c->gpr[2]=a?a:0x80020190u;return true;
}
bool psp_memory_hle_register(PSP_MemoryBlocks *m) {
 if(!m)return false;active=m;
 if(!psp_syscall_register(SC_ALLOC,alloc_call)||!psp_syscall_register(SC_FREE,free_call)||!psp_syscall_register(SC_HEAD,head_call)||
 !psp_nid_register(NID_ALLOC,SC_ALLOC)||!psp_nid_register(NID_FREE,SC_FREE)||!psp_nid_register(NID_HEAD,SC_HEAD)){psp_memory_hle_unregister();return false;}
 return true;
}
void psp_memory_hle_unregister(void) {
 psp_syscall_unregister(SC_ALLOC);psp_syscall_unregister(SC_FREE);psp_syscall_unregister(SC_HEAD);
 psp_nid_unregister(NID_ALLOC);psp_nid_unregister(NID_FREE);psp_nid_unregister(NID_HEAD);active=0;
}
