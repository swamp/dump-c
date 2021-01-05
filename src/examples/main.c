/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Peter Bjorklund. All rights reserved.
 *  Licensed under the MIT License. See LICENSE in the project root for license information.
 *--------------------------------------------------------------------------------------------*/
#include <clog/clog.h>
#include <stdio.h>
#include <swamp-dump/dump.h>
#include <swamp-dump/dump_ascii.h>
#include <swamp-runtime/allocator.h>
#include <swamp-runtime/print.h>

#include <swamp-typeinfo/chunk.h>
#include <swamp-typeinfo/deserialize.h>
#include <swamp-typeinfo/typeinfo.h>

#include <flood/in_stream.h>
#include <flood/out_stream.h>

clog_config g_clog;

static void tyran_log_implementation(enum clog_type type, const char* string)
{
    (void) type;
    fprintf(stderr, "%s\n", string);
}

int rtti(SwtiChunk* chunk)
{
    const uint8_t octets[] = {
        0x0a,
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
        5,
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
    };

    int error = swtiDeserialize(octets, sizeof(octets), chunk);
    if (error != 0) {
        CLOG_ERROR("deserialize problem");
        return error;
    }

    return 0;
}

int main()
{
    g_clog.log = tyran_log_implementation;
    swamp_allocator allocator;
    swamp_allocator_init(&allocator);

    SwtiChunk chunk;
    rtti(&chunk);

    const SwtiType* recordType = chunk.types[4];
    swamp_struct* tempStruct = swamp_allocator_alloc_struct(&allocator, 5);

    //    const swamp_value *v = swamp_allocator_alloc_integer(&allocator, 42);

    const swamp_value* b = swamp_allocator_alloc_boolean(&allocator, 1);

    swamp_struct* posStruct = swamp_allocator_alloc_struct(&allocator, 2);

    const swamp_value* x = swamp_allocator_alloc_integer(&allocator, 10);
    const swamp_value* y = swamp_allocator_alloc_integer(&allocator, 120);
    posStruct->fields[0] = x;
    posStruct->fields[1] = y;

    const swamp_value* str = swamp_allocator_alloc_string(&allocator, "hello");

    swamp_struct* ar = swamp_allocator_alloc_struct(&allocator, 1);
    ar->fields[0] = posStruct;

    swamp_list* lr = swamp_allocator_alloc_list_create(&allocator, &posStruct, 1);

    //    swamp_enum* custom = swamp_allocator_alloc_enum(&allocator, 1, 1);
    //    const swamp_value *maybeParamInt = swamp_allocator_alloc_integer(&allocator, 99);
    // custom->fields[0] = maybeParamInt;

    swamp_enum* custom = swamp_allocator_alloc_enum(&allocator, 0, 0);

    tempStruct->fields[0] = b;
    tempStruct->fields[1] = str;
    tempStruct->fields[2] = posStruct;
    tempStruct->fields[3] = lr;
    tempStruct->fields[4] = custom;

    uint8_t temp[1024];
    FldOutStream outStream;
    fldOutStreamInit(&outStream, temp, 1024);

    swampDumpToOctets(&outStream, tempStruct, recordType);

    struct FldInStream inStream;
    fldInStreamInit(&inStream, temp, outStream.pos);

    const swamp_value* outValue;
    swampDumpFromOctets(&inStream, &allocator, recordType, &outValue);
    //    swamp_value_print(outValue, "result");

    FldOutStream asciiOut;
    fldOutStreamInit(&asciiOut, temp, 1024);
    swampDumpToAscii(outValue, recordType, 0, &asciiOut);
    fldOutStreamWriteUint8(&asciiOut, 0);

    const char* temp2 = temp;

    fputs(temp2, stderr);
    fputs("\n", stderr);

    return 0;
}
