#include <windows.h>
#include <stdint.h>
#include "quick_cleanup.h"
#include "offsets.h"
#include "game_functions.h"
#include "button.h"
#include "scene.h"

MewjectorAPI g_mj;

static fn_house_status_ui_setup g_origHouseStatusUISetup = NULL;
static fn_house_button_activate g_origHouseButtonActivate = NULL;
static fn_house_button_can_activate g_origHouseButtonCanActivate = NULL;

/* 
* HouseButton activation gate can return false if we have a blocker, 
* just bypass that gate so we don't allow the cat menu to fuck us over...
*/
static uint8_t __fastcall HookHouseButtonCanActivate(void* houseButton, int32_t buttonIndex, uint8_t strictMouse)
{
    if (IsQuickCleanupButton(houseButton))
    {
        return 1U;
    }

    if (g_origHouseButtonCanActivate)
    {
        return g_origHouseButtonCanActivate(houseButton, buttonIndex, strictMouse);
    }

    return 0U;
}

/* Intercept only buttons we created... (All original game HouseButtons are forwarded to the trampoline unchanged)... */
static void __fastcall HookHouseButtonActivate(void* houseButton, uint8_t fromMouse)
{
    if (IsQuickCleanupButton(houseButton))
    {
        uint32_t popCount;
        uint32_t deadCatCount;

        popCount = PopAllPoopInHouse();
        deadCatCount = RemoveAllDeadCatsInHouse();

        if (g_mj.Log)
        {
            g_mj.Log(MOD_NAME, "Button clicked, popped %u poop pickup(s) and removed %u dead cat(s) in the House scene! fromMouse=%u", popCount, deadCatCount, (uint32_t)fromMouse);
        }

        return;
    }

    if (g_origHouseButtonActivate)
    {
        g_origHouseButtonActivate(houseButton, fromMouse);
    }
}

static void __fastcall HookHouseStatusUISetup(void* houseStatusUI, uint8_t arg1, void* arg2, uint8_t arg3)
{
    if (g_origHouseStatusUISetup)
    {
        g_origHouseStatusUISetup(houseStatusUI, arg1, arg2, arg3);
    }

    SetupButton(houseStatusUI);
}

static void Initialize(void)
{
    void* setupTrampoline;
    void* activateTrampoline;
    void* canActivateTrampoline;

    if (!MJ_Resolve(&g_mj))
    {
        return;
    }

    setupTrampoline = NULL;
    activateTrampoline = NULL;
    canActivateTrampoline = NULL;

    if (g_mj.InstallHook(RVA_HOUSE_BUTTON_CAN_ACTIVATE, HOUSE_BUTTON_CAN_ACTIVATE_STOLEN_BYTES, (void*)HookHouseButtonCanActivate, &canActivateTrampoline, 24, MOD_NAME))
    {
        g_origHouseButtonCanActivate = (fn_house_button_can_activate)canActivateTrampoline;

        if (g_mj.Log)
        {
            g_mj.Log(MOD_NAME, "Loaded HouseButton interactability hook!");
        }
    }
    else if (g_mj.Log)
    {
        g_mj.Log(MOD_NAME, "Failed to hook HouseButton interactability!");
    }

    if (g_mj.InstallHook(RVA_HOUSE_BUTTON_ACTIVATE, HOUSE_BUTTON_ACTIVATE_STOLEN_BYTES, (void*)HookHouseButtonActivate, &activateTrampoline, 24, MOD_NAME))
    {
        g_origHouseButtonActivate = (fn_house_button_activate)activateTrampoline;

        if (g_mj.Log)
        {
            g_mj.Log(MOD_NAME, "Loaded HouseButton activation hook!");
        }
    }
    else if (g_mj.Log)
    {
        g_mj.Log(MOD_NAME, "Failed to hook HouseButton activation!");
    }

    if (g_mj.InstallHook(RVA_HOUSE_STATUS_UI_SETUP, HOUSE_STATUS_UI_SETUP_STOLEN_BYTES, (void*)HookHouseStatusUISetup, &setupTrampoline, 24, MOD_NAME))
    {
        g_origHouseStatusUISetup = (fn_house_status_ui_setup)setupTrampoline;

        if (g_mj.Log)
        {
            g_mj.Log(MOD_NAME, "Loaded. Hooked HouseStatusUI setup and will initialize '%s'!", BUTTON_NODE_NAME);
        }
    }
    else if (g_mj.Log)
    {
        g_mj.Log(MOD_NAME, "Failed to hook HouseStatusUI setup!");
    }
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID reserved)
{
    (void)reserved;

    if (reason == DLL_PROCESS_ATTACH)
    {
        DisableThreadLibraryCalls(hModule);
        Initialize();
    }

    return TRUE;
}