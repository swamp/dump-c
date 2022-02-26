/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Peter Bjorklund. All rights reserved.
 *  Licensed under the MIT License. See LICENSE in the project root for license information.
 *--------------------------------------------------------------------------------------------*/
#ifndef SWAMP_DUMP_DUMP_H
#define SWAMP_DUMP_DUMP_H

#include <swamp-runtime/types.h>
#include <swamp-dump/dump_unmanaged.h>

struct SwtiType;
struct SwtiUnmanagedType;
struct FldInStream;
struct FldOutStream;
struct SwampDynamicMemory;
struct SwampUnmanagedMemory;

int swampDumpToOctets(struct FldOutStream* stream, const void* v, const struct SwtiType* type);
int swampDumpToOctetsRaw(struct FldOutStream* stream, const void* v, const struct SwtiType* type);


int swampDumpFromOctets(struct FldInStream* inStream, const struct SwtiType* tiType, unmanagedTypeCreator creator,
                        void* context, void* target, struct SwampDynamicMemory* memory, struct SwampUnmanagedMemory* targetUnmanagedMemory);
int swampDumpFromOctetsRaw(struct FldInStream* inStream, const struct SwtiType* tiType, unmanagedTypeCreator creator,
                           void* context,void* target, struct SwampDynamicMemory* memory, struct SwampUnmanagedMemory* targetUnmanagedMemory);
#endif
