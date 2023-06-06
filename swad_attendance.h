// swad_attendance.h: control of attendance

#ifndef _SWAD_ATT
#define _SWAD_ATT
/*
    SWAD (Shared Workspace At a Distance),
    is a web platform developed at the University of Granada (Spain),
    and used to support university teaching.

    This file is part of SWAD core.
    Copyright (C) 1999-2023 Antonio Ca�as Vargas

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

#include "swad_date.h"
#include "swad_user.h"

/*****************************************************************************/
/************************** Public types and constants ***********************/
/*****************************************************************************/

#define Att_MAX_CHARS_ATTENDANCE_EVENT_TITLE	(128 - 1)	// 127
#define Att_MAX_BYTES_ATTENDANCE_EVENT_TITLE	((Att_MAX_CHARS_ATTENDANCE_EVENT_TITLE + 1) * Str_MAX_BYTES_PER_CHAR - 1)	// 2047

#define Att_NUM_ORDERS_NEWEST_OLDEST 2
typedef enum
  {
   Att_NEWEST_FIRST,
   Att_OLDEST_FIRST,
  } Att_OrderNewestOldest_t;

#define Att_ORDER_DEFAULT Dat_STR_TIME

struct Att_Event
  {
   /* Fields stored in database */
   long AttCod;
   long CrsCod;
   HidVis_HiddenOrVisible_t HiddenOrVisible;
   long UsrCod;
   time_t TimeUTC[Dat_NUM_START_END_TIME];
   bool Open;
   bool CommentTchVisible;
   char Title[Att_MAX_BYTES_ATTENDANCE_EVENT_TITLE + 1];

   /* Field computed, not associated to the event in database */
   unsigned NumStdsTotal;	// Number total of students who have assisted to the event
  };

struct Att_Events
  {
   bool LstIsRead;		// Is the list already read from database, or it needs to be read?
   unsigned Num;		// Number of attendance events
   struct
     {
      long AttCod;		// Attendance event code
      unsigned NumStdsFromList;	// Number of students (taken from a list) who has assisted to the event
      bool Selected;		// I have selected this attendance event
     } *Lst;			// List of attendance events
   Dat_StartEndTime_t SelectedOrder;
   struct Att_Event Event;	// Selected/current event
   bool ShowDetails;
   char *StrAttCodsSelected;
   unsigned CurrentPage;
  };

/*****************************************************************************/
/***************************** Public prototypes *****************************/
/*****************************************************************************/

void Att_SeeEvents (void);

bool Att_CheckIfICanEditEvents (void);

void Att_ReqCreatOrEditEvent (void);
bool Att_GetEventDataByCod (struct Att_Event *Event);
void Att_GetEventDataFromRow (MYSQL_ROW row,struct Att_Event *Event);

void Att_AskRemEvent (void);
void Att_GetAndRemEvent (void);
void Att_RemoveEventFromDB (long AttCod);

void Att_HideEvent (void);
void Att_UnhideEvent (void);
void Att_ReceiveFormEvent (void);
void Att_CreateEvent (struct Att_Event *Event,const char *Description);
void Att_UpdateEvent (struct Att_Event *Event,const char *Description);

void Att_RemoveCrsEvents (long CrsCod);

unsigned Att_GetNumEvents (HieLvl_Level_t Scope,unsigned *NumNotif);

void Att_SeeOneEvent (void);

void Att_PutParsCodGrps (long AttCod);

void Att_RegisterMeAsStdInEvent (void);
void Att_RegisterStudentsInEvent (void);

void Att_ReqListUsrsAttendanceCrs (void);
void Att_ListMyAttendanceCrs (void);
void Att_PrintMyAttendanceCrs (void);
void Att_ListUsrsAttendanceCrs (void);
void Att_PrintUsrsAttendanceCrs (void);

#endif
