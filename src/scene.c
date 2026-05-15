#include <windows.h>
#include <stdint.h>
#include <string.h>
#include "quick_cleanup.h"
#include "offsets.h"
#include "game_types.h"
#include "game_functions.h"
#include "game_strings.h"
#include "scene.h"

/* Gets the global MewDirector, most of the useful stuff is reachable from here... */
void* GetMewDirectorSingleton(void)
{
    UINT_PTR gameBase;
    void* mewDirector;

    if (!g_mj.GetGameBase)
    {
        return NULL;
    }

    gameBase = g_mj.GetGameBase();

    if (!gameBase)
    {
        return NULL;
    }

    mewDirector = NULL;

    __try
    {
        mewDirector = *(void**)(gameBase + (UINT_PTR)DATAOFF_MEWDIRECTOR_SINGLETON);
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        mewDirector = NULL;
    }

    return mewDirector;
}

void* FindSceneByName(const char* sceneName)
{
    void* mewDirector;
    void* director;
    VectorPtr* scenes;
    void** cursor;

    if (!sceneName)
    {
        return NULL;
    }

    mewDirector = GetMewDirectorSingleton();

    if (!mewDirector)
    {
        return NULL;
    }

    director = NULL;

    __try
    {
        director = *(void**)((uint8_t*)mewDirector + OFF_MEWDIRECTOR_DIRECTOR);
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        director = NULL;
    }

    if (!director)
    {
        return NULL;
    }

    scenes = (VectorPtr*)director;

    __try
    {
        if (!scenes->begin || !scenes->end || scenes->end < scenes->begin)
        {
            return NULL;
        }

        for (cursor = scenes->begin; cursor < scenes->end; ++cursor)
        {
            void* scene;
            NarrowString* name;

            scene = *cursor;

            if (!scene)
            {
                continue;
            }

            name = (NarrowString*)((uint8_t*)scene + OFF_SCENE_NAME);

            if (NarrowStringEqualsLiteral(name, sceneName))
            {
                return scene;
            }
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return NULL;
    }

    return NULL;
}

static int IsPoopPickupComponent(void* component)
{
    void* itemRecord;
    NarrowString* itemKey;
    int result;

    if (!component)
    {
        return 0;
    }

    result = 0;

    __try
    {
        itemRecord = *(void**)((uint8_t*)component + OFF_PICKUP_ITEM_RECORD);

        if (itemRecord)
        {
            itemKey = (NarrowString*)((uint8_t*)itemRecord + OFF_PICKUP_ITEM_RECORD_KEY_STRING);
            result = NarrowStringEqualsLiteral(itemKey, POO_ITEM_KEY);
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        result = 0;
    }

    return result;
}

/* Searches the scene component list and invokes the poop-pickup pop routine for every matching pickup... */
uint32_t PopAllPoopInHouse(void)
{
    UINT_PTR gameBase;
    void* houseScene;
    PodVectorPtr* componentList;
    fn_pop_poo_pickup popPooPickup;
    uint32_t index;
    uint32_t popCount;

    if (!g_mj.GetGameBase)
    {
        return 0U;
    }

    gameBase = g_mj.GetGameBase();

    if (!gameBase)
    {
        return 0U;
    }

    houseScene = FindSceneByName(HOUSE_SCENE_NAME);

    if (!houseScene)
    {
        if (g_mj.Log)
        {
            g_mj.Log(MOD_NAME, "Could not find scene '%s', no poop popped!", HOUSE_SCENE_NAME);
        }

        return 0U;
    }

    componentList = NULL;

    __try
    {
        if (*(uint8_t*)((uint8_t*)houseScene + OFF_SCENE_DOING_DESTRUCTION))
        {
            return 0U;
        }

        componentList = *(PodVectorPtr**)((uint8_t*)houseScene + OFF_SCENE_COMPONENT_LISTS);
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        componentList = NULL;
    }

    if (!componentList)
    {
        return 0U;
    }

    popPooPickup = (fn_pop_poo_pickup)(gameBase + (UINT_PTR)RVA_POP_POO_PICKUP);
    popCount = 0U;

    __try
    {
        for (index = 0U; index < componentList->size; ++index)
        {
            void* component;

            component = componentList->data[index];

            if (!component)
            {
                continue;
            }

            if (IsPoopPickupComponent(component))
            {
                popPooPickup(component);
                ++popCount;
            }
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        if (g_mj.Log)
        {
            g_mj.Log(MOD_NAME, "Exception while scanning House scene components for poop pickups!");
        }
    }

    return popCount;
}

/* Extremely scientific cat test!!!! Poke the first pointer and see if it meows... */
static int IsHouseCatComponent(void* component)
{
    UINT_PTR gameBase;
    void* expectedVTable;
    int result;

    if (!component || !g_mj.GetGameBase)
    {
        return 0;
    }

    gameBase = g_mj.GetGameBase();

    if (!gameBase)
    {
        return 0;
    }

    expectedVTable = (void*)(gameBase + (UINT_PTR)DATAOFF_HOUSE_CAT_VTABLE);
    result = 0;

    __try
    {
        result = (*(void**)component == expectedVTable);
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        result = 0;
    }

    return result;
}

static int IsDeadHouseCatComponent(void* component)
{
    UINT_PTR gameBase;
    fn_house_cat_is_alive houseCatIsAlive;
    void* deadBodyComponent;
    int result;

    if (!IsHouseCatComponent(component) || !g_mj.GetGameBase)
    {
        return 0;
    }

    gameBase = g_mj.GetGameBase();

    if (!gameBase)
    {
        return 0;
    }

    houseCatIsAlive = (fn_house_cat_is_alive)(gameBase + (UINT_PTR)RVA_HOUSE_CAT_IS_ALIVE);
    deadBodyComponent = NULL;
    result = 0;

    __try
    {
        /*
        * A removable corpse must first have the dead-body component that the House scene creates for dead cats...
        */
        deadBodyComponent = *(void**)((uint8_t*)component + OFF_HOUSE_CAT_DEAD_BODY_COMPONENT);

        if (!deadBodyComponent)
        {
            return 0;
        }

        result = houseCatIsAlive(component) ? 0 : 1;
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        result = 0;
    }

    return result;
}

static uint32_t TeardownDeadHouseCat(void* houseCat, fn_house_cat_teardown houseCatTeardown)
{
    uint8_t activeFlag;
    void* backingSceneObject;

    if (!houseCat || !houseCatTeardown)
    {
        return 0U;
    }

    activeFlag = 0U;
    backingSceneObject = NULL;

    __try
    {
        activeFlag = *(uint8_t*)((uint8_t*)houseCat + OFF_HOUSE_CAT_ACTIVE_FLAG); // HouseCat active/enabled byte observed before safe teardown...
        backingSceneObject = *(void**)((uint8_t*)houseCat + OFF_HOUSE_CAT_DEAD_BODY_COMPONENT);
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return 0U;
    }

    if (!activeFlag || !backingSceneObject)
    {
        return 0U;
    }

    if (!IsDeadHouseCatComponent(houseCat))
    {
        if (g_mj.Log)
        {
            g_mj.Log(MOD_NAME, "Dead cat cleanup: Skipped HouseCat %p because it no longer looks like a corpse!", houseCat);
        }

        return 0U;
    }

    __try
    {
        houseCatTeardown(houseCat);
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        if (g_mj.Log)
        {
            g_mj.Log(MOD_NAME, "Exception while calling HouseCat teardown for dead cat component %p!", houseCat);
        }

        return 0U;
    }

    if (g_mj.Log)
    {
        g_mj.Log(MOD_NAME, "Dead cat cleanup: Called HouseCat teardown for %p, backing scene object was %p!", houseCat, backingSceneObject);
    }

    return 1U;
}

static uint32_t RecordOrganGrinderCatDonationReward(void)
{
    UINT_PTR gameBase;
    void* mewDirector;
    void* globalProgressionData;
    fn_global_progression_add_npc_donation addNpcDonation;

    if (!g_mj.GetGameBase)
    {
        return 0U;
    }

    gameBase = g_mj.GetGameBase();

    if (!gameBase)
    {
        return 0U;
    }

    mewDirector = GetMewDirectorSingleton();

    if (!mewDirector)
    {
        return 0U;
    }

    globalProgressionData = NULL;

    __try
    {
        globalProgressionData = *(void**)((uint8_t*)mewDirector + OFF_MEWDIRECTOR_GLOBAL_PROGRESSION);
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        globalProgressionData = NULL;
    }

    if (!globalProgressionData)
    {
        return 0U;
    }

    addNpcDonation = (fn_global_progression_add_npc_donation)(gameBase + (UINT_PTR)RVA_GLOBAL_PROGRESSION_ADD_NPC_DONATION);

    __try
    {
        addNpcDonation(globalProgressionData, NPC_ORGAN_GRINDER, NULL, 1);
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        if (g_mj.Log)
        {
            g_mj.Log(MOD_NAME, "Exception while counting a dead cat as an Organ Grinder donation reward!");
        }

        return 0U;
    }

    return 1U;
}


/* Two-pass: Collect candidates first, then call HouseCat teardown after scan to avoid nasty mid-iteration list mutation... */
uint32_t DonateAllDeadCatsInHouseToOrganGrinder(void)
{
    UINT_PTR gameBase;
    void* houseScene;
    PodVectorPtr* componentList;
    fn_house_cat_teardown houseCatTeardown;
    void* deadCats[512];
    uint32_t index;
    uint32_t houseCatCount;
    uint32_t aliveCatCount;
    uint32_t deadCatCount;
    uint32_t removeCount;
    uint32_t donationCount;

    if (!g_mj.GetGameBase)
    {
        return 0U;
    }

    gameBase = g_mj.GetGameBase();

    if (!gameBase)
    {
        return 0U;
    }

    houseScene = FindSceneByName(HOUSE_SCENE_NAME);

    if (!houseScene)
    {
        if (g_mj.Log)
        {
            g_mj.Log(MOD_NAME, "Could not find scene '%s', no dead cats donated!", HOUSE_SCENE_NAME);
        }

        return 0U;
    }

    componentList = NULL;

    __try
    {
        if (*(uint8_t*)((uint8_t*)houseScene + OFF_SCENE_DOING_DESTRUCTION))
        {
            return 0U;
        }

        componentList = *(PodVectorPtr**)((uint8_t*)houseScene + OFF_SCENE_COMPONENT_LISTS);
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        componentList = NULL;
    }

    if (!componentList)
    {
        return 0U;
    }

    houseCatCount = 0U;
    aliveCatCount = 0U;
    deadCatCount = 0U;
    memset(deadCats, 0, sizeof(deadCats));

    __try
    {
        for (index = 0U; index < componentList->size && deadCatCount < 512U; ++index)
        {
            void* component;

            component = componentList->data[index];

            if (!component)
            {
                continue;
            }

            if (IsHouseCatComponent(component))
            {
                ++houseCatCount;

                if (IsDeadHouseCatComponent(component))
                {
                    deadCats[deadCatCount] = component;
                    ++deadCatCount;
                }
                else
                {
                    ++aliveCatCount;
                }
            }
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        if (g_mj.Log)
        {
            g_mj.Log(MOD_NAME, "Exception while scanning House scene components for dead cats!");
        }

        return 0U;
    }

    if (g_mj.Log)
    {
        g_mj.Log(MOD_NAME, "House cat scan: %u HouseCat component(s), %u alive/non-dead, %u dead candidate(s)!", houseCatCount, aliveCatCount, deadCatCount);
    }

    houseCatTeardown = (fn_house_cat_teardown)(gameBase + (UINT_PTR)RVA_HOUSE_CAT_TEARDOWN);
    removeCount = 0U;
    donationCount = 0U;

    for (index = 0U; index < deadCatCount; ++index)
    {
        if (deadCats[index])
        {
            if (TeardownDeadHouseCat(deadCats[index], houseCatTeardown))
            {
                ++removeCount;
                donationCount += RecordOrganGrinderCatDonationReward();
            }
        }
    }

    if (g_mj.Log)
    {
        g_mj.Log(MOD_NAME, "Dead cat cleanup: Safely tore down %u dead HouseCat scene object(s) and counted %u as Organ Grinder cat donation reward(s)!", removeCount, donationCount);
    }

    return donationCount;
}