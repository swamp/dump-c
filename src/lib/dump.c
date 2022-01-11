/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Peter Bjorklund. All rights reserved.
 *  Licensed under the MIT License. See LICENSE in the project root for license information.
 *--------------------------------------------------------------------------------------------*/
#include <clog/clog.h>
#include <flood/out_stream.h>
#include <swamp-dump/dump.h>
#include <swamp-typeinfo/typeinfo.h>

int swampDumpToOctetsHelper(FldOutStream* stream, const void* v, const SwtiType* type)
{
    switch (type->type) {
        case SwtiTypeBoolean: {
            const SwampBool truth = *(SwampBool*)v;
            fldOutStreamWriteUInt8(stream, truth);
        } break;
        case SwtiTypeInt: {
            SwampInt32 value = *(SwampInt32*)v;
            fldOutStreamWriteInt32(stream, value);
        } break;
        case SwtiTypeFixed: {
            SwampFixed32 value = *(SwampFixed32*)v;
            fldOutStreamWriteInt32(stream, value);
        } break;
        case SwtiTypeString: {
            const SwampString* p = * (const SwampString**) v;
            size_t stringLength = p->characterCount;
            fldOutStreamWriteUInt8(stream, stringLength+1); // include zero terminator
            fldOutStreamWriteOctets(stream, (const uint8_t*) p->characters, stringLength+1);
        } break;
        case SwtiTypeRecord: {
            const SwtiRecordType* record = (const SwtiRecordType*) type;
            for (size_t i = 0; i < record->fieldCount; i++) {
                const SwtiRecordTypeField* field = &record->fields[i];
                int errorCode = swampDumpToOctetsHelper(stream, ((uint8_t*)v) + field->memoryOffsetInfo.memoryOffset, field->fieldType);
                if (errorCode != 0) {
                    return errorCode;
                }
            }
        } break;
        case SwtiTypeTuple: {
            const SwtiTupleType* tuple = (const SwtiTupleType *) type;
            for (size_t i = 0; i < tuple->fieldCount; i++) {
                const SwtiTupleTypeField* field = &tuple->fields[i];
                int errorCode = swampDumpToOctetsHelper(stream, ((uint8_t*)v) + field->memoryOffsetInfo.memoryOffset, tuple->fields[i].fieldType);
                if (errorCode != 0) {
                    return errorCode;
                }
            }
        } break;
        case SwtiTypeArray: {
            const SwtiArrayType* arrayType = (const SwtiArrayType*) type;
            const SwampArray * array = *(const SwampArray**)v;
            fldOutStreamWriteUInt8(stream, array->count);
            size_t itemOffset = 0;
            for (size_t i = 0; i < array->count; i++) {
                int errorCode = swampDumpToOctetsHelper(stream, ((uint8_t*)v) + itemOffset, arrayType->itemType);
                if (errorCode != 0) {
                    return errorCode;
                }
                itemOffset += array->itemSize;
            }
        } break;
        case SwtiTypeList: {
            const SwtiListType* listType = (const SwtiListType*) type;
            const SwampList* list = *(const SwampList**)v;
            fldOutStreamWriteUInt8(stream, list->count);
            size_t itemOffset = 0;
            for (size_t i=0; i < list->count; ++i) {
                int errorCode = swampDumpToOctetsHelper(stream, (uint8_t*)list->value + itemOffset, listType->itemType);
                if (errorCode != 0) {
                    return errorCode;
                }

                itemOffset += list->itemSize;
            }
        } break;
        case SwtiTypeFunction: {
            CLOG_SOFT_ERROR("function can not be serialized to a dump format");
            return -1;
        } break;
        case SwtiTypeBlob: {
            const SwampBlob* blob = *(const SwampBlob**) v;
            fldOutStreamWriteUInt32(stream, blob->octetCount);
            fldOutStreamWriteOctets(stream, blob->octets, blob->octetCount);
        } break;
        case SwtiTypeAlias: {
            const SwtiAliasType* alias = (const SwtiAliasType*) type;
            return swampDumpToOctetsHelper(stream, v, alias->targetType);
        }
        case SwtiTypeCustom: {
            const SwtiCustomType* customType = (const SwtiCustomType*) type;
            const uint8_t enumValue = *(const uint8_t*)v;
            fldOutStreamWriteUInt8(stream, enumValue);
            const SwtiCustomTypeVariant* variant = &customType->variantTypes[enumValue];
            for (size_t i = 0; i < variant->paramCount; ++i) {
                const SwtiType* paramType = variant->fields[i].fieldType;
                int error;
                if ((error = swampDumpToOctetsHelper(stream, ((uint8_t*)v) + variant->fields[i].memoryOffsetInfo.memoryOffset, paramType)) != 0) {
                    CLOG_SOFT_ERROR("could not serialize variant");
                    return error;
                }
            }
            break;
        }
        case SwtiTypeUnmanaged: {
            //const SwtiUnmanagedType* unmanagedType = (const SwtiUnmanagedType*) type;
            const SwampUnmanaged* unmanagedValue = *(const SwampUnmanaged**) v;
            int serializeErr = unmanagedValue->serialize(unmanagedValue->ptr, stream->p, stream->size - stream->pos);
            if (serializeErr < 0) {
                return serializeErr;
            }
            stream->p += serializeErr;
            stream->pos += serializeErr;
        } break;
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

int swampDumpToOctets(FldOutStream* stream, const void* v, const SwtiType* type)
{
    writeVersion(stream);

    return swampDumpToOctetsHelper(stream, v, type);
}

int swampDumpToOctetsRaw(FldOutStream* stream, const void* v, const SwtiType* type)
{
    return swampDumpToOctetsHelper(stream, v, type);
}
