#ifndef QUICK_CLEANUP_BUTTON_H
#define QUICK_CLEANUP_BUTTON_H

#include <stdint.h>

int IsQuickCleanupButton(void* button);
int IsDeadCatOnlyCleanupModeActive(void);
void UpdateQuickCleanupButtonLabel(void* button);
void SetupButton(void* houseStatusUI);

#endif