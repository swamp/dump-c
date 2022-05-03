/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Peter Bjorklund. All rights reserved.
 *  Licensed under the MIT License. See LICENSE in the project root for license information.
 *--------------------------------------------------------------------------------------------*/
#ifndef SWAMP_DUMP_ASCII_NO_COLOR_H
#define SWAMP_DUMP_ASCII_NO_COLOR_H

#include <swamp-runtime/types.h>
#include <swamp-typeinfo/typeinfo.h>

struct FldOutStream;

int swampDumpToAsciiNoColor(const uint8_t * v, const SwtiType* type, int flags, int indentation, struct FldOutStream* fp);
const char* swampDumpToAsciiStringNoColor(const void* v, const SwtiType* type, int flags, char* target, size_t maxCount);

#endif
