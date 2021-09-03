/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Peter Bjorklund. All rights reserved.
 *  Licensed under the MIT License. See LICENSE in the project root for license information.
 *--------------------------------------------------------------------------------------------*/
#include <clog/clog.h>
#include <flood/out_stream.h>
#include <swamp-dump/dump.h>
#include <swamp-typeinfo/typeinfo.h>

int swampDumpToOctetsHelper(FldOutStream* stream, const swamp_value* v, const SwtiType* type)
{
    switch (type->type) {
        case SwtiTypeBoolean: {
            swamp_bool truth = swamp_value_bool(v);
            fldOutStreamWriteUInt8(stream, truth);
        } break;
        case SwtiTypeInt: {
            swamp_int32 value = swamp_value_int(v);
            fldOutStreamWriteInt32(stream, value);
        } break;
        case SwtiTypeFixed: {
            swamp_fixed32 value = swamp_value_fixed(v);
            fldOutStreamWriteInt32(stream, value);
        } break;
        case SwtiTypeString: {
            const swamp_string* p = swamp_value_string(v);
            size_t stringLength = tc_strlen(p->characters);
            fldOutStreamWriteUInt8(stream, stringLength);
            fldOutStreamWriteOctets(stream, (const uint8_t*) p->characters, stringLength);
        } break;
        case SwtiTypeRecord: {
            const swamp_struct* p = swamp_value_struct(v);
            const SwtiRecordType* record = (const SwtiRecordType*) type;
            if (p->info.field_count != record->fieldCount) {
                CLOG_SOFT_ERROR("problem with field count");
                return -2;
            }
            for (size_t i = 0; i < record->fieldCount; i++) {
                const SwtiRecordTypeField* field = &record->fields[i];
                int errorCode = swampDumpToOctetsHelper(stream, p->fields[i], field->fieldType);
                if (errorCode != 0) {
                    return errorCode;
                }
            }
        } break;
        case SwtiTypeArray: {
            const SwtiArrayType* array = (const SwtiArrayType*) type;
            const swamp_struct* p = swamp_value_struct(v);
            fldOutStreamWriteUInt8(stream, p->info.field_count);

            for (size_t i = 0; i < p->info.field_count; i++) {
                int errorCode = swampDumpToOctetsHelper(stream, p->fields[i], array->itemType);
                if (errorCode != 0) {
                    return errorCode;
                }
            }
        } break;
        case SwtiTypeTuple: {
            const SwtiTupleType* tuple = (const SwtiTupleType *) type;
            const swamp_struct* p = swamp_value_struct(v);

            for (size_t i = 0; i < p->info.field_count; i++) {
                int errorCode = swampDumpToOctetsHelper(stream, p->fields[i], tuple->parameterTypes[i]);
                if (errorCode != 0) {
                    return errorCode;
                }
            }
        } break;
        case SwtiTypeList: {
            const SwtiListType* list = (const SwtiListType*) type;
            const swamp_list* p = swamp_value_list(v);
            fldOutStreamWriteUInt8(stream, p->count);

            int i = 0;
            SWAMP_LIST_FOR_LOOP(p)
            int errorCode = swampDumpToOctetsHelper(stream, value, list->itemType);
            if (errorCode != 0) {
                return errorCode;
            }
            i++;
            SWAMP_LIST_FOR_LOOP_END();
        } break;
        case SwtiTypeFunction: {
            CLOG_SOFT_ERROR("function can not be serialized to a dump format");
            return -1;
        } break;
        case SwtiTypeBlob: {
            const swamp_blob* blob = swamp_value_blob(v);
            fldOutStreamWriteUInt32(stream, blob->octet_count);
            fldOutStreamWriteOctets(stream, blob->octets, blob->octet_count);
        } break;
        case SwtiTypeAlias: {
            const SwtiAliasType* alias = (const SwtiAliasType*) type;

            return swampDumpToOctetsHelper(stream, v, alias->targetType);
        }
        case SwtiTypeCustom: {
            const SwtiCustomType* custom = (const SwtiCustomType*) type;
            const swamp_enum* enumValue = swamp_value_enum(v);
            fldOutStreamWriteUInt8(stream, enumValue->enum_type);
            const SwtiCustomTypeVariant* variant = &custom->variantTypes[enumValue->enum_type];
            for (size_t i = 0; i < variant->paramCount; ++i) {
                const SwtiType* paramType = variant->paramTypes[i];
                int error;
                if ((error = swampDumpToOctetsHelper(stream, enumValue->fields[i], paramType)) != 0) {
                    CLOG_SOFT_ERROR("could not serialize variant");
                    return error;
                }
            }
            break;
        }
        case SwtiTypeUnmanaged: {
            const SwtiUnmanagedType* unmanaged = (const SwtiUnmanagedType*) type;
            const swamp_unmanaged* unmanagedValue = swamp_value_unmanaged(v);
            unmanagedValue->serialize(unmanagedValue->ptr, stream);
        }
        default:
            CLOG_ERROR("Unknown type to serialize %d", type->type);
    }

    return 0;
}

static int writeVersion(FldOutStream* stream)
{
    const uint8_t major = 0;
    const uint8_t minor = 1;
    const uint8_t patch = 0;

    fldOutStreamWriteUInt8(stream, major);
    fldOutStreamWriteUInt8(stream, minor);
    fldOutStreamWriteUInt8(stream, patch);

    return 0;
}

int swampDumpToOctets(FldOutStream* stream, const swamp_value* v, const SwtiType* type)
{
    writeVersion(stream);

    return swampDumpToOctetsHelper(stream, v, type);
}

int swampDumpToOctetsRaw(FldOutStream* stream, const swamp_value* v, const SwtiType* type)
{
    return swampDumpToOctetsHelper(stream, v, type);
}
