#include "psp_prx_imports.h"
#include "psp_nid.h"
#include "../memory/psp_memory.h"

#include <stdlib.h>
#include <string.h>

#define ELF_HEADER_SIZE 52u
#define ELF_SECTION_HEADER_SIZE 40u
#define ELF_SHT_PROGBITS 1u
#define ELF_PT_LOAD 1u
#define PSP_IMPORT_ENTRY_MIN_SIZE 20u
#define MIPS_JR_RA 0x03E00008u
#define MIPS_SYSCALL(code) (((code) << 6) | 0x0Cu)

static uint16_t rd16(const uint8_t *p) {
    return (uint16_t)p[0] | ((uint16_t)p[1] << 8);
}
static uint32_t rd32(const uint8_t *p) {
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) |
           ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}
static void wr32(uint8_t *p, uint32_t v) {
    p[0]=(uint8_t)v; p[1]=(uint8_t)(v>>8);
    p[2]=(uint8_t)(v>>16); p[3]=(uint8_t)(v>>24);
}
static bool span(size_t total, uint32_t off, uint32_t len) {
    return off <= total && len <= total - off;
}
static bool map_addr(uint32_t vaddr, bool prx, uint32_t lowest,
                     uint32_t ram_size, uint32_t *offset) {
    uint64_t mapped;
    if (prx) {
        if (vaddr < lowest) return false;
        mapped = (uint64_t)PSP_RAM_BASE + ((uint64_t)vaddr-lowest);
    } else mapped = vaddr;
    if (mapped < PSP_RAM_BASE ||
        mapped-PSP_RAM_BASE > ram_size) return false;
    *offset=(uint32_t)(mapped-PSP_RAM_BASE);
    return true;
}

bool psp_prx_resolve_imports(const uint8_t *elf, size_t elf_size,
                             uint8_t *ram, uint32_t ram_size) {
    if (!elf || !ram || elf_size < ELF_HEADER_SIZE) return false;
    uint16_t type=rd16(elf+16);
    bool prx=(type==0xFFA0u);
    if (!prx && type!=2u) return false;

    uint32_t shoff=rd32(elf+32);
    uint16_t shentsize=rd16(elf+46), shnum=rd16(elf+48);
    uint16_t shstrndx=rd16(elf+50);
    if (shnum==0) return true;
    if (shentsize<ELF_SECTION_HEADER_SIZE || shstrndx>=shnum ||
        shoff>elf_size || (size_t)shnum>(elf_size-shoff)/shentsize)
        return false;

    const uint8_t *shstr=elf+shoff+(size_t)shstrndx*shentsize;
    uint32_t str_off=rd32(shstr+16), str_size=rd32(shstr+20);
    if (!span(elf_size,str_off,str_size)) return false;

    uint32_t lowest=UINT32_MAX;
    uint32_t phoff=rd32(elf+28);
    uint16_t phentsize=rd16(elf+42), phnum=rd16(elf+44);
    if (phentsize<32u || phoff>elf_size ||
        (size_t)phnum>(elf_size-phoff)/phentsize) return false;
    for (uint16_t i=0;i<phnum;i++) {
        const uint8_t *ph=elf+phoff+(size_t)i*phentsize;
        if (rd32(ph)==ELF_PT_LOAD && rd32(ph+20)!=0 &&
            rd32(ph+8)<lowest) lowest=rd32(ph+8);
    }
    if (lowest==UINT32_MAX) return false;

    /* First pass validates and stages every patch; no partial modifications. */
    typedef struct { uint32_t offset, syscall_code; } Patch;
    Patch *patches=NULL;
    size_t count=0, capacity=0;
    bool found_import_section=false, ok=true;

    for (uint16_t i=0;i<shnum && ok;i++) {
        const uint8_t *sh=elf+shoff+(size_t)i*shentsize;
        uint32_t name_off=rd32(sh), section_type=rd32(sh+4);
        uint32_t sec_off=rd32(sh+16), sec_size=rd32(sh+20);
        if (name_off>=str_size || section_type!=ELF_SHT_PROGBITS) continue;
        const char *name=(const char *)(elf+str_off+name_off);
        size_t remain=str_size-name_off;
        if (!memchr(name,'\\0',remain)) { ok=false; break; }
        if (strcmp(name,".lib.stub")!=0) continue;
        found_import_section=true;
        if (!span(elf_size,sec_off,sec_size)) { ok=false; break; }
        size_t cursor=0;
        while (cursor<sec_size) {
            if (sec_size-cursor<PSP_IMPORT_ENTRY_MIN_SIZE) { ok=false; break; }
            const uint8_t *entry=elf+sec_off+cursor;
            uint8_t words=entry[8];
            if (words<5u || (size_t)words*4u>sec_size-cursor) { ok=false; break; }
            uint16_t stub_count=rd16(entry+10);
            uint32_t nid_addr=rd32(entry+12), stub_addr=rd32(entry+16);
            uint32_t nid_off, stub_off;
            if (stub_count && (!map_addr(nid_addr,prx,lowest,ram_size,&nid_off) ||
                !map_addr(stub_addr,prx,lowest,ram_size,&stub_off) ||
                (uint64_t)nid_off+(uint64_t)stub_count*4u>ram_size ||
                (uint64_t)stub_off+(uint64_t)stub_count*8u>ram_size)) {
                ok=false; break;
            }
            for (uint16_t n=0;n<stub_count;n++) {
                uint32_t nid=rd32(ram+nid_off+(uint32_t)n*4u), code;
                if (!psp_nid_resolve(nid,&code)) { ok=false; break; }
                if (count==capacity) {
                    size_t next=capacity?capacity*2u:16u;
                    Patch *grown=(Patch *)realloc(patches,next*sizeof(*patches));
                    if (!grown) { ok=false; break; }
                    patches=grown; capacity=next;
                }
                patches[count].offset=stub_off+(uint32_t)n*8u;
                patches[count].syscall_code=code;
                count++;
            }
            if (!ok) break;
            cursor+=(size_t)words*4u;
        }
    }
    if (ok && found_import_section) {
        for (size_t i=0;i<count;i++) {
            uint8_t *stub=ram+patches[i].offset;
            wr32(stub,MIPS_JR_RA);
            wr32(stub+4,MIPS_SYSCALL(patches[i].syscall_code));
        }
    }
    free(patches);
    return ok;
}
