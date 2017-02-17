#include "include/usbld.h"
#include "include/lang.h"
#include "include/gui.h"
#include "include/appsupport.h"
#include "include/themes.h"
#include "include/system.h"
#include "include/ioman.h"

#include "include/usbsupport.h"
#include "include/ethsupport.h"
#include "include/hddsupport.h"

static int appForceUpdate = 1;
static int appItemCount = 0;

static config_set_t *configApps;

// forward declaration
static item_list_t appItemList;

static struct config_value_t* appGetConfigValue(int id) {
	struct config_value_t* cur = configApps->head;

	while (id--) {
		cur = cur->next;
	}

	return cur;
}

static char* appGetELFName(char* name) {
	// Looking for the ELF name
	char* pos = strrchr(name, '/');
	if (!pos)
		pos = strrchr(name, ':');
	if (pos) {
		return pos + 1;
	}

	return name;
}

void appInit(void) {
	LOG("APPSUPPORT Init\n");
	appForceUpdate = 1;
	configGetInt(configGetByType(CONFIG_OPL), "app_frames_delay", &appItemList.delay);
	configApps = configGetByType(CONFIG_APPS);
	appItemList.enabled = 1;
}

item_list_t* appGetObject(int initOnly) {
	if (initOnly && !appItemList.enabled)
		return NULL;
	return &appItemList;
}

static int appNeedsUpdate(void) {
	if (appForceUpdate) {
		appForceUpdate = 0;
		return 1;
	}

	return 0;
}

static int appUpdateItemList(void) {
	appItemCount = 0;
	configClear(configApps);
	configRead(configApps);

	if (configApps->head) {
		struct config_value_t* cur = configApps->head;
		while (cur) {
			cur = cur->next;
			appItemCount++;
		}
	}
	return appItemCount;
}

static int appGetItemCount(void) {
	return appItemCount;
}

static char* appGetItemName(int id) {
	struct config_value_t* cur = appGetConfigValue(id);
	return cur->key;
}

static int appGetItemNameLength(int id) {
	return 32;
}

static char* appGetItemStartup(int id) {
	struct config_value_t* cur = appGetConfigValue(id);
	return appGetELFName(cur->val);
}

#ifndef __CHILDPROOF
static void appDeleteItem(int id) {
	struct config_value_t* cur = appGetConfigValue(id);
	fileXioRemove(cur->val);
	cur->key[0] = '\0';
	configApps->modified = 1;
	configWrite(configApps);

	appForceUpdate = 1;
}

static void appRenameItem(int id, char* newName) {
	struct config_value_t* cur = appGetConfigValue(id);

	char value[255];
	strncpy(value, cur->val, 255);
	configRemoveKey(configApps, cur->key);
	configSetStr(configApps, newName, value);
	configWrite(configApps);

	appForceUpdate = 1;
}
#endif

static void appLaunchItem(int id, config_set_t* configSet) {
	struct config_value_t* cur = appGetConfigValue(id);
	int fd = fioOpen(cur->val, O_RDONLY);
	if (fd >= 0) {
		fioClose(fd);

		int exception = NO_EXCEPTION;
		if (strncmp(cur->val, "pfs0:", 5) == 0)
			exception = UNMOUNT_EXCEPTION;

		char filename[255];
		sprintf(filename,"%s",cur->val);
		shutdown(exception); // CAREFUL: shutdown will call appCleanUp, so configApps/cur will be freed
		sysExecElf(filename, 0, NULL);
	}
	else
		guiMsgBox(_l(_STR_ERR_FILE_INVALID), 0, NULL);
}

static config_set_t* appGetConfig(int id) {
	config_set_t* config = configAlloc(0, NULL, NULL);
	struct config_value_t* cur = appGetConfigValue(id);
	configSetStr(config, CONFIG_ITEM_NAME, appGetELFName(cur->val));
	configSetStr(config, CONFIG_ITEM_LONGNAME, cur->key);
	configSetStr(config, CONFIG_ITEM_STARTUP, cur->val);
	return config;
}

static int appGetImage(char* folder, int isRelative, char* value, char* suffix, GSTEXTURE* resultTex, short psm) {
	value = appGetELFName(value);
	// We search on ever devices from fatest to slowest (HDD > ETH > USB)
	static item_list_t *listSupport = NULL;
	if ( (listSupport = hddGetObject(1)) ) {
		if (listSupport->itemGetImage(folder, isRelative, value, suffix, resultTex, psm) >= 0)
			return 0;
	}

	if ( (listSupport = ethGetObject(1)) ) {
		if (listSupport->itemGetImage(folder, isRelative, value, suffix, resultTex, psm) >= 0)
			return 0;
	}

	if ( (listSupport = usbGetObject(1)) )
		return listSupport->itemGetImage(folder, isRelative, value, suffix, resultTex, psm);

	return -1;
}

static void appCleanUp(int exception) {
	if (appItemList.enabled) {
		LOG("APPSUPPORT CleanUp\n");
	}
}

static item_list_t appItemList = {
		APP_MODE, 0, NO_COMPAT, 0, MENU_MIN_INACTIVE_FRAMES, "Applications", _STR_APPS, &appInit, &appNeedsUpdate,	&appUpdateItemList,
#ifdef __CHILDPROOF
		&appGetItemCount, NULL, &appGetItemName, &appGetItemNameLength, &appGetItemStartup, NULL, NULL, &appLaunchItem,
#else
		&appGetItemCount, NULL, &appGetItemName, &appGetItemNameLength, &appGetItemStartup, &appDeleteItem, &appRenameItem, &appLaunchItem,
#endif
#ifdef VMC
		&appGetConfig, &appGetImage, &appCleanUp, NULL, APP_ICON
#else
		&appGetConfig, &appGetImage, &appCleanUp, APP_ICON
#endif
};
