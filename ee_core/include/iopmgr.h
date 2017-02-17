/*
  Copyright 2009-2010, Ifcaro, jimmikaelkael & Polo
  Copyright 2006-2008 Polo
  Licenced under Academic Free License version 3.0
  Review OpenUsbLd README & LICENSE files for further details.
  
  Some parts of the code are taken from HD Project by Polo
*/

#ifndef IOPMGR_H
#define IOPMGR_H

typedef
struct
{
   u32 psize;
   u32 daddr;
   int fcode;
   u32 unknown;
 
} SifCmdHdr;

typedef
struct
{
 SifCmdHdr chdr;
 int       size;
 int       flag;
 char      arg[0x50];
} SifCmdResetData __attribute__((aligned(16)));

typedef
struct romdir
{
 char           fileName[10];
 unsigned short extinfo_size;
 int            fileSize;
} romdir_t;

int  New_Reset_Iop(const char *arg, int flag);
int  Reset_Iop(const char *arg, int flag);
int  Sync_Iop(void);

#endif /* IOPMGR */
