// swad_calendar.c: Draw month and calendar

/*
    SWAD (Shared Workspace At a Distance),
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
/********************************* Headers ***********************************/
/*****************************************************************************/

#define _GNU_SOURCE 		// For asprintf
#include <stdio.h>		// For asprintf
#include <string.h>		// For string functions

#include "swad_box.h"
#include "swad_calendar.h"
#include "swad_call_for_exam.h"
#include "swad_database.h"
#include "swad_error.h"
#include "swad_figure.h"
#include "swad_form.h"
#include "swad_global.h"
#include "swad_hierarchy_level.h"
#include "swad_holiday.h"
#include "swad_HTML.h"
#include "swad_parameter.h"
#include "swad_setting.h"
#include "swad_setting_database.h"
#include "swad_user_database.h"

/*****************************************************************************/
/************** External global variables from others modules ****************/
/*****************************************************************************/

extern struct Globals Gbl;

/*****************************************************************************/
/***************************** Public constants ******************************/
/*****************************************************************************/

const bool Cal_DayIsValidAsFirstDayOfWeek[7] =
  {
   [0] = true,	// monday
   [1] = false,	// tuesday
   [2] = false,	// wednesday
   [3] = false,	// thursday
   [4] = false,	// friday
   [5] = false,	// saturday
   [6] = true,	// sunday
  };

/*****************************************************************************/
/***************************** Private prototypes ****************************/
/*****************************************************************************/

static void Cal_PutIconsFirstDayOfWeek (__attribute__((unused)) void *Args);

static unsigned Cal_GetParamFirstDayOfWeek (void);

static void Cal_DrawCalendar (Act_Action_t ActionSeeCalendar,
                              Act_Action_t ActionChangeCalendar1stDay,
                              void (*FunctionToDrawContextualIcons) (void *Args),void *Args,
                              bool PrintView);
static void Cal_PutIconsCalendar (__attribute__((unused)) void *Args);

/*****************************************************************************/
/************** Put icons to select the first day of the week ****************/
/*****************************************************************************/

void Cal_PutIconsToSelectFirstDayOfWeek (void)
  {
   extern const char *Hlp_PROFILE_Settings_calendar;
   extern const char *Txt_Calendar;

   Box_BoxBegin (NULL,Txt_Calendar,
                 Cal_PutIconsFirstDayOfWeek,NULL,
                 Hlp_PROFILE_Settings_calendar,Box_NOT_CLOSABLE);
      Set_BeginSettingsHead ();
	 Cal_ShowFormToSelFirstDayOfWeek (ActChg1stDay,
					  NULL,NULL);
      Set_EndSettingsHead ();
   Box_BoxEnd ();
  }

/*****************************************************************************/
/************ Put contextual icons in first-day-of-week setting **************/
/*****************************************************************************/

static void Cal_PutIconsFirstDayOfWeek (__attribute__((unused)) void *Args)
  {
   /***** Put icon to show a figure *****/
   Fig_PutIconToShowFigure (Fig_FIRST_DAY_OF_WEEK);
  }

/*****************************************************************************/
/************** Show form to select the first day of the week ****************/
/*****************************************************************************/

void Cal_ShowFormToSelFirstDayOfWeek (Act_Action_t Action,
                                      void (*FuncParams) (void *Args),void *Args)
  {
   extern const char *The_Colors[The_NUM_THEMES];
   extern const char *Txt_First_day_of_the_week_X;
   extern const char *Txt_DAYS_SMALL[7];
   unsigned FirstDayOfWeek;
   char *Title;
   char Icon[32 + 1];

   Set_BeginOneSettingSelector ();
      for (FirstDayOfWeek  = 0;	// Monday
	   FirstDayOfWeek <= 6;	// Sunday
	   FirstDayOfWeek++)
	 if (Cal_DayIsValidAsFirstDayOfWeek[FirstDayOfWeek])
	   {
	    if (FirstDayOfWeek == Gbl.Prefs.FirstDayOfWeek)
	       HTM_DIV_Begin ("class=\"PREF_ON PREF_ON_%s\"",
	                      The_Colors[Gbl.Prefs.Theme]);
	    else
	       HTM_DIV_Begin ("class=\"PREF_OFF\"");
	    Frm_BeginForm (Action);
	       Par_PutHiddenParamUnsigned (NULL,"FirstDayOfWeek",FirstDayOfWeek);
	       if (FuncParams)	// Extra parameters depending on the action
		  FuncParams (Args);
	       snprintf (Icon,sizeof (Icon),"first-day-of-week-%u.png",FirstDayOfWeek);
	       if (asprintf (&Title,Txt_First_day_of_the_week_X,
	                     Txt_DAYS_SMALL[FirstDayOfWeek]) < 0)
		  Err_NotEnoughMemoryExit ();
	       Ico_PutSettingIconLink (Icon,Ico_BLACK,Title);
	       free (Title);
	    Frm_EndForm ();
	    HTM_DIV_End ();
	   }
   Set_EndOneSettingSelector ();
  }

/*****************************************************************************/
/************************* Change first day of week **************************/
/*****************************************************************************/

void Cal_ChangeFirstDayOfWeek (void)
  {
   /***** Get param with icon set *****/
   Gbl.Prefs.FirstDayOfWeek = Cal_GetParamFirstDayOfWeek ();

   /***** Store icon first day of week database *****/
   if (Gbl.Usrs.Me.Logged)
      Set_DB_UpdateMySettingsAboutFirstDayOfWeek (Gbl.Prefs.FirstDayOfWeek);

   /***** Set settings from current IP *****/
   Set_SetSettingsFromIP ();
  }

/*****************************************************************************/
/*********************** Get parameter with icon set *************************/
/*****************************************************************************/

static unsigned Cal_GetParamFirstDayOfWeek (void)
  {
   unsigned FirstDayOfWeek;

   FirstDayOfWeek = (unsigned)
	            Par_GetParToUnsignedLong ("FirstDayOfWeek",
                                              0,
                                              6,
                                              Cal_FIRST_DAY_OF_WEEK_DEFAULT);
   if (!Cal_DayIsValidAsFirstDayOfWeek[FirstDayOfWeek])
      FirstDayOfWeek = Cal_FIRST_DAY_OF_WEEK_DEFAULT;

   return FirstDayOfWeek;
  }

/*****************************************************************************/
/******************** Get first day of week from string **********************/
/*****************************************************************************/

unsigned Cal_GetFirstDayOfWeekFromStr (const char *Str)
  {
   unsigned UnsignedNum;

   if (sscanf (Str,"%u",&UnsignedNum) == 1)
      if (Cal_DayIsValidAsFirstDayOfWeek[UnsignedNum])
         return (Dat_Format_t) UnsignedNum;

   return Cal_FIRST_DAY_OF_WEEK_DEFAULT;
  }

/*****************************************************************************/
/***************************** Draw current month ****************************/
/*****************************************************************************/

void Cal_DrawCurrentMonth (void)
  {
   extern const char *Lan_STR_LANG_ID[1 + Lan_NUM_LANGUAGES];
   char ParamsStr[Frm_MAX_BYTES_PARAMS_STR];

   /***** Draw the month in JavaScript *****/
   /* JavaScript will write HTML here */
   HTM_DIV_Begin ("id=\"CurrentMonth\"");
   HTM_DIV_End ();

   /* Write script to draw the month */
   HTM_SCRIPT_Begin (NULL,NULL);
      HTM_Txt ("\tGbl_HTMLContent = '';");
      HTM_TxtF ("\tDrawCurrentMonth ('CurrentMonth',%u,%ld,%ld,%u,'%s/%s',",
		Gbl.Prefs.FirstDayOfWeek,
		(long) Gbl.StartExecutionTimeUTC,
		Gbl.Hierarchy.Ctr.PlcCod,
		Gbl.Prefs.Theme,
		Cfg_URL_SWAD_CGI,Lan_STR_LANG_ID[Gbl.Prefs.Language]);
      Frm_SetParamsForm (ParamsStr,ActSeeCal,true);
      HTM_TxtF ("'%s',",ParamsStr);
      Frm_SetParamsForm (ParamsStr,ActSeeDatCfe,true);
      HTM_TxtF ("'%s');",ParamsStr);
   HTM_SCRIPT_End ();
  }

/*****************************************************************************/
/************************ Draw an academic calendar **************************/
/*****************************************************************************/

void Cal_ShowCalendar (void)
  {
   Cal_DrawCalendar (ActSeeCal,ActChgCal1stDay,
                     Cal_PutIconsCalendar,NULL,
                     false);
  }

void Cal_PrintCalendar (void)
  {
   Cal_DrawCalendar (ActUnk,ActUnk,
                     NULL,NULL,
                     true);
  }

static void Cal_DrawCalendar (Act_Action_t ActionSeeCalendar,
                              Act_Action_t ActionChangeCalendar1stDay,
                              void (*FunctionToDrawContextualIcons) (void *Args),void *Args,
                              bool PrintView)
  {
   extern const char *Hlp_START_Calendar;
   extern const char *Lan_STR_LANG_ID[1 + Lan_NUM_LANGUAGES];
   char ParamsStr[Frm_MAX_BYTES_PARAMS_STR];

   /***** Begin box *****/
   Box_BoxBegin (NULL,NULL,
                 FunctionToDrawContextualIcons,Args,
	         PrintView ? NULL :
	                     Hlp_START_Calendar,Box_NOT_CLOSABLE);

      /***** Write header *****/
      Lay_WriteHeaderClassPhoto (PrintView,false,
				 Gbl.Hierarchy.Ins.InsCod,
				 Gbl.Hierarchy.Deg.DegCod,
				 Gbl.Hierarchy.Crs.CrsCod);

      /***** Preference selector to change first day of week *****/
      if (!PrintView)
	{
	 Set_BeginSettingsHead ();
	    Cal_ShowFormToSelFirstDayOfWeek (ActionChangeCalendar1stDay,
					     NULL,NULL);
	 Set_EndSettingsHead ();
	}

      /***** Draw several months *****/
      /* JavaScript will write HTML here */
      HTM_DIV_Begin ("id=\"calendar\"");
      HTM_DIV_End ();

      /* Write script to draw the month */
      HTM_SCRIPT_Begin (NULL,NULL);
	 HTM_Txt ("\tGbl_HTMLContent = '';");
	 HTM_TxtF ("\tCal_DrawCalendar('calendar',%u,%ld,%ld,%s,%u,'%s/%s',",
		   Gbl.Prefs.FirstDayOfWeek,
		   (long) Gbl.StartExecutionTimeUTC,
		   Gbl.Hierarchy.Ctr.PlcCod,
		   PrintView ? "true" :
			       "false",
		   Gbl.Prefs.Theme,
		   Cfg_URL_SWAD_CGI,Lan_STR_LANG_ID[Gbl.Prefs.Language]);
	 Frm_SetParamsForm (ParamsStr,ActionSeeCalendar,true);
	 HTM_TxtF ("'%s',",ParamsStr);
	 Frm_SetParamsForm (ParamsStr,ActSeeDatCfe,true);
	 HTM_TxtF ("'%s');",ParamsStr);
      HTM_SCRIPT_End ();

   /***** End box *****/
   Box_BoxEnd ();
  }

/*****************************************************************************/
/******************** Put contextual icons in calendar ***********************/
/*****************************************************************************/

static void Cal_PutIconsCalendar (__attribute__((unused)) void *Args)
  {
   /***** Print calendar *****/
   Ico_PutContextualIconToPrint (ActPrnCal,
				 NULL,NULL);

   /***** View holidays *****/
   if (Gbl.Hierarchy.Level == HieLvl_INS)		// Institution selected
      Hld_PutIconToSeeHlds ();
  }

/*****************************************************************************/
/************************** Put icon to see calendar *************************/
/*****************************************************************************/

void Cal_PutIconToSeeCalendar (__attribute__((unused)) void *Args)
  {
   extern const char *Txt_Calendar;

   Lay_PutContextualLinkOnlyIcon (ActSeeCal,NULL,
				  NULL,NULL,
				  "calendar.svg",Ico_BLACK,
				  Txt_Calendar);
  }

/*****************************************************************************/
/***** Get and show number of users who have chosen a first day of week ******/
/*****************************************************************************/

void Cal_GetAndShowNumUsrsPerFirstDayOfWeek (void)
  {
   extern const bool Cal_DayIsValidAsFirstDayOfWeek[7];
   extern const char *Hlp_ANALYTICS_Figures_calendar;
   extern const char *The_Colors[The_NUM_THEMES];
   extern const char *Txt_FIGURE_TYPES[Fig_NUM_FIGURES];
   extern const char *Txt_Calendar;
   extern const char *Txt_First_day_of_the_week_X;
   extern const char *Txt_DAYS_SMALL[7];
   extern const char *Txt_Number_of_users;
   extern const char *Txt_PERCENT_of_users;
   unsigned FirstDayOfWeek;
   char *SubQuery;
   char *Icon;
   char *Title;
   unsigned NumUsrs[7];	// 7: seven days in a week
   unsigned NumUsrsTotal = 0;

   /***** Begin box and table *****/
   Box_BoxTableBegin (NULL,Txt_FIGURE_TYPES[Fig_FIRST_DAY_OF_WEEK],
                      NULL,NULL,
                      Hlp_ANALYTICS_Figures_calendar,Box_NOT_CLOSABLE,2);

      /***** Heading row *****/
      HTM_TR_Begin (NULL);
	 HTM_TH (Txt_Calendar        ,HTM_HEAD_LEFT);
	 HTM_TH (Txt_Number_of_users ,HTM_HEAD_RIGHT);
	 HTM_TH (Txt_PERCENT_of_users,HTM_HEAD_RIGHT);
      HTM_TR_End ();

      /***** For each day... *****/
      for (FirstDayOfWeek = 0;	// Monday
	   FirstDayOfWeek <= 6;	// Sunday
	   FirstDayOfWeek++)
	 if (Cal_DayIsValidAsFirstDayOfWeek[FirstDayOfWeek])
	   {
	    /* Get number of users who have chosen this first day of week from database */
	    if (asprintf (&SubQuery,"usr_data.FirstDayOfWeek=%u",
			  (unsigned) FirstDayOfWeek) < 0)
	       Err_NotEnoughMemoryExit ();
	    NumUsrs[FirstDayOfWeek] = Usr_DB_GetNumUsrsWhoChoseAnOption (SubQuery);
	    free (SubQuery);

	    /* Update total number of users */
	    NumUsrsTotal += NumUsrs[FirstDayOfWeek];
	   }

      /***** Write number of users who have chosen each first day of week *****/
      for (FirstDayOfWeek = 0;	// Monday
	   FirstDayOfWeek <= 6;	// Sunday
	   FirstDayOfWeek++)
	 if (Cal_DayIsValidAsFirstDayOfWeek[FirstDayOfWeek])
	   {
	    HTM_TR_Begin (NULL);

	       HTM_TD_Begin ("class=\"CM\"");
		  if (asprintf (&Icon,"first-day-of-week-%u.png",
				FirstDayOfWeek) < 0)
		     Err_NotEnoughMemoryExit ();
		  if (asprintf (&Title,Txt_First_day_of_the_week_X,
		                Txt_DAYS_SMALL[FirstDayOfWeek]) < 0)
		     Err_NotEnoughMemoryExit ();
		  Ico_PutIcon (Icon,Ico_BLACK,Title,"ICOx20");
		  free (Title);
		  free (Icon);
	       HTM_TD_End ();

	       HTM_TD_Begin ("class=\"RM DAT_%s\"",The_Colors[Gbl.Prefs.Theme]);
		  HTM_Unsigned (NumUsrs[FirstDayOfWeek]);
	       HTM_TD_End ();

	       HTM_TD_Begin ("class=\"RM DAT_%s\"",The_Colors[Gbl.Prefs.Theme]);
		  HTM_Percentage (NumUsrsTotal ? (double) NumUsrs[FirstDayOfWeek] * 100.0 /
						 (double) NumUsrsTotal :
						  0.0);
	       HTM_TD_End ();

	    HTM_TR_End ();
	   }

   /***** End table and box *****/
   Box_BoxTableEnd ();
  }
