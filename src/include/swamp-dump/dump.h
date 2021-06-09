/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Peter Bjorklund. All rights reserved.
 *  Licensed under the MIT License. See LICENSE in the project root for license information.
 *--------------------------------------------------------------------------------------------*/
#ifndef SWAMP_DUMP_DUMP_H
#define SWAMP_DUMP_DUMP_H

#include <swamp-runtime/types.h>

struct SwtiType;
struct FldInStream;
struct FldOutStream;
struct swamp_allocator;

int swampDumpToOctets(struct FldOutStream* stream, const swamp_value* v, const struct SwtiType* type);
int swampDumpToOctetsRaw(struct FldOutStream* stream, const swamp_value* v, const struct SwtiType* type);
int swampDumpFromOctets(struct FldInStream* inStream, struct swamp_allocator* allocator, const struct SwtiType* tiType,
                        const swamp_value** out);
int swampDumpFromOctetsRaw(struct FldInStream* inStream, struct swamp_allocator* allocator,
                           const struct SwtiType* tiType, const swamp_value** out);
#endif
