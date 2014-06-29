// Copyright 2014 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#ifndef _FX_MEMORY_H_
#define _FX_MEMORY_H_
#ifndef _FX_SYSTEM_H_
#include "fx_system.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif
#define FX_Alloc(type, size)						(type*)malloc(sizeof(type) * (size))
#define FX_Realloc(type, ptr, size)					(type*)realloc(ptr, sizeof(type) * (size))
#define FX_AllocNL(type, size)						FX_Alloc(type, size)
#define FX_ReallocNL(type, ptr, size)				FX_Realloc(type, ptr, size)
#define FX_Free(ptr)								free(ptr)
void*	FXMEM_DefaultAlloc(size_t byte_size, int flags);
void*	FXMEM_DefaultRealloc(void* pointer, size_t new_size, int flags);
void	FXMEM_DefaultFree(void* pointer, int flags);
#ifdef __cplusplus
}
#endif

#ifdef __cplusplus
class CFX_Object
{
public:
    void*			operator new (size_t size, FX_LPCSTR file, int line)
    {
        return malloc(size);
    }
    void			operator delete (void* p, FX_LPCSTR file, int line)
    {
        free(p);
    }
    void*			operator new (size_t size)
    {
        return malloc(size);
    }
    void			operator delete (void* p)
    {
        free(p);
    }
    void*			operator new[] (size_t size, FX_LPCSTR file, int line)
    {
        return malloc(size);
    }
    void			operator delete[] (void* p, FX_LPCSTR file, int line)
    {
        free(p);
    }
    void*			operator new[] (size_t size)
    {
        return malloc(size);
    }
    void			operator delete[] (void* p)
    {
        free(p);
    }
    void*			operator new (size_t, void* buf)
    {
        return buf;
    }
    void			operator delete (void*, void*)							{}
};
#endif

#ifdef __cplusplus
#if defined(_DEBUG)
#define FX_NEW new(__FILE__, __LINE__)
#else
#define FX_NEW new
#endif
#define FX_NEW_VECTOR(Pointer, Class, Count) \
    { \
        Pointer = FX_Alloc(Class, Count); \
        if (Pointer) { \
            for (int i = 0; i < (Count); i ++) new (Pointer + i) Class; \
        } \
    }
#define FX_DELETE_VECTOR(Pointer, Class, Count) \
    { \
        for (int i = 0; i < (Count); i ++) Pointer[i].~Class(); \
        FX_Free(Pointer); \
    }
class CFX_DestructObject : public CFX_Object
{
public:
    virtual ~CFX_DestructObject() {}
};
#endif


#ifdef __cplusplus
extern "C" {
#endif
typedef struct _IFX_Allocator {

    void*	(*m_AllocDebug)(struct _IFX_Allocator* pAllocator, size_t size, FX_LPCSTR file, int line);

    void*	(*m_Alloc)(struct _IFX_Allocator* pAllocator, size_t size);

    void*	(*m_ReallocDebug)(struct _IFX_Allocator* pAllocator, void* p, size_t size, FX_LPCSTR file, int line);

    void*	(*m_Realloc)(struct _IFX_Allocator* pAllocator, void* p, size_t size);

    void	(*m_Free)(struct _IFX_Allocator* pAllocator, void* p);
} IFX_Allocator;
#ifdef __cplusplus
}
#endif


#ifdef _DEBUG
#define FX_Allocator_Alloc(fxAllocator, type, size) \
    ((fxAllocator) ? (type*)(fxAllocator)->m_AllocDebug((fxAllocator), (size) * sizeof(type), __FILE__, __LINE__) : (FX_Alloc(type, size)))
#define FX_Allocator_Realloc(fxAllocator, type, ptr, new_size) \
    ((fxAllocator) ? (type*)(fxAllocator)->m_ReallocDebug((fxAllocator), (ptr), (new_size) * sizeof(type), __FILE__, __LINE__) : (FX_Realloc(type, ptr, new_size)))
#else
#define FX_Allocator_Alloc(fxAllocator, type, size) \
    ((fxAllocator) ? (type*)(fxAllocator)->m_Alloc((fxAllocator), (size) * sizeof(type)) : (FX_Alloc(type, size)))
#define FX_Allocator_Realloc(fxAllocator, type, ptr, new_size) \
    ((fxAllocator) ? (type*)(fxAllocator)->m_Realloc((fxAllocator), (ptr), (new_size) * sizeof(type)) : (FX_Realloc(type, ptr, new_size)))
#endif

#define FX_Allocator_Free(fxAllocator, ptr) \
    ((fxAllocator) ? (fxAllocator)->m_Free((fxAllocator), (ptr)) : (FX_Free(ptr)))


#ifdef __cplusplus
inline void* operator new(size_t size, IFX_Allocator* fxAllocator)
{
    return (void*)FX_Allocator_Alloc(fxAllocator, FX_BYTE, size);
}
inline void operator delete(void* ptr, IFX_Allocator* fxAllocator)
{
}
#define FX_NewAtAllocator(fxAllocator) \
    ::new(fxAllocator)
#define FX_DeleteAtAllocator(pointer, fxAllocator, __class__) \
    (pointer)->~__class__(); \
    FX_Allocator_Free(fxAllocator, pointer)

#if defined(_DEBUG)
#define FX_NEWAT(pAllocator) new(pAllocator, __FILE__, __LINE__)
#else
#define FX_NEWAT(pAllocator) new(pAllocator)
#endif
class CFX_GrowOnlyPool : public IFX_Allocator, public CFX_Object
{
public:

    CFX_GrowOnlyPool(IFX_Allocator* pAllocator = NULL, size_t trunk_size = 16384);

    ~CFX_GrowOnlyPool();

    void	SetAllocator(IFX_Allocator* pAllocator);

    void	SetTrunkSize(size_t trunk_size)
    {
        m_TrunkSize = trunk_size;
    }

    void*	AllocDebug(size_t size, FX_LPCSTR file, int line)
    {
        return Alloc(size);
    }

    void*	Alloc(size_t size);

    void*	ReallocDebug(void* p, size_t new_size, FX_LPCSTR file, int line)
    {
        return NULL;
    }

    void*	Realloc(void* p, size_t new_size)
    {
        return NULL;
    }

    void	Free(void*) {}

    void	FreeAll();
private:

    size_t	m_TrunkSize;

    void*	m_pFirstTrunk;

    IFX_Allocator*	m_pAllocator;
};
#endif


#endif //_FX_MEMORY_H_
