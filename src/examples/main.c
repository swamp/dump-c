/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Peter Bjorklund. All rights reserved.
 *  Licensed under the MIT License. See LICENSE in the project root for license information.
 *--------------------------------------------------------------------------------------------*/
#include <clog/clog.h>
#include <clog/console.h>
#include <stdio.h>

#include <swamp-dump/dump.h>
#include <swamp-dump/dump_ascii.h>
#include <swamp-dump/dump_yaml.h>
#include <swamp-dump/types.h>

#include <swamp-runtime/print.h>

#include <swamp-typeinfo/chunk.h>
#include <swamp-typeinfo/deserialize.h>
#include <swamp-typeinfo/typeinfo.h>

#include <flood/in_stream.h>
#include <flood/out_stream.h>

clog_config g_clog;

int rtti(SwtiChunk* chunk)
{
    const uint8_t octets[] = {
        0, // Major
        1, // Minor
        3, // Patch
        0x0b, // Types that follow
        SwtiTypeInt,
        SwtiTypeList,
        0x05,
        SwtiTypeArray,
        0x05,
        SwtiTypeAlias,
        0x4,
        'C',
        'o',
        'o',
        'l',
        4,
        SwtiTypeRecord,
        6,
        1,
        'a',
        9,
        4,
        'n',
        'a',
        'm',
        'e',
        6,
        3,
        'p',
        'o',
        's',
        5,
        2,
        'a',
        'r',
        1,
        2,
        'm',
        'a',
        8,
        2,
        't',
        'i',
        10,
        SwtiTypeRecord,
        2,
        1,
        'x',
        0,
        1,
        'y',
        0,
        SwtiTypeString,
        SwtiTypeFunction,
        2,
        1,
        2,
        SwtiTypeCustom,
        5,
        'M',
        'a',
        'y',
        'b',
        'e',
        2,
        3,
        'N',
        'o',
        't',
        0,
        4,
        'J',
        'u',
        's',
        't',
        1,
        0,
        SwtiTypeBoolean,
        SwtiTypeBlob
    };

    int error = swtiDeserialize(octets, sizeof(octets), chunk);
    if (error < 0) {
        CLOG_ERROR("deserialize problem");
        return error;
    }

    return 0;
}

int main(int argc, const char* argv[])
{
    g_clog.log = clog_console;

    return 0;
}
