#ifndef QUICK_CLEANUP_H
#define QUICK_CLEANUP_H

#include <windows.h>
#include <stdint.h>
#include "mewjector.h"

#define MOD_NAME "Quick-Cleanup"

#define HOUSE_SCENE_NAME "House"
#define POO_ITEM_KEY "poop"

#define BUTTON_NODE_NAME "cleanup_button"
#define BUTTON_ROLE_NAME "Quick_Cleanup"
#define BUTTON_LABEL_LOCALIZATION_KEY "HOUSE_BUTTON_CLEAN_UP"
#define BUTTON_LABEL_FALLBACK_TEXT L"ERROR!"

extern MewjectorAPI g_mj;

#endif