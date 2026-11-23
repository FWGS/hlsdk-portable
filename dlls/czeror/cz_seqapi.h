/*
cz_seqapi.h - the sequence engine API for engines that only stub it
Copyright (C) 2026 sergiopoverony
*/
#pragma once
#if !defined( CZ_SEQAPI_H )
#define CZ_SEQAPI_H

#include <stddef.h>

struct sequenceEntry_;

// the engine's entry if it has one, else the entry built from sequences/*.seq
struct sequenceEntry_ *CZ_SequenceGet( const char *fileName, const char *entryName );

// a sentence of a "!sentences" group of the .seq files: writes the sound path to found,
// returns the picked index or -1 if there's no such group
int CZ_SequencePickSentence( const char *groupName, char *found, size_t size );

void CZ_SequencePrecacheGroup( const char *groupName );

// forget the built entries, each start of a map (also of the same one) builds and
// precaches them again
void CZ_SequenceReset( void );

#endif // CZ_SEQAPI_H
