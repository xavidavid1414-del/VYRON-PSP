#include "../source/loader/psp_io_hle.h"
#include "../source/cpu/psp_syscall.h"
#include "../source/cpu/psp_cpu.h"
#include "../source/memory/psp_memory.h"
#include "../source/loader/psp_nid.h"
#include <assert.h>
#include <string.h>
static void w32(uint8_t*p,uint32_t v){p[0]=(uint8_t)v;p[1]=(uint8_t)(v>>8);p[2]=(uint8_t)(v>>16);p[3]=(uint8_t)(v>>24);}
int main(void){
 uint8_t iso[20*2048]={0},ram[4096]={0};PSP_Memory mem={ram,sizeof(ram)};PSP_ISO_FS fs={0};PSP_IO_Service io;
 uint8_t*p=iso+16*2048;p[0]=1;memcpy(p+1,"CD001",5);p[6]=1;
 uint8_t*root=p+156;root[0]=34;w32(root+2,17);w32(root+10,2048);
 uint8_t*r=iso+17*2048;r[0]=34;w32(r+2,17);w32(r+10,2048);r[25]=2;r[32]=1;r[33]=0;
 r+=34;r[0]=38;w32(r+2,18);w32(r+10,5);r[25]=0;r[32]=7;memcpy(r+33,"HELLO;1",7);
 memcpy(iso+18*2048,"WORLD",5);
 assert(psp_iso_fs_init(&fs,iso,sizeof(iso)));PSP_ISO_Entry e;assert(psp_iso_fs_find_path(&fs,"HELLO",&e));assert(e.size==5);
 psp_io_service_init(&io,&fs);psp_syscall_reset();psp_nid_reset();assert(psp_io_hle_register(&io));
 const char path[]="disc0:/HELLO";memcpy(ram+0x20,path,sizeof(path));PSP_CPU cpu={0};uint32_t code;
 assert(psp_nid_resolve(0x109F50BCu,&code)&&code==6);cpu.gpr[4]=PSP_RAM_BASE+0x20;cpu.gpr[5]=1;assert(psp_syscall_dispatch(&cpu,&mem,code));uint32_t fd=cpu.gpr[2];assert(fd>=3);
 assert(psp_nid_resolve(0x6A638D83u,&code));cpu.gpr[4]=fd;cpu.gpr[5]=PSP_RAM_BASE+0x100;cpu.gpr[6]=5;assert(psp_syscall_dispatch(&cpu,&mem,code));assert(cpu.gpr[2]==5);assert(!memcmp(ram+0x100,"WORLD",5));
 assert(psp_nid_resolve(0x27EB27B8u,&code));cpu.gpr[4]=fd;cpu.gpr[5]=0;cpu.gpr[6]=0;assert(psp_syscall_dispatch(&cpu,&mem,code));assert(cpu.gpr[2]==0);
 assert(psp_nid_resolve(0x810C4BC3u,&code));cpu.gpr[4]=fd;assert(psp_syscall_dispatch(&cpu,&mem,code));assert(cpu.gpr[2]==0);
 psp_io_hle_unregister();psp_syscall_reset();psp_nid_reset();return 0;
}
