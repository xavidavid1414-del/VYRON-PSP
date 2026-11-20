#include "psp_memory_blocks.h"
#include <string.h>
#define PSP_USER_ARENA_OFFSET 0x00800000u
#define PSP_BLOCK_ALIGN 256u
static uint32_t align_up(uint32_t n) { return (n + PSP_BLOCK_ALIGN - 1u) & ~(PSP_BLOCK_ALIGN - 1u); }
void psp_memory_blocks_reset(PSP_MemoryBlocks *m) {
 if (!m) return;
 memset(m->blocks,0,sizeof(m->blocks)); m->next_uid=0x100u;
}
void psp_memory_blocks_set_floor(PSP_MemoryBlocks *m, uint32_t used_end_offset) {
 if (!m) return;
 uint32_t floor = align_up(used_end_offset);
 if (floor < PSP_USER_ARENA_OFFSET) floor = PSP_USER_ARENA_OFFSET;
 if (floor > m->arena_end) floor = m->arena_end;
 m->arena_start = floor;
 psp_memory_blocks_reset(m);
}
void psp_memory_blocks_init(PSP_MemoryBlocks *m, PSP_Memory *memory) {
 if (!m) return;
 memset(m,0,sizeof(*m)); m->memory=memory;
 m->arena_start=PSP_USER_ARENA_OFFSET;
 m->arena_end=memory && memory->size>m->arena_start ? memory->size : m->arena_start;
 psp_memory_blocks_reset(m);
}
static bool overlaps(PSP_MemoryBlocks *m,uint32_t start,uint32_t size) {
 uint64_t end=(uint64_t)start+size;
 for(unsigned i=0;i<PSP_MEMORY_BLOCK_MAX;i++) if(m->blocks[i].active) {
  uint64_t a=m->blocks[i].address-PSP_RAM_BASE,b=a+m->blocks[i].size;
  if(start<b && a<end) return true;
 }
 return false;
}
uint32_t psp_memory_block_alloc(PSP_MemoryBlocks *m,uint32_t partition,uint32_t type,uint32_t size,uint32_t address) {
 if(!m||!m->memory||!m->memory->ram||partition!=PSP_MEMORY_PARTITION_USER||!size||type>PSP_MEMORY_BLOCK_ADDR) return 0;
 uint32_t rounded=align_up(size); if(rounded<size) return 0;
 uint32_t chosen=0;
 if(type==PSP_MEMORY_BLOCK_ADDR) {
  if(address<PSP_RAM_BASE) return 0;
  chosen=address-PSP_RAM_BASE;
  if((chosen&(PSP_BLOCK_ALIGN-1u))||chosen<m->arena_start||chosen>m->arena_end||rounded>m->arena_end-chosen||overlaps(m,chosen,rounded)) return 0;
 } else if(type==PSP_MEMORY_BLOCK_LOW) {
  for(uint32_t p=align_up(m->arena_start);p<=m->arena_end && rounded<=m->arena_end-p;p+=PSP_BLOCK_ALIGN) if(!overlaps(m,p,rounded)){chosen=p;break;}
 } else {
  if(rounded<=m->arena_end-m->arena_start) {
   uint32_t p=(m->arena_end-rounded)&~(PSP_BLOCK_ALIGN-1u);
   for(;;) { if(p<m->arena_start) break; if(!overlaps(m,p,rounded)){chosen=p;break;} if(p<m->arena_start+PSP_BLOCK_ALIGN) break; p-=PSP_BLOCK_ALIGN; }
  }
 }
 if(!chosen) return 0;
 for(unsigned i=0;i<PSP_MEMORY_BLOCK_MAX;i++) if(!m->blocks[i].active) {
  uint32_t uid=m->next_uid++; if(uid<0x100u) {m->next_uid=0x101u;uid=0x100u;}
  m->blocks[i]=(PSP_MemoryBlock){uid,PSP_RAM_BASE+chosen,rounded,true}; return uid;
 }
 return 0;
}
bool psp_memory_block_free(PSP_MemoryBlocks *m,uint32_t uid) {
 if(!m||!uid) return false;
 for(unsigned i=0;i<PSP_MEMORY_BLOCK_MAX;i++) if(m->blocks[i].active&&m->blocks[i].uid==uid) {memset(&m->blocks[i],0,sizeof(m->blocks[i]));return true;}
 return false;
}
uint32_t psp_memory_block_head(PSP_MemoryBlocks *m,uint32_t uid) {
 if(!m||!uid) return 0;
 for(unsigned i=0;i<PSP_MEMORY_BLOCK_MAX;i++) if(m->blocks[i].active&&m->blocks[i].uid==uid) return m->blocks[i].address;
 return 0;
}
