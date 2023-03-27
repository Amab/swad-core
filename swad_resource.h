// swad_resource.h: resources for course program and rubrics

#ifndef _SWAD_RSC
#define _SWAD_RSC
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

#include <mysql/mysql.h>	// To access MySQL databases

#include "swad_string.h"

/*****************************************************************************/
/************************** Public types and constants ***********************/
/*****************************************************************************/

#define Rsc_MAX_CHARS_RESOURCE_TITLE	(128 - 1)	// 127
#define Rsc_MAX_BYTES_RESOURCE_TITLE	((Rsc_MAX_CHARS_RESOURCE_TITLE + 1) * Str_MAX_BYTES_PER_CHAR - 1)	// 2047

#define Rsc_NUM_TYPES 12
typedef enum
  {
  Rsc_NONE,
  // gui TEACHING_GUIDE	// Link to teaching guide
  // bib BIBLIOGRAPHY	// Link to bibliography
  // faq FAQ		// Link to FAQ
  // lnk LINKS		// Link to links
  // tmt TIMETABLE	// Link to timetable
  Rsc_ASSIGNMENT,
  Rsc_PROJECT,		// A project is only for some students
  Rsc_CALL_FOR_EXAM,
  // tst TEST		// User selects tags, teacher should select
  Rsc_EXAM,
  Rsc_GAME,
  Rsc_RUBRIC,
  Rsc_DOCUMENT,
  Rsc_MARKS,
  // grp GROUPS		// ??? User select groups
  Rsc_ATTENDANCE_EVENT,
  Rsc_FORUM_THREAD,
  Rsc_SURVEY,
  } Rsc_Type_t;

struct Rsc_Link
  {
   Rsc_Type_t Type;
   long Cod;
  };

/*****************************************************************************/
/***************************** Public prototypes *****************************/
/*****************************************************************************/

void Rsc_WriteRowClipboard (bool SubmitOnClick,const struct Rsc_Link *Link);
void Rsc_WriteLinkName (const struct Rsc_Link *Link,bool PutFormToGo,
                        const char *Icon,const char *IconTitle);
void Rsc_WriteResourceEmpty (__attribute__((unused)) long Cod,
                             __attribute__((unused)) bool PutFormToGo,
                             const char *Icon,const char *IconTitle);

void Rsc_GetResourceTitleFromLink (struct Rsc_Link *Link,
                                   char Title[Rsc_MAX_BYTES_RESOURCE_TITLE + 1]);

void Rsc_GetLinkDataFromRow (MYSQL_RES *mysql_res,struct Rsc_Link *Link);
Rsc_Type_t Rsc_GetTypeFromString (const char *Str);

bool Rsc_CheckIfICanGetLink (void);

#endif
