// swad_timeline_user.h: social timeline users

#ifndef _SWAD_TML_USR
#define _SWAD_TML_USR
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
/********************************** Headers **********************************/
/*****************************************************************************/

#include "swad_form.h"
#include "swad_media.h"
#include "swad_notification.h"
#include "swad_user.h"

/*****************************************************************************/
/****************************** Public constants *****************************/
/*****************************************************************************/

#define Tml_Usr_DEF_USRS_SHOWN	5	// Default maximum number of users shown who have share/fav a note
#define Tml_Usr_MAX_USRS_SHOWN	1000	// Top     maximum number of users shown who have share/fav a note

/*****************************************************************************/
/******************************** Public types *******************************/
/*****************************************************************************/

typedef enum
  {
   Tml_Usr_TIMELINE_USR,		// Show the timeline of a user
   Tml_Usr_TIMELINE_GBL,		// Show the timeline of the users follwed by me
  } Tml_Usr_UsrOrGbl_t;

typedef enum
  {
   Tml_Usr_SHOW_FEW_USRS,	// Show a few first favers/sharers
   Tml_Usr_SHOW_ALL_USRS,	// Show all favers/sharers
  } Tml_Usr_HowManyUsrs_t;

#define Tml_Usr_NUM_FAV_SHA 3
typedef enum
  {
   Tml_Usr_FAV_UNF_NOTE = 0,
   Tml_Usr_FAV_UNF_COMM = 1,
   Tml_Usr_SHA_UNS_NOTE = 2,
  } Tml_Usr_FavSha_t;

/*****************************************************************************/
/****************************** Public prototypes ****************************/
/*****************************************************************************/

void Tml_Usr_RemoveUsrContent (long UsrCod);

bool Tml_Usr_CheckIfFavedSharedByUsr (Tml_Usr_FavSha_t FavSha,long Cod,long UsrCod);

void Tml_Usr_PutIconFavSha (Tml_Usr_FavSha_t FavSha,
                            long Cod,long UsrCod,unsigned NumUsrs,
                            Tml_Usr_HowManyUsrs_t HowManyUsrs);

bool Tml_Usr_CheckIfICanFavSha (long Cod,long UsrCod);
bool Tml_Usr_CheckIfICanRemove (long Cod,long UsrCod);

#endif
