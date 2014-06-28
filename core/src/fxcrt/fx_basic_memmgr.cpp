// Copyright 2014 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#include "../../include/fxcrt/fx_basic.h"
#ifdef __cplusplus
extern "C" {
#endif
void*	FXMEM_DefaultAlloc(size_t byte_size, int flags)
{
    return (void*)malloc(byte_size);
}
void*	FXMEM_DefaultRealloc(void* pointer, size_t new_size, int flags)
{
    return realloc(pointer, new_size);
}
void	FXMEM_DefaultFree(void* pointer, int flags)
{
    free(pointer);
}
static void* _DefAllocDebug(IFX_Allocator* pAllocator, size_t size, FX_LPCSTR filename, int line)
{
    return malloc(size);
}
static void* _DefAlloc(IFX_Allocator* pAllocator, size_t size)
{
    return malloc(size);
}
static void* _DefReallocDebug(IFX_Allocator* pAllocator, void* p, size_t size, FX_LPCSTR filename, int line)
{
    return realloc(p, size);
}
static void* _DefRealloc(IFX_Allocator* pAllocator, void* p, size_t size)
{
    return realloc(p, size);
}
static void _DefFree(IFX_Allocator* pAllocator, void* p)
{
    free(p);
}
static IFX_Allocator g_DefAllocator = {_DefAllocDebug, _DefAlloc, _DefReallocDebug, _DefRealloc, _DefFree};
IFX_Allocator* FXMEM_GetDefAllocator()
{
    return &g_DefAllocator;
}
#ifdef __cplusplus
}
#endif
extern "C" {
    static void* _GOPAllocDebug(IFX_Allocator* pAllocator, size_t size, FX_LPCSTR file, int line)
    {
        return ((CFX_GrowOnlyPool*)pAllocator)->Alloc(size);
    }
    static void* _GOPAlloc(IFX_Allocator* pAllocator, size_t size)
    {
        return ((CFX_GrowOnlyPool*)pAllocator)->Alloc(size);
    }
    static void* _GOPReallocDebug(IFX_Allocator* pAllocator, void* p, size_t new_size, FX_LPCSTR file, int line)
    {
        return ((CFX_GrowOnlyPool*)pAllocator)->Realloc(p, new_size);
    }
    static void* _GOPRealloc(IFX_Allocator* pAllocator, void* p, size_t new_size)
    {
        return ((CFX_GrowOnlyPool*)pAllocator)->Realloc(p, new_size);
    }
    static void _GOPFree(IFX_Allocator* pAllocator, void* p)
    {
    }
};
CFX_GrowOnlyPool::CFX_GrowOnlyPool(IFX_Allocator* pAllocator, size_t trunk_size)
{
    m_TrunkSize = trunk_size;
    m_pFirstTrunk = NULL;
    m_pAllocator = pAllocator ? pAllocator : FXMEM_GetDefAllocator();
    m_AllocDebug = _GOPAllocDebug;
    m_Alloc = _GOPAlloc;
    m_ReallocDebug = _GOPReallocDebug;
    m_Realloc = _GOPRealloc;
    m_Free = _GOPFree;
}
CFX_GrowOnlyPool::~CFX_GrowOnlyPool()
{
    FreeAll();
}
void CFX_GrowOnlyPool::SetAllocator(IFX_Allocator* pAllocator)
{
    ASSERT(m_pFirstTrunk == NULL);
    m_pAllocator = pAllocator ? pAllocator : FXMEM_GetDefAllocator();
}
struct _FX_GrowOnlyTrunk {
    size_t	m_Size;
    size_t	m_Allocated;
    _FX_GrowOnlyTrunk*	m_pNext;
};
void CFX_GrowOnlyPool::FreeAll()
{
    _FX_GrowOnlyTrunk* pTrunk = (_FX_GrowOnlyTrunk*)m_pFirstTrunk;
    while (pTrunk) {
        _FX_GrowOnlyTrunk* pNext = pTrunk->m_pNext;
        m_pAllocator->m_Free(m_pAllocator, pTrunk);
        pTrunk = pNext;
    }
    m_pFirstTrunk = NULL;
}
void* CFX_GrowOnlyPool::Alloc(size_t size)
{
    size = (size + 3) / 4 * 4;
    _FX_GrowOnlyTrunk* pTrunk = (_FX_GrowOnlyTrunk*)m_pFirstTrunk;
    while (pTrunk) {
        if (pTrunk->m_Size - pTrunk->m_Allocated >= size) {
            void* p = (FX_LPBYTE)(pTrunk + 1) + pTrunk->m_Allocated;
            pTrunk->m_Allocated += size;
            return p;
        }
        pTrunk = pTrunk->m_pNext;
    }
    size_t alloc_size = size > m_TrunkSize ? size : m_TrunkSize;
    pTrunk = (_FX_GrowOnlyTrunk*)m_pAllocator->m_Alloc(m_pAllocator, sizeof(_FX_GrowOnlyTrunk) + alloc_size);
    pTrunk->m_Size = alloc_size;
    pTrunk->m_Allocated = size;
    pTrunk->m_pNext = (_FX_GrowOnlyTrunk*)m_pFirstTrunk;
    m_pFirstTrunk = pTrunk;
    return pTrunk + 1;
}
extern const FX_BYTE OneLeadPos[256] = {
    8, 7, 6, 6, 5, 5, 5, 5, 4, 4, 4, 4, 4, 4, 4, 4,
    3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
    2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
    2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
};
extern const FX_BYTE ZeroLeadPos[256] = {
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
    2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
    3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
    4, 4, 4, 4, 4, 4, 4, 4, 5, 5, 5, 5, 6, 6, 7, 8,
};
