/*
    Copyright 2015 Wolfgang Thaller.

    This file is part of Retro68.

    Retro68 is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    Retro68 is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    Under Section 7 of GPL version 3, you are granted additional
    permissions described in the GCC Runtime Library Exception, version
    3.1, as published by the Free Software Foundation.

    You should have received a copy of the GNU General Public License and
    a copy of the GCC Runtime Library Exception along with this program;
    see the files COPYING and COPYING.RUNTIME respectively.  If not, see
    <http://www.gnu.org/licenses/>.
*/

#include <stdlib.h>
#include <errno.h>
#include <reent.h>
#include <string.h>
#warning BIDDLE: figure out the retro malloc stuff for BeBox
#include <Multiverse.h>
//#include <MacMemory.h>

void referenceMyMalloc(void) {}

void *_malloc_r(struct _reent *reent_ptr, size_t sz)
{
    Ptr p = NewPtr(sz);

    if(!p)
        reent_ptr->_errno = ENOMEM;

    return p;
}
void *_calloc_r(struct _reent *reent_ptr, size_t sz, size_t sz2)
{
    Ptr p = NewPtrClear(sz*sz2);

    if(!p)
        reent_ptr->_errno = ENOMEM;

    return p;
}

void _free_r(struct _reent *reent_ptr, void *ptr)
{
    if(ptr != NULL)
        DisposePtr(ptr);
}

void *_realloc_r(struct _reent *reent_ptr, void *ptr, size_t sz)
{
    if(ptr == NULL)
    {
        Ptr p = NewPtr(sz);

        if(!p)
            reent_ptr->_errno = ENOMEM;

        return p;
    }
    else
    {
        MemError();
        SetPtrSize(ptr, sz);
        if(MemError())
        {
            size_t oldSz = GetPtrSize(ptr);
            if(sz > oldSz)
            {
                void *newPtr = NewPtr(sz);
                if(!newPtr)
                {
                    reent_ptr->_errno = ENOMEM;
                    return NULL;
                }
                memcpy(newPtr, ptr, oldSz);
                return newPtr;
            }
            else
            {
                reent_ptr->_errno = ENOMEM;
                return NULL;
            }
        }
        else
            return ptr;
    }
}

void *malloc(size_t sz)
{
    return _malloc_r(_REENT, sz);
}

void free(void *p)
{
    _free_r(_REENT, p);
}

void *realloc(void *ptr, size_t sz)
{
    return _realloc_r(_REENT, ptr, sz);
}

void *calloc(size_t sz1, size_t sz2)
{
    return _calloc_r(_REENT, sz1, sz2);
}

void *memalign(size_t alignment, size_t sz)
{
    Ptr p = NewPtr(sz);

    if(!p)
        errno = ENOMEM;

    // FIXME:
    // NewPtr aligns to 4 bytes on 68020 and 68030,
    // and to 16 bytes on 68040 and PowerPC.

    // Do something else when more alignment is required.
    // This might be hard, as adding extra overhead to all normal allocations
    // just so that we can distinguish things in free() doesn't sound like it's worth it.

    return p;
}

