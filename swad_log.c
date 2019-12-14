// swad_log.c: access log stored in database

/*
    SWAD (Shared Workspace At a Distance),
    is a web platform developed at the University of Granada (Spain),
    and used to support university teaching.

    This file is part of SWAD core.
    Copyright (C) 1999-2019 Antonio Ca�as Vargas

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

#include "swad_action.h"
#include "swad_config.h"
#include "swad_database.h"
#include "swad_global.h"
#include "swad_HTML.h"
#include "swad_log.h"
#include "swad_profile.h"
#include "swad_role.h"
#include "swad_statistic.h"

/*****************************************************************************/
/****************************** Public constants *****************************/
/*****************************************************************************/

/*****************************************************************************/
/***************************** Private constants *****************************/
/*****************************************************************************/

#define Log_SECONDS_IN_RECENT_LOG ((time_t) (Cfg_DAYS_IN_RECENT_LOG * 24UL * 60UL * 60UL))	// Remove entries in recent log oldest than this time

/*****************************************************************************/
/****************************** Private types ********************************/
/*****************************************************************************/

/*****************************************************************************/
/************** External global variables from others modules ****************/
/*****************************************************************************/

extern struct Globals Gbl;

/*****************************************************************************/
/************************* Private global variables **************************/
/*****************************************************************************/

/*****************************************************************************/
/***************************** Private prototypes ****************************/
/*****************************************************************************/

/*****************************************************************************/
/**************************** Log access in database *************************/
/*****************************************************************************/

void Log_LogAccess (const char *Comments)
  {
   long LogCod;
   long ActCod = Act_GetActCod (Gbl.Action.Act);
   Rol_Role_t RoleToStore = (Gbl.Action.Act == ActLogOut) ? Gbl.Usrs.Me.Role.LoggedBeforeCloseSession :
                                                            Gbl.Usrs.Me.Role.Logged;

   /***** Insert access into database *****/
   /* Log access in historical log (log_full) */
   LogCod =
   DB_QueryINSERTandReturnCode ("can not log access (full)",
				"INSERT INTO log_full "
				"(ActCod,CtyCod,InsCod,CtrCod,DegCod,CrsCod,UsrCod,"
				"Role,ClickTime,TimeToGenerate,TimeToSend,IP)"
				" VALUES "
				"(%ld,%ld,%ld,%ld,%ld,%ld,%ld,"
				"%u,NOW(),%ld,%ld,'%s')",
				ActCod,
				Gbl.Hierarchy.Cty.CtyCod,
				Gbl.Hierarchy.Ins.InsCod,
				Gbl.Hierarchy.Ctr.CtrCod,
				Gbl.Hierarchy.Deg.DegCod,
				Gbl.Hierarchy.Crs.CrsCod,
				Gbl.Usrs.Me.UsrDat.UsrCod,
				(unsigned) RoleToStore,
				Gbl.TimeGenerationInMicroseconds,
				Gbl.TimeSendInMicroseconds,
				Gbl.IP);

   /* Log access in recent log (log_recent) */
   DB_QueryINSERT ("can not log access (recent)",
		   "INSERT INTO log_recent "
	           "(LogCod,ActCod,CtyCod,InsCod,CtrCod,DegCod,CrsCod,UsrCod,"
	           "Role,ClickTime,TimeToGenerate,TimeToSend,IP)"
                   " VALUES "
                   "(%ld,%ld,%ld,%ld,%ld,%ld,%ld,%ld,"
                   "%u,NOW(),%ld,%ld,'%s')",
		   LogCod,ActCod,
		   Gbl.Hierarchy.Cty.CtyCod,
		   Gbl.Hierarchy.Ins.InsCod,
		   Gbl.Hierarchy.Ctr.CtrCod,
		   Gbl.Hierarchy.Deg.DegCod,
		   Gbl.Hierarchy.Crs.CrsCod,
		   Gbl.Usrs.Me.UsrDat.UsrCod,
		   (unsigned) RoleToStore,
		   Gbl.TimeGenerationInMicroseconds,
		   Gbl.TimeSendInMicroseconds,
		   Gbl.IP);

   /* Log comments */
   if (Comments)
      DB_QueryINSERT ("can not log access (comments)",
		      "INSERT INTO log_comments"
		      " (LogCod,Comments)"
		      " VALUES"
		      " (%ld,'%s')",
		      LogCod,Comments);

   /* Log search string */
   if (Gbl.Search.LogSearch && Gbl.Search.Str[0])
      DB_QueryINSERT ("can not log access (search)",
		      "INSERT INTO log_search"
		      " (LogCod,SearchStr)"
		      " VALUES"
		      " (%ld,'%s')",
		      LogCod,Gbl.Search.Str);

   if (Gbl.WebService.IsWebService)
      /* Log web service plugin and function */
      DB_QueryINSERT ("can not log access (comments)",
		      "INSERT INTO log_ws"
	              " (LogCod,PlgCod,FunCod)"
                      " VALUES"
                      " (%ld,%ld,%u)",
	              LogCod,Gbl.WebService.PlgCod,
		      (unsigned) Gbl.WebService.Function);
   else if (Gbl.Banners.BanCodClicked > 0)
      /* Log banner clicked */
      DB_QueryINSERT ("can not log banner clicked",
		      "INSERT INTO log_banners"
	              " (LogCod,BanCod)"
                      " VALUES"
                      " (%ld,%ld)",
	              LogCod,Gbl.Banners.BanCodClicked);

   /***** Increment my number of clicks *****/
   if (Gbl.Usrs.Me.Logged)
      Prf_IncrementNumClicksUsr (Gbl.Usrs.Me.UsrDat.UsrCod);
  }

/*****************************************************************************/
/************ Sometimes, we delete old entries in recent log table ***********/
/*****************************************************************************/

void Log_RemoveOldEntriesRecentLog (void)
  {
   /***** Remove all expired clipboards *****/
   DB_QueryDELETE ("can not remove old entries from recent log",
		   "DELETE LOW_PRIORITY FROM log_recent"
                   " WHERE ClickTime<FROM_UNIXTIME(UNIX_TIMESTAMP()-%lu)",
		   Log_SECONDS_IN_RECENT_LOG);
  }

/*****************************************************************************/
/*************** Put a link to show last clicks in real time *****************/
/*****************************************************************************/

void Log_PutLinkToLastClicks (void)
  {
   extern const char *Txt_Last_clicks;

   Lay_PutContextualLinkIconText (ActLstClk,NULL,NULL,
				  "mouse-pointer.svg",
				  Txt_Last_clicks);
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
   Box_BoxBegin (NULL,Txt_Last_clicks_in_real_time,NULL,
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
   extern const char *Txt_Centre;
   extern const char *Txt_Degree;
   extern const char *Txt_Action;
   extern const char *Txt_ROLES_SINGUL_Abc[Rol_NUM_ROLES][Usr_NUM_SEXS];
   MYSQL_RES *mysql_res;
   MYSQL_ROW row;
   unsigned long NumRow;
   unsigned long NumRows;
   long ActCod;
   const char *ClassRow;
   time_t TimeDiff;
   struct Country Cty;
   struct Instit Ins;
   struct Centre Ctr;
   struct Degree Deg;

   /***** Get last clicks from database *****/
   /* Important for maximum performance:
      do the LIMIT in the big log table before the JOIN */
   NumRows = DB_QuerySELECT (&mysql_res,"can not get last clicks",
			     "SELECT last_logs.LogCod,last_logs.ActCod,"
			     "last_logs.Dif,last_logs.Role,"
			     "last_logs.CtyCod,last_logs.InsCod,"
			     "last_logs.CtrCod,last_logs.DegCod,"
			     "actions.Txt"
			     " FROM"
			     " (SELECT LogCod,ActCod,"
			     "UNIX_TIMESTAMP()-UNIX_TIMESTAMP(ClickTime) AS Dif,"
			     "Role,CtyCod,InsCod,CtrCod,DegCod"
			     " FROM log_recent ORDER BY LogCod DESC LIMIT 20)"
			     " AS last_logs LEFT JOIN actions"	// LEFT JOIN because action may be not present in table of actions
			     " ON last_logs.ActCod=actions.ActCod"
			     " WHERE actions.Language='es'"	// TODO: Change to user's language
			     " OR actions.Language IS NULL");	// When action is not present in table of actions

   /***** Write list of connected users *****/
   HTM_TABLE_BeginCenterPadding (1);
   HTM_TR_Begin (NULL);

   HTM_TH (1,1,"LC_CLK",Txt_Click);		// Click
   HTM_TH (1,1,"LC_TIM",Txt_ELAPSED_TIME);	// Elapsed time
   HTM_TH (1,1,"LC_ROL",Txt_Role);		// Role
   HTM_TH (1,1,"LC_CTY",Txt_Country);		// Country
   HTM_TH (1,1,"LC_INS",Txt_Institution);	// Institution
   HTM_TH (1,1,"LC_CTR",Txt_Centre);		// Centre
   HTM_TH (1,1,"LC_DEG",Txt_Degree);		// Degree
   HTM_TH (1,1,"LC_ACT",Txt_Action);		// Action

   HTM_TR_End ();

   for (NumRow = 0;
	NumRow < NumRows;
	NumRow++)
     {
      row = mysql_fetch_row (mysql_res);

      /* Get action code (row[1]) */
      ActCod = Str_ConvertStrCodToLongCod (row[1]);

      /* Use a special color for this row depending on the action */
      ClassRow = (Act_GetBrowserTab (Act_GetActionFromActCod (ActCod)) == Act_DOWNLD_FILE) ? "DAT_SMALL_YELLOW" :
	         (ActCod == Act_GetActCod (ActLogIn   ) ||
	          ActCod == Act_GetActCod (ActLogInNew)) ? "DAT_SMALL_GREEN" :
                 (ActCod == Act_GetActCod (ActLogOut  )) ? "DAT_SMALL_RED" :
                 (ActCod == Act_GetActCod (ActWebSvc  )) ? "DAT_SMALL_BLUE" :
                                                           "DAT_SMALL_GREY";

      /* Compute elapsed time from last access */
      if (sscanf (row[2],"%ld",&TimeDiff) != 1)
         TimeDiff = (time_t) 0;

      /* Get country code (row[4]) */
      Cty.CtyCod = Str_ConvertStrCodToLongCod (row[4]);
      Cty_GetCountryName (Cty.CtyCod,Cty.Name[Gbl.Prefs.Language]);

      /* Get institution code (row[5]) */
      Ins.InsCod = Str_ConvertStrCodToLongCod (row[5]);
      Ins_GetShortNameOfInstitution (&Ins);

      /* Get centre code (row[6]) */
      Ctr.CtrCod = Str_ConvertStrCodToLongCod (row[6]);
      Ctr_GetShortNameOfCentreByCod (&Ctr);

      /* Get degree code (row[7]) */
      Deg.DegCod = Str_ConvertStrCodToLongCod (row[7]);
      Deg_GetShortNameOfDegreeByCod (&Deg);

      /* Print table row */
      HTM_TR_Begin (NULL);

      HTM_TD_Begin ("class=\"LC_CLK %s\"",ClassRow);
      HTM_Txt (row[0]);				// Click
      HTM_TD_End ();

      HTM_TD_Begin ("class=\"LC_TIM %s\"",ClassRow);		// Elapsed time
      Dat_WriteHoursMinutesSecondsFromSeconds (TimeDiff);
      HTM_TD_End ();

      HTM_TD_Begin ("class=\"LC_ROL %s\"",ClassRow);
      HTM_Txt (					// Role
	       Txt_ROLES_SINGUL_Abc[Rol_ConvertUnsignedStrToRole (row[3])][Usr_SEX_UNKNOWN]);
      HTM_TD_End ();

      HTM_TD_Begin ("class=\"LC_CTY %s\"",ClassRow);
      HTM_Txt (Cty.Name[Gbl.Prefs.Language]);	// Country
      HTM_TD_End ();

      HTM_TD_Begin ("class=\"LC_INS %s\"",ClassRow);
      HTM_Txt (Ins.ShrtName);			// Institution
      HTM_TD_End ();

      HTM_TD_Begin ("class=\"LC_CTR %s\"",ClassRow);
      HTM_Txt (Ctr.ShrtName);			// Centre
      HTM_TD_End ();

      HTM_TD_Begin ("class=\"LC_DEG %s\"",ClassRow);
      HTM_Txt (Deg.ShrtName);			// Degree
      HTM_TD_End ();

      HTM_TD_Begin ("class=\"LC_ACT %s\"",ClassRow);
      if (row[8])
	 if (row[8][0])
	    HTM_Txt (row[8]);			// Action
      HTM_TD_End ();

      HTM_TR_End ();
     }
   HTM_TABLE_End ();

   /***** Free structure that stores the query result *****/
   mysql_free_result (mysql_res);
  }
