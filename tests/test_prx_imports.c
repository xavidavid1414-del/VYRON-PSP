#include "../source/loader/psp_prx_imports.h"
#include "../source/loader/psp_nid.h"
#include "../source/memory/psp_memory.h"
#include <assert.h>
#include <string.h>

static void w16(unsigned char *p,unsigned v){p[0]=(unsigned char)v;p[1]=(unsigned char)(v>>8);}
static void w32(unsigned char *p,unsigned v){p[0]=(unsigned char)v;p[1]=(unsigned char)(v>>8);p[2]=(unsigned char)(v>>16);p[3]=(unsigned char)(v>>24);}
int main(void) {
    unsigned char elf[512]={0}, ram[0x1000]={0};
    /* Minimal ELF32 PRX with one PT_LOAD and a .lib.stub section. */
    elf[0]=0x7f;elf[1]='E';elf[2]='L';elf[3]='F';elf[4]=1;elf[5]=1;elf[6]=1;
    w16(elf+16,0xFFA0);w16(elf+18,8);w32(elf+20,1);
    w32(elf+28,52);w16(elf+40,52);w16(elf+42,32);w16(elf+44,1);
    w32(elf+52,1);w32(elf+60,0x08800000);w32(elf+68,4);w32(elf+72,4);
    /* Section table: null, .lib.stub, string table. */
    w32(elf+32,128);w16(elf+46,40);w16(elf+48,3);w16(elf+50,2);
    memcpy(elf+300,"\\0.lib.stub\\0.shstrtab\\0",23);
    unsigned char *s1=elf+168;w32(s1,1);w32(s1+4,1);w32(s1+16,240);w32(s1+20,20);
    unsigned char *s2=elf+208;w32(s2,11);w32(s2+4,3);w32(s2+16,300);w32(s2+20,23);
    /* Import descriptor: len=5 words, one function, NID/stub virtual addresses. */
    elf[248]=5;w16(elf+250,1);w32(elf+252,0x08800020);w32(elf+256,0x08800030);
    /* Runtime RAM contains the NID at mapped +0x20. */
    w32(ram+0x20,0x1F4011E6u);
    psp_nid_reset();assert(psp_nid_register(0x1F4011E6u,0x100u));
    assert(psp_prx_resolve_imports(elf,sizeof(elf),ram,sizeof(ram)));
    assert(ram[0x30]==0x08 && ram[0x31]==0x00 && ram[0x32]==0xE0 && ram[0x33]==0x03);
    assert(ram[0x34]==0x0C && ram[0x35]==0x40 && ram[0x36]==0x00 && ram[0x37]==0x00);
    /* An unknown import must fail without changing its stub. */
    w32(ram+0x20,0xDEADBEEFu);memset(ram+0x30,0xAA,8);
    assert(!psp_prx_resolve_imports(elf,sizeof(elf),ram,sizeof(ram)));
    for(unsigned i=0;i<8;i++) assert(ram[0x30+i]==0xAA);
    psp_nid_reset();return 0;
}
