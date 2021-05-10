// swad_account_database.c: user's account operations with database

/*
    SWAD (Shared Workspace At a Distance),
    is a web platform developed at the University of Granada (Spain),
    and used to support university teaching.

    This file is part of SWAD core.
    Copyright (C) 1999-2021 Antonio Ca�as Vargas

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

#include "swad_account.h"
#include "swad_account_database.h"
#include "swad_calendar.h"
#include "swad_global.h"

/*****************************************************************************/
/****************************** Public constants *****************************/
/*****************************************************************************/

/*****************************************************************************/
/***************************** Private constants *****************************/
/*****************************************************************************/

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
/*************** Check if there are users with this user's ID ****************/
/*****************************************************************************/
// Returns the number of users got

unsigned Acc_DB_GetUsrsWithID (MYSQL_RES **mysql_res,
                               const char ID[ID_MAX_BYTES_USR_ID + 1])
  {
   return (unsigned)
   DB_QuerySELECT (mysql_res,"can not get user's codes",
		   "SELECT usr_ids.UsrCod"	// row[0]
		    " FROM usr_ids,"
			  "usr_data"
		   " WHERE usr_ids.UsrID='%s'"
		     " AND usr_ids.UsrCod=usr_data.UsrCod"
		     " AND usr_data.Password=''",
		   ID);
  }

/*****************************************************************************/
/****** Check if a nickname matches any of the nicknames of other users ******/
/*****************************************************************************/

bool Acc_DB_CheckIfNicknameAlreadyExists (const char NewNickWithoutArr[Nck_MAX_BYTES_NICK_FROM_FORM + 1])
  {
   return (DB_QueryCOUNT ("can not check if nickname already existed",
			  "SELECT COUNT(*)"
			   " FROM usr_nicknames"
			  " WHERE Nickname='%s'"
			    " AND UsrCod<>%ld",
			  NewNickWithoutArr,
			  Gbl.Usrs.Me.UsrDat.UsrCod) != 0);	// A nickname of another user is the same that this nickname
  }

/*****************************************************************************/
/******** Check if an email matches any of the emails of other users *********/
/*****************************************************************************/

bool Acc_DB_CheckIfEmailAlreadyExists (const char NewEmail[Cns_MAX_BYTES_EMAIL_ADDRESS + 1])
  {
   return (DB_QueryCOUNT ("can not check if email already existed",
			  "SELECT COUNT(*)"
			   " FROM usr_emails"
		          " WHERE E_mail='%s'"
		            " AND Confirmed='Y'",
	                  NewEmail) != 0);	// An email of another user is the same that my email
  }

/*****************************************************************************/
/****************************** Create new user ******************************/
/*****************************************************************************/

void Acc_DB_CreateNewUsr (struct UsrData *UsrDat)
  {
   extern const char *The_ThemeId[The_NUM_THEMES];
   extern const char *Ico_IconSetId[Ico_NUM_ICON_SETS];
   extern const char *Pri_VisibilityDB[Pri_NUM_OPTIONS_PRIVACY];
   extern const char *Lan_STR_LANG_ID[1 + Lan_NUM_LANGUAGES];
   extern const char *Usr_StringsSexDB[Usr_NUM_SEXS];
   char BirthdayStrDB[Usr_BIRTHDAY_STR_DB_LENGTH + 1];

   /***** Create new user *****/
   Usr_CreateBirthdayStrDB (UsrDat,BirthdayStrDB);	// It can include start and ending apostrophes
   UsrDat->UsrCod =
   DB_QueryINSERTandReturnCode ("can not create user",
 	                        "INSERT INTO usr_data"
				" (EncryptedUsrCod,Password,"
				  "Surname1,Surname2,FirstName,Sex,"
				  "Theme,IconSet,Language,FirstDayOfWeek,DateFormat,"
				  "PhotoVisibility,BaPrfVisibility,ExPrfVisibility,"
				  "CtyCod,"
				  "LocalPhone,FamilyPhone,"
				  "Birthday,Comments,"
				  "Menu,SideCols,NotifNtfEvents,EmailNtfEvents)"
				" VALUES"
				" ('%s','%s',"
				  "'%s','%s','%s','%s',"
				  "'%s','%s','%s',%u,%u,"
				  "'%s','%s','%s',"
				  "%ld,"
				  "'%s','%s',"
				  "%s,'%s',"
				  "%u,%u,-1,0)",
				UsrDat->EnUsrCod,
				UsrDat->Password,
				UsrDat->Surname1,
				UsrDat->Surname2,
				UsrDat->FrstName,
				Usr_StringsSexDB[UsrDat->Sex],
				The_ThemeId[UsrDat->Prefs.Theme],
				Ico_IconSetId[UsrDat->Prefs.IconSet],
				Lan_STR_LANG_ID[UsrDat->Prefs.Language],
				Cal_FIRST_DAY_OF_WEEK_DEFAULT,
				(unsigned) Dat_FORMAT_DEFAULT,
				Pri_VisibilityDB[UsrDat->PhotoVisibility],
				Pri_VisibilityDB[UsrDat->BaPrfVisibility],
				Pri_VisibilityDB[UsrDat->ExPrfVisibility],
				UsrDat->CtyCod,
				UsrDat->Phone[0],
				UsrDat->Phone[1],
				BirthdayStrDB,
				UsrDat->Comments ? UsrDat->Comments :
						   "",
				(unsigned) Mnu_MENU_DEFAULT,
				(unsigned) Cfg_DEFAULT_COLUMNS);
  }


/*****************************************************************************/
/*************************** Create new user's ID ****************************/
/*****************************************************************************/

void Acc_DB_CreateNewUsrID (long UsrCod,
		            const char ID[ID_MAX_BYTES_USR_ID + 1],
		            bool Confirmed)
  {
   DB_QueryINSERT ("can not store user's ID when creating user",
		   "INSERT INTO usr_ids"
		   " (UsrCod,UsrID,CreatTime,Confirmed)"
		   " VALUES"
		   " (%ld,'%s',NOW(),'%c')",
		   UsrCod,
		   ID,
		   Confirmed ? 'Y' :
			       'N');
  }
