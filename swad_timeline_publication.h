// swad_timeline_publication.h: social timeline publications

#ifndef _SWAD_TL_PUB
#define _SWAD_TL_PUB
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

// Number of recent publishings got and shown the first time, before refreshing
#define TL_Pub_MAX_REC_PUBS_TO_GET_AND_SHOW	   10	// Recent publishings to show (first time)
#define TL_Pub_MAX_NEW_PUBS_TO_GET_AND_SHOW	10000	// New publishings retrieved (big number)
#define TL_Pub_MAX_OLD_PUBS_TO_GET_AND_SHOW	   20	// Old publishings are retrieved in packs of this size

/*****************************************************************************/
/******************************** Public types *******************************/
/*****************************************************************************/

#define TL_NUM_PUB_TYPES	4
// If the numbers assigned to each event type change,
// it is necessary to change old numbers to new ones in database table tl_notes
typedef enum
  {
   TL_PUB_UNKNOWN		= 0,
   TL_PUB_ORIGINAL_NOTE		= 1,
   TL_PUB_SHARED_NOTE		= 2,
   TL_Pub_COMMENT_TO_NOTE	= 3,
  } TL_Pub_PubType_t;

struct TL_Pub_Publication
  {
   long PubCod;
   long NotCod;
   long PublisherCod;		// Sharer or writer of the publication
   TL_Pub_PubType_t PubType;
   TL_TopMessage_t TopMessage;	// Used to show feedback on the action made
   struct TL_Pub_Publication *Next;	// Used for chained list
  };

/*****************************************************************************/
/****************************** Public prototypes ****************************/
/*****************************************************************************/

void TL_Pub_GetListPubsToShowInTimeline (struct TL_Timeline *Timeline);
void TL_Pub_FreeListPubs (struct TL_Timeline *Timeline);

void TL_Pub_InsertNewPubsInTimeline (struct TL_Timeline *Timeline);
void TL_Pub_ShowOldPubsInTimeline (struct TL_Timeline *Timeline);

void TL_Pub_PutLinkToViewNewPublications (void);
void TL_Pub_PutLinkToViewOldPublications (void);

void TL_Pub_PutHiddenParamPubCod (long PubCod);
long TL_Pub_GetParamPubCod (void);

void TL_Pub_CreateNotifToAuthor (long AuthorCod,long PubCod,
                                 Ntf_NotifyEvent_t NotifyEvent);

long TL_Pub_GetNotCodFromPubCod (long PubCod);

void TL_Pub_GetNotifPublication (char SummaryStr[Ntf_MAX_BYTES_SUMMARY + 1],
                                 char **ContentStr,
                                 long PubCod,bool GetContent);

void TL_Pub_PublishPubInTimeline (struct TL_Pub_Publication *Pub);

unsigned long TL_Pub_GetNumPubsUsr (long UsrCod);

#endif