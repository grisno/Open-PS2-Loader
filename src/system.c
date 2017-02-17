/*
  Copyright 2009, Ifcaro
  Licenced under Academic Free License version 3.0
  Review OpenUsbLd README & LICENSE files for further details.  
*/

#include "include/usbld.h"
#include "include/util.h"
#include "include/pad.h"
#include "include/system.h"
#include "include/ioman.h"
#ifdef VMC
typedef struct {
	char VMC_filename[1024];
	int  VMC_size_mb;
	int  VMC_blocksize;
	int  VMC_thread_priority;
	int  VMC_card_slot;
} createVMCparam_t;

extern void *genvmc_irx;
extern int size_genvmc_irx;
#endif

extern void *imgdrv_irx;
extern int size_imgdrv_irx;

extern void *eesync_irx;
extern int size_eesync_irx;

extern void *cdvdfsv_irx;
extern int size_cdvdfsv_irx;

extern void *cddev_irx;
extern int size_cddev_irx;

extern void *ps2dev9_irx;
extern int size_ps2dev9_irx;

extern void *smstcpip_irx;
extern int size_smstcpip_irx;

extern void *ingame_smstcpip_irx;
extern int size_ingame_smstcpip_irx;

extern void *smsmap_irx;
extern int size_smsmap_irx;

extern void *udptty_irx;
extern int size_udptty_irx;

extern void *ioptrap_irx;
extern int size_ioptrap_irx;

extern void *smbman_irx;
extern int size_smbman_irx;

extern void *discid_irx;
extern int size_discid_irx;

extern void *iomanx_irx;
extern int size_iomanx_irx;

extern void *filexio_irx;
extern int size_filexio_irx;

extern void *poweroff_irx;
extern int size_poweroff_irx;

extern void *ps2atad_irx;
extern int size_ps2atad_irx;

extern void *ps2hdd_irx;
extern int size_ps2hdd_irx;

extern void *hdldsvr_irx;
extern int size_hdldsvr_irx;

extern void *eecore_elf;
extern int size_eecore_elf;

extern void *alt_eecore_elf;
extern int size_alt_eecore_elf;

extern void *elfldr_elf;
extern int size_elfldr_elf;

extern void *smsutils_irx;
extern int size_smsutils_irx;

extern void *usbd_irx;
extern int size_usbd_irx;

#define MAX_MODULES	32
static void *g_sysLoadedModBuffer[MAX_MODULES];

#define ELF_MAGIC		0x464c457f
#define ELF_PT_LOAD		1

// CDVD Registers
#define CDVD_R_SCMD ((volatile u8*)0xBF402016)
#define CDVD_R_SDIN ((volatile u8*)0xBF402017)

// DEV9 Registers
#define DEV9_R_1460 ((volatile u16*)0xBF801460)
#define DEV9_R_1464 ((volatile u16*)0xBF801464)
#define DEV9_R_1466 ((volatile u16*)0xBF801466)
#define DEV9_R_146C ((volatile u16*)0xBF80146C)
#define DEV9_R_146E ((volatile u16*)0xBF80146E)
#define DEV9_R_1474 ((volatile u16*)0xBF801474)

#define	ROMSEG0(vaddr)	(0xbfc00000 | vaddr) 	// arghhh! avoid using this macro: some PS2 with modchips badly disabled seems 
						// to not tolerate very well direct BIOS access!!!
#define	KSEG0(vaddr)	(0x80000000 | vaddr)
#define	JAL(addr)	(0x0c000000 | ((addr & 0x03ffffff) >> 2))

typedef struct {
	u8	ident[16];	// struct definition for ELF object header
	u16	type;
	u16	machine;
	u32	version;
	u32	entry;
	u32	phoff;
	u32	shoff;
	u32	flags;
	u16	ehsize;
	u16	phentsize;
	u16	phnum;
	u16	shentsize;
	u16	shnum;
	u16	shstrndx;
} elf_header_t;

typedef struct {
	u32	type;		// struct definition for ELF program section header
	u32	offset;
	void	*vaddr;
	u32	paddr;
	u32	filesz;
	u32	memsz;
	u32	flags;
	u32	align;
} elf_pheader_t;

typedef struct {
	void *irxaddr;
	int irxsize;
} irxptr_t;

typedef struct {
	char fileName[10];
	u16  extinfoSize;
	int  fileSize;
} romdir_t;

int sysLoadModuleBuffer(void *buffer, int size, int argc, char *argv) {

	int i, id, ret, index = 0;

	// check we have not reached MAX_MODULES
	for (i=0; i<MAX_MODULES; i++) {
		if (g_sysLoadedModBuffer[i] == NULL) {
			index = i;
			break;
		}
	}
	if (i == MAX_MODULES)
		return -1;

	// check if the module was already loaded
	for (i=0; i<MAX_MODULES; i++) {
		if (g_sysLoadedModBuffer[i] == buffer) {
			return 0;
		}
	}

	// load the module
	id = SifExecModuleBuffer(buffer, size, argc, argv, &ret);
	if ((id < 0) || (ret))
		return -2;

	// add the module to the list
	g_sysLoadedModBuffer[index] = buffer;

	return 0;
}

void sysReset(int modload_mask) {

	SifInitRpc(0);
	cdInit(CDVD_INIT_NOCHECK);
	cdInit(CDVD_INIT_EXIT);

	while(!SifIopReset("rom0:UDNL rom0:EELOADCNF",0));
	while(!SifIopSync());

	fioExit();
	SifExitIopHeap();
	SifLoadFileExit();
	SifExitRpc();
	SifExitCmd();

	SifInitRpc(0);
	FlushCache(0);
	FlushCache(2);

	// init loadfile & iopheap services
	SifLoadFileInit();
	SifInitIopHeap();

	// apply sbv patches
	sbv_patch_enable_lmb();
	sbv_patch_disable_prefix_check();
	sbv_patch_fioremove();

	SifLoadModule("rom0:SIO2MAN", 0, NULL);

	if (modload_mask & SYS_LOAD_MC_MODULES) {
		SifLoadModule("rom0:MCMAN", 0, NULL);
		SifLoadModule("rom0:MCSERV", 0, NULL);
	}
	if (modload_mask & SYS_LOAD_PAD_MODULES) {
		SifLoadModule("rom0:PADMAN", 0, NULL);
	}

	// clears modules list
	memset((void *)&g_sysLoadedModBuffer[0], 0, MAX_MODULES*4);

	// load modules
	sysLoadModuleBuffer(&discid_irx, size_discid_irx, 0, NULL);
	sysLoadModuleBuffer(&iomanx_irx, size_iomanx_irx, 0, NULL);
	sysLoadModuleBuffer(&filexio_irx, size_filexio_irx, 0, NULL);
	sysLoadModuleBuffer(&poweroff_irx, size_poweroff_irx, 0, NULL);
#ifdef VMC
	sysLoadModuleBuffer(&genvmc_irx, size_genvmc_irx, 0, NULL);
#endif

	poweroffInit();
}

void sysPowerOff(void) {
	u16 dev9_hw_type;

	DIntr();
	ee_kmode_enter();

	// Get dev9 hardware type
	dev9_hw_type = *DEV9_R_146E & 0xf0;

	// Shutdown Pcmcia
	if ( dev9_hw_type == 0x20 )
	{
		*DEV9_R_146C = 0;
		*DEV9_R_1474 = 0;
	}
	// Shutdown Expansion Bay
	else if ( dev9_hw_type == 0x30 )
	{
		*DEV9_R_1466 = 1;
		*DEV9_R_1464 = 0;
		*DEV9_R_1460 = *DEV9_R_1464;
		*DEV9_R_146C = *DEV9_R_146C & ~4;
		*DEV9_R_1460 = *DEV9_R_146C & ~1;
	}

	//Wait a sec
	delay(5);

	// PowerOff PS2
	*CDVD_R_SDIN = 0;
	*CDVD_R_SCMD = 0xF;

	ee_kmode_exit();
	EIntr();
}

void delay(int count) {
	int i;
	int ret;
	for (i  = 0; i < count; i++) {
	        ret = 0x01000000;
		while(ret--) asm("nop\nnop\nnop\nnop");
	}
}

int sysPS3Detect(void) {	//return 0=PS2 1=PS3-HARD 2=PS3-SOFT
	int i, size = -1;
	void* buffer = readFile("rom0:XPARAM2", -1, &size);
	if (buffer) {
		for (i = 0; i < size; i++)
			if (!strcmp((const char*) ((u32) buffer + i), "SCPS_110.01")) {
				free(buffer);
				return 2;
			}

		free(buffer);
		return 1;
	}
	return 0;
}

int sysSetIPConfig(char* ipconfig) {
	int ipconfiglen;
	char str[16];

	memset(ipconfig, 0, IPCONFIG_MAX_LEN);
	ipconfiglen = 0;

	// add ip to g_ipconfig buf
	sprintf(str, "%d.%d.%d.%d", ps2_ip[0], ps2_ip[1], ps2_ip[2], ps2_ip[3]);
	strncpy(&ipconfig[ipconfiglen], str, 15);
	ipconfiglen += strlen(str) + 1;

	// add netmask to g_ipconfig buf
	sprintf(str, "%d.%d.%d.%d", ps2_netmask[0], ps2_netmask[1], ps2_netmask[2], ps2_netmask[3]);
	strncpy(&ipconfig[ipconfiglen], str, 15);
	ipconfiglen += strlen(str) + 1;

	// add gateway to g_ipconfig buf
	sprintf(str, "%d.%d.%d.%d", ps2_gateway[0], ps2_gateway[1], ps2_gateway[2], ps2_gateway[3]);
	strncpy(&ipconfig[ipconfiglen], str, 15);
	ipconfiglen += strlen(str) + 1;

	return ipconfiglen;
}

static unsigned int crctab[0x400];

unsigned int USBA_crc32(char *string) {
	int crc, table, count, byte;

	for (table=0; table<256; table++) {
		crc = table << 24;

		for (count=8; count>0; count--) {
			if (crc < 0) crc = crc << 1;
			else crc = (crc << 1) ^ 0x04C11DB7;
		}
		crctab[255-table] = crc;
	}

	do {
		byte = string[count++];
		crc = crctab[byte ^ ((crc >> 24) & 0xFF)] ^ ((crc << 8) & 0xFFFFFF00);
	} while (string[count-1] != 0);

	return crc;
}

int sysGetDiscID(char *hexDiscID) {
	cdInit(CDVD_INIT_NOCHECK);
	LOG("SYSTEM CDVD RPC inited\n");
	if (cdStatus() == CDVD_STAT_OPEN) // If tray is open, error
		return -1;
		
	while (cdGetDiscType() == CDVD_TYPE_DETECT) {;}	// Trick : if tray is open before startup it detects it as closed...
	if (cdGetDiscType() == CDVD_TYPE_NODISK)
		return -1;

	cdDiskReady(0); 	
	LOG("SYSTEM Disc drive is ready\n");
	CdvdDiscType_t cdmode = cdGetDiscType();	// If tray is closed, get disk type
	if (cdmode == CDVD_TYPE_NODISK)
		return -1;

	if ((cdmode != CDVD_TYPE_PS2DVD) && (cdmode != CDVD_TYPE_PS2CD) && (cdmode != CDVD_TYPE_PS2CDDA)) {
		cdStop();
		cdSync(0);
		LOG("SYSTEM Disc stopped, Disc is not ps2 disc!\n");
		return -2;
	}

	cdStandby();
	cdSync(0);
	LOG("SYSTEM Disc standby\n");

	int fd = fioOpen("discID:", O_RDONLY);
	if (fd < 0) {
		cdStop();
		cdSync(0);
		LOG("SYSTEM Disc stopped\n");
		return -3;
	}

	unsigned char discID[5];
	memset(discID, 0, 5);
	fioRead(fd, discID, 5);
	fioClose(fd);

	cdStop();
	cdSync(0);
	LOG("SYSTEM Disc stopped\n");

	// convert to hexadecimal string
	snprintf(hexDiscID, 15, "%02X %02X %02X %02X %02X", discID[0], discID[1], discID[2], discID[3], discID[4]);
	LOG("SYSTEM PS2 Disc ID = %s\n", hexDiscID);

	return 1;
}

int sysPcmciaCheck(void) {
	int ret;

	fileXioInit();
	ret = fileXioDevctl("dev9x0:", 0x4401, NULL, 0, NULL, 0);

	if (ret == 0) 	// PCMCIA
		return 1;

	return 0;	// ExpBay
}

void sysGetCDVDFSV(void **data_irx, int *size_irx)
{
	*data_irx = (void *)&cdvdfsv_irx;
	*size_irx = size_cdvdfsv_irx;
}

void sysExecExit() {
	__asm__ __volatile__(
			"	li $3, 0x04;"
			"	syscall;"
			"	nop;"
	);
}

static void restoreSyscallHandler(void)
{
	__asm__ __volatile__ (
		"addiu 	$a0, $zero, 8\n\t"
		"lui 	$a1, 0x8000\n\t"
		"ori 	$a1, $a1, 0x0280\n\t"
		"addiu 	$v1, $zero, 0x0e\n\t"
		"syscall\n\t"
		"nop\n\t"
	);	
}

#ifdef VMC
#define IRX_NUM 11
#else
#define IRX_NUM 10
#endif
  
#ifdef VMC
static void sendIrxKernelRAM(int size_cdvdman_irx, void **cdvdman_irx, int size_mcemu_irx, void **mcemu_irx) { // Send IOP modules that core must use to Kernel RAM
#else
static void sendIrxKernelRAM(int size_cdvdman_irx, void **cdvdman_irx) { // Send IOP modules that core must use to Kernel RAM
#endif

	restoreSyscallHandler();

	void *irxtab = (void *)0x80033010;
	void *irxptr = (void *)0x80033100;
	irxptr_t irxptr_tab[IRX_NUM];
	void *irxsrc[IRX_NUM];
	int i, n;
	u32 irxsize, curIrxSize;

	n = 0;
	irxptr_tab[n++].irxsize = size_imgdrv_irx;
	irxptr_tab[n++].irxsize = size_eesync_irx;
	irxptr_tab[n++].irxsize = size_cdvdman_irx;
	irxptr_tab[n++].irxsize = size_cdvdfsv_irx;
	irxptr_tab[n++].irxsize = size_cddev_irx;
	irxptr_tab[n++].irxsize = size_usbd_irx;
	irxptr_tab[n++].irxsize = size_smsmap_irx;
	irxptr_tab[n++].irxsize = size_udptty_irx;
	irxptr_tab[n++].irxsize = size_ioptrap_irx;
	irxptr_tab[n++].irxsize = size_ingame_smstcpip_irx;
#ifdef VMC
	irxptr_tab[n++].irxsize = size_mcemu_irx;
#endif

	n = 0;
	irxsrc[n++] = (void *)&imgdrv_irx;
	irxsrc[n++] = (void *)&eesync_irx;
	irxsrc[n++] = (void *)cdvdman_irx;
	irxsrc[n++] = (void *)&cdvdfsv_irx;
	irxsrc[n++] = (void *)&cddev_irx;
	irxsrc[n++] = (void *)usbd_irx;
	irxsrc[n++] = (void *)&smsmap_irx;
	irxsrc[n++] = (void *)&udptty_irx;
	irxsrc[n++] = (void *)&ioptrap_irx;
	irxsrc[n++] = (void *)&ingame_smstcpip_irx;
#ifdef VMC
	irxsrc[n++] = (void *)mcemu_irx; 
#endif

	irxsize = 0;

	DIntr();
	ee_kmode_enter();

	*(u32 *)0x80033000 = 0x80033010;

	for (i = 0; i < IRX_NUM; i++) {
		curIrxSize = irxptr_tab[i].irxsize;
		if ((((u32)irxptr + curIrxSize) >= 0x80050000) && ((u32)irxptr < 0x80060000))
			irxptr = (void *)0x80060000;
		irxptr_tab[i].irxaddr = irxptr;

		if (curIrxSize > 0) {
			ee_kmode_exit();
			EIntr();
			LOG("SYSTEM IRX address start: %08x end: %08x\n", (int)irxptr_tab[i].irxaddr, (int)(irxptr_tab[i].irxaddr+curIrxSize));
			DIntr();
			ee_kmode_enter();

			memcpy((void *)irxptr_tab[i].irxaddr, (void *)irxsrc[i], curIrxSize);

			irxptr += curIrxSize;
			irxsize += curIrxSize;
		}
	}

	memcpy((void *)irxtab, (void *)&irxptr_tab[0], sizeof(irxptr_tab));

	ee_kmode_exit();
	EIntr();
}

#ifdef VMC
void sysLaunchLoaderElf(char *filename, char *mode_str, int size_cdvdman_irx, void **cdvdman_irx, int size_mcemu_irx, void **mcemu_irx, int compatflags, int alt_ee_core) {
#else
void sysLaunchLoaderElf(char *filename, char *mode_str, int size_cdvdman_irx, void **cdvdman_irx, int compatflags, int alt_ee_core) {
#endif
	u8 *boot_elf = NULL;
	elf_header_t *eh;
	elf_pheader_t *eph;
	void *pdata;
	int i;
	char *argv[3];
	char config_str[255];
	char ipconfig[IPCONFIG_MAX_LEN] __attribute__((aligned(64)));

	sysSetIPConfig(ipconfig); // TODO only needed for ETH mode, and already done in ethsupport.ethLoadModules

	if (gExitPath[0] == '\0')
		strncpy(gExitPath, "Browser", 32);

#ifdef VMC
	LOG("SYSTEM LaunchLoaderElf called with size_mcemu_irx = %d\n", size_mcemu_irx);
	sendIrxKernelRAM(size_cdvdman_irx, cdvdman_irx, size_mcemu_irx, mcemu_irx);
#else
	sendIrxKernelRAM(size_cdvdman_irx, cdvdman_irx);
#endif

	// NB: LOADER.ELF is embedded
	if (alt_ee_core)
		boot_elf = (u8 *)&alt_eecore_elf;
	else
		boot_elf = (u8 *)&eecore_elf;
	eh = (elf_header_t *)boot_elf;
	if (_lw((u32)&eh->ident) != ELF_MAGIC)
		while (1);

	eph = (elf_pheader_t *)(boot_elf + eh->phoff);

	// Scan through the ELF's program headers and copy them into RAM, then
	// zero out any non-loaded regions.
	for (i = 0; i < eh->phnum; i++) {
		if (eph[i].type != ELF_PT_LOAD)
		continue;

		pdata = (void *)(boot_elf + eph[i].offset);
		memcpy(eph[i].vaddr, pdata, eph[i].filesz);

		if (eph[i].memsz > eph[i].filesz)
			memset(eph[i].vaddr + eph[i].filesz, 0, eph[i].memsz - eph[i].filesz);
	}

	// Let's go.
	fioExit();
	SifInitRpc(0);
	SifExitRpc();
	FlushCache(0);
	FlushCache(2);

	sprintf(config_str, "%s %d %s %d %d %d.%d.%d.%d %d.%d.%d.%d %d.%d.%d.%d", mode_str, gDisableDebug, gExitPath, gUSBDelay, gHDDSpindown, \
		ps2_ip[0], ps2_ip[1], ps2_ip[2], ps2_ip[3], \
		ps2_netmask[0], ps2_netmask[1], ps2_netmask[2], ps2_netmask[3], \
		ps2_gateway[0], ps2_gateway[1], ps2_gateway[2], ps2_gateway[3]);

	char cmask[10];
	snprintf(cmask, 10, "%d", compatflags);
	argv[0] = config_str;	
	argv[1] = filename;
	argv[2] = cmask;

	ExecPS2((void *)eh->entry, 0, 3, argv);
}

int sysExecElf(char *path, int argc, char **argv) {
	u8 *boot_elf = NULL;
	elf_header_t *eh;
	elf_pheader_t *eph;

	void *pdata;
	int i;
	char *elf_argv[1];

	// NB: ELFLDR.ELF is embedded
	boot_elf = (u8 *)&elfldr_elf;
	eh = (elf_header_t *)boot_elf;
	if (_lw((u32)&eh->ident) != ELF_MAGIC)
		while (1);

	eph = (elf_pheader_t *)(boot_elf + eh->phoff);

	// Scan through the ELF's program headers and copy them into RAM, then
	// zero out any non-loaded regions.
	for (i = 0; i < eh->phnum; i++) {
		if (eph[i].type != ELF_PT_LOAD)
		continue;

		pdata = (void *)(boot_elf + eph[i].offset);
		memcpy(eph[i].vaddr, pdata, eph[i].filesz);

		if (eph[i].memsz > eph[i].filesz)
			memset(eph[i].vaddr + eph[i].filesz, 0, eph[i].memsz - eph[i].filesz);
	}

	// Let's go.
	fioExit();
	SifInitRpc(0);
	SifExitRpc();
	FlushCache(0);
	FlushCache(2);

	elf_argv[0] = path;
	for (i=0; i<argc; i++)
		elf_argv[i+1] = argv[i];

	ExecPS2((void *)eh->entry, 0, argc+1, elf_argv);

	return 0;
}

int sysCheckMC(void) {
	int dummy, ret;

	mcGetInfo(0, 0, &dummy, &dummy, &dummy);
	mcSync(0, NULL, &ret);

	if( -1 == ret || 0 == ret) return 0;

	mcGetInfo(1, 0, &dummy, &dummy, &dummy);
	mcSync(0, NULL, &ret);

	if( -1 == ret || 0 == ret ) return 1;

	return -11;
}

#ifdef VMC
// createSize == -1 : delete, createSize == 0 : probing, createSize > 0 : creation
int sysCheckVMC(const char* prefix, const char* sep, char* name, int createSize, vmc_superblock_t* vmc_superblock) {
	int size = -1;
	char path[255];
	snprintf(path, 255, "%sVMC%s%s.bin", prefix, sep, name);

	if (createSize == -1)
		fileXioRemove(path);
	else {
		int fd = fileXioOpen(path, O_RDONLY, FIO_S_IRUSR | FIO_S_IWUSR | FIO_S_IXUSR | FIO_S_IRGRP | FIO_S_IWGRP | FIO_S_IXGRP | FIO_S_IROTH | FIO_S_IWOTH | FIO_S_IXOTH);
		if (fd >= 0) {
			size = fileXioLseek(fd, 0, SEEK_END);

			if (vmc_superblock) {
				memset(vmc_superblock, 0, sizeof(vmc_superblock_t));
				fileXioLseek(fd, 0, SEEK_SET);
				fileXioRead(fd, (void*)vmc_superblock, sizeof(vmc_superblock_t));

				LOG("SYSTEM File size  : 0x%X\n", size);
				LOG("SYSTEM Magic      : %s\n", vmc_superblock->magic);
				LOG("SYSTEM Card type  : %d\n", vmc_superblock->mc_type);
				LOG("SYSTEM Flags      : 0x%X\n", (vmc_superblock->mc_flag & 0xFF) | 0x100);
				LOG("SYSTEM Page_size  : 0x%X\n", vmc_superblock->page_size);
				LOG("SYSTEM Block_size : 0x%X\n", vmc_superblock->pages_per_block);
				LOG("SYSTEM Card_size  : 0x%X\n", vmc_superblock->pages_per_cluster * vmc_superblock->clusters_per_card);

				if(!strncmp(vmc_superblock->magic, "Sony PS2 Memory Card Format", 27) && vmc_superblock->mc_type == 0x2
					&& size == vmc_superblock->pages_per_cluster * vmc_superblock->clusters_per_card * vmc_superblock->page_size) {
					LOG("SYSTEM VMC file structure valid: %s\n", path);
				} else
					size = 0;
			}

			if (size % 1048576) // invalid size, should be a an integer (8, 16, 32, 64, ...)
				size = 0;
			else
				size /= 1048576;

			fileXioClose(fd);

			if (createSize && (createSize != size))
				fileXioRemove(path);
		}

		if (createSize && (createSize != size)) {
			createVMCparam_t createParam;
			strcpy(createParam.VMC_filename, path);
			createParam.VMC_size_mb = createSize;
			createParam.VMC_blocksize = 16;
			createParam.VMC_thread_priority = 0x0f;
			createParam.VMC_card_slot = -1;
			fileXioDevctl("genvmc:", 0xC0DE0001, (void*) &createParam, sizeof(createParam), NULL, 0);
		}
	}
	return size;
}
#endif
