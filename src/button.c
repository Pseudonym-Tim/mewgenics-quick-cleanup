#include <windows.h>
#include <stdint.h>
#include <string.h>
#include "quick_cleanup.h"
#include "offsets.h"
#include "game_types.h"
#include "game_functions.h"
#include "game_strings.h"
#include "button.h"

static volatile LONG g_setupLock = 0;
static SetupRecord g_setupRecords[64];
static uint32_t g_setupRecordCount = 0U;

static int IsRecordedButtonAliveForNode(const SetupRecord* record, void* manager, void* rootNode, void* buttonNode)
{
    int result;

    if (!record || !record->button || !manager || !rootNode || !buttonNode)
    {
        return 0;
    }

    result = 0;

    __try
    { 
        // (Confirms the cached HouseButton still belongs to this manager/node pair)...
        result = record->manager == manager && record->rootNode == rootNode && record->buttonNode == buttonNode && *(void**)((uint8_t*)record->button + OFF_HOUSE_BUTTON_MANAGER) == manager && *(void**)((uint8_t*)record->button + OFF_HOUSE_BUTTON_NODE) == buttonNode;
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        result = 0;
    }

    return result;
}

static SetupRecord* FindLiveSetupRecordForButton(void* button)
{
    uint32_t index;

    if (!button)
    {
        return NULL;
    }

    for (index = 0U; index < g_setupRecordCount; ++index)
    {
        SetupRecord* record;
        int result;

        record = &g_setupRecords[index];

        if (record->button != button || !record->manager || !record->buttonNode)
        {
            continue;
        }

        result = 0;

        __try
        {
            result = *(uint8_t*)((uint8_t*)record->manager + OFF_SCENE_DOING_DESTRUCTION) == 0U && *(void**)((uint8_t*)button + OFF_HOUSE_BUTTON_MANAGER) == record->manager && *(void**)((uint8_t*)button + OFF_HOUSE_BUTTON_NODE) == record->buttonNode;
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            result = 0;
        }

        if (result)
        {
            return record;
        }

        record->houseStatusUI = NULL;
        record->manager = NULL;
        record->rootNode = NULL;
        record->buttonNode = NULL;
        record->button = NULL;
        record->deadCatsOnlyLabelActive = 0U;
    }

    return NULL;
}

int IsQuickCleanupButton(void* button)
{
    return FindLiveSetupRecordForButton(button) ? 1 : 0;
}

int IsDeadCatOnlyCleanupModeActive(void)
{
    return (GetAsyncKeyState(VK_SHIFT) & 0x8000) ? 1 : 0;
}

static void SetQuickCleanupButtonLabelMode(void* button, uint8_t deadCatsOnlyMode)
{
    UINT_PTR gameBase;
    const char* labelKey;
    const wchar_t* fallbackText;
    fn_assign_wide_game_string assignWideGameString;

    if (!button || !g_mj.GetGameBase)
    {
        return;
    }

    gameBase = g_mj.GetGameBase();

    if (!gameBase)
    {
        return;
    }

    labelKey = deadCatsOnlyMode ? BUTTON_LABEL_DEAD_CATS_ONLY_LOCALIZATION_KEY : BUTTON_LABEL_LOCALIZATION_KEY;
    fallbackText = deadCatsOnlyMode ? BUTTON_LABEL_DEAD_CATS_ONLY_FALLBACK_TEXT : BUTTON_LABEL_FALLBACK_TEXT;

    if (!SetButtonLabelFromLocalizationKey(button, labelKey))
    {
        assignWideGameString = (fn_assign_wide_game_string)(gameBase + (UINT_PTR)RVA_ASSIGN_WIDE_GAME_STRING);
        SetButtonLabelInline(button, assignWideGameString, fallbackText);
    }
}

void UpdateQuickCleanupButtonLabel(void* button)
{
    SetupRecord* record;
    uint8_t deadCatsOnlyMode;

    record = FindLiveSetupRecordForButton(button);

    if (!record)
    {
        return;
    }

    deadCatsOnlyMode = IsDeadCatOnlyCleanupModeActive() ? 1U : 0U;

    if (record->deadCatsOnlyLabelActive == deadCatsOnlyMode)
    {
        return;
    }

    SetQuickCleanupButtonLabelMode(button, deadCatsOnlyMode);
    record->deadCatsOnlyLabelActive = deadCatsOnlyMode;
}

static SetupRecord* FindCurrentSetupRecord(void* manager, void* rootNode, void* buttonNode)
{
    uint32_t index;

    for (index = 0U; index < g_setupRecordCount; ++index)
    {
        if (IsRecordedButtonAliveForNode(&g_setupRecords[index], manager, rootNode, buttonNode))
        {
            return &g_setupRecords[index];
        }
    }

    return NULL;
}

static SetupRecord* FindReusableSetupRecord(void* houseStatusUI)
{
    uint32_t index;

    for (index = 0U; index < g_setupRecordCount; ++index)
    {
        if (g_setupRecords[index].houseStatusUI == houseStatusUI)
        {
            return &g_setupRecords[index];
        }
    }

    return NULL;
}

static void RememberSetup(void* houseStatusUI, void* manager, void* rootNode, void* buttonNode, void* button)
{
    SetupRecord* reusableRecord;

    reusableRecord = FindReusableSetupRecord(houseStatusUI);

    if (!reusableRecord && g_setupRecordCount < 64U)
    {
        reusableRecord = &g_setupRecords[g_setupRecordCount];
        ++g_setupRecordCount;
    }

    if (!reusableRecord)
    {
        return;
    }

    reusableRecord->houseStatusUI = houseStatusUI;
    reusableRecord->manager = manager;
    reusableRecord->rootNode = rootNode;
    reusableRecord->buttonNode = buttonNode;
    reusableRecord->button = button;
    reusableRecord->deadCatsOnlyLabelActive = 0U;
}

static int BeginSetupGuard(void)
{
    return InterlockedCompareExchange(&g_setupLock, 1, 0) == 0;
}

static void EndSetupGuard(void)
{
    InterlockedExchange(&g_setupLock, 0);
}

/* Sneaks a component into the scene manager's bucket of important nonsense, grow if it's packed... */
static int AppendSceneComponentList(void* manager, uint32_t capacityOffset, uint32_t sizeOffset, uint32_t dataOffset, void* component, fn_resize_ptr_array resizePtrArray)
{
    uint32_t capacity;
    uint32_t size;
    uint32_t newCapacity;
    void*** dataPtr;
    void** data;

    if (!manager || !component || !resizePtrArray)
    {
        return 0;
    }

    __try
    {
        capacity = *(uint32_t*)((uint8_t*)manager + capacityOffset);
        size = *(uint32_t*)((uint8_t*)manager + sizeOffset);
        dataPtr = (void***)((uint8_t*)manager + dataOffset);
        data = *dataPtr;

        if (size == capacity)
        {
            double grownCapacity;

            grownCapacity = (double)capacity * 1.5;
            newCapacity = (uint32_t)grownCapacity;

            if (newCapacity < 2U)
            {
                newCapacity = 2U;
            }

            data = (void**)resizePtrArray(data, (uint64_t)newCapacity * sizeof(void*));
            *dataPtr = data;
            *(uint32_t*)((uint8_t*)manager + capacityOffset) = newCapacity;

            if (newCapacity < size)
            {
                *(uint32_t*)((uint8_t*)manager + sizeOffset) = newCapacity;
                size = newCapacity;
            }
        }

        data[size] = component;
        *(uint32_t*)((uint8_t*)manager + sizeOffset) = size + 1U;
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return 0;
    }

    return 1;
}

/* Allocates a super awesome HouseButton component... register it with the current scene, and bind it to an existing UI node... */
static void* CreateGenericHouseButtonFromNode(void* manager, void* context, void* node, const char* labelLocalizationKey)
{
    UINT_PTR gameBase;
    void* typeDescriptor;
    void* houseButton;
    NarrowString labelKey;
    fn_alloc_component_from_type allocComponentFromType;
    fn_house_button_construct houseButtonConstruct;
    fn_scene_register_component sceneRegisterComponent;
    fn_resize_ptr_array resizePtrArray;
    fn_context_attach_component_ref contextAttachComponentRef;
    fn_house_button_setup_from_node houseButtonSetupFromNode;

    if (!manager || !context || !node || !labelLocalizationKey || !g_mj.GetGameBase)
    {
        return NULL;
    }

    gameBase = g_mj.GetGameBase();

    if (!gameBase)
    {
        return NULL;
    }

    __try
    {
        if (*(uint8_t*)((uint8_t*)manager + OFF_SCENE_DOING_DESTRUCTION))
        {
            return NULL;
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return NULL;
    }

    allocComponentFromType = (fn_alloc_component_from_type)(gameBase + (UINT_PTR)RVA_ALLOC_COMPONENT_FROM_TYPE);
    houseButtonConstruct = (fn_house_button_construct)(gameBase + (UINT_PTR)RVA_HOUSE_BUTTON_CONSTRUCT);
    sceneRegisterComponent = (fn_scene_register_component)(gameBase + (UINT_PTR)RVA_SCENE_REGISTER_COMPONENT);
    resizePtrArray = (fn_resize_ptr_array)(gameBase + (UINT_PTR)RVA_RESIZE_PTR_ARRAY);
    contextAttachComponentRef = (fn_context_attach_component_ref)(gameBase + (UINT_PTR)RVA_CONTEXT_ATTACH_COMPONENT_REF);
    houseButtonSetupFromNode = (fn_house_button_setup_from_node)(gameBase + (UINT_PTR)RVA_HOUSE_BUTTON_SETUP_FROM_NODE);
    typeDescriptor = (void*)(gameBase + (UINT_PTR)DATAOFF_HOUSE_BUTTON_TYPE_DESCRIPTOR);

    houseButton = allocComponentFromType(typeDescriptor);

    if (!houseButton)
    {
        return NULL;
    }

    memset(houseButton, 0, SIZE_HOUSE_BUTTON); // HouseButton instance size (Observed from construction/allocation path)...
    houseButton = houseButtonConstruct(houseButton);

    if (!houseButton)
    {
        return NULL;
    }

    __try
    {
        *(void**)((uint8_t*)houseButton + OFF_COMPONENT_CONTEXT) = context; // (Base component scene/context reference)...
        *(void**)((uint8_t*)houseButton + OFF_HOUSE_BUTTON_MANAGER) = manager; // (Owning scene manager)...
        *(void**)((uint8_t*)houseButton + OFF_COMPONENT_MANAGER_HEADER) = *(void**)manager;
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return NULL;
    }

    sceneRegisterComponent(manager, houseButton);

    __try
    {
        *(uint8_t*)((uint8_t*)houseButton + OFF_COMPONENT_FLAGS_A) = (uint8_t)((*(uint8_t*)((uint8_t*)houseButton + OFF_COMPONENT_FLAGS_A) & COMPONENT_FLAGS_A_KEEP_SETUP_MASK) | COMPONENT_FLAGS_A_SET_SETUP_MASK);
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return NULL;
    }

    if (!AppendSceneComponentList(manager, OFF_MANAGER_BUCKET_UPDATE_CAPACITY, OFF_MANAGER_BUCKET_UPDATE_SIZE, OFF_MANAGER_BUCKET_UPDATE_DATA, houseButton, resizePtrArray))
    {
        return NULL;
    }

    __try
    {
        *(uint8_t*)((uint8_t*)houseButton + OFF_COMPONENT_FLAGS_A) |= COMPONENT_FLAGS_A_CLICKABLE_MASK;
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return NULL;
    }

    if (!AppendSceneComponentList(manager, OFF_MANAGER_BUCKET_CLICK_CAPACITY, OFF_MANAGER_BUCKET_CLICK_SIZE, OFF_MANAGER_BUCKET_CLICK_DATA, houseButton, resizePtrArray))
    {
        return NULL;
    }

    __try
    {
        *(uint8_t*)((uint8_t*)houseButton + OFF_COMPONENT_FLAGS_A) &= COMPONENT_FLAGS_A_POST_CLICK_MASK;
        *(uint8_t*)((uint8_t*)houseButton + OFF_COMPONENT_FLAGS_B) = (uint8_t)((*(uint8_t*)((uint8_t*)houseButton + OFF_COMPONENT_FLAGS_B) & COMPONENT_FLAGS_B_KEEP_RENDER_MASK) | COMPONENT_FLAGS_B_SET_RENDER_MASK);
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return NULL;
    }

    if (!AppendSceneComponentList(manager, OFF_MANAGER_BUCKET_RENDER_CAPACITY, OFF_MANAGER_BUCKET_RENDER_SIZE, OFF_MANAGER_BUCKET_RENDER_DATA, houseButton, resizePtrArray))
    {
        return NULL;
    }

    __try
    {
        *(uint8_t*)((uint8_t*)houseButton + OFF_COMPONENT_FLAGS_B) &= COMPONENT_FLAGS_B_POST_RENDER_MASK;
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return NULL;
    }

    contextAttachComponentRef((uint8_t*)context + OFF_CONTEXT_COMPONENT_REFS, &houseButton); // (Context component-reference array starts here)...

    __try
    {
        *(uint8_t*)((uint8_t*)houseButton + OFF_COMPONENT_LAYER) = *(uint8_t*)((uint8_t*)context + OFF_CONTEXT_LAYER); // Copy the context layer/group byte used by registered components...
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return NULL;
    }

    memset(&labelKey, 0, sizeof(labelKey));

    // Temporary callback binding wantonly expected by HouseButton setup, we do this just so we don't have any problems and it DOESN'T FUCKING BREAK...
    uint8_t callbackScratch[SIZE_CALLBACK_BINDING_SCRATCH];

    memset(callbackScratch, 0, sizeof(callbackScratch));

    ((fn_init_narrow_string_from_literal)(gameBase + (UINT_PTR)RVA_INIT_NARROW_STRING))(&labelKey, labelLocalizationKey);
    houseButtonSetupFromNode(houseButton, node, callbackScratch, &labelKey);
    ((fn_destroy_narrow_string)(gameBase + (UINT_PTR)RVA_DESTROY_NARROW_STRING))(&labelKey);

    return houseButton;
}

/* Finds our fake-looking button node and gives it a soul, a localized label, and permission to be clicked... */
void SetupButton(void* houseStatusUI)
{
    UINT_PTR gameBase;
    void* rootNode;
    void* sceneRef;
    void* manager;
    void* context;
    void* buttonNode;
    void* button;
    NarrowString buttonName;
    fn_find_child_by_name findChildByName;
    fn_context_from_manager contextFromManager;
    fn_assign_game_string_literal assignNarrowStringLiteral;

    if (!houseStatusUI || !g_mj.GetGameBase)
    {
        return;
    }

    if (!BeginSetupGuard())
    {
        return;
    }

    gameBase = g_mj.GetGameBase();

    if (!gameBase)
    {
        EndSetupGuard();
        return;
    }

    rootNode = *(void**)((uint8_t*)houseStatusUI + OFF_HOUSE_STATUS_UI_ROOT_NODE);
    sceneRef = *(void**)((uint8_t*)houseStatusUI + OFF_HOUSE_STATUS_UI_SCENE_REF);

    if (!rootNode || !sceneRef)
    {
        EndSetupGuard();
        return;
    }

    manager = *(void**)((uint8_t*)sceneRef + OFF_SCENE_REF_MANAGER);

    if (!manager)
    {
        EndSetupGuard();
        return;
    }

    findChildByName = (fn_find_child_by_name)(gameBase + (UINT_PTR)RVA_UI_FIND_CHILD_BY_NAME);
    contextFromManager = (fn_context_from_manager)(gameBase + (UINT_PTR)RVA_CONTEXT_FROM_MANAGER);
    assignNarrowStringLiteral = (fn_assign_game_string_literal)(gameBase + (UINT_PTR)RVA_ASSIGN_GAME_STRING_LITERAL);

    InitSmallNarrowString(&buttonName, BUTTON_NODE_NAME);
    buttonNode = findChildByName(rootNode, &buttonName);

    if (!buttonNode)
    {
        if (g_mj.Log)
        {
            g_mj.Log(MOD_NAME, "HouseStatusUI is live, but '%s' was not found under the HouseStatusUI root node!", BUTTON_NODE_NAME);
        }

        EndSetupGuard();
        return;
    }

    if (FindCurrentSetupRecord(manager, rootNode, buttonNode))
    {
        if (g_mj.Log)
        {
            g_mj.Log(MOD_NAME, "'%s' already has a live HouseButton binding for the current scene instance!", BUTTON_NODE_NAME);
        }

        EndSetupGuard();
        return;
    }

    context = contextFromManager(manager);
    button = CreateGenericHouseButtonFromNode(manager, context, buttonNode, BUTTON_LABEL_LOCALIZATION_KEY);

    if (!button)
    {
        if (g_mj.Log)
        {
            g_mj.Log(MOD_NAME, "Generic HouseButton creation returned NULL for '%s'!", BUTTON_NODE_NAME);
        }

        EndSetupGuard();
        return;
    }

    assignNarrowStringLiteral((uint8_t*)button + OFF_HOUSE_BUTTON_ROLE_NAME, BUTTON_ROLE_NAME, (uint64_t)(sizeof(BUTTON_ROLE_NAME) - 1U));

    if (!SetButtonLabelFromLocalizationKey(button, BUTTON_LABEL_LOCALIZATION_KEY))
    {
        SetButtonLabelInline(button, (fn_assign_wide_game_string)(gameBase + (UINT_PTR)RVA_ASSIGN_WIDE_GAME_STRING), BUTTON_LABEL_FALLBACK_TEXT);
    }

    RememberSetup(houseStatusUI, manager, rootNode, buttonNode, button);

    if (g_mj.Log)
    {
        g_mj.Log(MOD_NAME, "Set up '%s' as a custom HouseButton at %p! labelKey=%s", BUTTON_NODE_NAME, button, BUTTON_LABEL_LOCALIZATION_KEY);
    }

    EndSetupGuard();
}

