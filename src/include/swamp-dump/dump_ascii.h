/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Peter Bjorklund. All rights reserved.
 *  Licensed under the MIT License. See LICENSE in the project root for license information.
 *--------------------------------------------------------------------------------------------*/
#ifndef SWAMP_DUMP_ASCII_H
#define SWAMP_DUMP_ASCII_H

#include <swamp-runtime/types.h>
#include <swamp-typeinfo/typeinfo.h>
struct FldOutStream;


int swampDumpToAscii(const swamp_value* v, const SwtiType* type, int flags, int indentation, struct FldOutStream* fp);
const char* swampDumpToAsciiString(const swamp_value* v, const SwtiType* type, int flags, char* target, size_t maxCount);

#endif
