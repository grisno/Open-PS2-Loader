#ifndef __ETH_SUPPORT_H
#define __ETH_SUPPORT_H

#include "include/iosupport.h"

#ifdef VMC
#include "include/mcemu.h"
typedef struct {
	int        active;    /* Activation flag */
	char       fname[64]; /* File name (memorycard?.bin) */
	u16        fid;       /* SMB File ID */
	int        flags;     /* Card flag */
	vmc_spec_t specs;     /* Card specifications */
} smb_vmc_infos_t;
#endif

#define ERROR_ETH_NOT_STARTED				100
#define ERROR_ETH_MODULE_PS2DEV9_FAILURE	200
#define ERROR_ETH_MODULE_SMSUTILS_FAILURE	201
#define ERROR_ETH_MODULE_SMSTCPIP_FAILURE	202
#define ERROR_ETH_MODULE_SMSMAP_FAILURE		203
#define ERROR_ETH_MODULE_SMBMAN_FAILURE		204
#define ERROR_ETH_SMB_LOGON					300
#define ERROR_ETH_SMB_ECHO					301
#define ERROR_ETH_SMB_OPENSHARE				302

void ethInit();
item_list_t* ethGetObject(int initOnly);

#endif
