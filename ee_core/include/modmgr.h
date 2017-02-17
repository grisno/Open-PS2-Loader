/*
  Copyright 2009-2010, Ifcaro, jimmikaelkael & Polo
  Copyright 2006-2008 Polo
  Licenced under Academic Free License version 3.0
  Review OpenUsbLd README & LICENSE files for further details.
  
  Some parts of the code are taken from HD Project by Polo
*/

#ifndef MODMGR_H
#define MODMGR_H

#define LF_PATH_MAX	252
#define LF_ARG_MAX	252

enum _lf_functions {
        LF_F_MOD_LOAD = 0,
        LF_F_ELF_LOAD,

        LF_F_SET_ADDR,
        LF_F_GET_ADDR,

        LF_F_MG_MOD_LOAD,
        LF_F_MG_ELF_LOAD,

        LF_F_MOD_BUF_LOAD,

        LF_F_MOD_STOP,
        LF_F_MOD_UNLOAD,

        LF_F_SEARCH_MOD_BY_NAME,
        LF_F_SEARCH_MOD_BY_ADDRESS,
};

struct _lf_module_load_arg {
 union
 {
  int arg_len;
  int	result;
 } p;
 int  modres;
 char path[LF_PATH_MAX];
 char args[LF_ARG_MAX];
}  __attribute__((aligned (16)));

struct _lf_module_buffer_load_arg {
 union
 {
  void *ptr;
  int   result;
 } p;
 union
 {
  int arg_len;
  int modres;
 } q;
 char unused[LF_PATH_MAX];
 char args[LF_ARG_MAX];
} __attribute__((aligned (16)));

struct _lf_elf_load_arg {
 union
 {
  u32 epc;
  int result;
 } p;
 u32  gp;
 char path[LF_PATH_MAX];
 char secname[LF_ARG_MAX];
} __attribute__((aligned (16)));

typedef struct 
{
	u32 epc;
	u32 gp;
	u32 sp;
	u32 dummy;  
} t_ExecData;

int  LoadFileInit();
void LoadFileExit();
int  LoadModule(const char *path, int arg_len, const char *args);
int  LoadModuleAsync(const char *path, int arg_len, const char *args);
void GetIrxKernelRAM(void);
int  LoadIRXfromKernel(void *irxkernelmem, int irxsize, int arglen, char *argv);
int  LoadMemModule(void *modptr, unsigned int modsize, int arg_len, const char *args);
int  LoadElf(const char *path, t_ExecData *data);
void ChangeModuleName(const char *name, const char *newname);
void ListModules(void);

#endif /* MODMGR */

