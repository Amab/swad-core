// swad_calendar.h: Draw month and calendar

#ifndef _SWAD_CAL
#define _SWAD_CAL
/*
    SWAD (Shared Workspace At a Distance in Spanish),
    is a web platform developed at the University of Granada (Spain),
    and used to support university teaching.

    This file is part of SWAD core.
    Copyright (C) 1999-2018 Antonio Ca�as Vargas

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

#include "swad_action.h"

/*****************************************************************************/
/***************************** Public constants ******************************/
/*****************************************************************************/

#define Cal_FIRST_DAY_OF_WEEK_DEFAULT	0	// Monday

/*****************************************************************************/
/******************************* Public types ********************************/
/*****************************************************************************/

/*****************************************************************************/
/***************************** Public prototypes *****************************/
/*****************************************************************************/

void Cal_PutIconsToSelectFirstDayOfWeek (void);
void Cal_ShowFormToSelFirstDayOfWeek (Act_Action_t Action,void (*FuncParams) (),
                                      const char *ClassIcon);
void Cal_ChangeFirstDayOfWeek (void);
unsigned Cal_GetFirstDayOfWeekFromStr (const char *Str);

void Cal_DrawCurrentMonth (void);
Act_Action_t Cal_GetActionToSeeCalendar ();

void Cal_DrawCalendarSys (void);
void Cal_DrawCalendarCty (void);
void Cal_DrawCalendarIns (void);
void Cal_DrawCalendarCtr (void);
void Cal_DrawCalendarDeg (void);
void Cal_DrawCalendarCrs (void);
void Cal_PrintCalendar (void);

#endif
