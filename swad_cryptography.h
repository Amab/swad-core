// swad_cryptography.h: cryptography

#ifndef _SWAD_COM
#define _SWAD_COM
/*
    SWAD (Shared Workspace At a Distance in Spanish),
    is a web platform developed at the University of Granada (Spain),
    and used to support university teaching.

    This file is part of SWAD core.
    Copyright (C) 1999-2021 Antonio Ca�as Vargas

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU Affero General Public License as
    published by the Free Software Foundation, either version 3 of the
    License, or (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Affero General Public License for more details.

    You should have received a copy of the GNU Affero General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
/*****************************************************************************/
/***************************** Public constants ******************************/
/*****************************************************************************/

#define Cry_BYTES_ENCRYPTED_STR_SHA256_BASE64	43	// 42,66 base-64 characters store 256 bits
#define Cry_LENGTH_ENCRYPTED_STR_SHA512_BASE64	86	// 85,33 base-64 characters store 512 bits

/*****************************************************************************/
/********************************* Headers ***********************************/
/*****************************************************************************/

#include <stdio.h>	// For FILE

#include "sha2/sha2.h"		// For sha-256 and sha-512 algorithms

/*****************************************************************************/
/***************************** Public prototypes *****************************/
/*****************************************************************************/

void Cry_EncryptSHA256Base64 (const char *PlainText,
                              char EncryptedText[Cry_BYTES_ENCRYPTED_STR_SHA256_BASE64 + 1]);
void Cry_EncryptSHA512Base64 (const char *PlainText,
                              char EncryptedText[Cry_LENGTH_ENCRYPTED_STR_SHA512_BASE64 + 1]);

void Cry_CreateUniqueNameEncrypted (char UniqueNameEncrypted[Cry_BYTES_ENCRYPTED_STR_SHA256_BASE64 + 1]);

#endif
