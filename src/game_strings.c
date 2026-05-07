#include <windows.h>
#include <string.h>
#include <wchar.h>
#include "quick_cleanup.h"
#include "offsets.h"
#include "game_strings.h"

void InitSmallNarrowString(NarrowString* output, const char* text)
{
    size_t length;

    if (!output)
    {
        return;
    }

    memset(output, 0, sizeof(NarrowString));

    if (!text)
    {
        output->capacity = 15ULL;
        return;
    }

    length = strlen(text);

    if (length > 15U)
    {
        length = 15U;
    }

    memcpy(output->storage.inline_buf, text, length);
    output->storage.inline_buf[length] = '\0';
    output->size = (uint64_t)length;
    output->capacity = 15ULL;
}

void InitSmallWideString(WideString* output, const wchar_t* text)
{
    size_t length;

    if (!output)
    {
        return;
    }

    memset(output, 0, sizeof(WideString));

    if (!text)
    {
        output->capacity = 7ULL;
        return;
    }

    length = wcslen(text);

    if (length > 7U)
    {
        length = 7U;
    }

    memcpy(output->storage.inline_buf, text, length * sizeof(wchar_t));
    output->storage.inline_buf[length] = L'\0';
    output->size = (uint64_t)length;
    output->capacity = 7ULL;
}

const char* GetNarrowStringChars(const NarrowString* value)
{
    if (!value)
    {
        return NULL;
    }

    if (value->capacity < 16ULL)
    {
        return value->storage.inline_buf;
    }

    return value->storage.heap_ptr;
}

int NarrowStringEqualsLiteral(const NarrowString* value, const char* text)
{
    size_t length;
    const char* chars;

    if (!value || !text)
    {
        return 0;
    }

    length = strlen(text);

    if (value->size != (uint64_t)length)
    {
        return 0;
    }

    chars = GetNarrowStringChars(value);

    if (!chars)
    {
        return 0;
    }

    return memcmp(chars, text, length) == 0;
}

void SetButtonLabelInline(void* button, fn_assign_wide_game_string assignWideNarrowString, const wchar_t* fallbackText)
{
    WideString labelText;

    if (!button || !assignWideNarrowString)
    {
        return;
    }

    InitSmallWideString(&labelText, fallbackText);
    assignWideNarrowString((uint8_t*)button + OFF_HOUSE_BUTTON_LABEL_TEXT, &labelText);
}

int SetButtonLabelFromLocalizationKey(void* button, const char* localizationKey)
{
    UINT_PTR gameBase;
    void* localizationManager;
    NarrowString keyString;
    WideString localizedText;
    fn_init_narrow_string_from_literal initNarrowString;
    fn_destroy_narrow_string destroyNarrowString;
    fn_localize_narrow_key localizeNarrowKey;
    fn_assign_wide_game_string assignWideNarrowString;
    fn_destroy_wide_game_string destroyWideNarrowString;

    if (!button || !localizationKey || !localizationKey[0] || !g_mj.GetGameBase)
    {
        return 0;
    }

    gameBase = g_mj.GetGameBase();

    if (!gameBase)
    {
        return 0;
    }

    localizationManager = (void*)(gameBase + (UINT_PTR)DATAOFF_LOCALIZATION_MANAGER);
    initNarrowString = (fn_init_narrow_string_from_literal)(gameBase + (UINT_PTR)RVA_INIT_NARROW_STRING);
    destroyNarrowString = (fn_destroy_narrow_string)(gameBase + (UINT_PTR)RVA_DESTROY_NARROW_STRING);
    localizeNarrowKey = (fn_localize_narrow_key)(gameBase + (UINT_PTR)RVA_LOCALIZE_NARROW_KEY);
    assignWideNarrowString = (fn_assign_wide_game_string)(gameBase + (UINT_PTR)RVA_ASSIGN_WIDE_GAME_STRING);
    destroyWideNarrowString = (fn_destroy_wide_game_string)(gameBase + (UINT_PTR)RVA_DESTROY_WIDE_GAME_STRING);

    memset(&keyString, 0, sizeof(keyString));
    memset(&localizedText, 0, sizeof(localizedText));

    initNarrowString(&keyString, localizationKey);
    localizeNarrowKey(localizationManager, &localizedText, &keyString);

    if (localizedText.size == 0ULL)
    {
        destroyNarrowString(&keyString);
        return 0;
    }

    assignWideNarrowString((uint8_t*)button + OFF_HOUSE_BUTTON_LABEL_TEXT, &localizedText);
    destroyWideNarrowString(&localizedText);
    destroyNarrowString(&keyString);
    return 1;
}