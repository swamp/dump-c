/*---------------------------------------------------------------------------------------------
*  Copyright (c) Peter Bjorklund. All rights reserved.
*  Licensed under the MIT License. See LICENSE in the project root for license information.
*--------------------------------------------------------------------------------------------*/
#include <clog/clog.h>
#include <flood/in_stream.h>
#include <swamp-dump/dump.h>
#include <swamp-typeinfo/typeinfo.h>
#include <tiny-libc/tiny_libc.h>
#include <swamp-runtime/dynamic_memory.h>
#include <swamp-runtime/swamp_allocate.h>
#include <swamp-runtime/context.h>

static int swampDumpFromOctetsHelper(FldInStream* inStream, const SwtiType* tiType,
                             unmanagedTypeCreator creator, void* context, void* target, SwampDynamicMemory* memory, SwampUnmanagedMemory* targetUnmanagedMemory)
{
   switch (tiType->type) {
       case SwtiTypeInt: {
           fldInStreamReadInt32(inStream, (SwampInt32*) target);
       } break;

       case SwtiTypeBoolean: {
           tc_memcpy_octets(target, inStream->p, sizeof(SwampBool));
           inStream->p += sizeof(SwampBool);
           inStream->pos += sizeof(SwampBool);
       } break;

       case SwtiTypeString: {
           uint8_t stringLengthIncludingTerminator;
           fldInStreamReadUInt8(inStream, &stringLengthIncludingTerminator);

           const SwampString* newString = swampStringAllocateWithSize(memory, (const char*)inStream->p, stringLengthIncludingTerminator-1);
           *(const SwampString**)target = newString;
           inStream->p += stringLengthIncludingTerminator;
           inStream->pos += stringLengthIncludingTerminator;
           break;
       }

       case SwtiTypeRecord: {
           const SwtiRecordType* recordType = (const SwtiRecordType*) tiType;
           for (size_t i = 0; i < recordType->fieldCount; ++i) {
               const SwtiRecordTypeField* field = &recordType->fields[i];
               swampDumpFromOctetsHelper(inStream, field->fieldType, creator, context, (uint8_t *)target + field->memoryOffsetInfo.memoryOffset, memory, targetUnmanagedMemory);
           }
           break;
       }

       case SwtiTypeTuple: {
           const SwtiTupleType* tupleType = (const SwtiTupleType*) tiType;
           for (size_t i = 0; i < tupleType->fieldCount; ++i) {
               const SwtiTupleTypeField* field = &tupleType->fields[i];
               swampDumpFromOctetsHelper(inStream, field->fieldType, creator, context, (uint8_t *)target + field->memoryOffsetInfo.memoryOffset, memory, targetUnmanagedMemory);
           }
           break;
       }

       case SwtiTypeCustom: {
           const SwtiCustomType* custom = (const SwtiCustomType*) tiType;
           uint8_t enumIndex;
           fldInStreamReadUInt8(inStream, &enumIndex);
           const SwtiCustomTypeVariant* variant = &custom->variantTypes[enumIndex];
           *(uint8_t*) target = enumIndex;
           for (size_t i = 0; i < variant->paramCount; ++i) {
               const SwtiCustomTypeVariantField* field = &variant->fields[i];
               swampDumpFromOctetsHelper(inStream, field->fieldType, creator, context, (uint8_t *)target + field->memoryOffsetInfo.memoryOffset, memory, targetUnmanagedMemory);
           }
           break;
       }

       case SwtiTypeArray: {
           const SwtiArrayType* arrayType = (const SwtiArrayType*) tiType;
           uint8_t arrayLength;
           fldInStreamReadUInt8(inStream, &arrayLength);
           SwampArray* array = swampArrayAllocatePrepare(memory, arrayLength, arrayType->memoryInfo.memorySize, arrayType->memoryInfo.memoryAlign);
           for (size_t i = 0; i < arrayLength; ++i) {
               swampDumpFromOctetsHelper(inStream, arrayType->itemType, creator, context, (uint8_t *)array->value + i * array->itemSize, memory, targetUnmanagedMemory);
           }

           *(const SwampArray**) target = array;
       }            break;


       case SwtiTypeList: {
           const SwtiListType* listType = (const SwtiListType*) tiType;
           uint8_t listLength;
           fldInStreamReadUInt8(inStream, &listLength);
           SwampList* list = swampListAllocatePrepare(memory, listLength, listType->memoryInfo.memorySize, listType->memoryInfo.memoryAlign);
           for (size_t i = 0; i < listLength; ++i) {
               swampDumpFromOctetsHelper(inStream, listType->itemType, creator, context, (uint8_t *)list->value + i * list->itemSize, memory, targetUnmanagedMemory);
           }

           *(const SwampList**) target = list;

       } break;

       case SwtiTypeAlias: {
           const SwtiAliasType* alias = (const SwtiAliasType*) tiType;
           return swampDumpFromOctetsHelper(inStream, alias->targetType, creator, context, target, memory, targetUnmanagedMemory);
       }
       case SwtiTypeFunction: {
           CLOG_SOFT_ERROR("functions can not be serialized")
           return -1;
       }
       case SwtiTypeBlob: {
           uint32_t octetCount;
           int errorCode = fldInStreamReadUInt32(inStream, &octetCount);
           if (errorCode < 0) {
               return errorCode;
           }
           SwampBlob* newBlob = swampBlobAllocate(memory, octetCount == 0 ? 0 : inStream->p, octetCount);
           inStream->p += octetCount;
           inStream->pos += octetCount;
           *(const SwampBlob**) target = newBlob;
           break;
       }
       case SwtiTypeUnmanaged: {
           const SwtiUnmanagedType* unmanagedType = (const SwtiUnmanagedType*) tiType;
           if (creator == 0) {
               CLOG_ERROR("tried to deserialize unmanaged '%s', but no creator was provided", unmanagedType->internal.name)
               return -2;
           }

           SwampUnmanaged* unmanagedValue = swampUnmanagedMemoryAllocate(targetUnmanagedMemory, unmanagedType->internal.name);
           //swampUnmanagedMemoryAdd(targetUnmanagedMemory, unmanagedValue);
           creator(context, unmanagedType, unmanagedValue);
           int errorCode = unmanagedValue->deSerialize(unmanagedValue->ptr, inStream->p, inStream->size - inStream->pos);
           if (errorCode < 0) {
               CLOG_SOFT_ERROR("could not deserialize unmanaged type %s %d", unmanagedType->internal.name, errorCode)
               return errorCode;
           }
           inStream->p += errorCode;
           inStream->pos += errorCode;
           *(const SwampUnmanaged**)target = unmanagedValue;
           break;
       }
       default:
           CLOG_ERROR("can not deserialize dump from type %d", tiType->type)
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
       CLOG_SOFT_ERROR("swamp-dump: wrong version %d.%d.%d", major, minor, patch)
       return -1;
   }

   return 0;
}

int swampDumpFromOctets(FldInStream* inStream, const SwtiType* tiType,
                       unmanagedTypeCreator creator, void* context, void* target, SwampDynamicMemory* memory, SwampUnmanagedMemory* targetUnmanagedMemory)
{
   int error;
   if ((error = readVersion(inStream)) < 0) {
       return error;
   }

   return swampDumpFromOctetsHelper(inStream, tiType, creator, context, target, memory, targetUnmanagedMemory);
}

int swampDumpFromOctetsRaw(FldInStream* inStream, const SwtiType* tiType,
                          unmanagedTypeCreator creator, void* context, void* target, SwampDynamicMemory* memory, SwampUnmanagedMemory* targetUnmanagedMemory)
{
   return swampDumpFromOctetsHelper(inStream, tiType, creator, context, target, memory, targetUnmanagedMemory);
}
