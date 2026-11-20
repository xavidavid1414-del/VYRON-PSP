#include "psp_nid.h"
#include <assert.h>
int main(void) {
    uint32_t code=0; psp_nid_reset();
    assert(!psp_nid_resolve(0x1234u,&code));
    assert(!psp_nid_register(0,1));
    assert(!psp_nid_register(0x1234u,0x100000u));
    assert(psp_nid_register(0x1234u,0x100u));
    assert(psp_nid_resolve(0x1234u,&code) && code==0x100u);
    assert(psp_nid_register(0x1234u,0x101u));
    assert(psp_nid_resolve(0x1234u,&code) && code==0x101u);
    assert(psp_nid_unregister(0x1234u));
    assert(!psp_nid_resolve(0x1234u,&code));
    for(uint32_t i=1;i<=PSP_NID_MAX_ENTRIES;i++) assert(psp_nid_register(i,i));
    assert(!psp_nid_register(0x1000u,0x1000u));
    psp_nid_reset(); return 0;
}
