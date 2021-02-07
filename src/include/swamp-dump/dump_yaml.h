/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Peter Bjorklund. All rights reserved.
 *  Licensed under the MIT License. See LICENSE in the project root for license information.
 *--------------------------------------------------------------------------------------------*/
#ifndef SWAMP_DUMP_DUMP_YAML_H
#define SWAMP_DUMP_DUMP_YAML_H

struct FldInStream;
struct SwtiType;
struct swamp_value;
struct swamp_allocator;
struct SwtiType;

#include <stddef.h>

int swampDumpFromYaml(struct FldInStream* inStream, struct swamp_allocator* allocator, const struct SwtiType* tiType,
    const struct swamp_value** out);

int swampDumpToYaml(const struct swamp_value* v, const struct SwtiType* type, int flags, int indentation, struct FldOutStream* fp);
const char* swampDumpToYamlString(const struct swamp_value* v, const struct SwtiType* type, int flags, char* target, size_t maxCount);

#endif // SWAMP_DUMP_DUMP_YAML_H
