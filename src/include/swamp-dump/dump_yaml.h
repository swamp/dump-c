/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Peter Bjorklund. All rights reserved.
 *  Licensed under the MIT License. See LICENSE in the project root for license information.
 *--------------------------------------------------------------------------------------------*/
#ifndef SWAMP_DUMP_DUMP_YAML_H
#define SWAMP_DUMP_DUMP_YAML_H

struct FldInStream;
struct FldOutStream;
struct SwtiType;
struct SwtiType;

#include <stddef.h>

int swampDumpFromYaml(struct FldInStream* inStream, const struct SwtiType* tiType,
    const void** out);

int swampDumpToYaml(const void* v, const struct SwtiType* type, int flags, int indentation, struct FldOutStream* fp);
const char* swampDumpToYamlString(const struct swamp_value* v, const struct SwtiType* type, int flags, char* target, size_t maxCount);

#endif // SWAMP_DUMP_DUMP_YAML_H
