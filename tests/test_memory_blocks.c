#include "../source/memory/psp_memory_blocks.h"
#include "../source/memory/psp_memory_hle.h"
#include "../source/cpu/psp_syscall.h"
#include "../source/loader/psp_nid.h"
#include <assert.h>
#include <stdlib.h>
int main(void){
 PSP_Memory mem={0};mem.ram=calloc(1,PSP_RAM_SIZE);mem.size=PSP_RAM_SIZE;assert(mem.ram);
 PSP_MemoryBlocks m;psp_memory_blocks_init(&m,&mem);
 uint32_t a=psp_memory_block_alloc(&m,2,PSP_MEMORY_BLOCK_LOW,1000,0);assert(a);
 uint32_t head=psp_memory_block_head(&m,a);assert(head==PSP_RAM_BASE+0x00800000u);
 uint32_t b=psp_memory_block_alloc(&m,2,PSP_MEMORY_BLOCK_HIGH,512,0);assert(b);assert(psp_memory_block_head(&m,b)>head);
 assert(!psp_memory_block_alloc(&m,1,0,100,0));assert(!psp_memory_block_alloc(&m,2,0,0,0));
 assert(!psp_memory_block_alloc(&m,2,PSP_MEMORY_BLOCK_ADDR,256,PSP_RAM_BASE));
 assert(psp_memory_block_free(&m,a));assert(!psp_memory_block_head(&m,a));assert(!psp_memory_block_free(&m,a));
 psp_syscall_reset();psp_nid_reset();assert(psp_memory_hle_register(&m));
 PSP_CPU cpu={0};cpu.gpr[4]=2;cpu.gpr[6]=PSP_MEMORY_BLOCK_ADDR;cpu.gpr[7]=300;cpu.gpr[29]=PSP_RAM_BASE+0x100;
 psp_memory_write32(&mem,cpu.gpr[29]+16,PSP_RAM_BASE+0x00802000u);
 uint32_t code=0;assert(psp_nid_resolve(0x237DBD4Fu,&code));assert(psp_syscall_dispatch(&cpu,&mem,code));uint32_t uid=cpu.gpr[2];assert(uid);
 assert(psp_nid_resolve(0x9D9A5BA1u,&code));cpu.gpr[4]=uid;assert(psp_syscall_dispatch(&cpu,&mem,code));assert(cpu.gpr[2]==PSP_RAM_BASE+0x00802000u);
 assert(psp_nid_resolve(0xB6D61D02u,&code));cpu.gpr[4]=uid;assert(psp_syscall_dispatch(&cpu,&mem,code));assert(cpu.gpr[2]==0);
 psp_memory_hle_unregister();psp_syscall_reset();psp_nid_reset();free(mem.ram);return 0;
}
