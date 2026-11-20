#include "psp_io_hle.h"
#include "../cpu/psp_syscall.h"
#include "../loader/psp_nid.h"
#include "../memory/psp_memory.h"
#include <string.h>
#define SC_OPEN 6u
#define SC_CLOSE 7u
#define SC_READ 8u
#define SC_LSEEK 9u
#define NID_OPEN 0x109F50BCu
#define NID_CLOSE 0x810C4BC3u
#define NID_READ 0x6A638D83u
#define NID_LSEEK 0x27EB27B8u
static PSP_IO_Service *active;
void psp_io_service_init(PSP_IO_Service*s,PSP_ISO_FS*fs){if(!s)return;memset(s,0,sizeof(*s));s->fs=fs;s->next_fd=3;}
void psp_io_service_set_fs(PSP_IO_Service*s,PSP_ISO_FS*fs){if(!s)return;s->fs=fs;psp_io_service_reset(s);}
void psp_io_service_reset(PSP_IO_Service*s){if(!s)return;memset(s->handles,0,sizeof(s->handles));s->next_fd=3;}
static bool read_path(PSP_Memory*m,uint32_t addr,char*out,size_t cap){
 if(!m||!out||!cap)return false;
 for(size_t i=0;i+1<cap;i++){uint64_t a=(uint64_t)addr+i;if(a>UINT32_MAX||!psp_memory_is_valid(m,(uint32_t)a,1))return false;char c=(char)psp_memory_read8(m,(uint32_t)a);out[i]=c;if(!c)return i>0;}
 out[cap-1]=0;return false;
}
static bool open_call(PSP_CPU*c,PSP_Memory*m,uint32_t code){
 if(!c||!m||!active||code!=SC_OPEN)return false;
 char raw[256];if(!read_path(m,c->gpr[4],raw,sizeof(raw))||!active->fs){c->gpr[2]=0x80010002u;return true;}
 const char*p=raw;
 if(!strncmp(p,"disc0:/",7)||!strncmp(p,"umd0:/",6)){p=strchr(p,':')+1;while(*p=='/')p++;}
 else {c->gpr[2]=0x80010002u;return true;}
 if((c->gpr[5]&3u)!=1u){c->gpr[2]=0x80010002u;return true;}
 PSP_ISO_Entry e;if(!psp_iso_fs_find_path(active->fs,p,&e)||e.directory){c->gpr[2]=0x80010013u;return true;}
 for(unsigned i=0;i<PSP_IO_MAX_HANDLES;i++)if(!active->handles[i].active){uint32_t fd=active->next_fd++;if(fd<3){active->next_fd=4;fd=3;}active->handles[i]=(PSP_IO_Handle){fd,0,e,true};c->gpr[2]=fd;return true;}
 c->gpr[2]=0x80020190u;return true;
}
static PSP_IO_Handle*get_handle(uint32_t fd){if(!active)return 0;for(unsigned i=0;i<PSP_IO_MAX_HANDLES;i++)if(active->handles[i].active&&active->handles[i].fd==fd)return &active->handles[i];return 0;}
static bool close_call(PSP_CPU*c,PSP_Memory*m,uint32_t code){(void)m;if(!c||code!=SC_CLOSE)return false;PSP_IO_Handle*h=get_handle(c->gpr[4]);if(!h){c->gpr[2]=0x80010009u;return true;}memset(h,0,sizeof(*h));c->gpr[2]=0;return true;}
static bool read_call(PSP_CPU*c,PSP_Memory*m,uint32_t code){
 if(!c||!m||code!=SC_READ)return false;PSP_IO_Handle*h=get_handle(c->gpr[4]);uint32_t addr=c->gpr[5],len=c->gpr[6];
 if(!h||!psp_memory_is_valid(m,addr,len)){c->gpr[2]=0x80010009u;return true;}
 uint32_t remaining=h->entry.size-h->offset;if(len>remaining)len=remaining;if(!len){c->gpr[2]=0;return true;}
 uint8_t temp[512];uint32_t done=0;while(done<len){uint32_t chunk=len-done;if(chunk>sizeof(temp))chunk=sizeof(temp);size_t got=psp_iso_fs_read(active->fs,&h->entry,h->offset,temp,chunk);if(!got)break;for(size_t i=0;i<got;i++)psp_memory_write8(m,addr+done+(uint32_t)i,temp[i]);done+=(uint32_t)got;h->offset+=(uint32_t)got;if(got<chunk)break;}
 c->gpr[2]=done;return true;
}
static bool seek_call(PSP_CPU*c,PSP_Memory*m,uint32_t code){
 (void)m;if(!c||code!=SC_LSEEK)return false;PSP_IO_Handle*h=get_handle(c->gpr[4]);if(!h){c->gpr[2]=0x80010009u;return true;}
 int32_t delta=(int32_t)c->gpr[5];uint32_t whence=c->gpr[6];int64_t base=whence==0?0:whence==1?h->offset:whence==2?h->entry.size:-1;int64_t next=base+delta;
 if(base<0||next<0||next>h->entry.size){c->gpr[2]=0x80010016u;return true;}h->offset=(uint32_t)next;c->gpr[2]=h->offset;return true;
}
bool psp_io_hle_register(PSP_IO_Service*s){
 if(!s)return false;active=s;
 if(!psp_syscall_register(SC_OPEN,open_call)||!psp_syscall_register(SC_CLOSE,close_call)||!psp_syscall_register(SC_READ,read_call)||!psp_syscall_register(SC_LSEEK,seek_call)||
 !psp_nid_register(NID_OPEN,SC_OPEN)||!psp_nid_register(NID_CLOSE,SC_CLOSE)||!psp_nid_register(NID_READ,SC_READ)||!psp_nid_register(NID_LSEEK,SC_LSEEK)){psp_io_hle_unregister();return false;}
 return true;
}
void psp_io_hle_unregister(void){psp_syscall_unregister(SC_OPEN);psp_syscall_unregister(SC_CLOSE);psp_syscall_unregister(SC_READ);psp_syscall_unregister(SC_LSEEK);psp_nid_unregister(NID_OPEN);psp_nid_unregister(NID_CLOSE);psp_nid_unregister(NID_READ);psp_nid_unregister(NID_LSEEK);active=0;}
