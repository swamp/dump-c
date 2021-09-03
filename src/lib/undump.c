/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Peter Bjorklund. All rights reserved.
 *  Licensed under the MIT License. See LICENSE in the project root for license information.
 *--------------------------------------------------------------------------------------------*/
#include <clog/clog.h>
#include <flood/in_stream.h>
#include <swamp-dump/dump.h>
#include <swamp-runtime/allocator.h>
#include <swamp-typeinfo/typeinfo.h>
#include <tiny-libc/tiny_libc.h>

int swampDumpFromOctetsHelper(FldInStream* inStream, struct swamp_allocator* allocator, const SwtiType* tiType,
                              unmanagedTypeCreator creator, const swamp_value** out)
{
    switch (tiType->type) {
        case SwtiTypeInt: {
            swamp_int32 v;
            fldInStreamReadInt32(inStream, &v);
            *out = swamp_allocator_alloc_integer(allocator, v);
        } break;
        case SwtiTypeBoolean: {
            uint8_t truth;
            fldInStreamReadUInt8(inStream, &truth);
            *out = swamp_allocator_alloc_boolean(allocator, truth);
        } break;
        case SwtiTypeString: {
            uint8_t stringLength;
            fldInStreamReadUInt8(inStream, &stringLength);
            uint8_t* characters = tc_malloc_type_count(uint8_t, stringLength + 1);
            fldInStreamReadOctets(inStream, characters, stringLength);
            characters[stringLength] = 0;
            *out = swamp_allocator_alloc_string(allocator, (const char*) characters);
            break;
        }
        case SwtiTypeRecord: {
            const SwtiRecordType* record = (const SwtiRecordType*) tiType;
            swamp_struct* temp = (swamp_struct*) swamp_allocator_alloc_struct(allocator, record->fieldCount);
            for (size_t i = 0; i < record->fieldCount; ++i) {
                const SwtiRecordTypeField* field = &record->fields[i];
                swampDumpFromOctetsHelper(inStream, allocator, field->fieldType, creator, &temp->fields[i]);
            }
            *out = (const swamp_value*) temp;
            break;
        }
        case SwtiTypeArray: {
            const SwtiArrayType* array = (const SwtiArrayType*) tiType;
            uint8_t arrayLength;
            fldInStreamReadUInt8(inStream, &arrayLength);
            swamp_struct* temp = (swamp_struct*) swamp_allocator_alloc_struct(allocator, arrayLength);
            for (size_t i = 0; i < arrayLength; ++i) {
                swampDumpFromOctetsHelper(inStream, allocator, array->itemType, creator, &temp->fields[i]);
            }
            *out = (const swamp_value*) temp;
            break;
        }
        case SwtiTypeTuple: {
            const SwtiTupleType* tuple = (const SwtiTupleType*) tiType;
            swamp_struct* temp = (swamp_struct*) swamp_allocator_alloc_struct(allocator, tuple->parameterCount);
            for (size_t i = 0; i < tuple->parameterCount; ++i) {
                swampDumpFromOctetsHelper(inStream, allocator, tuple->parameterTypes[i], creator, &temp->fields[i]);
            }
            *out = (const swamp_value*) temp;
            break;
        }
        case SwtiTypeList: {
            const SwtiListType* list = (const SwtiListType*) tiType;
            uint8_t listLength;
            fldInStreamReadUInt8(inStream, &listLength);
            const swamp_value** targetValues = tc_malloc_type_count(swamp_value*, listLength);
            for (size_t i = 0; i < listLength; ++i) {
                swampDumpFromOctetsHelper(inStream, allocator, list->itemType, creator, &targetValues[i]);
            }
            swamp_struct* temp = swamp_allocator_alloc_list_create(allocator, targetValues, listLength);
            tc_free(targetValues);
            *out = (const swamp_value*) temp;
            break;
        }
        case SwtiTypeCustom: {
            const SwtiCustomType* custom = (const SwtiCustomType*) tiType;
            uint8_t enumIndex;
            fldInStreamReadUInt8(inStream, &enumIndex);
            const SwtiCustomTypeVariant* variant = &custom->variantTypes[enumIndex];
            swamp_enum* newEnum = (swamp_enum*) swamp_allocator_alloc_enum(allocator, enumIndex, variant->paramCount);
            for (size_t i = 0; i < variant->paramCount; ++i) {
                const SwtiType* paramType = variant->paramTypes[i];
                swampDumpFromOctetsHelper(inStream, allocator, paramType, creator, &newEnum->fields[i]);
            }
            *out = (const swamp_value*) newEnum;
            break;
        }
        case SwtiTypeAlias: {
            const SwtiAliasType* alias = (const SwtiAliasType*) tiType;
            return swampDumpFromOctetsHelper(inStream, allocator, alias->targetType, creator, out);
        }
        case SwtiTypeFunction: {
            CLOG_SOFT_ERROR("functions can not be serialized");
            return -1;
        }
        case SwtiTypeBlob: {
            uint32_t octetCount;
            int errorCode = fldInStreamReadUInt32(inStream, &octetCount);
            if (errorCode < 0) {
                return errorCode;
            }
            *out = swamp_allocator_alloc_blob(allocator, octetCount == 0 ? 0 : inStream->p, octetCount, 0);
            inStream->p += octetCount;
            inStream->pos += octetCount;
            break;
        }
        case SwtiTypeUnmanaged: {
            const SwtiUnmanagedType* unmanagedType = (const SwtiUnmanagedType*) tiType;
            if (creator == 0) {
                CLOG_ERROR("tried to deserialize unmanaged '%s', but no creator was provided", unmanagedType->internal.name);
                return -2;
            }
            const swamp_unmanaged* unmanagedValue = creator(unmanagedType);
            int errorCode = unmanagedValue->deserialize(unmanagedValue->ptr, allocator, inStream);
            if (errorCode < 0) {
                CLOG_SOFT_ERROR("could not deserialize unmanaged type %s %d", unmanagedType->internal.name, errorCode);
                return errorCode;
            }
        }
        default:
            CLOG_ERROR("can not deserialize dump from type %d", tiType->type);
            return -1;
            break;
    }

    return 0;
}

static int readVersion(FldInStream* inStream)
{
    uint8_t major, minor, patch;
    fldInStreamReadUInt8(inStream, &major);
    fldInStreamReadUInt8(inStream, &minor);
    fldInStreamReadUInt8(inStream, &patch);

    int supportedVersion = (major == 0) && (minor == 1);
    if (!supportedVersion) {
        CLOG_SOFT_ERROR("swamp-dump: wrong version %d.%d.%d", major, minor, patch);
        return -1;
    }

    return 0;
}

int swampDumpFromOctets(FldInStream* inStream, struct swamp_allocator* allocator, const SwtiType* tiType,
                        unmanagedTypeCreator creator, const swamp_value** out)
{
    int error;
    if ((error = readVersion(inStream)) < 0) {
        return error;
    }

    return swampDumpFromOctetsHelper(inStream, allocator, tiType, creator, out);
}

int swampDumpFromOctetsRaw(FldInStream* inStream, struct swamp_allocator* allocator, const SwtiType* tiType,
                           unmanagedTypeCreator creator, const swamp_value** out)
{
    return swampDumpFromOctetsHelper(inStream, allocator, tiType, creator, out);
}
