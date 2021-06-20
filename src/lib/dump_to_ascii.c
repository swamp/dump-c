/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Peter Bjorklund. All rights reserved.
 *  Licensed under the MIT License. See LICENSE in the project root for license information.
 *--------------------------------------------------------------------------------------------*/
#include <clog/clog.h>
#include <flood/out_stream.h>
#include <stdarg.h>
#include <swamp-dump/dump.h>
#include <swamp-dump/types.h>
#include <swamp-runtime/print.h>
#include <swamp-typeinfo/typeinfo.h>

void printTabs(FldOutStream* fp, int indentation)
{
    for (size_t i = 0; i < indentation; ++i) {
        fldOutStreamWriteOctets(fp, "    ", 4);
    }
}

void printNewLineWithTabs(FldOutStream* fp, int indentation)
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

void printWithColorf(FldOutStream* fp, int fg, const char* s, ...)
{
    va_list pl;

    fldOutStreamWritef(fp, "\033[%dm", fg);

    va_start(pl, s);
    fldOutStreamWritevf(fp, s, pl);
    va_end(pl);
}

static int typeIsSimple(const SwtiType* type)
{
    enum SwtiTypeValue v = type->type;
    return (v == SwtiTypeBoolean) || (v == SwtiTypeInt) || (v == SwtiTypeFixed) || (v == SwtiTypeString);
}

int swampDumpToAscii(const swamp_value* v, const SwtiType* type, int flags, int indentation, FldOutStream* fp)
{
    switch (type->type) {
        case SwtiTypeBoolean: {
            swamp_bool value = swamp_value_bool(v);
            printWithColorf(fp, 92, "%s", value ? "True" : "False");
        } break;
        case SwtiTypeInt: {
            swamp_int32 value = swamp_value_int(v);

            printWithColorf(fp, 91, "%d", value);
        } break;
        case SwtiTypeFixed: {
            swamp_int32 value = swamp_value_int(v);

            printWithColorf(fp, 91, "%f", value / 100.0f);
        } break;
        case SwtiTypeString: {
            const swamp_string* p = swamp_value_string(v);
            printWithColorf(fp, 91, "\"");
            printWithColorf(fp, 33, "%s", p->characters);
            printWithColorf(fp, 91, "\"");
        } break;
        case SwtiTypeRecord: {
            const swamp_struct* p = swamp_value_struct(v);
            const SwtiRecordType* record = (const SwtiRecordType*) type;
            if (p->info.field_count != record->fieldCount) {
                for (size_t i=0; i<record->fieldCount;++i) {
                    CLOG_SOFT_ERROR("  field:%s", record->fields[i].name);
                }
                swamp_value_print(p, "unknown struct");
                CLOG_ERROR("problem with field count %s %d", record->internal.name, p->info.field_count);
                return -2;
            }
            printWithColorf(fp, 94, "{ ", fp);
            for (size_t i = 0; i < record->fieldCount; i++) {
                const SwtiRecordTypeField* field = &record->fields[i];
                if (i > 0) {
                    if (!typeIsSimple(field->fieldType)) {
                        printNewLineWithTabs(fp, indentation);
                    }
                    printWithColorf(fp, 35, ", ");
                }
                printWithColorf(fp, 92, field->name);
                printWithColorf(fp, 32, " = ");
                int errorCode = swampDumpToAscii(p->fields[i], field->fieldType, flags, indentation + 1, fp);
                if (errorCode != 0) {
                    return errorCode;
                }
            }
            printWithColorf(fp, 94, " }");
        } break;
        case SwtiTypeArray: {
            const swamp_struct* p = swamp_value_struct(v);
            const SwtiArrayType* array = (const SwtiArrayType*) type;

            printWithColorf(fp, 94, "[| ", fp);
            for (size_t i = 0; i < p->info.field_count; i++) {
                if (i > 0) {
                    printNewLineWithTabs(fp, indentation);
                    printWithColorf(fp, 35, ", ");
                }
                int errorCode = swampDumpToAscii(p->fields[i], array->itemType, flags, indentation + 1, fp);
                if (errorCode != 0) {
                    return errorCode;
                }
            }
            printWithColorf(fp, 94, " |]");
        } break;
        case SwtiTypeTuple: {
            const swamp_struct* p = swamp_value_struct(v);
            const SwtiTupleType* tuple = (const SwtiTupleType*) type;

            if (p->info.field_count != tuple->parameterCount) {
                CLOG_SOFT_ERROR("mismatch tuple and struct types here");
                return -48;
            }
            printWithColorf(fp, 94, "( ", fp);
            for (size_t i = 0; i < p->info.field_count; i++) {
                if (i > 0) {
                    printNewLineWithTabs(fp, indentation);
                    printWithColorf(fp, 35, ", ");
                }
                int errorCode = swampDumpToAscii(p->fields[i], tuple->parameterTypes[i], flags, indentation + 1, fp);
                if (errorCode != 0) {
                    return errorCode;
                }
            }
            printWithColorf(fp, 94, " )");
        } break;
        case SwtiTypeList: {
            const swamp_list* p = swamp_value_list(v);
            const SwtiArrayType* array = (const SwtiArrayType*) type;

            printWithColorf(fp, 94, "[ ", fp);
            int i = 0;
            SWAMP_LIST_FOR_LOOP(p)
            if (i > 0) {
                printNewLineWithTabs(fp, indentation);
                printWithColorf(fp, 35, ", ");
            }
            int errorCode = swampDumpToAscii(value, array->itemType, flags, indentation + 1, fp);
            if (errorCode != 0) {
                return errorCode;
            }
            i++;
            SWAMP_LIST_FOR_LOOP_END()
            printWithColorf(fp, 94, " ]");
        } break;
        case SwtiTypeFunction:
            CLOG_SOFT_ERROR("can not dump functions")
            return -1;
        case SwtiTypeAlias: {
            const SwtiAliasType* alias = (const SwtiAliasType*) type;
            if (flags & swampDumpFlagAlias) {
                printWithColorf(fp, 92, alias->internal.name);
                printWithColorf(fp, 91, " => ");
            }
            int errorCode = swampDumpToAscii(v, alias->targetType, flags, indentation + 1, fp);
            if (errorCode != 0) {
                return errorCode;
            }
            break;
        }
        case SwtiTypeCustom: {
            const SwtiCustomType* custom = (const SwtiCustomType*) type;
            const swamp_enum* p = swamp_value_enum(v);
            const SwtiCustomTypeVariant* variant = &custom->variantTypes[p->enum_type];
            printWithColorf(fp, 92, custom->internal.name);
            printWithColorf(fp, 93, ":");
            printWithColorf(fp, 95, variant->name);
            for (size_t i = 0; i < variant->paramCount; ++i) {
                printWithColorf(fp, 91, " ");
                const SwtiType* paramType = variant->paramTypes[i];
                int errorCode = swampDumpToAscii(p->fields[i], paramType, flags, indentation + 1, fp);
                if (errorCode != 0) {
                    return errorCode;
                }
            }
            break;
        }
        case SwtiTypeBlob: {
            const swamp_blob* blob = swamp_value_blob(v);
            printWithColorf(fp, 91, "blob %d", blob->octet_count);

            if (flags & swampDumpFlagBlobExpanded) {
                size_t count = blob->octet_count > 2048 ? 2048 : blob->octet_count;
                if (flags & swampDumpFlagBlobAutoFormat) {
                    int allIsPrintable = 1;
                    for (size_t i=0; i < count;++i) {
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
                    for (size_t i=0; i<count;++i) {
                        if ((i % 64) == 0) {
                            printNewLineWithDots(fp, indentation+1);
                        }
                        printWithColorf(fp, 37, "%c", blob->octets[i]);
                    }
                } else {
                    for (size_t i=0; i<count;++i) {
                        if ((i % 32) == 0) {
                            printNewLineWithDots(fp, indentation+1);
                        }
                        printWithColorf(fp, 12, "%02X ", blob->octets[i]);
                    }
                }
            }
        }
    }

    return 0;
}

const char* swampDumpToAsciiString(const swamp_value* v, const SwtiType* type, int flags, char* target, size_t maxCount)
{
    FldOutStream outStream;

    if (maxCount < 64) {
        return 0;
    }

    fldOutStreamInit(&outStream, (uint8_t*) target, maxCount - 6); // reserve for zero

    int errorCode = swampDumpToAscii(v, type, flags, 0, &outStream);
    outStream.size = maxCount;
    fldOutStreamWritef(&outStream, "\033[0m");
    fldOutStreamWriteUInt8(&outStream, 0);
    if (errorCode != 0) {
        return 0;
    }

    return target;
}
