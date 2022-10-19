// swad_log.c: access log stored in database

/*
    SWAD (Shared Workspace At a Distance),
    is a web platform developed at the University of Granada (Spain),
    and used to support university teaching.

    This file is part of SWAD core.
    Copyright (C) 1999-2022 Antonio Ca�as Vargas

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU Affero General 3 License as
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
/*********************************** Headers *********************************/
/*****************************************************************************/

#include <stdlib.h>		// For free
#include <string.h>		// For strlen

#include "swad_action.h"
#include "swad_banner.h"
#include "swad_box.h"
#include "swad_center_database.h"
#include "swad_config.h"
#include "swad_database.h"
#include "swad_degree_database.h"
#include "swad_exam_log.h"
#include "swad_global.h"
#include "swad_hierarchy.h"
#include "swad_HTML.h"
#include "swad_institution_database.h"
#include "swad_log.h"
#include "swad_log_database.h"
#include "swad_profile.h"
#include "swad_profile_database.h"
#include "swad_role.h"
#include "swad_statistic.h"

/*****************************************************************************/
/************** External global variables from others modules ****************/
/*****************************************************************************/

extern struct Globals Gbl;

/*****************************************************************************/
/**************************** Log access in database *************************/
/*****************************************************************************/

void Log_LogAccess (const char *Comments)
  {
   long LogCod;
   long ActCod = Act_GetActCod (Gbl.Action.Act);
   size_t MaxLength;
   char *CommentsDB;
   long BanCodClicked;
   Rol_Role_t RoleToStore = (Gbl.Action.Act == ActLogOut) ? Gbl.Usrs.Me.Role.LoggedBeforeCloseSession :
                                                            Gbl.Usrs.Me.Role.Logged;

   /***** Insert access into database *****/
   /* Log access in historical log */
   LogCod = Log_DB_LogAccessInHistoricalLog (ActCod,RoleToStore);

   /* Log access in recent log (log_recent) */
   Log_DB_LogAccessInRecentLog (LogCod,ActCod,RoleToStore);

   /* Log access while answering exam prints */
   ExaLog_LogAccess (LogCod);

   /* Log comments */
   if (Comments)
     {
      MaxLength = strlen (Comments) * Str_MAX_BYTES_PER_CHAR;
      if ((CommentsDB = malloc (MaxLength + 1)) != NULL)
	{
	 Str_Copy (CommentsDB,Comments,MaxLength);
	 Str_ChangeFormat (Str_FROM_TEXT,Str_TO_TEXT,
			   CommentsDB,MaxLength,true);	// Avoid SQL injection
	 Log_DB_LogComments (LogCod,CommentsDB);
	 free (CommentsDB);
	}
     }

   /* Log search string */
   if (Gbl.Search.LogSearch && Gbl.Search.Str[0])
      Log_DB_LogSearchString (LogCod);

   if (Gbl.WebService.IsWebService)
      /* Log web service plugin and API function */
      Log_DB_LogAPI (LogCod);
   else
     {
      BanCodClicked = Ban_GetBanCodClicked ();
      if (BanCodClicked > 0)
	 /* Log banner clicked */
	 Log_DB_LogBanner (LogCod,BanCodClicked);
     }

   /***** Increment my number of clicks *****/
   if (Gbl.Usrs.Me.Logged)
      Prf_DB_IncrementNumClicksUsr (Gbl.Usrs.Me.UsrDat.UsrCod);
  }

/*****************************************************************************/
/*************** Put a link to show last clicks in real time *****************/
/*****************************************************************************/

void Log_PutLinkToLastClicks (void)
  {
   extern const char *Txt_Last_clicks;

   Lay_PutContextualLinkIconText (ActLstClk,NULL,
                                  NULL,NULL,
				  "mouse-pointer.svg",Ico_BLACK,
				  Txt_Last_clicks,NULL);
  }

/*****************************************************************************/
/****************************** Show last clicks *****************************/
/*****************************************************************************/

void Log_ShowLastClicks (void)
  {
   extern const char *Hlp_USERS_Connected_last_clicks;
   extern const char *Txt_Last_clicks_in_real_time;

   /***** Contextual menu *****/
   Mnu_ContextMenuBegin ();
      Sta_PutLinkToGlobalHits ();	// Global hits
      Sta_PutLinkToCourseHits ();	// Course hits
   Mnu_ContextMenuEnd ();

   /***** Begin box *****/
   Box_BoxBegin (NULL,Txt_Last_clicks_in_real_time,
                 NULL,NULL,
                 Hlp_USERS_Connected_last_clicks,Box_NOT_CLOSABLE);

      /***** Get and show last clicks *****/
      HTM_DIV_Begin ("id=\"lastclicks\" class=\"CM\"");	// Used for AJAX based refresh
	 Log_GetAndShowLastClicks ();
      HTM_DIV_End ();					// Used for AJAX based refresh

   /***** End box *****/
   Box_BoxEnd ();
  }

/*****************************************************************************/
/**************** Get last clicks from database and show them ****************/
/*****************************************************************************/

void Log_GetAndShowLastClicks (void)
  {
   extern const char *Txt_Click;
   extern const char *Txt_ELAPSED_TIME;
   extern const char *Txt_Role;
   extern const char *Txt_Country;
   extern const char *Txt_Institution;
   extern const char *Txt_Center;
   extern const char *Txt_Degree;
   extern const char *Txt_Action;
   extern const char *Txt_ROLES_SINGUL_Abc[Rol_NUM_ROLES][Usr_NUM_SEXS];
   MYSQL_RES *mysql_res;
   MYSQL_ROW row;
   unsigned NumClicks;
   unsigned NumClick;
   long ActCod;
   Act_Action_t Action;
   const char *ClassRow;
   time_t TimeDiff;
   struct Hie_Hierarchy Hie;

   /***** Get last clicks from database *****/
   NumClicks = Log_DB_GetLastClicks (&mysql_res);

   /***** Write list of connected users *****/
   HTM_TABLE_BeginCenterPadding (1);

      /* Heading row */
      HTM_TR_Begin (NULL);
	 HTM_TH_Span (Txt_Click       ,HTM_HEAD_RIGHT,1,1,"LC_CLK");	// Click
	 HTM_TH_Span (Txt_ELAPSED_TIME,HTM_HEAD_RIGHT,1,1,"LC_TIM");	// Elapsed time
	 HTM_TH_Span (Txt_Role        ,HTM_HEAD_LEFT ,1,1,"LC_ROL");	// Role
	 HTM_TH_Span (Txt_Country     ,HTM_HEAD_LEFT ,1,1,"LC_CTY");	// Country
	 HTM_TH_Span (Txt_Institution ,HTM_HEAD_LEFT ,1,1,"LC_INS");	// Institution
	 HTM_TH_Span (Txt_Center      ,HTM_HEAD_LEFT ,1,1,"LC_CTR");	// Center
	 HTM_TH_Span (Txt_Degree      ,HTM_HEAD_LEFT ,1,1,"LC_DEG");	// Degree
	 HTM_TH_Span (Txt_Action      ,HTM_HEAD_LEFT ,1,1,"LC_ACT");	// Action
      HTM_TR_End ();

      for (NumClick = 0;
	   NumClick < NumClicks;
	   NumClick++)
	{
	 row = mysql_fetch_row (mysql_res);

	 /* Get action code (row[1]) */
	 ActCod = Str_ConvertStrCodToLongCod (row[1]);
	 Action = Act_GetActionFromActCod (ActCod);

	 /* Use a special color for this row depending on the action */
	 ClassRow = (Act_GetBrowserTab (Action) == Act_DOWNLD_FILE) ? "DAT_SMALL_YELLOW" :
		    (ActCod == Act_GetActCod (ActLogIn   ) ||
		     ActCod == Act_GetActCod (ActLogInNew)) ? "DAT_SMALL_GREEN" :
		    (ActCod == Act_GetActCod (ActLogOut  )) ? "DAT_SMALL_RED" :
		    (ActCod == Act_GetActCod (ActWebSvc  )) ? "DAT_SMALL_BLUE" :
							      "DAT_SMALL";

	 /* Compute elapsed time from last access */
	 if (sscanf (row[2],"%ld",&TimeDiff) != 1)
	    TimeDiff = (time_t) 0;

	 /* Get country code (row[4]) */
	 Hie.Cty.CtyCod = Str_ConvertStrCodToLongCod (row[4]);
	 Cty_GetCountryName (Hie.Cty.CtyCod,Gbl.Prefs.Language,
			     Hie.Cty.Name[Gbl.Prefs.Language]);

	 /* Get institution code (row[5]),
	        center      code (row[6])
	    and degree      code (row[7]) */
	 Hie.Ins.InsCod = Str_ConvertStrCodToLongCod (row[5]);
	 Hie.Ctr.CtrCod = Str_ConvertStrCodToLongCod (row[6]);
	 Hie.Deg.DegCod = Str_ConvertStrCodToLongCod (row[7]);
	 Ins_DB_GetShortNameOfInstitution (Hie.Ins.InsCod,Hie.Ins.ShrtName);
	 Ctr_DB_GetShortNameOfCenterByCod (Hie.Ctr.CtrCod,Hie.Ctr.ShrtName);
	 Deg_DB_GetShortNameOfDegreeByCod (Hie.Deg.DegCod,Hie.Deg.ShrtName);

	 /* Print table row */
	 HTM_TR_Begin (NULL);

	    HTM_TD_Begin ("class=\"LC_CLK %s_%s\"",
	                  ClassRow,The_GetSuffix ());
	       HTM_Txt (row[0]);					// Click
	    HTM_TD_End ();

	    HTM_TD_Begin ("class=\"LC_TIM %s_%s\"",
	                  ClassRow,The_GetSuffix ());
	       Dat_WriteHoursMinutesSecondsFromSeconds (TimeDiff);	// Elapsed time
	    HTM_TD_End ();

	    HTM_TD_Begin ("class=\"LC_ROL %s_%s\"",
	                  ClassRow,The_GetSuffix ());
	       HTM_Txt (Txt_ROLES_SINGUL_Abc[Rol_ConvertUnsignedStrToRole (row[3])][Usr_SEX_UNKNOWN]);	// Role
	    HTM_TD_End ();

	    HTM_TD_Begin ("class=\"LC_CTY %s_%s\"",
	                  ClassRow,The_GetSuffix ());
	       HTM_Txt (Hie.Cty.Name[Gbl.Prefs.Language]);		// Country
	    HTM_TD_End ();

	    HTM_TD_Begin ("class=\"LC_INS %s_%s\"",
	                  ClassRow,The_GetSuffix ());
	       HTM_Txt (Hie.Ins.ShrtName);				// Institution
	    HTM_TD_End ();

	    HTM_TD_Begin ("class=\"LC_CTR %s_%s\"",
	                  ClassRow,The_GetSuffix ());
	       HTM_Txt (Hie.Ctr.ShrtName);				// Center
	    HTM_TD_End ();

	    HTM_TD_Begin ("class=\"LC_DEG %s_%s\"",
	                  ClassRow,The_GetSuffix ());
	       HTM_Txt (Hie.Deg.ShrtName);				// Degree
	    HTM_TD_End ();

	    HTM_TD_Begin ("class=\"LC_ACT %s_%s\"",
	                  ClassRow,The_GetSuffix ());
	       HTM_Txt (Act_GetActionText (Action));			// Action
	    HTM_TD_End ();

	 HTM_TR_End ();
	}

   HTM_TABLE_End ();

   /***** Free structure that stores the query result *****/
   DB_FreeMySQLResult (&mysql_res);
  }
