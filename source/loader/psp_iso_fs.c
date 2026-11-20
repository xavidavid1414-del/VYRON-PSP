#include "psp_iso_fs.h"
#include <string.h>
#include <ctype.h>
#define ISO_SECTOR_SIZE 2048u
#define ISO_PVD_OFFSET (16u*ISO_SECTOR_SIZE)
static uint32_t rd32(const uint8_t*p){return (uint32_t)p[0]|((uint32_t)p[1]<<8)|((uint32_t)p[2]<<16)|((uint32_t)p[3]<<24);}
static bool eq(const char*a,const char*b){while(*a&&*b){if(toupper((unsigned char)*a)!=toupper((unsigned char)*b))return false;a++;b++;}if(*a==';'){a++;while(isdigit((unsigned char)*a))a++;}return !*a&&!*b;}
bool psp_iso_fs_init(PSP_ISO_FS*fs,const uint8_t*data,size_t size){
 if(!fs||!data||size<ISO_PVD_OFFSET+ISO_SECTOR_SIZE)return false;
 const uint8_t*p=data+ISO_PVD_OFFSET;if(p[0]!=1||memcmp(p+1,"CD001",5)||p[6]!=1)return false;
 const uint8_t*r=p+156;if(r[0]<34)return false;uint32_t ex=rd32(r+2),len=rd32(r+10);uint64_t off=(uint64_t)ex*ISO_SECTOR_SIZE;
 if(off>size||len>size-off)return false;fs->data=data;fs->size=size;fs->root_extent=ex;fs->root_size=len;return true;
}
static bool find_dir(PSP_ISO_FS*fs,uint32_t extent,uint32_t length,const char*name,PSP_ISO_Entry*out){
 uint64_t base=(uint64_t)extent*ISO_SECTOR_SIZE;if(base>fs->size||length>fs->size-base)return false;
 size_t pos=0;while(pos<length){const uint8_t*r=fs->data+base+pos;uint8_t n=r[0];if(!n){pos=((pos/ISO_SECTOR_SIZE)+1u)*ISO_SECTOR_SIZE;continue;}
 if(n<34||n>length-pos)return false;uint8_t nl=r[32];if(33u+nl>n)return false;
 if(nl&&nl<128){char filename[128];memcpy(filename,r+33,nl);filename[nl]=0;if(eq(filename,name)){memset(out,0,sizeof(*out));out->extent=rd32(r+2);out->size=rd32(r+10);out->directory=(r[25]&2)!=0;memcpy(out->name,filename,nl+1);return true;}}
 pos+=n;}return false;
}
bool psp_iso_fs_find(PSP_ISO_FS*fs,const char*name,PSP_ISO_Entry*out){return fs&&fs->data&&name&&out&&find_dir(fs,fs->root_extent,fs->root_size,name,out);}
bool psp_iso_fs_find_path(PSP_ISO_FS*fs,const char*path,PSP_ISO_Entry*out){
 if(!fs||!fs->data||!path||!out)return false;while(*path=='/')path++;
 uint32_t extent=fs->root_extent,length=fs->root_size;PSP_ISO_Entry current={0};
 if(!*path){memset(out,0,sizeof(*out));strcpy(out->name,"/");out->extent=extent;out->size=length;out->directory=true;return true;}
 char part[128];while(*path){size_t n=0;while(path[n]&&path[n]!='/'){if(n>=127)return false;part[n]=path[n];n++;}part[n]=0;
 if(!find_dir(fs,extent,length,part,&current))return false;path+=n;while(*path=='/')path++;
 if(*path){if(!current.directory)return false;extent=current.extent;length=current.size;}}
 *out=current;return true;
}
size_t psp_iso_fs_read(PSP_ISO_FS*fs,const PSP_ISO_Entry*e,uint32_t off,uint8_t*out,size_t len){
 if(!fs||!fs->data||!e||!out||e->directory||off>=e->size)return 0;
 size_t n=len;if(n>e->size-off)n=e->size-off;uint64_t base=(uint64_t)e->extent*ISO_SECTOR_SIZE+off;
 if(base>fs->size||n>fs->size-base)return 0;memcpy(out,fs->data+base,n);return n;
}
