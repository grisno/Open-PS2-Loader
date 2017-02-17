#ifndef __USBLD_H
#define __USBLD_H

#include <tamtypes.h>
#include <kernel.h>
#include <sifrpc.h>
#include <fileio.h>
#include <iopcontrol.h>
#include <iopheap.h>
#include <string.h>
#include <loadfile.h>
#include <stdio.h>
#include <sbv_patches.h>
#include <libcdvd.h>
#include <libpad.h>
#include <libmc.h>
#include <debug.h>
#include <gsKit.h>
#include <dmaKit.h>
#include <gsToolkit.h>
#include <malloc.h>
#include <math.h>
#include <libpwroff.h>
#include <fileXio_rpc.h>
#include <smod.h>
#include <smem.h>
#include <debug.h>
#include <ps2smb.h>
#include "config.h"
#ifdef VMC
#include <sys/fcntl.h>
#endif

#define USBLD_VERSION "0.9"

#define IO_MENU_UPDATE_DEFFERED 2

void setErrorMessage(int strId, int error);
int loadConfig(int types);
int saveConfig(int types, int showUI);
void applyConfig(int themeID, int langID);
void menuDeferredUpdate(void* data);
void moduleUpdateMenu(int mode, int themeChanged);
void handleHdlSrv();
void shutdown();

char *gBaseMCDir;

//// IP config

#define IPCONFIG_MAX_LEN	64

int ps2_ip[4];
int ps2_netmask[4];
int ps2_gateway[4];
int pc_ip[4];
int gPCPort;
char gPCShareName[32];
char gPCUserName[32];
char gPCPassword[32];

//// Settings

// describes what is happening in the network startup thread (>0 means loading, <0 means error)...
int gNetworkStartup;
// true if the ip config should be saved as well
int gIPConfigChanged;
int gHDDSpindown;
/// Indicates the hdd module loading sequence
int gHddStartup;
/// 0 = off, 1 = manual, 2 = auto
int gUSBStartMode;
int gHDDStartMode;
int gETHStartMode;
int gAPPStartMode;
int gAutosort;
int gAutoRefresh;
int gUseInfoScreen;
int gEnableArt;
int gWideScreen;
int gVMode; // 0 - Auto, 1 - PAL, 2 - NTSC
int gVSync; // 0 - False, 1 - True

// 0,1,2 scrolling speed
int gScrollSpeed;
// Exit path
char gExitPath[32];
// Disable Debug Colors
int gDisableDebug;
// Default device
int gDefaultDevice;

int gEnableDandR;

int gCheckUSBFragmentation;
int gUSBDelay;
char gUSBPrefix[32];
char gETHPrefix[32];

int gRememberLastPlayed;

char *infotxt;

unsigned char gDefaultBgColor[3];
unsigned char gDefaultTextColor[3];
unsigned char gDefaultSelTextColor[3];
unsigned char gDefaultUITextColor[3];

#define MENU_ITEM_HEIGHT 19

#endif
