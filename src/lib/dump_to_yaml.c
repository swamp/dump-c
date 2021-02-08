/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Peter Bjorklund. All rights reserved.
 *  Licensed under the MIT License. See LICENSE in the project root for license information.
 *--------------------------------------------------------------------------------------------*/
#include <clog/clog.h>
#include <flood/out_stream.h>
#include <stdarg.h>
#include <swamp-dump/dump.h>
#include <swamp-dump/types.h>
#include <swamp-typeinfo/typeinfo.h>

static void printTabs(FldOutStream* fp, int indentation)
{
    for (size_t i = 0; i < indentation; ++i) {
        fldOutStreamWriteOctets(fp, "  ", 2);
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
        fldOutStreamWriteOctets(fp, "  ", 2);
    }
}

void printNewLineWithDots(FldOutStream* fp, int indentation)
{
    fldOutStreamWriteUInt8(fp, '\n');
    printDots(fp, indentation);
}

static void printWithColorf(FldOutStream* fp, int fg, const char* s, ...)
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
    return (v == SwtiTypeBoolean) || (v == SwtiTypeInt) || (v == SwtiTypeFixed) || (v == SwtiTypeString) || (v == SwtiTypeCustom) || (v == SwtiTypeBlob);
}

int swampDumpToYaml(const swamp_value* v, const SwtiType* type, int flags, int indentation, int alreadyindent, FldOutStream* fp)
{
    switch (type->type) {
        case SwtiTypeBoolean: {
            swamp_bool value = swamp_value_bool(v);
            printWithColorf(fp, 92, "%s\n", value ? "true" : "false");
        } break;
        case SwtiTypeInt: {
            swamp_int32 value = swamp_value_int(v);

            printWithColorf(fp, 91, "%d\n", value);
        } break;
        case SwtiTypeFixed: {
            swamp_int32 value = swamp_value_int(v);

            printWithColorf(fp, 91, "%f\n", value / 1000.0f);
        } break;
        case SwtiTypeString: {
            const swamp_string* p = swamp_value_string(v);
            printWithColorf(fp, 33, "%s\n", p->characters);
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
                printTabs(fp, i == 0 ? indentation-alreadyindent : indentation);
                printWithColorf(fp, 92, field->name);
                printWithColorf(fp, 32, ": ");
                if (!typeIsSimple(field->fieldType)) {
                    printNewLineWithTabs(fp, 0);
                } else {
                }
                int errorCode = swampDumpToYaml(p->fields[i], field->fieldType, flags, indentation + 1, alreadyindent, fp);
                if (errorCode != 0) {
                    return errorCode;
                }
            }
        } break;
        case SwtiTypeArray: {
            const swamp_struct* p = swamp_value_struct(v);
            const SwtiArrayType* array = (const SwtiArrayType*) type;

            printWithColorf(fp, 94, "\n", fp);
            for (size_t i = 0; i < p->info.field_count; i++) {
                printNewLineWithTabs(fp, indentation-alreadyindent);
                printWithColorf(fp, 35, "- ");
                int errorCode = swampDumpToYaml(p->fields[i], array->itemType, flags, indentation + 1, alreadyindent, fp);
                if (errorCode != 0) {
                    return errorCode;
                }
            }
        } break;
        case SwtiTypeList: {
            const swamp_list* p = swamp_value_list(v);
            const SwtiArrayType* array = (const SwtiArrayType*) type;

            int i = 0;
            SWAMP_LIST_FOR_LOOP(p)
                printTabs(fp, indentation-alreadyindent);
                printWithColorf(fp, 35, "- ");
                int errorCode = swampDumpToYaml(value, array->itemType, flags, indentation + 1, indentation + 1, fp);
                if (errorCode != 0) {
                    return errorCode;
                }
                i++;
            SWAMP_LIST_FOR_LOOP_END()
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
            int errorCode = swampDumpToYaml(v, alias->targetType, flags, indentation + 1, alreadyindent, fp);
            if (errorCode != 0) {
                return errorCode;
            }
            break;
        }
        case SwtiTypeCustom: {
            const SwtiCustomType* custom = (const SwtiCustomType*) type;
            const swamp_enum* p = swamp_value_enum(v);
            const SwtiCustomTypeVariant* variant = &custom->variantTypes[p->enum_type];
            printWithColorf(fp, 95, variant->name);
            for (size_t i = 0; i < variant->paramCount; ++i) {
                printWithColorf(fp, 91, " ");
                const SwtiType* paramType = variant->paramTypes[i];
                int errorCode = swampDumpToYaml(p->fields[i], paramType, flags, indentation + 1, alreadyindent, fp);
                if (errorCode != 0) {
                    return errorCode;
                }
            }
            printWithColorf(fp, 94, "\n");
            break;
        }
        case SwtiTypeBlob: {
            const swamp_blob* blob = swamp_value_blob(v);
            const char* chompModifier = ">@";
            if (flags & swampDumpFlagBlobAscii) {
                chompModifier = ">";
            }
            printWithColorf(fp, 91, chompModifier, blob->octet_count);

            if (flags & swampDumpFlagBlobExpanded) {
                size_t count = blob->octet_count > 2048 ? 2048 : blob->octet_count;
                if (flags & swampDumpFlagBlobAscii) {
                    for (size_t i=0; i<count;++i) {
                        if ((i % 64) == 0) {
                            printNewLineWithDots(fp, indentation);
                        }
                        printWithColorf(fp, 37, "%c", blob->octets[i]);
                    }
                } else {
                    for (size_t i=0; i<count;++i) {
                        if ((i % 32) == 0) {
                            printNewLineWithDots(fp, indentation);
                        }
                        printWithColorf(fp, 12, "%02X", blob->octets[i]);
                    }
                }
            }
        }
    }

    return 0;
}

const char* swampDumpToYamlString(const swamp_value* v, const SwtiType* type, int flags, char* target, size_t maxCount)
{
    FldOutStream outStream;

    fldOutStreamInit(&outStream, (uint8_t*) target, maxCount - 6); // reserve for zero

    fldOutStreamWritef(&outStream, "%YAML 1.2\n---\n");
    int errorCode = swampDumpToYaml(v, type, flags, 0, 0, &outStream);
    if (errorCode != 0) {
        return 0;
    }

    outStream.size = maxCount;
    fldOutStreamWritef(&outStream, "\033[0m");
    fldOutStreamWriteUInt8(&outStream, 0);

    return target;
}
