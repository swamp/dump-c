/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Peter Bjorklund. All rights reserved.
 *  Licensed under the MIT License. See LICENSE in the project root for license information.
 *--------------------------------------------------------------------------------------------*/
#include <clog/clog.h>
#include <flood/out_stream.h>
#include <stdarg.h>
#include <swamp-dump/types.h>
#include <swamp-runtime/types.h>
#include <swamp-typeinfo/typeinfo.h>
#include <swamp-dump/dump_ascii_no_color.h>

static void printTabs(FldOutStream* fp, int indentation)
{
    for (size_t i = 0; i < indentation; ++i) {
        fldOutStreamWriteOctets(fp, "    ", 4);
    }
}

static void printNewLineWithTabs(FldOutStream* fp, int indentation)
{
    fldOutStreamWriteUInt8(fp, '\n');
    printTabs(fp, indentation);
}

static void printDots(FldOutStream* fp, int indentation)
{
    for (size_t i = 0; i < indentation; ++i) {
        fldOutStreamWriteOctets(fp, "..", 2);
    }
}

static void printNewLineWithDots(FldOutStream* fp, int indentation)
{
    fldOutStreamWriteUInt8(fp, '\n');
    printDots(fp, indentation);
}

static void printWithColorf(FldOutStream* fp, const char* s, ...)
{
    va_list pl;

    va_start(pl, s);
    fldOutStreamWritevf(fp, s, pl);
    va_end(pl);
}

static int typeIsSimple(const SwtiType* type)
{
    enum SwtiTypeValue v = type->type;
    return (v == SwtiTypeBoolean) || (v == SwtiTypeInt) || (v == SwtiTypeFixed) || (v == SwtiTypeString);
}

int swampDumpToAsciiNoColor(const uint8_t * v, const SwtiType* type, int flags, int indentation, FldOutStream* fp)
{
    switch (type->type) {
        case SwtiTypeBoolean: {
            SwampBool value = *((const SwampBool*)v);
            printWithColorf(fp, "%s", value ? "True" : "False");
        } break;
        case SwtiTypeInt: {
            SwampInt32 value = *((const SwampInt32 *)v);

            printWithColorf(fp, "%d", value);
        } break;
        case SwtiTypeFixed: {
            SwampFixed32 value = *((const SwampFixed32 *)v);

            printWithColorf(fp, "%.3f", value / (float)SWAMP_FIXED_FACTOR);
        } break;
        case SwtiTypeString: {
            const SwampString* p = *((const SwampString**)v);
            if (p->characterCount > 32*1024) {
                CLOG_ERROR("can not have these long strings")
            }
            if (!(flags & swampDumpFlagNoStringQuotesOnce)) {
                printWithColorf(fp, "\"");
            }
            printWithColorf(fp, "%s", p->characters);
            if (!(flags & swampDumpFlagNoStringQuotesOnce)) {
                printWithColorf(fp, "\"");
            }
        } break;
        case SwtiTypeRecord: {
            const SwtiRecordType* record = (const SwtiRecordType*) type;
            printWithColorf(fp, "{ ", fp);
            for (size_t i = 0; i < record->fieldCount; i++) {
                const SwtiRecordTypeField* field = &record->fields[i];
                if (i > 0) {
                    if (!typeIsSimple(field->fieldType)) {
                        printNewLineWithTabs(fp, indentation);
                    }
                    printWithColorf(fp, ", ");
                }
                printWithColorf(fp, field->name);
                printWithColorf(fp, " = ");
                int errorCode = swampDumpToAsciiNoColor(v + field->memoryOffsetInfo.memoryOffset, field->fieldType, flags, indentation + 1, fp);
                if (errorCode != 0) {
                    return errorCode;
                }
            }
            printWithColorf(fp, " }");
        } break;
        case SwtiTypeArray: {
            const SwtiArrayType* arrayType = ((const SwtiArrayType*) type);
            const SwampArray* array = *(const SwampArray**) v;

            const uint8_t* p = array->value;
            printWithColorf(fp, "[| ", fp);
            for (size_t i = 0; i < array->count; i++) {
                if (i > 0) {
                    printNewLineWithTabs(fp, indentation);
                    printWithColorf(fp, ", ");
                }
                int errorCode = swampDumpToAsciiNoColor(p, arrayType->itemType, flags, indentation + 1, fp);
                if (errorCode != 0) {
                    return errorCode;
                }

                p += array->itemSize;
            }
            printWithColorf(fp, " |]");
        } break;
        case SwtiTypeList: {
            const SwampList* list = *((const SwampList**) v);
            const SwtiListType* listType = (const SwtiListType*) type;

            printWithColorf(fp, "[ ", fp);
            const uint8_t* itemPointer = list->value;
            for (size_t i=0; i<list->count; ++i) {
                if (i > 0) {
                    printNewLineWithTabs(fp, indentation);
                    printWithColorf(fp, ", ");
                }
                int errorCode = swampDumpToAsciiNoColor(itemPointer, listType->itemType, flags, indentation + 1, fp);
                if (errorCode != 0) {
                    return errorCode;
                }
                itemPointer += list->itemSize;
            }
            printWithColorf(fp, " ]");
        } break;
        case SwtiTypeTuple: {
            const SwtiTupleType* tuple = (const SwtiTupleType*) type;

            printWithColorf(fp, "( ", fp);
            for (size_t i = 0; i < tuple->fieldCount; i++) {
                const SwtiTupleTypeField* field = &tuple->fields[i];
                if (i > 0) {
                    printNewLineWithTabs(fp, indentation);
                    printWithColorf(fp, ", ");
                }
                int errorCode = swampDumpToAsciiNoColor(v + field->memoryOffsetInfo.memoryOffset, field->fieldType, flags | swampDumpFlagAliasOnce,
                                                 indentation + 1, fp);
                if (errorCode != 0) {
                    return errorCode;
                }
            }
            printWithColorf(fp, " )");
        } break;

        case SwtiTypeFunction:
            CLOG_SOFT_ERROR("can not dump functions")
            return -1;
        case SwtiTypeUnmanaged: {
            const SwampUnmanaged* unmanaged = *(const SwampUnmanaged**) v;
            printWithColorf(fp, "< (%p) ", (void*)unmanaged);
            if (unmanaged->toString) {
                size_t writtenCharacterCount = unmanaged->toString(unmanaged->ptr, 0, fp->p, fp->size - fp->pos - 8);
                fp->pos += writtenCharacterCount;
                fp->p += writtenCharacterCount;
            }
            printWithColorf(fp, ">");
            return 0;
        }
        case SwtiTypeAlias: {
            const SwtiAliasType* alias = (const SwtiAliasType*) type;
            if (flags & swampDumpFlagAlias || flags & swampDumpFlagAliasOnce) {
                printWithColorf(fp, alias->internal.name);
                printWithColorf(fp, " => ");
            }
            int errorCode = swampDumpToAsciiNoColor(v, alias->targetType, flags & ~swampDumpFlagAliasOnce, indentation + 1,
                                             fp);
            if (errorCode != 0) {
                return errorCode;
            }
            break;
        }

        case SwtiTypeRefId: {
            const SwtiTypeRefIdType* typeRefId = (const SwtiTypeRefIdType *) type;
            SwampInt32 value = *((const SwampInt32 *)v);

            printWithColorf(fp, "$");
            printWithColorf(fp, typeRefId->referencedType->name);
            printWithColorf(fp, "(%d)", value);
        } break;

        case SwtiTypeCustom: {
            const SwtiCustomType* custom = (const SwtiCustomType*) type;
            const uint8_t* p = (const uint8_t*)v;
            if (*p >= custom->variantCount) {
                CLOG_ERROR("illegal variant index %d", *p);
            }
            const SwtiCustomTypeVariant* variant = &custom->variantTypes[*p];
            if (flags & swampDumpFlagCustomTypeVariantPrefix) {
                printWithColorf(fp, custom->internal.name);
                printWithColorf(fp, ":");
            }
            printWithColorf(fp, variant->name);
            for (size_t i = 0; i < variant->paramCount; ++i) {
                printWithColorf(fp, " ");
                const SwtiCustomTypeVariantField* field = &variant->fields[i];
                int errorCode = swampDumpToAsciiNoColor(p + field->memoryOffsetInfo.memoryOffset, field->fieldType, flags, indentation + 1, fp);
                if (errorCode != 0) {
                    return errorCode;
                }
            }
            break;
        }
        case SwtiTypeBlob: {
            const SwampBlob* blob = *((const SwampBlob**) v);
            printWithColorf(fp, "blob %d", blob->octetCount);

            if (flags & swampDumpFlagBlobExpanded) {
                size_t count = blob->octetCount > 2048 ? 2048 : blob->octetCount;
                if (flags & swampDumpFlagBlobAutoFormat) {
                    int allIsPrintable = 1;
                    for (size_t i = 0; i < count; ++i) {
                        uint8_t ch = blob->octets[i];
                        if (ch < 32 || ch > 126) {
                            allIsPrintable = 0;
                            break;
                        }
                    }
                    if (allIsPrintable) {
                        flags |= swampDumpFlagBlobAscii;
                    }
                }
                if (flags & swampDumpFlagBlobAscii) {
                    for (size_t i = 0; i < count; ++i) {
                        if ((i % 64) == 0) {
                            printNewLineWithDots(fp, indentation + 1);
                        }
                        printWithColorf(fp, "%c", blob->octets[i]);
                    }
                } else {
                    for (size_t i = 0; i < count; ++i) {
                        if ((i % 32) == 0) {
                            printNewLineWithDots(fp, indentation + 1);
                        }
                        printWithColorf(fp, "%02X ", blob->octets[i]);
                    }
                }
            }
            break;
        }
        case SwtiTypeChar: {
            SwampCharacter ordinalValue = *((const SwampCharacter *)v);
            printWithColorf(fp, "'");
            printWithColorf(fp, "%c", (char) ordinalValue);
            printWithColorf(fp, "'");
            break;
        }
        case SwtiTypeAny: {
            printWithColorf(fp, "ANY");
            break;
        }
        case SwtiTypeAnyMatchingTypes: {
            printWithColorf(fp, "*");
            break;
        }
        case SwtiTypeResourceName: {
            printWithColorf(fp, "@");
            break;
        }

        default: {
            CLOG_ERROR("unknown type %d", type->type);
        }
    }

    return 0;
}

const char* swampDumpToAsciiStringNoColor(const void* v, const SwtiType* type, int flags, char* target, size_t maxCount)
{
    FldOutStream outStream;

    if (maxCount < 64) {
        return 0;
    }

    fldOutStreamInit(&outStream, (uint8_t*) target, maxCount - 6); // reserve for zero

    int errorCode = swampDumpToAsciiNoColor(v, type, flags, 0, &outStream);
    outStream.size = maxCount;
    fldOutStreamWriteUInt8(&outStream, 0);
    if (errorCode != 0) {
        return 0;
    }

    return target;
}
