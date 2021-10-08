/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Peter Bjorklund. All rights reserved.
 *  Licensed under the MIT License. See LICENSE in the project root for license information.
 *--------------------------------------------------------------------------------------------*/
#ifndef SWAMP_DUMP_DUMP_H
#define SWAMP_DUMP_DUMP_H

#include <swamp-runtime/types.h>

struct SwtiType;
struct SwtiUnmanagedType;
struct FldInStream;
struct FldOutStream;
struct swamp_allocator;

typedef const void* (*unmanagedTypeCreator)(void* context, const struct SwtiUnmanagedType* type);

int swampDumpToOctets(struct FldOutStream* stream, const void* v, const struct SwtiType* type);
int swampDumpToOctetsRaw(struct FldOutStream* stream, const void* v, const struct SwtiType* type);
int swampDumpFromOctets(struct FldInStream* inStream, const struct SwtiType* tiType, unmanagedTypeCreator creator,
                        void* context, const void** out);
int swampDumpFromOctetsRaw(struct FldInStream* inStream, const struct SwtiType* tiType, unmanagedTypeCreator creator,
                           void* context, const void** out);
#endif
