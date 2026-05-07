#ifndef QUICK_CLEANUP_GAME_STRINGS_H
#define QUICK_CLEANUP_GAME_STRINGS_H

#include "game_types.h"
#include "game_functions.h"

void InitSmallNarrowString(NarrowString* output, const char* text);
void InitSmallWideString(WideString* output, const wchar_t* text);
const char* GetNarrowStringChars(const NarrowString* value);
int NarrowStringEqualsLiteral(const NarrowString* value, const char* text);
void SetButtonLabelInline(void* button, fn_assign_wide_game_string assignWideNarrowString, const wchar_t* fallbackText);
int SetButtonLabelFromLocalizationKey(void* button, const char* localizationKey);

#endif