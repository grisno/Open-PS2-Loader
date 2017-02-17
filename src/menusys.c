/*
  Copyright 2009, Ifcaro & volca
  Licenced under Academic Free License version 3.0
  Review OpenUsbLd README & LICENSE files for further details.  
*/

#include "include/menusys.h"
#include "include/iosupport.h"
#include "include/usbld.h"
#include "include/renderman.h"
#include "include/fntsys.h"
#include "include/lang.h"
#include "include/themes.h"
#include "include/pad.h"
#include "include/gui.h"
#include "include/system.h"
#include "include/ioman.h"
#include "assert.h"

#define MENU_SETTINGS		0
#define MENU_GFX_SETTINGS	1
#define MENU_IP_CONFIG		2
#define MENU_SAVE_CHANGES	3
#define MENU_START_HDL		4
#define MENU_ABOUT			5
#define MENU_EXIT			6
#define MENU_POWER_OFF		7

// global menu variables
static menu_list_t* menu;
static menu_list_t* selected_item;

static int actionStatus;
static int itemConfigId;
static config_set_t* itemConfig;

// "main menu submenu"
static submenu_list_t* mainMenu;
// active item in the main menu
static submenu_list_t* mainMenuCurrent;

static s32 menuSemaId;
static ee_sema_t menuSema;

static void _menuLoadConfig() {
	WaitSema(menuSemaId);
	if (!itemConfig) {
		item_list_t* list = selected_item->item->userdata;
		itemConfig = list->itemGetConfig(itemConfigId);
	}
	actionStatus = 0;
	SignalSema(menuSemaId);
}

static void _menuSaveConfig() {
	WaitSema(menuSemaId);
	configWrite(itemConfig);
	itemConfigId = -1; // to invalidate cache and force reload
	actionStatus = 0;
	SignalSema(menuSemaId);
}

static void _menuRequestConfig() {
	WaitSema(menuSemaId);
	if (itemConfigId != selected_item->item->current->item.id) {
		if (itemConfig) {
			configFree(itemConfig);
			itemConfig = NULL;
		}
		item_list_t* list = selected_item->item->userdata;
		if (itemConfigId == -1 || guiInactiveFrames >= list->delay) {
			itemConfigId = selected_item->item->current->item.id;
			ioPutRequest(IO_CUSTOM_SIMPLEACTION, &_menuLoadConfig);
		}
	} else if (itemConfig)
		actionStatus = 0;
	SignalSema(menuSemaId);
}

config_set_t* menuLoadConfig() {
	actionStatus = 1;
	itemConfigId = -1;
	guiHandleDeferedIO(&actionStatus, _l(_STR_LOADING_SETTINGS), IO_CUSTOM_SIMPLEACTION, &_menuRequestConfig);
	return itemConfig;
}

void menuSaveConfig() {
	actionStatus = 1;
	guiHandleDeferedIO(&actionStatus, _l(_STR_SAVING_SETTINGS), IO_CUSTOM_SIMPLEACTION, &_menuSaveConfig);
}

static void menuInitMainMenu() {
	if (mainMenu)
		submenuDestroy(&mainMenu);

	// initialize the menu
#ifndef __CHILDPROOF
	submenuAppendItem(&mainMenu, -1, NULL, MENU_SETTINGS, _STR_SETTINGS);
	submenuAppendItem(&mainMenu, -1, NULL, MENU_GFX_SETTINGS, _STR_GFX_SETTINGS);
	submenuAppendItem(&mainMenu, -1, NULL, MENU_IP_CONFIG, _STR_IPCONFIG);
	submenuAppendItem(&mainMenu, -1, NULL, MENU_SAVE_CHANGES, _STR_SAVE_CHANGES);
	if (gHDDStartMode && gEnableDandR) // enabled at all?
		submenuAppendItem(&mainMenu, -1, NULL, MENU_START_HDL, _STR_STARTHDL);
#endif
	submenuAppendItem(&mainMenu, -1, NULL, MENU_ABOUT, _STR_ABOUT);
	submenuAppendItem(&mainMenu, -1, NULL, MENU_EXIT, _STR_EXIT);
	submenuAppendItem(&mainMenu, -1, NULL, MENU_POWER_OFF, _STR_POWEROFF);

	mainMenuCurrent = mainMenu;
}

// -------------------------------------------------------------------------------------------
// ---------------------------------------- Menu manipulation --------------------------------
// -------------------------------------------------------------------------------------------
void menuInit() {
	menu = NULL;
	selected_item = NULL;
	itemConfigId = -1;
	itemConfig = NULL;
	mainMenu = NULL;
	mainMenuCurrent = NULL;
	menuInitMainMenu();

	menuSema.init_count = 1;
	menuSema.max_count = 1;
	menuSema.option = 0;
	menuSemaId = CreateSema(&menuSema);
}

void menuEnd() {
	// destroy menu
	menu_list_t *cur = menu;
	
	while (cur) {
		menu_list_t *td = cur;
		cur = cur->next;
		
		if (&td->item)
			submenuDestroy(&td->item->submenu);
		
		menuRemoveHints(td->item);
		
		free(td);
	}

	submenuDestroy(&mainMenu);

	if (itemConfig) {
		configFree(itemConfig);
		itemConfig = NULL;
	}

	DeleteSema(menuSemaId);
}

static menu_list_t* AllocMenuItem(menu_item_t* item) {
	menu_list_t* it;
	
	it = malloc(sizeof(menu_list_t));
	
	it->prev = NULL;
	it->next = NULL;
	it->item = item;
	
	return it;
}

void menuAppendItem(menu_item_t* item) {
	assert(item);
	
	if (menu == NULL) {
		menu = AllocMenuItem(item);
		selected_item = menu;
		return;
	}
	
	menu_list_t *cur = menu;
	
	// traverse till the end
	while (cur->next)
		cur = cur->next;
	
	// create new item
	menu_list_t *newitem = AllocMenuItem(item);
	
	// link
	cur->next = newitem;
	newitem->prev = cur;
}

void submenuRebuildCache(submenu_list_t* submenu) {
	while (submenu) {
		if (submenu->item.cache_id)
			free(submenu->item.cache_id);
		if(submenu->item.cache_uid)
			free(submenu->item.cache_uid);

		int size = gTheme->gameCacheCount * sizeof(int);
		submenu->item.cache_id = malloc(size);
		memset(submenu->item.cache_id, -1, size);
		submenu->item.cache_uid = malloc(size);
		memset(submenu->item.cache_uid, -1, size);

		submenu = submenu->next;
	}
}

static submenu_list_t* submenuAllocItem(int icon_id, char *text, int id, int text_id) {
	submenu_list_t* it = (submenu_list_t*) malloc(sizeof(submenu_list_t));
	
	it->prev = NULL;
	it->next = NULL;
	it->item.icon_id = icon_id;
	it->item.text = text;
	it->item.text_id = text_id;
	it->item.id = id;
	it->item.cache_id = NULL;
	it->item.cache_uid = NULL;
	submenuRebuildCache(it);
	
	return it;
}

submenu_list_t* submenuAppendItem(submenu_list_t** submenu, int icon_id, char *text, int id, int text_id) {
	if (*submenu == NULL) {
		*submenu = submenuAllocItem(icon_id, text, id, text_id);
		return *submenu; 
	}
	
	submenu_list_t *cur = *submenu;
	
	// traverse till the end
	while (cur->next)
		cur = cur->next;
	
	// create new item
	submenu_list_t *newitem = submenuAllocItem(icon_id, text, id, text_id);
	
	// link
	cur->next = newitem;
	newitem->prev = cur;
	
	return newitem;
}

static void submenuDestroyItem(submenu_list_t* submenu) {
	free(submenu->item.cache_id);
	free(submenu->item.cache_uid);

	free(submenu);
}

void submenuRemoveItem(submenu_list_t** submenu, int id) {
	submenu_list_t* cur = *submenu;
	submenu_list_t* prev = NULL;
	
	while (cur) {
		if (cur->item.id == id) {
			submenu_list_t* next = cur->next;
			
			if (prev)
				prev->next = cur->next;
			
			if (*submenu == cur)
				*submenu = next;
			
			submenuDestroyItem(cur);
			
			cur = next;
		} else {
			prev = cur;
			cur = cur->next;
		}
	}
}

void submenuDestroy(submenu_list_t** submenu) {
	// destroy sub menu
	submenu_list_t *cur = *submenu;
	
	while (cur) {
		submenu_list_t *td = cur;
		cur = cur->next;
		
		submenuDestroyItem(td);
	}
	
	*submenu = NULL;
}

void menuAddHint(menu_item_t *menu, int text_id, int icon_id) {
	// allocate a new hint item
	menu_hint_item_t* hint = malloc(sizeof(menu_hint_item_t));

	hint->text_id = text_id;
	hint->icon_id = icon_id;
	hint->next = NULL;

	if (menu->hints) {
		menu_hint_item_t* top = menu->hints;

		// rewind to end
		for (; top->next; top = top->next);

		top->next = hint;
	} else {
		menu->hints = hint;
	}
}

void menuRemoveHints(menu_item_t *menu) {
	while (menu->hints) {
		menu_hint_item_t* hint = menu->hints;
		menu->hints = hint->next;
		free(hint);
	}
}

char *menuItemGetText(menu_item_t* it) {
	if (it->text_id >= 0)
		return _l(it->text_id);
	else
		return it->text;
}

char *submenuItemGetText(submenu_item_t* it) {
	if (it->text_id >= 0)
		return _l(it->text_id);
	else
		return it->text;
}

static void swap(submenu_list_t* a, submenu_list_t* b) {
	submenu_list_t *pa, *nb;
	pa = a->prev;
	nb = b->next;
	
	a->next = nb;
	b->prev = pa;
	b->next = a;
	a->prev = b;
	
	if (pa)
		pa->next = b;
	
	if (nb)
		nb->prev = a;
}

// Sorts the given submenu by comparing the on-screen titles
void submenuSort(submenu_list_t** submenu) {
	// a simple bubblesort
	// *submenu = mergeSort(*submenu);
	submenu_list_t *head = *submenu;
	int sorted = 0;
	
	if ((submenu == NULL) || (*submenu == NULL) || ((*submenu)->next == NULL))
		return;
	
	while (!sorted) {
		sorted = 1;
		
		submenu_list_t *tip = head;
		
		while (tip->next) {
			submenu_list_t *nxt = tip->next;
			
			char *txt1 = submenuItemGetText(&tip->item);
			char *txt2 = submenuItemGetText(&nxt->item);
			
			int cmp = stricmp(txt1, txt2);
			
			if (cmp > 0) {
				swap(tip, nxt);
				
				if (tip == head) 
					head = nxt;
				
				sorted = 0;
			} else {
				tip = tip->next;
			}
		}
	}
	
	*submenu = head;
}

static void menuNextH() {
	if(selected_item->next) {
		selected_item = selected_item->next;
		itemConfigId = -1;
	}
}

static void menuPrevH() {
	if(selected_item->prev) {
		selected_item = selected_item->prev;
		itemConfigId = -1;
	}
}

static void menuNextV() {
	submenu_list_t *cur = selected_item->item->current;
	
	if(cur && cur->next) {
		selected_item->item->current = cur->next;

		// if the current item is beyond the page start, move the page start one page down
		cur = selected_item->item->pagestart;
		int itms = ((items_list_t*) gTheme->itemsList->extended)->displayedItems + 1;
		while (--itms && cur)
			if (selected_item->item->current == cur)
				return;
			else
				cur = cur->next;

		selected_item->item->pagestart = selected_item->item->current;
	}
}

static void menuPrevV() {
	submenu_list_t *cur = selected_item->item->current;

	if(cur && cur->prev) {
		selected_item->item->current = cur->prev;

		// if the current item is on the page start, move the page start one page up
		if (selected_item->item->pagestart == cur) {
			int itms = ((items_list_t*) gTheme->itemsList->extended)->displayedItems + 1; // +1 because the selection will move as well
			while (--itms && selected_item->item->pagestart->prev)
				selected_item->item->pagestart = selected_item->item->pagestart->prev;
		}
	}
}

static void menuNextPage() {
	submenu_list_t *cur = selected_item->item->pagestart;

	if (cur) {
		int itms = ((items_list_t*) gTheme->itemsList->extended)->displayedItems + 1;
		while (--itms && cur->next)
			cur = cur->next;

		selected_item->item->current = cur;
		selected_item->item->pagestart = selected_item->item->current;
	}
}

static void menuPrevPage() {
	submenu_list_t *cur = selected_item->item->pagestart;

	if (cur) {
		int itms = ((items_list_t*) gTheme->itemsList->extended)->displayedItems + 1;
		while (--itms && cur->prev)
			cur = cur->prev;

		selected_item->item->current = cur;
		selected_item->item->pagestart = selected_item->item->current;
	}
}

static void menuFirstPage() {
	selected_item->item->current = selected_item->item->submenu;
	selected_item->item->pagestart = selected_item->item->current;
}

static void menuLastPage() {
	submenu_list_t *cur = selected_item->item->current;
	if (cur) {
		while (cur->next)
			cur = cur->next; // go to end

		selected_item->item->current = cur;

		int itms = ((items_list_t*) gTheme->itemsList->extended)->displayedItems;
		while (--itms && cur->prev) // and move back to have a full page
			cur = cur->prev;

		selected_item->item->pagestart = cur;
	}
}

void menuSetSelectedItem(menu_item_t* item) {
	menu_list_t* itm = menu;
	
	while (itm) {
		if (itm->item == item) {
			selected_item = itm;
			return;
		}
			
		itm = itm->next;
	}
}

void menuRenderMenu() {
	guiDrawBGPlasma();

	if (!mainMenu)
		return;

	// draw the animated menu
	if (!mainMenuCurrent)
		mainMenuCurrent = mainMenu;

	submenu_list_t* it = mainMenu;

	// calculate the number of items
	int count = 0; int sitem = 0;
	for (; it; count++, it=it->next) {
		if (it == mainMenuCurrent)
			sitem = count;
	}

	int spacing = 25;
	int y = (gTheme->usedHeight >> 1) - (spacing * (count >> 1));
	int cp = 0; // current position
	for (it = mainMenu; it; it = it->next, cp++) {
		// render, advance
		fntRenderString(gTheme->fonts[0], 320, y, ALIGN_CENTER, 0, 0, submenuItemGetText(&it->item), (cp == sitem) ? gTheme->selTextColor : gTheme->textColor);
		y += spacing;
	}
}

void menuHandleInputMenu() {
	if (!mainMenu)
		return;

	if (!mainMenuCurrent)
		mainMenuCurrent = mainMenu;

	if (getKey(KEY_UP)) {
		if (mainMenuCurrent->prev)
			mainMenuCurrent = mainMenuCurrent->prev;
		else	// rewind to the last item
			while (mainMenuCurrent->next)
				mainMenuCurrent = mainMenuCurrent->next;
	}

	if (getKey(KEY_DOWN)) {
		if (mainMenuCurrent->next)
			mainMenuCurrent = mainMenuCurrent->next;
		else
			mainMenuCurrent = mainMenu;
	}

	if (getKeyOn(KEY_CROSS)) {
		// execute the item via looking at the id of it
		int id = mainMenuCurrent->item.id;

		if (id == MENU_SETTINGS) {
			guiShowConfig();
		} else if (id == MENU_GFX_SETTINGS) {
			guiShowUIConfig();
		} else if (id == MENU_IP_CONFIG) {
			guiShowIPConfig();
		} else if (id == MENU_SAVE_CHANGES) {
			saveConfig(CONFIG_OPL, 1);
		} else if (id == MENU_START_HDL) {
			handleHdlSrv();
		} else if (id == MENU_ABOUT) {
			guiShowAbout();
		} else if (id == MENU_EXIT) {
			sysExecExit();
		} else if (id == MENU_POWER_OFF) {
			sysPowerOff();
		}

		// so the exit press wont propagate twice
		readPads();
	}

	if(getKeyOn(KEY_START) || getKeyOn(KEY_CIRCLE)) {
		if (gAPPStartMode || gETHStartMode || gUSBStartMode || gHDDStartMode)
			guiSwitchScreen(GUI_SCREEN_MAIN, TRANSITION_LEFT);
	}
}

void menuRenderMain() {
	// selected_item can't be NULL here as we only allow to switch to "Main" rendering when there is at least one device activated
	theme_element_t* elem = gTheme->mainElems.first;
	while (elem) {
		if (elem->drawElem)
			elem->drawElem(selected_item, selected_item->item->current, NULL, elem);

		elem = elem->next;
	}
}

void menuHandleInputMain() {
	if(getKey(KEY_LEFT)) {
		menuPrevH();
	} else if(getKey(KEY_RIGHT)) {
		menuNextH();
	} else if(getKey(KEY_UP)) {
		menuPrevV();
	} else if(getKey(KEY_DOWN)){
		menuNextV();
	} else if(getKeyOn(KEY_CROSS)) {
		if (selected_item->item->current && gUseInfoScreen && gTheme->infoElems.first)
			guiSwitchScreen(GUI_SCREEN_INFO, TRANSITION_DOWN);
		else
			selected_item->item->execCross(selected_item->item);
	} else if(getKeyOn(KEY_TRIANGLE)) {
		selected_item->item->execTriangle(selected_item->item);
	} else if(getKeyOn(KEY_CIRCLE)) {
		selected_item->item->execCircle(selected_item->item);
	} else if(getKeyOn(KEY_SQUARE)) {
		selected_item->item->execSquare(selected_item->item);
	} else if(getKeyOn(KEY_START)) {
		// reinit main menu - show/hide items valid in the active context
		menuInitMainMenu();
		guiSwitchScreen(GUI_SCREEN_MENU, TRANSITION_RIGHT);
	} else if(getKeyOn(KEY_SELECT)) {
		selected_item->item->refresh(selected_item->item);
	} else if(getKey(KEY_L1)) {
		menuPrevPage();
	} else if(getKey(KEY_R1)) {
		menuNextPage();
	} else if (getKeyOn(KEY_L2)) { // home
		menuFirstPage();
	} else if (getKeyOn(KEY_R2)) { // end
		menuLastPage();
	}
}

void menuRenderInfo() {
	// selected_item->item->current can't be NULL here as we only allow to switch to "Info" rendering when there is at least one item
	_menuRequestConfig();

	//WaitSema(menuSemaId); If I'm not mistaking (assignment of itemConfig pointer is atomic), not needed
	theme_element_t* elem = gTheme->infoElems.first;
	while (elem) {
		if (elem->drawElem)
			elem->drawElem(selected_item, selected_item->item->current, itemConfig, elem);

		elem = elem->next;
	}
	//SignalSema(menuSemaId);
}

void menuHandleInputInfo() {
	if(getKeyOn(KEY_CROSS)) {
		selected_item->item->execCross(selected_item->item);
	} else if(getKey(KEY_UP)) {
		menuPrevV();
	} else if(getKey(KEY_DOWN)){
		menuNextV();
	} else if(getKeyOn(KEY_CIRCLE)) {
		guiSwitchScreen(GUI_SCREEN_MAIN, TRANSITION_UP);
	} else if(getKey(KEY_L1)) {
		menuPrevPage();
	} else if(getKey(KEY_R1)) {
		menuNextPage();
	} else if (getKeyOn(KEY_L2)) {
		menuFirstPage();
	} else if (getKeyOn(KEY_R2)) {
		menuLastPage();
	}
}
