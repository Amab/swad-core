// swad_timeline.h: social timeline

#ifndef _SWAD_TML_COM
#define _SWAD_TML_COM
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

#include "swad_timeline_note.h"
#include "swad_timeline_post.h"

/*****************************************************************************/
/****************************** Public constants *****************************/
/*****************************************************************************/

/*****************************************************************************/
/******************************** Public types *******************************/
/*****************************************************************************/

struct Tml_Com_Comment
  {
   long PubCod;			// Unique code/identifier for each publication
   long UsrCod;			// Publisher
   long NotCod;			// Note code to which this comment belongs
   time_t DateTimeUTC;		// Date-time of publication in UTC time
   unsigned NumFavs;		// Number of times (users) this comment has been favourited
   struct Tml_Pst_Content Content;
  };

/*****************************************************************************/
/****************************** Public prototypes ****************************/
/*****************************************************************************/

void Tml_Com_PutIconToToggleComm (const char UniqueId[Frm_MAX_BYTES_ID + 1]);
void Tml_Com_PutIconCommDisabled (void);

void Tml_Com_PutPhotoAndFormToWriteNewComm (const struct Tml_Timeline *Timeline,
	                                    long NotCod,
                                            const char IdNewComm[Frm_MAX_BYTES_ID + 1]);

void Tml_Com_WriteCommsInNote (const struct Tml_Timeline *Timeline,
			       const struct Tml_Not_Note *Not);

void Tml_Com_ShowHiddenCommsUsr (void);
void Tml_Com_ShowHiddenCommsGbl (void);

void Tml_Com_ReceiveCommUsr (void);
void Tml_Com_ReceiveCommGbl (void);

void Tml_Com_RequestRemComUsr (void);
void Tml_Com_RequestRemComGbl (void);
void Tml_Com_RemoveComUsr (void);
void Tml_Com_RemoveComGbl (void);

void Tml_Com_RemoveCommMediaAndDBEntries (long PubCod);

void Tml_Com_GetDataOfCommByCod (struct Tml_Com_Comment *Com);

#endif
