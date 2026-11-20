#include "psp_nid.h"
static PSP_NID_Entry entries[PSP_NID_MAX_ENTRIES];
void psp_nid_reset(void) {
    for (unsigned int i=0;i<PSP_NID_MAX_ENTRIES;i++) {
        entries[i].nid=0; entries[i].syscall_code=0; entries[i].active=false;
    }
}
bool psp_nid_register(uint32_t nid,uint32_t syscall_code) {
    if(nid==0 || syscall_code>0xFFFFFu) return false;
    for(unsigned int i=0;i<PSP_NID_MAX_ENTRIES;i++)
        if(entries[i].active && entries[i].nid==nid) {
            entries[i].syscall_code=syscall_code; return true;
        }
    for(unsigned int i=0;i<PSP_NID_MAX_ENTRIES;i++)
        if(!entries[i].active) {
            entries[i].nid=nid; entries[i].syscall_code=syscall_code;
            entries[i].active=true; return true;
        }
    return false;
}
bool psp_nid_unregister(uint32_t nid) {
    for(unsigned int i=0;i<PSP_NID_MAX_ENTRIES;i++)
        if(entries[i].active && entries[i].nid==nid) {
            entries[i].nid=0; entries[i].syscall_code=0;
            entries[i].active=false; return true;
        }
    return false;
}
bool psp_nid_resolve(uint32_t nid,uint32_t *syscall_code) {
    if(syscall_code==0) return false;
    for(unsigned int i=0;i<PSP_NID_MAX_ENTRIES;i++)
        if(entries[i].active && entries[i].nid==nid) {
            *syscall_code=entries[i].syscall_code; return true;
        }
    return false;
}
