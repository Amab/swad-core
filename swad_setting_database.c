// swad_setting.c: user's settings / preferences, operations with database

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
/********************************** Headers **********************************/
/*****************************************************************************/

#include "swad_database.h"
#include "swad_global.h"

/*****************************************************************************/
/*************** External global variables from others modules ***************/
/*****************************************************************************/

extern struct Globals Gbl;

/*****************************************************************************/
/***************************** Private constants *****************************/
/*****************************************************************************/

/*****************************************************************************/
/***************************** Private prototypes ****************************/
/*****************************************************************************/
/*****************************************************************************/
/**************** Update my language to the current language *****************/
/*****************************************************************************/

void Set_DB_UpdateMySettingsAboutLanguage (void)
  {
   extern const char *Lan_STR_LANG_ID[1 + Lan_NUM_LANGUAGES];

   DB_QueryUPDATE ("can not update your language",
		   "UPDATE usr_data"
		     " SET Language='%s'"
		   " WHERE UsrCod=%ld",
	           Lan_STR_LANG_ID[Gbl.Prefs.Language],
	           Gbl.Usrs.Me.UsrDat.UsrCod);
  }

/*****************************************************************************/
/******************** Update my settings about date format *******************/
/*****************************************************************************/

void Set_DB_UpdateMySettingsAboutDateFormat (Dat_Format_t DateFormat)
  {
   DB_QueryUPDATE ("can not update your setting about date format",
		   "UPDATE usr_data"
		     " SET DateFormat=%u"
		   " WHERE UsrCod=%ld",
		   (unsigned) DateFormat,
		   Gbl.Usrs.Me.UsrDat.UsrCod);
  }

/*****************************************************************************/
/***************** Update my settings about first day of week ****************/
/*****************************************************************************/

void Set_DB_UpdateMySettingsAboutFirstDayOfWeek (unsigned FirstDayOfWeek)
  {
   DB_QueryUPDATE ("can not update your setting about first day of week",
		   "UPDATE usr_data"
		     " SET FirstDayOfWeek=%u"
		   " WHERE UsrCod=%ld",
		   FirstDayOfWeek,
		   Gbl.Usrs.Me.UsrDat.UsrCod);
  }

/*****************************************************************************/
/********************* Update my settings about icon set *********************/
/*****************************************************************************/

void Set_DB_UpdateMySettingsAboutIconSet (const char *IconSetId)
  {
   DB_QueryUPDATE ("can not update your setting about icon set",
		   "UPDATE usr_data"
		     " SET IconSet='%s'"
		   " WHERE UsrCod=%ld",
		   IconSetId,
		   Gbl.Usrs.Me.UsrDat.UsrCod);
  }

/*****************************************************************************/
/***************** Update my settings about first day of week ****************/
/*****************************************************************************/

void Set_DB_UpdateMySettingsAboutMenu (Mnu_Menu_t Menu)
  {
   DB_QueryUPDATE ("can not update your setting about menu",
		   "UPDATE usr_data"
		     " SET Menu=%u"
		   " WHERE UsrCod=%ld",
		   (unsigned) Menu,
		   Gbl.Usrs.Me.UsrDat.UsrCod);
  }

/*****************************************************************************/
/*********************** Update my settings about theme **********************/
/*****************************************************************************/

void Set_DB_UpdateMySettingsAboutTheme (const char *ThemeId)
  {
   DB_QueryUPDATE ("can not update your setting about theme",
		   "UPDATE usr_data"
		     " SET Theme='%s'"
		   " WHERE UsrCod=%ld",
		   ThemeId,
		   Gbl.Usrs.Me.UsrDat.UsrCod);
  }

/*****************************************************************************/
/************** Update layout of side colums on user data table **************/
/*****************************************************************************/

void Set_DB_UpdateMySettingsAboutSideCols (void)
  {
   DB_QueryUPDATE ("can not update your setting about side columns",
		   "UPDATE usr_data"
		     " SET SideCols=%u"
		   " WHERE UsrCod=%ld",
		   Gbl.Prefs.SideCols,
		   Gbl.Usrs.Me.UsrDat.UsrCod);
  }

/*****************************************************************************/
/***************** Update my settings about photo visibility *****************/
/*****************************************************************************/

void Set_DB_UpdateMySettingsAboutPhoto (void)
  {
   extern const char *Pri_VisibilityDB[Pri_NUM_OPTIONS_PRIVACY];

   DB_QueryUPDATE ("can not update user's settings",
		   "UPDATE usr_data"
		     " SET PhotoVisibility='%s'"
		   " WHERE UsrCod=%ld",
		   Pri_VisibilityDB[Gbl.Usrs.Me.UsrDat.PhotoVisibility],
		   Gbl.Usrs.Me.UsrDat.UsrCod);
  }

/*****************************************************************************/
/************ Update my settings about public profile visibility *************/
/*****************************************************************************/

void Set_DB_UpdateMySettingsAboutBasicProfile (void)
  {
   extern const char *Pri_VisibilityDB[Pri_NUM_OPTIONS_PRIVACY];

   DB_QueryUPDATE ("can not update your setting"
		   " about public profile visibility",
		   "UPDATE usr_data"
		     " SET BaPrfVisibility='%s'"
		   " WHERE UsrCod=%ld",
                   Pri_VisibilityDB[Gbl.Usrs.Me.UsrDat.BaPrfVisibility],
                   Gbl.Usrs.Me.UsrDat.UsrCod);
  }

void Set_DB_UpdateMySettingsAboutExtendedProfile (void)
  {
   extern const char *Pri_VisibilityDB[Pri_NUM_OPTIONS_PRIVACY];

   DB_QueryUPDATE ("can not update your setting"
		   " about public profile visibility",
		   "UPDATE usr_data"
		     " SET ExPrfVisibility='%s'"
		   " WHERE UsrCod=%ld",
                   Pri_VisibilityDB[Gbl.Usrs.Me.UsrDat.ExPrfVisibility],
                   Gbl.Usrs.Me.UsrDat.UsrCod);
  }

/*****************************************************************************/
/******************* Update my settings about notify events ******************/
/*****************************************************************************/

void Set_DB_UpdateMySettingsAboutNotifyEvents (void)
  {
   DB_QueryUPDATE ("can not update user's settings",
		   "UPDATE usr_data"
		     " SET NotifNtfEvents=%u,"
		          "EmailNtfEvents=%u"
		   " WHERE UsrCod=%ld",
	           Gbl.Usrs.Me.UsrDat.NtfEvents.CreateNotif,
	           Gbl.Usrs.Me.UsrDat.NtfEvents.SendEmail,
	           Gbl.Usrs.Me.UsrDat.UsrCod);
  }

/*****************************************************************************/
/************ Register last prefs in current course in database **************/
/*****************************************************************************/

void Set_DB_InsertUsrInCrsSettings (long UsrCod,long CrsCod)
  {
   extern const char *Usr_StringsUsrListTypeInDB[Usr_NUM_USR_LIST_TYPES];

   DB_QueryINSERT ("can not register user in course",
		   "INSERT INTO crs_user_settings"
		   " (UsrCod,CrsCod,"
		     "LastDowGrpCod,LastComGrpCod,LastAssGrpCod,"
		     "NumAccTst,LastAccTst,NumQstsLastTst,"
		     "UsrListType,ColsClassPhoto,ListWithPhotos)"
		   " VALUES"
		   " (%ld,%ld,"
		     "-1,-1,-1,"
		     "0,FROM_UNIXTIME(%ld),0,"
		     "'%s',%u,'%c')",
	           UsrCod,
	           CrsCod,
	           (long) (time_t) 0,	// The user never accessed to tests in this course
	           Usr_StringsUsrListTypeInDB[Usr_SHOW_USRS_TYPE_DEFAULT],
	           Usr_CLASS_PHOTO_COLS_DEF,
	           Usr_LIST_WITH_PHOTOS_DEF ? 'Y' :
					      'N');
  }

/*****************************************************************************/
/******** Update the group of my last access to a file browser zone **********/
/*****************************************************************************/

void Set_DB_UpdateGrpLastAccZone (const char *FieldNameDB,long GrpCod)
  {
   DB_QueryUPDATE ("can not update the group of the last access to a file browser",
		   "UPDATE crs_user_settings"
		     " SET %s=%ld"
		   " WHERE UsrCod=%ld"
		     " AND CrsCod=%ld",
                   FieldNameDB,GrpCod,
                   Gbl.Usrs.Me.UsrDat.UsrCod,
                   Gbl.Hierarchy.Crs.CrsCod);
  }

/*****************************************************************************/
/****************** Remove a user from a courses setting *********************/
/*****************************************************************************/

void Set_DB_RemUsrFromCrsSettings (long UsrCod,long CrsCod)
  {
   DB_QueryDELETE ("can not remove a user from a course",
		   "DELETE FROM crs_user_settings"
		   " WHERE UsrCod=%ld"
		     " AND CrsCod=%ld",
		   UsrCod,
		   CrsCod);
  }

/*****************************************************************************/
/***************** Remove a user from all courses settings *******************/
/*****************************************************************************/

void Set_DB_RemUsrFromAllCrssSettings (long UsrCod)
  {
   DB_QueryDELETE ("can not remove a user from all courses",
		   "DELETE FROM crs_user_settings"
		   " WHERE UsrCod=%ld",
		   UsrCod);
  }

/*****************************************************************************/
/*************** Remove all users from settings in a course ******************/
/*****************************************************************************/

void Set_DB_RemAllUsrsFromCrsSettings (long CrsCod)
  {
   DB_QueryDELETE ("can not remove users from a course settings",
		   "DELETE FROM crs_user_settings"
		   " WHERE CrsCod=%ld",
		   CrsCod);
  }

/*****************************************************************************/
/**************** Update settings from current IP in database ****************/
/*****************************************************************************/

void Set_DB_UpdateSettingsFromIP (void)
  {
   extern const char *The_ThemeId[The_NUM_THEMES];
   extern const char *Ico_IconSetId[Ico_NUM_ICON_SETS];

   DB_QueryREPLACE ("can not store settings from current IP address",
		    "REPLACE INTO set_ip_settings"
		    " (IP,UsrCod,LastChange,"
		      "FirstDayOfWeek,DateFormat,Theme,IconSet,Menu,SideCols)"
		    " VALUES"
		    " ('%s',%ld,NOW(),"
		      "%u,%u,'%s','%s',%u,%u)",
	            Gbl.IP,
	            Gbl.Usrs.Me.UsrDat.UsrCod,
	            Gbl.Prefs.FirstDayOfWeek,
	            (unsigned) Gbl.Prefs.DateFormat,
	            The_ThemeId[Gbl.Prefs.Theme],
	            Ico_IconSetId[Gbl.Prefs.IconSet],
	            (unsigned) Gbl.Prefs.Menu,
	            Gbl.Prefs.SideCols);
  }

/*****************************************************************************/
/*************** Update my settings from current IP in database **************/
/*****************************************************************************/

void Set_DB_UpdateMySettingsFromIP (void)
  {
   extern const char *The_ThemeId[The_NUM_THEMES];
   extern const char *Ico_IconSetId[Ico_NUM_ICON_SETS];

   DB_QueryUPDATE ("can not update your settings",
		   "UPDATE set_ip_settings"
		     " SET FirstDayOfWeek=%u,"
			  "DateFormat=%u,"
			  "Theme='%s',"
			  "IconSet='%s',"
			  "Menu=%u,"
			  "SideCols=%u"
		   " WHERE UsrCod=%ld",
		   Gbl.Prefs.FirstDayOfWeek,
		   (unsigned) Gbl.Prefs.DateFormat,
		   The_ThemeId[Gbl.Prefs.Theme],
		   Ico_IconSetId[Gbl.Prefs.IconSet],
		   (unsigned) Gbl.Prefs.Menu,
		   Gbl.Prefs.SideCols,
		   Gbl.Usrs.Me.UsrDat.UsrCod);
   }

/*****************************************************************************/
/******************** Get settings changed from current IP *******************/
/*****************************************************************************/

unsigned Set_DB_GetSettingsFromIP (MYSQL_RES **mysql_res)
  {
   return (unsigned)
   DB_QuerySELECT (mysql_res,"can not get settings",
		   "SELECT FirstDayOfWeek,"	// row[0]
			  "DateFormat,"		// row[1]
			  "Theme,"		// row[2]
			  "IconSet,"		// row[3]
			  "Menu,"		// row[4]
			  "SideCols"		// row[5]
		    " FROM set_ip_settings"
		   " WHERE IP='%s'",
		   Gbl.IP);
  }

/*****************************************************************************/
/*********************** Remove old settings from IP *************************/
/*****************************************************************************/

void Set_DB_RemoveOldSettingsFromIP (void)
  {
   /***** Remove old settings *****/
   DB_QueryDELETE ("can not remove old settings",
		   "DELETE LOW_PRIORITY FROM set_ip_settings"
		   " WHERE LastChange<FROM_UNIXTIME(UNIX_TIMESTAMP()-%lu)",
                   Cfg_TIME_TO_DELETE_IP_PREFS);
  }