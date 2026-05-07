#ifndef QUICK_CLEANUP_GAME_FUNCTIONS_H
#define QUICK_CLEANUP_GAME_FUNCTIONS_H

#include <stdint.h>
#include "game_types.h"

typedef void (__fastcall *fn_house_status_ui_setup)(void* houseStatusUI, uint8_t arg1, void* arg2, uint8_t arg3);
typedef void (__fastcall *fn_house_button_activate)(void* houseButton, uint8_t fromMouse);
typedef uint8_t (__fastcall *fn_house_button_can_activate)(void* houseButton, int32_t buttonIndex, uint8_t strictMouse);
typedef void* (__fastcall *fn_find_child_by_name)(void* rootNode, NarrowString* name);
typedef void* (__fastcall *fn_context_from_manager)(void* manager);
typedef void* (__fastcall *fn_alloc_component_from_type)(void* typeDescriptor);
typedef void* (__fastcall *fn_house_button_construct)(void* houseButton);
typedef void (__fastcall *fn_scene_register_component)(void* manager, void* component);
typedef void* (__fastcall *fn_resize_ptr_array)(void* oldData, uint64_t newSizeBytes);
typedef void (__fastcall *fn_context_attach_component_ref)(void* contextComponentRefs, void** componentRef);
typedef void (__fastcall *fn_house_button_setup_from_node)(void* houseButton, void* node, void* callbackBinding, NarrowString* labelKey);
typedef void* (__fastcall *fn_assign_game_string_literal)(void* targetString, const char* text, uint64_t length);
typedef NarrowString* (__fastcall *fn_init_narrow_string_from_literal)(NarrowString* outputString, const char* text);
typedef void (__fastcall *fn_destroy_narrow_string)(NarrowString* value);
typedef WideString* (__fastcall *fn_localize_narrow_key)(void* localizationManager, WideString* outputString, NarrowString* keyString);
typedef void* (__fastcall *fn_assign_wide_game_string)(void* targetString, const WideString* sourceString);
typedef void (__fastcall *fn_destroy_wide_game_string)(WideString* value);
typedef void (__fastcall *fn_pop_poo_pickup)(void* pickupComponent);
typedef uint8_t (__fastcall *fn_house_cat_is_alive)(void* houseCat);
typedef void (__fastcall *fn_house_cat_teardown)(void* houseCat);

#endif