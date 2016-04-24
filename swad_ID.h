// swad_ID.h: Users' IDs

#ifndef _SWAD_ID
#define _SWAD_ID
/*
    SWAD (Shared Workspace At a Distance in Spanish),
    is a web platform developed at the University of Granada (Spain),
    and used to support university teaching.

    This file is part of SWAD core.
    Copyright (C) 1999-2016 Antonio Ca�as Vargas

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
/********************************* Headers ***********************************/
/*****************************************************************************/

#include "swad_user.h"

/*****************************************************************************/
/************************* Public types and constants ************************/
/*****************************************************************************/

#define ID_MIN_DIGITS_AUTOMATIC_DETECT_USR_ID	 6	// Used for detect automatically users' IDs inside a text
#define ID_MIN_DIGITS_USR_ID			 1	// A user's ID must have at least these digits
#define ID_MIN_LENGTH_USR_ID			 5	// This number MUST be >= 3
#define ID_MAX_LENGTH_USR_ID			16

#define ID_MAX_BYTES_LIST_USRS_IDS	(ID_MAX_LENGTH_USR_ID*Cfg_MAX_USRS_IN_LIST*10)

struct ListIDs
  {
   bool Confirmed;
   char ID[ID_MAX_LENGTH_USR_ID+1];
  };

/*****************************************************************************/
/***************************** Public prototypes *****************************/
/*****************************************************************************/

void ID_GetListIDsFromUsrCod (struct UsrData *UsrDat);
void ID_ReallocateListIDs (struct UsrData *UsrDat,unsigned NumIDs);
void ID_FreeListIDs (struct UsrData *UsrDat);
unsigned ID_GetListUsrCodsFromUsrID (struct UsrData *UsrDat,
                                     const char *EncryptedPassword,	// If NULL or empty ==> do not check password
                                     struct ListUsrCods *ListUsrCods,
                                     bool OnlyConfirmedIDs);

void ID_PutParamOtherUsrIDPlain (void);
void ID_GetParamOtherUsrIDPlain (void);

bool ID_CheckIfUsrIDIsValid (const char *UsrID);
bool ID_CheckIfUsrIDSeemsAValidID (const char *UsrID);

void ID_WriteUsrIDs (struct UsrData *UsrDat);

void ID_PutLinkToChangeUsrIDs (void);
void ID_ShowFormOthIDs (void);
void ID_ShowFormChangeUsrID (const struct UsrData *UsrDat,bool ItsMe);
void ID_RemoveMyUsrID (void);
void ID_RemoveOtherUsrID (void);
void ID_NewMyUsrID (void);
void ID_NewOtherUsrID (void);

void ID_RequestConfirmOtherUsrID (void);
void ID_ConfirmOtherUsrID (void);
void ID_ConfirmUsrID (long UsrCod,const char *UsrID);

#endif
