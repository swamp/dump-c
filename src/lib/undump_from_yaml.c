/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Peter Bjorklund. All rights reserved.
 *  Licensed under the MIT License. See LICENSE in the project root for license information.
 *--------------------------------------------------------------------------------------------*/
#include <swamp-dump/dump_yaml.h>

#include <clog/clog.h>
#include <flood/in_stream.h>
#include <flood/text_in_stream.h>
#include <swamp-runtime/types.h>
#include <swamp-typeinfo/typeinfo.h>

static int detectIndentation(FldTextInStream* inStream)
{
    char ch;
    int column;
    while (1) {
        if (inStream->inStream->pos == inStream->inStream->size) {
            column = 0;
            break;
        }
        int error = fldTextInStreamReadCh(inStream, &ch);
        if (error < 0) {
            return error;
        }
        if (ch == 10) {
        } else if (ch == 32) {
        } else if (ch == 13) {
            // Just ignore strange CR characters
        } else {
            fldTextInStreamUnreadCh(inStream);
            column = inStream->column;
            break;
        }
    }

    if ((column % 2) != 0) {
        CLOG_SOFT_ERROR("not a proper indentation column:%d '%c'", column + 1, *inStream->inStream->p);
        return -4;
    }

    return column / 2;
}

int requireIndentation(FldTextInStream* inStream, int requiredIndentation)
{
    int indentation = detectIndentation(inStream);
    if (indentation < 0) {
        return indentation;
    }

    if (indentation != requiredIndentation) {
        CLOG_SOFT_ERROR("%s:unexpected indentation.  required %d but encountered %d", fldTextInStreamPositionString(inStream), requiredIndentation, indentation)
        return -4;
    }

    return 0;
}

int isAlpha(char c)
{
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

int skipLeadingSpaces(FldTextInStream* inStream)
{
    char ch;
    int spacesSkipped = 0;

    while (1) {
        int error = fldTextInStreamReadCh(inStream, &ch);
        if (error < 0) {
            return error;
        }
        if (ch == 32) {
            spacesSkipped++;
        } else {
            fldTextInStreamUnreadCh(inStream);
            break;
        }
    }

    return spacesSkipped;
}

int readVariableIdentifier(FldTextInStream* inStream, const char** fieldName)
{
    char ch;
    int charsFound = 0;
    static char name[128];

    while (1) {
        int error = fldTextInStreamReadCh(inStream, &ch);
        if (error < 0) {
            return error;
        }
        if (isAlpha(ch)) {
            name[charsFound++] = ch;
        } else {
            fldTextInStreamUnreadCh(inStream);
            break;
        }
    }
    name[charsFound] = 0;
    *fieldName = name;

    return charsFound;
}

int readTypeIdentifierValue(FldTextInStream* inStream, const char** fieldName)
{
    int skipErr = skipLeadingSpaces(inStream);
    if (skipErr < 0) {
        return skipErr;
    }

    return readVariableIdentifier(inStream, fieldName);
}

int skipWhitespaceAndEndOfLine(FldTextInStream* inStream)
{
    int errorCode = skipLeadingSpaces(inStream);
    if (errorCode < 0) {
        return errorCode;
    }

    uint8_t ch;
    int error = fldTextInStreamReadCh(inStream, &ch);
    if (error < 0) {
        return error;
    }

    if (ch != 10) {
        return -5;
    }

    return 0;
}


typedef enum Marker {
    MarkerNone,
    MarkerHex,
    MarkerAscii,
} Marker;

int skipWhitespaceAndBreakMarkerEndOfLine(FldTextInStream* inStream, int* marker)
{
    int errorCode = skipLeadingSpaces(inStream);
    if (errorCode < 0) {
        return errorCode;
    }

    char ch;
    int error = fldTextInStreamReadCh(inStream, &ch);
    if (error < 0) {
        return error;
    }

    if (ch == 10) {
        *marker = MarkerNone;
        return 0;
    } else if (ch == '>') {
        char ch2;

        int error = fldTextInStreamReadCh(inStream, &ch2);
        if (error < 0) {
            return error;
        }
        if (ch2 == 'x') {
            *marker = MarkerHex;
        }
        *marker = MarkerAscii;
        if (ch2 == 10) {
            return 0;
        }
    }

    return skipWhitespaceAndEndOfLine(inStream);
}

static int readBoolean(FldTextInStream* inStream)
{
    const char* foundName;

    int skipError = skipLeadingSpaces(inStream);
    if (skipError < 0) {
        return skipError;
    }
    int errorCode = readVariableIdentifier(inStream, &foundName);
    if (errorCode < 0) {
        return errorCode;
    }

    if (tc_str_equal(foundName, "true")) {
        return 1;
    } else if (tc_str_equal(foundName, "false")) {
        return 0;
    }

    CLOG_SOFT_ERROR("expected a boolean, but received '%s'", foundName);

    return -5;
}

static int readStringUntilEndOfLine(FldTextInStream* inStream, const char** foundString)
{
    char ch;
    int charsFound = 0;
    static char name[128];

    while (1) {
        int error = fldTextInStreamReadCh(inStream, &ch);
        if (error < 0) {
            return error;
        }
        if (ch == 10) {
            break;
        } else if (ch == 13) {
            // Windows support
            continue;
        } else {
            name[charsFound++] = ch;
        } 
    }
    name[charsFound] = 0;
    *foundString = name;

    return charsFound;
}

int readBlob(FldTextInStream* inStream, int indentation, SwampBlob** out)
{
    FldTextInStreamState save;

    uint8_t* buf = tc_malloc(16 * 1024);
    uint8_t* p = buf;

    while (1) {
        fldTextInStreamTell(inStream, &save);
        int detectedIndentation = detectIndentation(inStream);
        if (detectedIndentation < 0) {
            fldTextInStreamSeek(inStream, &save);
            return detectedIndentation;
        }
        if (detectedIndentation != indentation) {
            fldTextInStreamSeek(inStream, &save);
            break;
        }

        const char* foundString;
        int charCount = readStringUntilEndOfLine(inStream, &foundString);
        if (charCount < 0) {
            return charCount;
        }

        for (size_t i = 0; i < charCount; ++i) {
            *p++ = foundString[i];
        }
    }

    *out = 0; //swamp_allocator_alloc_blob(allocator, buf, p - buf, 0);

    tc_free(buf);

    return 0;
}

static int readStringValue(FldTextInStream* inStream, const char** foundString)
{
    int errorCode = skipLeadingSpaces(inStream);
    if (errorCode < 0) {
        return errorCode;
    }

    return readStringUntilEndOfLine(inStream, foundString);
}

static int checkListContinuation(FldTextInStream* inStream, int indentation)
{
    FldTextInStreamState save;

    fldTextInStreamTell(inStream, &save);

    int detectedIndentation = detectIndentation(inStream);
    if (detectedIndentation < 0) {
        CLOG_WARN("list wasn't continued because indentation returned error");
        fldTextInStreamSeek(inStream, &save);
        return detectedIndentation;
    }

    if (detectedIndentation != indentation) {
        fldTextInStreamSeek(inStream, &save);
        return 0;
    }

    uint8_t ch;
    int error = fldTextInStreamReadCh(inStream, &ch);
    if (error < 0) {
        fldTextInStreamSeek(inStream, &save);
        return 0;
    }

    if (ch != '-') {
        return -4;
    }

    error = fldTextInStreamReadCh(inStream, &ch);
    if (error < 0) {
        return error;
    }

    if (ch != 32) {
        return -4;
    }

    return 1;
}

static int readIntegerValue(FldTextInStream* inStream, int32_t* v)
{
    int errorCode = skipLeadingSpaces(inStream);
    if (errorCode < 0) {
        return errorCode;
    }

    const char* foundString;
    errorCode = readStringUntilEndOfLine(inStream, &foundString);
    if (errorCode < 0) {
        return errorCode;
    }

    *v = atoi(foundString);

    return 0;
}

int readFieldNameColonWithIndentation(FldTextInStream* inStream, int requiredIndentation, const char** fieldName)
{
    int errorCode = requireIndentation(inStream, requiredIndentation);
    if (errorCode < 0) {
        CLOG_SOFT_ERROR("couldn't read fieldname and colon because indentation is wrong");
        return errorCode;
    }

    int charactersRead = readVariableIdentifier(inStream, fieldName);
    if (charactersRead < 0) {
        return charactersRead;
    }

    uint8_t ch;
    int error = fldTextInStreamReadCh(inStream, &ch);
    if (error < 0) {
        return error;
    }

    if (ch != ':') {
        CLOG_SOFT_ERROR("expected colon");
        return -6;
    }

    return 0;
}

int swampDumpFromYamlHelper(FldTextInStream* inStream, int indentation, struct swamp_allocator* allocator,
                            const SwtiType* tiType, const void** out)
{
    /*
    switch (tiType->type) {
        case SwtiTypeInt: {
            int32_t v;
            int errorCode = readIntegerValue(inStream, &v);
            if (errorCode < 0) {
                return errorCode;
            }
            *out = swamp_allocator_alloc_integer(allocator, v);
        } break;
        case SwtiTypeBoolean: {
            int truth = readBoolean(inStream);
            if (truth < 0) {
                return truth;
            }
            *out = swamp_allocator_alloc_boolean(allocator, truth);
        } break;
        case SwtiTypeString: {
            const char* characters;
            int errorCode = readStringValue(inStream, &characters);
            if (errorCode < 0) {
                return errorCode;
            }
            *out = swamp_allocator_alloc_string(allocator, (const char*) characters);
            break;
        }
        case SwtiTypeRecord: {
            const SwtiRecordType* record = (const SwtiRecordType*) tiType;
            swamp_struct* temp = (swamp_struct*) swamp_allocator_alloc_struct(allocator, record->fieldCount);
            for (size_t i = 0; i < record->fieldCount; ++i) {
                const SwtiRecordTypeField* field = &record->fields[i];
                const char* foundName;
                int errorCode = readFieldNameColonWithIndentation(inStream, indentation, &foundName);
                if (errorCode < 0) {
                    CLOG_SOFT_ERROR("couldn't read field '%s' encountered '%s' errorCode:%d", field->name, foundName,
                                    errorCode);
                    return errorCode;
                }
                if (!tc_str_equal(foundName, field->name)) {
                    CLOG_SOFT_ERROR("field name mismatch. Expected '%s' but got '%s'", field->name, foundName);
                    return -6;
                }
                int subIndentation = indentation;
                const SwtiType* unaliasedType = swtiUnalias(field->fieldType);
                if ((unaliasedType->type == SwtiTypeRecord) || (unaliasedType->type == SwtiTypeList) ||
                    (unaliasedType->type == SwtiTypeBlob)) {
                    Marker marker;
                    int skipError = skipWhitespaceAndBreakMarkerEndOfLine(inStream, &marker);
                    if (skipError < 0) {
                        CLOG_SOFT_ERROR("this wasn't a end of line")
                        return skipError;
                    }
                    subIndentation++;
                }
                int resultCode = swampDumpFromYamlHelper(inStream, subIndentation, allocator, field->fieldType,
                                                         &temp->fields[i]);
                if (resultCode < 0) {
                    CLOG_SOFT_ERROR("couldn't read value for field %s' (%d)", field->name, resultCode);
                    return resultCode;
                }
            }
            *out = (const swamp_value*) temp;
            break;
        }
        case SwtiTypeArray: {
            const SwtiArrayType* array = (const SwtiArrayType*) tiType;
            uint8_t arrayLength;
            swamp_struct* temp = (swamp_struct*) swamp_allocator_alloc_struct(allocator, arrayLength);
            for (size_t i = 0; i < arrayLength; ++i) {
                swampDumpFromYamlHelper(inStream, indentation, allocator, array->itemType, &temp->fields[i]);
            }
            *out = (const swamp_value*) temp;
            break;
        }
        case SwtiTypeList: {
            const SwtiListType* list = (const SwtiListType*) tiType;
            uint8_t listLength = 0;
            const int maxListLength = 256;

            const swamp_value** targetValues = tc_malloc_type_count(swamp_value*, maxListLength);
            while (1) {
                int didContinue = checkListContinuation(inStream, indentation);
                if (didContinue < 0) {
                    return didContinue;
                }
                if (!didContinue) {
                    break;
                }
                int errorCode = swampDumpFromYamlHelper(inStream, indentation + 1, allocator, list->itemType,
                                                        &targetValues[listLength]);
                if (errorCode < 0) {
                    CLOG_SOFT_ERROR("couldn't read list item %d", listLength);
                    return errorCode;
                }
                listLength++;
            }

            swamp_struct* temp = swamp_allocator_alloc_list_create(allocator, targetValues, listLength);
            tc_free(targetValues);
            *out = (const swamp_value*) temp;
            break;
        }
        case SwtiTypeCustom: {
            const SwtiCustomType* custom = (const SwtiCustomType*) tiType;
            const char* foundVariantName;
            int errCode = readTypeIdentifierValue(inStream, &foundVariantName);
            if (errCode < 0) {
                return errCode;
            }

            int enumIndex = -1;
            for (size_t i = 0; i < custom->variantCount; ++i) {
                const SwtiCustomTypeVariant* variant = &custom->variantTypes[i];
                if (tc_str_equal(variant->name, foundVariantName)) {
                    enumIndex = i;
                    break;
                }
            }
            if (enumIndex < 0) {
                return -4;
            }

            const SwtiCustomTypeVariant* variant = &custom->variantTypes[enumIndex];
            swamp_enum* newEnum = (swamp_enum*) swamp_allocator_alloc_enum(allocator, enumIndex, variant->paramCount);
            for (size_t i = 0; i < variant->paramCount; ++i) {
                const SwtiType* paramType = variant->paramTypes[i];
                swampDumpFromYamlHelper(inStream, indentation, allocator, paramType, &newEnum->fields[i]);
            }
            *out = (const swamp_value*) newEnum;
            break;
        }
        case SwtiTypeAlias: {
            const SwtiAliasType* alias = (const SwtiAliasType*) tiType;
            return swampDumpFromYamlHelper(inStream, indentation, allocator, alias->targetType, out);
        }
        case SwtiTypeFunction: {
            CLOG_SOFT_ERROR("functions can not be serialized");
            return -1;
        }
        case SwtiTypeResourceName: {
            CLOG_SOFT_ERROR("resource names can not be serialized");
            return -1;
        }
        case SwtiTypeBlob: {
            const swamp_blob* blob;
            int errorCode = readBlob(inStream, indentation, allocator, &blob);
            if (errorCode < 0) {
                return errorCode;
            }
            *out = blob;
            break;
        }
        default:
            CLOG_ERROR("can not deserialize dump from type %d", tiType->type);
            return -1;
            break;
    }

     */

    return 0;
}

int swampDumpFromYaml(FldInStream* inStream, const SwtiType* tiType,
                      const void** out)
{
    FldTextInStream textStream;
    fldTextInStreamInit(&textStream, inStream);

    if (*inStream->p == '%') {
        const char* foundString;
        readStringUntilEndOfLine(&textStream, &foundString);
        if (!tc_str_equal("%YAML 1.2", foundString)) {
            return -2;
        }
        readStringUntilEndOfLine(&textStream, &foundString);
        if (!tc_str_equal("---", foundString)) {
            return -2;
        }
    }

    return swampDumpFromYamlHelper(&textStream, 0, 0, tiType, out);
}
