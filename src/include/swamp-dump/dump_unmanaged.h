/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Peter Bjorklund. All rights reserved.
 *  Licensed under the MIT License. See LICENSE in the project root for license information.
 *--------------------------------------------------------------------------------------------*/
#ifndef SWAMP_DUMP_DUMP_UNMANAGED_H
#define SWAMP_DUMP_DUMP_UNMANAGED_H

struct SwtiUnmanagedType;
struct SwampUnmanaged;

typedef const void* (*unmanagedTypeCreator)(void* context, const struct SwtiUnmanagedType* type, struct SwampUnmanaged* target);

#endif
