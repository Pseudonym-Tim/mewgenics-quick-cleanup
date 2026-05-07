#ifndef QUICK_CLEANUP_GAME_TYPES_H
#define QUICK_CLEANUP_GAME_TYPES_H

#include <stdint.h>
#include <wchar.h>

/*
* 32 bytes...
* Small string capacity threshold: 15 chars...
*/
typedef struct NarrowString
{
    union
    {
        char* heap_ptr;
        char inline_buf[16];
    } storage;
    uint64_t size;
    uint64_t capacity;
} NarrowString;

/*
* Game wide string type used for selector labels...
* 32 bytes...
* Small string capacity threshold: 7 wchar_t code units...
*/
typedef struct WideString
{
    union
    {
        wchar_t* heap_ptr;
        wchar_t inline_buf[8];
    } storage;
    uint64_t size;
    uint64_t capacity;
} WideString;

typedef struct VectorPtr
{
    void** begin;
    void** end;
    void** capacityEnd;
} VectorPtr;

/*
* Simple little plain old data vector...
*/
typedef struct PodVectorPtr
{
    uint32_t capacity;
    uint32_t size;
    void** data;
} PodVectorPtr;

typedef struct SetupRecord
{
    void* houseStatusUI;
    void* manager;
    void* rootNode;
    void* buttonNode;
    void* button;
} SetupRecord;

#endif