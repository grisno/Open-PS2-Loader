/*
   Copyright 2006-2008, Romz
   Copyright 2010, Polo
   Licenced under Academic Free License version 3.0
   Review OpenUsbLd README & LICENSE files for further details.
   */

#ifndef __MCEMU_UTILS_H
#define __MCEMU_UTILS_H

#include <irx.h>

typedef struct {
	int version;
	void **exports;
} modinfo_t;

/* SMS Utils Imports */
#define smsutils_IMPORTS_start DECLARE_IMPORT_TABLE( smsutils, 1, 1 )

void mips_memcpy ( void*, const void*, unsigned );
#define I_mips_memcpy DECLARE_IMPORT( 4, mips_memcpy )

void mips_memset ( void*, int, unsigned );
#define I_mips_memset DECLARE_IMPORT( 5, mips_memset )

#define smsutils_IMPORTS_end END_IMPORT_TABLE


#define oplutils_IMPORTS_start DECLARE_IMPORT_TABLE( oplutils, 1, 1 )

int getModInfo(u8 *modname, modinfo_t *info);
#define I_getModInfo DECLARE_IMPORT(4, getModInfo)

/* MASS Transfer Imports */
#ifdef USB_DRIVER

int mass_stor_readSector(unsigned int lba, int nsectors, unsigned char* buffer);
#define I_mass_stor_readSector DECLARE_IMPORT(5, mass_stor_readSector)

int mass_stor_writeSector(unsigned int lba, int nsectors, unsigned char* buffer);
#define I_mass_stor_writeSector DECLARE_IMPORT(6, mass_stor_writeSector)

#endif

/* ATAD Transfer Imports */
#ifdef HDD_DRIVER

/* These are used with the dir parameter of ata_device_dma_transfer().  */
#define ATA_DIR_READ	0
#define ATA_DIR_WRITE	1

int ata_device_dma_transfer( unsigned int unit, void *buf,  unsigned int lba,  unsigned int sectors, int dir);
#define I_ata_device_dma_transfer DECLARE_IMPORT(5, ata_device_dma_transfer)

#endif

/* SMB Transfer Imports */
#ifdef SMB_DRIVER

int smb_OpenAndX(char *filename, u16 *FID, int Write);
#define I_smb_OpenAndX DECLARE_IMPORT(5, smb_OpenAndX)

int smb_ReadFile(u16 FID, u32 offsetlow, u32 offsethigh, void *readbuf, u16 nbytes);
#define I_smb_ReadFile DECLARE_IMPORT(6, smb_ReadFile)

int smb_WriteFile(u16 FID, u32 offsetlow, u32 offsethigh, void *writebuf, u16 nbytes);
#define I_smb_WriteFile DECLARE_IMPORT(7, smb_WriteFile)

#endif

#define oplutils_IMPORTS_end END_IMPORT_TABLE

#endif  /* __MCEMU_UTILS_H */
