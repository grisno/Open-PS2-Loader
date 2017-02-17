#include "include/usbld.h"
#include "include/themes.h"
#include "include/util.h"
#include "include/gui.h"
#include "include/renderman.h"
#include "include/textures.h"
#include "include/ioman.h"
#include "include/fntsys.h"
#include "include/lang.h"

#define MENU_POS_V 50
#define HINT_HEIGHT 32
#define DECORATOR_SIZE 20

theme_t* gTheme;

static int screenWidth;
static int screenHeight;
static int guiThemeID = 0;

static int nThemes = 0;
static theme_file_t themes[THM_MAX_FILES];
static char **guiThemesNames = NULL;

#define TYPE_ATTRIBUTE_TEXT		0
#define TYPE_STATIC_TEXT		1
#define TYPE_ATTRIBUTE_IMAGE	2
#define TYPE_GAME_IMAGE			3
#define TYPE_STATIC_IMAGE		4
#define TYPE_BACKGROUND			5
#define TYPE_MENU_ICON			6
#define TYPE_MENU_TEXT			7
#define	TYPE_ITEMS_LIST			8
#define	TYPE_ITEM_ICON			9
#define	TYPE_ITEM_COVER			10
#define TYPE_ITEM_TEXT			11
#define TYPE_HINT_TEXT			12
#define TYPE_INFO_HINT_TEXT		13
#define TYPE_LOADING_ICON		14

#define DISPLAY_ALWAYS		0
#define DISPLAY_DEFINED		1
#define DISPLAY_NEVER		2

#define SIZING_NONE			-1
#define SIZING_CLIP			0
#define SIZING_WRAP			1

static char *elementsType[] = {
	"AttributeText",
	"StaticText",
	"AttributeImage",
	"GameImage",
	"StaticImage",
	"Background",
	"MenuIcon",
	"MenuText",
	"ItemsList",
	"ItemIcon",
	"ItemCover",
	"ItemText",
	"HintText",
	"InfoHintText",
	"LoadingIcon",
};

// Common functions for Text ////////////////////////////////////////////////////////////////////////////////////////////////

static void endMutableText(theme_element_t* elem) {
	mutable_text_t* mutableText = (mutable_text_t*) elem->extended;
	if (mutableText) {
		if (mutableText->value)
			free(mutableText->value);

		if (mutableText->alias)
			free(mutableText->alias);

		free(mutableText);
	}

	free(elem);
}

static mutable_text_t* initMutableText(char* themePath, config_set_t* themeConfig, theme_t* theme, char* name, int type, struct theme_element* elem,
		char* value, char* alias, int displayMode, int sizingMode) {

	mutable_text_t* mutableText = (mutable_text_t*) malloc(sizeof(mutable_text_t));
	mutableText->currentConfigId = 0;
	mutableText->currentValue = NULL;
	mutableText->alias = NULL;

	char elemProp[64];

	snprintf(elemProp, 64, "%s_display", name);
	configGetInt(themeConfig, elemProp, &displayMode);
	mutableText->displayMode = displayMode;

	int length = strlen(value) + 1;
	mutableText->value = (char*) malloc(length * sizeof(char));
	memcpy(mutableText->value, value, length);

	snprintf(elemProp, 64, "%s_wrap", name);
	if (configGetInt(themeConfig, elemProp, &sizingMode)) {
		if (sizingMode > 0)
			mutableText->sizingMode = SIZING_WRAP;
	}

	if ((elem->width != DIM_UNDEF) || (elem->height != DIM_UNDEF)) {
		if (sizingMode == SIZING_NONE)
			sizingMode = SIZING_CLIP;

		if (elem->width == DIM_UNDEF)
			elem->width = screenWidth;

		if (elem->height == DIM_UNDEF)
			elem->height = screenHeight;
	} else
		sizingMode = SIZING_NONE;
	mutableText->sizingMode = sizingMode;

	if (type == TYPE_ATTRIBUTE_TEXT) {
		snprintf(elemProp, 64, "%s_title", name);
		configGetStr(themeConfig, elemProp, &alias);
		if (!alias) {
			if (value[0] == '#')
				alias = &value[1];
			else
				alias = value;
		}
		length = strlen(alias) + 1 + 3;
		mutableText->alias = (char*) malloc(length * sizeof(char));
		if (mutableText->sizingMode == SIZING_WRAP)
			snprintf(mutableText->alias, length, "%s :\n", alias);
		else
			snprintf(mutableText->alias, length, "%s : ", alias);
	} else {
		if (mutableText->sizingMode == SIZING_WRAP)
			fntFitString(elem->font, mutableText->value, elem->width);
	}

	return mutableText;
}

// StaticText ///////////////////////////////////////////////////////////////////////////////////////////////////////////////

static void drawStaticText(struct menu_list* menu, struct submenu_list* item, config_set_t* config, struct theme_element* elem) {
	mutable_text_t* mutableText = (mutable_text_t*) elem->extended;
	if (mutableText->sizingMode == SIZING_NONE)
		fntRenderString(elem->font, elem->posX, elem->posY, elem->aligned, 0, 0, mutableText->value, elem->color);
	else
		fntRenderString(elem->font, elem->posX, elem->posY, elem->aligned, elem->width, elem->height, mutableText->value, elem->color);
}

static void initStaticText(char* themePath, config_set_t* themeConfig, theme_t* theme, theme_element_t* elem, char* name,
		char* value) {

	char elemProp[64];

	snprintf(elemProp, 64, "%s_value", name);
	configGetStr(themeConfig, elemProp, &value);
	if (value) {
		elem->extended = initMutableText(themePath, themeConfig, theme, name, TYPE_STATIC_TEXT, elem, value, NULL, DISPLAY_ALWAYS, SIZING_NONE);
		elem->endElem = &endMutableText;
		elem->drawElem = &drawStaticText;
	} else
		LOG("THEMES StaticText %s: NO value, elem disabled !!\n", name);
}

// AttributeText ////////////////////////////////////////////////////////////////////////////////////////////////////////////

static void drawAttributeText(struct menu_list* menu, struct submenu_list* item, config_set_t* config, struct theme_element* elem) {
	mutable_text_t* mutableText = (mutable_text_t*) elem->extended;
	if (config) {
		if (mutableText->currentConfigId != config->uid) {
			// force refresh
			mutableText->currentConfigId = config->uid;
			mutableText->currentValue = NULL;
			if (configGetStr(config, mutableText->value, &mutableText->currentValue)) {
				if (mutableText->sizingMode == SIZING_WRAP)
					fntFitString(elem->font, mutableText->currentValue, elem->width);
			}
		}
		if (mutableText->currentValue) {
			if (mutableText->displayMode == DISPLAY_NEVER) {
				if (mutableText->sizingMode == SIZING_NONE)
					fntRenderString(elem->font, elem->posX, elem->posY, elem->aligned, 0, 0, mutableText->currentValue, elem->color);
				else
					fntRenderString(elem->font, elem->posX, elem->posY, elem->aligned, elem->width, elem->height, mutableText->currentValue, elem->color);
			} else {
				char result[300];
				snprintf(result, 300, "%s%s", mutableText->alias, mutableText->currentValue);
				if (mutableText->sizingMode == SIZING_NONE)
					fntRenderString(elem->font, elem->posX, elem->posY, elem->aligned, 0, 0, result, elem->color);
				else
					fntRenderString(elem->font, elem->posX, elem->posY, elem->aligned, elem->width, elem->height, result, elem->color);
			}
			return;
		}
	}
	if (mutableText->displayMode == DISPLAY_ALWAYS) {
		if (mutableText->sizingMode == SIZING_NONE)
			fntRenderString(elem->font, elem->posX, elem->posY, elem->aligned, 0, 0, mutableText->alias, elem->color);
		else
			fntRenderString(elem->font, elem->posX, elem->posY, elem->aligned, elem->width, elem->height, mutableText->alias, elem->color);
	}
}

static void initAttributeText(char* themePath, config_set_t* themeConfig, theme_t* theme, theme_element_t* elem, char* name,
		char* attribute) {

	char elemProp[64];

	snprintf(elemProp, 64, "%s_attribute", name);
	configGetStr(themeConfig, elemProp, &attribute);
	if (attribute) {
		elem->extended = initMutableText(themePath, themeConfig, theme, name, TYPE_ATTRIBUTE_TEXT, elem, attribute, NULL, DISPLAY_ALWAYS, SIZING_NONE);
		elem->endElem = &endMutableText;
		elem->drawElem = &drawAttributeText;
	} else
		LOG("THEMES AttributeText %s: NO attribute, elem disabled !!\n", name);
}

// Common functions for Image ///////////////////////////////////////////////////////////////////////////////////////////////

static void findDuplicate(theme_element_t* first, char* cachePattern, char* defaultTexture, char* overlayTexture, mutable_image_t* target) {
	theme_element_t* elem = first;
	while (elem) {
		if ((elem->type == TYPE_STATIC_IMAGE) || (elem->type == TYPE_ATTRIBUTE_IMAGE) || (elem->type == TYPE_GAME_IMAGE) || (elem->type == TYPE_BACKGROUND)) {
			mutable_image_t* source = (mutable_image_t*) elem->extended;

			if (cachePattern && source->cache && !strcmp(cachePattern, source->cache->suffix)) {
				target->cache = source->cache;
				target->cacheLinked = 1;
				LOG("THEMES Re-using a cache for pattern %s\n", cachePattern);
			}

			if (defaultTexture && source->defaultTexture && !strcmp(defaultTexture, source->defaultTexture->name)) {
				target->defaultTexture = source->defaultTexture;
				target->defaultTextureLinked = 1;
				LOG("THEMES Re-using the default texture for %s\n", defaultTexture);
			}

			if (overlayTexture && source->overlayTexture && !strcmp(overlayTexture, source->overlayTexture->name)) {
				target->overlayTexture = source->overlayTexture;
				target->overlayTextureLinked = 1;
				LOG("THEMES Re-using the overlay texture for %s\n", overlayTexture);
			}
		}

		elem = elem->next;
	}
}

static void freeImageTexture(image_texture_t* texture) {
	if (texture) {
		if (texture->source.Mem)
			free(texture->source.Mem);

		free(texture->name);

		free(texture);
	}
}

static image_texture_t* initImageTexture(char* themePath, config_set_t* themeConfig, char* name, char* imgName, int isOverlay) {
	image_texture_t* texture = (image_texture_t*) malloc(sizeof(image_texture_t));
	texPrepare(&texture->source, isOverlay ? GS_PSM_CT32 : GS_PSM_CT24);
	texture->name = NULL;

	char path[255];
	snprintf(path, 255, "%s%s", themePath, imgName);
	if (texDiscoverLoad(&texture->source, path, -1, isOverlay ? GS_PSM_CT32 : GS_PSM_CT24) >= 0) {
		int length = strlen(imgName) + 1;
		texture->name = (char*) malloc(length * sizeof(char));
		memcpy(texture->name, imgName, length);

		if (isOverlay) {
			int intValue;
			char elemProp[64];
			snprintf(elemProp, 64, "%s_overlay_ulx", name);
			if (configGetInt(themeConfig, elemProp, &intValue))
				texture->upperLeft_x = intValue;
			snprintf(elemProp, 64, "%s_overlay_uly", name);
			if (configGetInt(themeConfig, elemProp, &intValue))
				texture->upperLeft_y = intValue;
			snprintf(elemProp, 64, "%s_overlay_urx", name);
			if (configGetInt(themeConfig, elemProp, &intValue))
				texture->upperRight_x = intValue;
			snprintf(elemProp, 64, "%s_overlay_ury", name);
			if (configGetInt(themeConfig, elemProp, &intValue))
				texture->upperRight_y = intValue;
			snprintf(elemProp, 64, "%s_overlay_llx", name);
			if (configGetInt(themeConfig, elemProp, &intValue))
				texture->lowerLeft_x = intValue;
			snprintf(elemProp, 64, "%s_overlay_lly", name);
			if (configGetInt(themeConfig, elemProp, &intValue))
				texture->lowerLeft_y = intValue;
			snprintf(elemProp, 64, "%s_overlay_lrx", name);
			if (configGetInt(themeConfig, elemProp, &intValue))
				texture->lowerRight_x = intValue;
			snprintf(elemProp, 64, "%s_overlay_lry", name);
			if (configGetInt(themeConfig, elemProp, &intValue))
				texture->lowerRight_y = intValue;
		}
	} else {
		freeImageTexture(texture);
		texture = NULL;
	}

	return texture;
}

static void endMutableImage(struct theme_element* elem) {
	mutable_image_t* mutableImage = (mutable_image_t*) elem->extended;
	if (mutableImage) {
		if (mutableImage->cache && !mutableImage->cacheLinked)
			cacheDestroyCache(mutableImage->cache);

		if (mutableImage->defaultTexture && !mutableImage->defaultTextureLinked)
			freeImageTexture(mutableImage->defaultTexture);

		if (mutableImage->overlayTexture && !mutableImage->overlayTextureLinked)
			freeImageTexture(mutableImage->overlayTexture);

		free(mutableImage);
	}

	free(elem);
}

static mutable_image_t* initMutableImage(char* themePath, config_set_t* themeConfig, theme_t* theme, char* name, int type,
		char* cachePattern, int cacheCount, char* defaultTexture, char* overlayTexture) {

	mutable_image_t* mutableImage = (mutable_image_t*) malloc(sizeof(mutable_image_t));
	mutableImage->currentUid = -1;
	mutableImage->currentConfigId = 0;
	mutableImage->currentValue = NULL;
	mutableImage->cache = NULL;
	mutableImage->cacheLinked = 0;
	mutableImage->defaultTexture = NULL;
	mutableImage->defaultTextureLinked = 0;
	mutableImage->overlayTexture = NULL;
	mutableImage->overlayTextureLinked = 0;

	char elemProp[64];

	if (type == TYPE_ATTRIBUTE_IMAGE) {
		snprintf(elemProp, 64, "%s_attribute", name);
		configGetStr(themeConfig, elemProp, &cachePattern);
		LOG("THEMES MutableImage %s: type: %s using cache pattern: %s\n", name, elementsType[type], cachePattern);
	} else if ((type == TYPE_GAME_IMAGE) || type == (TYPE_BACKGROUND)) {
		snprintf(elemProp, 64, "%s_pattern", name);
		configGetStr(themeConfig, elemProp, &cachePattern);
		snprintf(elemProp, 64, "%s_count", name);
		configGetInt(themeConfig, elemProp, &cacheCount);
		LOG("THEMES MutableImage %s: type: %s using cache pattern: %s count: %d\n", name, elementsType[type], cachePattern, cacheCount);
	}

	snprintf(elemProp, 64, "%s_default", name);
	configGetStr(themeConfig, elemProp, &defaultTexture);

	if (type != TYPE_BACKGROUND) {
		snprintf(elemProp, 64, "%s_overlay", name);
		configGetStr(themeConfig, elemProp, &overlayTexture);
	}

	findDuplicate(theme->mainElems.first, cachePattern, defaultTexture, overlayTexture, mutableImage);
	findDuplicate(theme->infoElems.first, cachePattern, defaultTexture, overlayTexture, mutableImage);

	if (cachePattern && !mutableImage->cache) {
		if (type == TYPE_ATTRIBUTE_IMAGE)
			mutableImage->cache = cacheInitCache(-1, themePath, 0, cachePattern, 1);
		else
			mutableImage->cache = cacheInitCache(theme->gameCacheCount++, "ART", 1, cachePattern, cacheCount);
	}

	if (defaultTexture && !mutableImage->defaultTexture)
		mutableImage->defaultTexture = initImageTexture(themePath, themeConfig, name, defaultTexture, 0);

	if (overlayTexture && !mutableImage->overlayTexture)
		mutableImage->overlayTexture = initImageTexture(themePath, themeConfig, name, overlayTexture, 1);

	return mutableImage;
}

// StaticImage //////////////////////////////////////////////////////////////////////////////////////////////////////////////

static void drawStaticImage(struct menu_list* menu, struct submenu_list* item, config_set_t* config, struct theme_element* elem) {
	mutable_image_t* staticImage = (mutable_image_t*) elem->extended;
	if (staticImage->overlayTexture) {
		rmDrawOverlayPixmap(&staticImage->overlayTexture->source, elem->posX, elem->posY, elem->aligned, elem->width, elem->height, elem->scaled, gDefaultCol,
				&staticImage->defaultTexture->source, staticImage->overlayTexture->upperLeft_x, staticImage->overlayTexture->upperLeft_y, staticImage->overlayTexture->upperRight_x, staticImage->overlayTexture->upperRight_y,
				staticImage->overlayTexture->lowerLeft_x, staticImage->overlayTexture->lowerLeft_y, staticImage->overlayTexture->lowerRight_x, staticImage->overlayTexture->lowerRight_y);
	} else
		rmDrawPixmap(&staticImage->defaultTexture->source, elem->posX, elem->posY, elem->aligned, elem->width, elem->height, elem->scaled, gDefaultCol);
}

static void initStaticImage(char* themePath, config_set_t* themeConfig, theme_t* theme, theme_element_t* elem, char* name,
		char* imageName) {

	mutable_image_t* mutableImage = initMutableImage(themePath, themeConfig, theme, name, elem->type, NULL, 0, imageName, NULL);
	elem->extended = mutableImage;
	elem->endElem = &endMutableImage;

	if (mutableImage->defaultTexture)
		elem->drawElem = &drawStaticImage;
	else
		LOG("THEMES StaticImage %s: NO image name, elem disabled !!\n", name);
}

// GameImage ////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static GSTEXTURE* getGameImageTexture(image_cache_t* cache, void* support, struct submenu_item* item) {
	if (gEnableArt) {
		item_list_t * list = (item_list_t *) support;
		char* startup = list->itemGetStartup(item->id);
		return cacheGetTexture(cache, list, &item->cache_id[cache->userId], &item->cache_uid[cache->userId], startup);
	}

	return NULL;
}

static void drawGameImage(struct menu_list* menu, struct submenu_list* item, config_set_t* config, struct theme_element* elem) {
	mutable_image_t* gameImage = (mutable_image_t*) elem->extended;
	if (item) {
		GSTEXTURE* texture = getGameImageTexture(gameImage->cache, menu->item->userdata, &item->item);
		if (!texture || !texture->Mem) {
			if (gameImage->defaultTexture)
				texture = &gameImage->defaultTexture->source;
			else {
				if (elem->type == TYPE_BACKGROUND)
					guiDrawBGPlasma();
				return;
			}
		}

		if (gameImage->overlayTexture) {
			rmDrawOverlayPixmap(&gameImage->overlayTexture->source, elem->posX, elem->posY, elem->aligned, elem->width, elem->height, elem->scaled, gDefaultCol,
				texture, gameImage->overlayTexture->upperLeft_x, gameImage->overlayTexture->upperLeft_y, gameImage->overlayTexture->upperRight_x, gameImage->overlayTexture->upperRight_y,
				gameImage->overlayTexture->lowerLeft_x, gameImage->overlayTexture->lowerLeft_y, gameImage->overlayTexture->lowerRight_x, gameImage->overlayTexture->lowerRight_y);
		} else
			rmDrawPixmap(texture, elem->posX, elem->posY, elem->aligned, elem->width, elem->height, elem->scaled, gDefaultCol);

	} else if (elem->type == TYPE_BACKGROUND) {
		if (gameImage->defaultTexture)
			rmDrawPixmap(&gameImage->defaultTexture->source, elem->posX, elem->posY, elem->aligned, elem->width, elem->height, elem->scaled, gDefaultCol);
		else
			guiDrawBGPlasma();
	}
}

static void initGameImage(char* themePath, config_set_t* themeConfig, theme_t* theme, theme_element_t* elem, char* name,
		char* pattern, int count, char* texture, char* overlay) {

	mutable_image_t* mutableImage = initMutableImage(themePath, themeConfig, theme, name, elem->type, pattern, count, texture, overlay);
	elem->extended = mutableImage;
	elem->endElem = &endMutableImage;

	if (mutableImage->cache)
		elem->drawElem = &drawGameImage;
	else
		LOG("THEMES GameImage %s: NO pattern, elem disabled !!\n", name);
}

// AttributeImage ///////////////////////////////////////////////////////////////////////////////////////////////////////////

static void drawAttributeImage(struct menu_list* menu, struct submenu_list* item, config_set_t* config, struct theme_element* elem) {
	mutable_image_t* attributeImage = (mutable_image_t*) elem->extended;
	if (config) {
		if (attributeImage->currentConfigId != config->uid) {
			// force refresh
			attributeImage->currentUid = -1;
			attributeImage->currentConfigId = config->uid;
			attributeImage->currentValue = NULL;
			configGetStr(config, attributeImage->cache->suffix, &attributeImage->currentValue);
		}
		if (attributeImage->currentValue) {
			int posZ = 0;
			GSTEXTURE* texture = cacheGetTexture(attributeImage->cache, menu->item->userdata, &posZ, &attributeImage->currentUid, attributeImage->currentValue);
			if (texture && texture->Mem) {
				if (attributeImage->overlayTexture) {
					rmDrawOverlayPixmap(&attributeImage->overlayTexture->source, elem->posX, elem->posY, elem->aligned, elem->width, elem->height, elem->scaled, gDefaultCol,
						texture, attributeImage->overlayTexture->upperLeft_x, attributeImage->overlayTexture->upperLeft_y, attributeImage->overlayTexture->upperRight_x, attributeImage->overlayTexture->upperRight_y,
						attributeImage->overlayTexture->lowerLeft_x, attributeImage->overlayTexture->lowerLeft_y, attributeImage->overlayTexture->lowerRight_x, attributeImage->overlayTexture->lowerRight_y);
				} else
					rmDrawPixmap(texture, elem->posX, elem->posY, elem->aligned, elem->width, elem->height, elem->scaled, gDefaultCol);

				return;
			}
		}
	}
	if (attributeImage->defaultTexture)
		rmDrawPixmap(&attributeImage->defaultTexture->source, elem->posX, elem->posY, elem->aligned, elem->width, elem->height, elem->scaled, gDefaultCol);
}

static void initAttributeImage(char* themePath, config_set_t* themeConfig, theme_t* theme, theme_element_t* elem, char* name) {
	mutable_image_t* mutableImage = initMutableImage(themePath, themeConfig, theme, name, elem->type, NULL, 1, NULL, NULL);
	elem->extended = mutableImage;
	elem->endElem = &endMutableImage;

	if (mutableImage->cache)
		elem->drawElem = &drawAttributeImage;
	else
		LOG("THEMES AttributeImage %s: NO attribute, elem disabled !!\n", name);
}

// BasicElement /////////////////////////////////////////////////////////////////////////////////////////////////////////////

static void endBasic(theme_element_t* elem) {
	if (elem->extended)
		free(elem->extended);

	free(elem);
}

static theme_element_t* initBasic(char* themePath, config_set_t* themeConfig, theme_t* theme,
		char* name,	int type, int x, int y, short aligned, int w, int h, short scaled, u64 color, int font) {

	int intValue;
	unsigned char charColor[3];
	char* temp;
	char elemProp[64];

	theme_element_t* elem = (theme_element_t*) malloc(sizeof(theme_element_t));

	elem->type = type;
	elem->extended = NULL;
	elem->drawElem = NULL;
	elem->endElem = &endBasic;
	elem->next = NULL;

	snprintf(elemProp, 64, "%s_x", name);
	if (configGetStr(themeConfig, elemProp, &temp)) {
		if (!strncmp(temp, "POS_MID", 7))
			x = screenWidth >> 1;
		else
			x = atoi(temp);
	}
	if (x < 0)
		elem->posX = screenWidth + x;
	else
		elem->posX = x;

	snprintf(elemProp, 64, "%s_y", name);
	if (configGetStr(themeConfig, elemProp, &temp)) {
		if (!strncmp(temp, "POS_MID", 7))
			y = screenHeight >> 1;
		else
			y = atoi(temp);
	}
	if (y < 0)
		elem->posY = ceil((screenHeight + y) * theme->usedHeight / screenHeight);
	else
		elem->posY = y;

	snprintf(elemProp, 64, "%s_width", name);
	if (configGetStr(themeConfig, elemProp, &temp)) {
		if (!strncmp(temp, "DIM_INF", 7))
			elem->width = screenWidth;
		else
			elem->width = atoi(temp);
	} else
		elem->width = w;

	snprintf(elemProp, 64, "%s_height", name);
	if (configGetStr(themeConfig, elemProp, &temp)) {
		if (!strncmp(temp, "DIM_INF", 7))
			elem->height = screenHeight;
		else
			elem->height = atoi(temp);
	} else
		elem->height = h;

	snprintf(elemProp, 64, "%s_aligned", name);
	if (configGetInt(themeConfig, elemProp, &intValue))
		elem->aligned = intValue;
	else
		elem->aligned = aligned;

	snprintf(elemProp, 64, "%s_scaled", name);
	if (configGetInt(themeConfig, elemProp, &intValue))
		elem->scaled = intValue;
	else
		elem->scaled = scaled;

	snprintf(elemProp, 64, "%s_color", name);
	if (configGetColor(themeConfig, elemProp, charColor))
		elem->color = GS_SETREG_RGBA(charColor[0], charColor[1], charColor[2], 0xff);
	else
		elem->color = color;

	elem->font = font;
	snprintf(elemProp, 64, "%s_font", name);
	if (configGetInt(themeConfig, elemProp, &intValue)) {
		if (intValue > 0 && intValue < THM_MAX_FONTS)
			elem->font = theme->fonts[intValue];
	}

	return elem;
}

// Internal elements ////////////////////////////////////////////////////////////////////////////////////////////////////////

static void drawBackground(struct menu_list* menu, struct submenu_list* item, config_set_t* config, struct theme_element* elem) {
	guiDrawBGPlasma();
}

static void initBackground(char* themePath, config_set_t* themeConfig, theme_t* theme, theme_element_t* elem, char* name,
		char* pattern, int count, char* texture) {

	mutable_image_t* mutableImage = initMutableImage(themePath, themeConfig, theme, name, elem->type, pattern, count, texture, NULL);
	elem->extended = mutableImage;
	elem->endElem = &endMutableImage;

	if (mutableImage->cache)
		elem->drawElem = &drawGameImage;
	else if (mutableImage->defaultTexture)
		elem->drawElem = &drawStaticImage;
	else
		elem->drawElem = &drawBackground;
}

static void drawMenuIcon(struct menu_list* menu, struct submenu_list* item, config_set_t* config, struct theme_element* elem) {
	GSTEXTURE* menuIconTex = thmGetTexture(menu->item->icon_id);
	if (menuIconTex && menuIconTex->Mem)
		rmDrawPixmap(menuIconTex, elem->posX, elem->posY, elem->aligned, elem->width, elem->height, elem->scaled, gDefaultCol);
}

static void drawMenuText(struct menu_list* menu, struct submenu_list* item, config_set_t* config, struct theme_element* elem) {
	GSTEXTURE* leftIconTex = NULL, *rightIconTex = NULL;
	if (menu->prev)
		leftIconTex = thmGetTexture(LEFT_ICON);
	if (menu->next)
		rightIconTex = thmGetTexture(RIGHT_ICON);

	if (elem->aligned) {
		int offset = elem->width >> 1;
		if (leftIconTex && leftIconTex->Mem)
			rmDrawPixmap(leftIconTex, elem->posX - offset, elem->posY, elem->aligned, leftIconTex->Width, leftIconTex->Height, elem->scaled, gDefaultCol);
		if (rightIconTex && rightIconTex->Mem)
			rmDrawPixmap(rightIconTex, elem->posX + offset, elem->posY, elem->aligned, rightIconTex->Width, rightIconTex->Height, elem->scaled, gDefaultCol);
	}
	else {
		if (leftIconTex && leftIconTex->Mem)
			rmDrawPixmap(leftIconTex, elem->posX - leftIconTex->Width, elem->posY, elem->aligned, leftIconTex->Width, leftIconTex->Height, elem->scaled, gDefaultCol);
		if (rightIconTex && rightIconTex->Mem)
			rmDrawPixmap(rightIconTex, elem->posX + elem->width, elem->posY, elem->aligned, rightIconTex->Width, rightIconTex->Height, elem->scaled, gDefaultCol);
	}
	fntRenderString(elem->font, elem->posX, elem->posY, elem->aligned, 0, 0, menuItemGetText(menu->item), elem->color);
}

static void drawItemsList(struct menu_list* menu, struct submenu_list* item, config_set_t* config, struct theme_element* elem) {
	if (item) {
		items_list_t* itemsList = (items_list_t*) elem->extended;

		int posX = elem->posX, posY = elem->posY;
		if (elem->aligned) {
			posX -= elem->width >> 1;
			posY -= elem->height >> 1;
		}

		submenu_list_t *ps  = menu->item->pagestart;
		int others = 0;
		u64 color;
		while (ps && (others++ < itemsList->displayedItems)) {
			if (ps == item)
				color = gTheme->selTextColor;
			else
				color = elem->color;

			if (itemsList->decoratorImage) {
				GSTEXTURE* itemIconTex = getGameImageTexture(itemsList->decoratorImage->cache, menu->item->userdata, &ps->item);
				if (itemIconTex && itemIconTex->Mem)
					rmDrawPixmap(itemIconTex, posX, posY, ALIGN_NONE, DECORATOR_SIZE, DECORATOR_SIZE, elem->scaled, gDefaultCol);
				else {
					if (itemsList->decoratorImage->defaultTexture)
						rmDrawPixmap(&itemsList->decoratorImage->defaultTexture->source, posX, posY, ALIGN_NONE, DECORATOR_SIZE, DECORATOR_SIZE, elem->scaled, gDefaultCol);
				}
				fntRenderString(elem->font, posX + DECORATOR_SIZE, posY, ALIGN_NONE, elem->width, elem->height, submenuItemGetText(&ps->item), color);
			} else
				fntRenderString(elem->font, posX, posY, ALIGN_NONE, elem->width, elem->height, submenuItemGetText(&ps->item), color);

			posY += MENU_ITEM_HEIGHT;
			ps = ps->next;
		}
	}
}

static void initItemsList(char* themePath, config_set_t* themeConfig, theme_t* theme, theme_element_t* elem, char* name, char* decorator) {
	char elemProp[64];

	items_list_t* itemsList = (items_list_t*) malloc(sizeof(items_list_t));

	if (elem->width == DIM_UNDEF)
		elem->width = screenWidth;

	if (elem->height == DIM_UNDEF)
		elem->height = theme->usedHeight - (MENU_POS_V + HINT_HEIGHT);

	itemsList->displayedItems = elem->height / MENU_ITEM_HEIGHT;
	LOG("THEMES ItemsList %s: displaying %d elems, item height: %d\n", name, itemsList->displayedItems, elem->height);

	itemsList->decorator = NULL;
	snprintf(elemProp, 64, "%s_decorator", name);
	configGetStr(themeConfig, elemProp, &decorator);
	if (decorator)
		itemsList->decorator = decorator; // Will be used later (thmValidate)

	itemsList->decoratorImage = NULL;

	elem->extended = itemsList;
	// elem->endElem = &endBasic; does the job

	elem->drawElem = &drawItemsList;
}

static void drawItemText(struct menu_list* menu, struct submenu_list* item, config_set_t* config, struct theme_element* elem) {
	if (item) {
		item_list_t *support = menu->item->userdata;
		fntRenderString(elem->font, elem->posX, elem->posY, elem->aligned, 0, 0, support->itemGetStartup(item->item.id), elem->color);
	}
}

static void drawHintText(struct menu_list* menu, struct submenu_list* item, config_set_t* config, struct theme_element* elem) {
	menu_hint_item_t* hint = menu->item->hints;
	if (hint) {
		int x = elem->posX;

		for (; hint; hint = hint->next) {
			x = guiDrawIconAndText(hint->icon_id, hint->text_id, elem->font, x, elem->posY, elem->color);
			x += elem->width;
		}
	}
}

static void drawInfoHintText(struct menu_list* menu, struct submenu_list* item, config_set_t* config, struct theme_element* elem) {
	int x = elem->posX;
	x = guiDrawIconAndText(CROSS_ICON, _STR_RUN, elem->font, x, elem->posY, elem->color);
	x += elem->width;
	x = guiDrawIconAndText(CIRCLE_ICON, _STR_O_BACK, elem->font, x, elem->posY, elem->color);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static void validateGUIElems(char* themePath, config_set_t* themeConfig, theme_t* theme) {
	// 1. check we have a valid Background elements
	if ( !theme->mainElems.first || (theme->mainElems.first->type != TYPE_BACKGROUND) ) {
		LOG("THEMES No valid background found for main, add default BG_ART\n");
		theme_element_t* backgroundElem = initBasic(themePath, themeConfig, theme, "bg", TYPE_BACKGROUND, 0, 0, ALIGN_NONE, screenWidth, screenHeight, SCALING_NONE, gDefaultCol, theme->fonts[0]);
		if (themePath)
			initBackground(themePath, themeConfig, theme, backgroundElem, "bg", "BG", 1, "background");
		else
			initBackground(themePath, themeConfig, theme, backgroundElem, "bg", "BG", 1, NULL);
		backgroundElem->next = theme->mainElems.first;
		theme->mainElems.first = backgroundElem;
	}

	if (theme->infoElems.first) {
		if (theme->infoElems.first->type != TYPE_BACKGROUND) {
			LOG("THEMES No valid background found for info, add default BG_ART\n");
			theme_element_t* backgroundElem = initBasic(themePath, themeConfig, theme, "bg", TYPE_BACKGROUND, 0, 0, ALIGN_NONE, screenWidth, screenHeight, SCALING_NONE, gDefaultCol, theme->fonts[0]);
			if (themePath)
				initBackground(themePath, themeConfig, theme, backgroundElem, "bg", "BG", 1, "background");
			else
				initBackground(themePath, themeConfig, theme, backgroundElem, "bg", "BG", 1, NULL);
			backgroundElem->next = theme->infoElems.first;
			theme->infoElems.first = backgroundElem;
		}
	}

	// 2. check we have a valid ItemsList element, and link its decorator to the target element
	if (theme->itemsList) {
		items_list_t* itemsList = (items_list_t*) theme->itemsList->extended;
		if (itemsList->decorator) {
			// Second pass to find the decorator
			theme_element_t* decoratorElem = theme->mainElems.first;
			while (decoratorElem) {
				if (decoratorElem->type == TYPE_GAME_IMAGE) {
					mutable_image_t* gameImage = (mutable_image_t*) decoratorElem->extended;
					if (!strcmp(itemsList->decorator, gameImage->cache->suffix)) {
						// if user want to cache less than displayed items, then disable itemslist icons, if not would load constantly
						if (gameImage->cache->count >= itemsList->displayedItems)
							itemsList->decoratorImage = gameImage;
						break;
					}
				}

				decoratorElem = decoratorElem->next;
			}
			itemsList->decorator = NULL;
		}
	} else {
		LOG("THEMES No itemsList found, adding a default one\n");
		theme->itemsList = initBasic(themePath, themeConfig, theme, "il", TYPE_ITEMS_LIST, 150, MENU_POS_V, ALIGN_NONE, DIM_UNDEF, DIM_UNDEF, SCALING_RATIO, theme->textColor, theme->fonts[0]);
		initItemsList(themePath, themeConfig, theme, theme->itemsList, "il", NULL);
		theme->itemsList->next = theme->mainElems.first->next; // Position the itemsList as second element (right after the Background)
		theme->mainElems.first->next = theme->itemsList;
	}
}

static int addGUIElem(char* themePath, config_set_t* themeConfig, theme_t* theme, theme_elems_t* elems, char* type, char* name) {

	int enabled = 1;
	char elemProp[64];
	theme_element_t* elem = NULL;

	snprintf(elemProp, 64, "%s_enabled", name);
	configGetInt(themeConfig, elemProp, &enabled);

	if (enabled) {
		snprintf(elemProp, 64, "%s_type", name);
		configGetStr(themeConfig, elemProp, &type);
		if (type) {
			if (!strcmp(elementsType[TYPE_ATTRIBUTE_TEXT], type)) {
				elem = initBasic(themePath, themeConfig, theme, name, TYPE_ATTRIBUTE_TEXT, 0, 0, ALIGN_CENTER, DIM_UNDEF, DIM_UNDEF, SCALING_RATIO, theme->textColor, theme->fonts[0]);
				initAttributeText(themePath, themeConfig, theme, elem, name, NULL);
			} else if (!strcmp(elementsType[TYPE_STATIC_TEXT], type)) {
				elem = initBasic(themePath, themeConfig, theme, name, TYPE_STATIC_TEXT, 0, 0, ALIGN_CENTER, DIM_UNDEF, DIM_UNDEF, SCALING_RATIO, theme->textColor, theme->fonts[0]);
				initStaticText(themePath, themeConfig, theme, elem, name, NULL);
			} else if (!strcmp(elementsType[TYPE_ATTRIBUTE_IMAGE], type)) {
				elem = initBasic(themePath, themeConfig, theme, name, TYPE_ATTRIBUTE_IMAGE, 0, 0, ALIGN_CENTER, DIM_UNDEF, DIM_UNDEF, SCALING_RATIO, gDefaultCol, theme->fonts[0]);
				initAttributeImage(themePath, themeConfig, theme, elem, name);
			} else if (!strcmp(elementsType[TYPE_GAME_IMAGE], type)) {
				elem = initBasic(themePath, themeConfig, theme, name, TYPE_GAME_IMAGE, 0, 0, ALIGN_CENTER, DIM_UNDEF, DIM_UNDEF, SCALING_RATIO, gDefaultCol, theme->fonts[0]);
				initGameImage(themePath, themeConfig, theme, elem, name, NULL, 1, NULL, NULL);
			} else if (!strcmp(elementsType[TYPE_STATIC_IMAGE], type)) {
				elem = initBasic(themePath, themeConfig, theme, name, TYPE_STATIC_IMAGE, 0, 0, ALIGN_CENTER, DIM_UNDEF, DIM_UNDEF, SCALING_RATIO, gDefaultCol, theme->fonts[0]);
				initStaticImage(themePath, themeConfig, theme, elem, name, NULL);
			} else if (!strcmp(elementsType[TYPE_BACKGROUND], type)) {
				if (!elems->first) { // Background elem can only be the first one
					elem = initBasic(themePath, themeConfig, theme, name, TYPE_BACKGROUND, 0, 0, ALIGN_NONE, screenWidth, screenHeight, SCALING_NONE, gDefaultCol, theme->fonts[0]);
					initBackground(themePath, themeConfig, theme, elem, name, NULL, 1, NULL);
				}
			} else if (!strcmp(elementsType[TYPE_MENU_ICON], type)) {
				elem = initBasic(themePath, themeConfig, theme, name, TYPE_MENU_ICON, 40, 40, ALIGN_CENTER, DIM_UNDEF, DIM_UNDEF, SCALING_RATIO, gDefaultCol, theme->fonts[0]);
				elem->drawElem = &drawMenuIcon;
			} else if (!strcmp(elementsType[TYPE_MENU_TEXT], type)) {
				elem = initBasic(themePath, themeConfig, theme, name, TYPE_MENU_TEXT, screenWidth >> 1, 20, ALIGN_CENTER, 200, 20, SCALING_RATIO, theme->textColor, theme->fonts[0]);
				elem->drawElem = &drawMenuText;
			} else if (!strcmp(elementsType[TYPE_ITEMS_LIST], type)) {
				if (!theme->itemsList) {
					elem = initBasic(themePath, themeConfig, theme, name, TYPE_ITEMS_LIST, 150, MENU_POS_V, ALIGN_NONE, DIM_UNDEF, DIM_UNDEF, SCALING_RATIO, theme->textColor, theme->fonts[0]);
					initItemsList(themePath, themeConfig, theme, elem, name, NULL);
					theme->itemsList = elem;
				}
			} else if (!strcmp(elementsType[TYPE_ITEM_ICON], type)) {
				elem = initBasic(themePath, themeConfig, theme, name, TYPE_GAME_IMAGE, 80, theme->usedHeight >> 1, ALIGN_CENTER, DIM_UNDEF, DIM_UNDEF, SCALING_RATIO, gDefaultCol, theme->fonts[0]);
				initGameImage(themePath, themeConfig, theme, elem, name, "ICO", 20, NULL, NULL);
			} else if (!strcmp(elementsType[TYPE_ITEM_COVER], type)) {
				elem = initBasic(themePath, themeConfig, theme, name, TYPE_GAME_IMAGE, 520, theme->usedHeight >> 1, ALIGN_CENTER, DIM_UNDEF, DIM_UNDEF, SCALING_RATIO, gDefaultCol, theme->fonts[0]);
				initGameImage(themePath, themeConfig, theme, elem, name, "COV", 10, NULL, NULL);
			} else if (!strcmp(elementsType[TYPE_ITEM_TEXT], type)) {
				elem = initBasic(themePath, themeConfig, theme, name, TYPE_ITEM_TEXT, 520, 370, ALIGN_CENTER, DIM_UNDEF, DIM_UNDEF, SCALING_RATIO, theme->textColor, theme->fonts[0]);
				elem->drawElem = &drawItemText;
			} else if (!strcmp(elementsType[TYPE_HINT_TEXT], type)) {
				elem = initBasic(themePath, themeConfig, theme, name, TYPE_HINT_TEXT, 16, -HINT_HEIGHT, ALIGN_NONE, 12, 20, SCALING_RATIO, theme->textColor, theme->fonts[0]);
				elem->drawElem = &drawHintText;
			} else if (!strcmp(elementsType[TYPE_INFO_HINT_TEXT], type)) {
				elem = initBasic(themePath, themeConfig, theme, name, TYPE_INFO_HINT_TEXT, 16, -HINT_HEIGHT, ALIGN_NONE, 12, 20, SCALING_RATIO, theme->textColor, theme->fonts[0]);
				elem->drawElem = &drawInfoHintText;
			} else if (!strcmp(elementsType[TYPE_LOADING_ICON], type)) {
				if (!theme->loadingIcon)
					theme->loadingIcon = initBasic(themePath, themeConfig, theme, name, TYPE_LOADING_ICON, -40, -60, ALIGN_CENTER, DIM_UNDEF, DIM_UNDEF, SCALING_RATIO, gDefaultCol, theme->fonts[0]);
			}

			if (elem) {
				if (!elems->first)
					elems->first = elem;

				if (!elems->last)
					elems->last = elem;
				else {
					elems->last->next = elem;
					elems->last = elem;
				}
			}
		} else
			return 0; // ends the reading of elements
	}

	return 1;
}

static void freeGUIElems(theme_elems_t* elems) {
	theme_element_t* elem = elems->first;
	while (elem) {
		elems->first = elem->next;
		elem->endElem(elem);
		elem = elems->first;
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

GSTEXTURE* thmGetTexture(unsigned int id) {
	if (id >= TEXTURES_COUNT)
		return NULL;
	else {
		// see if the texture is valid
		GSTEXTURE* txt = &gTheme->textures[id];
		
		if (txt->Mem)
			return txt;
		else
			return NULL;
	}
}

static void thmFree(theme_t* theme) {
	if (theme) {
		// free elements
		freeGUIElems(&theme->mainElems);
		freeGUIElems(&theme->infoElems);

		// free textures
		GSTEXTURE* texture;
		int id = 0;
		for(; id < TEXTURES_COUNT; id++) {
			texture = &theme->textures[id];
			if (texture->Mem != NULL) {
				free(texture->Mem);
				texture->Mem = NULL;
			}
		}

		// free fonts
		for (id = 0; id < THM_MAX_FONTS; ++id)
			fntRelease(theme->fonts[id]);

		free(theme);
		theme = NULL;
	}
}

static int thmReadEntry(int index, char* path, char* separator, char* name, unsigned int mode) {
	if (FIO_SO_ISDIR(mode) && strstr(name, "thm_")) {
		theme_file_t* currTheme = &themes[nThemes + index];

		int length = strlen(name) - 4 + 1;
		currTheme->name = (char*) malloc(length * sizeof(char));
		memcpy(currTheme->name, name + 4, length);
		currTheme->name[length - 1] = '\0';

		length = strlen(path) + 1 + strlen(name) + 1 + 1;
		currTheme->filePath = (char*) malloc(length * sizeof(char));
		sprintf(currTheme->filePath, "%s%s%s%s", path, separator, name, separator);

		LOG("THEMES Theme found: %s\n", currTheme->filePath);

		index++;
	}
	return index;
}

/* themePath must contains the leading separator (as it is dependent of the device, we can't know here) */
static int thmLoadResource(int texId, char* themePath, short psm, int useDefault) {
	int success = -1;
	GSTEXTURE* texture = &gTheme->textures[texId];

	if (themePath != NULL)
		success = texDiscoverLoad(texture, themePath, texId, psm); // only set success here

	if ((success < 0) && useDefault)
		texPngLoad(texture, NULL, texId, psm);  // we don't mind the result of "default"

	return success;
}

static void thmSetColors(theme_t* theme) {
	memcpy(theme->bgColor, gDefaultBgColor, 3);
	theme->textColor = GS_SETREG_RGBA(gDefaultTextColor[0], gDefaultTextColor[1], gDefaultTextColor[2], 0xff);
	theme->uiTextColor = GS_SETREG_RGBA(gDefaultUITextColor[0], gDefaultUITextColor[1], gDefaultUITextColor[2], 0xff);
	theme->selTextColor = GS_SETREG_RGBA(gDefaultSelTextColor[0], gDefaultSelTextColor[1], gDefaultSelTextColor[2], 0xff);

	theme_element_t* elem = theme->mainElems.first;
	while (elem) {
		elem->color = theme->textColor;
		elem = elem->next;
	}
}

static void thmLoadFonts(config_set_t* themeConfig, const char* themePath, theme_t* theme) {
    int fntID; // theme side font id, not the fntSys handle
	for (fntID = 0; fntID < THM_MAX_FONTS; ++fntID) {
		// does the font by the key exist?
		char fntKey[16];

		if (fntID == 0) {
			snprintf(fntKey, 16, "default_font");
			theme->fonts[0] = FNT_DEFAULT;
		} else {
			snprintf(fntKey, 16, "font%d", fntID);
			theme->fonts[fntID] = theme->fonts[0];
		}

		char fullPath[128];
		char *fntFile;
		if (configGetStr(themeConfig, fntKey, &fntFile)) {
			snprintf(fullPath, 128, "%s%s", themePath, fntFile);
			int fntHandle = fntLoadFile(fullPath);

			// Do we have a valid font? Assign the font handle to the theme font slot
			if (fntHandle != FNT_ERROR)
				theme->fonts[fntID] = fntHandle;
		}
	}
}

static void thmLoad(char* themePath) {
	LOG("THEMES Load theme path=%s\n", themePath);
	theme_t* curT = gTheme;
	theme_t* newT = (theme_t*) malloc(sizeof(theme_t));
	memset(newT, 0, sizeof(theme_t));

	newT->useDefault = 1;
	newT->usedHeight = 480;
	thmSetColors(newT);
	newT->mainElems.first = NULL;
	newT->mainElems.last = NULL;
	newT->infoElems.first = NULL;
	newT->infoElems.last = NULL;
	newT->gameCacheCount = 0;
	newT->itemsList = NULL;
	newT->loadingIcon = NULL;
	newT->loadingIconCount = LOAD7_ICON - LOAD0_ICON + 1;

	config_set_t* themeConfig = NULL;
	if (!themePath) {
		themeConfig = configAlloc(0, NULL, NULL);
		addGUIElem(themePath, themeConfig, newT, &newT->mainElems, elementsType[TYPE_MENU_ICON], "_");
		addGUIElem(themePath, themeConfig, newT, &newT->mainElems, elementsType[TYPE_MENU_TEXT], "_");
		addGUIElem(themePath, themeConfig, newT, &newT->mainElems, elementsType[TYPE_ITEMS_LIST], "_");
		addGUIElem(themePath, themeConfig, newT, &newT->mainElems, elementsType[TYPE_ITEM_ICON], "_");
		addGUIElem(themePath, themeConfig, newT, &newT->mainElems, elementsType[TYPE_ITEM_COVER], "_");
		addGUIElem(themePath, themeConfig, newT, &newT->mainElems, elementsType[TYPE_ITEM_TEXT], "_");
		addGUIElem(themePath, themeConfig, newT, &newT->mainElems, elementsType[TYPE_HINT_TEXT], "_");
		addGUIElem(themePath, themeConfig, newT, &newT->mainElems, elementsType[TYPE_LOADING_ICON], "_");
	} else {
		char path[255];
		snprintf(path, 255, "%sconf_theme.cfg", themePath);
		themeConfig = configAlloc(0, NULL, path);
		configRead(themeConfig); // try to load the theme config file

		int intValue;
		if (configGetInt(themeConfig, "use_default", &intValue))
			newT->useDefault = intValue;

		if (configGetInt(themeConfig, "use_real_height", &intValue)) {
			if (intValue)
				newT->usedHeight = screenHeight;
		}

		configGetColor(themeConfig, "bg_color", newT->bgColor);

		unsigned char color[3];
		if (configGetColor(themeConfig, "text_color", color))
			newT->textColor = GS_SETREG_RGBA(color[0], color[1], color[2], 0xff);

		if (configGetColor(themeConfig, "ui_text_color", color))
			newT->uiTextColor = GS_SETREG_RGBA(color[0], color[1], color[2], 0xff);

		if (configGetColor(themeConfig, "sel_text_color", color))
			newT->selTextColor = GS_SETREG_RGBA(color[0], color[1], color[2], 0xff);

		// before loading the element definitions, we have to have the fonts prepared
		// for that, we load the fonts and a translation table
		thmLoadFonts(themeConfig, themePath, newT);

		int i = 1;
		snprintf(path, 255, "main0");
		while (addGUIElem(themePath, themeConfig, newT, &newT->mainElems, NULL, path))
			snprintf(path, 255, "main%d", i++);

		i = 1;
		snprintf(path, 255, "info0");
		while(addGUIElem(themePath, themeConfig, newT, &newT->infoElems, NULL, path))
			snprintf(path, 255, "info%d", i++);
	}

	validateGUIElems(themePath, themeConfig, newT);
	configFree(themeConfig);

	LOG("THEMES Number of cache: %d\n", newT->gameCacheCount);
	LOG("THEMES Used height: %d\n", newT->usedHeight);

	/// Now swap theme and start loading textures
	if (newT->usedHeight == screenHeight)
		rmResetShiftRatio();
	else
		rmSetShiftRatio((float) screenHeight / newT->usedHeight);
	
	int i;
	// default all to not loaded...
	for (i = 0; i < TEXTURES_COUNT; i++) {
		newT->textures[i].Mem = NULL;
	}

	// LOGO, loaded here to avoid flickering during startup with device in AUTO + theme set
	texPngLoad(&newT->textures[LOGO_PICTURE], NULL, LOGO_PICTURE, GS_PSM_CT24);

	gTheme = newT;
	thmFree(curT);

	// First start with busy icon
	char* path = themePath;
	int customBusy = 0;
	for (i = LOAD0_ICON; i <= LOAD7_ICON; i++) {
		if (thmLoadResource(i, path, GS_PSM_CT32, gTheme->useDefault) >= 0)
			customBusy = 1;
		else {
			if (customBusy)
				break;
			else
				path = NULL;
		}
	}
	gTheme->loadingIconCount = i;

	// Customizable icons
	for (i = USB_ICON; i <= START_ICON; i++)
		thmLoadResource(i, themePath, GS_PSM_CT32, gTheme->useDefault);

	// Not  customizable icons
	for (i = L1_ICON; i <= R2_ICON; i++)
		thmLoadResource(i, NULL, GS_PSM_CT32, 1);
}

static void thmRebuildGuiNames() {
	if (guiThemesNames)
		free(guiThemesNames);

	// build the languages name list
	guiThemesNames = (char**) malloc((nThemes + 2) * sizeof(char**));

	// add default internal
	guiThemesNames[0] = "<OPL>";

	int i = 0;
	for (; i < nThemes; i++) {
		guiThemesNames[i + 1] =  themes[i].name;
	}

	guiThemesNames[nThemes + 1] = NULL;
}

void thmAddElements(char* path, char* separator, int mode) {
	nThemes += listDir(path, separator, THM_MAX_FILES - nThemes, &thmReadEntry);
	thmRebuildGuiNames();

	char* temp;
	if (configGetStr(configGetByType(CONFIG_OPL), "theme", &temp)) {
		LOG("THEMES Trying to set again theme: %s\n", temp);
		if (thmSetGuiValue(thmFindGuiID(temp), 0))
			moduleUpdateMenu(mode, 1);
	}
}

void thmInit() {
	LOG("THEMES Init\n");
	gTheme = NULL;

	thmReloadScreenExtents();

	// initialize default internal
	thmLoad(NULL);

	thmAddElements(gBaseMCDir, "/", -1);
}

void thmReinit(char* path) {
	thmLoad(NULL);
	guiThemeID = 0;

	int i = 0;
	while (i < nThemes) {
		if (strncmp(themes[i].filePath, path, strlen(path)) == 0) {
			LOG("THEMES Remove theme: %s\n", themes[i].filePath);
			nThemes--;
			free(themes[i].name);
			themes[i].name = themes[nThemes].name;
			themes[nThemes].name = NULL;
			free(themes[i].filePath);
			themes[i].filePath = themes[nThemes].filePath;
			themes[nThemes].filePath = NULL;
		} else
			i++;
	}

	thmRebuildGuiNames();
}

void thmReloadScreenExtents() {
	rmGetScreenExtents(&screenWidth, &screenHeight);
}

char* thmGetValue() {
	return guiThemesNames[guiThemeID];
}

int thmSetGuiValue(int themeID, int reload) {
	if (themeID != -1) {
		if (guiThemeID != themeID || reload) {
			if (themeID != 0)
				thmLoad(themes[themeID - 1].filePath);
			else
				thmLoad(NULL);

			guiThemeID = themeID;
			configSetStr(configGetByType(CONFIG_OPL), "theme", thmGetValue());
			return 1;
		}
		else if (guiThemeID == 0)
			thmSetColors(gTheme);
	}
	return 0;
}

int thmGetGuiValue() {
	return guiThemeID;
}

int thmFindGuiID(char* theme) {
	if (theme) {
		int i = 0;
		for (; i < nThemes; i++) {
			if (stricmp(themes[i].name, theme) == 0)
				return i + 1;
		}
	}
	return 0;
}

char **thmGetGuiList() {
	return guiThemesNames;
}

void thmEnd() {
	thmFree(gTheme);

	int i = 0;
	for (; i < nThemes; i++) {
		free(themes[i].name);
		free(themes[i].filePath);
	}

	free(guiThemesNames);
}
