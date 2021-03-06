// swad_user.c: users

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

#define _GNU_SOURCE 		// For asprintf
#include <ctype.h>		// For isalnum, isdigit, etc.
#include <limits.h>		// For maximum values
#include <linux/limits.h>	// For PATH_MAX
#include <stddef.h>		// For NULL
#include <stdio.h>		// For asprintf
#include <stdlib.h>		// For exit, system, malloc, free, rand, etc.
#include <string.h>		// For string functions
#include <sys/wait.h>		// For the macro WEXITSTATUS
#include <unistd.h>		// For access, lstat, getpid, chdir, symlink, unlink

#include "swad_account.h"
#include "swad_agenda.h"
#include "swad_announcement.h"
#include "swad_box.h"
#include "swad_calendar.h"
#include "swad_config.h"
#include "swad_connected_database.h"
#include "swad_course.h"
#include "swad_database.h"
#include "swad_department.h"
#include "swad_duplicate.h"
#include "swad_enrolment.h"
#include "swad_error.h"
#include "swad_figure.h"
#include "swad_figure_cache.h"
#include "swad_follow.h"
#include "swad_form.h"
#include "swad_global.h"
#include "swad_group.h"
#include "swad_help.h"
#include "swad_hierarchy.h"
#include "swad_hierarchy_level.h"
#include "swad_HTML.h"
#include "swad_ID.h"
#include "swad_language.h"
#include "swad_MFU.h"
#include "swad_nickname.h"
#include "swad_notification.h"
#include "swad_parameter.h"
#include "swad_password.h"
#include "swad_photo.h"
#include "swad_privacy.h"
#include "swad_QR.h"
#include "swad_record.h"
#include "swad_role.h"
#include "swad_setting.h"
#include "swad_tab.h"
#include "swad_user.h"

/*****************************************************************************/
/****************************** Public constants *****************************/
/*****************************************************************************/

const char *Usr_StringsSexIcons[Usr_NUM_SEXS] =
  {
   [Usr_SEX_UNKNOWN] = "?",
   [Usr_SEX_FEMALE ] = "&female;",
   [Usr_SEX_MALE   ] = "&male;",
   [Usr_SEX_ALL    ] = "*",
   };

const char *Usr_StringsSexDB[Usr_NUM_SEXS] =
  {
   [Usr_SEX_UNKNOWN] = "unknown",
   [Usr_SEX_FEMALE ] = "female",
   [Usr_SEX_MALE   ] = "male",
   [Usr_SEX_ALL    ] = "all",
   };

const char *Usr_StringsUsrListTypeInDB[Usr_NUM_USR_LIST_TYPES] =
  {
   [Usr_LIST_UNKNOWN       ] = "",
   [Usr_LIST_AS_CLASS_PHOTO] = "classphoto",
   [Usr_LIST_AS_LISTING    ] = "list",
  };

/*****************************************************************************/
/***************************** Private constants *****************************/
/*****************************************************************************/

static const char *Usr_IconsClassPhotoOrList[Usr_NUM_USR_LIST_TYPES] =
  {
   [Usr_LIST_UNKNOWN       ] = "",
   [Usr_LIST_AS_CLASS_PHOTO] = "th.svg",
   [Usr_LIST_AS_LISTING    ] = "list-ol.svg",
  };

static const char *Usr_NameSelUnsel[Rol_NUM_ROLES] =
  {
   [Rol_GST] = "SEL_UNSEL_GSTS",
   [Rol_STD] = "SEL_UNSEL_STDS",
   [Rol_NET] = "SEL_UNSEL_NETS",
   [Rol_TCH] = "SEL_UNSEL_TCHS",
  };
static const char *Usr_ParamUsrCod[Rol_NUM_ROLES] =
  {
   [Rol_UNK] = "UsrCodAll",	//  here means all users
   [Rol_GST] = "UsrCodGst",
   [Rol_STD] = "UsrCodStd",
   [Rol_NET] = "UsrCodNET",
   [Rol_TCH] = "UsrCodTch",
  };

#define Usr_NUM_MAIN_FIELDS_DATA_ADM	 7
#define Usr_NUM_ALL_FIELDS_DATA_GST	14
#define Usr_NUM_ALL_FIELDS_DATA_STD	10
#define Usr_NUM_ALL_FIELDS_DATA_TCH	11
const char *Usr_UsrDatMainFieldNames[Usr_NUM_MAIN_FIELDS_DATA_USR];

#define Usr_MAX_BYTES_QUERY_GET_LIST_USRS (16 * 1024 - 1)

/*****************************************************************************/
/****************************** Private types ********************************/
/*****************************************************************************/

/*****************************************************************************/
/************** External global variables from others modules ****************/
/*****************************************************************************/

extern struct Globals Gbl;

/*****************************************************************************/
/****************************** Private variables ****************************/
/*****************************************************************************/

static void (*Usr_FuncParamsBigList) (void *Args);	// Used to pass pointer to function

/*****************************************************************************/
/***************************** Private prototypes ****************************/
/*****************************************************************************/

static void Usr_GetMyLastData (void);
static void Usr_GetUsrCommentsFromString (char *Str,struct UsrData *UsrDat);
static Usr_Sex_t Usr_GetSexFromStr (const char *Str);

static bool Usr_CheckIfMyBirthdayHasNotBeenCongratulated (void);
static void Usr_InsertMyBirthday (void);

static void Usr_RemoveTemporaryTableMyCourses (void);

static void Usr_GetParamOtherUsrIDNickOrEMail (void);

static bool Usr_ChkUsrAndGetUsrDataFromDirectLogin (void);
static bool Usr_ChkUsrAndGetUsrDataFromSession (void);
static void Usr_ShowAlertUsrDoesNotExistsOrWrongPassword (void);
static void Usr_ShowAlertThereAreMoreThanOneUsr (void);

static void Usr_SetMyPrefsAndRoles (void);

static void Usr_PutLinkToLogOut (__attribute__((unused)) void *Args);

static void Usr_InsertMyLastData (void);

static void Usr_WriteRowGstAllData (struct UsrData *UsrDat);
static void Usr_WriteRowStdAllData (struct UsrData *UsrDat,char *GroupNames);
static void Usr_WriteRowTchAllData (struct UsrData *UsrDat);
static void Usr_WriteRowAdmData (unsigned NumUsr,struct UsrData *UsrDat);
static void Usr_WriteMainUsrDataExceptUsrID (struct UsrData *UsrDat,
                                             const char *BgColor);
static void Usr_WriteEmail (struct UsrData *UsrDat,const char *BgColor);
static void Usr_WriteUsrData (const char *BgColor,
                              const char *Data,const char *Link,
                              bool NonBreak,bool Accepted);

static void Usr_BuildQueryToGetUsrsLstCrs (char **Query,Rol_Role_t Role);

static void Usr_GetAdmsLst (HieLvl_Level_t Scope);
static void Usr_GetGstsLst (HieLvl_Level_t Scope);
static void Usr_GetListUsrsFromQuery (char *Query,Rol_Role_t Role,HieLvl_Level_t Scope);
static void Usr_AllocateUsrsList (Rol_Role_t Role);

static void Usr_PutButtonToConfirmIWantToSeeBigList (unsigned NumUsrs,
                                                     void (*FuncParams) (void *Args),void *Args,
                                                     const char *OnSubmit);
static void Usr_PutParamsConfirmIWantToSeeBigList (void *Args);

static void Usr_BuildParamName (char **ParamName,
				const char *ParamRoot,
				const char *ParamSuffix);

static void Usr_AllocateListSelectedEncryptedUsrCods (struct SelectedUsrs *SelectedUsrs,
						      Rol_Role_t Role);
static void Usr_AllocateListOtherRecipients (void);

static void Usr_FormToSelectUsrListType (void (*FuncParams) (void *Args),void *Args,
                                         Usr_ShowUsrsType_t ListType);
static void Usr_PutCheckboxToSelectAllUsers (Rol_Role_t Role,
			                     struct SelectedUsrs *SelectedUsrs);
static Usr_Sex_t Usr_GetSexOfUsrsLst (Rol_Role_t Role);
static void Usr_PutCheckboxToSelectUser (Rol_Role_t Role,
                                         const char *EncryptedUsrCod,
                                         bool UsrIsTheMsgSender,
					 struct SelectedUsrs *SelectedUsrs);
static void Usr_PutCheckboxListWithPhotos (void);

static void Usr_ListMainDataGsts (bool PutCheckBoxToSelectUsr);
static void Usr_ListMainDataStds (bool PutCheckBoxToSelectUsr);
static void Usr_ListMainDataTchs (Rol_Role_t Role,
				  bool PutCheckBoxToSelectUsr);
static void Usr_ListUsrsForSelection (Rol_Role_t Role,
				      struct SelectedUsrs *SelectedUsrs);
static void Usr_ListRowsAllDataTchs (Rol_Role_t Role,
                                     const char *FieldNames[Usr_NUM_ALL_FIELDS_DATA_TCH],
                                     unsigned NumColumns);
static void Usr_GetAndUpdateUsrListType (void);
static void Usr_GetUsrListTypeFromForm (void);
static void Usr_GetMyUsrListTypeFromDB (void);
static void Usr_UpdateMyUsrListTypeInDB (void);

static void Usr_GetParamColsClassPhotoFromForm (void);
static void Usr_GetMyColsClassPhotoFromDB (void);
static void Usr_UpdateMyColsClassPhotoInDB (void);

static void Usr_GetAndUpdatePrefAboutListWithPhotos (void);
static bool Usr_GetParamListWithPhotosFromForm (void);
static void Usr_UpdateMyPrefAboutListWithPhotosPhotoInDB (void);

static void Usr_PutLinkToSeeAdmins (void);
static void Usr_PutLinkToSeeGuests (void);

static bool Usr_SetOptionsListUsrsAllowed (Rol_Role_t UsrsRole,
                                           bool ICanChooseOption[Usr_LIST_USRS_NUM_OPTIONS]);
static void Usr_PutOptionsListUsrs (const bool ICanChooseOption[Usr_LIST_USRS_NUM_OPTIONS]);
static void Usr_ShowOneListUsrsOption (Usr_ListUsrsOption_t ListUsrsAction,
                                       const char *Label);
static Usr_ListUsrsOption_t Usr_GetListUsrsOption (Usr_ListUsrsOption_t DefaultAction);

static void Usr_PutIconsListGsts (__attribute__((unused)) void *Args);
static void Usr_PutIconsListStds (__attribute__((unused)) void *Args);
static void Usr_PutIconsListTchs (__attribute__((unused)) void *Args);

static void Usr_PutIconToPrintGsts (void);
static void Usr_PutIconToPrintStds (void);
static void Usr_PutIconToPrintTchs (void);
static void Usr_PutIconToShowGstsAllData (void);
static void Usr_PutIconToShowStdsAllData (void);
static void Usr_PutIconToShowTchsAllData (void);
static void Usr_ShowGstsAllDataParams (__attribute__((unused)) void *Args);
static void Usr_ShowStdsAllDataParams (__attribute__((unused)) void *Args);
static void Usr_ShowTchsAllDataParams (__attribute__((unused)) void *Args);

static void Usr_DrawClassPhoto (Usr_ClassPhotoType_t ClassPhotoType,
                                Rol_Role_t Role,
				struct SelectedUsrs *SelectedUsrs,
				bool PutCheckBoxToSelectUsr);

static FigCch_FigureCached_t Usr_GetFigureNumUsrsInCrss (unsigned Roles);
static double Usr_GetNumCrssPerUsr (HieLvl_Level_t Scope,long Cod,Rol_Role_t Role);
static double Usr_GetNumUsrsPerCrs (HieLvl_Level_t Scope,long Cod,Rol_Role_t Role);

/*****************************************************************************/
/**** Show alert about number of clicks remaining before sending my photo ****/
/*****************************************************************************/

void Usr_InformAboutNumClicksBeforePhoto (void)
  {
   extern const char *Txt_You_must_send_your_photo_because_;
   extern const char *Txt_You_can_only_perform_X_further_actions_;
   extern const char *Txt_Upload_photo;

   if (Gbl.Usrs.Me.NumAccWithoutPhoto)
     {
      if (Gbl.Usrs.Me.NumAccWithoutPhoto >= Pho_MAX_CLICKS_WITHOUT_PHOTO)
         Ale_ShowAlert (Ale_WARNING,Txt_You_must_send_your_photo_because_);
      else if (Act_GetBrowserTab (Gbl.Action.Act) == Act_BRW_1ST_TAB)
         Ale_ShowAlertAndButton (ActReqMyPho,NULL,NULL,
                                 NULL,NULL,
                                 Btn_CONFIRM_BUTTON,Txt_Upload_photo,
				 Ale_WARNING,Txt_You_can_only_perform_X_further_actions_,
                                 Pho_MAX_CLICKS_WITHOUT_PHOTO - Gbl.Usrs.Me.NumAccWithoutPhoto);
     }
  }

/*****************************************************************************/
/************************** Create data for a user ***************************/
/*****************************************************************************/

void Usr_UsrDataConstructor (struct UsrData *UsrDat)
  {
   /***** Allocate memory for the comments *****/
   if ((UsrDat->Comments = malloc (Cns_MAX_BYTES_TEXT + 1)) == NULL)
      Err_NotEnoughMemoryExit ();

   /***** Initialize to zero the data of the user *****/
   Usr_ResetUsrDataExceptUsrCodAndIDs (UsrDat);
   UsrDat->IDs.Num = 0;
   UsrDat->IDs.List = NULL;
  }

/*****************************************************************************/
/****************** Reset user's data except UsrCod and ID *******************/
/*****************************************************************************/
// UsrCod and ID are not changed

void Usr_ResetUsrDataExceptUsrCodAndIDs (struct UsrData *UsrDat)
  {
   UsrDat->EnUsrCod[0] = '\0';
   UsrDat->Nickname[0] = '\0';
   UsrDat->Password[0] = '\0';
   UsrDat->Roles.InCurrentCrs = Rol_UNK;// not yet got from database
   UsrDat->Roles.InCrss = -1;		// not yet got from database
   UsrDat->Accepted = false;

   UsrDat->Sex = Usr_SEX_UNKNOWN;
   UsrDat->Surname1[0]  = '\0';
   UsrDat->Surname2[0]  = '\0';
   UsrDat->FrstName[0] = '\0';
   UsrDat->FullName[0]  = '\0';

   UsrDat->Email[0] = '\0';
   UsrDat->EmailConfirmed = false;

   UsrDat->Photo[0] = '\0';
   UsrDat->PhotoVisibility = Pri_PHOTO_VIS_DEFAULT;
   UsrDat->BaPrfVisibility = Pri_BASIC_PROFILE_VIS_DEFAULT;
   UsrDat->ExPrfVisibility = Pri_EXTENDED_PROFILE_VIS_DEFAULT;

   UsrDat->CtyCod = -1L;
   UsrDat->StrBirthday[0] = '\0';
   UsrDat->Birthday.Day   = 0;
   UsrDat->Birthday.Month = 0;
   UsrDat->Birthday.Year  = 0;
   UsrDat->Phone[0][0]    =
   UsrDat->Phone[1][0]    = '\0';
   if (UsrDat->Comments)
      UsrDat->Comments[0] = '\0';

   UsrDat->InsCtyCod  = -1L;
   UsrDat->InsCod     = -1L;
   UsrDat->Tch.CtrCod = -1L;
   UsrDat->Tch.DptCod = -1L;
   UsrDat->Tch.Office[0]      = '\0';
   UsrDat->Tch.OfficePhone[0] = '\0';

   UsrDat->Prefs.Language       = Lan_LANGUAGE_UNKNOWN;			// Language unknown
   UsrDat->Prefs.FirstDayOfWeek = Cal_FIRST_DAY_OF_WEEK_DEFAULT;	// Default first day of week
   UsrDat->Prefs.DateFormat     = Dat_FORMAT_DEFAULT		;	// Default date format
   UsrDat->Prefs.Theme          = The_THEME_DEFAULT;
   UsrDat->Prefs.IconSet        = Ico_ICON_SET_DEFAULT;
   UsrDat->Prefs.Menu           = Mnu_MENU_DEFAULT;
   UsrDat->Prefs.SideCols       = Cfg_DEFAULT_COLUMNS;
   UsrDat->Prefs.AcceptThirdPartyCookies = false;	// By default, don't accept third party cookies
   UsrDat->NtfEvents.SendEmail = 0;       		// By default, don't notify anything
  }

/*****************************************************************************/
/**************************** Reset my last data *****************************/
/*****************************************************************************/

void Usr_ResetMyLastData (void)
  {
   Gbl.Usrs.Me.UsrLast.WhatToSearch  = Sch_WHAT_TO_SEARCH_DEFAULT;
   Gbl.Usrs.Me.UsrLast.LastHie.Scope = HieLvl_UNK;
   Gbl.Usrs.Me.UsrLast.LastHie.Cod   = -1L;
   Gbl.Usrs.Me.UsrLast.LastAct       = ActUnk;
   Gbl.Usrs.Me.UsrLast.LastRole      = Rol_UNK;
   Gbl.Usrs.Me.UsrLast.LastTime      = 0;
   Gbl.Usrs.Me.UsrLast.LastAccNotif  = 0;
  }

/*****************************************************************************/
/**************** Free memory used to store the data of a user ***************/
/*****************************************************************************/

void Usr_UsrDataDestructor (struct UsrData *UsrDat)
  {
   /***** Free memory allocated for comments *****/
   if (UsrDat->Comments)
     {
      free (UsrDat->Comments);
      UsrDat->Comments = NULL;
     }

   /***** Free memory allocated for list of IDs *****/
   ID_FreeListIDs (UsrDat);
  }

/*****************************************************************************/
/************* Get all the user's data from a given user's code **************/
/*****************************************************************************/
// Input: UsrDat->UsrCod must hold user's code

void Usr_GetAllUsrDataFromUsrCod (struct UsrData *UsrDat,
                                  Usr_GetPrefs_t GetPrefs,
                                  Usr_GetRoleInCurrentCrs_t GetRoleInCurrentCrs)
  {
   ID_GetListIDsFromUsrCod (UsrDat);
   Usr_GetUsrDataFromUsrCod (UsrDat,GetPrefs,GetRoleInCurrentCrs);
  }

/*****************************************************************************/
/**************** Allocate memory for the list of users' codes ***************/
/*****************************************************************************/

void Usr_AllocateListUsrCods (struct ListUsrCods *ListUsrCods)
  {
   if ((ListUsrCods->Lst = malloc (ListUsrCods->NumUsrs *
                                   sizeof (*ListUsrCods->Lst))) == NULL)
      Err_NotEnoughMemoryExit ();
  }

/*****************************************************************************/
/****************** Free memory for the list of users' codes *****************/
/*****************************************************************************/

void Usr_FreeListUsrCods (struct ListUsrCods *ListUsrCods)
  {
   if (ListUsrCods->NumUsrs && ListUsrCods->Lst)
     {
      free (ListUsrCods->Lst);
      ListUsrCods->Lst = NULL;
      ListUsrCods->NumUsrs = 0;
     }
  }

/*****************************************************************************/
/************************ Check if I am a given user *************************/
/*****************************************************************************/

bool Usr_ItsMe (long UsrCod)
  {
   return Gbl.Usrs.Me.Logged &&
	  (UsrCod == Gbl.Usrs.Me.UsrDat.UsrCod);
  }

/*****************************************************************************/
/******** Get user's code from database using encrypted user's code **********/
/*****************************************************************************/
// Input: UsrDat->EncryptedUsrCod must hold user's encrypted code

void Usr_GetUsrCodFromEncryptedUsrCod (struct UsrData *UsrDat)
  {
   MYSQL_RES *mysql_res;
   MYSQL_ROW row;

   if (UsrDat->EnUsrCod[0])
     {
      /***** Get user's code from database *****/
      if (DB_QuerySELECT (&mysql_res,"can not get user's code",
			  "SELECT UsrCod"
			   " FROM usr_data"
			  " WHERE EncryptedUsrCod='%s'",
			  UsrDat->EnUsrCod) != 1)
         Err_ShowErrorAndExit ("Error when getting user's code.");

      /***** Get user's code *****/
      row = mysql_fetch_row (mysql_res);
      UsrDat->UsrCod = Str_ConvertStrCodToLongCod (row[0]);

      /***** Free structure that stores the query result *****/
      DB_FreeMySQLResult (&mysql_res);
     }
   else
      UsrDat->UsrCod = -1L;
  }

/*****************************************************************************/
/************ Get user's data from database giving a user's code *************/
/*****************************************************************************/
// UsrDat->UsrCod must contain an existing user's code

void Usr_GetUsrDataFromUsrCod (struct UsrData *UsrDat,
                               Usr_GetPrefs_t GetPrefs,
                               Usr_GetRoleInCurrentCrs_t GetRoleInCurrentCrs)
  {
   extern const char *Ico_IconSetId[Ico_NUM_ICON_SETS];
   extern const char *The_ThemeId[The_NUM_THEMES];
   extern const char *Lan_STR_LANG_ID[1 + Lan_NUM_LANGUAGES];
   MYSQL_RES *mysql_res;
   MYSQL_ROW row;
   unsigned NumRows;
   The_Theme_t Theme;
   Ico_IconSet_t IconSet;
   Lan_Language_t Lan;

   /***** Get user's data from database *****/
   switch (GetPrefs)
     {
      case Usr_DONT_GET_PREFS:
	 NumRows = (unsigned)
	 DB_QuerySELECT (&mysql_res,"can not get user's data",
		         "SELECT EncryptedUsrCod,"	// row[ 0]
			        "Password,"		// row[ 1]
			        "Surname1,"		// row[ 2]
			        "Surname2,"		// row[ 3]
			        "FirstName,"		// row[ 4]
			        "Sex,"			// row[ 5]
			        "Photo,"		// row[ 6]
			        "PhotoVisibility,"	// row[ 7]
			        "BaPrfVisibility,"	// row[ 8]
			        "ExPrfVisibility,"	// row[ 9]
			        "CtyCod,"		// row[10]
			        "InsCtyCod,"		// row[11]
			        "InsCod,"		// row[12]
			        "DptCod,"		// row[13]
			        "CtrCod,"		// row[14]
			        "Office,"		// row[15]
			        "OfficePhone,"		// row[16]
			        "LocalPhone,"		// row[17]
			        "FamilyPhone,"		// row[18]
			        "DATE_FORMAT(Birthday,"
			        "'%%Y%%m%%d'),"		// row[19]
			        "Comments,"		// row[20]
			        "NotifNtfEvents,"	// row[21]
			        "EmailNtfEvents"	// row[22]
			  " FROM usr_data"
		         " WHERE UsrCod=%ld",
		          UsrDat->UsrCod);
	 break;
      case Usr_GET_PREFS:
      default:
	 NumRows = (unsigned)
	 DB_QuerySELECT (&mysql_res,"can not get user's data",
		         "SELECT EncryptedUsrCod,"	// row[ 0]
			        "Password,"		// row[ 1]
			        "Surname1,"		// row[ 2]
			        "Surname2,"		// row[ 3]
			        "FirstName,"		// row[ 4]
			        "Sex,"			// row[ 5]
			        "Photo,"		// row[ 6]
			        "PhotoVisibility,"	// row[ 7]
			        "BaPrfVisibility,"	// row[ 8]
			        "ExPrfVisibility,"	// row[ 9]
			        "CtyCod,"		// row[10]
			        "InsCtyCod,"		// row[11]
			        "InsCod,"		// row[12]
			        "DptCod,"		// row[13]
			        "CtrCod,"		// row[14]
			        "Office,"		// row[15]
			        "OfficePhone,"		// row[16]
			        "LocalPhone,"		// row[17]
			        "FamilyPhone,"		// row[18]
			        "DATE_FORMAT(Birthday,"
			        "'%%Y%%m%%d'),"		// row[19]
			        "Comments,"		// row[20]
			        "NotifNtfEvents,"	// row[21]
			        "EmailNtfEvents,"	// row[22]

			        // Settings (usually not necessary
			        // when getting another user's data)
			        "Language,"		// row[23]
			        "FirstDayOfWeek,"	// row[24]
			        "DateFormat,"		// row[25]
			        "Theme,"		// row[26]
			        "IconSet,"		// row[27]
			        "Menu,"			// row[28]
			        "SideCols,"		// row[29]
			        "ThirdPartyCookies"	// row[30]
			  " FROM usr_data"
		         " WHERE UsrCod=%ld",
		         UsrDat->UsrCod);
         break;
     }

   if (NumRows != 1)
      Err_WrongUserExit ();

   /***** Read user's data *****/
   row = mysql_fetch_row (mysql_res);

   /* Get encrypted user's code (row[0]) */
   Str_Copy (UsrDat->EnUsrCod,row[0],sizeof (UsrDat->EnUsrCod) - 1);

   /* Get encrypted password (row[1]) */
   Str_Copy (UsrDat->Password,row[1],sizeof (UsrDat->Password) - 1);

   /* Get roles */
   switch (GetRoleInCurrentCrs)
     {
      case Usr_DONT_GET_ROLE_IN_CURRENT_CRS:
	 UsrDat->Roles.InCurrentCrs = Rol_UNK;
	 UsrDat->Roles.InCrss = -1;
	 break;
      case Usr_GET_ROLE_IN_CURRENT_CRS:
	 UsrDat->Roles.InCurrentCrs = Rol_GetRoleUsrInCrs (UsrDat->UsrCod,
							   Gbl.Hierarchy.Crs.CrsCod);
	 UsrDat->Roles.InCrss = -1;	// Force roles to be got from database
	 // Rol_GetRolesInAllCrss (UsrDat);
	 break;
     }

   /* Get name (row[2], row[3], row[4]) */
   Str_Copy (UsrDat->Surname1,row[2],sizeof (UsrDat->Surname1) - 1);
   Str_Copy (UsrDat->Surname2,row[3],sizeof (UsrDat->Surname2) - 1);
   Str_Copy (UsrDat->FrstName,row[4],sizeof (UsrDat->FrstName) - 1);
   Str_ConvertToTitleType (UsrDat->Surname1 );
   Str_ConvertToTitleType (UsrDat->Surname2 );
   Str_ConvertToTitleType (UsrDat->FrstName);
   Usr_BuildFullName (UsrDat);	// Create full name using FirstName, Surname1 and Surname2

   /* Get sex (row[5]) */
   UsrDat->Sex = Usr_GetSexFromStr (row[5]);

   /* Get photo (row[6]) */
   Str_Copy (UsrDat->Photo,row[6],sizeof (UsrDat->Photo) - 1);

   /* Get photo visibility (row[7]) */
   UsrDat->PhotoVisibility = Pri_GetVisibilityFromStr (row[7]);

   /* Get profile visibility (row[8], row[9]) */
   UsrDat->BaPrfVisibility = Pri_GetVisibilityFromStr (row[8]);
   UsrDat->ExPrfVisibility = Pri_GetVisibilityFromStr (row[9]);

   /* Get country (row[10]) */
   UsrDat->CtyCod = Str_ConvertStrCodToLongCod (row[10]);

   /* Get institution country (row[11]) and institution (row[12]) */
   UsrDat->InsCtyCod = Str_ConvertStrCodToLongCod (row[11]);
   UsrDat->InsCod    = Str_ConvertStrCodToLongCod (row[12]);

   /* Get department (row[13]) */
   UsrDat->Tch.DptCod = Str_ConvertStrCodToLongCod (row[13]);

   /* Get center (row[14]) */
   UsrDat->Tch.CtrCod = Str_ConvertStrCodToLongCod (row[14]);

   /* Get office (row[15]) and office phone (row[16]) */
   Str_Copy (UsrDat->Tch.Office     ,row[15],sizeof (UsrDat->Tch.Office     ) - 1);
   Str_Copy (UsrDat->Tch.OfficePhone,row[16],sizeof (UsrDat->Tch.OfficePhone) - 1);

   /* Get phones (row[17]) and row[18] */
   Str_Copy (UsrDat->Phone[0],row[17],sizeof (UsrDat->Phone[0]) - 1);
   Str_Copy (UsrDat->Phone[1],row[18],sizeof (UsrDat->Phone[1]) - 1);

   /* Get birthday (row[19]) */
   Dat_GetDateFromYYYYMMDD (&(UsrDat->Birthday),row[19]);
   Dat_ConvDateToDateStr (&(UsrDat->Birthday),UsrDat->StrBirthday);

   /* Get comments (row[20]) */
   Usr_GetUsrCommentsFromString (row[20] ? row[20] :
	                                   "",
	                         UsrDat);

   /* Get on which events the user wants to be notified inside the platform (row[21]) */
   if (sscanf (row[21],"%u",&UsrDat->NtfEvents.CreateNotif) != 1)
      UsrDat->NtfEvents.CreateNotif = (unsigned) -1;	// 0xFF..FF

   /* Get on which events the user wants to be notified by email (row[22]) */
   if (sscanf (row[22],"%u",&UsrDat->NtfEvents.SendEmail) != 1)
      UsrDat->NtfEvents.SendEmail = 0;
   if (UsrDat->NtfEvents.SendEmail >= (1 << Ntf_NUM_NOTIFY_EVENTS))	// Maximum binary value for NotifyEvents is 000...0011...11
      UsrDat->NtfEvents.SendEmail = 0;

   /***** Get user's settings *****/
   if (GetPrefs == Usr_GET_PREFS)
     {
      /* Get language (row[23]) */
      UsrDat->Prefs.Language = Lan_LANGUAGE_UNKNOWN;	// Language unknown
      for (Lan  = (Lan_Language_t) 1;
	   Lan <= (Lan_Language_t) Lan_NUM_LANGUAGES;
	   Lan++)
	 if (!strcasecmp (row[23],Lan_STR_LANG_ID[Lan]))
	   {
	    UsrDat->Prefs.Language = Lan;
	    break;
	   }

      /* Get first day of week (row[24]) */
      UsrDat->Prefs.FirstDayOfWeek = Cal_GetFirstDayOfWeekFromStr (row[24]);

      /* Get date format (row[25]) */
      UsrDat->Prefs.DateFormat = Dat_GetDateFormatFromStr (row[25]);

      /* Get theme (row[26]) */
      UsrDat->Prefs.Theme = The_THEME_DEFAULT;
      for (Theme  = (The_Theme_t) 0;
	   Theme <= (The_Theme_t) (The_NUM_THEMES - 1);
	   Theme++)
	 if (!strcasecmp (row[26],The_ThemeId[Theme]))
	   {
	    UsrDat->Prefs.Theme = Theme;
	    break;
	   }

      /* Get icon set (row[27]) */
      UsrDat->Prefs.IconSet = Ico_ICON_SET_DEFAULT;
      for (IconSet  = (Ico_IconSet_t) 0;
	   IconSet <= (Ico_IconSet_t) (Ico_NUM_ICON_SETS - 1);
	   IconSet++)
	 if (!strcasecmp (row[27],Ico_IconSetId[IconSet]))
	   {
	    UsrDat->Prefs.IconSet = IconSet;
	    break;
	   }

      /* Get menu (row[28]) */
      UsrDat->Prefs.Menu = Mnu_GetMenuFromStr (row[28]);

      /* Get if user wants to show side columns (row[29]) */
      if (sscanf (row[29],"%u",&UsrDat->Prefs.SideCols) == 1)
	{
	 if (UsrDat->Prefs.SideCols > Lay_SHOW_BOTH_COLUMNS)
	    UsrDat->Prefs.SideCols = Cfg_DEFAULT_COLUMNS;
	}
      else
	 UsrDat->Prefs.SideCols = Cfg_DEFAULT_COLUMNS;

      /* Get if user accepts third party cookies (row[30]) */
      UsrDat->Prefs.AcceptThirdPartyCookies = (row[30][0] == 'Y');
     }

   /***** Free structure that stores the query result *****/
   DB_FreeMySQLResult (&mysql_res);

   /***** Get nickname and email *****/
   Nck_DB_GetNicknameFromUsrCod (UsrDat->UsrCod,UsrDat->Nickname);
   Mai_GetEmailFromUsrCod (UsrDat);
  }

/*****************************************************************************/
/********* Get the comments in the record of a user from a string ************/
/*****************************************************************************/

static void Usr_GetUsrCommentsFromString (char *Str,struct UsrData *UsrDat)
  {
   /***** Check that memory for comments is allocated *****/
   if (UsrDat->Comments)
      /***** Copy comments from Str to Comments *****/
      Str_Copy (UsrDat->Comments,Str,Cns_MAX_BYTES_TEXT - 1);
  }

/*****************************************************************************/
/********** Get user's last data from database giving a user's code **********/
/*****************************************************************************/

static void Usr_GetMyLastData (void)
  {
   MYSQL_RES *mysql_res;
   MYSQL_ROW row;
   unsigned NumRows;
   unsigned UnsignedNum;
   long ActCod;

   /***** Get user's last data from database *****/
   NumRows = (unsigned)
   DB_QuerySELECT (&mysql_res,"can not get user's last data",
		   "SELECT WhatToSearch,"			// row[0]
			  "LastSco,"				// row[1]
			  "LastCod,"				// row[2]
			  "LastAct,"				// row[3]
			  "LastRole,"				// row[4]
			  "UNIX_TIMESTAMP(LastTime),"		// row[5]
			  "UNIX_TIMESTAMP(LastAccNotif)"	// row[6]
		    " FROM usr_last"
		   " WHERE UsrCod=%ld",
		   Gbl.Usrs.Me.UsrDat.UsrCod);
   if (NumRows == 0)
     {
      /***** Free structure that stores the query result *****/
      DB_FreeMySQLResult (&mysql_res);

      /***** Create entry for me in table of user's last data *****/
      Usr_ResetMyLastData ();
      Usr_InsertMyLastData ();
     }
   else if (NumRows == 1)
     {
      row = mysql_fetch_row (mysql_res);

      /* Get last type of search (row[0]) */
      Gbl.Usrs.Me.UsrLast.WhatToSearch = Sch_SEARCH_UNKNOWN;
      if (sscanf (row[0],"%u",&UnsignedNum) == 1)
         if (UnsignedNum < Sch_NUM_WHAT_TO_SEARCH)
            Gbl.Usrs.Me.UsrLast.WhatToSearch = (Sch_WhatToSearch_t) UnsignedNum;
      if (Gbl.Usrs.Me.UsrLast.WhatToSearch == Sch_SEARCH_UNKNOWN)
	 Gbl.Usrs.Me.UsrLast.WhatToSearch = Sch_WHAT_TO_SEARCH_DEFAULT;

      /* Get last hierarchy: scope (row[1]) and code (row[2]) */
      Gbl.Usrs.Me.UsrLast.LastHie.Scope = Sco_GetScopeFromDBStr (row[1]);
      switch (Gbl.Usrs.Me.UsrLast.LastHie.Scope)
        {
         case HieLvl_SYS:	// System
            Gbl.Usrs.Me.UsrLast.LastHie.Cod = -1L;
            break;
         case HieLvl_CTY:	// Country
         case HieLvl_INS:	// Institution
         case HieLvl_CTR:	// Center
         case HieLvl_DEG:	// Degree
         case HieLvl_CRS:	// Course
            Gbl.Usrs.Me.UsrLast.LastHie.Cod = Str_ConvertStrCodToLongCod (row[2]);
            if (Gbl.Usrs.Me.UsrLast.LastHie.Cod <= 0)
              {
               Gbl.Usrs.Me.UsrLast.LastHie.Scope = HieLvl_UNK;
               Gbl.Usrs.Me.UsrLast.LastHie.Cod = -1L;
              }
            break;
         default:
            Gbl.Usrs.Me.UsrLast.LastHie.Scope = HieLvl_UNK;
            Gbl.Usrs.Me.UsrLast.LastHie.Cod = -1L;
            break;
        }

      /* Get last action (row[3]) */
      ActCod = Str_ConvertStrCodToLongCod (row[3]);
      Gbl.Usrs.Me.UsrLast.LastAct = Act_GetActionFromActCod (ActCod);

      /* Get last role (row[4]) */
      Gbl.Usrs.Me.UsrLast.LastRole = Rol_ConvertUnsignedStrToRole (row[4]);

      /* Get last access to platform (row[5]) */
      Gbl.Usrs.Me.UsrLast.LastTime = 0L;
      if (row[5])
         sscanf (row[5],"%ld",&(Gbl.Usrs.Me.UsrLast.LastTime));

      /* Get last access to notifications (row[6]) */
      Gbl.Usrs.Me.UsrLast.LastAccNotif = 0L;
      if (row[6])
         sscanf (row[6],"%ld",&(Gbl.Usrs.Me.UsrLast.LastAccNotif));

      /***** Free structure that stores the query result *****/
      DB_FreeMySQLResult (&mysql_res);
     }
   else
      Err_ShowErrorAndExit ("Error when getting user's last data.");
  }

/*****************************************************************************/
/****************************** Get sex from string **************************/
/*****************************************************************************/

static Usr_Sex_t Usr_GetSexFromStr (const char *Str)
  {
   Usr_Sex_t Sex;

   for (Sex  = (Usr_Sex_t) 0;
        Sex <= (Usr_Sex_t) (Usr_NUM_SEXS - 1);
        Sex++)
      if (!strcasecmp (Str,Usr_StringsSexDB[Sex]))
         return Sex;

   return Usr_SEX_UNKNOWN;
  }

/*****************************************************************************/
/********** Build full name using FirstName, Surname1 and Surname2 ***********/
/*****************************************************************************/

void Usr_BuildFullName (struct UsrData *UsrDat)
  {
   Str_Copy (UsrDat->FullName,UsrDat->FrstName,sizeof (UsrDat->FullName) - 1);
   if (UsrDat->Surname1[0])
     {
      Str_Concat (UsrDat->FullName," "             ,sizeof (UsrDat->FullName) - 1);
      Str_Concat (UsrDat->FullName,UsrDat->Surname1,sizeof (UsrDat->FullName) - 1);
     }
   if (UsrDat->Surname2[0])
     {
      Str_Concat (UsrDat->FullName," "             ,sizeof (UsrDat->FullName) - 1);
      Str_Concat (UsrDat->FullName,UsrDat->Surname2,sizeof (UsrDat->FullName) - 1);
     }
  }

/*****************************************************************************/
/********* Write user name in two lines. 1: first name, 2: surnames **********/
/*****************************************************************************/

void Usr_WriteFirstNameBRSurnames (const struct UsrData *UsrDat)
  {
   /***** Write first name and surname 1 *****/
   HTM_Txt (UsrDat->FrstName);
   HTM_BR ();
   HTM_Txt (UsrDat->Surname1);

   /***** Write surname2 if exists *****/
   if (UsrDat->Surname2[0])
      HTM_TxtF ("&nbsp;%s",UsrDat->Surname2);
  }

/*****************************************************************************/
/********************* Flush all caches related to users *********************/
/*****************************************************************************/

void Usr_FlushCachesUsr (void)
  {
   Usr_FlushCacheUsrBelongsToIns ();
   Usr_FlushCacheUsrBelongsToCtr ();
   Usr_FlushCacheUsrBelongsToDeg ();
   Usr_FlushCacheUsrBelongsToCrs ();
   Usr_FlushCacheUsrBelongsToCurrentCrs ();
   Usr_FlushCacheUsrHasAcceptedInCurrentCrs ();
   Usr_FlushCacheUsrSharesAnyOfMyCrs ();
   Rol_FlushCacheMyRoleInCurrentCrs ();
   Rol_FlushCacheRoleUsrInCrs ();
   Grp_FlushCacheUsrSharesAnyOfMyGrpsInCurrentCrs ();
   Grp_FlushCacheIBelongToGrp ();
   Fol_FlushCacheFollow ();
  }

/*****************************************************************************/
/***** Check if a user is an administrator of a degree/center/institution ****/
/*****************************************************************************/

bool Usr_CheckIfUsrIsAdm (long UsrCod,HieLvl_Level_t Scope,long Cod)
  {
   /***** Get if a user is administrator of a degree from database *****/
   return (DB_QueryCOUNT ("can not check if a user is administrator",
			  "SELECT COUNT(*)"
			   " FROM usr_admins"
			  " WHERE UsrCod=%ld"
			    " AND Scope='%s'"
			    " AND Cod=%ld",
			  UsrCod,
			  Sco_GetDBStrFromScope (Scope),
			  Cod) != 0);
  }

/*****************************************************************************/
/********************* Check if a user is a superuser ************************/
/*****************************************************************************/

void Usr_FlushCacheUsrIsSuperuser (void)
  {
   Gbl.Cache.UsrIsSuperuser.UsrCod = -1L;
   Gbl.Cache.UsrIsSuperuser.IsSuperuser = false;
  }

bool Usr_CheckIfUsrIsSuperuser (long UsrCod)
  {
   /***** 1. Fast check: Trivial case *****/
   if (UsrCod <= 0)
      return false;

   /***** 2. Fast check: If cached... *****/
   if (UsrCod == Gbl.Cache.UsrIsSuperuser.UsrCod)
      return Gbl.Cache.UsrIsSuperuser.IsSuperuser;

   /***** 3. Slow check: If not cached, get if a user is superuser from database *****/
   Gbl.Cache.UsrIsSuperuser.UsrCod = UsrCod;
   Gbl.Cache.UsrIsSuperuser.IsSuperuser =
      (DB_QueryCOUNT ("can not check if a user is superuser",
		      "SELECT COUNT(*)"
		       " FROM usr_admins"
		      " WHERE UsrCod=%ld"
		        " AND Scope='%s'",
		      UsrCod,
		      Sco_GetDBStrFromScope (HieLvl_SYS)) != 0);
   return Gbl.Cache.UsrIsSuperuser.IsSuperuser;
  }

/*****************************************************************************/
/**************** Check if I can change another user's data ******************/
/*****************************************************************************/

bool Usr_ICanChangeOtherUsrData (const struct UsrData *UsrDat)
  {
   bool ItsMe = Usr_ItsMe (UsrDat->UsrCod);

   if (ItsMe)
      return true;

   /***** Check if I have permission to see another user's IDs *****/
   switch (Gbl.Usrs.Me.Role.Logged)
     {
      case Rol_TCH:
	 /* Check 1: I can change data of users who do not exist in database */
         if (UsrDat->UsrCod <= 0)	// User does not exist (when creating a new user)
            return true;

         /* Check 2: I change data of users without password */
         if (!UsrDat->Password[0])	// User has no password (never logged)
            return true;

         return false;
      case Rol_DEG_ADM:
      case Rol_CTR_ADM:
      case Rol_INS_ADM:
      case Rol_SYS_ADM:
         return Usr_ICanEditOtherUsr (UsrDat);
      default:
	 return false;
     }
  }

/*****************************************************************************/
/***************** Check if I can edit another user's data *******************/
/*****************************************************************************/

bool Usr_ICanEditOtherUsr (const struct UsrData *UsrDat)
  {
   bool ItsMe = Usr_ItsMe (UsrDat->UsrCod);

   if (ItsMe)
      return true;

   switch (Gbl.Usrs.Me.Role.Logged)
     {
      case Rol_DEG_ADM:
	 /* If I am an administrator of current degree,
	    I only can edit users from current degree who have accepted */
	 if (Usr_CheckIfUsrBelongsToDeg (UsrDat->UsrCod,Gbl.Hierarchy.Deg.DegCod))
	    // Degree admins can't edit superusers' data
	    if (!Usr_CheckIfUsrIsSuperuser (UsrDat->UsrCod))
	       return true;
	 return false;
      case Rol_CTR_ADM:
	 /* If I am an administrator of current center,
	    I only can edit from current center who have accepted */
	 if (Usr_CheckIfUsrBelongsToCtr (UsrDat->UsrCod,Gbl.Hierarchy.Ctr.CtrCod))
	    // Center admins can't edit superusers' data
	    if (!Usr_CheckIfUsrIsSuperuser (UsrDat->UsrCod))
	       return true;
	 return false;
      case Rol_INS_ADM:
	 /* If I am an administrator of current institution,
	    I only can edit from current institution who have accepted */
	 if (Usr_CheckIfUsrBelongsToIns (UsrDat->UsrCod,Gbl.Hierarchy.Ins.InsCod))
	    // Institution admins can't edit superusers' data
	    if (!Usr_CheckIfUsrIsSuperuser (UsrDat->UsrCod))
	       return true;
	 return false;
      case Rol_SYS_ADM:
	 return true;
      default:
	 return false;
     }
  }

/*****************************************************************************/
/********************* Get number of courses of a user ***********************/
/*****************************************************************************/

unsigned Usr_GetNumCrssOfUsr (long UsrCod)
  {
   /***** Get the number of courses of a user from database ******/
   return (unsigned)
   DB_QueryCOUNT ("can not get the number of courses of a user",
		  "SELECT COUNT(*)"
		   " FROM crs_users"
		  " WHERE UsrCod=%ld",
		  UsrCod);
  }

/*****************************************************************************/
/*************** Get number of courses of a user not accepted ****************/
/*****************************************************************************/

unsigned Usr_GetNumCrssOfUsrNotAccepted (long UsrCod)
  {
   /***** Get the number of courses of a user not accepted from database ******/
   return (unsigned)
   DB_QueryCOUNT ("can not get the number of courses of a user",
		  "SELECT COUNT(*)"
		   " FROM crs_users"
		  " WHERE UsrCod=%ld"
		    " AND Accepted='N'",
		  UsrCod);
  }

/*****************************************************************************/
/********* Get number of courses in with a user have a given role ************/
/*****************************************************************************/

unsigned Usr_GetNumCrssOfUsrWithARole (long UsrCod,Rol_Role_t Role)
  {
   /***** Get the number of courses of a user with a role from database ******/
   return (unsigned)
   DB_QueryCOUNT ("can not get the number of courses of a user with a role",
		  "SELECT COUNT(*)"
		   " FROM crs_users"
		  " WHERE UsrCod=%ld"
		    " AND Role=%u",
		  UsrCod,
		  (unsigned) Role);
  }

/*****************************************************************************/
/********* Get number of courses in with a user have a given role ************/
/*****************************************************************************/

unsigned Usr_GetNumCrssOfUsrWithARoleNotAccepted (long UsrCod,Rol_Role_t Role)
  {
   /***** Get the number of courses of a user with a role from database ******/
   return (unsigned)
   DB_QueryCOUNT ("can not get the number of courses of a user with a role",
		  "SELECT COUNT(*)"
		   " FROM crs_users"
		  " WHERE UsrCod=%ld"
		    " AND Role=%u"
		    " AND Accepted='N'",
		  UsrCod,
		  (unsigned) Role);
  }

/*****************************************************************************/
/****** Get number of users with some given roles in courses of a user *******/
/*****************************************************************************/

#define Usr_MAX_BYTES_ROLES_STR (Rol_NUM_ROLES * (Cns_MAX_DECIMAL_DIGITS_UINT + 1))
unsigned Usr_GetNumUsrsInCrssOfAUsr (long UsrCod,Rol_Role_t UsrRole,
                                     unsigned OthersRoles)
  {
   Rol_Role_t Role;
   char UnsignedStr[Cns_MAX_DECIMAL_DIGITS_UINT + 1];
   char OthersRolesStr[Usr_MAX_BYTES_ROLES_STR + 1];
   char SubQueryRole[64];
   unsigned NumUsrs;
   // This query can be made in a unique, but slower, query
   // The temporary table achieves speedup from ~2s to few ms

   /***** Remove temporary table if exists *****/
   DB_Query ("can not remove temporary tables",
	     "DROP TEMPORARY TABLE IF EXISTS usr_courses_tmp");

   /***** Create temporary table with all user's courses
          as student/non-editing teacher/teacher *****/
   switch (UsrRole)
     {
      case Rol_STD:	// Student
	 sprintf (SubQueryRole," AND Role=%u",
	          (unsigned) Rol_STD);
	 break;
      case Rol_NET:	// Non-editing teacher
	 sprintf (SubQueryRole," AND Role=%u",
	          (unsigned) Rol_NET);
	 break;
      case Rol_TCH:	// or teacher
	 sprintf (SubQueryRole," AND Role=%u",
	          (unsigned) Rol_TCH);
	 break;
      default:
	 SubQueryRole[0] = '\0';
	 Err_WrongRoleExit ();
	 break;
     }
   DB_Query ("can not create temporary table",
	     "CREATE TEMPORARY TABLE IF NOT EXISTS usr_courses_tmp"
	     " (CrsCod INT NOT NULL,UNIQUE INDEX (CrsCod))"
	     " ENGINE=MEMORY"
	     " SELECT CrsCod"
	       " FROM crs_users"
	      " WHERE UsrCod=%ld"
	         "%s",
	     UsrCod,SubQueryRole);

   /***** Get the number of students/teachers in a course from database ******/
   OthersRolesStr[0] = '\0';
   for (Role =  Rol_STD;	// First possible role in a course
	Role <= Rol_TCH;	// Last possible role in a course
	Role++)
      if ((OthersRoles & (1 << Role)))
        {
         sprintf (UnsignedStr,"%u",(unsigned) Role);
         if (OthersRolesStr[0])	// Not empty
	    Str_Concat (OthersRolesStr,",",sizeof (OthersRolesStr) - 1);
	 Str_Concat (OthersRolesStr,UnsignedStr,sizeof (OthersRolesStr) - 1);
        }
   NumUsrs = (unsigned)
   DB_QueryCOUNT ("can not get number of users",
		  "SELECT COUNT(DISTINCT crs_users.UsrCod)"
		   " FROM crs_users,"
			 "usr_courses_tmp"
		  " WHERE crs_users.CrsCod=usr_courses_tmp.CrsCod"
		    " AND crs_users.Role IN (%s)",
		  OthersRolesStr);

   /***** Remove temporary table *****/
   DB_Query ("can not remove temporary tables",
	     "DROP TEMPORARY TABLE IF EXISTS usr_courses_tmp");

   return NumUsrs;
  }

/*****************************************************************************/
/************ Check if I can view the record card of a student ***************/
/*****************************************************************************/

bool Usr_CheckIfICanViewRecordStd (const struct UsrData *UsrDat)
  {
   bool ItsMe;

   /***** 1. Fast check: Am I logged? *****/
   if (!Gbl.Usrs.Me.Logged)
      return false;

   /***** 2. Fast check: Is it a valid user code? *****/
   if (UsrDat->UsrCod <= 0)
      return false;

   /***** 3. Fast check: Is it a course selected? *****/
   if (Gbl.Hierarchy.Crs.CrsCod <= 0)
      return false;

   /***** 4. Fast check: Is he/she a student? *****/
   if (UsrDat->Roles.InCurrentCrs != Rol_STD)
      return false;

   /***** 5. Fast check: Am I a system admin? *****/
   if (Gbl.Usrs.Me.Role.Logged == Rol_SYS_ADM)
      return true;

   /***** 6. Fast check: Do I belong to the current course? *****/
   if (!Gbl.Usrs.Me.IBelongToCurrentCrs)
      return false;

   /***** 7. Fast check: It's me? *****/
   ItsMe = Usr_ItsMe (UsrDat->UsrCod);
   if (ItsMe)
      return true;

   /***** 8. Fast / slow check: Does he/she belong to the current course? *****/
   if (!Usr_CheckIfUsrBelongsToCurrentCrs (UsrDat))
      return false;

   /***** 9. Fast / slow check depending on roles *****/
   switch (Gbl.Usrs.Me.Role.Logged)
     {
      case Rol_STD:
      case Rol_NET:
	 return Grp_CheckIfUsrSharesAnyOfMyGrpsInCurrentCrs (UsrDat);
      case Rol_TCH:
	 return true;
      default:
	 return false;
     }
  }

/*****************************************************************************/
/************ Check if I can view the record card of a teacher ***************/
/*****************************************************************************/
// Teacher here is intended as:
// - a non-editing teacher
// - or a teacher

bool Usr_CheckIfICanViewRecordTch (struct UsrData *UsrDat)
  {
   /***** 1. Fast check: Am I logged? *****/
   if (!Gbl.Usrs.Me.Logged)
      return false;

   /***** 2. Fast check: Is it a valid user code? *****/
   if (UsrDat->UsrCod <= 0)
      return false;

   /***** 3. Fast check: Is it a course selected? *****/
   if (Gbl.Hierarchy.Crs.CrsCod <= 0)
      return false;

   /***** 4. Fast check: Is he/she a non-editing teacher or a teacher? *****/
   return (UsrDat->Roles.InCurrentCrs == Rol_NET ||
           UsrDat->Roles.InCurrentCrs == Rol_TCH);
  }

/*****************************************************************************/
/********* Check if I can view test/exam/match result of another user ********/
/*****************************************************************************/

bool Usr_CheckIfICanViewTstExaMchResult (const struct UsrData *UsrDat)
  {
   bool ItsMe;

   /***** 1. Fast check: Am I logged? *****/
   if (!Gbl.Usrs.Me.Logged)
      return false;

   /***** 2. Fast check: Is it a valid user code? *****/
   if (UsrDat->UsrCod <= 0)
      return false;

   /***** 3. Fast check: Is it a course selected? *****/
   if (Gbl.Hierarchy.Crs.CrsCod <= 0)
      return false;

   /***** 4. Fast check: Am I a system admin? *****/
   if (Gbl.Usrs.Me.Role.Logged == Rol_SYS_ADM)
      return true;

   /***** 5. Fast check: Do I belong to the current course? *****/
   if (!Gbl.Usrs.Me.IBelongToCurrentCrs)
      return false;

   /***** 6. Fast check: It's me? *****/
   ItsMe = Usr_ItsMe (UsrDat->UsrCod);
   if (ItsMe)
      return true;

   /***** 7. Fast check: Does he/she belong to the current course? *****/
   if (!Usr_CheckIfUsrBelongsToCurrentCrs (UsrDat))
      return false;

   /***** 8. Fast / slow check depending on roles *****/
   switch (Gbl.Usrs.Me.Role.Logged)
     {
      case Rol_NET:
	 return Grp_CheckIfUsrSharesAnyOfMyGrpsInCurrentCrs (UsrDat);
      case Rol_TCH:
	 return true;
      default:
	 return false;
     }
  }

/*****************************************************************************/
/********** Check if I can view assigments / works of another user ***********/
/*****************************************************************************/

bool Usr_CheckIfICanViewAsgWrk (const struct UsrData *UsrDat)
  {
   bool ItsMe;

   /***** 1. Fast check: Am I logged? *****/
   if (!Gbl.Usrs.Me.Logged)
      return false;

   /***** 2. Fast check: Is it a valid user code? *****/
   if (UsrDat->UsrCod <= 0)
      return false;

   /***** 3. Fast check: Is it a course selected? *****/
   if (Gbl.Hierarchy.Crs.CrsCod <= 0)
      return false;

   /***** 4. Fast check: Does he/she belong to the current course? *****/
   // Only users beloging to course can have files in assignments/works
   if (!Usr_CheckIfUsrBelongsToCurrentCrs (UsrDat))
      return false;

   /***** 5. Fast check: Am I a system admin? *****/
   if (Gbl.Usrs.Me.Role.Logged == Rol_SYS_ADM)
      return true;

   /***** 6. Fast check: Do I belong to the current course? *****/
   if (!Gbl.Usrs.Me.IBelongToCurrentCrs)
      return false;

   /***** 7. Fast check: It's me? *****/
   ItsMe = Usr_ItsMe (UsrDat->UsrCod);
   if (ItsMe)
      return true;

   /***** 8. Fast / slow check depending on roles *****/
   switch (Gbl.Usrs.Me.Role.Logged)
     {
      case Rol_NET:
	 return Grp_CheckIfUsrSharesAnyOfMyGrpsInCurrentCrs (UsrDat);
      case Rol_TCH:
	 return true;
      default:
	 return false;
     }
  }

/*****************************************************************************/
/************** Check if I can view attendance of another user ***************/
/*****************************************************************************/

bool Usr_CheckIfICanViewAtt (const struct UsrData *UsrDat)
  {
   bool ItsMe;

   /***** 1. Fast check: Am I logged? *****/
   if (!Gbl.Usrs.Me.Logged)
      return false;

   /***** 2. Fast check: Is it a valid user code? *****/
   if (UsrDat->UsrCod <= 0)
      return false;

   /***** 3. Fast check: Is it a course selected? *****/
   if (Gbl.Hierarchy.Crs.CrsCod <= 0)
      return false;

   /***** 4. Fast check: Am I a system admin? *****/
   if (Gbl.Usrs.Me.Role.Logged == Rol_SYS_ADM)
      return true;

   /***** 5. Fast check: Do I belong to the current course? *****/
   if (!Gbl.Usrs.Me.IBelongToCurrentCrs)
      return false;

   /***** 6. Fast check: It's me? *****/
   ItsMe = Usr_ItsMe (UsrDat->UsrCod);
   if (ItsMe)
      return true;

   /***** 7. Fast / slow check depending on roles *****/
   switch (Gbl.Usrs.Me.Role.Logged)
     {
      case Rol_NET:
	 return Grp_CheckIfUsrSharesAnyOfMyGrpsInCurrentCrs (UsrDat);
      case Rol_TCH:
	 return true;
      default:
	 return false;
     }
  }

/*****************************************************************************/
/******************* Check if I can view a user's agenda *********************/
/*****************************************************************************/

bool Usr_CheckIfICanViewUsrAgenda (struct UsrData *UsrDat)
  {
   bool ItsMe;

   /***** 1. Fast check: Am I logged? *****/
   if (!Gbl.Usrs.Me.Logged)
      return false;

   /***** 2. Fast check: It's me? *****/
   ItsMe = Usr_ItsMe (UsrDat->UsrCod);
   if (ItsMe)
      return true;

   /***** 3. Fast check: Am I logged as system admin? *****/
   if (Gbl.Usrs.Me.Role.Logged == Rol_SYS_ADM)
      return true;

   /***** 4. Slow check: Get if user shares any course with me from database *****/
   return Usr_CheckIfUsrSharesAnyOfMyCrs (UsrDat);
  }

/*****************************************************************************/
/*************** Check if a user belongs to any of my courses ****************/
/*****************************************************************************/

void Usr_FlushCacheUsrSharesAnyOfMyCrs (void)
  {
   Gbl.Cache.UsrSharesAnyOfMyCrs.UsrCod = -1L;
   Gbl.Cache.UsrSharesAnyOfMyCrs.SharesAnyOfMyCrs = false;
  }

bool Usr_CheckIfUsrSharesAnyOfMyCrs (struct UsrData *UsrDat)
  {
   bool ItsMe;

   /***** 1. Fast check: Am I logged? *****/
   if (!Gbl.Usrs.Me.Logged)
      return false;

   /***** 2. Fast check: It is a valid user code? *****/
   if (UsrDat->UsrCod <= 0)
      return false;

   /***** 3. Fast check: It's me? *****/
   ItsMe = Usr_ItsMe (UsrDat->UsrCod);
   if (ItsMe)
      return true;

   /***** 4. Fast check: Is already calculated if user shares any course with me? *****/
   if (UsrDat->UsrCod == Gbl.Cache.UsrSharesAnyOfMyCrs.UsrCod)
      return Gbl.Cache.UsrSharesAnyOfMyCrs.SharesAnyOfMyCrs;

   /***** 5. Fast check: Is course selected and we both belong to it? *****/
   if (Gbl.Usrs.Me.IBelongToCurrentCrs)
      if (Usr_CheckIfUsrBelongsToCurrentCrs (UsrDat))	// Course selected and we both belong to it
         return true;

   /***** 6. Fast/slow check: Does he/she belong to any course? *****/
   Rol_GetRolesInAllCrss (UsrDat);
   if (!(UsrDat->Roles.InCrss & ((1 << Rol_STD) |	// Any of his/her roles is student
	                         (1 << Rol_NET) |	// or non-editing teacher
			         (1 << Rol_TCH))))	// or teacher?
      return false;

   /***** 7. Slow check: Get if user shares any course with me from database *****/
   /* Fill the list with the courses I belong to (if not already filled) */
   Usr_GetMyCourses ();

   /* Check if user shares any course with me */
   Gbl.Cache.UsrSharesAnyOfMyCrs.UsrCod = UsrDat->UsrCod;
   Gbl.Cache.UsrSharesAnyOfMyCrs.SharesAnyOfMyCrs =
      (DB_QueryCOUNT ("can not check if a user shares any course with you",
		      "SELECT COUNT(*)"
		       " FROM crs_users"
		      " WHERE UsrCod=%ld"
		        " AND CrsCod IN"
		            " (SELECT CrsCod"
		               " FROM my_courses_tmp)",
		      UsrDat->UsrCod) != 0);
   return Gbl.Cache.UsrSharesAnyOfMyCrs.SharesAnyOfMyCrs;
  }

/*****************************************************************************/
/*** Check if a user belongs to any of my courses but has a different role ***/
/*****************************************************************************/

bool Usr_CheckIfUsrSharesAnyOfMyCrsWithDifferentRole (long UsrCod)
  {
   bool UsrSharesAnyOfMyCrsWithDifferentRole;

   /***** 1. Fast check: Am I logged? *****/
   if (!Gbl.Usrs.Me.Logged)
      return false;

   /***** 2. Slow check: Get if user shares any course with me
                         with a different role, from database *****/
   /* Fill the list with the courses I belong to (if not already filled) */
   Usr_GetMyCourses ();

   /* Remove temporary table if exists */
   DB_Query ("can not remove temporary tables",
	     "DROP TEMPORARY TABLE IF EXISTS usr_courses_tmp");

   /* Create temporary table with all user's courses for a role */
   DB_Query ("can not create temporary table",
	     "CREATE TEMPORARY TABLE IF NOT EXISTS usr_courses_tmp "
	     "(CrsCod INT NOT NULL,Role TINYINT NOT NULL,"
   	     "UNIQUE INDEX(CrsCod,Role)) ENGINE=MEMORY"
  	     " SELECT CrsCod,"
  	             "Role"
  	       " FROM crs_users"
  	      " WHERE UsrCod=%ld",
	     UsrCod);

   /* Get if a user shares any course with me from database */
   UsrSharesAnyOfMyCrsWithDifferentRole =
      (DB_QueryCOUNT ("can not check if a user shares any course with you",
		      "SELECT COUNT(*)"
		       " FROM my_courses_tmp,"
		             "usr_courses_tmp"
                      " WHERE my_courses_tmp.CrsCod=usr_courses_tmp.CrsCod"
                        " AND my_courses_tmp.Role<>usr_courses_tmp.Role") != 0);

   /* Remove temporary table if exists */
   DB_Query ("can not remove temporary tables",
	     "DROP TEMPORARY TABLE IF EXISTS usr_courses_tmp");

   return UsrSharesAnyOfMyCrsWithDifferentRole;
  }

/*****************************************************************************/
/**** Get all my countries (those of my courses) and store them in a list ****/
/*****************************************************************************/

void Usr_GetMyCountrs (void)
  {
   MYSQL_RES *mysql_res;
   MYSQL_ROW row;
   unsigned NumCty;
   unsigned NumCtys;
   long CtyCod;

   /***** If my countries are yet filled, there's nothing to do *****/
   if (!Gbl.Usrs.Me.MyCtys.Filled)
     {
      Gbl.Usrs.Me.MyCtys.Num = 0;

      /***** Get my institutions from database *****/
      NumCtys = Usr_GetCtysFromUsr (Gbl.Usrs.Me.UsrDat.UsrCod,&mysql_res);
      for (NumCty = 0;
	   NumCty < NumCtys;
	   NumCty++)
	{
	 /* Get next country */
	 row = mysql_fetch_row (mysql_res);

	 /* Get country code */
	 if ((CtyCod = Str_ConvertStrCodToLongCod (row[0])) > 0)
	   {
	    if (Gbl.Usrs.Me.MyCtys.Num == Cty_MAX_COUNTRS_PER_USR)
	       Err_ShowErrorAndExit ("Maximum number of countries of a user exceeded.");

	    Gbl.Usrs.Me.MyCtys.Ctys[Gbl.Usrs.Me.MyCtys.Num].CtyCod  = CtyCod;
	    Gbl.Usrs.Me.MyCtys.Ctys[Gbl.Usrs.Me.MyCtys.Num].MaxRole = Rol_ConvertUnsignedStrToRole (row[1]);

	    Gbl.Usrs.Me.MyCtys.Num++;
	   }
	}

      /***** Free structure that stores the query result *****/
      DB_FreeMySQLResult (&mysql_res);

      /***** Set boolean that indicates that my institutions are yet filled *****/
      Gbl.Usrs.Me.MyCtys.Filled = true;
     }
  }

/*****************************************************************************/
/** Get all my institutions (those of my courses) and store them in a list ***/
/*****************************************************************************/

void Usr_GetMyInstits (void)
  {
   MYSQL_RES *mysql_res;
   MYSQL_ROW row;
   unsigned NumIns;
   unsigned NumInss;
   long InsCod;

   /***** If my institutions are yet filled, there's nothing to do *****/
   if (!Gbl.Usrs.Me.MyInss.Filled)
     {
      Gbl.Usrs.Me.MyInss.Num = 0;

      /***** Get my institutions from database *****/
      NumInss = Usr_GetInssFromUsr (Gbl.Usrs.Me.UsrDat.UsrCod,-1L,&mysql_res);
      for (NumIns = 0;
	   NumIns < NumInss;
	   NumIns++)
	{
	 /* Get next institution */
	 row = mysql_fetch_row (mysql_res);

	 /* Get institution code */
	 if ((InsCod = Str_ConvertStrCodToLongCod (row[0])) > 0)
	   {
	    if (Gbl.Usrs.Me.MyInss.Num == Ins_MAX_INSTITS_PER_USR)
	       Err_ShowErrorAndExit ("Maximum number of institutions of a user exceeded.");

	    Gbl.Usrs.Me.MyInss.Inss[Gbl.Usrs.Me.MyInss.Num].InsCod  = InsCod;
	    Gbl.Usrs.Me.MyInss.Inss[Gbl.Usrs.Me.MyInss.Num].MaxRole = Rol_ConvertUnsignedStrToRole (row[1]);

	    Gbl.Usrs.Me.MyInss.Num++;
	   }
	}

      /***** Free structure that stores the query result *****/
      DB_FreeMySQLResult (&mysql_res);

      /***** Set boolean that indicates that my institutions are yet filled *****/
      Gbl.Usrs.Me.MyInss.Filled = true;
     }
  }

/*****************************************************************************/
/***** Get all my centers (those of my courses) and store them in a list *****/
/*****************************************************************************/

void Usr_GetMyCenters (void)
  {
   MYSQL_RES *mysql_res;
   MYSQL_ROW row;
   unsigned NumCtr;
   unsigned NumCtrs;
   long CtrCod;

   /***** If my centers are yet filled, there's nothing to do *****/
   if (!Gbl.Usrs.Me.MyCtrs.Filled)
     {
      Gbl.Usrs.Me.MyCtrs.Num = 0;

      /***** Get my centers from database *****/
      NumCtrs = Usr_GetCtrsFromUsr (Gbl.Usrs.Me.UsrDat.UsrCod,-1L,&mysql_res);
      for (NumCtr = 0;
	   NumCtr < NumCtrs;
	   NumCtr++)
	{
	 /* Get next center */
	 row = mysql_fetch_row (mysql_res);

	 /* Get center code */
	 if ((CtrCod = Str_ConvertStrCodToLongCod (row[0])) > 0)
	   {
	    if (Gbl.Usrs.Me.MyCtrs.Num == Ctr_MAX_CENTERS_PER_USR)
	       Err_ShowErrorAndExit ("Maximum number of centers of a user exceeded.");

	    Gbl.Usrs.Me.MyCtrs.Ctrs[Gbl.Usrs.Me.MyCtrs.Num].CtrCod = CtrCod;
	    Gbl.Usrs.Me.MyCtrs.Ctrs[Gbl.Usrs.Me.MyCtrs.Num].MaxRole = Rol_ConvertUnsignedStrToRole (row[1]);

	    Gbl.Usrs.Me.MyCtrs.Num++;
	   }
	}

      /***** Free structure that stores the query result *****/
      DB_FreeMySQLResult (&mysql_res);

      /***** Set boolean that indicates that my centers are yet filled *****/
      Gbl.Usrs.Me.MyCtrs.Filled = true;
     }
  }

/*****************************************************************************/
/***** Get all my degrees (those of my courses) and store them in a list *****/
/*****************************************************************************/

void Usr_GetMyDegrees (void)
  {
   MYSQL_RES *mysql_res;
   MYSQL_ROW row;
   unsigned NumDeg;
   unsigned NumDegs;
   long DegCod;

   /***** If my degrees are yet filled, there's nothing to do *****/
   if (!Gbl.Usrs.Me.MyDegs.Filled)
     {
      Gbl.Usrs.Me.MyDegs.Num = 0;

      /***** Get my degrees from database *****/
      NumDegs = Usr_GetDegsFromUsr (Gbl.Usrs.Me.UsrDat.UsrCod,-1L,&mysql_res);
      for (NumDeg = 0;
	   NumDeg < NumDegs;
	   NumDeg++)
	{
	 /* Get next degree */
	 row = mysql_fetch_row (mysql_res);

	 /* Get degree code */
	 if ((DegCod = Str_ConvertStrCodToLongCod (row[0])) > 0)
	   {
	    if (Gbl.Usrs.Me.MyDegs.Num == Deg_MAX_DEGREES_PER_USR)
	       Err_ShowErrorAndExit ("Maximum number of degrees of a user exceeded.");

	    Gbl.Usrs.Me.MyDegs.Degs[Gbl.Usrs.Me.MyDegs.Num].DegCod  = DegCod;
	    Gbl.Usrs.Me.MyDegs.Degs[Gbl.Usrs.Me.MyDegs.Num].MaxRole = Rol_ConvertUnsignedStrToRole (row[1]);

	    Gbl.Usrs.Me.MyDegs.Num++;
	   }
	}

      /***** Free structure that stores the query result *****/
      DB_FreeMySQLResult (&mysql_res);

      /***** Set boolean that indicates that my degrees are yet filled *****/
      Gbl.Usrs.Me.MyDegs.Filled = true;
     }
  }

/*****************************************************************************/
/*************** Get all my courses and store them in a list *****************/
/*****************************************************************************/

void Usr_GetMyCourses (void)
  {
   MYSQL_RES *mysql_res;
   MYSQL_ROW row;
   unsigned NumCrss;
   unsigned NumCrs;
   long CrsCod;

   /***** Trivial check 1: if my courses are already filled, there's nothing to do *****/
   if (Gbl.Usrs.Me.MyCrss.Filled)
      return;

   /***** Trivial check 2: if user's code is not set, don't query database *****/
   if (Gbl.Usrs.Me.UsrDat.UsrCod <= 0)
      return;

   /***** Remove temporary table with my courses *****/
   Usr_RemoveTemporaryTableMyCourses ();

   /***** Create temporary table with my courses *****/
   DB_Query ("can not create temporary table",
	     "CREATE TEMPORARY TABLE IF NOT EXISTS my_courses_tmp"
	     " (CrsCod INT NOT NULL,"
		 "Role TINYINT NOT NULL,"
	       "DegCod INT NOT NULL,"
	       "UNIQUE INDEX(CrsCod,Role,DegCod)) ENGINE=MEMORY"
	     " SELECT crs_users.CrsCod,"
		     "crs_users.Role,"
		     "crs_courses.DegCod"
	       " FROM crs_users,"
		     "crs_courses,"
		     "deg_degrees"
	      " WHERE crs_users.UsrCod=%ld"
		" AND crs_users.CrsCod=crs_courses.CrsCod"
		" AND crs_courses.DegCod=deg_degrees.DegCod"
	      " ORDER BY deg_degrees.ShortName,"
			"crs_courses.ShortName",
	     Gbl.Usrs.Me.UsrDat.UsrCod);

   /***** Get my courses from database *****/
   NumCrss = (unsigned)
   DB_QuerySELECT (&mysql_res,"can not get which courses you belong to",
		   "SELECT CrsCod,"	// row[0]
			  "Role,"		// row[1]
			  "DegCod"	// row[2]
		   " FROM my_courses_tmp");
   for (NumCrs = 0;
	NumCrs < NumCrss;
	NumCrs++)
     {
      /* Get next course */
      row = mysql_fetch_row (mysql_res);

      /* Get course code */
      if ((CrsCod = Str_ConvertStrCodToLongCod (row[0])) > 0)
	{
	 if (Gbl.Usrs.Me.MyCrss.Num == Crs_MAX_COURSES_PER_USR)
	    Err_ShowErrorAndExit ("Maximum number of courses of a user exceeded.");

	 Gbl.Usrs.Me.MyCrss.Crss[Gbl.Usrs.Me.MyCrss.Num].CrsCod = CrsCod;
	 Gbl.Usrs.Me.MyCrss.Crss[Gbl.Usrs.Me.MyCrss.Num].Role   = Rol_ConvertUnsignedStrToRole (row[1]);
	 Gbl.Usrs.Me.MyCrss.Crss[Gbl.Usrs.Me.MyCrss.Num].DegCod = Str_ConvertStrCodToLongCod (row[2]);
	 Gbl.Usrs.Me.MyCrss.Num++;
	}
     }

   /***** Free structure that stores the query result *****/
   DB_FreeMySQLResult (&mysql_res);

   /***** Set boolean that indicates that my courses are already filled *****/
   Gbl.Usrs.Me.MyCrss.Filled = true;
  }

/*****************************************************************************/
/************************ Free the list of my countries ************************/
/*****************************************************************************/

void Usr_FreeMyCountrs (void)
  {
   if (Gbl.Usrs.Me.MyCtys.Filled)
     {
      /***** Reset list *****/
      Gbl.Usrs.Me.MyCtys.Filled = false;
      Gbl.Usrs.Me.MyCtys.Num    = 0;
     }
  }

/*****************************************************************************/
/********************* Free the list of my institutions **********************/
/*****************************************************************************/

void Usr_FreeMyInstits (void)
  {
   if (Gbl.Usrs.Me.MyInss.Filled)
     {
      /***** Reset list *****/
      Gbl.Usrs.Me.MyInss.Filled = false;
      Gbl.Usrs.Me.MyInss.Num    = 0;
     }
  }

/*****************************************************************************/
/************************ Free the list of my centers ************************/
/*****************************************************************************/

void Usr_FreeMyCenters (void)
  {
   if (Gbl.Usrs.Me.MyCtrs.Filled)
     {
      /***** Reset list *****/
      Gbl.Usrs.Me.MyCtrs.Filled = false;
      Gbl.Usrs.Me.MyCtrs.Num    = 0;
     }
  }

/*****************************************************************************/
/************************ Free the list of my degrees ************************/
/*****************************************************************************/

void Usr_FreeMyDegrees (void)
  {
   if (Gbl.Usrs.Me.MyDegs.Filled)
     {
      /***** Reset list *****/
      Gbl.Usrs.Me.MyDegs.Filled = false;
      Gbl.Usrs.Me.MyDegs.Num    = 0;
     }
  }

/*****************************************************************************/
/************************ Free the list of my courses ************************/
/*****************************************************************************/

void Usr_FreeMyCourses (void)
  {
   if (Gbl.Usrs.Me.MyCrss.Filled)
     {
      /***** Reset list *****/
      Gbl.Usrs.Me.MyCrss.Filled = false;
      Gbl.Usrs.Me.MyCrss.Num    = 0;

      /***** Remove temporary table with my courses *****/
      Usr_RemoveTemporaryTableMyCourses ();
     }
  }

/*****************************************************************************/
/************************ Free the list of my courses ************************/
/*****************************************************************************/

static void Usr_RemoveTemporaryTableMyCourses (void)
  {
   char Query[128];

   /***** Remove temporary table with my courses *****/
   sprintf (Query,"DROP TEMPORARY TABLE IF EXISTS my_courses_tmp");
   if (mysql_query (&Gbl.mysql,Query))
      DB_ExitOnMySQLError ("can not remove temporary table");
  }

/*****************************************************************************/
/**************** Check if a user belongs to an institution ******************/
/*****************************************************************************/

void Usr_FlushCacheUsrBelongsToIns (void)
  {
   Gbl.Cache.UsrBelongsToIns.UsrCod = -1L;
   Gbl.Cache.UsrBelongsToIns.InsCod = -1L;
   Gbl.Cache.UsrBelongsToIns.Belongs = false;
  }

bool Usr_CheckIfUsrBelongsToIns (long UsrCod,long InsCod)
  {
   /***** 1. Fast check: Trivial case *****/
   if (UsrCod <= 0 ||
       InsCod <= 0)
      return false;

   /***** 2. Fast check: If cached... *****/
   if (UsrCod == Gbl.Cache.UsrBelongsToIns.UsrCod &&
       InsCod != Gbl.Cache.UsrBelongsToIns.InsCod)
      return Gbl.Cache.UsrBelongsToIns.Belongs;

   /***** 3. Slow check: Get is user belongs to institution from database *****/
   Gbl.Cache.UsrBelongsToIns.UsrCod = UsrCod;
   Gbl.Cache.UsrBelongsToIns.InsCod = InsCod;
   Gbl.Cache.UsrBelongsToIns.Belongs =
      (DB_QueryCOUNT ("can not check if a user belongs to an institution",
		      "SELECT COUNT(DISTINCT ctr_centers.InsCod)"
		       " FROM crs_users,"
			     "crs_courses,"
			     "deg_degrees,"
			     "ctr_centers"
		      " WHERE crs_users.UsrCod=%ld"
			" AND crs_users.Accepted='Y'"	// Only if user accepted
			" AND crs_users.CrsCod=crs_courses.CrsCod"
			" AND crs_courses.DegCod=deg_degrees.DegCod"
			" AND deg_degrees.CtrCod=ctr_centers.CtrCod"
			" AND ctr_centers.InsCod=%ld",
		      UsrCod,InsCod) != 0);
   return Gbl.Cache.UsrBelongsToIns.Belongs;
  }

/*****************************************************************************/
/******************* Check if a user belongs to a center *********************/
/*****************************************************************************/

void Usr_FlushCacheUsrBelongsToCtr (void)
  {
   Gbl.Cache.UsrBelongsToCtr.UsrCod = -1L;
   Gbl.Cache.UsrBelongsToCtr.CtrCod = -1L;
   Gbl.Cache.UsrBelongsToCtr.Belongs = false;
  }

bool Usr_CheckIfUsrBelongsToCtr (long UsrCod,long CtrCod)
  {
   /***** 1. Fast check: Trivial case *****/
   if (UsrCod <= 0 ||
       CtrCod <= 0)
      return false;

   /***** 2. Fast check: If cached... *****/
   if (UsrCod == Gbl.Cache.UsrBelongsToCtr.UsrCod &&
       CtrCod == Gbl.Cache.UsrBelongsToCtr.CtrCod)
      return Gbl.Cache.UsrBelongsToCtr.Belongs;

   /***** 3. Slow check: Get is user belongs to center from database *****/
   Gbl.Cache.UsrBelongsToCtr.UsrCod = UsrCod;
   Gbl.Cache.UsrBelongsToCtr.CtrCod = CtrCod;
   Gbl.Cache.UsrBelongsToCtr.Belongs =
      (DB_QueryCOUNT ("can not check if a user belongs to a center",
		      "SELECT COUNT(DISTINCT deg_degrees.CtrCod)"
		       " FROM crs_users,"
		             "crs_courses,"
		             "deg_degrees"
		      " WHERE crs_users.UsrCod=%ld"
		        " AND crs_users.Accepted='Y'"	// Only if user accepted
		        " AND crs_users.CrsCod=crs_courses.CrsCod"
		        " AND crs_courses.DegCod=deg_degrees.DegCod"
		        " AND deg_degrees.CtrCod=%ld",
		      UsrCod,
		      CtrCod) != 0);
   return Gbl.Cache.UsrBelongsToCtr.Belongs;
  }

/*****************************************************************************/
/******************* Check if a user belongs to a degree *********************/
/*****************************************************************************/

void Usr_FlushCacheUsrBelongsToDeg (void)
  {
   Gbl.Cache.UsrBelongsToDeg.UsrCod = -1L;
   Gbl.Cache.UsrBelongsToDeg.DegCod = -1L;
   Gbl.Cache.UsrBelongsToDeg.Belongs = false;
  }

bool Usr_CheckIfUsrBelongsToDeg (long UsrCod,long DegCod)
  {
   /***** 1. Fast check: Trivial case *****/
   if (UsrCod <= 0 ||
       DegCod <= 0)
      return false;

   /***** 2. Fast check: If cached... *****/
   if (UsrCod == Gbl.Cache.UsrBelongsToDeg.UsrCod &&
       DegCod == Gbl.Cache.UsrBelongsToDeg.DegCod)
      return Gbl.Cache.UsrBelongsToDeg.Belongs;

   /***** 3. Slow check: Get if user belongs to degree from database *****/
   Gbl.Cache.UsrBelongsToDeg.UsrCod = UsrCod;
   Gbl.Cache.UsrBelongsToDeg.DegCod = DegCod;
   Gbl.Cache.UsrBelongsToDeg.Belongs =
      (DB_QueryCOUNT ("can not check if a user belongs to a degree",
		      "SELECT COUNT(DISTINCT crs_courses.DegCod)"
		       " FROM crs_users,"
		             "crs_courses"
		      " WHERE crs_users.UsrCod=%ld"
		        " AND crs_users.Accepted='Y'"	// Only if user accepted
		        " AND crs_users.CrsCod=crs_courses.CrsCod"
		        " AND crs_courses.DegCod=%ld",
		      UsrCod,DegCod) != 0);
   return Gbl.Cache.UsrBelongsToDeg.Belongs;
  }

/*****************************************************************************/
/******************** Check if a user belongs to a course ********************/
/*****************************************************************************/

void Usr_FlushCacheUsrBelongsToCrs (void)
  {
   Gbl.Cache.UsrBelongsToCrs.UsrCod = -1L;
   Gbl.Cache.UsrBelongsToCrs.CrsCod = -1L;
   Gbl.Cache.UsrBelongsToCrs.CountOnlyAcceptedCourses = false;
   Gbl.Cache.UsrBelongsToCrs.Belongs = false;
  }

bool Usr_CheckIfUsrBelongsToCrs (long UsrCod,long CrsCod,
                                 bool CountOnlyAcceptedCourses)
  {
   const char *SubQuery;

   /***** 1. Fast check: Trivial cases *****/
   if (UsrCod <= 0 ||
       CrsCod <= 0)
      return false;

   /***** 2. Fast check: If cached... *****/
   if (UsrCod == Gbl.Cache.UsrBelongsToCrs.UsrCod &&
       CrsCod == Gbl.Cache.UsrBelongsToCrs.CrsCod &&
       CountOnlyAcceptedCourses == Gbl.Cache.UsrBelongsToCrs.CountOnlyAcceptedCourses)
      return Gbl.Cache.UsrBelongsToCrs.Belongs;

   /***** 3. Slow check: Get if user belongs to course from database *****/
   SubQuery = (CountOnlyAcceptedCourses ? " AND crs_users.Accepted='Y'" :	// Only if user accepted
	                                  "");
   Gbl.Cache.UsrBelongsToCrs.UsrCod = UsrCod;
   Gbl.Cache.UsrBelongsToCrs.CrsCod = CrsCod;
   Gbl.Cache.UsrBelongsToCrs.CountOnlyAcceptedCourses = CountOnlyAcceptedCourses;
   Gbl.Cache.UsrBelongsToCrs.Belongs =
      (DB_QueryCOUNT ("can not check if a user belongs to a course",
		      "SELECT COUNT(*)"
		       " FROM crs_users"
		      " WHERE CrsCod=%ld"
		        " AND UsrCod=%ld%s",
		      CrsCod,UsrCod,SubQuery) != 0);
   return Gbl.Cache.UsrBelongsToCrs.Belongs;
  }

/*****************************************************************************/
/***** Check if user belongs (no matter if he/she has accepted or not) *******/
/***** to the current course                                           *******/
/*****************************************************************************/

void Usr_FlushCacheUsrBelongsToCurrentCrs (void)
  {
   Gbl.Cache.UsrBelongsToCurrentCrs.UsrCod = -1L;
   Gbl.Cache.UsrBelongsToCurrentCrs.Belongs = false;
  }

bool Usr_CheckIfUsrBelongsToCurrentCrs (const struct UsrData *UsrDat)
  {
   /***** 1. Fast check: Trivial cases *****/
   if (UsrDat->UsrCod <= 0 ||
       Gbl.Hierarchy.Crs.CrsCod <= 0)
      return false;

   /***** 2. Fast check: If cached... *****/
   if (UsrDat->UsrCod == Gbl.Cache.UsrBelongsToCurrentCrs.UsrCod)
      return Gbl.Cache.UsrBelongsToCurrentCrs.Belongs;

   /***** 3. Fast check: If we know role of user in the current course *****/
   if (UsrDat->Roles.InCurrentCrs != Rol_UNK)
     {
      Gbl.Cache.UsrBelongsToCurrentCrs.UsrCod  = UsrDat->UsrCod;
      Gbl.Cache.UsrBelongsToCurrentCrs.Belongs = UsrDat->Roles.InCurrentCrs == Rol_STD ||
	                                         UsrDat->Roles.InCurrentCrs == Rol_NET ||
	                                         UsrDat->Roles.InCurrentCrs == Rol_TCH;
      return Gbl.Cache.UsrBelongsToCurrentCrs.Belongs;
     }

   /***** 4. Fast / slow check: Get if user belongs to current course *****/
   Gbl.Cache.UsrBelongsToCurrentCrs.UsrCod  = UsrDat->UsrCod;
   Gbl.Cache.UsrBelongsToCurrentCrs.Belongs = Usr_CheckIfUsrBelongsToCrs (UsrDat->UsrCod,
						                          Gbl.Hierarchy.Crs.CrsCod,
						                          false);
   return Gbl.Cache.UsrBelongsToCurrentCrs.Belongs;
  }

/*****************************************************************************/
/***** Check if user belongs (no matter if he/she has accepted or not) *******/
/***** to the current course                                           *******/
/*****************************************************************************/

void Usr_FlushCacheUsrHasAcceptedInCurrentCrs (void)
  {
   Gbl.Cache.UsrHasAcceptedInCurrentCrs.UsrCod = -1L;
   Gbl.Cache.UsrHasAcceptedInCurrentCrs.Accepted = false;
  }

bool Usr_CheckIfUsrHasAcceptedInCurrentCrs (const struct UsrData *UsrDat)
  {
   /***** 1. Fast check: Trivial cases *****/
   if (UsrDat->UsrCod <= 0 ||
       Gbl.Hierarchy.Crs.CrsCod <= 0)
      return false;

   /***** 2. Fast check: If cached... *****/
   if (UsrDat->UsrCod == Gbl.Cache.UsrHasAcceptedInCurrentCrs.UsrCod)
      return Gbl.Cache.UsrHasAcceptedInCurrentCrs.Accepted;

   /***** 3. Fast / slow check: Get if user belongs to current course
                                and has accepted *****/
   Gbl.Cache.UsrHasAcceptedInCurrentCrs.UsrCod = UsrDat->UsrCod;
   Gbl.Cache.UsrHasAcceptedInCurrentCrs.Accepted = Usr_CheckIfUsrBelongsToCrs (UsrDat->UsrCod,
						                               Gbl.Hierarchy.Crs.CrsCod,
						                               true);
   return Gbl.Cache.UsrHasAcceptedInCurrentCrs.Accepted;
  }

/*****************************************************************************/
/********************** Check if I belong to a country **********************/
/*****************************************************************************/

bool Usr_CheckIfIBelongToCty (long CtyCod)
  {
   unsigned NumMyCty;

   /***** Fill the list with the institutions I belong to *****/
   Usr_GetMyCountrs ();

   /***** Check if the country passed as parameter is any of my countries *****/
   for (NumMyCty = 0;
        NumMyCty < Gbl.Usrs.Me.MyCtys.Num;
        NumMyCty++)
      if (Gbl.Usrs.Me.MyCtys.Ctys[NumMyCty].CtyCod == CtyCod)
         return true;
   return false;
  }

/*****************************************************************************/
/******************** Check if I belong to an institution ********************/
/*****************************************************************************/

bool Usr_CheckIfIBelongToIns (long InsCod)
  {
   unsigned NumMyIns;

   /***** Fill the list with the institutions I belong to *****/
   Usr_GetMyInstits ();

   /***** Check if the institution passed as parameter is any of my institutions *****/
   for (NumMyIns = 0;
        NumMyIns < Gbl.Usrs.Me.MyInss.Num;
        NumMyIns++)
      if (Gbl.Usrs.Me.MyInss.Inss[NumMyIns].InsCod == InsCod)
         return true;
   return false;
  }

/*****************************************************************************/
/*********************** Check if I belong to a center ***********************/
/*****************************************************************************/

bool Usr_CheckIfIBelongToCtr (long CtrCod)
  {
   unsigned NumMyCtr;

   /***** Fill the list with the centers I belong to *****/
   Usr_GetMyCenters ();

   /***** Check if the center passed as parameter is any of my centers *****/
   for (NumMyCtr = 0;
        NumMyCtr < Gbl.Usrs.Me.MyCtrs.Num;
        NumMyCtr++)
      if (Gbl.Usrs.Me.MyCtrs.Ctrs[NumMyCtr].CtrCod == CtrCod)
         return true;
   return false;
  }

/*****************************************************************************/
/*********************** Check if I belong to a degree ***********************/
/*****************************************************************************/

bool Usr_CheckIfIBelongToDeg (long DegCod)
  {
   unsigned NumMyDeg;

   /***** Fill the list with the degrees I belong to *****/
   Usr_GetMyDegrees ();

   /***** Check if the degree passed as parameter is any of my degrees *****/
   for (NumMyDeg = 0;
        NumMyDeg < Gbl.Usrs.Me.MyDegs.Num;
        NumMyDeg++)
      if (Gbl.Usrs.Me.MyDegs.Degs[NumMyDeg].DegCod == DegCod)
         return true;
   return false;
  }

/*****************************************************************************/
/*********************** Check if I belong to a course ***********************/
/*****************************************************************************/

bool Usr_CheckIfIBelongToCrs (long CrsCod)
  {
   unsigned NumMyCrs;

   /***** Fill the list with the courses I belong to *****/
   Usr_GetMyCourses ();

   /***** Check if the course passed as parameter is any of my courses *****/
   for (NumMyCrs = 0;
        NumMyCrs < Gbl.Usrs.Me.MyCrss.Num;
        NumMyCrs++)
      if (Gbl.Usrs.Me.MyCrss.Crss[NumMyCrs].CrsCod == CrsCod)
         return true;

   return false;
  }

/*****************************************************************************/
/**************** Get the countries of a user from database ******************/
/*****************************************************************************/
// Returns the number of rows of the result

unsigned Usr_GetCtysFromUsr (long UsrCod,MYSQL_RES **mysql_res)
  {
   extern const char *Lan_STR_LANG_ID[1 + Lan_NUM_LANGUAGES];

   /***** Get the institutions a user belongs to from database *****/
   return (unsigned)
   DB_QuerySELECT (mysql_res,"can not get the countries a user belongs to",
		   "SELECT cty_countrs.CtyCod,"	// row[0]
			  "MAX(crs_users.Role)"	// row[1]
		    " FROM crs_users,"
			  "crs_courses,"
			  "deg_degrees,"
			  "ctr_centers,"
			  "ins_instits,"
			  "cty_countrs"
		   " WHERE crs_users.UsrCod=%ld"
		     " AND crs_users.CrsCod=crs_courses.CrsCod"
		     " AND crs_courses.DegCod=deg_degrees.DegCod"
		     " AND deg_degrees.CtrCod=ctr_centers.CtrCod"
		     " AND ctr_centers.InsCod=ins_instits.InsCod"
		     " AND ins_instits.CtyCod=cty_countrs.CtyCod"
		   " GROUP BY cty_countrs.CtyCod"
		   " ORDER BY cty_countrs.Name_%s",
		   UsrCod,
		   Lan_STR_LANG_ID[Gbl.Prefs.Language]);
  }

/*****************************************************************************/
/************** Get the institutions of a user from database *****************/
/*****************************************************************************/
// Returns the number of rows of the result

unsigned Usr_GetInssFromUsr (long UsrCod,long CtyCod,MYSQL_RES **mysql_res)
  {
   /***** Get the institutions a user belongs to from database *****/
   if (CtyCod > 0)
      return (unsigned)
      DB_QuerySELECT (mysql_res,"can not get the institutions a user belongs to",
		      "SELECT ins_instits.InsCod,"	// row[0]
			     "MAX(crs_users.Role)"	// row[1]
		       " FROM crs_users,"
			     "crs_courses,"
			     "deg_degrees,"
			     "ctr_centers,"
			     "ins_instits"
		      " WHERE crs_users.UsrCod=%ld"
		        " AND crs_users.CrsCod=crs_courses.CrsCod"
		        " AND crs_courses.DegCod=deg_degrees.DegCod"
		        " AND deg_degrees.CtrCod=ctr_centers.CtrCod"
		        " AND ctr_centers.InsCod=ins_instits.InsCod"
		        " AND ins_instits.CtyCod=%ld"
		      " GROUP BY ins_instits.InsCod"
		      " ORDER BY ins_instits.ShortName",
		      UsrCod,
		      CtyCod);
   else
      return (unsigned)
      DB_QuerySELECT (mysql_res,"can not get the institutions a user belongs to",
		      "SELECT ins_instits.InsCod,"	// row[0]
			     "MAX(crs_users.Role)"	// row[1]
		       " FROM crs_users,"
			     "crs_courses,"
			     "deg_degrees,"
			     "ctr_centers,"
			     "ins_instits"
		      " WHERE crs_users.UsrCod=%ld"
		        " AND crs_users.CrsCod=crs_courses.CrsCod"
		        " AND crs_courses.DegCod=deg_degrees.DegCod"
		        " AND deg_degrees.CtrCod=ctr_centers.CtrCod"
		        " AND ctr_centers.InsCod=ins_instits.InsCod"
		      " GROUP BY ins_instits.InsCod"
		      " ORDER BY ins_instits.ShortName",
		      UsrCod);
  }

/*****************************************************************************/
/***************** Get the centers of a user from database *******************/
/*****************************************************************************/
// Returns the number of rows of the result

unsigned Usr_GetCtrsFromUsr (long UsrCod,long InsCod,MYSQL_RES **mysql_res)
  {
   /***** Get from database the centers a user belongs to *****/
   if (InsCod > 0)
      return (unsigned)
      DB_QuerySELECT (mysql_res,"can not check the centers a user belongs to",
		      "SELECT ctr_centers.CtrCod,"	// row[0]
			     "MAX(crs_users.Role)"	// row[1]
		       " FROM crs_users,"
			     "crs_courses,"
			     "deg_degrees,"
			     "ctr_centers"
		      " WHERE crs_users.UsrCod=%ld"
		        " AND crs_users.CrsCod=crs_courses.CrsCod"
		        " AND crs_courses.DegCod=deg_degrees.DegCod"
		        " AND deg_degrees.CtrCod=ctr_centers.CtrCod"
		        " AND ctr_centers.InsCod=%ld"
		      " GROUP BY ctr_centers.CtrCod"
		      " ORDER BY ctr_centers.ShortName",
		      UsrCod,
		      InsCod);
   else
      return (unsigned)
      DB_QuerySELECT (mysql_res,"can not check the centers a user belongs to",
		      "SELECT deg_degrees.CtrCod,"	// row[0]
			     "MAX(crs_users.Role)"	// row[1]
		       " FROM crs_users,"
			     "crs_courses,"
			     "deg_degrees,"
			     "ctr_centers"
		      " WHERE crs_users.UsrCod=%ld"
		        " AND crs_users.CrsCod=crs_courses.CrsCod"
		        " AND crs_courses.DegCod=deg_degrees.DegCod"
		        " AND deg_degrees.CtrCod=ctr_centers.CtrCod"
		      " GROUP BY ctr_centers.CtrCod"
		      " ORDER BY ctr_centers.ShortName",
		      UsrCod);
  }

/*****************************************************************************/
/***************** Get the degrees of a user from database *******************/
/*****************************************************************************/
// Returns the number of rows of the result

unsigned Usr_GetDegsFromUsr (long UsrCod,long CtrCod,MYSQL_RES **mysql_res)
  {
   /***** Get from database the degrees a user belongs to *****/
   if (CtrCod > 0)
      return (unsigned)
      DB_QuerySELECT (mysql_res,"can not check the degrees a user belongs to",
		      "SELECT deg_degrees.DegCod,"	// row[0]
			     "MAX(crs_users.Role)"	// row[1]
		       " FROM crs_users,"
			     "crs_courses,"
			     "deg_degrees"
		      " WHERE crs_users.UsrCod=%ld"
		        " AND crs_users.CrsCod=crs_courses.CrsCod"
		        " AND crs_courses.DegCod=deg_degrees.DegCod"
		        " AND deg_degrees.CtrCod=%ld"
		      " GROUP BY deg_degrees.DegCod"
		      " ORDER BY deg_degrees.ShortName",
		      UsrCod,
		      CtrCod);
   else
      return (unsigned)
      DB_QuerySELECT (mysql_res,"can not check the degrees a user belongs to",
		      "SELECT deg_degrees.DegCod,"	// row[0]
			     "MAX(crs_users.Role)"	// row[1]
		       " FROM crs_users,"
			     "crs_courses,"
			     "deg_degrees"
		      " WHERE crs_users.UsrCod=%ld"
		        " AND crs_users.CrsCod=crs_courses.CrsCod"
		        " AND crs_courses.DegCod=deg_degrees.DegCod"
		      " GROUP BY deg_degrees.DegCod"
		      " ORDER BY deg_degrees.ShortName",
		      UsrCod);
  }

/*****************************************************************************/
/************** Get all the courses of a user from database ******************/
/*****************************************************************************/
// Returns the number of rows of the result

unsigned Usr_GetCrssFromUsr (long UsrCod,long DegCod,MYSQL_RES **mysql_res)
  {
   /***** Get from database the courses a user belongs to *****/
   if (DegCod > 0)	// Courses in a degree
      return (unsigned)
      DB_QuerySELECT (mysql_res,"can not get the courses a user belongs to",
		      "SELECT crs_users.CrsCod,"	// row[0]
			     "crs_users.Role,"		// row[1]
			     "crs_courses.DegCod"	// row[2]
		       " FROM crs_users,"
			     "crs_courses"
		      " WHERE crs_users.UsrCod=%ld"
		        " AND crs_users.CrsCod=crs_courses.CrsCod"
		        " AND crs_courses.DegCod=%ld"
		      " ORDER BY crs_courses.ShortName",
		      UsrCod,
		      DegCod);
   else			// All the courses
      return (unsigned)
      DB_QuerySELECT (mysql_res,"can not get the courses a user belongs to",
		      "SELECT crs_users.CrsCod,"	// row[0]
			     "crs_users.Role,"		// row[1]
			     "crs_courses.DegCod"	// row[2]
		       " FROM crs_users,"
			     "crs_courses,"
			     "deg_degrees"
		      " WHERE crs_users.UsrCod=%ld"
		        " AND crs_users.CrsCod=crs_courses.CrsCod"
		        " AND crs_courses.DegCod=deg_degrees.DegCod"
		      " ORDER BY deg_degrees.ShortName,"
			        "crs_courses.ShortName",
		      UsrCod);
  }

/*****************************************************************************/
/********* Get the degree in which a user is enroled in more courses *********/
/*****************************************************************************/

void Usr_GetMainDeg (long UsrCod,
		     char ShrtName[Cns_HIERARCHY_MAX_BYTES_SHRT_NAME + 1],
		     Rol_Role_t *MaxRole)
  {
   MYSQL_RES *mysql_res;
   MYSQL_ROW row;

   /***** Get a random student from current course from database *****/
   if (DB_QuerySELECT (&mysql_res,"can not get user's main degree",
		       "SELECT deg_degrees.ShortName,"	// row[0]
		              "main_degree.MaxRole"	// row[1]
		       " FROM deg_degrees,"

		       // The second table contain only one row with the main degree
		       " (SELECT crs_courses.DegCod AS DegCod,"
		                "MAX(crs_users.Role) AS MaxRole,"
		                "COUNT(*) AS N"
		         " FROM crs_users,"
		               "crs_courses"
		        " WHERE crs_users.UsrCod=%ld"
		          " AND crs_users.CrsCod=crs_courses.CrsCod"
		        " GROUP BY crs_courses.DegCod"
		        " ORDER BY N DESC"	// Ordered by number of courses in which user is enroled
		        " LIMIT 1)"		// We need only the main degree
		       " AS main_degree"

		       " WHERE deg_degrees.DegCod=main_degree.DegCod",
		       UsrCod))
     {
      row = mysql_fetch_row (mysql_res);

      /* Get degree name (row[0]) */
      Str_Copy (ShrtName,row[0],Cns_HIERARCHY_MAX_BYTES_SHRT_NAME);

      /* Get maximum role (row[1]) */
      *MaxRole = Rol_ConvertUnsignedStrToRole (row[1]);
     }
   else	// User is not enroled in any course
     {
      ShrtName[0] = '\0';
      *MaxRole = Rol_UNK;
     }

   /***** Free structure that stores the query result *****/
   DB_FreeMySQLResult (&mysql_res);
  }

/*****************************************************************************/
/******** Check if a user exists with a given encrypted user's code **********/
/*****************************************************************************/

bool Usr_ChkIfEncryptedUsrCodExists (const char EncryptedUsrCod[Cry_BYTES_ENCRYPTED_STR_SHA256_BASE64])
  {
   /***** Get if an encrypted user's code already existed in database *****/
   return (DB_QueryCOUNT ("can not check if an encrypted user's code"
			  " already existed",
			  "SELECT COUNT(*)"
			   " FROM usr_data"
			  " WHERE EncryptedUsrCod='%s'",
			  EncryptedUsrCod) != 0);
  }

/*****************************************************************************/
/***************************** Write landing page ****************************/
/*****************************************************************************/

void Usr_WriteLandingPage (void)
  {
   /***** Form to log in *****/
   Usr_WriteFormLogin (ActLogIn,NULL);

   /***** Form to go to request the creation of a new account *****/
   Acc_ShowFormGoToRequestNewAccount ();
  }

/*****************************************************************************/
/************************ Write form for user log out ************************/
/*****************************************************************************/

void Usr_WriteFormLogout (void)
  {
   /***** Form to change my role *****/
   Usr_ShowFormsLogoutAndRole ();

   /***** Show help to enrol me *****/
   Hlp_ShowHelpWhatWouldYouLikeToDo ();
  }

/*****************************************************************************/
/********************* Message and form shown after log out ******************/
/*****************************************************************************/

void Usr_Logout (void)
  {
   // extern const char *Txt_The_session_has_been_closed;

   /***** Confirmation message *****/
   // Ale_ShowFixedAlert (Ale_INFO,Txt_The_session_has_been_closed);

   /***** Form to log in *****/
   Usr_WriteFormLogin (ActLogIn,NULL);

   /***** Advertisement about mobile app *****/
   Lay_AdvertisementMobile ();
  }

/*****************************************************************************/
/*************************** Put link to log in ******************************/
/*****************************************************************************/

void Usr_PutLinkToLogin (void)
  {
   extern const char *Txt_Log_in;

   Lay_PutContextualLinkIconText (ActFrmLogIn,NULL,
                                  NULL,NULL,
				  "sign-in-alt-green.svg",
				  Txt_Log_in);
  }

/*****************************************************************************/
/************************ Write form for user log in *************************/
/*****************************************************************************/

void Usr_WriteFormLogin (Act_Action_t NextAction,void (*FuncParams) (void))
  {
   extern const char *Hlp_PROFILE_LogIn;
   extern const char *Txt_Log_in;
   extern const char *Txt_User[Usr_NUM_SEXS];
   extern const char *Txt_nick_email_or_ID;
   extern const char *Txt_Password;
   extern const char *Txt_password;

   /***** Contextual menu *****/
   Mnu_ContextMenuBegin ();
   Acc_PutLinkToCreateAccount ();	// Create account
   Pwd_PutLinkToSendNewPasswd ();	// Send new password
   Lan_PutLinkToChangeLanguage ();	// Change language
   Mnu_ContextMenuEnd ();

   HTM_DIV_Begin ("class=\"CM\"");

   /***** Begin form *****/
   Frm_BeginForm (NextAction);
   if (FuncParams)
      FuncParams ();

   /***** Begin box and table *****/
   Box_BoxTableBegin (NULL,Txt_Log_in,
                      NULL,NULL,
                      Hlp_PROFILE_LogIn,Box_NOT_CLOSABLE,2);

   /***** User's ID/nickname *****/
   HTM_DIV_Begin ("class=\"LM\"");
   HTM_LABEL_Begin ("for=\"UsrId\"");
   Ico_PutIcon ("user.svg",Txt_User[Usr_SEX_UNKNOWN],"CONTEXT_ICO_16x16");
   HTM_LABEL_End ();
   HTM_INPUT_TEXT ("UsrId",Cns_MAX_CHARS_EMAIL_ADDRESS,Gbl.Usrs.Me.UsrIdLogin,
                   HTM_DONT_SUBMIT_ON_CHANGE,
		   "id=\"UsrId\" size=\"18\" placeholder=\"%s\""
		   " autofocus=\"autofocus\" required=\"required\"",
	           Txt_nick_email_or_ID);
   HTM_DIV_End ();

   /***** User's password *****/
   HTM_DIV_Begin ("class=\"LM\"");
   HTM_LABEL_Begin ("for=\"UsrPwd\"");
   Ico_PutIcon ("key.svg",Txt_Password,"CONTEXT_ICO_16x16");
   HTM_LABEL_End ();
   HTM_INPUT_PASSWORD ("UsrPwd",Txt_password,NULL,false,
		       "id=\"UsrPwd\"");
   HTM_DIV_End ();

   /***** End table, send button and end box *****/
   Box_BoxTableWithButtonEnd (Btn_CONFIRM_BUTTON,Txt_Log_in);

   /***** End form *****/
   Frm_EndForm ();

   HTM_DIV_End ();
  }

/*****************************************************************************/
/******************* Write type and name of logged user **********************/
/*****************************************************************************/

void Usr_WelcomeUsr (void)
  {
   extern const unsigned Txt_Current_CGI_SWAD_Language;
   extern const char *Txt_NEW_YEAR_GREETING;
   extern const char *Txt_Happy_birthday_X;
   extern const char *Txt_Please_check_your_email_address;
   extern const char *Txt_Check;
   extern const char *Txt_Switching_to_LANGUAGE[1 + Lan_NUM_LANGUAGES];

   if (Gbl.Usrs.Me.Logged)
     {
      if (Gbl.Usrs.Me.UsrDat.Prefs.Language == Txt_Current_CGI_SWAD_Language)
        {
         if (Gbl.Usrs.Me.UsrDat.FrstName[0])
           {
	    /***** New year greeting *****/
	    if (Gbl.Now.Date.Day   == 1 &&
		Gbl.Now.Date.Month == 1)
	       Ale_ShowAlert (Ale_INFO,Txt_NEW_YEAR_GREETING,
			      Gbl.Now.Date.Year);

            /***** Birthday congratulation *****/
            if (Gbl.Usrs.Me.UsrDat.Birthday.Day   == Gbl.Now.Date.Day &&
                Gbl.Usrs.Me.UsrDat.Birthday.Month == Gbl.Now.Date.Month)
               if (Usr_CheckIfMyBirthdayHasNotBeenCongratulated ())
                 {
                  /* Mark my birthday as already congratulated */
                  Usr_InsertMyBirthday ();

		  /* Begin alert */
		  Ale_ShowAlertAndButton1 (Ale_INFO,Txt_Happy_birthday_X,
			                   Gbl.Usrs.Me.UsrDat.FrstName);

		  /* Show cake icon */
		  HTM_IMG (Gbl.Prefs.URLIconSet,"birthday-cake.svg",NULL,
			   "class=\"ICO160x160\"");

		  /* End alert */
		  Ale_ShowAlertAndButton2 (ActUnk,NULL,NULL,
		                           NULL,NULL,
		                           Btn_NO_BUTTON,NULL);
                 }

	    /***** Alert with button to check email address *****/
	    if ( Gbl.Usrs.Me.UsrDat.Email[0] &&
		!Gbl.Usrs.Me.UsrDat.EmailConfirmed)	// Email needs to be confirmed
	       Ale_ShowAlertAndButton (ActFrmMyAcc,NULL,NULL,
	                               NULL,NULL,
				       Btn_CONFIRM_BUTTON,Txt_Check,
				       Ale_WARNING,Txt_Please_check_your_email_address);
           }

         /***** Games tool *****/
         /*
         Ale_ShowAlert (Ale_INFO,
			"Herramienta <a href=\"https://github.com/acanas/swad-core/wiki/ASSESSMENT.Games.es\" target=\"_blank\">"
			"Evaluaci&oacute;n &gt; Juegos</a><br />"
			"<br />"
			"Imagen de la izquierda: pantalla proyectada en el aula por el profesor.<br />"
			"Imagen de la derecha: pantalla del estudiante en su m&oacute;vil."
			"<br />"
			"<br />"
	                "<img src=\"/img/juego.png\" alt=\"Juegos\" style=\"width:100%%\">");
	 */

         /***** Institutional video *****/
         /*
         Ale_ShowAlert (Ale_INFO,
			"<a href=\"https://abierta.ugr.es/creative_commons/\" target=\"_blank\">"
			"Curso MOOC LICENCIAS CREATIVE COMMONS Y OER</a><br />"
			"Reconocimiento de 1 cr&eacute;dito por actividades universitarias<br />"
			"&iexcl;&Uacute;ltimos d&iacute;as!<br />"
			"<br />"
			"<video style=\"width:480px; height:270px;\""
			" poster=\"/img/abierta-ugr-creative-commons1280x720.jpg\""
			" controls>"
			"<source src=\"https://abierta.ugr.es/creative_commons/promo_cc.mp4\""
			" type=\"video/mp4\">"
	                "<img src=\"/img/abierta-ugr-creative-commons1280x720.jpg\""
	                " class=\"img-responsive\""
	                " alt=\"Responsive image\">"
	                "</video>");
	 */

         /***** Show the global announcements I have not seen *****/
         Ann_ShowMyAnnouncementsNotMarkedAsSeen ();
        }
      else
	 /* The current language is not my preferred language
	    ==> change automatically to my language */
         Ale_ShowAlert (Ale_INFO,Txt_Switching_to_LANGUAGE[Gbl.Usrs.Me.UsrDat.Prefs.Language]);
     }
  }

/*****************************************************************************/
/************ Write birthday string to insert or update database *************/
/*****************************************************************************/
// It can include start and ending apostrophes

void Usr_CreateBirthdayStrDB (const struct UsrData *UsrDat,
                              char BirthdayStrDB[Usr_BIRTHDAY_STR_DB_LENGTH + 1])
  {
   if (UsrDat->Birthday.Year  == 0 ||
       UsrDat->Birthday.Month == 0 ||
       UsrDat->Birthday.Day   == 0)
      Str_Copy (BirthdayStrDB,"NULL",Usr_BIRTHDAY_STR_DB_LENGTH);	// Without apostrophes
   else
      snprintf (BirthdayStrDB,Usr_BIRTHDAY_STR_DB_LENGTH + 1,
                "'%04u-%02u-%02u'",	// With apostrophes
	        UsrDat->Birthday.Year,
	        UsrDat->Birthday.Month,
	        UsrDat->Birthday.Day);
  }

/*****************************************************************************/
/*************** Check if my birthday is already congratulated ***************/
/*****************************************************************************/

static bool Usr_CheckIfMyBirthdayHasNotBeenCongratulated (void)
  {
   /***** Delete old birthdays *****/
   return (DB_QueryCOUNT ("can not check if my birthday has been congratulated",
			  "SELECT COUNT(*)"
			   " FROM usr_birthdays_today"
			  " WHERE UsrCod=%ld",
			  Gbl.Usrs.Me.UsrDat.UsrCod) == 0);
  }

/*****************************************************************************/
/*** Insert my user's code in the table of birthdays already congratulated ***/
/*****************************************************************************/

static void Usr_InsertMyBirthday (void)
  {
   /***** Delete old birthdays *****/
   DB_QueryDELETE ("can not delete old birthdays",
		   "DELETE FROM usr_birthdays_today"
		   " WHERE Today<>CURDATE()");

   /***** Insert new birthday *****/
   DB_QueryINSERT ("can not insert birthday",
		   "INSERT INTO usr_birthdays_today"
	           " (UsrCod,Today)"
	           " VALUES"
	           " (%ld,CURDATE())",
		   Gbl.Usrs.Me.UsrDat.UsrCod);
  }

/*****************************************************************************/
/************************* Filter some user's data ***************************/
/*****************************************************************************/

void Usr_FilterUsrBirthday (struct Date *Birthday)
  {
   /***** Fix birthday *****/
   if (Birthday->Year < Gbl.Now.Date.Year-99 ||
       Birthday->Year > Gbl.Now.Date.Year-16)
      Birthday->Year  =
      Birthday->Month =
      Birthday->Day   = 0;
  }

/*****************************************************************************/
/************************ Write form for user log in *************************/
/*****************************************************************************/

void Usr_PutFormLogIn (void)
  {
   extern const char *Txt_Log_in;

   /***** Link to log in form *****/
   Frm_BeginForm (ActFrmLogIn);
   Ico_PutIconLink ("sign-in-alt-white.svg",Txt_Log_in);
   Frm_EndForm ();
  }

/*****************************************************************************/
/******************* Write type and name of logged user **********************/
/*****************************************************************************/

void Usr_WriteLoggedUsrHead (void)
  {
   extern const char *The_ClassUsr[The_NUM_THEMES];
   extern const char *Txt_Role;
   extern const char *Txt_ROLES_SINGUL_Abc[Rol_NUM_ROLES][Usr_NUM_SEXS];
   unsigned NumAvailableRoles = Rol_GetNumAvailableRoles ();

   HTM_DIV_Begin ("class=\"HEAD_USR %s\"",The_ClassUsr[Gbl.Prefs.Theme]);

   /***** User's role *****/
   if (NumAvailableRoles == 1)
     {
      Frm_BeginForm (ActFrmRolSes);
      HTM_BUTTON_SUBMIT_Begin (Txt_Role,
			       Str_BuildStringStr ("BT_LINK %s",
						   The_ClassUsr[Gbl.Prefs.Theme]),
			       NULL);
      Str_FreeString ();
      HTM_Txt (Txt_ROLES_SINGUL_Abc[Gbl.Usrs.Me.Role.Logged][Gbl.Usrs.Me.UsrDat.Sex]);
      HTM_BUTTON_End ();
      Frm_EndForm ();

      HTM_Colon ();
     }
   else
      Rol_PutFormToChangeMyRole ("SEL_ROLE");
   HTM_NBSP ();

   /***** Show my photo *****/
   Pho_ShowUsrPhotoIfAllowed (&Gbl.Usrs.Me.UsrDat,"PHOTO18x24",Pho_ZOOM,false);

   /***** User's name *****/
   if (Gbl.Usrs.Me.UsrDat.FrstName[0])
      HTM_TxtF ("&nbsp;%s",Gbl.Usrs.Me.UsrDat.FrstName);

   HTM_DIV_End ();
  }

/*****************************************************************************/
/*************** Put a form to close current session (log out) ***************/
/*****************************************************************************/

void Usr_PutFormLogOut (void)
  {
   extern const char *Txt_Log_out;

   /***** Link to log out form *****/
   Frm_BeginForm (ActLogOut);
   Ico_PutIconLink ("sign-out-alt-white.svg",Txt_Log_out);
   Frm_EndForm ();
  }

/*****************************************************************************/
/******* Get parameter with my plain user's ID or nickname from a form *******/
/*****************************************************************************/

void Usr_GetParamUsrIdLogin (void)
  {
   Par_GetParToText ("UsrId",Gbl.Usrs.Me.UsrIdLogin,Cns_MAX_BYTES_EMAIL_ADDRESS);
   // Users' IDs are always stored internally without leading zeros
   Str_RemoveLeadingZeros (Gbl.Usrs.Me.UsrIdLogin);
  }

/*****************************************************************************/
/******* Get parameter user's identificator of other user from a form ********/
/*****************************************************************************/

static void Usr_GetParamOtherUsrIDNickOrEMail (void)
  {
   /***** Get parameter with the plain user's ID, @nick or email of another user *****/
   Par_GetParToText ("OtherUsrIDNickOrEMail",
                     Gbl.Usrs.Other.UsrDat.UsrIDNickOrEmail,
                     Cns_MAX_BYTES_EMAIL_ADDRESS);

   // If it's a user's ID (if does not contain '@')
   if (strchr (Gbl.Usrs.Other.UsrDat.UsrIDNickOrEmail,(int) '@') != NULL)	// '@' not found
     {
      // Users' IDs are always stored internally in capitals and without leading zeros
      Str_RemoveLeadingZeros (Gbl.Usrs.Other.UsrDat.UsrIDNickOrEmail);
      // Str_ConvertToUpperText (Gbl.Usrs.Other.UsrDat.UsrIDNickOrEmail);
     }
  }

/*****************************************************************************/
/******* Get parameter user's identificator of other user from a form ********/
/******* and get user's data                                          ********/
/*****************************************************************************/
// Returns the number of users for a given ID, @nick or email

unsigned Usr_GetParamOtherUsrIDNickOrEMailAndGetUsrCods (struct ListUsrCods *ListUsrCods)
  {
   extern const char *Txt_The_ID_nickname_or_email_X_is_not_valid;
   bool Wrong = false;

   /***** Reset default list of users' codes *****/
   ListUsrCods->NumUsrs = 0;
   ListUsrCods->Lst = NULL;

   /***** Get parameter with the plain user's ID, @nick or email of another user *****/
   Usr_GetParamOtherUsrIDNickOrEMail ();

   /***** Check if it's an ID, a nickname or an email address *****/
   if (Gbl.Usrs.Other.UsrDat.UsrIDNickOrEmail[0])
     {
      if (Nck_CheckIfNickWithArrIsValid (Gbl.Usrs.Other.UsrDat.UsrIDNickOrEmail))	// 1: It's a nickname
	{
	 if ((Gbl.Usrs.Other.UsrDat.UsrCod = Nck_GetUsrCodFromNickname (Gbl.Usrs.Other.UsrDat.UsrIDNickOrEmail)) > 0)
	   {
	    ListUsrCods->NumUsrs = 1;	// One user found
	    Usr_AllocateListUsrCods (ListUsrCods);
	    ListUsrCods->Lst[0] = Gbl.Usrs.Other.UsrDat.UsrCod;
	   }
	}
      else if (Mai_CheckIfEmailIsValid (Gbl.Usrs.Other.UsrDat.UsrIDNickOrEmail))		// 2: It's an email
	{
	 if ((Gbl.Usrs.Other.UsrDat.UsrCod = Mai_GetUsrCodFromEmail (Gbl.Usrs.Other.UsrDat.UsrIDNickOrEmail)) > 0)
	   {
	    ListUsrCods->NumUsrs = 1;	// One user found
	    Usr_AllocateListUsrCods (ListUsrCods);
	    ListUsrCods->Lst[0] = Gbl.Usrs.Other.UsrDat.UsrCod;
	   }
	}
      else											// 3: It's not a nickname nor email
	{
	 // Users' IDs are always stored internally in capitals and without leading zeros
	 Str_RemoveLeadingZeros (Gbl.Usrs.Other.UsrDat.UsrIDNickOrEmail);
	 if (ID_CheckIfUsrIDIsValid (Gbl.Usrs.Other.UsrDat.UsrIDNickOrEmail))
	   {
	    /* Allocate space for the list */
	    ID_ReallocateListIDs (&Gbl.Usrs.Other.UsrDat,1);

	    Str_Copy (Gbl.Usrs.Other.UsrDat.IDs.List[0].ID,
	              Gbl.Usrs.Other.UsrDat.UsrIDNickOrEmail,
	              sizeof (Gbl.Usrs.Other.UsrDat.IDs.List[0].ID) - 1);
	    Str_ConvertToUpperText (Gbl.Usrs.Other.UsrDat.IDs.List[0].ID);

	    /* Check if user's ID exists in database */
	    ID_GetListUsrCodsFromUsrID (&Gbl.Usrs.Other.UsrDat,NULL,ListUsrCods,false);
	   }
	 else	// Not a valid user's nickname, email or ID
	    Wrong = true;
	}
     }
   else	// Empty string
      Wrong = true;

   if (Wrong)
      /***** String is not a valid user's nickname, email or ID *****/
      Ale_ShowAlert (Ale_WARNING,Txt_The_ID_nickname_or_email_X_is_not_valid,
	             Gbl.Usrs.Other.UsrDat.UsrIDNickOrEmail);

   return ListUsrCods->NumUsrs;
  }

/*****************************************************************************/
/********* Put hidden parameter encrypted user's code of other user **********/
/*****************************************************************************/

void Usr_PutParamMyUsrCodEncrypted (void *EncryptedUsrCod)
  {
   Usr_PutParamUsrCodEncrypted ((const char *) EncryptedUsrCod);
  }

void Usr_PutParamOtherUsrCodEncrypted (void *EncryptedUsrCod)
  {
   Usr_PutParamUsrCodEncrypted ((const char *) EncryptedUsrCod);
  }

void Usr_PutParamUsrCodEncrypted (const char EncryptedUsrCod[Cry_BYTES_ENCRYPTED_STR_SHA256_BASE64 + 1])
  {
   Par_PutHiddenParamString (NULL,"OtherUsrCod",EncryptedUsrCod);
  }

/*****************************************************************************/
/********* Get hidden parameter encrypted user's code of other user **********/
/*****************************************************************************/

void Usr_GetParamOtherUsrCodEncrypted (struct UsrData *UsrDat)
  {
   Par_GetParToText ("OtherUsrCod",UsrDat->EnUsrCod,
                     Cry_BYTES_ENCRYPTED_STR_SHA256_BASE64);
   if (UsrDat->EnUsrCod[0])	// If parameter exists...
     {
      Usr_GetUsrCodFromEncryptedUsrCod (UsrDat);
      if (UsrDat->UsrCod <= 0)	// Check is user's code is valid
         Err_WrongUserExit ();
     }
   else
      UsrDat->UsrCod = -1L;
  }

/*****************************************************************************/
/********* Get hidden parameter encrypted user's code of other user **********/
/*****************************************************************************/

void Usr_GetParamOtherUsrCodEncryptedAndGetListIDs (void)
  {
   Usr_GetParamOtherUsrCodEncrypted (&Gbl.Usrs.Other.UsrDat);
   if (Gbl.Usrs.Other.UsrDat.UsrCod > 0)	// If parameter exists...
      ID_GetListIDsFromUsrCod (&Gbl.Usrs.Other.UsrDat);
   else       					// Parameter does not exist
     {
      Gbl.Usrs.Other.UsrDat.UsrIDNickOrEmail[0] = '\0';
      Gbl.Usrs.Other.UsrDat.IDs.Num = 0;
      Gbl.Usrs.Other.UsrDat.IDs.List = NULL;
     }
  }

/*****************************************************************************/
/*** Get parameter encrypted user's code of other user and get user's data ***/
/*****************************************************************************/
// Return true if user exists

bool Usr_GetParamOtherUsrCodEncryptedAndGetUsrData (void)
  {
   /***** Get parameter with encrypted user's code *****/
   Usr_GetParamOtherUsrCodEncryptedAndGetListIDs ();

   /***** Check if user exists and get her/his data *****/
   if (Usr_ChkUsrCodAndGetAllUsrDataFromUsrCod (&Gbl.Usrs.Other.UsrDat,
                                                Usr_DONT_GET_PREFS,
                                                Usr_DONT_GET_ROLE_IN_CURRENT_CRS))
      // Existing user
      return true;

   return false;
  }

/*****************************************************************************/
/** Check and get user data from session, from internal or external login... */
/*****************************************************************************/

void Usr_ChkUsrAndGetUsrData (void)
  {
   extern const char *Txt_The_session_has_expired;
   struct
     {
      bool PutForm;
      Act_Action_t Action;
      void (*FuncParams) (void);
     } FormLogin =
     {
      false,
      ActLogIn,
      NULL
     };
   Act_Action_t Action;

   if (Gbl.Session.HasBeenDisconnected)
     {
      if (!Gbl.Action.UsesAJAX)
	{
	 Gbl.Action.Act = ActLogOut;
	 Tab_SetCurrentTab ();
	 Ale_ShowAlert (Ale_WARNING,Txt_The_session_has_expired);
	 FormLogin.PutForm = true;
	}
     }
   else	// !Gbl.Session.HasBeenDisconnected
     {
      /***** Check user and get user's data *****/
      if (Gbl.Action.Act == ActCreUsrAcc)
	{
	 /***** Create my new account and login *****/
	 if (Acc_CreateMyNewAccountAndLogIn ())		// User logged in
	   {
	    Gbl.Usrs.Me.Logged = true;
	    Usr_SetMyPrefsAndRoles ();

	    Act_AdjustCurrentAction ();
	    Ses_CreateSession ();

	    /* Set settings from current IP */
	    Set_SetSettingsFromIP ();

	    /* Send message via email to confirm the new email address */
	    Mai_SendMailMsgToConfirmEmail ();
	    Ale_ShowAlerts (NULL);	// Show alert after sending email confirmation
	   }
	}
      else	// Gbl.Action.Act != ActCreUsrAcc
	{
	 /***** Check user and get user's data *****/
	 if (Gbl.Session.IsOpen)
	   {
	    if (Usr_ChkUsrAndGetUsrDataFromSession ())	// User logged in
	      {
	       Gbl.Usrs.Me.Logged = true;

	       Usr_SetMyPrefsAndRoles ();

	       if (Gbl.Action.IsAJAXAutoRefresh)	// If refreshing ==> don't refresh LastTime in session
		  Ses_UpdateSessionLastRefreshInDB ();
	       else
		 {
		  Act_AdjustCurrentAction ();
		  Ses_UpdateSessionDataInDB ();
		  Con_DB_UpdateMeInConnectedList ();
		 }
	      }
	    else
	       FormLogin.PutForm = true;
	   }
	 else if (Gbl.Action.Act == ActLogIn ||
	          Gbl.Action.Act == ActLogInUsrAgd)	// Login using @nickname, email or ID from form
	   {
	    if (Usr_ChkUsrAndGetUsrDataFromDirectLogin ())	// User logged in
	      {
	       Gbl.Usrs.Me.Logged = true;
	       Usr_SetMyPrefsAndRoles ();

	       Act_AdjustCurrentAction ();
	       Ses_CreateSession ();

	       Set_SetSettingsFromIP ();	// Set settings from current IP
	      }
	    else
	      {
	       FormLogin.PutForm = true;
	       if (Gbl.Action.Act == ActLogInUsrAgd)
		 {
	          FormLogin.Action = ActLogInUsrAgd;
	          FormLogin.FuncParams = Agd_PutParamAgd;
		 }
	      }
	   }
	 else if (Gbl.Action.Act == ActLogInNew)	// Empty account without password, login using encrypted user's code
	   {
	    /***** Get user's data *****/
	    Usr_GetParamOtherUsrCodEncrypted (&Gbl.Usrs.Me.UsrDat);
            Usr_GetUsrCodFromEncryptedUsrCod (&Gbl.Usrs.Me.UsrDat);
            if (Usr_ChkUsrCodAndGetAllUsrDataFromUsrCod (&Gbl.Usrs.Me.UsrDat,
                                                         Usr_GET_PREFS,
                                                         Usr_GET_ROLE_IN_CURRENT_CRS))
	      {
               // User logged in
	       Gbl.Usrs.Me.Logged = true;
	       Usr_SetMyPrefsAndRoles ();

	       Act_AdjustCurrentAction ();
	       Ses_CreateSession ();

	       Set_SetSettingsFromIP ();	// Set settings from current IP
	      }
	    else
	       FormLogin.PutForm = true;
	   }
	}
     }

   /***** If session disconnected or error in login, show form to login *****/
   if (FormLogin.PutForm)
     {
      Usr_WriteFormLogin (FormLogin.Action,FormLogin.FuncParams);
      Err_ShowErrorAndExit (NULL);
     }

   /***** Adjust tab and action *****/
   if (!Gbl.Action.UsesAJAX)
     {
      if (!Gbl.Usrs.Me.Logged &&	// No user logged...
	  Gbl.Action.Act == ActUnk)	// ...and unknown action
	 Act_AdjustActionWhenNoUsrLogged ();

      /***** When I change to another tab, go to:
             - my last action in that tab if it is known, or
             - the first option allowed *****/
      if (Gbl.Action.Act == ActMnu)
	{
	 /* Get my last action in current tab */
	 Action = (Gbl.Usrs.Me.Logged) ? MFU_GetMyLastActionInCurrentTab () :
	                                 ActUnk;
	 if (Action == ActUnk)
	    /* Get the first option allowed */
	    Action = Mnu_GetFirstActionAvailableInCurrentTab ();

	 Gbl.Action.Act = (Action == ActUnk) ? ((Gbl.Usrs.Me.Logged) ? ActSeeGblTL :	// Default action if logged
								       ActFrmLogIn) :		// Default action if not logged
					       Action;
	 Tab_SetCurrentTab ();
	}
     }
  }

/*****************************************************************************/
/************** Check user and get user's data when direct login *************/
/*****************************************************************************/
// Returns true if user logged in successfully
// Returns false if user not logged in

static bool Usr_ChkUsrAndGetUsrDataFromDirectLogin (void)
  {
   struct ListUsrCods ListUsrCods;
   bool PasswordCorrect = false;

   /***** Check if user typed anything *****/
   if (!Gbl.Usrs.Me.UsrIdLogin)
     {
      Usr_ShowAlertUsrDoesNotExistsOrWrongPassword ();
      return false;
     }
   if (!Gbl.Usrs.Me.UsrIdLogin[0])
     {
      Usr_ShowAlertUsrDoesNotExistsOrWrongPassword ();
      return false;
     }

   /***** Check if user has typed his user's ID, his nickname or his email address *****/
   if (Nck_CheckIfNickWithArrIsValid (Gbl.Usrs.Me.UsrIdLogin))	// 1: It's a nickname
     {
      // User is trying to log using his/her nickname
      if ((Gbl.Usrs.Me.UsrDat.UsrCod = Nck_GetUsrCodFromNickname (Gbl.Usrs.Me.UsrIdLogin)) <= 0)
	{
	 Usr_ShowAlertUsrDoesNotExistsOrWrongPassword ();
	 return false;
	}
     }
   else if (Mai_CheckIfEmailIsValid (Gbl.Usrs.Me.UsrIdLogin))		// 2: It's an email
     {
      // User is trying to log using his/her email
      if ((Gbl.Usrs.Me.UsrDat.UsrCod = Mai_GetUsrCodFromEmail (Gbl.Usrs.Me.UsrIdLogin)) <= 0)
	{
	 Usr_ShowAlertUsrDoesNotExistsOrWrongPassword ();
	 return false;
	}
     }
   else									// 3: It's not a nickname nor email
     {
      // Users' IDs are always stored internally in capitals and without leading zeros
      Str_RemoveLeadingZeros (Gbl.Usrs.Me.UsrIdLogin);
      if (ID_CheckIfUsrIDIsValid (Gbl.Usrs.Me.UsrIdLogin))
	{
	 /***** Allocate space for the list *****/
	 ID_ReallocateListIDs (&Gbl.Usrs.Me.UsrDat,1);

	 Str_Copy (Gbl.Usrs.Me.UsrDat.IDs.List[0].ID,Gbl.Usrs.Me.UsrIdLogin,
	           sizeof (Gbl.Usrs.Me.UsrDat.IDs.List[0].ID) - 1);
	 Str_ConvertToUpperText (Gbl.Usrs.Me.UsrDat.IDs.List[0].ID);

	 /* Check if user's ID exists in database, and get user's data */
	 if (ID_GetListUsrCodsFromUsrID (&Gbl.Usrs.Me.UsrDat,
	                                 Gbl.Usrs.Me.LoginEncryptedPassword,	// Check password
	                                 &ListUsrCods,false))
	   {
	    if (ListUsrCods.NumUsrs == 1)
	      {
	       /* Free memory used for list of users' codes found for this ID */
	       Usr_FreeListUsrCods (&ListUsrCods);

	       PasswordCorrect = true;
	      }
	    else	// ListUsrCods.NumUsrs > 1
	      {
	       /* Free memory used for list of users' codes found for this ID */
	       Usr_FreeListUsrCods (&ListUsrCods);

	       Usr_ShowAlertThereAreMoreThanOneUsr ();
	       return false;
	      }
	   }
	 else if (ID_GetListUsrCodsFromUsrID (&Gbl.Usrs.Me.UsrDat,
	                                      NULL,				// Don't check password
	                                      &ListUsrCods,false))
	   {
	    if (ListUsrCods.NumUsrs == 1)
	      {
	       /* Free memory used for list of users' codes found for this ID */
	       Usr_FreeListUsrCods (&ListUsrCods);

	       if (Pwd_CheckPendingPassword ())
		 {
	 	  Pwd_AssignMyPendingPasswordToMyCurrentPassword ();
	       	  PasswordCorrect = true;
	         }
	       else
	 	 {
		  Usr_ShowAlertUsrDoesNotExistsOrWrongPassword ();
	 	  return false;
	 	 }
	      }
	    else	// ListUsrCods.NumUsrs > 1
	      {
	       /* Free memory used for list of users' codes found for this ID */
	       Usr_FreeListUsrCods (&ListUsrCods);

	       Usr_ShowAlertThereAreMoreThanOneUsr ();
	       return false;
	      }
	   }
	 else	// No users found for this ID
	   {
	    Usr_ShowAlertUsrDoesNotExistsOrWrongPassword ();
	    return false;
	   }
	}
      else	// String is not a valid user's nickname, email or ID
	{
	 Usr_ShowAlertUsrDoesNotExistsOrWrongPassword ();
	 return false;
	}
     }

   /***** Get user's data *****/
   Usr_GetAllUsrDataFromUsrCod (&Gbl.Usrs.Me.UsrDat,
                                Usr_GET_PREFS,
                                Usr_GET_ROLE_IN_CURRENT_CRS);

   /***** Check password *****/
   /* Check user's password:
      is the encrypted password typed by user or coming from the session
      the same as that stored in database? */
   if (!PasswordCorrect)
      PasswordCorrect = Pwd_CheckCurrentPassword ();

   if (!PasswordCorrect)	// If my password is not correct...
     {
      if (Pwd_CheckPendingPassword ())
	 Pwd_AssignMyPendingPasswordToMyCurrentPassword ();
      else
	{
	 Usr_ShowAlertUsrDoesNotExistsOrWrongPassword ();
	 return false;
	}
     }

   return true;
  }

/*****************************************************************************/
/******** Check user and get user's data when the session is open ************/
/*****************************************************************************/

static bool Usr_ChkUsrAndGetUsrDataFromSession (void)
  {
   /***** Session is open and user's code is get from session *****/
   Gbl.Usrs.Me.UsrDat.UsrCod = Gbl.Session.UsrCod;

   /* Check if user exists in database, and get his/her data */
   if (!Usr_ChkUsrCodAndGetAllUsrDataFromUsrCod (&Gbl.Usrs.Me.UsrDat,
                                                 Usr_GET_PREFS,
                                                 Usr_GET_ROLE_IN_CURRENT_CRS))
     {
      Usr_ShowAlertUsrDoesNotExistsOrWrongPassword ();
      return false;
     }

   /* Check user's password:
      the encrypted password typed by user or coming from the session
      is the same as the stored in database? */
   if (!Pwd_CheckCurrentPassword ())	// If my password is not correct...
     {
      Usr_ShowAlertUsrDoesNotExistsOrWrongPassword ();
      return false;
     }

   return true;
  }

/*****************************************************************************/
/** Show alert indicating that user does not exists or password is incorrect */
/*****************************************************************************/

static void Usr_ShowAlertUsrDoesNotExistsOrWrongPassword (void)
  {
   extern const char *Txt_The_user_does_not_exist_or_password_is_incorrect;

   Ale_ShowAlert (Ale_WARNING,Txt_The_user_does_not_exist_or_password_is_incorrect);
  }

/*****************************************************************************/
/****** Show alert indicating that more than one user share the same ID ******/
/*****************************************************************************/

static void Usr_ShowAlertThereAreMoreThanOneUsr (void)
  {
   extern const char *Txt_There_are_more_than_one_user_with_the_ID_X_Please_type_a_nick_or_email;

   Gbl.Action.Act = ActFrmLogIn;
   Tab_SetCurrentTab ();
   Ale_ShowAlert (Ale_WARNING,Txt_There_are_more_than_one_user_with_the_ID_X_Please_type_a_nick_or_email,
	          Gbl.Usrs.Me.UsrIdLogin);
  }

/*****************************************************************************/
/********************** Set my settings and my roles *************************/
/*****************************************************************************/

static void Usr_SetMyPrefsAndRoles (void)
  {
   extern const char *The_ThemeId[The_NUM_THEMES];
   extern const char *Ico_IconSetId[Ico_NUM_ICON_SETS];
   char URL[PATH_MAX + 1];
   bool GetRoleAndActionFromLastData;
   Act_Action_t LastSuperAction;
   bool JustAfterLogin = Gbl.Action.Act == ActLogIn    ||
	                 Gbl.Action.Act == ActLogInLan ||
	                 Gbl.Action.Act == ActLogInNew ||
			 Gbl.Action.Act == ActAnnSee;

   // In this point I am logged

   /***** Set my language if unknown *****/
   if (Gbl.Usrs.Me.UsrDat.Prefs.Language == Lan_LANGUAGE_UNKNOWN)		// I have not chosen language
      Lan_UpdateMyLanguageToCurrentLanguage ();	// Update my language in database

   /***** Set settings from my settings *****/
   Gbl.Prefs.FirstDayOfWeek = Gbl.Usrs.Me.UsrDat.Prefs.FirstDayOfWeek;
   Gbl.Prefs.DateFormat     = Gbl.Usrs.Me.UsrDat.Prefs.DateFormat;
   Gbl.Prefs.Menu           = Gbl.Usrs.Me.UsrDat.Prefs.Menu;
   Gbl.Prefs.SideCols       = Gbl.Usrs.Me.UsrDat.Prefs.SideCols;

   Gbl.Prefs.Theme   = Gbl.Usrs.Me.UsrDat.Prefs.Theme;
   snprintf (URL,sizeof (URL),"%s/%s",
	     Cfg_URL_ICON_THEMES_PUBLIC,The_ThemeId[Gbl.Prefs.Theme]);
   Str_Copy (Gbl.Prefs.URLTheme  ,URL,sizeof (Gbl.Prefs.URLTheme  ) - 1);

   Gbl.Prefs.IconSet = Gbl.Usrs.Me.UsrDat.Prefs.IconSet;
   snprintf (URL,sizeof (URL),"%s/%s",
	     Cfg_URL_ICON_SETS_PUBLIC,Ico_IconSetId[Gbl.Prefs.IconSet]);
   Str_Copy (Gbl.Prefs.URLIconSet,URL,sizeof (Gbl.Prefs.URLIconSet) - 1);

   /***** Construct the path to my directory *****/
   Usr_ConstructPathUsr (Gbl.Usrs.Me.UsrDat.UsrCod,Gbl.Usrs.Me.PathDir);

   /***** Check if my photo exists and create a link to it ****/
   Gbl.Usrs.Me.MyPhotoExists = Pho_BuildLinkToPhoto (&Gbl.Usrs.Me.UsrDat,Gbl.Usrs.Me.PhotoURL);

   /***** Get my last data *****/
   Usr_GetMyLastData ();
   if (JustAfterLogin)	// If I just logged in...
     {
      /***** WhatToSearch is stored in session,
             but in login it is got from user's last data *****/
      Gbl.Search.WhatToSearch = Gbl.Usrs.Me.UsrLast.WhatToSearch;

      /***** Location in hierarchy and role are stored in session,
             but in login the are got from user's last data *****/
      if (Gbl.Hierarchy.Level == HieLvl_SYS)	// No country selected
        {
	 /***** Copy last hierarchy to current hierarchy *****/
	 Hie_SetHierarchyFromUsrLastHierarchy ();

	 /* Course may have changed ==> get my role in current course again */
	 if (Gbl.Hierarchy.Level == HieLvl_CRS)	// Course selected
	    Gbl.Usrs.Me.UsrDat.Roles.InCurrentCrs = Rol_GetMyRoleInCrs (Gbl.Hierarchy.Crs.CrsCod);

	 // role and action will be got from last data
         GetRoleAndActionFromLastData = true;
        }
      else	// Country (and may be institution, center, degree or course) selected
	 // Role and action will be got from last data
	 // only if I am in the same hierarchy location that the stored one
	 GetRoleAndActionFromLastData =
	    (Gbl.Hierarchy.Level == Gbl.Usrs.Me.UsrLast.LastHie.Scope &&	// The same scope...
	     Gbl.Hierarchy.Cod   == Gbl.Usrs.Me.UsrLast.LastHie.Cod);		// ...and code in hierarchy

      /***** Get role and action from last data *****/
      if (GetRoleAndActionFromLastData)
        {
	 /* Get role from last data */
	 Gbl.Usrs.Me.Role.Logged = Gbl.Usrs.Me.UsrLast.LastRole;

	 /* Last action is really got only if last access is recent */
	 if (Gbl.Usrs.Me.UsrLast.LastTime >= Gbl.StartExecutionTimeUTC -
	                                     Cfg_MAX_TIME_TO_REMEMBER_LAST_ACTION_ON_LOGIN)
	   {
	    /* Get action from last data */
	    LastSuperAction = Act_GetSuperAction (Gbl.Usrs.Me.UsrLast.LastAct);
	    if (LastSuperAction != ActUnk)
	      {
	       Gbl.Action.Act = LastSuperAction;
	       Tab_SetCurrentTab ();
	      }
	   }
	 /* If action is not set to last action,
	    it will be set later to a default action */
        }
     }

   /***** Set my roles *****/
   Rol_SetMyRoles ();
  }

/*****************************************************************************/
/************** Show forms to log out and to change my role ******************/
/*****************************************************************************/

void Usr_ShowFormsLogoutAndRole (void)
  {
   extern const char *Hlp_PROFILE_Session_role;
   extern const char *The_ClassFormInBox[The_NUM_THEMES];
   extern const char *Txt_Session;
   extern const char *Txt_Role;
   extern const char *Txt_You_are_now_LOGGED_IN_as_X;
   extern const char *Txt_logged[Usr_NUM_SEXS];
   extern const char *Txt_ROLES_SINGUL_abc[Rol_NUM_ROLES][Usr_NUM_SEXS];
   extern const char *Txt_ROLES_SINGUL_Abc[Rol_NUM_ROLES][Usr_NUM_SEXS];

   /***** Write message with my new logged role *****/
   if (Gbl.Usrs.Me.Role.HasChanged)
      Ale_ShowAlert (Ale_SUCCESS,Txt_You_are_now_LOGGED_IN_as_X,
	             Txt_logged[Gbl.Usrs.Me.UsrDat.Sex],
	             Txt_ROLES_SINGUL_abc[Gbl.Usrs.Me.Role.Logged][Gbl.Usrs.Me.UsrDat.Sex]);

   /***** Begin box *****/
   Box_BoxBegin (NULL,Txt_Session,
                 Usr_PutLinkToLogOut,NULL,
                 Hlp_PROFILE_Session_role,Box_NOT_CLOSABLE);

   /***** Put a form to change my role *****/
   if (Rol_GetNumAvailableRoles () == 1)
     {
      HTM_SPAN_Begin ("class=\"DAT\"");
	 HTM_TxtColonNBSP (Txt_Role);
      HTM_SPAN_End ();

      HTM_SPAN_Begin ("class=\"DAT_N_BOLD\"");
	 HTM_Txt (Txt_ROLES_SINGUL_Abc[Gbl.Usrs.Me.Role.Logged][Gbl.Usrs.Me.UsrDat.Sex]);
      HTM_SPAN_End ();
     }
   else
     {
      HTM_LABEL_Begin ("class=\"%s\"",The_ClassFormInBox[Gbl.Prefs.Theme]);
	 HTM_TxtColonNBSP (Txt_Role);
	 Rol_PutFormToChangeMyRole (NULL);
      HTM_LABEL_End ();
     }

   /***** End box *****/
   Box_BoxEnd ();
  }

/*****************************************************************************/
/************** Put an icon (form) to close the current session **************/
/*****************************************************************************/

static void Usr_PutLinkToLogOut (__attribute__((unused)) void *Args)
  {
   extern const char *Txt_Log_out;

   /***** Put form to log out *****/
   Lay_PutContextualLinkOnlyIcon (ActLogOut,NULL,
				  NULL,NULL,
				  "sign-out-alt-red.svg",
				  Txt_Log_out);
  }

/*****************************************************************************/
/******* Check a user's code and get all user's data from user's code ********/
/*****************************************************************************/
// Input: UsrDat->UsrCod must hold a valid user code
// Output: When true ==> UsrDat will hold all user's data
//         When false ==> UsrDat is reset, except user's code

bool Usr_ChkUsrCodAndGetAllUsrDataFromUsrCod (struct UsrData *UsrDat,
                                              Usr_GetPrefs_t GetPrefs,
                                              Usr_GetRoleInCurrentCrs_t GetRoleInCurrentCrs)
  {
   /***** Check if a user exists having this user's code *****/
   if (Usr_ChkIfUsrCodExists (UsrDat->UsrCod))
     {
      /* Get user's data */
      Usr_GetAllUsrDataFromUsrCod (UsrDat,GetPrefs,GetRoleInCurrentCrs);
      return true;
     }

   /***** No user's code found *****/
   UsrDat->UsrIDNickOrEmail[0] = '\0';
   Usr_ResetUsrDataExceptUsrCodAndIDs (UsrDat);
   return false;
  }

/*****************************************************************************/
/********* Update my last accessed course, tab and time in database **********/
/*****************************************************************************/

void Usr_UpdateMyLastData (void)
  {
   /***** Check if it exists an entry for me *****/
   if (DB_QueryCOUNT ("can not get last user's click",
		      "SELECT COUNT(*)"
		       " FROM usr_last"
		      " WHERE UsrCod=%ld",
		      Gbl.Usrs.Me.UsrDat.UsrCod))
      /***** Update my last accessed course, tab and time of click in database *****/
      // WhatToSearch, LastAccNotif remain unchanged
      DB_QueryUPDATE ("can not update last user's data",
		      "UPDATE usr_last"
		        " SET LastSco='%s',"
		             "LastCod=%ld,"
		             "LastAct=%ld,"
		             "LastRole=%u,"
		             "LastTime=NOW()"
                      " WHERE UsrCod=%ld",
		      Sco_GetDBStrFromScope (Gbl.Hierarchy.Level),
		      Gbl.Hierarchy.Cod,
		      Act_GetActCod (Gbl.Action.Act),
		      (unsigned) Gbl.Usrs.Me.Role.Logged,
		      Gbl.Usrs.Me.UsrDat.UsrCod);
   else
      Usr_InsertMyLastData ();
  }

/*****************************************************************************/
/*************** Create new entry for my last data in database ***************/
/*****************************************************************************/

static void Usr_InsertMyLastData (void)
  {
   /***** Insert my last accessed course, tab and time of click in database *****/
   DB_QueryINSERT ("can not insert last user's data",
		   "INSERT INTO usr_last"
	           " (UsrCod,WhatToSearch,"
	             "LastSco,LastCod,LastAct,LastRole,LastTime,LastAccNotif)"
                   " VALUES"
                   " (%ld,%u,"
                     "'%s',%ld,%ld,%u,NOW(),FROM_UNIXTIME(%ld))",
		   Gbl.Usrs.Me.UsrDat.UsrCod,
		   (unsigned) Sch_SEARCH_ALL,
		   Sco_GetDBStrFromScope (Gbl.Hierarchy.Level),
		   Gbl.Hierarchy.Cod,
		   Act_GetActCod (Gbl.Action.Act),
		   (unsigned) Gbl.Usrs.Me.Role.Logged,
		   (long) (time_t) 0);	// The user never accessed to notifications
  }

/*****************************************************************************/
/*************************** Remove user's last data *************************/
/*****************************************************************************/

void Usr_DB_RemoveUsrLastData (long UsrCod)
  {
   DB_QueryDELETE ("can not remove user's last data",
		   "DELETE FROM usr_last"
		   " WHERE UsrCod=%ld",
		   UsrCod);
  }

/*****************************************************************************/
/****************************** Remove user's data ***************************/
/*****************************************************************************/

void Usr_DB_RemoveUsrData (long UsrCod)
  {
   /***** Remove user's data *****/
   DB_QueryDELETE ("can not remove user's data",
		   "DELETE FROM usr_data"
		   " WHERE UsrCod=%ld",
		   UsrCod);
  }

/*****************************************************************************/
/*********** Write a row of a table with the main data of a user *************/
/*****************************************************************************/

#define Usr_MAX_BYTES_BG_COLOR (16 - 1)

void Usr_WriteRowUsrMainData (unsigned NumUsr,struct UsrData *UsrDat,
                              bool PutCheckBoxToSelectUsr,Rol_Role_t Role,
			      struct SelectedUsrs *SelectedUsrs)
  {
   extern const char *Txt_Enrolment_confirmed;
   extern const char *Txt_Enrolment_not_confirmed;
   char BgColor[Usr_MAX_BYTES_BG_COLOR + 1];
   bool UsrIsTheMsgSender = PutCheckBoxToSelectUsr &&
	                    (UsrDat->UsrCod == Gbl.Usrs.Other.UsrDat.UsrCod);
   struct Ins_Instit Ins;

   /***** Begin row *****/
   HTM_TR_Begin (NULL);

      /***** Checkbox to select user *****/
      // Two colors are used alternatively to better distinguish the rows
      if (UsrIsTheMsgSender)
	 Str_Copy (BgColor,"LIGHT_GREEN",sizeof (BgColor) - 1);
      else
	 snprintf (BgColor,sizeof (BgColor),"COLOR%u",Gbl.RowEvenOdd);

      if (PutCheckBoxToSelectUsr)
	{
	 HTM_TD_Begin ("class=\"CM %s\"",BgColor);
	    Usr_PutCheckboxToSelectUser (Role,UsrDat->EnUsrCod,UsrIsTheMsgSender,
					 SelectedUsrs);
	 HTM_TD_End ();
	}

      /***** User has accepted enrolment? *****/
      if (UsrIsTheMsgSender)
	 HTM_TD_Begin ("class=\"BM_SEL %s\" title=\"%s\"",
		       UsrDat->Accepted ? "USR_LIST_NUM_N" :
					  "USR_LIST_NUM",
		       UsrDat->Accepted ? Txt_Enrolment_confirmed :
					  Txt_Enrolment_not_confirmed);
      else
	 HTM_TD_Begin ("class=\"BM%u %s\" title=\"%s\"",
		       Gbl.RowEvenOdd,
		       UsrDat->Accepted ? "USR_LIST_NUM_N" :
					  "USR_LIST_NUM",
		       UsrDat->Accepted ? Txt_Enrolment_confirmed :
					  Txt_Enrolment_not_confirmed);
      HTM_Txt (UsrDat->Accepted ? "&check;" :
				  "&cross;");
      HTM_TD_End ();

      /***** Write number of user in the list *****/
      HTM_TD_Begin ("class=\"%s RM %s\"",
		    UsrDat->Accepted ? "USR_LIST_NUM_N" :
				       "USR_LIST_NUM",
		    BgColor);
	 HTM_Unsigned (NumUsr);
      HTM_TD_End ();

      if (Gbl.Usrs.Listing.WithPhotos)
	{
	 /***** Show user's photo *****/
	 HTM_TD_Begin ("class=\"CM %s\"",BgColor);
	    Pho_ShowUsrPhotoIfAllowed (UsrDat,"PHOTO21x28",Pho_ZOOM,false);
	 HTM_TD_End ();
	}

      /****** Write user's IDs ******/
      HTM_TD_Begin ("class=\"%s LM %s\"",
		    UsrDat->Accepted ? "DAT_SMALL_N" :
				       "DAT_SMALL",
		    BgColor);
	 ID_WriteUsrIDs (UsrDat,NULL);
      HTM_TD_End ();

      /***** Write rest of main user's data *****/
      Ins.InsCod = UsrDat->InsCod;
      Ins_GetDataOfInstitutionByCod (&Ins);
      Usr_WriteMainUsrDataExceptUsrID (UsrDat,BgColor);

      HTM_TD_Begin ("class=\"LM %s\"",BgColor);
	 Ins_DrawInstitutionLogoWithLink (&Ins,25);
      HTM_TD_End ();

   /***** End row *****/
   HTM_TR_End ();
  }

/*****************************************************************************/
/*************** Write a row of a table with the data of a guest *************/
/*****************************************************************************/

static void Usr_WriteRowGstAllData (struct UsrData *UsrDat)
  {
   struct Ins_Instit Ins;
   struct Ctr_Center Ctr;
   struct Dpt_Department Dpt;

   /***** Begin row *****/
   HTM_TR_Begin (NULL);

      if (Gbl.Usrs.Listing.WithPhotos)
	{
	 /***** Show guest's photo *****/
	 HTM_TD_Begin ("class=\"LM COLOR%u\"",Gbl.RowEvenOdd);
	    Pho_ShowUsrPhotoIfAllowed (UsrDat,"PHOTO21x28",Pho_NO_ZOOM,false);
	 HTM_TD_End ();
	}

      /****** Write user's ID ******/
      HTM_TD_Begin ("class=\"DAT_SMALL LM COLOR%u\"",Gbl.RowEvenOdd);
	 ID_WriteUsrIDs (UsrDat,NULL);
	 HTM_NBSP ();
      HTM_TD_End ();

      /***** Write rest of guest's main data *****/
      Ins.InsCod = UsrDat->InsCod;
      Ins_GetDataOfInstitutionByCod (&Ins);
      Usr_WriteMainUsrDataExceptUsrID (UsrDat,Gbl.ColorRows[Gbl.RowEvenOdd]);
      Usr_WriteEmail (UsrDat,Gbl.ColorRows[Gbl.RowEvenOdd]);
      Usr_WriteUsrData (Gbl.ColorRows[Gbl.RowEvenOdd],
			Ins.FullName,
			NULL,true,false);

      /***** Write the rest of the data of the guest *****/
      if (UsrDat->Tch.CtrCod > 0)
	{
	 Ctr.CtrCod = UsrDat->Tch.CtrCod;
	 Ctr_GetDataOfCenterByCod (&Ctr);
	}
      Usr_WriteUsrData (Gbl.ColorRows[Gbl.RowEvenOdd],
			UsrDat->Tch.CtrCod > 0 ? Ctr.FullName :
						 "&nbsp;",
			NULL,true,false);
      if (UsrDat->Tch.DptCod > 0)
	{
	 Dpt.DptCod = UsrDat->Tch.DptCod;
	 Dpt_GetDataOfDepartmentByCod (&Dpt);
	}
      Usr_WriteUsrData (Gbl.ColorRows[Gbl.RowEvenOdd],
			UsrDat->Tch.DptCod > 0 ? Dpt.FullName :
						 "&nbsp;",
			NULL,true,false);
      Usr_WriteUsrData (Gbl.ColorRows[Gbl.RowEvenOdd],
			UsrDat->Tch.Office[0] ? UsrDat->Tch.Office :
						"&nbsp;",
			NULL,true,false);
      Usr_WriteUsrData (Gbl.ColorRows[Gbl.RowEvenOdd],
			UsrDat->Tch.OfficePhone[0] ? UsrDat->Tch.OfficePhone :
						     "&nbsp;",
			NULL,true,false);
      Usr_WriteUsrData (Gbl.ColorRows[Gbl.RowEvenOdd],
			UsrDat->Phone[0][0] ? UsrDat->Phone[0] :
					      "&nbsp;",
			NULL,true,false);
      Usr_WriteUsrData (Gbl.ColorRows[Gbl.RowEvenOdd],
			UsrDat->Phone[1][0] ? UsrDat->Phone[1] :
					      "&nbsp;",
			NULL,true,false);
      Usr_WriteUsrData (Gbl.ColorRows[Gbl.RowEvenOdd],
			UsrDat->StrBirthday[0] ? UsrDat->StrBirthday :
						 "&nbsp;",
			NULL,true,false);

   /***** End row *****/
   HTM_TR_End ();
  }

/*****************************************************************************/
/************ Write a row of a table with the data of a student **************/
/*****************************************************************************/

static void Usr_WriteRowStdAllData (struct UsrData *UsrDat,char *GroupNames)
  {
   unsigned NumGrpTyp,NumField;
   MYSQL_RES *mysql_res;
   MYSQL_ROW row;
   char Text[Cns_MAX_BYTES_TEXT + 1];
   struct Ins_Instit Ins;
   bool ShowData = (Gbl.Usrs.Me.Role.Logged == Rol_TCH && UsrDat->Accepted) ||
                    Gbl.Usrs.Me.Role.Logged >= Rol_DEG_ADM;

   /***** Begin row *****/
   HTM_TR_Begin (NULL);

      if (Gbl.Usrs.Listing.WithPhotos)
	{
	 /***** Show student's photo *****/
	 HTM_TD_Begin ("class=\"LM COLOR%u\"",Gbl.RowEvenOdd);
	    Pho_ShowUsrPhotoIfAllowed (UsrDat,"PHOTO21x28",Pho_NO_ZOOM,false);
	 HTM_TD_End ();
	}

      /****** Write user's ID ******/
      HTM_TD_Begin ("class=\"%s LM COLOR%u\"",
		    UsrDat->Accepted ? "DAT_SMALL_N" :
				       "DAT_SMALL",
		    Gbl.RowEvenOdd);
	 ID_WriteUsrIDs (UsrDat,NULL);
	 HTM_NBSP ();
      HTM_TD_End ();

      /***** Write rest of main student's data *****/
      Ins.InsCod = UsrDat->InsCod;
      Ins_GetDataOfInstitutionByCod (&Ins);
      Usr_WriteMainUsrDataExceptUsrID (UsrDat,Gbl.ColorRows[Gbl.RowEvenOdd]);
      Usr_WriteEmail (UsrDat,Gbl.ColorRows[Gbl.RowEvenOdd]);
      Usr_WriteUsrData (Gbl.ColorRows[Gbl.RowEvenOdd],
			Ins.FullName,
			NULL,true,UsrDat->Accepted);

      /***** Write the rest of the data of the student *****/
      Usr_WriteUsrData (Gbl.ColorRows[Gbl.RowEvenOdd],
			UsrDat->Phone[0][0] ? (ShowData ? UsrDat->Phone[0] :
							  "********") :
					      "&nbsp;",
			NULL,true,UsrDat->Accepted);
      Usr_WriteUsrData (Gbl.ColorRows[Gbl.RowEvenOdd],
			UsrDat->Phone[1][0] ? (ShowData ? UsrDat->Phone[1] :
							  "********") :
					      "&nbsp;",
			NULL,true,UsrDat->Accepted);
      Usr_WriteUsrData (Gbl.ColorRows[Gbl.RowEvenOdd],
			UsrDat->StrBirthday[0] ? (ShowData ? UsrDat->StrBirthday :
							     "********") :
						 "&nbsp;",
			NULL,true,UsrDat->Accepted);

      if (Gbl.Scope.Current == HieLvl_CRS)
	{
	 /***** Write the groups a the que pertenece the student *****/
	 for (NumGrpTyp = 0;
	      NumGrpTyp < Gbl.Crs.Grps.GrpTypes.NumGrpTypes;
	      NumGrpTyp++)
	    if (Gbl.Crs.Grps.GrpTypes.LstGrpTypes[NumGrpTyp].NumGrps)         // If current course tiene groups of este type
	      {
	       Grp_GetNamesGrpsUsrBelongsTo (UsrDat->UsrCod,
					     Gbl.Crs.Grps.GrpTypes.LstGrpTypes[NumGrpTyp].GrpTypCod,
					     GroupNames);
	       Usr_WriteUsrData (Gbl.ColorRows[Gbl.RowEvenOdd],GroupNames,NULL,true,UsrDat->Accepted);
	      }

	 /***** Fields of the record dependientes of the course *****/
	 for (NumField = 0;
	      NumField < Gbl.Crs.Records.LstFields.Num;
	      NumField++)
	   {
	    /* Get the text of the field */
	    if (Rec_DB_GetFieldFromCrsRecord (&mysql_res,UsrDat->UsrCod,
					      Gbl.Crs.Records.LstFields.Lst[NumField].FieldCod))
	      {
	       row = mysql_fetch_row (mysql_res);
	       Str_Copy (Text,row[0],sizeof (Text) - 1);
	       Str_ChangeFormat (Str_FROM_HTML,Str_TO_RIGOROUS_HTML,
				 Text,Cns_MAX_BYTES_TEXT,false);        // Se convierte of HTML a HTML respetuoso
	      }
	    else
	       Text[0] = '\0';
	    Usr_WriteUsrData (Gbl.ColorRows[Gbl.RowEvenOdd],Text,NULL,false,UsrDat->Accepted);

	    /* Free structure that stores the query result */
	    DB_FreeMySQLResult (&mysql_res);
	   }
	}

   /***** End row *****/
   HTM_TR_End ();
  }

/*****************************************************************************/
/*** Write a row of a table with the data of a teacher or an administrator ***/
/*****************************************************************************/

static void Usr_WriteRowTchAllData (struct UsrData *UsrDat)
  {
   struct Ins_Instit Ins;
   struct Ctr_Center Ctr;
   struct Dpt_Department Dpt;
   bool ItsMe = Usr_ItsMe (UsrDat->UsrCod);
   bool ShowData = (ItsMe || UsrDat->Accepted ||
                    Gbl.Usrs.Me.Role.Logged == Rol_DEG_ADM ||
                    Gbl.Usrs.Me.Role.Logged == Rol_SYS_ADM);

   /***** Begin row *****/
   HTM_TR_Begin (NULL);
      if (Gbl.Usrs.Listing.WithPhotos)
	{
	 /***** Show teacher's photo *****/
	 HTM_TD_Begin ("class=\"LM COLOR%u\"",Gbl.RowEvenOdd);
	    Pho_ShowUsrPhotoIfAllowed (UsrDat,"PHOTO21x28",Pho_NO_ZOOM,false);
	 HTM_TD_End ();
	}

      /****** Write the user's ID ******/
      HTM_TD_Begin ("class=\"%s LM COLOR%u\"",
		    UsrDat->Accepted ? "DAT_SMALL_N" :
				       "DAT_SMALL",
		    Gbl.RowEvenOdd);
	 ID_WriteUsrIDs (UsrDat,NULL);
	 HTM_NBSP ();
      HTM_TD_End ();

      /***** Write rest of main teacher's data *****/
      Ins.InsCod = UsrDat->InsCod;
      Ins_GetDataOfInstitutionByCod (&Ins);
      Usr_WriteMainUsrDataExceptUsrID (UsrDat,Gbl.ColorRows[Gbl.RowEvenOdd]);
      Usr_WriteEmail (UsrDat,Gbl.ColorRows[Gbl.RowEvenOdd]);
      Usr_WriteUsrData (Gbl.ColorRows[Gbl.RowEvenOdd],
			Ins.FullName,
			NULL,true,UsrDat->Accepted);

      /***** Write the rest of teacher's data *****/
      if (ShowData && UsrDat->Tch.CtrCod > 0)
	{
	 Ctr.CtrCod = UsrDat->Tch.CtrCod;
	 Ctr_GetDataOfCenterByCod (&Ctr);
	}
      Usr_WriteUsrData (Gbl.ColorRows[Gbl.RowEvenOdd],
			(ShowData && UsrDat->Tch.CtrCod > 0) ? Ctr.FullName :
							       "&nbsp;",
			NULL,true,UsrDat->Accepted);
      if (ShowData && UsrDat->Tch.DptCod > 0)
	{
	 Dpt.DptCod = UsrDat->Tch.DptCod;
	 Dpt_GetDataOfDepartmentByCod (&Dpt);
	}
      Usr_WriteUsrData (Gbl.ColorRows[Gbl.RowEvenOdd],
			(ShowData && UsrDat->Tch.DptCod > 0) ? Dpt.FullName :
							       "&nbsp;",
			NULL,true,UsrDat->Accepted);
      Usr_WriteUsrData (Gbl.ColorRows[Gbl.RowEvenOdd],
			(ShowData && UsrDat->Tch.Office[0]) ? UsrDat->Tch.Office :
							      "&nbsp;",
			NULL,true,UsrDat->Accepted);
      Usr_WriteUsrData (Gbl.ColorRows[Gbl.RowEvenOdd],
			(ShowData && UsrDat->Tch.OfficePhone[0]) ? UsrDat->Tch.OfficePhone :
								   "&nbsp;",
			NULL,true,UsrDat->Accepted);

   HTM_TR_End ();
  }

/*****************************************************************************/
/********** Write a row of a table with the data of an administrator *********/
/*****************************************************************************/

static void Usr_WriteRowAdmData (unsigned NumUsr,struct UsrData *UsrDat)
  {
   struct Ins_Instit Ins;

   /***** Begin row *****/
   HTM_TR_Begin (NULL);

      /***** Write number of user *****/
      HTM_TD_Begin ("class=\"USR_LIST_NUM_N CM COLOR%u\"",Gbl.RowEvenOdd);
	 HTM_Unsigned (NumUsr);
      HTM_TD_End ();

      if (Gbl.Usrs.Listing.WithPhotos)
	{
	 /***** Show administrator's photo *****/
	 HTM_TD_Begin ("class=\"LM COLOR%u\"",Gbl.RowEvenOdd);
	    Pho_ShowUsrPhotoIfAllowed (UsrDat,"PHOTO21x28",Pho_ZOOM,false);
	 HTM_TD_End ();
	}

      /****** Write the user's ID ******/
      HTM_TD_Begin ("class=\"%s LM COLOR%u\"",
		    UsrDat->Accepted ? "DAT_SMALL_N" :
				       "DAT_SMALL",
		    Gbl.RowEvenOdd);
	 ID_WriteUsrIDs (UsrDat,NULL);
	 HTM_NBSP ();
      HTM_TD_End ();

      /***** Write rest of main administrator's data *****/
      Ins.InsCod = UsrDat->InsCod;
      Ins_GetDataOfInstitutionByCod (&Ins);
      Usr_WriteMainUsrDataExceptUsrID (UsrDat,Gbl.ColorRows[Gbl.RowEvenOdd]);

      HTM_TD_Begin ("class=\"LM %s\"",Gbl.ColorRows[Gbl.RowEvenOdd]);
	 Ins_DrawInstitutionLogoWithLink (&Ins,25);
      HTM_TD_End ();

   HTM_TR_End ();

   /***** Write degrees which are administrated by this administrator *****/
   Hie_GetAndWriteInsCtrDegAdminBy (UsrDat->UsrCod,
                                    Gbl.Usrs.Listing.WithPhotos ? Usr_NUM_MAIN_FIELDS_DATA_ADM :
                                	                          Usr_NUM_MAIN_FIELDS_DATA_ADM-1);
  }

/*****************************************************************************/
/************************* Write main data of a user *************************/
/*****************************************************************************/

static void Usr_WriteMainUsrDataExceptUsrID (struct UsrData *UsrDat,
                                             const char *BgColor)
  {
   Usr_WriteUsrData (BgColor,
                     UsrDat->Surname1[0] ? UsrDat->Surname1 :
                	                   "&nbsp;",
                     NULL,true,UsrDat->Accepted);
   Usr_WriteUsrData (BgColor,
                     UsrDat->Surname2[0] ? UsrDat->Surname2 :
                	                   "&nbsp;",
                     NULL,true,UsrDat->Accepted);
   Usr_WriteUsrData (BgColor,
                     UsrDat->FrstName[0] ? UsrDat->FrstName :
                	                    "&nbsp;",
                     NULL,true,UsrDat->Accepted);
  }

/*****************************************************************************/
/**************************** Write user's email *****************************/
/*****************************************************************************/

static void Usr_WriteEmail (struct UsrData *UsrDat,const char *BgColor)
  {
   bool ShowEmail;
   char MailLink[7 + Cns_MAX_BYTES_EMAIL_ADDRESS + 1];	// mailto:mail_address

   if (UsrDat->Email[0])
     {
      ShowEmail = Mai_ICanSeeOtherUsrEmail (UsrDat);
      if (ShowEmail)
         snprintf (MailLink,sizeof (MailLink),"mailto:%s",UsrDat->Email);
     }
   else
      ShowEmail = false;
   Usr_WriteUsrData (BgColor,
                     UsrDat->Email[0] ? (ShowEmail ? UsrDat->Email :
                	                             "********") :
                	                "&nbsp;",
                     ShowEmail ? MailLink :
                	         NULL,
                     true,UsrDat->Accepted);
  }

/*****************************************************************************/
/********************* Write a cell with data of a user **********************/
/*****************************************************************************/

static void Usr_WriteUsrData (const char *BgColor,
                              const char *Data,const char *Link,
                              bool NonBreak,bool Accepted)
  {
   /***** Begin table cell *****/
   HTM_TD_Begin ("class=\"%s LM %s\"",
		 Accepted ? (NonBreak ? "DAT_SMALL_NOBR_N" :
				        "DAT_SMALL_N") :
			    (NonBreak ? "DAT_SMALL_NOBR" :
				        "DAT_SMALL"),
		 BgColor);

      /***** Container to limit length *****/
      HTM_DIV_Begin ("class=\"USR_DAT\"");

	 /***** Begin link *****/
	 if (Link)
	    HTM_A_Begin ("href=\"%s\" class=\"%s\" target=\"_blank\"",
			 Link,Accepted ? "DAT_SMALL_NOBR_N" :
					 "DAT_SMALL_NOBR");

	 /***** Write data *****/
	 HTM_Txt (Data);
	 if (NonBreak)
	    HTM_NBSP ();

	 /***** End link *****/
	 if (Link)
	    HTM_A_End ();

      /***** End container *****/
      HTM_DIV_End ();

   /***** End table cell *****/
   HTM_TD_End ();
  }

/*****************************************************************************/
/******** Get the user's code of a random student from current course ********/
/*****************************************************************************/
// Returns user's code or -1 if no user found

long Usr_GetRamdomStdFromCrs (long CrsCod)
  {
   /***** Get a random student from current course from database *****/
   return DB_QuerySELECTCode ("can not get a random student from a course",
			      "SELECT UsrCod"
			       " FROM crs_users"
			      " WHERE CrsCod=%ld"
			        " AND Role=%u"
			      " ORDER BY RAND()"
			      " LIMIT 1",
			      CrsCod,
			      (unsigned) Rol_STD);
  }

/*****************************************************************************/
/*********** Get the user's code of a random student from a group ************/
/*****************************************************************************/
// Returns user's code or -1 if no user found

long Usr_GetRamdomStdFromGrp (long GrpCod)
  {
   /***** Get a random student from a group from database *****/
   return DB_QuerySELECTCode ("can not get a random student from a group",
			      "SELECT grp_users.UsrCod"
			       " FROM grp_users,"
				     "crs_users"
			      " WHERE grp_users.GrpCod=%ld"
			        " AND grp_users.UsrCod=crs_users.UsrCod"
			        " AND crs_users.Role=%u"
			      " ORDER BY RAND()"
			      " LIMIT 1",
			      GrpCod,
			      (unsigned) Rol_STD);
  }

/*****************************************************************************/
/* Get number of teachers in courses of the current instit. in a department **/
/*****************************************************************************/

unsigned Usr_GetNumTchsCurrentInsInDepartment (long DptCod)
  {
   /***** Get the number of teachers
          from the current institution in a department *****/
   return (unsigned)
   DB_QueryCOUNT ("can not get the number of teachers in a department",
		  "SELECT COUNT(DISTINCT usr_data.UsrCod)"
		   " FROM usr_data,"
		         "crs_users,"
		         "crs_courses,"
		         "deg_degrees,"
		         "ctr_centers"
		  " WHERE usr_data.InsCod=%ld"				// User in the current institution...
		    " AND usr_data.DptCod=%ld"				// ...and the specified department...
		    " AND usr_data.UsrCod=crs_users.UsrCod"		// ...who is...
		    " AND crs_users.Role IN (%u,%u)"			// ...a teacher...
		    " AND crs_users.CrsCod=crs_courses.CrsCod"		// ...in a course...
		    " AND crs_courses.DegCod=deg_degrees.DegCod"	// ...of a degree...
		    " AND deg_degrees.CtrCod=ctr_centers.InsCod"	// ...of a center...
		    " AND ctr_centers.InsCod=%ld",			// ...of the current institution
		  Gbl.Hierarchy.Ins.InsCod,DptCod,
		  (unsigned) Rol_NET,(unsigned) Rol_TCH,
		  Gbl.Hierarchy.Ins.InsCod);
  }

/*****************************************************************************/
/******* Get number of users who don't claim to belong to any country ********/
/*****************************************************************************/

void Usr_FlushCacheNumUsrsWhoDontClaimToBelongToAnyCty (void)
  {
   Gbl.Cache.NumUsrsWhoDontClaimToBelongToAnyCty.Valid = false;
  }

unsigned Usr_GetNumUsrsWhoDontClaimToBelongToAnyCty (void)
  {
   /***** 1. Fast check: If cached... *****/
   if (Gbl.Cache.NumUsrsWhoDontClaimToBelongToAnyCty.Valid)
      return Gbl.Cache.NumUsrsWhoDontClaimToBelongToAnyCty.NumUsrs;

   /***** 2. Slow: number of users who don't claim to belong to any country
                   from database *****/
   Gbl.Cache.NumUsrsWhoDontClaimToBelongToAnyCty.NumUsrs = (unsigned)
   DB_QueryCOUNT ("can not get number of users",
		  "SELECT COUNT(UsrCod)"
		   " FROM usr_data"
		  " WHERE CtyCod<0");
   Gbl.Cache.NumUsrsWhoDontClaimToBelongToAnyCty.Valid = true;
   FigCch_UpdateFigureIntoCache (FigCch_NUM_USRS_BELONG_CTY,HieLvl_CTY,-1L,
				 FigCch_UNSIGNED,&Gbl.Cache.NumUsrsWhoDontClaimToBelongToAnyCty.NumUsrs);
   return Gbl.Cache.NumUsrsWhoDontClaimToBelongToAnyCty.NumUsrs;
  }

unsigned Usr_GetCachedNumUsrsWhoDontClaimToBelongToAnyCty (void)
  {
   unsigned NumUsrs;

   /***** Get number of user who don't claim to belong to any country from cache *****/
   if (!FigCch_GetFigureFromCache (FigCch_NUM_USRS_BELONG_CTY,HieLvl_CTY,-1L,
				   FigCch_UNSIGNED,&NumUsrs))
      /***** Get current number of user who don't claim to belong to any country from database and update cache *****/
      NumUsrs = Usr_GetNumUsrsWhoDontClaimToBelongToAnyCty ();

   return NumUsrs;
  }

/*****************************************************************************/
/******** Get number of users who claim to belong to another country *********/
/*****************************************************************************/

void Usr_FlushCacheNumUsrsWhoClaimToBelongToAnotherCty (void)
  {
   Gbl.Cache.NumUsrsWhoClaimToBelongToAnotherCty.Valid = false;
  }

unsigned Usr_GetNumUsrsWhoClaimToBelongToAnotherCty (void)
  {
   /***** 1. Fast check: If cached... *****/
   if (Gbl.Cache.NumUsrsWhoClaimToBelongToAnotherCty.Valid)
      return Gbl.Cache.NumUsrsWhoClaimToBelongToAnotherCty.NumUsrs;

   /***** 2. Slow: number of users who claim to belong to another country
                   from database *****/
   Gbl.Cache.NumUsrsWhoClaimToBelongToAnotherCty.NumUsrs = (unsigned)
   DB_QueryCOUNT ("can not get number of users",
		  "SELECT COUNT(UsrCod)"
		   " FROM usr_data"
		  " WHERE CtyCod=0");
   Gbl.Cache.NumUsrsWhoClaimToBelongToAnotherCty.Valid = true;
   FigCch_UpdateFigureIntoCache (FigCch_NUM_USRS_BELONG_CTY,HieLvl_CTY,0,
				 FigCch_UNSIGNED,&Gbl.Cache.NumUsrsWhoClaimToBelongToAnotherCty.NumUsrs);
   return Gbl.Cache.NumUsrsWhoClaimToBelongToAnotherCty.NumUsrs;
  }

unsigned Usr_GetCachedNumUsrsWhoClaimToBelongToAnotherCty (void)
  {
   unsigned NumUsrsCty;

   /***** Get number of users who claim to belong to another country form cache *****/
   if (!FigCch_GetFigureFromCache (FigCch_NUM_USRS_BELONG_CTY,HieLvl_CTY,0,
                                   FigCch_UNSIGNED,&NumUsrsCty))
      /***** Get current number of users who claim to belong to another country from database and update cache *****/
      NumUsrsCty = Usr_GetNumUsrsWhoClaimToBelongToAnotherCty ();

   return NumUsrsCty;
  }

/*****************************************************************************/
/*********** Get number of users who claim to belong to a country ************/
/*****************************************************************************/

void Usr_FlushCacheNumUsrsWhoClaimToBelongToCty (void)
  {
   Gbl.Cache.NumUsrsWhoClaimToBelongToCty.CtyCod  = -1L;
   Gbl.Cache.NumUsrsWhoClaimToBelongToCty.NumUsrs = 0;
  }

unsigned Usr_GetNumUsrsWhoClaimToBelongToCty (struct Cty_Countr *Cty)
  {
   /***** 1. Fast check: Trivial case *****/
   if (Cty->CtyCod <= 0)
      return 0;

   /***** 2. Fast check: If already got... *****/
   if (Cty->NumUsrsWhoClaimToBelongToCty.Valid)
      return Cty->NumUsrsWhoClaimToBelongToCty.NumUsrs;

   /***** 3. Fast check: If cached... *****/
   if (Cty->CtyCod == Gbl.Cache.NumUsrsWhoClaimToBelongToCty.CtyCod)
     {
      Cty->NumUsrsWhoClaimToBelongToCty.NumUsrs = Gbl.Cache.NumUsrsWhoClaimToBelongToCty.NumUsrs;
      Cty->NumUsrsWhoClaimToBelongToCty.Valid = true;
      return Cty->NumUsrsWhoClaimToBelongToCty.NumUsrs;
     }

   /***** 4. Slow: number of users who claim to belong to an institution
                   from database *****/
   Gbl.Cache.NumUsrsWhoClaimToBelongToCty.CtyCod  = Cty->CtyCod;
   Gbl.Cache.NumUsrsWhoClaimToBelongToCty.NumUsrs =
   Cty->NumUsrsWhoClaimToBelongToCty.NumUsrs = (unsigned)
   DB_QueryCOUNT ("can not get number of users",
		  "SELECT COUNT(UsrCod)"
		   " FROM usr_data"
		  " WHERE CtyCod=%ld",
		  Cty->CtyCod);
   Cty->NumUsrsWhoClaimToBelongToCty.Valid = true;
   FigCch_UpdateFigureIntoCache (FigCch_NUM_USRS_BELONG_CTY,HieLvl_CTY,Gbl.Cache.NumUsrsWhoClaimToBelongToCty.CtyCod,
				 FigCch_UNSIGNED,&Gbl.Cache.NumUsrsWhoClaimToBelongToCty.NumUsrs);
   return Cty->NumUsrsWhoClaimToBelongToCty.NumUsrs;
  }

unsigned Usr_GetCachedNumUsrsWhoClaimToBelongToCty (struct Cty_Countr *Cty)
  {
   unsigned NumUsrsCty;

   /***** Get number of users who claim to belong to country from cache ******/
   if (!FigCch_GetFigureFromCache (FigCch_NUM_USRS_BELONG_CTY,HieLvl_CTY,Cty->CtyCod,
                                   FigCch_UNSIGNED,&NumUsrsCty))
      /***** Get current number of users who claim to belong to country from database and update cache ******/
      NumUsrsCty = Usr_GetNumUsrsWhoClaimToBelongToCty (Cty);

   return NumUsrsCty;
  }

/*****************************************************************************/
/******** Get number of users who claim to belong to an institution **********/
/*****************************************************************************/

void Usr_FlushCacheNumUsrsWhoClaimToBelongToIns (void)
  {
   Gbl.Cache.NumUsrsWhoClaimToBelongToIns.InsCod  = -1L;
   Gbl.Cache.NumUsrsWhoClaimToBelongToIns.NumUsrs = 0;
  }

unsigned Usr_GetNumUsrsWhoClaimToBelongToIns (struct Ins_Instit *Ins)
  {
   /***** 1. Fast check: Trivial case *****/
   if (Ins->InsCod <= 0)
      return 0;

   /***** 2. Fast check: If already got... *****/
   if (Ins->NumUsrsWhoClaimToBelongToIns.Valid)
      return Ins->NumUsrsWhoClaimToBelongToIns.NumUsrs;

   /***** 3. Fast check: If cached... *****/
   if (Ins->InsCod == Gbl.Cache.NumUsrsWhoClaimToBelongToIns.InsCod)
     {
      Ins->NumUsrsWhoClaimToBelongToIns.NumUsrs = Gbl.Cache.NumUsrsWhoClaimToBelongToIns.NumUsrs;
      Ins->NumUsrsWhoClaimToBelongToIns.Valid = true;
      return Ins->NumUsrsWhoClaimToBelongToIns.NumUsrs;
     }

   /***** 4. Slow: number of users who claim to belong to an institution
                   from database *****/
   Gbl.Cache.NumUsrsWhoClaimToBelongToIns.InsCod  = Ins->InsCod;
   Gbl.Cache.NumUsrsWhoClaimToBelongToIns.NumUsrs =
   Ins->NumUsrsWhoClaimToBelongToIns.NumUsrs = (unsigned)
   DB_QueryCOUNT ("can not get number of users",
		  "SELECT COUNT(UsrCod)"
		   " FROM usr_data"
		  " WHERE InsCod=%ld",
		  Ins->InsCod);
   Ins->NumUsrsWhoClaimToBelongToIns.Valid = true;
   FigCch_UpdateFigureIntoCache (FigCch_NUM_USRS_BELONG_INS,HieLvl_INS,Gbl.Cache.NumUsrsWhoClaimToBelongToIns.InsCod,
				 FigCch_UNSIGNED,&Gbl.Cache.NumUsrsWhoClaimToBelongToIns.NumUsrs);
   return Ins->NumUsrsWhoClaimToBelongToIns.NumUsrs;
  }

unsigned Usr_GetCachedNumUsrsWhoClaimToBelongToIns (struct Ins_Instit *Ins)
  {
   unsigned NumUsrsIns;

   /***** Get number of users who claim to belong to institution from cache *****/
   if (!FigCch_GetFigureFromCache (FigCch_NUM_USRS_BELONG_INS,HieLvl_INS,Ins->InsCod,
                                   FigCch_UNSIGNED,&NumUsrsIns))
      /***** Get current number of users who claim to belong to institution from database and update cache *****/
      NumUsrsIns = Usr_GetNumUsrsWhoClaimToBelongToIns (Ins);

   return NumUsrsIns;
  }

/*****************************************************************************/
/*********** Get number of users who claim to belong to a center *************/
/*****************************************************************************/

void Usr_FlushCacheNumUsrsWhoClaimToBelongToCtr (void)
  {
   Gbl.Cache.NumUsrsWhoClaimToBelongToCtr.CtrCod  = -1L;
   Gbl.Cache.NumUsrsWhoClaimToBelongToCtr.NumUsrs = 0;
  }

unsigned Usr_GetNumUsrsWhoClaimToBelongToCtr (struct Ctr_Center *Ctr)
  {
   /***** 1. Fast check: Trivial case *****/
   if (Ctr->CtrCod <= 0)
      return 0;

   /***** 2. Fast check: If already got... *****/
   if (Ctr->NumUsrsWhoClaimToBelongToCtr.Valid)
      return Ctr->NumUsrsWhoClaimToBelongToCtr.NumUsrs;

   /***** 3. Fast check: If cached... *****/
   if (Ctr->CtrCod == Gbl.Cache.NumUsrsWhoClaimToBelongToCtr.CtrCod)
     {
      Ctr->NumUsrsWhoClaimToBelongToCtr.NumUsrs = Gbl.Cache.NumUsrsWhoClaimToBelongToCtr.NumUsrs;
      Ctr->NumUsrsWhoClaimToBelongToCtr.Valid = true;
      return Ctr->NumUsrsWhoClaimToBelongToCtr.NumUsrs;
     }

   /***** 4. Slow: number of users who claim to belong to a center
                   from database *****/
   Gbl.Cache.NumUsrsWhoClaimToBelongToCtr.CtrCod  = Ctr->CtrCod;
   Gbl.Cache.NumUsrsWhoClaimToBelongToCtr.NumUsrs =
   Ctr->NumUsrsWhoClaimToBelongToCtr.NumUsrs = (unsigned)
   DB_QueryCOUNT ("can not get number of users",
		  "SELECT COUNT(UsrCod)"
		   " FROM usr_data"
		  " WHERE CtrCod=%ld",
		  Ctr->CtrCod);
   FigCch_UpdateFigureIntoCache (FigCch_NUM_USRS_BELONG_CTR,HieLvl_CTR,Gbl.Cache.NumUsrsWhoClaimToBelongToCtr.CtrCod,
				 FigCch_UNSIGNED,&Gbl.Cache.NumUsrsWhoClaimToBelongToCtr.NumUsrs);
   return Ctr->NumUsrsWhoClaimToBelongToCtr.NumUsrs;
  }

unsigned Usr_GetCachedNumUsrsWhoClaimToBelongToCtr (struct Ctr_Center *Ctr)
  {
   unsigned NumUsrsCtr;

   /***** Get number of users who claim to belong to center from cache *****/
   if (!FigCch_GetFigureFromCache (FigCch_NUM_USRS_BELONG_CTR,HieLvl_CTR,Ctr->CtrCod,
                                   FigCch_UNSIGNED,&NumUsrsCtr))
      /***** Get current number of users who claim to belong to center from database and update cache *****/
      NumUsrsCtr = Usr_GetNumUsrsWhoClaimToBelongToCtr (Ctr);

   return NumUsrsCtr;
  }

/*****************************************************************************/
/******* Build query to get list with data of users in current course ********/
/*****************************************************************************/

static void Usr_BuildQueryToGetUsrsLstCrs (char **Query,Rol_Role_t Role)
  {
   unsigned NumPositiveCods = 0;
   unsigned NumNegativeCods = 0;
   char LongStr[Cns_MAX_DECIMAL_DIGITS_LONG + 1];
   unsigned NumGrpSel;
   long GrpCod;
   unsigned NumGrpTyp;
   bool *AddStdsWithoutGroupOf;
   const char *QueryFields =
      "usr_data.UsrCod,"
      "usr_data.EncryptedUsrCod,"
      "usr_data.Password,"
      "usr_data.Surname1,"
      "usr_data.Surname2,"
      "usr_data.FirstName,"
      "usr_data.Sex,"
      "usr_data.Photo,"
      "usr_data.PhotoVisibility,"
      "usr_data.CtyCod,"
      "usr_data.InsCod,"
      "crs_users.Role,"
      "crs_users.Accepted";
   /*
   row[ 0]: usr_data.UsrCod
   row[ 1]: usr_data.EncryptedUsrCod
   row[ 2]: usr_data.Password (used to check if a teacher can edit user's data)
   row[ 3]: usr_data.Surname1
   row[ 4]: usr_data.Surname2
   row[ 5]: usr_data.FirstName
   row[ 6]: usr_data.Sex
   row[ 7]: usr_data.Photo
   row[ 8]: usr_data.PhotoVisibility
   row[ 9]: usr_data.CtyCod
   row[10]: usr_data.InsCod
   row[11]: crs_users.Role	(only if Scope == HieLvl_CRS)
   row[12]: crs_users.Accepted	(only if Scope == HieLvl_CRS)
   */

   /***** If there are no groups selected, don't do anything *****/
   if (!Gbl.Usrs.ClassPhoto.AllGroups &&
       !Gbl.Crs.Grps.LstGrpsSel.NumGrps)
     {
      *Query = NULL;
      return;
     }

   /***** Allocate space for query *****/
   if ((*Query = malloc (Usr_MAX_BYTES_QUERY_GET_LIST_USRS + 1)) == NULL)
      Err_NotEnoughMemoryExit ();

   /***** Create query for users in the course *****/
   if (Gbl.Action.Act == ActReqMsgUsr)        // Selecting users to write a message
      snprintf (*Query,Usr_MAX_BYTES_QUERY_GET_LIST_USRS + 1,
	        "SELECT %s"
	         " FROM crs_users,"
	               "usr_data"
	        " WHERE crs_users.CrsCod=%ld"
	          " AND crs_users.Role=%u"
	          " AND crs_users.UsrCod NOT IN"
	              " (SELECT ToUsrCod"
	                 " FROM msg_banned"
	                " WHERE FromUsrCod=%ld)"
	          " AND crs_users.UsrCod=usr_data.UsrCod",        // Do not get banned users
      	        QueryFields,
                Gbl.Hierarchy.Crs.CrsCod,
                (unsigned) Role,
                Gbl.Usrs.Me.UsrDat.UsrCod);
   else
      snprintf (*Query,Usr_MAX_BYTES_QUERY_GET_LIST_USRS + 1,
	        "SELECT %s"
	         " FROM crs_users,"
	               "usr_data"
	        " WHERE crs_users.CrsCod=%ld"
	          " AND crs_users.Role=%u"
	          " AND crs_users.UsrCod=usr_data.UsrCod",
	        QueryFields,
                Gbl.Hierarchy.Crs.CrsCod,
                (unsigned) Role);

   /***** Select users in selected groups *****/
   if (!Gbl.Usrs.ClassPhoto.AllGroups)
     {
      /***** Get list of groups types in current course *****/
      Grp_GetListGrpTypesInThisCrs (Grp_ONLY_GROUP_TYPES_WITH_GROUPS);

      /***** Allocate memory for list of booleans AddStdsWithoutGroupOf *****/
      if ((AddStdsWithoutGroupOf = calloc (Gbl.Crs.Grps.GrpTypes.NumGrpTypes,
                                           sizeof (*AddStdsWithoutGroupOf))) == NULL)
         Err_NotEnoughMemoryExit ();

      /***** Initialize vector of booleans that indicates whether it's necessary add to the list
             the students who don't belong to any group of each type *****/
      for (NumGrpTyp = 0;
           NumGrpTyp < Gbl.Crs.Grps.GrpTypes.NumGrpTypes;
           NumGrpTyp++)
         AddStdsWithoutGroupOf[NumGrpTyp] = false;

      /***** Create query with the students who belong to the groups selected *****/
      if (Gbl.Crs.Grps.LstGrpsSel.NumGrps)        // If there are groups selected...
        {
         /* Check if there are positive and negative codes in the list */
         for (NumGrpSel = 0;
              NumGrpSel < Gbl.Crs.Grps.LstGrpsSel.NumGrps;
              NumGrpSel++)
            if ((GrpCod = Gbl.Crs.Grps.LstGrpsSel.GrpCods[NumGrpSel]) > 0)
               NumPositiveCods++;
            else
               for (NumGrpTyp = 0;
                    NumGrpTyp < Gbl.Crs.Grps.GrpTypes.NumGrpTypes;
                    NumGrpTyp++)
                  if (Gbl.Crs.Grps.GrpTypes.LstGrpTypes[NumGrpTyp].GrpTypCod == -GrpCod)
                    {
                     AddStdsWithoutGroupOf[NumGrpTyp] = true;
                     break;
                    }
         /* If there are positive codes, add the students who belong to groups with those codes */
         if (NumPositiveCods)
           {
            Str_Concat (*Query," AND (crs_users.UsrCod IN"
			       " (SELECT DISTINCT UsrCod"
			          " FROM grp_users"
			         " WHERE",
                        Usr_MAX_BYTES_QUERY_GET_LIST_USRS);
            NumPositiveCods = 0;
            for (NumGrpSel = 0;
                 NumGrpSel < Gbl.Crs.Grps.LstGrpsSel.NumGrps;
                 NumGrpSel++)
               if ((GrpCod = Gbl.Crs.Grps.LstGrpsSel.GrpCods[NumGrpSel]) > 0)
                 {
                  Str_Concat (*Query,NumPositiveCods ? " OR GrpCod='" :
                				       " GrpCod='",
                	      Usr_MAX_BYTES_QUERY_GET_LIST_USRS);
                  snprintf (LongStr,sizeof (LongStr),"%ld",GrpCod);
                  Str_Concat (*Query,LongStr,Usr_MAX_BYTES_QUERY_GET_LIST_USRS);
                  Str_Concat (*Query,"'",Usr_MAX_BYTES_QUERY_GET_LIST_USRS);
                  NumPositiveCods++;
                 }
            Str_Concat (*Query,")",Usr_MAX_BYTES_QUERY_GET_LIST_USRS);
           }
        }

      /***** Create a query with the students who don't belong to any group *****/
      for (NumGrpTyp = 0;
           NumGrpTyp < Gbl.Crs.Grps.GrpTypes.NumGrpTypes;
           NumGrpTyp++)
         if (AddStdsWithoutGroupOf[NumGrpTyp])
           {
            if (NumPositiveCods || NumNegativeCods)
               Str_Concat (*Query," OR ",Usr_MAX_BYTES_QUERY_GET_LIST_USRS);
            else
               Str_Concat (*Query," AND (",Usr_MAX_BYTES_QUERY_GET_LIST_USRS);
            /* Select all the students of the course who don't belong to any group of type GrpTypCod */
            Str_Concat (*Query,"crs_users.UsrCod NOT IN"
			       " (SELECT DISTINCT grp_users.UsrCod"
			          " FROM grp_groups,"
			                "grp_users"
			         " WHERE grp_groups.GrpTypCod='",
                        Usr_MAX_BYTES_QUERY_GET_LIST_USRS);
            snprintf (LongStr,sizeof (LongStr),"%ld",
		      Gbl.Crs.Grps.GrpTypes.LstGrpTypes[NumGrpTyp].GrpTypCod);
            Str_Concat (*Query,LongStr,Usr_MAX_BYTES_QUERY_GET_LIST_USRS);
            Str_Concat (*Query,"' AND grp_groups.GrpCod=grp_users.GrpCod)",
                        Usr_MAX_BYTES_QUERY_GET_LIST_USRS);
            NumNegativeCods++;
           }
      if (NumPositiveCods ||
          NumNegativeCods)
         Str_Concat (*Query,")",Usr_MAX_BYTES_QUERY_GET_LIST_USRS);

      /***** Free memory used by the list of booleans AddStdsWithoutGroupOf *****/
      free (AddStdsWithoutGroupOf);

      /***** Free list of groups types in current course *****/
      Grp_FreeListGrpTypesAndGrps ();
     }

   /***** The last part of the query is for ordering the list *****/
   Str_Concat (*Query," ORDER BY "
		      "usr_data.Surname1,"
		      "usr_data.Surname2,"
		      "usr_data.FirstName,"
		      "usr_data.UsrCod",
	       Usr_MAX_BYTES_QUERY_GET_LIST_USRS);
  }

/*****************************************************************************/
/*********** Get list of users with a given role in a given scope ************/
/*****************************************************************************/
// Role can be:
// - Rol_STD	Student
// - Rol_NET	Non-editing teacher
// - Rol_TCH	Teacher

void Usr_GetListUsrs (HieLvl_Level_t Scope,Rol_Role_t Role)
  {
   char *Query = NULL;
   const char *QueryFields =
      "DISTINCT usr_data.UsrCod,"
      "usr_data.EncryptedUsrCod,"
      "usr_data.Password,"
      "usr_data.Surname1,"
      "usr_data.Surname2,"
      "usr_data.FirstName,"
      "usr_data.Sex,"
      "usr_data.Photo,"
      "usr_data.PhotoVisibility,"
      "usr_data.CtyCod,"
      "usr_data.InsCod";
   /*
   row[ 0]: usr_data.UsrCod
   row[ 1]: usr_data.EncryptedUsrCod
   row[ 2]: usr_data.Password
   row[ 3]: usr_data.Surname1
   row[ 4]: usr_data.Surname2
   row[ 5]: usr_data.FirstName
   row[ 6]: usr_data.Sex
   row[ 7]: usr_data.Photo
   row[ 8]: usr_data.PhotoVisibility
   row[ 9]: usr_data.CtyCod
   row[10]: usr_data.InsCod
   row[11]: crs_users.Role	(only if Scope == HieLvl_CRS)
   row[12]: crs_users.Accepted	(only if Scope == HieLvl_CRS)
   */

   /***** Build query *****/
   switch (Scope)
     {
      case HieLvl_SYS:
	 /* Get users in courses from the whole platform */
	 DB_BuildQuery (&Query,
		        "SELECT %s"
		         " FROM usr_data,"
		               "crs_users"
		        " WHERE usr_data.UsrCod=crs_users.UsrCod"
		          " AND crs_users.Role=%u"
		        " ORDER BY usr_data.Surname1,"
		                  "usr_data.Surname2,"
		                  "usr_data.FirstName,"
		                  "usr_data.UsrCod",
		        QueryFields,
		        (unsigned) Role);
	 break;
      case HieLvl_CTY:
	 /* Get users in courses from the current country */
	 DB_BuildQuery (&Query,
			"SELECT %s"
			 " FROM usr_data,"
			       "crs_users,"
			       "crs_courses,"
			       "deg_degrees,"
			       "ctr_centers,"
			       "ins_instits"
			" WHERE usr_data.UsrCod=crs_users.UsrCod"
			  " AND crs_users.Role=%u"
			  " AND crs_users.CrsCod=crs_courses.CrsCod"
			  " AND crs_courses.DegCod=deg_degrees.DegCod"
			  " AND deg_degrees.CtrCod=ctr_centers.CtrCod"
			  " AND ctr_centers.InsCod=ins_instits.InsCod"
			  " AND ins_instits.CtyCod=%ld"
			" ORDER BY usr_data.Surname1,"
			          "usr_data.Surname2,"
			          "usr_data.FirstName,"
			          "usr_data.UsrCod",
			QueryFields,
			(unsigned) Role,
			Gbl.Hierarchy.Cty.CtyCod);
	 break;
      case HieLvl_INS:
	 /* Get users in courses from the current institution */
	 DB_BuildQuery (&Query,
			"SELECT %s"
			 " FROM usr_data,"
			       "crs_users,"
			       "crs_courses,"
			       "deg_degrees,"
			       "ctr_centers"
			" WHERE usr_data.UsrCod=crs_users.UsrCod"
			  " AND crs_users.Role=%u"
			  " AND crs_users.CrsCod=crs_courses.CrsCod"
			  " AND crs_courses.DegCod=deg_degrees.DegCod"
			  " AND deg_degrees.CtrCod=ctr_centers.CtrCod"
			  " AND ctr_centers.InsCod=%ld"
			" ORDER BY usr_data.Surname1,"
			          "usr_data.Surname2,"
			          "usr_data.FirstName,"
			          "usr_data.UsrCod",
			QueryFields,
			(unsigned) Role,
			Gbl.Hierarchy.Ins.InsCod);
	 break;
      case HieLvl_CTR:
	 /* Get users in courses from the current center */
	 DB_BuildQuery (&Query,
			"SELECT %s"
			 " FROM usr_data,"
			       "crs_users,"
			       "crs_courses,"
			       "deg_degrees"
			" WHERE usr_data.UsrCod=crs_users.UsrCod"
			  " AND crs_users.Role=%u"
			  " AND crs_users.CrsCod=crs_courses.CrsCod"
			  " AND crs_courses.DegCod=deg_degrees.DegCod"
			  " AND deg_degrees.CtrCod=%ld"
			" ORDER BY usr_data.Surname1,"
			          "usr_data.Surname2,"
			          "usr_data.FirstName,"
			          "usr_data.UsrCod",
			QueryFields,
			(unsigned) Role,
			Gbl.Hierarchy.Ctr.CtrCod);
	 break;
      case HieLvl_DEG:
	 /* Get users in courses from the current degree */
	 DB_BuildQuery (&Query,
			"SELECT %s"
		 	 " FROM usr_data,"
			       "crs_users,"
			       "crs_courses"
			" WHERE usr_data.UsrCod=crs_users.UsrCod"
			  " AND crs_users.Role=%u"
			  " AND crs_users.CrsCod=crs_courses.CrsCod"
			  " AND crs_courses.DegCod=%ld"
			" ORDER BY usr_data.Surname1,"
			          "usr_data.Surname2,"
			          "usr_data.FirstName,"
			          "usr_data.UsrCod",
			QueryFields,
			(unsigned) Role,
			Gbl.Hierarchy.Deg.DegCod);
	 break;
      case HieLvl_CRS:
	 /* Get users from the current course */
	 Usr_BuildQueryToGetUsrsLstCrs (&Query,Role);
	 break;
      default:
	 Err_WrongScopeExit ();
	 break;
     }
/*
   if (Gbl.Usrs.Me.Roles.LoggedRole == Rol_SYS_ADM)
      Lay_ShowAlert (Lay_INFO,Query);
*/
   /***** Get list of users from database given a query *****/
   Usr_GetListUsrsFromQuery (Query,Role,Scope);
  }

/*****************************************************************************/
/*********** Search for users with a given role in current scope *************/
/*****************************************************************************/

void Usr_SearchListUsrs (Rol_Role_t Role)
  {
   char *Query = NULL;
   char SubQueryRole[64];
   const char *QueryFields =
      "DISTINCT usr_data.UsrCod,"
      "usr_data.EncryptedUsrCod,"
      "usr_data.Password,"
      "usr_data.Surname1,"
      "usr_data.Surname2,"
      "usr_data.FirstName,"
      "usr_data.Sex,"
      "usr_data.Photo,"
      "usr_data.PhotoVisibility,"
      "usr_data.CtyCod,"
      "usr_data.InsCod";
   /*
   row[ 0]: usr_data.UsrCod
   row[ 1]: usr_data.EncryptedUsrCod
   row[ 2]: usr_data.Password
   row[ 3]: usr_data.Surname1
   row[ 4]: usr_data.Surname2
   row[ 5]: usr_data.FirstName
   row[ 6]: usr_data.Sex
   row[ 7]: usr_data.Photo
   row[ 8]: usr_data.PhotoVisibility
   row[ 9]: usr_data.CtyCod
   row[10]: usr_data.InsCod
   row[11]: crs_users.Role	(only if Scope == HieLvl_CRS)
   row[12]: crs_users.Accepted	(only if Scope == HieLvl_CRS)
   */
   const char *OrderQuery = "candidate_users.UsrCod=usr_data.UsrCod"
			    " ORDER BY usr_data.Surname1,"
			              "usr_data.Surname2,"
			              "usr_data.FirstName,"
			              "usr_data.UsrCod";

   /***** Build query *****/
   // if Gbl.Scope.Current is course ==> 3 columns are retrieved: UsrCod, Sex, Accepted
   //                           else ==> 2 columns are retrieved: UsrCod, Sex
   // Search is faster (aproximately x2) using a temporary table to store users found in the whole platform
   switch (Role)
     {
      case Rol_UNK:	// Here Rol_UNK means any rol (role does not matter)
	 switch (Gbl.Scope.Current)
	   {
	    case HieLvl_SYS:
	       /* Search users from the whole platform */
	       DB_BuildQuery (&Query,
			      "SELECT %s"
			      " FROM candidate_users,usr_data"
			      " WHERE %s",
			      QueryFields,OrderQuery);
	       break;
	    case HieLvl_CTY:
	       /* Search users in courses from the current country */
	       DB_BuildQuery (&Query,
			      "SELECT %s"
			       " FROM candidate_users,"
			             "crs_users,"
			             "crs_courses,"
			             "deg_degrees,"
			             "ctr_centers,"
			             "ins_instits,"
			             "usr_data"
			      " WHERE candidate_users.UsrCod=crs_users.UsrCod"
			        " AND crs_users.CrsCod=crs_courses.CrsCod"
			        " AND crs_courses.DegCod=deg_degrees.DegCod"
			        " AND deg_degrees.CtrCod=ctr_centers.CtrCod"
			        " AND ctr_centers.InsCod=ins_instits.InsCod"
			        " AND ins_instits.CtyCod=%ld"
			        " AND %s",
			      QueryFields,
			      Gbl.Hierarchy.Cty.CtyCod,
			      OrderQuery);
	       break;
	    case HieLvl_INS:
	       /* Search users in courses from the current institution */
	       DB_BuildQuery (&Query,
			      "SELECT %s"
			       " FROM candidate_users,"
			             "crs_users,"
			             "crs_courses,"
			             "deg_degrees,"
			             "ctr_centers,"
			             "usr_data"
			      " WHERE candidate_users.UsrCod=crs_users.UsrCod"
			        " AND crs_users.CrsCod=crs_courses.CrsCod"
			        " AND crs_courses.DegCod=deg_degrees.DegCod"
			        " AND deg_degrees.CtrCod=ctr_centers.CtrCod"
			        " AND ctr_centers.InsCod=%ld"
			        " AND %s",
			      QueryFields,
			      Gbl.Hierarchy.Ins.InsCod,
			      OrderQuery);
	       break;
	    case HieLvl_CTR:
	       /* Search users in courses from the current center */
	       DB_BuildQuery (&Query,
			      "SELECT %s"
			       " FROM candidate_users,"
			             "crs_users,"
			             "crs_courses,"
			             "deg_degrees,"
			            " usr_data"
			      " WHERE candidate_users.UsrCod=crs_users.UsrCod"
			        " AND crs_users.CrsCod=crs_courses.CrsCod"
			        " AND crs_courses.DegCod=deg_degrees.DegCod"
			        " AND deg_degrees.CtrCod=%ld"
			        " AND %s",
			      QueryFields,
			      Gbl.Hierarchy.Ctr.CtrCod,
			      OrderQuery);
	       break;
	    case HieLvl_DEG:
	       /* Search users in courses from the current degree */
	       DB_BuildQuery (&Query,
			      "SELECT %s"
			       " FROM candidate_users,"
			             "crs_users,"
			             "crs_courses,"
			             "usr_data"
			      " WHERE candidate_users.UsrCod=crs_users.UsrCod"
			        " AND crs_users.CrsCod=crs_courses.CrsCod"
			        " AND crs_courses.DegCod=%ld"
			        " AND %s",
			      QueryFields,
			      Gbl.Hierarchy.Deg.DegCod,
			      OrderQuery);
	       break;
	    case HieLvl_CRS:
	       /* Search users in courses from the current course */
	       DB_BuildQuery (&Query,
			      "SELECT %s,"
			             "crs_users.Role,"
			             "crs_users.Accepted"
			       " FROM candidate_users,"
			             "crs_users,"
			             "usr_data"
			      " WHERE candidate_users.UsrCod=crs_users.UsrCod"
			        " AND crs_users.CrsCod=%ld"
			        " AND %s",
			      QueryFields,
			      Gbl.Hierarchy.Crs.CrsCod,
			      OrderQuery);
	       break;
	    default:
	       Err_WrongScopeExit ();
	       break;
	   }
         break;
      case Rol_GST:	// Guests (scope is not used)
	 /* Search users with no courses */
	 DB_BuildQuery (&Query,
			"SELECT %s"
			 " FROM candidate_users,"
			       "usr_data"
			" WHERE candidate_users.UsrCod NOT IN"
			      " (SELECT UsrCod"
			         " FROM crs_users)"
			  " AND %s",
			QueryFields,
			OrderQuery);
	 break;
      case Rol_STD:	// Student
      case Rol_NET:	// Non-editing teacher
      case Rol_TCH:	// Teacher
	 /*
	    To achieve maximum speed, it's important to do the things in this order:
	    1) Search for user's name (UsrQuery) getting candidate users
	    2) Filter the candidate users according to scope
	 */
	 switch (Role)
	   {
	    case Rol_STD:	// Student
	       sprintf (SubQueryRole," AND crs_users.Role=%u",
			(unsigned) Rol_STD);
	       break;
	    case Rol_NET:	// Non-editing teacher
	    case Rol_TCH:	// or teacher
	       sprintf (SubQueryRole," AND (crs_users.Role=%u"
			               " OR crs_users.Role=%u)",
			(unsigned) Rol_NET,(unsigned) Rol_TCH);
	       break;
	    default:
	       SubQueryRole[0] = '\0';
	       break;
	   }
	 switch (Gbl.Scope.Current)
	   {
	    case HieLvl_SYS:
	       /* Search users in courses from the whole platform */
	       DB_BuildQuery (&Query,
			      "SELECT %s"
			       " FROM candidate_users,"
			             "crs_users,"
			             "usr_data"
			      " WHERE candidate_users.UsrCod=crs_users.UsrCod"
			        "%s"
			        " AND %s",
			      QueryFields,
			      SubQueryRole,
			      OrderQuery);
	       break;
	    case HieLvl_CTY:
	       /* Search users in courses from the current country */
	       DB_BuildQuery (&Query,
			      "SELECT %s"
			       " FROM candidate_users,"
			             "crs_users,"
			             "crs_courses,"
			             "deg_degrees,"
			             "ctr_centers,"
			             "ins_instits,"
			             "usr_data"
			      " WHERE candidate_users.UsrCod=crs_users.UsrCod"
			        "%s"
			        " AND crs_users.CrsCod=crs_courses.CrsCod"
			        " AND crs_courses.DegCod=deg_degrees.DegCod"
			        " AND deg_degrees.CtrCod=ctr_centers.CtrCod"
			        " AND ctr_centers.InsCod=ins_instits.InsCod"
			        " AND ins_instits.CtyCod=%ld"
			        " AND %s",
			      QueryFields,
			      SubQueryRole,
			      Gbl.Hierarchy.Cty.CtyCod,
			      OrderQuery);
	       break;
	    case HieLvl_INS:
	       /* Search users in courses from the current institution */
	       DB_BuildQuery (&Query,
			      "SELECT %s"
			       " FROM candidate_users,"
			             "crs_users,"
			             "crs_courses,"
			             "deg_degrees,"
			             "ctr_centers,"
			             "usr_data"
			      " WHERE candidate_users.UsrCod=crs_users.UsrCod"
			        "%s"
			        " AND crs_users.CrsCod=crs_courses.CrsCod"
			        " AND crs_courses.DegCod=deg_degrees.DegCod"
			        " AND deg_degrees.CtrCod=ctr_centers.CtrCod"
			        " AND ctr_centers.InsCod=%ld"
			        " AND %s",
			      QueryFields,
			      SubQueryRole,
			      Gbl.Hierarchy.Ins.InsCod,
			      OrderQuery);
	       break;
	    case HieLvl_CTR:
	       /* Search users in courses from the current center */
	       DB_BuildQuery (&Query,
			      "SELECT %s"
			       " FROM candidate_users,"
			             "crs_users,"
			             "crs_courses,"
			             "deg_degrees,"
			             "usr_data"
			      " WHERE candidate_users.UsrCod=crs_users.UsrCod"
			        "%s"
			        " AND crs_users.CrsCod=crs_courses.CrsCod"
			        " AND crs_courses.DegCod=deg_degrees.DegCod"
			        " AND deg_degrees.CtrCod=%ld"
			        " AND %s",
			      QueryFields,
			      SubQueryRole,
			      Gbl.Hierarchy.Ctr.CtrCod,
			      OrderQuery);
	       break;
	    case HieLvl_DEG:
	       /* Search users in courses from the current degree */
	       DB_BuildQuery (&Query,
			      "SELECT %s"
			       " FROM candidate_users,"
			             "crs_users,"
			             "crs_courses,"
			             "usr_data"
			      " WHERE candidate_users.UsrCod=crs_users.UsrCod"
			        "%s"
			        " AND crs_users.CrsCod=crs_courses.CrsCod"
			        " AND crs_courses.DegCod=%ld"
			        " AND %s",
			      QueryFields,
			      SubQueryRole,
			      Gbl.Hierarchy.Deg.DegCod,
			      OrderQuery);
	       break;
	    case HieLvl_CRS:
	       /* Search users in courses from the current course */
	       DB_BuildQuery (&Query,
			      "SELECT %s,"
			             "crs_users.Role,"
			             "crs_users.Accepted"
			       " FROM candidate_users,"
			             "crs_users,"
			             "usr_data"
			      " WHERE candidate_users.UsrCod=crs_users.UsrCod"
			        "%s"
			        " AND crs_users.CrsCod=%ld"
			        " AND %s",
			      QueryFields,
			      SubQueryRole,
			      Gbl.Hierarchy.Crs.CrsCod,
			      OrderQuery);
	       break;
	    default:
	       Err_WrongScopeExit ();
	       break;
	   }
	 break;
      default:
	 Err_WrongRoleExit ();
	 break;
     }

   // if (Gbl.Usrs.Me.Roles.LoggedRole == Rol_SYS_ADM)
   //   Lay_ShowAlert (Lay_INFO,Query);

   /***** Get list of users from database given a query *****/
   Usr_GetListUsrsFromQuery (Query,Role,Gbl.Scope.Current);
  }

/*****************************************************************************/
/*************** Create temporary table with candidate users *****************/
/*****************************************************************************/

void Usr_CreateTmpTableAndSearchCandidateUsrs (const char SearchQuery[Sch_MAX_BYTES_SEARCH_QUERY + 1])
  {
   char Query[256 + Sch_MAX_BYTES_SEARCH_QUERY];

   /***** Create temporary table with candidate users *****/
   /*
      - Search is faster (aproximately x2) using temporary tables.
      - Searching for names is made in the whole platform
        and stored in this table.
   */
   sprintf (Query,"CREATE TEMPORARY TABLE candidate_users"
		  " (UsrCod INT NOT NULL,UNIQUE INDEX(UsrCod)) ENGINE=MEMORY"
		  " SELECT UsrCod"
		    " FROM usr_data"
		   " WHERE %s",
	    SearchQuery);
   if (mysql_query (&Gbl.mysql,Query))
      DB_ExitOnMySQLError ("can not create temporary table");
  }

/*****************************************************************************/
/***************** Drop temporary table with candidate users *****************/
/*****************************************************************************/

void Usr_DropTmpTableWithCandidateUsrs (void)
  {
   char Query[128];

   sprintf (Query,"DROP TEMPORARY TABLE IF EXISTS candidate_users");
   if (mysql_query (&Gbl.mysql,Query))
      DB_ExitOnMySQLError ("can not remove temporary table");
  }

/*****************************************************************************/
/******************** Get list with data of administrators *******************/
/*****************************************************************************/

static void Usr_GetAdmsLst (HieLvl_Level_t Scope)
  {
   const char *QueryFields =
      "UsrCod,"
      "EncryptedUsrCod,"
      "Password,"
      "Surname1,"
      "Surname2,"
      "FirstName,"
      "Sex,"
      "Photo,"
      "PhotoVisibility,"
      "CtyCod,"
      "InsCod";
   /*
   row[ 0]: usr_data.UsrCod
   row[ 1]: usr_data.EncryptedUsrCod
   row[ 2]: usr_data.Password
   row[ 3]: usr_data.Surname1
   row[ 4]: usr_data.Surname2
   row[ 5]: usr_data.FirstName
   row[ 6]: usr_data.Sex
   row[ 7]: usr_data.Photo
   row[ 8]: usr_data.PhotoVisibility
   row[ 9]: usr_data.CtyCod
   row[10]: usr_data.InsCod
   */
   char *Query = NULL;

   /***** Build query *****/
   // Important: it is better to use:
   // SELECT... WHERE UsrCod IN (SELECT...) OR UsrCod IN (SELECT...) <-- fast
   // instead of using or with different joins:
   // SELECT... WHERE (...) OR (...) <-- very slow
   switch (Scope)
     {
      case HieLvl_SYS:	// All admins
	 DB_BuildQuery (&Query,
			"SELECT %s"
			 " FROM usr_data"
			" WHERE UsrCod IN "
			       "(SELECT DISTINCT UsrCod"
				 " FROM usr_admins)"
			" ORDER BY Surname1,"
			          "Surname2,"
			          "FirstName,"
			          "UsrCod",
			QueryFields);
         break;
      case HieLvl_CTY:	// System admins
				// and admins of the institutions, centers and degrees in the current country
         DB_BuildQuery (&Query,
                        "SELECT %s"
                         " FROM usr_data"
			" WHERE UsrCod IN "
			       "(SELECT UsrCod"
			         " FROM usr_admins"
			        " WHERE Scope='%s')"
			   " OR UsrCod IN "
			       "(SELECT usr_admins.UsrCod"
			         " FROM usr_admins,"
				       "ins_instits"
				" WHERE usr_admins.Scope='%s'"
			          " AND usr_admins.Cod=ins_instits.InsCod"
			          " AND ins_instits.CtyCod=%ld)"
			   " OR UsrCod IN "
			"(SELECT usr_admins.UsrCod"
			  " FROM usr_admins,"
			        "ctr_centers,"
			        "ins_instits"
			 " WHERE usr_admins.Scope='%s'"
			   " AND usr_admins.Cod=ctr_centers.CtrCod"
			   " AND ctr_centers.InsCod=ins_instits.InsCod"
			   " AND ins_instits.CtyCod=%ld)"
			    " OR UsrCod IN "
				"(SELECT usr_admins.UsrCod"
				  " FROM usr_admins,"
				        "deg_degrees,"
					"ctr_centers,"
					"ins_instits"
				 " WHERE usr_admins.Scope='%s'"
				   " AND usr_admins.Cod=deg_degrees.DegCod"
				   " AND deg_degrees.CtrCod=ctr_centers.CtrCod"
				   " AND ctr_centers.InsCod=ins_instits.InsCod"
				   " AND ins_instits.CtyCod=%ld)"
			" ORDER BY Surname1,"
			          "Surname2,"
			          "FirstName,"
			          "UsrCod",
			QueryFields,
			Sco_GetDBStrFromScope (HieLvl_SYS),
			Sco_GetDBStrFromScope (HieLvl_INS),Gbl.Hierarchy.Cty.CtyCod,
			Sco_GetDBStrFromScope (HieLvl_CTR),Gbl.Hierarchy.Cty.CtyCod,
			Sco_GetDBStrFromScope (HieLvl_DEG),Gbl.Hierarchy.Cty.CtyCod);
         break;
      case HieLvl_INS:	// System admins,
				// admins of the current institution,
				// and admins of the centers and degrees in the current institution
         DB_BuildQuery (&Query,
                        "SELECT %s"
                         " FROM usr_data"
			" WHERE UsrCod IN "
			       "(SELECT UsrCod"
			         " FROM usr_admins"
			        " WHERE Scope='%s')"
			   " OR UsrCod IN "
			       "(SELECT UsrCod"
			         " FROM usr_admins"
			        " WHERE Scope='%s'"
			          " AND Cod=%ld)"
			   " OR UsrCod IN "
			       "(SELECT usr_admins.UsrCod"
			         " FROM usr_admins,"
				       "ctr_centers"
			        " WHERE usr_admins.Scope='%s'"
			          " AND usr_admins.Cod=ctr_centers.CtrCod"
			          " AND ctr_centers.InsCod=%ld)"
			   " OR UsrCod IN "
			       "(SELECT usr_admins.UsrCod"
			         " FROM usr_admins,"
				       "deg_degrees,"
				       "ctr_centers"
			        " WHERE usr_admins.Scope='%s'"
			          " AND usr_admins.Cod=deg_degrees.DegCod"
			          " AND deg_degrees.CtrCod=ctr_centers.CtrCod"
			          " AND ctr_centers.InsCod=%ld)"
			" ORDER BY Surname1,"
			          "Surname2,"
			          "FirstName,"
			          "UsrCod",
			QueryFields,
			Sco_GetDBStrFromScope (HieLvl_SYS),
			Sco_GetDBStrFromScope (HieLvl_INS),Gbl.Hierarchy.Ins.InsCod,
			Sco_GetDBStrFromScope (HieLvl_CTR),Gbl.Hierarchy.Ins.InsCod,
			Sco_GetDBStrFromScope (HieLvl_DEG),Gbl.Hierarchy.Ins.InsCod);
         break;
      case HieLvl_CTR:	// System admins,
				// admins of the current institution,
				// admins and the current center,
				// and admins of the degrees in the current center
	 DB_BuildQuery (&Query,
			"SELECT %s"
			 " FROM usr_data"
			" WHERE UsrCod IN "
			       "(SELECT UsrCod"
			         " FROM usr_admins"
			        " WHERE Scope='%s')"
			   " OR UsrCod IN "
			       "(SELECT UsrCod"
			         " FROM usr_admins"
			        " WHERE Scope='%s'"
			          " AND Cod=%ld)"
			   " OR UsrCod IN "
			       "(SELECT UsrCod"
			         " FROM usr_admins"
			        " WHERE Scope='%s'"
			          " AND Cod=%ld)"
			   " OR UsrCod IN "
			       "(SELECT usr_admins.UsrCod"
			        " FROM usr_admins,"
				      "deg_degrees"
			       " WHERE usr_admins.Scope='%s'"
			         " AND usr_admins.Cod=deg_degrees.DegCod"
			         " AND deg_degrees.CtrCod=%ld)"
			" ORDER BY Surname1,"
			          "Surname2,"
			          "FirstName,"
			          "UsrCod",
			QueryFields,
			Sco_GetDBStrFromScope (HieLvl_SYS),
			Sco_GetDBStrFromScope (HieLvl_INS),Gbl.Hierarchy.Ins.InsCod,
			Sco_GetDBStrFromScope (HieLvl_CTR),Gbl.Hierarchy.Ctr.CtrCod,
			Sco_GetDBStrFromScope (HieLvl_DEG),Gbl.Hierarchy.Ctr.CtrCod);
         break;
      case HieLvl_DEG:	// System admins
				// and admins of the current institution, center or degree
         DB_BuildQuery (&Query,
			"SELECT %s"
			 " FROM usr_data"
			" WHERE UsrCod IN "
			       "(SELECT UsrCod"
			         " FROM usr_admins"
			        " WHERE Scope='%s')"
			   " OR UsrCod IN "
			       "(SELECT UsrCod"
			         " FROM usr_admins"
			        " WHERE Scope='%s'"
			          " AND Cod=%ld)"
			   " OR UsrCod IN "
			       "(SELECT UsrCod"
			         " FROM usr_admins"
			        " WHERE Scope='%s'"
			          " AND Cod=%ld)"
			   " OR UsrCod IN "
			       "(SELECT UsrCod"
			         " FROM usr_admins"
			        " WHERE Scope='%s'"
			          " AND Cod=%ld)"
			" ORDER BY Surname1,"
			          "Surname2,"
			          "FirstName,"
			          "UsrCod",
			QueryFields,
			Sco_GetDBStrFromScope (HieLvl_SYS),
			Sco_GetDBStrFromScope (HieLvl_INS),Gbl.Hierarchy.Ins.InsCod,
			Sco_GetDBStrFromScope (HieLvl_CTR),Gbl.Hierarchy.Ctr.CtrCod,
			Sco_GetDBStrFromScope (HieLvl_DEG),Gbl.Hierarchy.Deg.DegCod);
         break;
      default:        // not aplicable
	 Err_WrongScopeExit ();
         break;
     }

   /***** Get list of administrators from database *****/
   Usr_GetListUsrsFromQuery (Query,Rol_DEG_ADM,Scope);
  }

/*****************************************************************************/
/************************ Get list with data of guests ***********************/
/*****************************************************************************/

static void Usr_GetGstsLst (HieLvl_Level_t Scope)
  {
   const char *QueryFields =
      "UsrCod,"
      "EncryptedUsrCod,"
      "Password,"
      "Surname1,"
      "Surname2,"
      "FirstName,"
      "Sex,"
      "Photo,"
      "PhotoVisibility,"
      "CtyCod,"
      "InsCod";
   /*
   row[ 0]: usr_data.UsrCod
   row[ 1]: usr_data.EncryptedUsrCod
   row[ 2]: usr_data.Password
   row[ 3]: usr_data.Surname1
   row[ 4]: usr_data.Surname2
   row[ 5]: usr_data.FirstName
   row[ 6]: usr_data.Sex
   row[ 7]: usr_data.Photo
   row[ 8]: usr_data.PhotoVisibility
   row[ 9]: usr_data.CtyCod
   row[10]: usr_data.InsCod
   */
   char *Query = NULL;

   /***** Build query *****/
   switch (Scope)
     {
      case HieLvl_SYS:
	 DB_BuildQuery (&Query,
         		"SELECT %s"
         	 	 " FROM usr_data"
			" WHERE UsrCod NOT IN"
			      " (SELECT UsrCod"
			         " FROM crs_users)"
			" ORDER BY Surname1,"
			          "Surname2,"
			          "FirstName,"
			          "UsrCod",
			QueryFields);
         break;
      case HieLvl_CTY:
	 DB_BuildQuery (&Query,
			"SELECT %s"
			 " FROM usr_data"
			" WHERE (CtyCod=%ld"
			       " OR"
			       " InsCtyCod=%ld)"
			  " AND UsrCod NOT IN"
			      " (SELECT UsrCod"
			         " FROM crs_users)"
			" ORDER BY Surname1,"
			          "Surname2,"
			          "FirstName,"
			          "UsrCod",
			QueryFields,
			Gbl.Hierarchy.Cty.CtyCod,
			Gbl.Hierarchy.Cty.CtyCod);
         break;
      case HieLvl_INS:
	 DB_BuildQuery (&Query,
			"SELECT %s"
			 " FROM usr_data"
			" WHERE InsCod=%ld"
			  " AND UsrCod NOT IN"
			      " (SELECT UsrCod"
			         " FROM crs_users)"
			" ORDER BY Surname1,"
			          "Surname2,"
			          "FirstName,"
			          "UsrCod",
			QueryFields,
			Gbl.Hierarchy.Ins.InsCod);
         break;
      case HieLvl_CTR:
	 DB_BuildQuery (&Query,
			"SELECT %s"
			 " FROM usr_data"
			" WHERE CtrCod=%ld"
			  " AND UsrCod NOT IN"
			      " (SELECT UsrCod"
			         " FROM crs_users)"
			" ORDER BY Surname1,"
			          "Surname2,"
			          "FirstName,"
			          "UsrCod",
			QueryFields,
			Gbl.Hierarchy.Ctr.CtrCod);
         break;
      default:        // not aplicable
         return;
     }

   /***** Get list of students from database *****/
   Usr_GetListUsrsFromQuery (Query,Rol_GST,Scope);
  }

/*****************************************************************************/
/*********** Get the user's codes of all the students of a degree ************/
/*****************************************************************************/

void Usr_GetUnorderedStdsCodesInDeg (long DegCod)
  {
   char *Query = NULL;

   /***** Get the students in a degree from database *****/
   DB_BuildQuery (&Query,
		  "SELECT DISTINCT usr_data.UsrCod,"	// row[ 0]
			 "usr_data.EncryptedUsrCod,"	// row[ 1]
			 "usr_data.Password,"		// row[ 2]
			 "usr_data.Surname1,"		// row[ 3]
			 "usr_data.Surname2,"		// row[ 4]
			 "usr_data.FirstName,"		// row[ 5]
			 "usr_data.Sex,"		// row[ 6]
			 "usr_data.Photo,"		// row[ 7]
			 "usr_data.PhotoVisibility,"	// row[ 8]
			 "usr_data.CtyCod,"		// row[ 9]
			 "usr_data.InsCod"		// row[10]
		   " FROM crs_courses,"
		         "crs_users,"
		         "usr_data"
		  " WHERE crs_courses.DegCod=%ld"
		    " AND crs_courses.CrsCod=crs_users.CrsCod"
		    " AND crs_users.Role=%u"
		    " AND crs_users.UsrCod=usr_data.UsrCod",
		  DegCod,
		  (unsigned) Rol_STD);

   /***** Get list of students from database *****/
   Usr_GetListUsrsFromQuery (Query,Rol_STD,HieLvl_DEG);

   /***** Free query string *****/
   free (Query);
  }

/*****************************************************************************/
/********************** Get list of users from database **********************/
/*****************************************************************************/

static void Usr_GetListUsrsFromQuery (char *Query,Rol_Role_t Role,HieLvl_Level_t Scope)
  {
   extern const char *Txt_The_list_of_X_users_is_too_large_to_be_displayed;
   MYSQL_RES *mysql_res;
   MYSQL_ROW row;
   unsigned NumUsr;
   struct UsrInList *UsrInList;
   bool Abort = false;

   if (Query == NULL)
     {
      Gbl.Usrs.LstUsrs[Role].NumUsrs = 0;
      return;
     }

   if (!Query[0])
     {
      Gbl.Usrs.LstUsrs[Role].NumUsrs = 0;
      return;
     }

   /***** Query database *****/
   if ((Gbl.Usrs.LstUsrs[Role].NumUsrs = (unsigned)
       DB_QuerySELECT (&mysql_res,"can not get list of users",
		       "%s",
		       Query)))
     {
      if (Gbl.Usrs.LstUsrs[Role].NumUsrs > Cfg_MAX_USRS_IN_LIST)
        {
	 Ale_ShowAlert (Ale_WARNING,Txt_The_list_of_X_users_is_too_large_to_be_displayed,
		        Gbl.Usrs.LstUsrs[Role].NumUsrs);
         Abort = true;
        }
      else
        {
         /***** Allocate memory for the list of users *****/
         Usr_AllocateUsrsList (Role);

         /***** Get list of users *****/
         for (NumUsr = 0;
              NumUsr < Gbl.Usrs.LstUsrs[Role].NumUsrs;
              NumUsr++)
           {
            /* Get next user */
            row = mysql_fetch_row (mysql_res);
            /*
            row[ 0]: usr_data.UsrCod
            row[ 1]: usr_data.EncryptedUsrCod
            row[ 2]: usr_data.Password (used to check if a teacher can edit user's data)
	    row[ 3]: usr_data.Surname1
	    row[ 4]: usr_data.Surname2
	    row[ 5]: usr_data.FirstName
            row[ 6]: usr_data.Sex
            row[ 7]: usr_data.Photo
            row[ 8]: usr_data.PhotoVisibility
	    row[ 9]: usr_data.CtyCod
	    row[10]: usr_data.InsCod
	    row[11]: crs_users.Role	(only if Scope == HieLvl_CRS)
	    row[12]: crs_users.Accepted	(only if Scope == HieLvl_CRS)
	    */
            UsrInList = &Gbl.Usrs.LstUsrs[Role].Lst[NumUsr];

            /* Get user's code (row[0]) */
            UsrInList->UsrCod = Str_ConvertStrCodToLongCod (row[0]);

            /* Get encrypted user's code (row[1]), encrypted password (row[2]),
               surname 1 (row[3]), surname 2 (row[4]), first name (row[5]), */
	    Str_Copy (UsrInList->EnUsrCod,row[1],sizeof (UsrInList->EnUsrCod) - 1);
	    Str_Copy (UsrInList->Password,row[2],sizeof (UsrInList->Password) - 1);
	    Str_Copy (UsrInList->Surname1,row[3],sizeof (UsrInList->Surname1) - 1);
	    Str_Copy (UsrInList->Surname2,row[4],sizeof (UsrInList->Surname2) - 1);
	    Str_Copy (UsrInList->FrstName,row[5],sizeof (UsrInList->FrstName) - 1);

            /* Get user's sex (row[6]) */
            UsrInList->Sex = Usr_GetSexFromStr (row[6]);

            /* Get user's photo (row[7]) */
	    Str_Copy (UsrInList->Photo   ,row[7],sizeof (UsrInList->Photo   ) - 1);

            /* Get user's photo visibility (row[8]) */
            UsrInList->PhotoVisibility = Pri_GetVisibilityFromStr (row[8]);

            /* Get user's country code (row[9]) */
	    UsrInList->CtyCod = Str_ConvertStrCodToLongCod (row[9]);

            /* Get user's institution code (row[10]) */
	    UsrInList->InsCod = Str_ConvertStrCodToLongCod (row[10]);

            /* Get user's role and acceptance of enrolment in course(s)
               (row[11], row[12] if Scope == HieLvl_CRS) */
            switch (Role)
              {
               case Rol_UNK:	// Here Rol_UNK means any user
		  switch (Scope)
		    {
		     case HieLvl_UNK:	// Unknown
			Err_WrongScopeExit ();
			break;
		     case HieLvl_SYS:	// System
			// Query result has not a column with the acceptation
			UsrInList->RoleInCurrentCrsDB = Rol_UNK;
			if (Usr_GetNumCrssOfUsr (UsrInList->UsrCod))
			   UsrInList->Accepted = (Usr_GetNumCrssOfUsrNotAccepted (UsrInList->UsrCod) == 0);
			else
			   UsrInList->Accepted = false;
			break;
		     case HieLvl_CTY:	// Country
		     case HieLvl_INS:	// Institution
		     case HieLvl_CTR:	// Center
		     case HieLvl_DEG:	// Degree
			// Query result has not a column with the acceptation
			UsrInList->RoleInCurrentCrsDB = Rol_UNK;
			UsrInList->Accepted = (Usr_GetNumCrssOfUsrNotAccepted (UsrInList->UsrCod) == 0);
			break;
		     case HieLvl_CRS:	// Course
			// Query result has a column with the acceptation
			UsrInList->RoleInCurrentCrsDB = Rol_ConvertUnsignedStrToRole (row[11]);
			UsrInList->Accepted = (row[12][0] == 'Y');
			break;
		    }
        	  break;
               case Rol_GST:        // Guests have no courses,...
            	    	    	    	// ...so they have not accepted...
                                        // ...inscription in any course
               case Rol_DEG_ADM:	// Any admin (degree, center, institution or system)
	          UsrInList->RoleInCurrentCrsDB = Rol_UNK;
	          UsrInList->Accepted = false;
	          break;
               case Rol_STD:
               case Rol_NET:
               case Rol_TCH:
		  switch (Scope)
		    {
		     case HieLvl_UNK:	// Unknown
			Err_WrongScopeExit ();
			break;
		     case HieLvl_SYS:	// System
		     case HieLvl_CTY:	// Country
		     case HieLvl_INS:	// Institution
		     case HieLvl_CTR:	// Center
		     case HieLvl_DEG:	// Degree
			// Query result has not a column with the acceptation
	                UsrInList->RoleInCurrentCrsDB = Rol_UNK;
			UsrInList->Accepted = (Usr_GetNumCrssOfUsrWithARoleNotAccepted (UsrInList->UsrCod,Role) == 0);
			break;
		     case HieLvl_CRS:	// Course
			// Query result has a column with the acceptation
			UsrInList->RoleInCurrentCrsDB = Rol_ConvertUnsignedStrToRole (row[11]);
			UsrInList->Accepted = (row[12][0] == 'Y');
			break;
		    }
        	  break;
               default:
		  Err_WrongRoleExit ();
        	  break;
	      }

            /* By default, users are not removed */
            UsrInList->Remove = false;
           }
        }
     }

   /***** Free structure that stores the query result *****/
   DB_FreeMySQLResult (&mysql_res);

   if (Abort)
      Err_ShowErrorAndExit (NULL);
  }

/*****************************************************************************/
/********************** Copy user's basic data from list *********************/
/*****************************************************************************/

void Usr_CopyBasicUsrDataFromList (struct UsrData *UsrDat,
                                   const struct UsrInList *UsrInList)
  {
   UsrDat->UsrCod             = UsrInList->UsrCod;
   Str_Copy (UsrDat->EnUsrCod,UsrInList->EnUsrCod,sizeof (UsrDat->EnUsrCod) - 1);
   Str_Copy (UsrDat->Surname1,UsrInList->Surname1,sizeof (UsrDat->Surname1) - 1);
   Str_Copy (UsrDat->Surname2,UsrInList->Surname2,sizeof (UsrDat->Surname2) - 1);
   Str_Copy (UsrDat->FrstName,UsrInList->FrstName,sizeof (UsrDat->FrstName) - 1);
   UsrDat->Sex                = UsrInList->Sex;
   Str_Copy (UsrDat->Photo   ,UsrInList->Photo   ,sizeof (UsrDat->Photo   ) - 1);
   UsrDat->PhotoVisibility    = UsrInList->PhotoVisibility;
   UsrDat->CtyCod             = UsrInList->CtyCod;
   UsrDat->InsCod             = UsrInList->InsCod;
   UsrDat->Roles.InCurrentCrs = UsrInList->RoleInCurrentCrsDB;
   UsrDat->Accepted           = UsrInList->Accepted;
  }

/*****************************************************************************/
/********************** Allocate space for list of users *********************/
/*****************************************************************************/

static void Usr_AllocateUsrsList (Rol_Role_t Role)
  {
/*
if (Gbl.Usrs.Me.Roles.LoggedRole == Rol_SYS_ADM)
   {
    snprintf (Gbl.Alert.Txt,sizeof (Gbl.Alert.Txt),
	      "Memory used by list = %lu",
	      (long) sizeof (struct UsrInList) * NumUsrs);
    Lay_ShowAlert (Lay_INFO,Gbl.Alert.Txt);
   }
*/
   if (Gbl.Usrs.LstUsrs[Role].NumUsrs)
      if ((Gbl.Usrs.LstUsrs[Role].Lst = calloc (Gbl.Usrs.LstUsrs[Role].NumUsrs,
                                                sizeof (*Gbl.Usrs.LstUsrs[Role].Lst))) == NULL)
         Err_NotEnoughMemoryExit ();
  }

/*****************************************************************************/
/********************* Free space used for list of users *********************/
/*****************************************************************************/

void Usr_FreeUsrsList (Rol_Role_t Role)
  {
   if (Gbl.Usrs.LstUsrs[Role].NumUsrs)
     {
      /***** Free the list itself *****/
      if (Gbl.Usrs.LstUsrs[Role].Lst)
        {
         free (Gbl.Usrs.LstUsrs[Role].Lst);
         Gbl.Usrs.LstUsrs[Role].Lst = NULL;
        }

      /***** Reset number of users *****/
      Gbl.Usrs.LstUsrs[Role].NumUsrs = 0;
     }
  }

/*****************************************************************************/
/******** Show form to confirm that I want to see a big list of users ********/
/*****************************************************************************/

bool Usr_GetIfShowBigList (unsigned NumUsrs,
                           void (*FuncParams) (void *Args),void *Args,
                           const char *OnSubmit)
  {
   bool ShowBigList;

   /***** If list of users is too big... *****/
   if (NumUsrs > Cfg_MIN_NUM_USERS_TO_CONFIRM_SHOW_BIG_LIST)
     {
      /***** Get parameter with user's confirmation
             to see a big list of users *****/
      if (!(ShowBigList = Par_GetParToBool ("ShowBigList")))
	 Usr_PutButtonToConfirmIWantToSeeBigList (NumUsrs,
	                                          FuncParams,Args,
	                                          OnSubmit);

      return ShowBigList;
     }
   else
      return true;        // List is not too big ==> show it
  }

/*****************************************************************************/
/******** Show form to confirm that I want to see a big list of users ********/
/*****************************************************************************/

static void Usr_PutButtonToConfirmIWantToSeeBigList (unsigned NumUsrs,
                                                     void (*FuncParams) (void *Args),void *Args,
                                                     const char *OnSubmit)
  {
   extern const char *Txt_The_list_of_X_users_is_too_large_to_be_displayed;
   extern const char *Txt_Show_anyway;

   /***** Show alert and button to confirm that I want to see the big list *****/
   Usr_FuncParamsBigList = FuncParams;	// Used to pass pointer to function
   Ale_ShowAlertAndButton (Gbl.Action.Act,Usr_USER_LIST_SECTION_ID,OnSubmit,
                           Usr_PutParamsConfirmIWantToSeeBigList,Args,
                           Btn_CONFIRM_BUTTON,Txt_Show_anyway,
			   Ale_WARNING,Txt_The_list_of_X_users_is_too_large_to_be_displayed,
                           NumUsrs);
  }

static void Usr_PutParamsConfirmIWantToSeeBigList (void *Args)
  {
   Grp_PutParamsCodGrps ();
   Usr_PutParamsPrefsAboutUsrList ();
   if (Usr_FuncParamsBigList)
      Usr_FuncParamsBigList (Args);
   Par_PutHiddenParamChar ("ShowBigList",'Y');
  }

/*****************************************************************************/
/************ Create list of selected users with one given user **************/
/*****************************************************************************/

void Usr_CreateListSelectedUsrsCodsAndFillWithOtherUsr (struct SelectedUsrs *SelectedUsrs)
  {
   /***** Create list of user codes and put encrypted user code in it *****/
   if (!SelectedUsrs->List[Rol_UNK])
     {
      if ((SelectedUsrs->List[Rol_UNK] =
	   malloc (Cry_BYTES_ENCRYPTED_STR_SHA256_BASE64 + 1)) == NULL)
         Err_NotEnoughMemoryExit ();
      Str_Copy (SelectedUsrs->List[Rol_UNK],Gbl.Usrs.Other.UsrDat.EnUsrCod,
		Cry_BYTES_ENCRYPTED_STR_SHA256_BASE64);
      SelectedUsrs->Filled = true;
     }
  }

/*****************************************************************************/
/************* Write parameter with the list of users selected ***************/
/*****************************************************************************/

void Usr_PutHiddenParSelectedUsrsCods (struct SelectedUsrs *SelectedUsrs)
  {
   char *ParamName;

   /***** Put a parameter indicating that a list of several users is present *****/
   Par_PutHiddenParamChar ("MultiUsrs",'Y');

   /***** Put a parameter with the encrypted user codes of several users *****/
   /* Build name of the parameter.
      Sometimes a unique action needs several distinct lists of users,
      so, it's necessary to use distinct names for the parameters. */
   Usr_BuildParamName (&ParamName,Usr_ParamUsrCod[Rol_UNK],SelectedUsrs->ParamSuffix);

   /* Put the parameter *****/
   if (Gbl.Session.IsOpen)
      Ses_InsertParamInDB (ParamName,SelectedUsrs->List[Rol_UNK]);
   else
      Par_PutHiddenParamString (NULL,ParamName,SelectedUsrs->List[Rol_UNK]);

   /***** Free allocated memory for parameter name *****/
   free (ParamName);
  }

/*****************************************************************************/
/************************* Get list of selected users ************************/
/*****************************************************************************/

void Usr_GetListsSelectedEncryptedUsrsCods (struct SelectedUsrs *SelectedUsrs)
  {
   extern const char *Par_SEPARATOR_PARAM_MULTIPLE;
   char *ParamName;
   unsigned Length;
   Rol_Role_t Role;

   if (!SelectedUsrs->Filled)	// Get list only if not already got
     {
      /***** Build name of the parameter.
	     Sometimes a unique action needs several distinct lists of users,
	     so, it's necessary to use distinct names for the parameters. *****/
      Usr_BuildParamName (&ParamName,Usr_ParamUsrCod[Rol_UNK],SelectedUsrs->ParamSuffix);

      /***** Get possible list of all selected users *****/
      Usr_AllocateListSelectedEncryptedUsrCods (SelectedUsrs,Rol_UNK);
      if (Gbl.Session.IsOpen)	// If the session is open, get parameter from DB
	{
	 Ses_GetParamFromDB (ParamName,SelectedUsrs->List[Rol_UNK],
			     Usr_MAX_BYTES_LIST_ENCRYPTED_USR_CODS);
	 Str_ChangeFormat (Str_FROM_FORM,Str_TO_TEXT,SelectedUsrs->List[Rol_UNK],
			   Usr_MAX_BYTES_LIST_ENCRYPTED_USR_CODS,true);
	}
      else
	 Par_GetParMultiToText (ParamName,SelectedUsrs->List[Rol_UNK],
				Usr_MAX_BYTES_LIST_ENCRYPTED_USR_CODS);

      /***** Free allocated memory for parameter name *****/
      free (ParamName);

      /***** Get list of selected users for each possible role *****/
      for (Role  = Rol_TCH;	// From the highest possible role of selected users...
	   Role >= Rol_GST;	// ...downto the lowest possible role of selected users
	   Role--)
	 if (Usr_ParamUsrCod[Role])
	   {
            /* Build name of the parameter */
	    Usr_BuildParamName (&ParamName,Usr_ParamUsrCod[Role],SelectedUsrs->ParamSuffix);

	    /* Get parameter with selected users with this role */
	    Usr_AllocateListSelectedEncryptedUsrCods (SelectedUsrs,Role);
	    Par_GetParMultiToText (ParamName,SelectedUsrs->List[Role],
				   Usr_MAX_BYTES_LIST_ENCRYPTED_USR_CODS);

	    /* Free allocated memory for parameter name */
	    free (ParamName);

	    /* Add selected users with this role
	       to the list with all selected users */
	    if (SelectedUsrs->List[Role][0])
	      {
	       if (SelectedUsrs->List[Rol_UNK][0])
		  if ((Length = strlen (SelectedUsrs->List[Rol_UNK])) <
		      Usr_MAX_BYTES_LIST_ENCRYPTED_USR_CODS)
		    {
		     SelectedUsrs->List[Rol_UNK][Length    ] = Par_SEPARATOR_PARAM_MULTIPLE[0];
		     SelectedUsrs->List[Rol_UNK][Length + 1] = '\0';
		    }
	       Str_Concat (SelectedUsrs->List[Rol_UNK],SelectedUsrs->List[Role],
			   Usr_MAX_BYTES_LIST_ENCRYPTED_USR_CODS);
	      }
	   }

      /***** List is filled *****/
      SelectedUsrs->Filled = true;
     }
  }

static void Usr_BuildParamName (char **ParamName,
				const char *ParamRoot,
				const char *ParamSuffix)
  {
   /* Build name of the parameter.
      Sometimes a unique action needs several distinct lists of users,
      so, it's necessary to use distinct names for the parameters. */
   if (ParamSuffix)
     {
      if (asprintf (ParamName,"%s%s",ParamRoot,ParamSuffix) < 0)
	 Err_NotEnoughMemoryExit ();
     }
   else
     {
      if (asprintf (ParamName,"%s",ParamRoot) < 0)
	 Err_NotEnoughMemoryExit ();
     }
  }

/*****************************************************************************/
/*** Get list of recipients of a message written explicitely by the sender ***/
/*****************************************************************************/
// Returns true if no errors

bool Usr_GetListMsgRecipientsWrittenExplicitelyBySender (bool WriteErrorMsgs)
  {
   extern const char *Par_SEPARATOR_PARAM_MULTIPLE;
   extern const char *Txt_There_is_no_user_with_nickname_X;
   extern const char *Txt_There_is_no_user_with_email_X;
   extern const char *Txt_There_are_more_than_one_user_with_the_ID_X_Please_type_a_nick_or_email;
   extern const char *Txt_There_is_no_user_with_ID_nick_or_email_X;
   extern const char *Txt_The_ID_nickname_or_email_X_is_not_valid;
   size_t LengthSelectedUsrsCods;
   size_t LengthUsrCod;
   const char *Ptr;
   char UsrIDNickOrEmail[Cns_MAX_BYTES_EMAIL_ADDRESS + 1];
   struct UsrData UsrDat;
   struct ListUsrCods ListUsrCods;
   bool Error = false;

   /***** Get list of selected encrypted users's codes if not already got.
          This list is necessary to add encrypted user's codes at the end. *****/
   Usr_GetListsSelectedEncryptedUsrsCods (&Gbl.Usrs.Selected);
   LengthSelectedUsrsCods = strlen (Gbl.Usrs.Selected.List[Rol_UNK]);

   /***** Allocate memory for the lists of recipients written explicetely *****/
   Usr_AllocateListOtherRecipients ();

   /***** Get recipients written explicetely *****/
   Par_GetParToText ("OtherRecipients",Gbl.Usrs.ListOtherRecipients,
                     Nck_MAX_BYTES_LIST_NICKS);

   /***** Add encrypted users' IDs to the list with all selected users *****/
   if (Gbl.Usrs.ListOtherRecipients[0])
     {
      /* Initialize structure with user's data */
      Usr_UsrDataConstructor (&UsrDat);

      /* Get next plain user's ID or nickname */
      Ptr = Gbl.Usrs.ListOtherRecipients;
      while (*Ptr)
        {
         /* Find next string in text until comma or semicolon (leading and trailing spaces are removed) */
         Str_GetNextStringUntilComma (&Ptr,UsrIDNickOrEmail,Cns_MAX_BYTES_EMAIL_ADDRESS);

         /* Check if string is plain user's ID or nickname and get encrypted user's ID */
         if (UsrIDNickOrEmail[0])
           {
	    /***** Reset default list of users' codes *****/
	    ListUsrCods.NumUsrs = 0;
	    ListUsrCods.Lst = NULL;

	    if (Nck_CheckIfNickWithArrIsValid (UsrIDNickOrEmail))	// 1: It's a nickname
	      {
	       if ((UsrDat.UsrCod = Nck_GetUsrCodFromNickname (UsrIDNickOrEmail)) > 0)
		 {
		  ListUsrCods.NumUsrs = 1;
		  Usr_AllocateListUsrCods (&ListUsrCods);
		  ListUsrCods.Lst[0] = UsrDat.UsrCod;
		 }
	       else
		 {
		  if (WriteErrorMsgs)
		     Ale_ShowAlert (Ale_WARNING,Txt_There_is_no_user_with_nickname_X,
			            UsrIDNickOrEmail);
		  Error = true;
		 }
	      }
	    else if (Mai_CheckIfEmailIsValid (UsrIDNickOrEmail))	// 2: It's an email
	      {
	       if ((UsrDat.UsrCod = Mai_GetUsrCodFromEmail (UsrIDNickOrEmail)) > 0)
		 {
		  ListUsrCods.NumUsrs = 1;
		  Usr_AllocateListUsrCods (&ListUsrCods);
		  ListUsrCods.Lst[0] = Gbl.Usrs.Other.UsrDat.UsrCod;
		 }
	       else
		 {
		  if (WriteErrorMsgs)
		     Ale_ShowAlert (Ale_WARNING,Txt_There_is_no_user_with_email_X,
			            UsrIDNickOrEmail);
		  Error = true;
		 }
	      }
            else							// 3: It's not a nickname nor email
              {
               // Users' IDs are always stored internally in capitals and without leading zeros
	       Str_RemoveLeadingZeros (UsrIDNickOrEmail);
	       Str_ConvertToUpperText (UsrIDNickOrEmail);
               if (ID_CheckIfUsrIDIsValid (UsrIDNickOrEmail))
		 {
		  // It seems a user's ID
		  /***** Allocate space for the list *****/
		  ID_ReallocateListIDs (&UsrDat,1);

		  Str_Copy (UsrDat.IDs.List[0].ID,UsrIDNickOrEmail,
		            sizeof (UsrDat.IDs.List[0].ID) - 1);

		  /***** Check if a user exists having this user's ID *****/
		  if (ID_GetListUsrCodsFromUsrID (&UsrDat,NULL,&ListUsrCods,false))
		    {
		     if (ListUsrCods.NumUsrs > 1)	// Two or more user share the same user's ID
		       {// TODO: Consider forbid IDs here
			if (WriteErrorMsgs)
			   Ale_ShowAlert (Ale_ERROR,Txt_There_are_more_than_one_user_with_the_ID_X_Please_type_a_nick_or_email,
				          UsrIDNickOrEmail);
			Error = true;
		       }
		    }
		  else	// No users found
		    {
		     if (WriteErrorMsgs)
			Ale_ShowAlert (Ale_ERROR,Txt_There_is_no_user_with_ID_nick_or_email_X,
				       UsrIDNickOrEmail);
		     Error = true;
		    }
		 }
	       else	// String is not a valid user's nickname, email or ID
		 {
		  if (WriteErrorMsgs)
		     Ale_ShowAlert (Ale_WARNING,Txt_The_ID_nickname_or_email_X_is_not_valid,
			            UsrIDNickOrEmail);
		  Error = true;
		 }
              }

            if (ListUsrCods.NumUsrs == 1)	// Only if user is valid
              {
               /* Get user's data */
	       Usr_GetUsrDataFromUsrCod (&UsrDat,	// Really only EncryptedUsrCod is needed
	                                 Usr_DONT_GET_PREFS,
	                                 Usr_DONT_GET_ROLE_IN_CURRENT_CRS);

               /* Find if encrypted user's code is already in list */
               if (!Usr_FindEncryptedUsrCodsInListOfSelectedEncryptedUsrCods (UsrDat.EnUsrCod,&Gbl.Usrs.Selected))        // If not in list ==> add it
                 {
                  LengthUsrCod = strlen (UsrDat.EnUsrCod);

                  /* Add encrypted user's code to list of users */
                  if (LengthSelectedUsrsCods == 0)	// First user in list
                    {
                     if (LengthUsrCod < Usr_MAX_BYTES_LIST_ENCRYPTED_USR_CODS)
                       {
                        /* Add user */
                        Str_Copy (Gbl.Usrs.Selected.List[Rol_UNK],
                                  UsrDat.EnUsrCod,
                                  Usr_MAX_BYTES_LIST_ENCRYPTED_USR_CODS);
                        LengthSelectedUsrsCods = LengthUsrCod;
                       }
                    }
                  else					// Not first user in list
                    {
                     if (LengthSelectedUsrsCods + (1 + LengthUsrCod) <
                	 Usr_MAX_BYTES_LIST_ENCRYPTED_USR_CODS)
                       {
                        /* Add separator */
                        Gbl.Usrs.Selected.List[Rol_UNK][LengthSelectedUsrsCods] = Par_SEPARATOR_PARAM_MULTIPLE[0];
                        LengthSelectedUsrsCods++;

                        /* Add user */
                        Str_Copy (Gbl.Usrs.Selected.List[Rol_UNK] + LengthSelectedUsrsCods,
                                  UsrDat.EnUsrCod,
                                  Usr_MAX_BYTES_LIST_ENCRYPTED_USR_CODS);
                        LengthSelectedUsrsCods += LengthUsrCod;
                       }
                    }
                 }
              }

	    /***** Free list of users' codes *****/
	    Usr_FreeListUsrCods (&ListUsrCods);
           }
        }

      /* Free memory used for user's data */
      Usr_UsrDataDestructor (&UsrDat);
     }
   return Error;
  }

/*****************************************************************************/
/************** Find if encrypted user's code is yet in list *****************/
/*****************************************************************************/
// Returns true if EncryptedUsrCodToFind is in list

bool Usr_FindEncryptedUsrCodsInListOfSelectedEncryptedUsrCods (const char *EncryptedUsrCodToFind,
							       struct SelectedUsrs *SelectedUsrs)
  {
   const char *Ptr;
   char EncryptedUsrCod[Cry_BYTES_ENCRYPTED_STR_SHA256_BASE64 + 1];

   if (SelectedUsrs->List[Rol_UNK])
     {
      Ptr = SelectedUsrs->List[Rol_UNK];
      while (*Ptr)
	{
	 Par_GetNextStrUntilSeparParamMult (&Ptr,EncryptedUsrCod,
	                                    Cry_BYTES_ENCRYPTED_STR_SHA256_BASE64);
	 if (!strcmp (EncryptedUsrCodToFind,EncryptedUsrCod))
	    return true;        // Found!
	}
     }
   return false;        // List not allocated or user not found
  }

/*****************************************************************************/
/******* Check if there are valid users in list of encrypted user codes ******/
/*****************************************************************************/

bool Usr_CheckIfThereAreUsrsInListOfSelectedEncryptedUsrCods (struct SelectedUsrs *SelectedUsrs)
  {
   const char *Ptr;
   struct UsrData UsrDat;

   /***** Loop over the list to check if there are valid users *****/
   Ptr = SelectedUsrs->List[Rol_UNK];
   while (*Ptr)
     {
      Par_GetNextStrUntilSeparParamMult (&Ptr,UsrDat.EnUsrCod,
                                         Cry_BYTES_ENCRYPTED_STR_SHA256_BASE64);
      Usr_GetUsrCodFromEncryptedUsrCod (&UsrDat);
      if (UsrDat.UsrCod > 0)
         return true;
     }
   return false;
  }

/*****************************************************************************/
/******** Count number of valid users in list of encrypted user codes ********/
/*****************************************************************************/

unsigned Usr_CountNumUsrsInListOfSelectedEncryptedUsrCods (struct SelectedUsrs *SelectedUsrs)
  {
   const char *Ptr;
   unsigned NumUsrs = 0;
   struct UsrData UsrDat;

   /***** Loop over the list to count the number of users *****/
   Ptr = SelectedUsrs->List[Rol_UNK];
   while (*Ptr)
     {
      Par_GetNextStrUntilSeparParamMult (&Ptr,UsrDat.EnUsrCod,
                                         Cry_BYTES_ENCRYPTED_STR_SHA256_BASE64);
      Usr_GetUsrCodFromEncryptedUsrCod (&UsrDat);
      if (UsrDat.UsrCod > 0)
         NumUsrs++;
     }
   return NumUsrs;
  }

/*****************************************************************************/
/****************** Allocate memory for list of students *********************/
/*****************************************************************************/
// Role = Rol_UNK here means all users

static void Usr_AllocateListSelectedEncryptedUsrCods (struct SelectedUsrs *SelectedUsrs,
						      Rol_Role_t Role)
  {
   if (!SelectedUsrs->List[Role])
     {
      if ((SelectedUsrs->List[Role] =
	   malloc (Usr_MAX_BYTES_LIST_ENCRYPTED_USR_CODS + 1)) == NULL)
         Err_NotEnoughMemoryExit ();
      SelectedUsrs->List[Role][0] = '\0';
     }
  }

/*****************************************************************************/
/******* Free memory used by lists of selected encrypted users' codes ********/
/*****************************************************************************/
// Role = Rol_UNK here means all users

void Usr_FreeListsSelectedEncryptedUsrsCods (struct SelectedUsrs *SelectedUsrs)
  {
   Rol_Role_t Role;

   if (SelectedUsrs->Filled)	// Only if lists are filled
     {
      /***** Free lists *****/
      for (Role  = (Rol_Role_t) 0;
	   Role <= (Rol_Role_t) (Rol_NUM_ROLES - 1);
	   Role++)
	 if (SelectedUsrs->List[Role])
	   {
	    free (SelectedUsrs->List[Role]);
	    SelectedUsrs->List[Role] = NULL;
	   }

      /***** Mark lists as empty *****/
      SelectedUsrs->Filled = false;
      // Lists of encrypted codes of users selected from form
      // are now marked as not filled
     }
  }

/*****************************************************************************/
/************* Get list of users selected to show their projects *************/
/*****************************************************************************/

void Usr_GetListSelectedUsrCods (struct SelectedUsrs *SelectedUsrs,
				 unsigned NumUsrsInList,
				 long **LstSelectedUsrCods)
  {
   unsigned NumUsr;
   const char *Ptr;
   struct UsrData UsrDat;

   /***** Create list of user codes *****/
   if ((*LstSelectedUsrCods = calloc (NumUsrsInList,
                                      sizeof (**LstSelectedUsrCods))) == NULL)
      Err_NotEnoughMemoryExit ();

   /***** Initialize structure with user's data *****/
   Usr_UsrDataConstructor (&UsrDat);

   /***** Loop over the list getting users' codes *****/
   for (NumUsr = 0, Ptr = SelectedUsrs->List[Rol_UNK];
	NumUsr < NumUsrsInList && *Ptr;
	NumUsr++)
     {
      Par_GetNextStrUntilSeparParamMult (&Ptr,UsrDat.EnUsrCod,
                                         Cry_BYTES_ENCRYPTED_STR_SHA256_BASE64);
      Usr_GetUsrCodFromEncryptedUsrCod (&UsrDat);
      (*LstSelectedUsrCods)[NumUsr] = UsrDat.UsrCod;
     }

   /***** Free memory used for user's data *****/
   Usr_UsrDataDestructor (&UsrDat);
  }

void Usr_FreeListSelectedUsrCods (long *LstSelectedUsrCods)
  {
   if (LstSelectedUsrCods)
      free (LstSelectedUsrCods);
  }

/*****************************************************************************/
/******** Create subquery string with users' codes separated by commas *******/
/******** from list of users' codes                                    *******/
/*****************************************************************************/

void Usr_CreateSubqueryUsrCods (long LstSelectedUsrCods[],
				unsigned NumUsrsInList,
				char **SubQueryUsrs)
  {
   char SubQueryOneUsr[1 + Cns_MAX_DECIMAL_DIGITS_LONG + 1];
   unsigned NumUsr;
   size_t MaxLength;

   /***** Allocate space for subquery *****/
   MaxLength = NumUsrsInList * (1 + Cns_MAX_DECIMAL_DIGITS_LONG);
   if ((*SubQueryUsrs = malloc (MaxLength + 1)) == NULL)
      Err_NotEnoughMemoryExit ();
   (*SubQueryUsrs)[0] = '\0';

   /***** Build subquery *****/
   for (NumUsr = 0;
	NumUsr < NumUsrsInList;
	NumUsr++)
      if (NumUsr)
	{
	 snprintf (SubQueryOneUsr,sizeof (SubQueryOneUsr),",%ld",
		   LstSelectedUsrCods[NumUsr]);
	 Str_Concat (*SubQueryUsrs,SubQueryOneUsr,MaxLength);
	}
      else
	 snprintf (*SubQueryUsrs,sizeof (SubQueryOneUsr),"%ld",
		   LstSelectedUsrCods[NumUsr]);
  }

void Usr_FreeSubqueryUsrCods (char *SubQueryUsrs)
  {
   free (SubQueryUsrs);
  }

/*****************************************************************************/
/********** Allocate memory for list of users's IDs or nicknames *************/
/*****************************************************************************/

static void Usr_AllocateListOtherRecipients (void)
  {
   if (!Gbl.Usrs.ListOtherRecipients)
     {
      if ((Gbl.Usrs.ListOtherRecipients = malloc (Nck_MAX_BYTES_LIST_NICKS + 1)) == NULL)
         Err_NotEnoughMemoryExit ();
      Gbl.Usrs.ListOtherRecipients[0] = '\0';
     }
  }

/*****************************************************************************/
/****************** Free memory for the list of nicknames ********************/
/*****************************************************************************/

void Usr_FreeListOtherRecipients (void)
  {
   if (Gbl.Usrs.ListOtherRecipients)
     {
      free (Gbl.Usrs.ListOtherRecipients);
      Gbl.Usrs.ListOtherRecipients = NULL;
     }
  }

/*****************************************************************************/
/*************************** Selection of list type **************************/
/*****************************************************************************/

void Usr_ShowFormsToSelectUsrListType (void (*FuncParams) (void *Args),void *Args)
  {
   Set_BeginSettingsHead ();
   Set_BeginOneSettingSelector ();

   /***** Select Usr_LIST_AS_CLASS_PHOTO *****/
   HTM_DIV_Begin ("class=\"%s\"",
		  Gbl.Usrs.Me.ListType == Usr_LIST_AS_CLASS_PHOTO ? "PREF_ON" :
								    "PREF_OFF");
   Usr_FormToSelectUsrListType (FuncParams,Args,
                                Usr_LIST_AS_CLASS_PHOTO);

   /* Number of columns in the class photo */
   Frm_BeginFormAnchor (Gbl.Action.Act,			// Repeat current action
			Usr_USER_LIST_SECTION_ID);
   Grp_PutParamsCodGrps ();
   Usr_PutParamUsrListType (Usr_LIST_AS_CLASS_PHOTO);
   Usr_PutParamListWithPhotos ();
   Usr_PutSelectorNumColsClassPhoto ();
   if (FuncParams)
      FuncParams (Args);
   Frm_EndForm ();
   HTM_DIV_End ();

   /***** Select Usr_LIST_AS_LISTING *****/
   HTM_DIV_Begin ("class=\"%s\"",
		  Gbl.Usrs.Me.ListType == Usr_LIST_AS_LISTING ? "PREF_ON" :
								"PREF_OFF");
   Usr_FormToSelectUsrListType (FuncParams,Args,
                                Usr_LIST_AS_LISTING);

   /* See the photos in list? */
   Frm_BeginFormAnchor (Gbl.Action.Act,			// Repeat current action
			Usr_USER_LIST_SECTION_ID);
   Grp_PutParamsCodGrps ();
   Usr_PutParamUsrListType (Usr_LIST_AS_LISTING);
   if (FuncParams)
      FuncParams (Args);
   Usr_PutCheckboxListWithPhotos ();
   Frm_EndForm ();
   HTM_DIV_End ();

   Set_EndOneSettingSelector ();
   Set_EndSettingsHead ();
  }

/*****************************************************************************/
/************* Put a radio element to select a users' list type **************/
/*****************************************************************************/

static void Usr_FormToSelectUsrListType (void (*FuncParams) (void *Args),void *Args,
                                         Usr_ShowUsrsType_t ListType)
  {
   extern const char *The_ClassFormLinkInBoxNoWrap[The_NUM_THEMES];
   extern const char *Txt_USR_LIST_TYPES[Usr_NUM_USR_LIST_TYPES];

   /***** Begin form *****/
   Frm_BeginFormAnchor (Gbl.Action.Act,			// Repeat current action
	                Usr_USER_LIST_SECTION_ID);
   Grp_PutParamsCodGrps ();
   Usr_PutParamUsrListType (ListType);
   Usr_PutParamListWithPhotos ();
   if (FuncParams)
      FuncParams (Args);

   /***** Link and image *****/
   HTM_BUTTON_SUBMIT_Begin (Txt_USR_LIST_TYPES[ListType],
			    The_ClassFormLinkInBoxNoWrap[Gbl.Prefs.Theme],
			    Gbl.Action.Act == ActReqMsgUsr ? "CopyMessageToHiddenFields();" :
							     NULL);
   Ico_PutIcon (Usr_IconsClassPhotoOrList[ListType],Txt_USR_LIST_TYPES[ListType],"ICO20x20");
   HTM_TxtF ("&nbsp;%s",Txt_USR_LIST_TYPES[ListType]);
   HTM_BUTTON_End ();

   /***** End form *****/
   Frm_EndForm ();
  }

/*****************************************************************************/
/******************** List users to select some of them **********************/
/*****************************************************************************/

void Usr_PutFormToSelectUsrsToGoToAct (struct SelectedUsrs *SelectedUsrs,
				       Act_Action_t NextAction,
				       void (*FuncParams) (void *Args),void *Args,
				       const char *Title,
                                       const char *HelpLink,
                                       const char *TxtButton,
				       bool PutFormDateRange)
  {
   extern const char *The_ClassFormInBox[The_NUM_THEMES];
   extern const char *Txt_Select_users;
   extern const char *Txt_Users;
   unsigned NumTotalUsrs;
   static const Dat_SetHMS SetHMS[Dat_NUM_START_END_TIME] =
     {
      [Dat_START_TIME] = Dat_HMS_DO_NOT_SET,
      [Dat_END_TIME  ] = Dat_HMS_DO_NOT_SET
     };

   /***** Begin box *****/
   Box_BoxBegin (NULL,Title,
                 NULL,NULL,
                 HelpLink,Box_NOT_CLOSABLE);

   /***** Get and update type of list,
          number of columns in class photo
          and preference about view photos *****/
   Usr_GetAndUpdatePrefsAboutUsrList ();

   /***** Get groups to show ******/
   Grp_GetParCodsSeveralGrpsToShowUsrs ();

   /***** Get and order lists of users from this course *****/
   Usr_GetListUsrs (HieLvl_CRS,Rol_STD);
   Usr_GetListUsrs (HieLvl_CRS,Rol_NET);
   Usr_GetListUsrs (HieLvl_CRS,Rol_TCH);
   NumTotalUsrs = Gbl.Usrs.LstUsrs[Rol_STD].NumUsrs +
	          Gbl.Usrs.LstUsrs[Rol_NET].NumUsrs +
	          Gbl.Usrs.LstUsrs[Rol_TCH].NumUsrs;

   /***** Draw class photos to select users *****/
   Box_BoxBegin (NULL,Txt_Select_users,
                 NULL,NULL,
                 HelpLink,Box_NOT_CLOSABLE);

   /***** Show form to select the groups *****/
   Grp_ShowFormToSelectSeveralGroups (FuncParams,Args,
                                      Grp_MY_GROUPS);

   /***** Begin section with user list *****/
   HTM_SECTION_Begin (Usr_USER_LIST_SECTION_ID);

   if (NumTotalUsrs)
     {
      if (Usr_GetIfShowBigList (NumTotalUsrs,
                                FuncParams,Args,
                                NULL))
        {
	 /***** Form to select type of list used for select several users *****/
	 Usr_ShowFormsToSelectUsrListType (FuncParams,Args);

	 /***** Link to register students *****/
	 Enr_CheckStdsAndPutButtonToRegisterStdsInCurrentCrs ();

	 /***** Form to select users and select date range ****/
         /* Begin form */
         Frm_BeginForm (NextAction);

         /* Hidden parameters */
         Grp_PutParamsCodGrps ();
         if (NextAction == ActAdmAsgWrkCrs)
           {
            Gbl.FileBrowser.FullTree = true;	// By default, show all files
            Brw_PutHiddenParamFullTreeIfSelected (&Gbl.FileBrowser.FullTree);
           }
         if (FuncParams)
            FuncParams (Args);

         HTM_TABLE_BeginCenterPadding (2);

         /* Put list of users to select some of them */
         HTM_TR_Begin (NULL);

         HTM_TD_Begin ("class=\"%s RT\"",The_ClassFormInBox[Gbl.Prefs.Theme]);
         HTM_TxtColon (Txt_Users);
         HTM_TD_End ();

	 HTM_TD_Begin ("class=\"%s LT\"",The_ClassFormInBox[Gbl.Prefs.Theme]);
         HTM_TABLE_BeginCenterPadding (2);
         Usr_ListUsersToSelect (Rol_TCH,SelectedUsrs);
         Usr_ListUsersToSelect (Rol_NET,SelectedUsrs);
         Usr_ListUsersToSelect (Rol_STD,SelectedUsrs);
         HTM_TABLE_End ();
         HTM_TD_End ();

         HTM_TR_End ();

         /* Starting and ending dates in the search */
         if (PutFormDateRange)
            Dat_PutFormStartEndClientLocalDateTimesWithYesterdayToday (SetHMS);

         HTM_TABLE_End ();

         /***** Send button *****/
	 Btn_PutConfirmButton (TxtButton);

         /***** End form *****/
         Frm_EndForm ();
        }
     }
   else	// NumTotalUsrs == 0
      /***** Show warning indicating no users found *****/
      Usr_ShowWarningNoUsersFound (Rol_UNK);

   /***** End section with user list *****/
   HTM_SECTION_End ();

   /***** End box *****/
   Box_BoxEnd ();

   /***** Free memory for users' list *****/
   Usr_FreeUsrsList (Rol_TCH);
   Usr_FreeUsrsList (Rol_NET);
   Usr_FreeUsrsList (Rol_STD);

   /***** Free memory for list of selected groups *****/
   Grp_FreeListCodSelectedGrps ();

   /***** End box *****/
   Box_BoxEnd ();
  }

void Usr_GetSelectedUsrsAndGoToAct (struct SelectedUsrs *SelectedUsrs,
				    void (*FuncWhenUsrsSelected) (void *ArgsSelected),void *ArgsSelected,
                                    void (*FuncWhenNoUsrsSelected) (void *ArgsNoSelected),void *ArgsNoSelected)
  {
   extern const char *Txt_You_must_select_one_ore_more_users;

   /***** Get lists of the selected users if not already got *****/
   Usr_GetListsSelectedEncryptedUsrsCods (SelectedUsrs);

   /***** Check number of users *****/
   if (Usr_CheckIfThereAreUsrsInListOfSelectedEncryptedUsrCods (SelectedUsrs))	// If some users are selected...
      FuncWhenUsrsSelected (ArgsSelected);
   else	// If no users are selected...
     {
      // ...write warning alert
      Ale_ShowAlert (Ale_WARNING,Txt_You_must_select_one_ore_more_users);
      // ...and show again the form
      FuncWhenNoUsrsSelected (ArgsNoSelected);
     }

   /***** Free memory used by list of selected users' codes *****/
   Usr_FreeListsSelectedEncryptedUsrsCods (SelectedUsrs);
  }

/*****************************************************************************/
/*********** List users with a given role to select some of them *************/
/*****************************************************************************/

void Usr_ListUsersToSelect (Rol_Role_t Role,struct SelectedUsrs *SelectedUsrs)
  {
   /***** If there are no users, don't list anything *****/
   if (!Gbl.Usrs.LstUsrs[Role].NumUsrs)
      return;

   /***** Draw the classphoto/list *****/
   switch (Gbl.Usrs.Me.ListType)
     {
      case Usr_LIST_AS_CLASS_PHOTO:
         Usr_DrawClassPhoto (Usr_CLASS_PHOTO_SEL,
                             Role,SelectedUsrs,true);
         break;
      case Usr_LIST_AS_LISTING:
         Usr_ListUsrsForSelection (Role,SelectedUsrs);
         break;
      default:
	 break;
     }
  }

/*****************************************************************************/
/******** Put a row, in a classphoto or a list, to select all users **********/
/*****************************************************************************/

static void Usr_PutCheckboxToSelectAllUsers (Rol_Role_t Role,
			                     struct SelectedUsrs *SelectedUsrs)
  {
   extern const char *Txt_ROLES_SINGUL_Abc[Rol_NUM_ROLES][Usr_NUM_SEXS];
   extern const char *Txt_ROLES_PLURAL_Abc[Rol_NUM_ROLES][Usr_NUM_SEXS];
   char *ParamName;
   Usr_Sex_t Sex;

   HTM_TR_Begin (NULL);

   HTM_TH_Begin (1,Usr_GetColumnsForSelectUsrs (),"LM LIGHT_BLUE");

   HTM_LABEL_Begin (NULL);
   if (Usr_NameSelUnsel[Role] && Usr_ParamUsrCod[Role])
     {
      Usr_BuildParamName (&ParamName,Usr_ParamUsrCod[Role],SelectedUsrs->ParamSuffix);
      HTM_INPUT_CHECKBOX (Usr_NameSelUnsel[Role],HTM_DONT_SUBMIT_ON_CHANGE,
			  "value=\"\" onclick=\"togglecheckChildren(this,'%s')\"",
			  ParamName);
      free (ParamName);
     }
   else
      Err_WrongRoleExit ();
   Sex = Usr_GetSexOfUsrsLst (Role);
   HTM_TxtColon (Gbl.Usrs.LstUsrs[Role].NumUsrs == 1 ? Txt_ROLES_SINGUL_Abc[Role][Sex] :
                                                       Txt_ROLES_PLURAL_Abc[Role][Sex]);
   HTM_LABEL_End ();

   HTM_TH_End ();

   HTM_TR_End ();
  }

/*****************************************************************************/
/************************** Get sex of a list of users ***********************/
/*****************************************************************************/

static Usr_Sex_t Usr_GetSexOfUsrsLst (Rol_Role_t Role)
  {
   Usr_Sex_t Sex;
   unsigned NumUsr;

   /***** If no users, sex is undefined *****/
   if (Gbl.Usrs.LstUsrs[Role].NumUsrs == 0)
      return Usr_SEX_UNKNOWN;

   /***** Initialize sex to that of first user in list *****/
   Sex = Gbl.Usrs.LstUsrs[Role].Lst[0].Sex;

   /***** Search if there is one user at least with different sex than the first one *****/
   for (NumUsr = 1;
        NumUsr < Gbl.Usrs.LstUsrs[Role].NumUsrs;
        NumUsr++)
      if (Gbl.Usrs.LstUsrs[Role].Lst[NumUsr].Sex != Sex)
         return Usr_SEX_UNKNOWN;

   return Sex;
  }

/*****************************************************************************/
/**** Get number of table columns, in classphoto or list, to select users ****/
/*****************************************************************************/

unsigned Usr_GetColumnsForSelectUsrs (void)
  {
   return (Gbl.Usrs.Me.ListType == Usr_LIST_AS_CLASS_PHOTO) ? Gbl.Usrs.ClassPhoto.Cols :
                                                             (Gbl.Usrs.Listing.WithPhotos ? 1 + Usr_NUM_MAIN_FIELDS_DATA_USR :
                                                                                            Usr_NUM_MAIN_FIELDS_DATA_USR);
  }

/*****************************************************************************/
/******* Put a checkbox, in a classphoto or a list, to select a user *********/
/*****************************************************************************/

static void Usr_PutCheckboxToSelectUser (Rol_Role_t Role,
                                         const char *EncryptedUsrCod,
                                         bool UsrIsTheMsgSender,
					 struct SelectedUsrs *SelectedUsrs)
  {
   bool CheckboxChecked;
   char *ParamName;

   if (Usr_NameSelUnsel[Role] && Usr_ParamUsrCod[Role])
     {
      /***** Check box must be checked? *****/
      if (UsrIsTheMsgSender)
	 CheckboxChecked = true;
      else
	 /* Check if user is in lists of selected users */
	 CheckboxChecked = Usr_FindEncryptedUsrCodsInListOfSelectedEncryptedUsrCods (EncryptedUsrCod,SelectedUsrs);

      /***** Check box *****/
      Usr_BuildParamName (&ParamName,Usr_ParamUsrCod[Role],SelectedUsrs->ParamSuffix);
      HTM_INPUT_CHECKBOX (ParamName,HTM_DONT_SUBMIT_ON_CHANGE,
			  "value=\"%s\"%s onclick=\"checkParent(this,'%s')\"",
			  EncryptedUsrCod,
			  CheckboxChecked ? " checked=\"checked\"" :
				            "",
			  Usr_NameSelUnsel[Role]);
      free (ParamName);
     }
   else
      Err_WrongRoleExit ();
  }

/*****************************************************************************/
/********* Put a checkbox to select whether list users with photos ***********/
/*****************************************************************************/

static void Usr_PutCheckboxListWithPhotos (void)
  {
   extern const char *The_ClassFormInBox[The_NUM_THEMES];
   extern const char *Txt_Display_photos;

   Par_PutHiddenParamChar ("WithPhotosExists",'Y');

   /***** Put checkbox to select whether list users with photos *****/
   HTM_LABEL_Begin ("class=\"%s\"",The_ClassFormInBox[Gbl.Prefs.Theme]);
   HTM_INPUT_CHECKBOX ("WithPhotos",HTM_SUBMIT_ON_CHANGE,
		       "value=\"Y\"%s",
		       Gbl.Usrs.Listing.WithPhotos ? " checked=\"checked\"" :
			                             "");
   HTM_Txt (Txt_Display_photos);
   HTM_LABEL_End ();
  }

/*****************************************************************************/
/*********************** Set field names of user's data **********************/
/*****************************************************************************/

void Usr_SetUsrDatMainFieldNames (void)
  {
   extern const char *Txt_No_INDEX;
   extern const char *Txt_Photo;
   extern const char *Txt_ID;
   extern const char *Txt_Surname_1;
   extern const char *Txt_Surname_2;
   extern const char *Txt_First_name;
   extern const char *Txt_Institution;

   /***** Initialize field names *****/
   Usr_UsrDatMainFieldNames[0] = "&nbsp;";
   Usr_UsrDatMainFieldNames[1] = Txt_No_INDEX;
   Usr_UsrDatMainFieldNames[2] = Txt_Photo;
   Usr_UsrDatMainFieldNames[3] = Txt_ID;
   Usr_UsrDatMainFieldNames[4] = Txt_Surname_1;
   Usr_UsrDatMainFieldNames[5] = Txt_Surname_2;
   Usr_UsrDatMainFieldNames[6] = Txt_First_name;
   Usr_UsrDatMainFieldNames[7] = Txt_Institution;
  }

/*****************************************************************************/
/************ Write header with main field names of user's data **************/
/*****************************************************************************/

void Usr_WriteHeaderFieldsUsrDat (bool PutCheckBoxToSelectUsr)
  {
   unsigned NumCol;

   HTM_TR_Begin (NULL);

   /***** First column used for selection *****/
   if (PutCheckBoxToSelectUsr)
      HTM_TH (1,1,"LM LIGHT_BLUE",NULL);

   /***** Columns for user's data fields *****/
   for (NumCol = 0;
        NumCol < Usr_NUM_MAIN_FIELDS_DATA_USR;
        NumCol++)
      if (NumCol != 2 || Gbl.Usrs.Listing.WithPhotos)        // Skip photo column if I don't want this column
         HTM_TH (1,1,"LM LIGHT_BLUE",Usr_UsrDatMainFieldNames[NumCol]);

   HTM_TR_End ();
  }

/*****************************************************************************/
/************************** List guests' main data ***************************/
/*****************************************************************************/

static void Usr_ListMainDataGsts (bool PutCheckBoxToSelectUsr)
  {
   unsigned NumUsr;
   struct UsrData UsrDat;

   if (Gbl.Usrs.LstUsrs[Rol_GST].NumUsrs)
     {
      /***** Initialize field names *****/
      Usr_SetUsrDatMainFieldNames ();

      /***** Put a row to select all users *****/
      if (PutCheckBoxToSelectUsr)
         Usr_PutCheckboxToSelectAllUsers (Rol_GST,&Gbl.Usrs.Selected);

      /***** Heading row with column names *****/
      Usr_WriteHeaderFieldsUsrDat (PutCheckBoxToSelectUsr);	// Columns for the data

      /***** Initialize structure with user's data *****/
      Usr_UsrDataConstructor (&UsrDat);

      /***** List guests' data *****/
      for (NumUsr = 0, Gbl.RowEvenOdd = 0;
           NumUsr < Gbl.Usrs.LstUsrs[Rol_GST].NumUsrs;
           NumUsr++, Gbl.RowEvenOdd = 1 - Gbl.RowEvenOdd)
        {
	 /* Copy user's basic data from list */
         Usr_CopyBasicUsrDataFromList (&UsrDat,&Gbl.Usrs.LstUsrs[Rol_GST].Lst[NumUsr]);

	 /* Get list of user's IDs */
         ID_GetListIDsFromUsrCod (&UsrDat);

         /* Show row for this guest */
	 Usr_WriteRowUsrMainData (NumUsr + 1,&UsrDat,true,Rol_GST,
				  &Gbl.Usrs.Selected);
        }

      /***** Free memory used for user's data *****/
      Usr_UsrDataDestructor (&UsrDat);
     }
   else        // Gbl.Usrs.LstUsrs[Rol_GST].NumUsrs == 0
      /***** Show warning indicating no guests found *****/
      Usr_ShowWarningNoUsersFound (Rol_GST);

   /***** Free memory for guests list *****/
   Usr_FreeUsrsList (Rol_GST);
  }

/*****************************************************************************/
/*************************** List main students' data ************************/
/*****************************************************************************/

static void Usr_ListMainDataStds (bool PutCheckBoxToSelectUsr)
  {
   unsigned NumUsr;
   char *GroupNames;
   struct UsrData UsrDat;

   /***** Initialize field names *****/
   Usr_SetUsrDatMainFieldNames ();

   GroupNames = NULL;        // To avoid warning

   if (Gbl.Usrs.LstUsrs[Rol_STD].NumUsrs)
     {
      /***** Allocate memory for the string with the list of group names where student belongs to *****/
      if ((GroupNames = malloc (Gbl.Crs.Grps.GrpTypes.NumGrpsTotal *
                                (Grp_MAX_BYTES_GROUP_NAME + 3))) == NULL)
         Err_NotEnoughMemoryExit ();

      /***** Begin table with list of students *****/
      if (!Gbl.Usrs.ClassPhoto.AllGroups)
        {
         HTM_TR_Begin (NULL);
         HTM_TD_Begin ("colspan=\"%u\" class=\"TIT CM\"",
		       1 + Usr_NUM_MAIN_FIELDS_DATA_USR);
         Grp_WriteNamesOfSelectedGrps ();
         HTM_TD_End ();
         HTM_TR_End ();
        }

      /***** Put a row to select all users *****/
      if (PutCheckBoxToSelectUsr)
	 Usr_PutCheckboxToSelectAllUsers (Rol_STD,&Gbl.Usrs.Selected);

      /***** Heading row with column names *****/
      Usr_WriteHeaderFieldsUsrDat (PutCheckBoxToSelectUsr);	// Columns for the data

      /***** Initialize structure with user's data *****/
      Usr_UsrDataConstructor (&UsrDat);

      /***** List students' data *****/
      for (NumUsr = 0, Gbl.RowEvenOdd = 0;
           NumUsr < Gbl.Usrs.LstUsrs[Rol_STD].NumUsrs;
           NumUsr++, Gbl.RowEvenOdd = 1 - Gbl.RowEvenOdd)
        {
	 /* Copy user's basic data from list */
         Usr_CopyBasicUsrDataFromList (&UsrDat,&Gbl.Usrs.LstUsrs[Rol_STD].Lst[NumUsr]);

	 /* Get list of user's IDs */
         ID_GetListIDsFromUsrCod (&UsrDat);

         /* Show row for this student */
         Usr_WriteRowUsrMainData (NumUsr + 1,&UsrDat,
                                  PutCheckBoxToSelectUsr,Rol_STD,
				  &Gbl.Usrs.Selected);
        }

      /***** Free memory used for user's data *****/
      Usr_UsrDataDestructor (&UsrDat);

      /***** Free memory used by the string with the list of group names where student belongs to *****/
      free (GroupNames);
     }
   else        // Gbl.Usrs.LstUsrs[Rol_STD].NumUsrs == 0
      /***** Show warning indicating no students found *****/
      Usr_ShowWarningNoUsersFound (Rol_STD);

   /***** Free memory for students list *****/
   Usr_FreeUsrsList (Rol_STD);
  }

/*****************************************************************************/
/************************** List main teachers' data *************************/
/*****************************************************************************/
// Role can be:
// - Rol_NET
// - Rol_TCH

static void Usr_ListMainDataTchs (Rol_Role_t Role,
				  bool PutCheckBoxToSelectUsr)
  {
   unsigned NumCol;
   unsigned NumUsr;
   struct UsrData UsrDat;

   if (Gbl.Usrs.LstUsrs[Role].NumUsrs)
     {
      /***** Put a row to select all users *****/
      if (PutCheckBoxToSelectUsr)
	 Usr_PutCheckboxToSelectAllUsers (Role,&Gbl.Usrs.Selected);

      /***** Heading row with column names *****/
      /* Begin row */
      HTM_TR_Begin (NULL);

	 /* First column used for selection  */
	 if (PutCheckBoxToSelectUsr)
	    HTM_TH (1,1,"LM LIGHT_BLUE",NULL);

	 /* Columns for the data */
	 for (NumCol = 0;
	      NumCol < Usr_NUM_MAIN_FIELDS_DATA_USR;
	      NumCol++)
	    if (NumCol != 2 || Gbl.Usrs.Listing.WithPhotos)        // Skip photo column if I don't want this column
	       HTM_TH (1,1,"LM LIGHT_BLUE",Usr_UsrDatMainFieldNames[NumCol]);

      /* End row */
      HTM_TR_End ();

      /***** Initialize structure with user's data *****/
      Usr_UsrDataConstructor (&UsrDat);

      /***** List teachers' data *****/
      for (NumUsr = 0, Gbl.RowEvenOdd = 0;
           NumUsr < Gbl.Usrs.LstUsrs[Role].NumUsrs;
           NumUsr++, Gbl.RowEvenOdd = 1 - Gbl.RowEvenOdd)
        {
	 /* Copy user's basic data from list */
         Usr_CopyBasicUsrDataFromList (&UsrDat,&Gbl.Usrs.LstUsrs[Role].Lst[NumUsr]);

	 /* Get list of user's IDs */
         ID_GetListIDsFromUsrCod (&UsrDat);

         /* Show row for this teacher */
	 Usr_WriteRowUsrMainData (NumUsr + 1,&UsrDat,
	                          PutCheckBoxToSelectUsr,Role,
				  &Gbl.Usrs.Selected);
        }

      /***** Free memory used for user's data *****/
      Usr_UsrDataDestructor (&UsrDat);
     }
  }

/*****************************************************************************/
/***************************** List guests' data *****************************/
/*****************************************************************************/

void Usr_ListAllDataGsts (void)
  {
   extern const char *Txt_Photo;
   extern const char *Txt_ID;
   extern const char *Txt_Surname_1;
   extern const char *Txt_Surname_2;
   extern const char *Txt_First_name;
   extern const char *Txt_Institution;
   extern const char *Txt_Email;
   extern const char *Txt_Center;
   extern const char *Txt_Department;
   extern const char *Txt_Office;
   extern const char *Txt_Phone;
   extern const char *Txt_Date_of_birth;
   unsigned NumColumnsCommonCard;
   unsigned NumCol;
   unsigned NumUsr;
   struct UsrData UsrDat;
   const char *FieldNames[Usr_NUM_ALL_FIELDS_DATA_GST];

   /***** Initialize field names *****/
   FieldNames[ 0] = Txt_Photo;
   FieldNames[ 1] = Txt_ID;
   FieldNames[ 2] = Txt_Surname_1;
   FieldNames[ 3] = Txt_Surname_2;
   FieldNames[ 4] = Txt_First_name;
   FieldNames[ 5] = Txt_Email;
   FieldNames[ 6] = Txt_Institution;
   FieldNames[ 7] = Txt_Center;
   FieldNames[ 8] = Txt_Department;
   FieldNames[ 9] = Txt_Office;
   FieldNames[10] = Txt_Phone;
   FieldNames[11] = Txt_Phone;
   FieldNames[12] = Txt_Phone;
   FieldNames[13] = Txt_Date_of_birth;

   /***** Get and update type of list,
          number of columns in class photo
          and preference about viewing photos *****/
   Usr_GetAndUpdatePrefsAboutUsrList ();

   /***** Get scope *****/
   Sco_SetScopesForListingGuests ();
   Sco_GetScope ("ScopeUsr");

   /****** Get list of guests ******/
   Usr_GetGstsLst (Gbl.Scope.Current);

   if (Gbl.Usrs.LstUsrs[Rol_GST].NumUsrs)
     {
      /***** Set number of columns *****/
      NumColumnsCommonCard = Usr_NUM_ALL_FIELDS_DATA_GST;

      /***** Begin table with list of guests *****/
      HTM_TABLE_BeginWide ();

	 /* Begin row */
	 HTM_TR_Begin (NULL);

	    /* Columns for the data */
	    for (NumCol = (Gbl.Usrs.Listing.WithPhotos ? 0 :
							 1);
		 NumCol < NumColumnsCommonCard;
		 NumCol++)
	       HTM_TH (1,1,"LM LIGHT_BLUE",FieldNames[NumCol]);

	 /* End row */
	 HTM_TR_End ();

	 /***** Initialize structure with user's data *****/
	 Usr_UsrDataConstructor (&UsrDat);

	 /***** List guests' data *****/
	 for (NumUsr = 0, Gbl.RowEvenOdd = 0;
	      NumUsr < Gbl.Usrs.LstUsrs[Rol_GST].NumUsrs; )
	   {
	    UsrDat.UsrCod = Gbl.Usrs.LstUsrs[Rol_GST].Lst[NumUsr].UsrCod;
	    if (Usr_ChkUsrCodAndGetAllUsrDataFromUsrCod (&UsrDat,
							 Usr_DONT_GET_PREFS,
							 Usr_DONT_GET_ROLE_IN_CURRENT_CRS))
	      {
	       UsrDat.Accepted = false;	// Guests have no courses,...
					   // ...so they have not accepted...
					   // ...inscription in any course
	       NumUsr++;
	       Usr_WriteRowGstAllData (&UsrDat);

	       Gbl.RowEvenOdd = 1 - Gbl.RowEvenOdd;
	      }
	   }

	 /***** Free memory used for user's data *****/
	 Usr_UsrDataDestructor (&UsrDat);

      /***** End table *****/
      HTM_TABLE_End ();
     }
   else        // Gbl.Usrs.LstUsrs[Rol_GST].NumUsrs == 0
      /***** Show warning indicating no guests found *****/
      Usr_ShowWarningNoUsersFound (Rol_GST);

   /***** Free memory for guests list *****/
   Usr_FreeUsrsList (Rol_GST);
  }

/*****************************************************************************/
/***************************** List students' data ***************************/
/*****************************************************************************/

void Usr_ListAllDataStds (void)
  {
   extern const char *Txt_Photo;
   extern const char *Txt_ID;
   extern const char *Txt_Surname_1;
   extern const char *Txt_Surname_2;
   extern const char *Txt_First_name;
   extern const char *Txt_Institution;
   extern const char *Txt_Email;
   extern const char *Txt_Phone;
   extern const char *Txt_Date_of_birth;
   extern const char *Txt_Group;
   extern const char *Txt_RECORD_FIELD_VISIBILITY_RECORD[Rec_NUM_TYPES_VISIBILITY];
   unsigned NumColumnsCommonCard;
   unsigned NumColumnsCardAndGroups;
   unsigned NumColumnsTotal;
   unsigned NumCol;
   unsigned NumUsr;
   char *GroupNames;
   unsigned NumGrpTyp,NumField;
   struct UsrData UsrDat;
   const char *FieldNames[Usr_NUM_ALL_FIELDS_DATA_STD];
   size_t Length;

   /***** Initialize field names *****/
   FieldNames[0] = Txt_Photo;
   FieldNames[1] = Txt_ID;
   FieldNames[2] = Txt_Surname_1;
   FieldNames[3] = Txt_Surname_2;
   FieldNames[4] = Txt_First_name;
   FieldNames[5] = Txt_Email;
   FieldNames[6] = Txt_Institution;
   FieldNames[7] = Txt_Phone;
   FieldNames[8] = Txt_Phone;
   FieldNames[9] = Txt_Date_of_birth;

   GroupNames = NULL;        // To avoid warning

   /***** Get and update type of list,
          number of columns in class photo
          and preference about viewing photos *****/
   Usr_GetAndUpdatePrefsAboutUsrList ();

   /***** Get scope *****/
   Sco_SetScopesForListingStudents ();
   Sco_GetScope ("ScopeUsr");

   /***** If the scope is the current course... *****/
   if (Gbl.Scope.Current == HieLvl_CRS)
     {
      /* Get list of groups types and groups in current course
         This is necessary to show columns with group selection */
      Grp_GetListGrpTypesInThisCrs (Grp_ONLY_GROUP_TYPES_WITH_GROUPS);

      /* Get groups to show */
      Grp_GetParCodsSeveralGrpsToShowUsrs ();
     }

   /****** Get list of students in current course ******/
   Usr_GetListUsrs (Gbl.Scope.Current,Rol_STD);

   if (Gbl.Usrs.LstUsrs[Rol_STD].NumUsrs)
     {
      if (Gbl.Scope.Current == HieLvl_CRS)
         /***** Get list of record fields in current course *****/
         Rec_GetListRecordFieldsInCurrentCrs ();

      /***** Set number of columns *****/
      NumColumnsCommonCard = Usr_NUM_ALL_FIELDS_DATA_STD;
      if (Gbl.Scope.Current == HieLvl_CRS)
        {
         NumColumnsCardAndGroups = NumColumnsCommonCard + Gbl.Crs.Grps.GrpTypes.NumGrpTypes;
         NumColumnsTotal = NumColumnsCardAndGroups + Gbl.Crs.Records.LstFields.Num;
        }
      else
         NumColumnsTotal = NumColumnsCardAndGroups = NumColumnsCommonCard;

      /***** Allocate memory for the string with the list of group names where student belongs to *****/
      if (Gbl.Scope.Current == HieLvl_CRS)
	{
	 Length = (Grp_MAX_BYTES_GROUP_NAME + 2) * Gbl.Crs.Grps.GrpTypes.NumGrpsTotal;
         if ((GroupNames = malloc (Length + 1)) == NULL)
            Err_NotEnoughMemoryExit ();
	}

      /***** Begin table with list of students *****/
      HTM_TABLE_BeginWide ();
      if (!Gbl.Usrs.ClassPhoto.AllGroups)
        {
         HTM_TR_Begin (NULL);
         HTM_TD_Begin ("colspan=\"%u\" class=\"TIT CM\"",NumColumnsTotal);
         Grp_WriteNamesOfSelectedGrps ();
         HTM_TD_End ();
         HTM_TR_End ();
        }

      /***** Heading row with column names *****/
      /* Begin row */
      HTM_TR_Begin (NULL);

	 /* 1. Columns for the data */
	 for (NumCol = (Gbl.Usrs.Listing.WithPhotos ? 0 :
						      1);
	      NumCol < NumColumnsCommonCard;
	      NumCol++)
	    HTM_TH (1,1,"LM LIGHT_BLUE",FieldNames[NumCol]);

	 /* 2. Columns for the groups */
	 if (Gbl.Scope.Current == HieLvl_CRS)
	   {
	    if (Gbl.Crs.Grps.GrpTypes.NumGrpTypes)
	       for (NumGrpTyp = 0;
		    NumGrpTyp < Gbl.Crs.Grps.GrpTypes.NumGrpTypes;
		    NumGrpTyp++)
		  if (Gbl.Crs.Grps.GrpTypes.LstGrpTypes[NumGrpTyp].NumGrps)         // If current course tiene groups of este type
		    {
		     HTM_TH_Begin (1,1,"LM LIGHT_BLUE");
		     HTM_TxtF ("%s&nbsp;%s",
			       Txt_Group,
			       Gbl.Crs.Grps.GrpTypes.LstGrpTypes[NumGrpTyp].GrpTypName);
		     HTM_TH_End ();
		    }

	    if (Gbl.Crs.Records.LstFields.Num)
	      {
	       /* 3. Names of record fields that depend on the course */
	       for (NumField = 0;
		    NumField < Gbl.Crs.Records.LstFields.Num;
		    NumField++)
		  HTM_TH (1,1,"LM LIGHT_BLUE",Gbl.Crs.Records.LstFields.Lst[NumField].Name);

	       /* 4. Visibility type for the record fields that depend on the course, in other row */
	       HTM_TR_End ();
	       HTM_TR_Begin (NULL);
	       for (NumCol = 0;
		    NumCol < NumColumnsCardAndGroups;
		    NumCol++)
		  if (NumCol != 1 || Gbl.Usrs.Listing.WithPhotos)        // Skip photo column if I don't want it in listing
		    {
		     HTM_TD_Begin ("class=\"VERY_LIGHT_BLUE\"");
		     HTM_TD_End ();
		    }
	       for (NumField = 0;
		    NumField < Gbl.Crs.Records.LstFields.Num;
		    NumField++)
		 {
		  HTM_TH_Begin (1,1,"LM VERY_LIGHT_BLUE");
		     HTM_TxtF ("(%s)",Txt_RECORD_FIELD_VISIBILITY_RECORD[Gbl.Crs.Records.LstFields.Lst[NumField].Visibility]);
		  HTM_TH_End ();
		 }
	      }
	   }

      /* End row */
      HTM_TR_End ();

      /***** Initialize structure with user's data *****/
      Usr_UsrDataConstructor (&UsrDat);

      /***** List students' data *****/
      for (NumUsr = 0, Gbl.RowEvenOdd = 0;
           NumUsr < Gbl.Usrs.LstUsrs[Rol_STD].NumUsrs; )
        {
         UsrDat.UsrCod = Gbl.Usrs.LstUsrs[Rol_STD].Lst[NumUsr].UsrCod;
         if (Usr_ChkUsrCodAndGetAllUsrDataFromUsrCod (&UsrDat,
                                                      Usr_DONT_GET_PREFS,
                                                      Usr_DONT_GET_ROLE_IN_CURRENT_CRS))
           {
            UsrDat.Accepted = Gbl.Usrs.LstUsrs[Rol_STD].Lst[NumUsr].Accepted;
            NumUsr++;
            Usr_WriteRowStdAllData (&UsrDat,GroupNames);

            Gbl.RowEvenOdd = 1 - Gbl.RowEvenOdd;
           }
        }

      /***** Free memory used for user's data *****/
      Usr_UsrDataDestructor (&UsrDat);

      /***** End table *****/
      HTM_TABLE_End ();

      /***** Free memory used by the string with the list of group names where student belongs to *****/
      if (Gbl.Scope.Current == HieLvl_CRS)
         free (GroupNames);
     }
   else        // Gbl.Usrs.LstUsrs[Rol_STD].NumUsrs == 0
      /***** Show warning indicating no students found *****/
      Usr_ShowWarningNoUsersFound (Rol_STD);

   /***** Free memory for list of selected groups *****/
   Grp_FreeListCodSelectedGrps ();

   /***** Free memory for students list *****/
   Usr_FreeUsrsList (Rol_STD);

   /***** Free list of groups types and groups in current course *****/
   Grp_FreeListGrpTypesAndGrps ();
  }

/*****************************************************************************/
/*************** List users (of current course) for selection ****************/
/*****************************************************************************/

static void Usr_ListUsrsForSelection (Rol_Role_t Role,
				      struct SelectedUsrs *SelectedUsrs)
  {
   unsigned NumUsr;
   struct UsrData UsrDat;

   if (Gbl.Usrs.LstUsrs[Role].NumUsrs)
     {
      /***** Initialize field names *****/
      Usr_SetUsrDatMainFieldNames ();

      /***** Put a row to select all users *****/
      Usr_PutCheckboxToSelectAllUsers (Role,SelectedUsrs);

      /***** Heading row with column names *****/
      Usr_WriteHeaderFieldsUsrDat (true);	// Columns for the data

      /***** Initialize structure with user's data *****/
      Usr_UsrDataConstructor (&UsrDat);

      /***** List users' data *****/
      for (NumUsr = 0, Gbl.RowEvenOdd = 0;
	   NumUsr < Gbl.Usrs.LstUsrs[Role].NumUsrs; )
	{
	 UsrDat.UsrCod = Gbl.Usrs.LstUsrs[Role].Lst[NumUsr].UsrCod;
	 if (Usr_ChkUsrCodAndGetAllUsrDataFromUsrCod (&UsrDat,
	                                              Usr_DONT_GET_PREFS,
	                                              Usr_DONT_GET_ROLE_IN_CURRENT_CRS))
	   {
	    UsrDat.Accepted = Gbl.Usrs.LstUsrs[Role].Lst[NumUsr].Accepted;
	    Usr_WriteRowUsrMainData (++NumUsr,&UsrDat,true,Role,SelectedUsrs);

	    Gbl.RowEvenOdd = 1 - Gbl.RowEvenOdd;
	   }
	}

      /***** Free memory used for user's data *****/
      Usr_UsrDataDestructor (&UsrDat);
     }
  }

/*****************************************************************************/
/************************** List all teachers' data **************************/
/*****************************************************************************/

void Usr_ListAllDataTchs (void)
  {
   extern const char *Txt_Photo;
   extern const char *Txt_ID;
   extern const char *Txt_Surname_1;
   extern const char *Txt_Surname_2;
   extern const char *Txt_First_name;
   extern const char *Txt_Institution;
   extern const char *Txt_Email;
   extern const char *Txt_Center;
   extern const char *Txt_Department;
   extern const char *Txt_Office;
   extern const char *Txt_Phone;
   unsigned NumUsrs;
   unsigned NumColumns;
   const char *FieldNames[Usr_NUM_ALL_FIELDS_DATA_TCH];

   /***** Initialize field names *****/
   FieldNames[ 0] = Txt_Photo;
   FieldNames[ 1] = Txt_ID;
   FieldNames[ 2] = Txt_Surname_1;
   FieldNames[ 3] = Txt_Surname_2;
   FieldNames[ 4] = Txt_First_name;
   FieldNames[ 5] = Txt_Email;
   FieldNames[ 6] = Txt_Institution;
   FieldNames[ 7] = Txt_Center;
   FieldNames[ 8] = Txt_Department;
   FieldNames[ 9] = Txt_Office;
   FieldNames[10] = Txt_Phone;

   /***** Get and update type of list,
          number of columns in class photo
          and preference about viewing photos *****/
   Usr_GetAndUpdatePrefsAboutUsrList ();

   /***** Get scope *****/
   Gbl.Scope.Allowed = 1 << HieLvl_SYS |
	               1 << HieLvl_CTY |
                       1 << HieLvl_INS |
                       1 << HieLvl_CTR |
                       1 << HieLvl_DEG |
                       1 << HieLvl_CRS;
   Gbl.Scope.Default = HieLvl_CRS;
   Sco_GetScope ("ScopeUsr");

   /***** Get list of teachers *****/
   Usr_GetListUsrs (Gbl.Scope.Current,Rol_NET);	// Non-editing teachers
   Usr_GetListUsrs (Gbl.Scope.Current,Rol_TCH);	// Teachers
   if (Gbl.Scope.Current == HieLvl_CRS)
      NumUsrs = Gbl.Usrs.LstUsrs[Rol_NET].NumUsrs +
		Gbl.Usrs.LstUsrs[Rol_TCH].NumUsrs;
   else
      NumUsrs = Usr_GetNumUsrsInCrss (Gbl.Scope.Current,
				     (Gbl.Scope.Current == HieLvl_CTY ? Gbl.Hierarchy.Cty.CtyCod :
				     (Gbl.Scope.Current == HieLvl_INS ? Gbl.Hierarchy.Ins.InsCod :
				     (Gbl.Scope.Current == HieLvl_CTR ? Gbl.Hierarchy.Ctr.CtrCod :
				     (Gbl.Scope.Current == HieLvl_DEG ? Gbl.Hierarchy.Deg.DegCod :
				     (Gbl.Scope.Current == HieLvl_CRS ? Gbl.Hierarchy.Crs.CrsCod :
								     -1L))))),
				      1 << Rol_NET |
				      1 << Rol_TCH);

   if (NumUsrs)
     {
      /***** Initialize number of columns *****/
      NumColumns = Usr_NUM_ALL_FIELDS_DATA_TCH;

      /***** Begin table with lists of teachers *****/
      HTM_TABLE_BeginWide ();

      /***** List teachers and non-editing teachers *****/
      Gbl.RowEvenOdd = 0;
      Usr_ListRowsAllDataTchs (Rol_TCH,FieldNames,NumColumns);
      Usr_ListRowsAllDataTchs (Rol_NET,FieldNames,NumColumns);

      /***** End table *****/
      HTM_TABLE_End ();
     }
   else        // NumUsrs == 0
      /***** Show warning indicating no teachers found *****/
      Usr_ShowWarningNoUsersFound (Rol_TCH);

   /***** Free memory for teachers lists *****/
   Usr_FreeUsrsList (Rol_TCH);	// Teachers
   Usr_FreeUsrsList (Rol_NET);	// Non-editing teachers
  }

/*****************************************************************************/
/*********************** List all teachers' data rows ************************/
/*****************************************************************************/

static void Usr_ListRowsAllDataTchs (Rol_Role_t Role,
                                     const char *FieldNames[Usr_NUM_ALL_FIELDS_DATA_TCH],
                                     unsigned NumColumns)
  {
   unsigned NumCol;
   struct UsrData UsrDat;
   unsigned NumUsr;

   /***** Heading row *****/
   HTM_TR_Begin (NULL);

   for (NumCol = (Gbl.Usrs.Listing.WithPhotos ? 0 :
						1);
	NumCol < NumColumns;
	NumCol++)
      HTM_TH (1,1,"LM LIGHT_BLUE",FieldNames[NumCol]);

   HTM_TR_End ();

   /***** Initialize structure with user's data *****/
   Usr_UsrDataConstructor (&UsrDat);

   /***** List data of teachers *****/
   for (NumUsr = 0;
	NumUsr < Gbl.Usrs.LstUsrs[Role].NumUsrs; )
     {
      UsrDat.UsrCod = Gbl.Usrs.LstUsrs[Role].Lst[NumUsr].UsrCod;
      if (Usr_ChkUsrCodAndGetAllUsrDataFromUsrCod (&UsrDat,
                                                   Usr_DONT_GET_PREFS,
                                                   Usr_DONT_GET_ROLE_IN_CURRENT_CRS))
	{
	 UsrDat.Accepted = Gbl.Usrs.LstUsrs[Role].Lst[NumUsr].Accepted;
	 NumUsr++;
	 Usr_WriteRowTchAllData (&UsrDat);

	 Gbl.RowEvenOdd = 1 - Gbl.RowEvenOdd;
	}
     }

   /***** Free memory used for user's data *****/
   Usr_UsrDataDestructor (&UsrDat);
  }

/*****************************************************************************/
/****************************** List users found *****************************/
/*****************************************************************************/
// Returns number of users found

unsigned Usr_ListUsrsFound (Rol_Role_t Role,
                            const char SearchQuery[Sch_MAX_BYTES_SEARCH_QUERY])
  {
   extern const char *Txt_user[Usr_NUM_SEXS];
   extern const char *Txt_users[Usr_NUM_SEXS];
   extern const char *Txt_ROLES_PLURAL_Abc[Rol_NUM_ROLES][Usr_NUM_SEXS];
   extern const char *Txt_ROLES_SINGUL_abc[Rol_NUM_ROLES][Usr_NUM_SEXS];
   extern const char *Txt_ROLES_PLURAL_abc[Rol_NUM_ROLES][Usr_NUM_SEXS];
   unsigned NumUsrs;
   unsigned NumUsr;
   struct UsrData UsrDat;
   Usr_Sex_t Sex;
   struct UsrInList *UsrInList;

   /***** Initialize field names *****/
   Usr_SetUsrDatMainFieldNames ();

   /***** Create temporary table with candidate users *****/
   // Search is faster (aproximately x2) using temporary tables
   Usr_CreateTmpTableAndSearchCandidateUsrs (SearchQuery);

   /***** Search for users *****/
   Usr_SearchListUsrs (Role);
   if ((NumUsrs = Gbl.Usrs.LstUsrs[Role].NumUsrs))
     {
      /***** Begin box and table *****/
      /* Number of users found */
      Sex = Usr_GetSexOfUsrsLst (Role);
      Box_BoxTableBegin (NULL,Str_BuildStringLongStr ((long) NumUsrs,
						      (Role == Rol_UNK) ? ((NumUsrs == 1) ? Txt_user[Sex] :
											    Txt_users[Sex]) :
								          ((NumUsrs == 1) ? Txt_ROLES_SINGUL_abc[Role][Sex] :
											    Txt_ROLES_PLURAL_abc[Role][Sex])),
			 NULL,NULL,
			 NULL,Box_NOT_CLOSABLE,2);
      Str_FreeString ();

      /***** Heading row with column names *****/
      Gbl.Usrs.Listing.WithPhotos = true;
      Usr_WriteHeaderFieldsUsrDat (false);	// Columns for the data

      /***** Initialize structure with user's data *****/
      Usr_UsrDataConstructor (&UsrDat);

      /***** List data of users *****/
      for (NumUsr = 0, Gbl.RowEvenOdd = 0;
           NumUsr < NumUsrs;
           NumUsr++)
        {
         UsrInList = &Gbl.Usrs.LstUsrs[Role].Lst[NumUsr];

	 /* Copy user's basic data from list */
         Usr_CopyBasicUsrDataFromList (&UsrDat,UsrInList);

	 /* Get list of user's IDs */
         ID_GetListIDsFromUsrCod (&UsrDat);

	 /* Write data of this user */
	 Usr_WriteRowUsrMainData (NumUsr + 1,&UsrDat,false,Role,
				  &Gbl.Usrs.Selected);

	 /* Write all the courses this user belongs to */
	 if (Role != Rol_GST &&				// Guests do not belong to any course
	     Gbl.Usrs.Me.Role.Logged >= Rol_DEG_ADM)		// Only admins can view the courses
	   {
	    HTM_TR_Begin (NULL);

	    HTM_TD_Begin ("colspan=\"2\" class=\"COLOR%u\"",Gbl.RowEvenOdd);
	    HTM_TD_End ();

	    HTM_TD_Begin ("colspan=\"%u\" class=\"COLOR%u\"",
			  Usr_NUM_MAIN_FIELDS_DATA_USR-2,
			  Gbl.RowEvenOdd);
	    if (Role == Rol_UNK)
	      {
	       Crs_GetAndWriteCrssOfAUsr (&UsrDat,Rol_TCH);
	       Crs_GetAndWriteCrssOfAUsr (&UsrDat,Rol_NET);
  	       Crs_GetAndWriteCrssOfAUsr (&UsrDat,Rol_STD);
	      }
	    else
	       Crs_GetAndWriteCrssOfAUsr (&UsrDat,Role);
	    HTM_TD_End ();

	    HTM_TR_End ();
	   }

	 Gbl.RowEvenOdd = 1 - Gbl.RowEvenOdd;
        }

      /***** Free memory used for user's data *****/
      Usr_UsrDataDestructor (&UsrDat);

      /***** End table and box *****/
      Box_BoxTableEnd ();
     }

   /***** Free memory for teachers list *****/
   Usr_FreeUsrsList (Role);

   /***** Drop temporary table with candidate users *****/
   Usr_DropTmpTableWithCandidateUsrs ();

   return NumUsrs;
  }

/*****************************************************************************/
/**************************** List administrators ****************************/
/*****************************************************************************/

void Usr_ListDataAdms (void)
  {
   extern const char *Hlp_USERS_Administrators;
   extern const char *The_ClassFormInBox[The_NUM_THEMES];
   extern const char *Txt_ROLES_PLURAL_Abc[Rol_NUM_ROLES][Usr_NUM_SEXS];
   extern const char *Txt_Scope;
   extern const char *Txt_No_INDEX;
   extern const char *Txt_Photo;
   extern const char *Txt_ID;
   extern const char *Txt_Surname_1;
   extern const char *Txt_Surname_2;
   extern const char *Txt_First_name;
   extern const char *Txt_Email;
   extern const char *Txt_Institution;
   unsigned NumCol;
   unsigned NumUsr;
   struct UsrData UsrDat;
   const char *FieldNames[Usr_NUM_MAIN_FIELDS_DATA_ADM];

   /***** Put contextual links *****/
   switch (Gbl.Usrs.Me.Role.Logged)
     {
      case Rol_TCH:
      case Rol_DEG_ADM:
      case Rol_CTR_ADM:
      case Rol_INS_ADM:
      case Rol_SYS_ADM:
	 /***** Contextual menu *****/
	 Mnu_ContextMenuBegin ();
	 if (Gbl.Usrs.Me.Role.Logged == Rol_SYS_ADM)
	   {
	    Usr_PutLinkToSeeGuests ();			// List guests
	    Dup_PutLinkToListDupUsrs ();		// List possible duplicate users
	   }
	 Enr_PutLinkToAdminOneUsr (ActReqMdfOneOth);	// Admin one user
	 if (Gbl.Usrs.Me.Role.Logged == Rol_SYS_ADM)
	    Enr_PutLinkToRemOldUsrs ();			// Remove old users
	 Mnu_ContextMenuEnd ();
	 break;
      default:
	 break;
     }

   /***** Initialize field names *****/
   FieldNames[0] = Txt_No_INDEX;
   FieldNames[1] = Txt_Photo;
   FieldNames[2] = Txt_ID;
   FieldNames[3] = Txt_Surname_1;
   FieldNames[4] = Txt_Surname_2;
   FieldNames[5] = Txt_First_name;
   FieldNames[6] = Txt_Institution;

   /***** Get and update type of list,
          number of columns in class photo
          and preference about viewing photos *****/
   Usr_GetAndUpdatePrefsAboutUsrList ();

   /***** Get scope *****/
   Gbl.Scope.Allowed = 1 << HieLvl_SYS |
	               1 << HieLvl_CTY |
                       1 << HieLvl_INS |
                       1 << HieLvl_CTR |
                       1 << HieLvl_DEG;
   Gbl.Scope.Default = HieLvl_DEG;
   Sco_GetScope ("ScopeUsr");

   /***** Get list of administrators *****/
   Usr_GetAdmsLst (Gbl.Scope.Current);

   /***** Begin box with list of administrators *****/
   Box_BoxBegin (NULL,Txt_ROLES_PLURAL_Abc[Rol_DEG_ADM][Usr_SEX_UNKNOWN],
                 NULL,NULL,
                 Hlp_USERS_Administrators,Box_NOT_CLOSABLE);

   /***** Form to select scope *****/
   HTM_DIV_Begin ("class=\"CM\"");
   Frm_BeginForm (ActLstOth);
   Usr_PutParamListWithPhotos ();
   HTM_LABEL_Begin ("class=\"%s\"",The_ClassFormInBox[Gbl.Prefs.Theme]);
   HTM_TxtColonNBSP (Txt_Scope);
   Sco_PutSelectorScope ("ScopeUsr",HTM_SUBMIT_ON_CHANGE);
   HTM_LABEL_End ();
   Frm_EndForm ();
   HTM_DIV_End ();

   if (Gbl.Usrs.LstUsrs[Rol_DEG_ADM].NumUsrs)
     {
      /****** Show photos? *****/
      HTM_DIV_Begin ("class=\"PREF_CONT\"");
      HTM_DIV_Begin ("class=\"PREF_OFF\"");
      Frm_BeginForm (ActLstOth);
      Sco_PutParamCurrentScope (&Gbl.Scope.Current);
      Usr_PutCheckboxListWithPhotos ();
      Frm_EndForm ();
      HTM_DIV_End ();
      HTM_DIV_End ();

      /***** Heading row with column names *****/
      HTM_TABLE_Begin (NULL);
      HTM_TR_Begin (NULL);

      for (NumCol = 0;
           NumCol < Usr_NUM_MAIN_FIELDS_DATA_ADM;
           NumCol++)
         if (NumCol != 1 || Gbl.Usrs.Listing.WithPhotos)        // Skip photo column if I don't want this column
            HTM_TH (1,1,"LM LIGHT_BLUE",FieldNames[NumCol]);

      HTM_TR_End ();

      /***** Initialize structure with user's data *****/
      Usr_UsrDataConstructor (&UsrDat);

      /***** List data of administrators *****/
      for (NumUsr = 0, Gbl.RowEvenOdd = 0;
           NumUsr < Gbl.Usrs.LstUsrs[Rol_DEG_ADM].NumUsrs; )
        {
         UsrDat.UsrCod = Gbl.Usrs.LstUsrs[Rol_DEG_ADM].Lst[NumUsr].UsrCod;
         if (Usr_ChkUsrCodAndGetAllUsrDataFromUsrCod (&UsrDat,
                                                      Usr_DONT_GET_PREFS,
                                                      Usr_DONT_GET_ROLE_IN_CURRENT_CRS))
           {
            UsrDat.Accepted = Gbl.Usrs.LstUsrs[Rol_DEG_ADM].Lst[NumUsr].Accepted;
            Usr_WriteRowAdmData (++NumUsr,&UsrDat);

            Gbl.RowEvenOdd = 1 - Gbl.RowEvenOdd;
           }
        }

      /***** Free memory used for user's data *****/
      Usr_UsrDataDestructor (&UsrDat);

      /***** End table *****/
      HTM_TABLE_End ();
     }
   else        // Gbl.Usrs.LstUsrs[Rol_DEG_ADM].NumUsrs == 0
      /***** Show warning indicating no admins found *****/
      Usr_ShowWarningNoUsersFound (Rol_DEG_ADM);

   /***** End box *****/
   Box_BoxEnd ();

   /***** Free memory for administrators list *****/
   Usr_FreeUsrsList (Rol_DEG_ADM);
  }

/*****************************************************************************/
/**************** Put hidden parameters with type of list, *******************/
/**************** number of columns in class photo         *******************/
/**************** and preference about viewing photos      *******************/
/*****************************************************************************/

void Usr_PutParamsPrefsAboutUsrList (void)
  {
   Usr_PutParamUsrListType (Gbl.Usrs.Me.ListType);
   Usr_PutParamColsClassPhoto ();
   Usr_PutParamListWithPhotos ();
  }

/*****************************************************************************/
/****************** Get and update type of list,        **********************/
/****************** number of columns in class photo    **********************/
/****************** and preference about viewing photos **********************/
/*****************************************************************************/

void Usr_GetAndUpdatePrefsAboutUsrList (void)
  {
   /***** Get and update type of list *****/
   Usr_GetAndUpdateUsrListType ();

   /***** Get and update number of columns in class photo *****/
   Usr_GetAndUpdateColsClassPhoto ();

   /***** Get and update preference about viewing photos *****/
   Usr_GetAndUpdatePrefAboutListWithPhotos ();
  }

/*****************************************************************************/
/****************** Get from form the type of users' list ********************/
/*****************************************************************************/

static void Usr_GetAndUpdateUsrListType (void)
  {
   /***** Get type of list used to select users from form *****/
   Usr_GetUsrListTypeFromForm ();

   if (Gbl.Usrs.Me.ListType != Usr_LIST_UNKNOWN)
      /* Save in the database the type of list preferred by me */
      Usr_UpdateMyUsrListTypeInDB ();
   else
      /* If parameter can't be retrieved from,
         get my preference from database */
      Usr_GetMyUsrListTypeFromDB ();
  }

/*****************************************************************************/
/************* Put a hidden parameter with the users' list type **************/
/*****************************************************************************/

void Usr_PutParamUsrListType (Usr_ShowUsrsType_t ListType)
  {
   Par_PutHiddenParamUnsigned (NULL,"UsrListType",(unsigned) ListType);
  }

/*****************************************************************************/
/****************** Get from form the type of users' list ********************/
/*****************************************************************************/

static void Usr_GetUsrListTypeFromForm (void)
  {
   Gbl.Usrs.Me.ListType = (Usr_ShowUsrsType_t)
	                  Par_GetParToUnsignedLong ("UsrListType",
                                                    0,
                                                    Usr_NUM_USR_LIST_TYPES - 1,
                                                    (unsigned long) Usr_LIST_UNKNOWN);
  }

/*****************************************************************************/
/************** Get my preference about type of users' list ******************/
/*****************************************************************************/

static void Usr_GetMyUsrListTypeFromDB (void)
  {
   MYSQL_RES *mysql_res;
   MYSQL_ROW row;
   unsigned NumRows;
   Usr_ShowUsrsType_t ListType;

   /***** Get type of listing of users from database *****/
   NumRows = (unsigned)
   DB_QuerySELECT (&mysql_res,"can not get type of listing of users",
		   "SELECT UsrListType"	// row[0]
		    " FROM crs_user_settings"
		   " WHERE UsrCod=%ld"
		     " AND CrsCod=%ld",
		   Gbl.Usrs.Me.UsrDat.UsrCod,
		   Gbl.Hierarchy.Crs.CrsCod);
   if (NumRows == 1)		// Should be one only row
     {
      /* Get type of users' listing used to select some of them */
      Gbl.Usrs.Me.ListType = Usr_SHOW_USRS_TYPE_DEFAULT;
      row = mysql_fetch_row (mysql_res);
      if (row[0])
         for (ListType  = (Usr_ShowUsrsType_t) 0;
              ListType <= (Usr_ShowUsrsType_t) (Usr_NUM_USR_LIST_TYPES - 1);
              ListType++)
            if (!strcasecmp (row[0],Usr_StringsUsrListTypeInDB[ListType]))
              {
               Gbl.Usrs.Me.ListType = ListType;
               break;
              }
     }
   else if (NumRows == 0)	// If I am an administrator or superuser
				// and I don't belong to current course,
				// then the result will be the default
      Gbl.Usrs.Me.ListType = Usr_SHOW_USRS_TYPE_DEFAULT;
   else				// Error in database:
				// more than one row for a user in course
      Err_ShowErrorAndExit ("Error when getting type of listing of users.");

   /***** Free structure that stores the query result *****/
   DB_FreeMySQLResult (&mysql_res);
  }

/*****************************************************************************/
/***************** Save my preference about type of users' list **************/
/*****************************************************************************/

static void Usr_UpdateMyUsrListTypeInDB (void)
  {
   /***** Update type of users listing *****/
   DB_QueryUPDATE ("can not update type of listing",
		   "UPDATE crs_user_settings"
		     " SET UsrListType='%s'"
                   " WHERE UsrCod=%ld"
                     " AND CrsCod=%ld",
		   Usr_StringsUsrListTypeInDB[Gbl.Usrs.Me.ListType],
		   Gbl.Usrs.Me.UsrDat.UsrCod,
		   Gbl.Hierarchy.Crs.CrsCod);
  }

/*****************************************************************************/
/************* Get and update number of columns in class photo ***************/
/*****************************************************************************/

void Usr_GetAndUpdateColsClassPhoto (void)
  {
   /***** Get the number of columns in class photo from form *****/
   Usr_GetParamColsClassPhotoFromForm ();

   if (Gbl.Usrs.ClassPhoto.Cols)
      /* Save the number of columns into the database */
      Usr_UpdateMyColsClassPhotoInDB ();
   else
      /* If parameter can't be retrieved from form,
         get my preference from database */
      Usr_GetMyColsClassPhotoFromDB ();
  }

/*****************************************************************************/
/****** Put a hidden parameter with the number of colums in class photo ******/
/*****************************************************************************/

void Usr_PutParamColsClassPhoto (void)
  {
   Par_PutHiddenParamUnsigned (NULL,"ColsClassPhoto",Gbl.Usrs.ClassPhoto.Cols);
  }

/*****************************************************************************/
/************* Get from form the number of colums in class photo *************/
/*****************************************************************************/

static void Usr_GetParamColsClassPhotoFromForm (void)
  {
   Gbl.Usrs.ClassPhoto.Cols = (unsigned)
	                      Par_GetParToUnsignedLong ("ColsClassPhoto",
                                                        1,
                                                        Usr_CLASS_PHOTO_COLS_MAX,
                                                        0);
  }

/*****************************************************************************/
/** Get my prefs. about number of colums in class photo for current course ***/
/*****************************************************************************/

static void Usr_GetMyColsClassPhotoFromDB (void)
  {
   MYSQL_RES *mysql_res;
   MYSQL_ROW row;
   unsigned NumRows;

   Gbl.Usrs.ClassPhoto.Cols = Usr_CLASS_PHOTO_COLS_DEF;

   /***** If user logged and course selected... *****/
   if (Gbl.Usrs.Me.Logged &&
       Gbl.Hierarchy.Level == HieLvl_CRS)	// Course selected
     {
      /***** Get number of columns in class photo from database *****/
      NumRows = (unsigned)
      DB_QuerySELECT (&mysql_res,"can not get number of columns in class photo",
		      "SELECT ColsClassPhoto"	// row[0]
		       " FROM crs_user_settings"
		      " WHERE UsrCod=%ld"
		        " AND CrsCod=%ld",
		      Gbl.Usrs.Me.UsrDat.UsrCod,
		      Gbl.Hierarchy.Crs.CrsCod);
      if (NumRows == 1)		// Should be one only row
        {
         /* Get number of columns in class photo */
         row = mysql_fetch_row (mysql_res);
         if (row[0])
            if (sscanf (row[0],"%u",&Gbl.Usrs.ClassPhoto.Cols) == 1)
               if (Gbl.Usrs.ClassPhoto.Cols < 1 ||
                   Gbl.Usrs.ClassPhoto.Cols > Usr_CLASS_PHOTO_COLS_MAX)
                  Gbl.Usrs.ClassPhoto.Cols = Usr_CLASS_PHOTO_COLS_DEF;
        }
      else if (NumRows > 1)	// Error in database:
				// more than one row for a user in course
         Err_ShowErrorAndExit ("Error when getting number of columns"
			       " in class photo.");

      /***** Free structure that stores the query result *****/
      DB_FreeMySQLResult (&mysql_res);
     }
  }

/*****************************************************************************/
/** Save my prefs. about number of colums in class photo for current course **/
/*****************************************************************************/

static void Usr_UpdateMyColsClassPhotoInDB (void)
  {
   if (Gbl.Usrs.Me.Logged &&
       Gbl.Hierarchy.Level == HieLvl_CRS)	// Course selected
      /***** Update number of colums in class photo for current course *****/
      DB_QueryUPDATE ("can not update number of columns in class photo",
		      "UPDATE crs_user_settings"
		        " SET ColsClassPhoto=%u"
                      " WHERE UsrCod=%ld"
                        " AND CrsCod=%ld",
		      Gbl.Usrs.ClassPhoto.Cols,
		      Gbl.Usrs.Me.UsrDat.UsrCod,
		      Gbl.Hierarchy.Crs.CrsCod);
  }

/*****************************************************************************/
/********** Get and update preference about photos in users' list ************/
/*****************************************************************************/

static void Usr_GetAndUpdatePrefAboutListWithPhotos (void)
  {
   /***** Get my preference about photos in users' list from form *****/
   if (Usr_GetParamListWithPhotosFromForm ())
      /* Save preference about photos in users' list into the database */
      Usr_UpdateMyPrefAboutListWithPhotosPhotoInDB ();
   else
      /* If parameter can't be retrieved from form,
         get my preference from database */
      Usr_GetMyPrefAboutListWithPhotosFromDB ();
  }

/*****************************************************************************/
/** Put a hidden parameter with the preference about photos in users' list ***/
/*****************************************************************************/

void Usr_PutParamListWithPhotos (void)
  {
   Par_PutHiddenParamChar ("WithPhotosExists",'Y');
   Par_PutHiddenParamChar ("WithPhotos",
                           Gbl.Usrs.Listing.WithPhotos ? 'Y' :
                        	                         'N');
  }

/*****************************************************************************/
/********* Get from form the preference about photos in users' list **********/
/*****************************************************************************/

static bool Usr_GetParamListWithPhotosFromForm (void)
  {
   /***** Get if exists parameter with preference about photos in users' list *****/
   if (Par_GetParToBool ("WithPhotosExists"))
     {
      /***** Parameter with preference about photos in users' list exists, so get it *****/
      Gbl.Usrs.Listing.WithPhotos = Par_GetParToBool ("WithPhotos");
      return true;
     }

   Gbl.Usrs.Listing.WithPhotos = Usr_LIST_WITH_PHOTOS_DEF;
   return false;
  }

/*****************************************************************************/
/***** Get my preference about photos in users' list for current course ******/
/*****************************************************************************/

void Usr_GetMyPrefAboutListWithPhotosFromDB (void)
  {
   MYSQL_RES *mysql_res;
   MYSQL_ROW row;
   unsigned NumRows;

   Gbl.Usrs.Listing.WithPhotos = Usr_LIST_WITH_PHOTOS_DEF;

   /***** If no user logged or not course selected... *****/
   if (Gbl.Usrs.Me.Logged && Gbl.Hierarchy.Crs.CrsCod)
     {
      /***** Get if listing of users must show photos from database *****/
      NumRows = (unsigned)
      DB_QuerySELECT (&mysql_res,"can not check if listing of users"
			         " must show photos",
		      "SELECT ListWithPhotos"	// row[0]
		       " FROM crs_user_settings"
		      " WHERE UsrCod=%ld"
		        " AND CrsCod=%ld",
		      Gbl.Usrs.Me.UsrDat.UsrCod,
		      Gbl.Hierarchy.Crs.CrsCod);
      if (NumRows == 1)                // Should be one only row
        {
         /* Get number of columns in class photo */
         Gbl.Usrs.Listing.WithPhotos = Usr_LIST_WITH_PHOTOS_DEF;
         row = mysql_fetch_row (mysql_res);
         Gbl.Usrs.Listing.WithPhotos = (row[0][0] == 'Y');
        }
      else if (NumRows > 1)        // Error in database:
				   // more than one row for a user in course
         Err_ShowErrorAndExit ("Error when checking if listing of users"
			       " must show photos.");

      /***** Free structure that stores the query result *****/
      DB_FreeMySQLResult (&mysql_res);
     }
  }

/*****************************************************************************/
/**** Save my preference about photos in users' list for current course ******/
/*****************************************************************************/

static void Usr_UpdateMyPrefAboutListWithPhotosPhotoInDB (void)
  {
   if (Gbl.Usrs.Me.Logged &&
       Gbl.Hierarchy.Level == HieLvl_CRS)	// Course selected
      /***** Update number of colums in class photo for current course *****/
      DB_QueryUPDATE ("can not update your preference about photos in listing",
		      "UPDATE crs_user_settings"
		        " SET ListWithPhotos='%c'"
                      " WHERE UsrCod=%ld"
                        " AND CrsCod=%ld",
		      Gbl.Usrs.Listing.WithPhotos ? 'Y' :
						    'N',
		      Gbl.Usrs.Me.UsrDat.UsrCod,
		      Gbl.Hierarchy.Crs.CrsCod);
  }

/*****************************************************************************/
/********** Put a link (form) to show list or class photo of guests **********/
/*****************************************************************************/

static void Usr_PutLinkToSeeAdmins (void)
  {
   extern const char *Rol_Icons[Rol_NUM_ROLES];
   extern const char *Txt_ROLES_PLURAL_Abc[Rol_NUM_ROLES][Usr_NUM_SEXS];

   /***** Put form to list admins *****/
   Lay_PutContextualLinkIconText (ActLstOth,NULL,
                                  NULL,NULL,
				  Rol_Icons[Rol_DEG_ADM],
				  Txt_ROLES_PLURAL_Abc[Rol_DEG_ADM][Usr_SEX_UNKNOWN]);
  }

/*****************************************************************************/
/********** Put a link (form) to show list or class photo of guests **********/
/*****************************************************************************/

static void Usr_PutLinkToSeeGuests (void)
  {
   extern const char *Txt_ROLES_PLURAL_Abc[Rol_NUM_ROLES][Usr_NUM_SEXS];

   /***** Put form to list guests *****/
   Lay_PutContextualLinkIconText (ActLstGst,NULL,
                                  NULL,NULL,
				  "users.svg",
				  Txt_ROLES_PLURAL_Abc[Rol_GST][Usr_SEX_UNKNOWN]);
  }

/*****************************************************************************/
/********************* Show list or class photo of guests ********************/
/*****************************************************************************/

void Usr_SeeGuests (void)
  {
   extern const char *Hlp_USERS_Guests;
   extern const char *The_ClassFormInBox[The_NUM_THEMES];
   extern const char *Txt_ROLES_PLURAL_Abc[Rol_NUM_ROLES][Usr_NUM_SEXS];
   extern const char *Txt_Scope;
   bool ICanChooseOption[Usr_LIST_USRS_NUM_OPTIONS];
   bool PutForm;

   /***** Contextual menu *****/
   Mnu_ContextMenuBegin ();
   Usr_PutLinkToSeeAdmins ();			// List admins
   Enr_PutLinkToAdminOneUsr (ActReqMdfOneOth);	// Admin one user
   if (Gbl.Usrs.Me.Role.Logged == Rol_SYS_ADM)
      Enr_PutLinkToRemOldUsrs ();		// Remove old users
   Mnu_ContextMenuEnd ();

   /***** Get and update type of list,
          number of columns in class photo
          and preference about view photos *****/
   Usr_GetAndUpdatePrefsAboutUsrList ();

   /***** Get scope *****/
   Sco_SetScopesForListingGuests ();
   Sco_GetScope ("ScopeUsr");

   /***** Get list of guests in current scope *****/
   Usr_GetGstsLst (Gbl.Scope.Current);

   /***** Begin box *****/
   Box_BoxBegin (NULL,Txt_ROLES_PLURAL_Abc[Rol_GST][Usr_SEX_UNKNOWN],
                 Usr_PutIconsListGsts,NULL,
		 Hlp_USERS_Guests,Box_NOT_CLOSABLE);

   /***** Form to select scope *****/
   if (Gbl.Usrs.Me.Role.Logged == Rol_SYS_ADM)
     {
      HTM_DIV_Begin ("class=\"CM\"");
      Frm_BeginForm (ActLstGst);
      Usr_PutParamsPrefsAboutUsrList ();
      HTM_LABEL_Begin ("class=\"%s\"",The_ClassFormInBox[Gbl.Prefs.Theme]);
      HTM_TxtColonNBSP (Txt_Scope);
      Sco_PutSelectorScope ("ScopeUsr",HTM_SUBMIT_ON_CHANGE);
      HTM_LABEL_End ();
      Frm_EndForm ();
      HTM_DIV_End ();
     }

   /***** Begin section with user list *****/
   HTM_SECTION_Begin (Usr_USER_LIST_SECTION_ID);

   if (Gbl.Usrs.LstUsrs[Rol_GST].NumUsrs)
     {
      if (Usr_GetIfShowBigList (Gbl.Usrs.LstUsrs[Rol_GST].NumUsrs,
	                        Sco_PutParamCurrentScope,&Gbl.Scope.Current,
	                        NULL))
        {
	 /***** Form to select type of list of users *****/
	 Usr_ShowFormsToSelectUsrListType (Sco_PutParamCurrentScope,&Gbl.Scope.Current);

         /***** Draw a class photo with guests *****/
         if (Gbl.Usrs.Me.ListType == Usr_LIST_AS_CLASS_PHOTO)
	    Lay_WriteHeaderClassPhoto (false,true,
				       (Gbl.Scope.Current == HieLvl_CTR ||
					Gbl.Scope.Current == HieLvl_INS) ? Gbl.Hierarchy.Ins.InsCod :
									      -1L,
				       -1L,
				       -1L);

         /* Set options allowed */
         PutForm = Usr_SetOptionsListUsrsAllowed (Rol_GST,ICanChooseOption);

         /* Begin form */
         if (PutForm)
	    Frm_BeginForm (ActDoActOnSevGst);

         /* Begin table */
	 HTM_TABLE_BeginWide ();

         /* Draw the classphoto/list */
         switch (Gbl.Usrs.Me.ListType)
           {
            case Usr_LIST_AS_CLASS_PHOTO:
               Usr_DrawClassPhoto (Usr_CLASS_PHOTO_SEL_SEE,
        	                   Rol_GST,&Gbl.Usrs.Selected,
				   PutForm);	// Put checkbox to select users?
               break;
            case Usr_LIST_AS_LISTING:
               Usr_ListMainDataGsts (PutForm);	// Put checkbox to select users?
               break;
            default:
               break;
           }

         /* End table */
         HTM_TABLE_End ();

	 /***** Which action, show records, follow...? *****/
         if (PutForm)
           {
	    Usr_PutOptionsListUsrs (ICanChooseOption);
	    Frm_EndForm ();
           }
	}
     }
   else	// Gbl.Usrs.LstUsrs[Rol_GST].NumUsrs == 0
      /***** Show warning indicating no guests found *****/
      Usr_ShowWarningNoUsersFound (Rol_GST);

   /***** End section with user list *****/
   HTM_SECTION_End ();

   /***** End box *****/
   Box_BoxEnd ();

   /***** Free memory for guests list *****/
   Usr_FreeUsrsList (Rol_GST);
  }

/*****************************************************************************/
/******************** Show list or class photo of students *******************/
/*****************************************************************************/

void Usr_SeeStudents (void)
  {
   extern const char *Hlp_USERS_Students;
   extern const char *The_ClassFormInBox[The_NUM_THEMES];
   extern const char *Txt_ROLES_PLURAL_Abc[Rol_NUM_ROLES][Usr_NUM_SEXS];
   extern const char *Txt_Scope;
   bool ICanChooseOption[Usr_LIST_USRS_NUM_OPTIONS];
   bool PutForm;

   /***** Put contextual links *****/
   switch (Gbl.Usrs.Me.Role.Logged)
     {
      case Rol_STD:
      case Rol_TCH:
      case Rol_DEG_ADM:
      case Rol_CTR_ADM:
      case Rol_INS_ADM:
      case Rol_SYS_ADM:
	 /***** Contextual menu *****/
         Mnu_ContextMenuBegin ();
	 Enr_PutLinkToAdminOneUsr (ActReqMdfOneStd);	// Admin one student
	 if (Gbl.Hierarchy.Level == HieLvl_CRS &&		// Course selected
	     Gbl.Usrs.Me.Role.Logged != Rol_STD)	// Teacher or admin
	   {
	    Enr_PutLinkToAdminSeveralUsrs (Rol_STD);	// Admin several students
	    Rec_PutLinkToEditRecordFields ();		// Edit record fields
	   }
         Mnu_ContextMenuEnd ();
	 break;
      default:
	 break;
     }

   /***** Get and update type of list,
          number of columns in class photo
          and preference about view photos *****/
   Usr_GetAndUpdatePrefsAboutUsrList ();

   /***** Get scope *****/
   Sco_SetScopesForListingStudents ();
   Sco_GetScope ("ScopeUsr");

   /***** Get groups to show ******/
   if (Gbl.Scope.Current == HieLvl_CRS)
      Grp_GetParCodsSeveralGrpsToShowUsrs ();

   /***** Get list of students *****/
   Usr_GetListUsrs (Gbl.Scope.Current,Rol_STD);

   /***** Begin box *****/
   Box_BoxBegin (NULL,Txt_ROLES_PLURAL_Abc[Rol_STD][Usr_SEX_UNKNOWN],
                 Usr_PutIconsListStds,NULL,
		 Hlp_USERS_Students,Box_NOT_CLOSABLE);

   /***** Form to select scope *****/
   switch (Gbl.Usrs.Me.Role.Logged)
     {
      case Rol_DEG_ADM:
      case Rol_CTR_ADM:
      case Rol_INS_ADM:
      case Rol_SYS_ADM:
	 HTM_DIV_Begin ("class=\"CM\"");
	 Frm_BeginForm (ActLstStd);
	 Usr_PutParamsPrefsAboutUsrList ();
	 HTM_LABEL_Begin ("class=\"%s\"",The_ClassFormInBox[Gbl.Prefs.Theme]);
	 HTM_TxtColonNBSP (Txt_Scope);
	 Sco_PutSelectorScope ("ScopeUsr",HTM_SUBMIT_ON_CHANGE);
	 HTM_LABEL_End ();
	 Frm_EndForm ();
	 HTM_DIV_End ();
	 break;
      default:
	 break;
     }

   /***** Form to select groups *****/
   if (Gbl.Scope.Current == HieLvl_CRS)
      Grp_ShowFormToSelectSeveralGroups (Sco_PutParamCurrentScope,&Gbl.Scope.Current,
                                         Grp_MY_GROUPS);

   /***** Begin section with user list *****/
   HTM_SECTION_Begin (Usr_USER_LIST_SECTION_ID);

   if (Gbl.Usrs.LstUsrs[Rol_STD].NumUsrs)
     {
      if (Usr_GetIfShowBigList (Gbl.Usrs.LstUsrs[Rol_STD].NumUsrs,
	                        Sco_PutParamCurrentScope,&Gbl.Scope.Current,
	                        NULL))
        {
	 /***** Form to select type of list of users *****/
	 Usr_ShowFormsToSelectUsrListType (Sco_PutParamCurrentScope,&Gbl.Scope.Current);

         /***** Draw a class photo with students of the course *****/
         if (Gbl.Usrs.Me.ListType == Usr_LIST_AS_CLASS_PHOTO)
	    Lay_WriteHeaderClassPhoto (false,true,
				       (Gbl.Scope.Current == HieLvl_CRS ||
					Gbl.Scope.Current == HieLvl_DEG ||
					Gbl.Scope.Current == HieLvl_CTR ||
					Gbl.Scope.Current == HieLvl_INS) ? Gbl.Hierarchy.Ins.InsCod :
								        -1L,
				       (Gbl.Scope.Current == HieLvl_CRS ||
					Gbl.Scope.Current == HieLvl_DEG) ? Gbl.Hierarchy.Deg.DegCod :
									-1L,
					Gbl.Scope.Current == HieLvl_CRS  ? Gbl.Hierarchy.Crs.CrsCod :
									-1L);

         /* Set options allowed */
         PutForm = Usr_SetOptionsListUsrsAllowed (Rol_STD,ICanChooseOption);

         /* Begin form */
         if (PutForm)
           {
	    Frm_BeginForm (ActDoActOnSevStd);
	    Grp_PutParamsCodGrps ();
           }

         /* Begin table */
         HTM_TABLE_BeginWide ();

         /* Draw the classphoto/list */
         switch (Gbl.Usrs.Me.ListType)
           {
            case Usr_LIST_AS_CLASS_PHOTO:
               Usr_DrawClassPhoto (Usr_CLASS_PHOTO_SEL_SEE,
        	                   Rol_STD,&Gbl.Usrs.Selected,
				   PutForm);	// Put checkbox to select users?
               break;
            case Usr_LIST_AS_LISTING:
               Usr_ListMainDataStds (PutForm);	// Put checkbox to select users?
               break;
            default:
               break;
           }

         /* End table */
         HTM_TABLE_End ();

	 /***** Which action, show records, follow...? *****/
         if (PutForm)
           {
	    Usr_PutOptionsListUsrs (ICanChooseOption);
	    Frm_EndForm ();
           }
        }
     }
   else	// Gbl.Usrs.LstUsrs[Rol_STD].NumUsrs == 0
      /***** Show warning indicating no students found *****/
      Usr_ShowWarningNoUsersFound (Rol_STD);

   /***** End section with user list *****/
   HTM_SECTION_End ();

   /***** End box *****/
   Box_BoxEnd ();

   /***** Free memory for students list *****/
   Usr_FreeUsrsList (Rol_STD);

   /***** Free memory for list of selected groups *****/
   Grp_FreeListCodSelectedGrps ();
  }

/*****************************************************************************/
/******************** Show list or class photo of teachers *******************/
/*****************************************************************************/

void Usr_SeeTeachers (void)
  {
   extern const char *Hlp_USERS_Teachers;
   extern const char *The_ClassFormInBox[The_NUM_THEMES];
   extern const char *Txt_ROLES_PLURAL_Abc[Rol_NUM_ROLES][Usr_NUM_SEXS];
   extern const char *Txt_Scope;
   unsigned NumUsrs;
   bool ICanChooseOption[Usr_LIST_USRS_NUM_OPTIONS];
   bool PutForm;

   /***** Put contextual links *****/
   switch (Gbl.Usrs.Me.Role.Logged)
     {
      case Rol_NET:
      case Rol_TCH:
      case Rol_DEG_ADM:
      case Rol_CTR_ADM:
      case Rol_INS_ADM:
      case Rol_SYS_ADM:
	 /***** Contextual menu *****/
	 Mnu_ContextMenuBegin ();
	 Enr_PutLinkToAdminOneUsr (ActReqMdfOneTch);	// Admin one teacher
	 if (Gbl.Hierarchy.Level == HieLvl_CRS &&		// Course selected
	     Gbl.Usrs.Me.Role.Logged >= Rol_DEG_ADM)	// I am logged as admin
	   {
	    Enr_PutLinkToAdminSeveralUsrs (Rol_NET);	// Admin several non-editing teachers
	    Enr_PutLinkToAdminSeveralUsrs (Rol_TCH);	// Admin several teachers
	   }
	 Mnu_ContextMenuEnd ();
         break;
      default:
	 break;
     }

   /***** Get and update type of list,
          number of columns in class photo
          and preference about view photos *****/
   Usr_GetAndUpdatePrefsAboutUsrList ();

   /***** Get scope *****/
   Gbl.Scope.Allowed = 1 << HieLvl_SYS |
	               1 << HieLvl_CTY |
                       1 << HieLvl_INS |
                       1 << HieLvl_CTR |
                       1 << HieLvl_DEG |
                       1 << HieLvl_CRS;
   Gbl.Scope.Default = HieLvl_CRS;
   Sco_GetScope ("ScopeUsr");

   /***** Get groups to show ******/
   if (Gbl.Scope.Current == HieLvl_CRS)
      Grp_GetParCodsSeveralGrpsToShowUsrs ();

   /***** Get lists of teachers *****/
   Usr_GetListUsrs (Gbl.Scope.Current,Rol_NET);	// Non-editing teachers
   Usr_GetListUsrs (Gbl.Scope.Current,Rol_TCH);	// Teachers
   if (Gbl.Scope.Current == HieLvl_CRS)
      NumUsrs = Gbl.Usrs.LstUsrs[Rol_NET].NumUsrs +
		Gbl.Usrs.LstUsrs[Rol_TCH].NumUsrs;
   else
      NumUsrs = Usr_GetNumUsrsInCrss (Gbl.Scope.Current,
				     (Gbl.Scope.Current == HieLvl_CTY ? Gbl.Hierarchy.Cty.CtyCod :
				     (Gbl.Scope.Current == HieLvl_INS ? Gbl.Hierarchy.Ins.InsCod :
				     (Gbl.Scope.Current == HieLvl_CTR ? Gbl.Hierarchy.Ctr.CtrCod :
				     (Gbl.Scope.Current == HieLvl_DEG ? Gbl.Hierarchy.Deg.DegCod :
				     (Gbl.Scope.Current == HieLvl_CRS ? Gbl.Hierarchy.Crs.CrsCod :
								     -1L))))),
				      1 << Rol_NET |
				      1 << Rol_TCH);

   /***** Begin box *****/
   Box_BoxBegin (NULL,Txt_ROLES_PLURAL_Abc[Rol_TCH][Usr_SEX_UNKNOWN],
                 Usr_PutIconsListTchs,NULL,
		 Hlp_USERS_Teachers,Box_NOT_CLOSABLE);

   /***** Form to select scope *****/
   HTM_DIV_Begin ("class=\"CM\"");
   Frm_BeginForm (ActLstTch);
   Usr_PutParamsPrefsAboutUsrList ();
   HTM_LABEL_Begin ("class=\"%s\"",The_ClassFormInBox[Gbl.Prefs.Theme]);
   HTM_TxtColonNBSP (Txt_Scope);
   Sco_PutSelectorScope ("ScopeUsr",HTM_SUBMIT_ON_CHANGE);
   HTM_LABEL_End ();
   Frm_EndForm ();
   HTM_DIV_End ();

   /***** Form to select groups *****/
   if (Gbl.Scope.Current == HieLvl_CRS)
      Grp_ShowFormToSelectSeveralGroups (Sco_PutParamCurrentScope,&Gbl.Scope.Current,
                                         Grp_MY_GROUPS);

   /***** Begin section with user list *****/
   HTM_SECTION_Begin (Usr_USER_LIST_SECTION_ID);

   if (NumUsrs)
     {
      if (Usr_GetIfShowBigList (NumUsrs,
	                        Sco_PutParamCurrentScope,&Gbl.Scope.Current,
	                        NULL))
        {
	 /***** Form to select type of list of users *****/
	 Usr_ShowFormsToSelectUsrListType (Sco_PutParamCurrentScope,&Gbl.Scope.Current);

         /***** Draw a class photo with teachers of the course *****/
         if (Gbl.Usrs.Me.ListType == Usr_LIST_AS_CLASS_PHOTO)
	    Lay_WriteHeaderClassPhoto (false,true,
				       (Gbl.Scope.Current == HieLvl_CRS ||
					Gbl.Scope.Current == HieLvl_DEG ||
					Gbl.Scope.Current == HieLvl_CTR ||
					Gbl.Scope.Current == HieLvl_INS) ? Gbl.Hierarchy.Ins.InsCod :
									-1L,
				       (Gbl.Scope.Current == HieLvl_CRS ||
					Gbl.Scope.Current == HieLvl_DEG) ? Gbl.Hierarchy.Deg.DegCod :
									-1L,
					Gbl.Scope.Current == HieLvl_CRS  ? Gbl.Hierarchy.Crs.CrsCod :
									-1L);

         /* Set options allowed */
         PutForm = Usr_SetOptionsListUsrsAllowed (Rol_TCH,ICanChooseOption);

         /* Begin form */
         if (PutForm)
           {
            Frm_BeginForm (ActDoActOnSevTch);
	    Grp_PutParamsCodGrps ();
           }

         /* Begin table */
         HTM_TABLE_BeginWide ();

         /***** Draw the classphoto/list  *****/
         switch (Gbl.Usrs.Me.ListType)
           {
            case Usr_LIST_AS_CLASS_PHOTO:
               /* List teachers and non-editing teachers */
               Usr_DrawClassPhoto (Usr_CLASS_PHOTO_SEL_SEE,
        	                   Rol_TCH,&Gbl.Usrs.Selected,
				   PutForm);	// Put checkbox to select users?
               Usr_DrawClassPhoto (Usr_CLASS_PHOTO_SEL_SEE,
        	                   Rol_NET,&Gbl.Usrs.Selected,
				   PutForm);	// Put checkbox to select users?
               break;
            case Usr_LIST_AS_LISTING:
	       /* Initialize field names */
	       Usr_SetUsrDatMainFieldNames ();

	       /* List teachers and non-editing teachers */
               Usr_ListMainDataTchs (Rol_TCH,
        			     PutForm);	// Put checkbox to select users?
               Usr_ListMainDataTchs (Rol_NET,
        			     PutForm);	// Put checkbox to select users?
               break;
            default:
               break;
           }

         /* End table */
         HTM_TABLE_End ();

	 /***** Which action, show records, follow...? *****/
         if (PutForm)
           {
	    Usr_PutOptionsListUsrs (ICanChooseOption);
	    Frm_EndForm ();
           }
	}
     }
   else	// NumUsrs == 0
      /***** Show warning indicating no teachers found *****/
      Usr_ShowWarningNoUsersFound (Rol_TCH);

   /***** End section with user list *****/
   HTM_SECTION_End ();

   /***** End box *****/
   Box_BoxEnd ();

   /***** Free memory for teachers lists *****/
   Usr_FreeUsrsList (Rol_TCH);	// Teachers
   Usr_FreeUsrsList (Rol_NET);	// Non-editing teachers

   /***** Free memory for list of selected groups *****/
   Grp_FreeListCodSelectedGrps ();
  }

/*****************************************************************************/
/**************** Set allowed options to do with several users ***************/
/*****************************************************************************/
// Returns true if any option is allowed

static bool Usr_SetOptionsListUsrsAllowed (Rol_Role_t UsrsRole,
                                           bool ICanChooseOption[Usr_LIST_USRS_NUM_OPTIONS])
  {
   Usr_ListUsrsOption_t Opt;
   bool OptionsAllowed;

   /***** Check which options I can choose *****/
   /* Set default (I can not choose options) */
   for (Opt  = (Usr_ListUsrsOption_t) 1;	// Skip unknown option
	Opt <= (Usr_ListUsrsOption_t) (Usr_LIST_USRS_NUM_OPTIONS - 1);
	Opt++)
      ICanChooseOption[Opt] = false;

   /* Activate some options depending on users' role, on my role, etc. */
   switch (UsrsRole)
     {
      case Rol_GST:
	 ICanChooseOption[Usr_OPTION_RECORDS]    = (Gbl.Usrs.Me.Role.Logged == Rol_SYS_ADM);
	 break;
      case Rol_STD:
	 ICanChooseOption[Usr_OPTION_RECORDS]    =
	 ICanChooseOption[Usr_OPTION_MESSAGE]    =
	 ICanChooseOption[Usr_OPTION_FOLLOW]     =
	 ICanChooseOption[Usr_OPTION_UNFOLLOW]   = (Gbl.Scope.Current == HieLvl_CRS &&
						    (Gbl.Usrs.Me.Role.Logged == Rol_STD ||
						     Gbl.Usrs.Me.Role.Logged == Rol_NET ||
						     Gbl.Usrs.Me.Role.Logged == Rol_TCH ||
						     Gbl.Usrs.Me.Role.Logged == Rol_SYS_ADM));

         ICanChooseOption[Usr_OPTION_HOMEWORK]   =
         ICanChooseOption[Usr_OPTION_ATTENDANCE] =
         ICanChooseOption[Usr_OPTION_EMAIL]      = (Gbl.Scope.Current == HieLvl_CRS &&
						    (Gbl.Usrs.Me.Role.Logged == Rol_NET ||
						     Gbl.Usrs.Me.Role.Logged == Rol_TCH ||
						     Gbl.Usrs.Me.Role.Logged == Rol_SYS_ADM));
	 break;
      case Rol_TCH:
	 ICanChooseOption[Usr_OPTION_RECORDS]    =
	 ICanChooseOption[Usr_OPTION_MESSAGE]    =
	 ICanChooseOption[Usr_OPTION_EMAIL]      =
	 ICanChooseOption[Usr_OPTION_FOLLOW]     =
	 ICanChooseOption[Usr_OPTION_UNFOLLOW]   = (Gbl.Scope.Current == HieLvl_CRS &&
						    (Gbl.Usrs.Me.Role.Logged == Rol_STD ||
						     Gbl.Usrs.Me.Role.Logged == Rol_NET ||
						     Gbl.Usrs.Me.Role.Logged == Rol_TCH ||
						     Gbl.Usrs.Me.Role.Logged == Rol_SYS_ADM));
         ICanChooseOption[Usr_OPTION_HOMEWORK]   = (Gbl.Scope.Current == HieLvl_CRS &&
						    (Gbl.Usrs.Me.Role.Logged == Rol_NET ||
						     Gbl.Usrs.Me.Role.Logged == Rol_TCH ||
						     Gbl.Usrs.Me.Role.Logged == Rol_SYS_ADM));
	 break;
      default:
	 return false;
     }

   /***** Count allowed options *****/
   OptionsAllowed = false;
   for (Opt  = (Usr_ListUsrsOption_t) 1;	// Skip unknown option
	!OptionsAllowed &&
	Opt <= (Usr_ListUsrsOption_t) (Usr_LIST_USRS_NUM_OPTIONS - 1);
	Opt++)
      if (ICanChooseOption[Opt])
	 OptionsAllowed = true;

   return OptionsAllowed;
  }

/*****************************************************************************/
/*************** Put different options to do with several users **************/
/*****************************************************************************/
// Returns true if at least one action can be shown

static void Usr_PutOptionsListUsrs (const bool ICanChooseOption[Usr_LIST_USRS_NUM_OPTIONS])
  {
   extern const char *The_ClassFormInBox[The_NUM_THEMES];
   extern const char *Txt_View_records;
   extern const char *Txt_View_homework;
   extern const char *Txt_View_attendance;
   extern const char *Txt_Send_message;
   extern const char *Txt_Create_email_message;
   extern const char *Txt_Follow;
   extern const char *Txt_Unfollow;
   extern const char *Txt_Continue;
   const char *Label[Usr_LIST_USRS_NUM_OPTIONS] =
     {
      [Usr_OPTION_UNKNOWN   ] = NULL,
      [Usr_OPTION_RECORDS   ] = Txt_View_records,
      [Usr_OPTION_HOMEWORK  ] = Txt_View_homework,
      [Usr_OPTION_ATTENDANCE] = Txt_View_attendance,
      [Usr_OPTION_MESSAGE   ] = Txt_Send_message,
      [Usr_OPTION_EMAIL     ] = Txt_Create_email_message,
      [Usr_OPTION_FOLLOW    ] = Txt_Follow,
      [Usr_OPTION_UNFOLLOW  ] = Txt_Unfollow,
     };
   Usr_ListUsrsOption_t Opt;

   /***** Get the selected option from form *****/
   Gbl.Usrs.Selected.Option = Usr_GetListUsrsOption (Usr_LIST_USRS_DEFAULT_OPTION);

   /***** Write list of options *****/
   /* Begin list of options */
   HTM_UL_Begin ("class=\"LIST_LEFT %s\"",The_ClassFormInBox[Gbl.Prefs.Theme]);

      /* Show option items */
      for (Opt  = (Usr_ListUsrsOption_t) 1;	// Skip unknown option
	   Opt <= (Usr_ListUsrsOption_t) (Usr_LIST_USRS_NUM_OPTIONS - 1);
	   Opt++)
	 if (ICanChooseOption[Opt])
	    Usr_ShowOneListUsrsOption (Opt,Label[Opt]);

   /* End list of options */
   HTM_UL_End ();

   /***** Put button to confirm *****/
   Btn_PutConfirmButton (Txt_Continue);
  }

/*****************************************************************************/
/************ Put start/end of action to register/remove one user ************/
/*****************************************************************************/

static void Usr_ShowOneListUsrsOption (Usr_ListUsrsOption_t ListUsrsAction,
                                       const char *Label)
  {
   HTM_LI_Begin (NULL);
   HTM_LABEL_Begin (NULL);
   HTM_INPUT_RADIO ("ListUsrsAction",false,
		    "value=\"%u\"%s",
		    (unsigned) ListUsrsAction,
		    ListUsrsAction == Gbl.Usrs.Selected.Option ? " checked=\"checked\"" : "");
   HTM_Txt (Label);
   HTM_LABEL_End ();
   HTM_LI_End ();
  }

/*****************************************************************************/
/********************** Do action on several students ************************/
/*****************************************************************************/

void Usr_DoActionOnSeveralUsrs1 (void)
  {
   extern const char *Txt_You_must_select_one_ore_more_users;

   /***** Get parameters from form *****/
   /* Get list of selected users */
   Usr_GetListsSelectedEncryptedUsrsCods (&Gbl.Usrs.Selected);

   /* Check if there are selected users */
   if (Usr_CheckIfThereAreUsrsInListOfSelectedEncryptedUsrCods (&Gbl.Usrs.Selected))
     {
      /* Get the action to do */
      Gbl.Usrs.Selected.Option = Usr_GetListUsrsOption (Usr_OPTION_UNKNOWN);

      /***** Change action depending on my selection *****/
      Gbl.Action.Original = Gbl.Action.Act;	// To check if action changes

      switch (Gbl.Usrs.Selected.Option)
	{
	 case Usr_OPTION_RECORDS:
	    switch (Gbl.Action.Act)
	      {
	       case ActDoActOnSevGst:
		  Gbl.Action.Act = ActSeeRecSevGst;
		  break;
	       case ActDoActOnSevStd:
		  Gbl.Action.Act = ActSeeRecSevStd;
		  break;
	       case ActDoActOnSevTch:
		  Gbl.Action.Act = ActSeeRecSevTch;
		  break;
	       default:
		  break;
	      }
	    break;
	 case Usr_OPTION_HOMEWORK:
	    switch (Gbl.Action.Act)
	      {
	       case ActDoActOnSevStd:
	       case ActDoActOnSevTch:
		  Gbl.Action.Act = ActAdmAsgWrkCrs;
		  break;
	       default:
		  break;
	      }
	    break;
	 case Usr_OPTION_ATTENDANCE:
	    switch (Gbl.Action.Act)
	      {
	       case ActDoActOnSevStd:
		  Gbl.Action.Act = ActSeeLstUsrAtt;
		  break;
	       default:
		  break;
	      }
	    break;
	 case Usr_OPTION_MESSAGE:
	    switch (Gbl.Action.Act)
	      {
	       case ActDoActOnSevStd:
	       case ActDoActOnSevTch:
		  Gbl.Action.Act = ActReqMsgUsr;
		  break;
	       default:
		  break;
	      }
	    break;
	 case Usr_OPTION_EMAIL:
	    switch (Gbl.Action.Act)
	      {
	       case ActDoActOnSevStd:
		  Gbl.Action.Act = ActMaiUsr;
		  break;
	       default:
		  break;
	      }
	    break;
	 case Usr_OPTION_FOLLOW:
	    switch (Gbl.Action.Act)
	      {
	       case ActDoActOnSevStd:
		  Gbl.Action.Act = ActReqFolSevStd;
		  break;
	       case ActDoActOnSevTch:
		  Gbl.Action.Act = ActReqFolSevTch;
		  break;
	       default:
		  break;
	      }
	    break;
	 case Usr_OPTION_UNFOLLOW:
	    switch (Gbl.Action.Act)
	      {
	       case ActDoActOnSevStd:
		  Gbl.Action.Act = ActReqUnfSevStd;
		  break;
	       case ActDoActOnSevTch:
		  Gbl.Action.Act = ActReqUnfSevTch;
		  break;
	       default:
		  break;
	      }
	    break;
	 default:
	    break;
	}

      if (Gbl.Action.Act == Gbl.Action.Original)	// Fail, no change in action
	 Ale_CreateAlert (Ale_ERROR,NULL,"Wrong action.");
      else						// Success, action has changed
	 Tab_SetCurrentTab ();
     }
   else							// If no users selected...
      Ale_CreateAlert (Ale_WARNING,NULL,		// ...write warning notice
	               Txt_You_must_select_one_ore_more_users);
  }

void Usr_DoActionOnSeveralUsrs2 (void)
  {
   /***** Show possible alerts *****/
   Ale_ShowAlerts (NULL);

   /***** If success, action has changed.
          No change in action means an error in form has happened,
          so show again the form to selected users *****/
   switch (Gbl.Action.Act)
     {
      case ActDoActOnSevGst:
	 Usr_SeeGuests ();
	 break;
      case ActDoActOnSevStd:
	 Usr_SeeStudents ();
	 break;
      case ActDoActOnSevTch:
	 Usr_SeeTeachers ();
	 break;
      default:
	 break;
     }

   /***** Free memory used by list of selected users' codes *****/
   Usr_FreeListsSelectedEncryptedUsrsCods (&Gbl.Usrs.Selected);
  }

/*****************************************************************************/
/*************** Get action to do with list of selected users ****************/
/*****************************************************************************/

static Usr_ListUsrsOption_t Usr_GetListUsrsOption (Usr_ListUsrsOption_t DefaultAction)
  {
   return (Usr_ListUsrsOption_t) Par_GetParToUnsignedLong ("ListUsrsAction",
							   0,
							   Usr_LIST_USRS_NUM_OPTIONS - 1,
							   (unsigned long) DefaultAction);
  }

/*****************************************************************************/
/***************** Put contextual icons in list of guests ********************/
/*****************************************************************************/

static void Usr_PutIconsListGsts (__attribute__((unused)) void *Args)
  {
   switch (Gbl.Usrs.Me.ListType)
     {
      case Usr_LIST_AS_CLASS_PHOTO:
	 if (Gbl.Usrs.LstUsrs[Rol_GST].NumUsrs)
	    /***** Put icon to print guests *****/
	    Usr_PutIconToPrintGsts ();
	 break;
      case Usr_LIST_AS_LISTING:
	 /***** Put icon to show all data of guests *****/
	 Usr_PutIconToShowGstsAllData ();
	 break;
      default:
	 break;
     }

   /***** Put icon to show a figure *****/
   Fig_PutIconToShowFigure (Fig_USERS);
  }

/*****************************************************************************/
/**************** Put contextual icons in list of students *******************/
/*****************************************************************************/

static void Usr_PutIconsListStds (__attribute__((unused)) void *Args)
  {
   switch (Gbl.Usrs.Me.ListType)
     {
      case Usr_LIST_AS_CLASS_PHOTO:
	 if (Gbl.Usrs.LstUsrs[Rol_STD].NumUsrs)
	    /***** Put icon to print students *****/
	    Usr_PutIconToPrintStds ();
	 break;
      case Usr_LIST_AS_LISTING:
	 /***** Put icon to show all data of students *****/
	 Usr_PutIconToShowStdsAllData ();
	 break;
      default:
	 break;
     }

   /***** Put icon to show a figure *****/
   Fig_PutIconToShowFigure (Fig_USERS);
  }

/*****************************************************************************/
/**************** Put contextual icons in list of teachers *******************/
/*****************************************************************************/

static void Usr_PutIconsListTchs (__attribute__((unused)) void *Args)
  {
   switch (Gbl.Usrs.Me.ListType)
     {
      case Usr_LIST_AS_CLASS_PHOTO:
	 if (Gbl.Usrs.LstUsrs[Rol_TCH].NumUsrs)
	    /***** Put icon to print teachers *****/
	    Usr_PutIconToPrintTchs ();
	 break;
      case Usr_LIST_AS_LISTING:
	 /***** Put icon to show all data of teachers *****/
	 Usr_PutIconToShowTchsAllData ();
	 break;
      default:
	 break;
     }

   /***** Put icon to show a figure *****/
   Fig_PutIconToShowFigure (Fig_USERS);
  }

/*****************************************************************************/
/***************** Functions used to print lists of users ********************/
/*****************************************************************************/

static void Usr_PutIconToPrintGsts (void)
  {
   Ico_PutContextualIconToPrint (ActPrnGstPho,
                                 Usr_ShowGstsAllDataParams,NULL);
  }

static void Usr_PutIconToPrintStds (void)
  {
   Ico_PutContextualIconToPrint (ActPrnStdPho,
                                 Usr_ShowStdsAllDataParams,NULL);
  }

static void Usr_PutIconToPrintTchs (void)
  {
   Ico_PutContextualIconToPrint (ActPrnTchPho,
                                 Usr_ShowTchsAllDataParams,NULL);
  }

/*****************************************************************************/
/**************** Functions used to list all data of users *******************/
/*****************************************************************************/

static void Usr_PutIconToShowGstsAllData (void)
  {
   extern const char *Txt_Show_all_data_in_a_table;

   Lay_PutContextualLinkOnlyIcon (ActLstGstAll,NULL,
                                  Usr_ShowGstsAllDataParams,NULL,
				  "table.svg",
				  Txt_Show_all_data_in_a_table);
  }

static void Usr_PutIconToShowStdsAllData (void)
  {
   extern const char *Txt_Show_all_data_in_a_table;

   Lay_PutContextualLinkOnlyIcon (ActLstStdAll,NULL,
                                  Usr_ShowStdsAllDataParams,NULL,
			          "table.svg",
				  Txt_Show_all_data_in_a_table);
  }

static void Usr_PutIconToShowTchsAllData (void)
  {
   extern const char *Txt_Show_all_data_in_a_table;

   Lay_PutContextualLinkOnlyIcon (ActLstTchAll,NULL,
                                  Usr_ShowTchsAllDataParams,NULL,
			          "table.svg",
				  Txt_Show_all_data_in_a_table);
  }

static void Usr_ShowGstsAllDataParams (__attribute__((unused)) void *Args)
  {
   Usr_PutParamListWithPhotos ();
  }

static void Usr_ShowStdsAllDataParams (__attribute__((unused)) void *Args)
  {
   Grp_PutParamsCodGrps ();
   Usr_PutParamListWithPhotos ();
  }

static void Usr_ShowTchsAllDataParams (__attribute__((unused)) void *Args)
  {
   Sco_PutParamCurrentScope (&Gbl.Scope.Current);
   Usr_PutParamListWithPhotos ();
  }

/*****************************************************************************/
/************* Draw class photo with guests ready to be printed **************/
/*****************************************************************************/

void Usr_SeeGstClassPhotoPrn (void)
  {
   /***** Get and update type of list,
          number of columns in class photo
          and preference about view photos *****/
   Usr_GetAndUpdatePrefsAboutUsrList ();

   /***** Get scope *****/
   Sco_SetScopesForListingGuests ();
   Sco_GetScope ("ScopeUsr");

   /***** Get list of guests *****/
   Usr_GetGstsLst (Gbl.Scope.Current);

   if (Gbl.Usrs.LstUsrs[Rol_GST].NumUsrs)
     {
      /***** Draw the guests' class photo *****/
      Lay_WriteHeaderClassPhoto (true,true,
				 (Gbl.Scope.Current == HieLvl_CTR ||
				  Gbl.Scope.Current == HieLvl_INS) ? Gbl.Hierarchy.Ins.InsCod :
                                                                        -1L,
				 -1L,-1L);
      HTM_TABLE_BeginWide ();
      Usr_DrawClassPhoto (Usr_CLASS_PHOTO_PRN,
                          Rol_GST,&Gbl.Usrs.Selected,false);
      HTM_TABLE_End ();
     }
   else	// Gbl.Usrs.LstUsrs[Rol_GST].NumUsrs
      /***** Show warning indicating no guests found *****/
      Usr_ShowWarningNoUsersFound (Rol_GST);

   /***** Free memory for guests list *****/
   Usr_FreeUsrsList (Rol_GST);
  }

/*****************************************************************************/
/**************** Draw class photo with students ready to print **************/
/*****************************************************************************/

void Usr_SeeStdClassPhotoPrn (void)
  {
   /***** Get and update type of list,
          number of columns in class photo
          and preference about view photos *****/
   Usr_GetAndUpdatePrefsAboutUsrList ();

   /***** Get scope *****/
   Sco_SetScopesForListingStudents ();
   Sco_GetScope ("ScopeUsr");

   /****** Get groups to show ******/
   Grp_GetParCodsSeveralGrpsToShowUsrs ();

   /***** Get list of students *****/
   Usr_GetListUsrs (Gbl.Scope.Current,Rol_STD);

   if (Gbl.Usrs.LstUsrs[Rol_STD].NumUsrs)
     {
      /***** Draw the students' class photo *****/
      Lay_WriteHeaderClassPhoto (true,true,
				 (Gbl.Scope.Current == HieLvl_CRS ||
				  Gbl.Scope.Current == HieLvl_DEG ||
				  Gbl.Scope.Current == HieLvl_CTR ||
				  Gbl.Scope.Current == HieLvl_INS) ? Gbl.Hierarchy.Ins.InsCod :
					                                -1L,
				 (Gbl.Scope.Current == HieLvl_CRS ||
				  Gbl.Scope.Current == HieLvl_DEG) ? Gbl.Hierarchy.Deg.DegCod :
					                                -1L,
				  Gbl.Scope.Current == HieLvl_CRS  ? Gbl.Hierarchy.Crs.CrsCod :
					                                -1L);
      HTM_TABLE_BeginWide ();
      Usr_DrawClassPhoto (Usr_CLASS_PHOTO_PRN,
                          Rol_STD,&Gbl.Usrs.Selected,false);
      HTM_TABLE_End ();
     }
   else	// Gbl.Usrs.LstUsrs[Rol_STD].NumUsrs == 0
      /***** Show warning indicating no students found *****/
      Usr_ShowWarningNoUsersFound (Rol_STD);

   /***** Free memory for students list *****/
   Usr_FreeUsrsList (Rol_STD);

   /***** Free memory for list of selected groups *****/
   Grp_FreeListCodSelectedGrps ();
  }

/*****************************************************************************/
/**************** Draw class photo with teachers ready to print **************/
/*****************************************************************************/

void Usr_SeeTchClassPhotoPrn (void)
  {
   unsigned NumUsrs;

   /***** Get and update type of list,
          number of columns in class photo
          and preference about view photos *****/
   Usr_GetAndUpdatePrefsAboutUsrList ();

   /***** Get scope *****/
   Gbl.Scope.Allowed = 1 << HieLvl_SYS |
	               1 << HieLvl_CTY |
                       1 << HieLvl_INS |
                       1 << HieLvl_CTR |
                       1 << HieLvl_DEG |
                       1 << HieLvl_CRS;
   Gbl.Scope.Default = HieLvl_CRS;
   Sco_GetScope ("ScopeUsr");

   /****** Get groups to show ******/
   Grp_GetParCodsSeveralGrpsToShowUsrs ();

   /***** Get list of teachers *****/
   Usr_GetListUsrs (Gbl.Scope.Current,Rol_NET);	// Non-editing teachers
   Usr_GetListUsrs (Gbl.Scope.Current,Rol_TCH);	// Teachers
   if (Gbl.Scope.Current == HieLvl_CRS)
      NumUsrs = Gbl.Usrs.LstUsrs[Rol_NET].NumUsrs +
		Gbl.Usrs.LstUsrs[Rol_TCH].NumUsrs;
   else
      NumUsrs = Usr_GetNumUsrsInCrss (Gbl.Scope.Current,
				     (Gbl.Scope.Current == HieLvl_CTY ? Gbl.Hierarchy.Cty.CtyCod :
				     (Gbl.Scope.Current == HieLvl_INS ? Gbl.Hierarchy.Ins.InsCod :
				     (Gbl.Scope.Current == HieLvl_CTR ? Gbl.Hierarchy.Ctr.CtrCod :
				     (Gbl.Scope.Current == HieLvl_DEG ? Gbl.Hierarchy.Deg.DegCod :
				     (Gbl.Scope.Current == HieLvl_CRS ? Gbl.Hierarchy.Crs.CrsCod :
								     -1L))))),
				      1 << Rol_NET |
				      1 << Rol_TCH);

   if (NumUsrs)
     {
      /***** Draw the teachers' class photo *****/
      Lay_WriteHeaderClassPhoto (true,true,
				 (Gbl.Scope.Current == HieLvl_CRS ||
				  Gbl.Scope.Current == HieLvl_DEG ||
				  Gbl.Scope.Current == HieLvl_CTR ||
				  Gbl.Scope.Current == HieLvl_INS) ? Gbl.Hierarchy.Ins.InsCod :
					                                -1L,
				 (Gbl.Scope.Current == HieLvl_CRS ||
				  Gbl.Scope.Current == HieLvl_DEG) ? Gbl.Hierarchy.Deg.DegCod :
					                                -1L,
				  Gbl.Scope.Current == HieLvl_CRS  ? Gbl.Hierarchy.Crs.CrsCod :
					                                -1L);
      HTM_TABLE_BeginWide ();

      /* List teachers and non-editing teachers */
      Usr_DrawClassPhoto (Usr_CLASS_PHOTO_PRN,
			  Rol_TCH,&Gbl.Usrs.Selected,false);
      Usr_DrawClassPhoto (Usr_CLASS_PHOTO_PRN,
			  Rol_NET,&Gbl.Usrs.Selected,false);

      HTM_TABLE_End ();
     }
   else	// NumUsrs == 0
      /***** Show warning indicating no teachers found *****/
      Usr_ShowWarningNoUsersFound (Rol_TCH);

   /***** Free memory for teachers lists *****/
   Usr_FreeUsrsList (Rol_TCH);	// Teachers
   Usr_FreeUsrsList (Rol_NET);	// Non-editing teachers

   /***** Free memory for list of selected groups *****/
   Grp_FreeListCodSelectedGrps ();
  }

/*****************************************************************************/
/****************** Draw class photo with students or teachers ***************/
/*****************************************************************************/

static void Usr_DrawClassPhoto (Usr_ClassPhotoType_t ClassPhotoType,
                                Rol_Role_t Role,
				struct SelectedUsrs *SelectedUsrs,
				bool PutCheckBoxToSelectUsr)
  {
   unsigned NumUsr;
   bool TRIsOpen = false;
   bool UsrIsTheMsgSender;
   const char *ClassPhoto = "PHOTO21x28";	// Default photo size
   struct UsrData UsrDat;

   if (Gbl.Usrs.LstUsrs[Role].NumUsrs)
     {
      /***** Set width and height of photos *****/
      switch (ClassPhotoType)
	{
	 case Usr_CLASS_PHOTO_SEL:
	    ClassPhoto = "PHOTO21x28";
	    break;
	 case Usr_CLASS_PHOTO_SEL_SEE:
	    ClassPhoto = "PHOTO45x60";
	    break;
	 case Usr_CLASS_PHOTO_PRN:
	    ClassPhoto = "PHOTO45x60";
	    break;
	}

      /***** Put a row to select all users *****/
      if (PutCheckBoxToSelectUsr)
	 Usr_PutCheckboxToSelectAllUsers (Role,SelectedUsrs);

      /***** Initialize structure with user's data *****/
      Usr_UsrDataConstructor (&UsrDat);

      /***** Loop for showing users photos, names and place of birth *****/
      for (NumUsr = 0;
	   NumUsr < Gbl.Usrs.LstUsrs[Role].NumUsrs; )
	{
	 if ((NumUsr % Gbl.Usrs.ClassPhoto.Cols) == 0)
	   {
	    HTM_TR_Begin (NULL);
	    TRIsOpen = true;
	   }

	 /* Copy user's basic data from list */
	 Usr_CopyBasicUsrDataFromList (&UsrDat,&Gbl.Usrs.LstUsrs[Role].Lst[NumUsr]);

	 /* Get list of user's IDs */
	 ID_GetListIDsFromUsrCod (&UsrDat);

	 /***** Begin user's cell *****/
	 if (ClassPhotoType == Usr_CLASS_PHOTO_SEL &&
	     UsrDat.UsrCod == Gbl.Usrs.Other.UsrDat.UsrCod)
	   {
	    UsrIsTheMsgSender = true;
	    HTM_TD_Begin ("class=\"CLASSPHOTO CB LIGHT_GREEN\"");
	   }
	 else
	   {
	    UsrIsTheMsgSender = false;
	    HTM_TD_Begin ("class=\"CLASSPHOTO CB\"");
	   }

	 /***** Checkbox to select this user *****/
	 if (PutCheckBoxToSelectUsr)
	    Usr_PutCheckboxToSelectUser (Role,UsrDat.EnUsrCod,UsrIsTheMsgSender,
					 SelectedUsrs);

	 /***** Show photo *****/
	 Pho_ShowUsrPhotoIfAllowed (&UsrDat,ClassPhoto,Pho_ZOOM,false);

	 /***** Photo foot *****/
	 HTM_DIV_Begin ("class=\"CLASSPHOTO_CAPTION\"");

	 /* Name */
	 if (UsrDat.FrstName[0])
	    HTM_Txt (UsrDat.FrstName);
	 else
	    HTM_NBSP ();
	 HTM_BR ();
	 if (UsrDat.Surname1[0])
	    HTM_Txt (UsrDat.Surname1);
	 else
	    HTM_NBSP ();
	 HTM_BR ();
	 if (UsrDat.Surname2[0])
	    HTM_Txt (UsrDat.Surname2);
	 else
	    HTM_NBSP ();

	 HTM_DIV_End ();

	 /***** End user's cell *****/
	 HTM_TD_End ();

	 if ((++NumUsr % Gbl.Usrs.ClassPhoto.Cols) == 0)
	   {
	    HTM_TR_End ();
	    TRIsOpen = false;
	   }
	}
      if (TRIsOpen)
	 HTM_TR_End ();

      /***** Free memory used for user's data *****/
      Usr_UsrDataDestructor (&UsrDat);
     }
  }

/*****************************************************************************/
/***************** Write selector of columns in class photo ******************/
/*****************************************************************************/

void Usr_PutSelectorNumColsClassPhoto (void)
  {
   extern const char *The_ClassFormInBox[The_NUM_THEMES];
   extern const char *Txt_columns;
   unsigned Cols;

   /***** Begin selector *****/
   HTM_LABEL_Begin ("class=\"%s\"",The_ClassFormInBox[Gbl.Prefs.Theme]);
      HTM_SELECT_Begin (HTM_SUBMIT_ON_CHANGE,
			"name=\"ColsClassPhoto\"");

	 /***** Put a row in selector for every number from 1 to Usr_CLASS_PHOTO_COLS_MAX *****/
	 for (Cols = 1;
	      Cols <= Usr_CLASS_PHOTO_COLS_MAX;
	      Cols++)
	    HTM_OPTION (HTM_Type_UNSIGNED,&Cols,
			Cols == Gbl.Usrs.ClassPhoto.Cols,false,
			"%u",Cols);

      /***** End selector *****/
      HTM_SELECT_End ();
      HTM_Txt (Txt_columns);
   HTM_LABEL_End ();
  }

/*****************************************************************************/
/********** Build the relative path of a user from his user's code ***********/
/*****************************************************************************/

void Usr_ConstructPathUsr (long UsrCod,char PathUsr[PATH_MAX + 1 + Cns_MAX_DECIMAL_DIGITS_LONG + 1])
  {
   char PathAboveUsr[PATH_MAX + 1];

   /***** Path for users *****/
   Fil_CreateDirIfNotExists (Cfg_PATH_USR_PRIVATE);

   /***** Path above user's ID *****/
   snprintf (PathAboveUsr,sizeof (PathAboveUsr),"%s/%02u",
	     Cfg_PATH_USR_PRIVATE,(unsigned) (UsrCod % 100));
   Fil_CreateDirIfNotExists (PathAboveUsr);

   /***** Path for user *****/
   snprintf (PathUsr,PATH_MAX + 1 + Cns_MAX_DECIMAL_DIGITS_LONG + 1,"%s/%ld",
	     PathAboveUsr,UsrCod);
  }

/*****************************************************************************/
/************** Check if a user exists with a given user's code **************/
/*****************************************************************************/

bool Usr_ChkIfUsrCodExists (long UsrCod)
  {
   /***** Trivial check: user's code should be > 0 *****/
   if (UsrCod <= 0)	// Wrong user's code
      return false;

   /***** Get if a user exists in database *****/
   return (DB_QueryCOUNT ("can not check if a user exists",
			  "SELECT COUNT(*)"
			   " FROM usr_data"
			  " WHERE UsrCod=%ld",
			  UsrCod) != 0);
  }

/*****************************************************************************/
/********************** Show warning "no users found" ************************/
/*****************************************************************************/
// Use Rol_UNK type to display "no users found"

void Usr_ShowWarningNoUsersFound (Rol_Role_t Role)
  {
   extern const char *Txt_No_users_found[Rol_NUM_ROLES];
   extern const char *Txt_Register_students;
   extern const char *Txt_Register_teacher;

   if (Gbl.Usrs.ClassPhoto.AllGroups &&		// All groups selected
       Role == Rol_STD &&			// No students found
       Gbl.Usrs.Me.Role.Logged == Rol_TCH)	// Course selected and I am logged as teacher
      /***** Show alert and button to enrol students *****/
      Ale_ShowAlertAndButton (ActReqEnrSevStd,NULL,NULL,
                              NULL,NULL,
                              Btn_CREATE_BUTTON,Txt_Register_students,
			      Ale_WARNING,Txt_No_users_found[Rol_STD]);

   else if (Gbl.Usrs.ClassPhoto.AllGroups &&		// All groups selected
            Role == Rol_TCH &&				// No teachers found
            Gbl.Hierarchy.Level == HieLvl_CRS &&		// Course selected
            Gbl.Usrs.Me.Role.Logged >= Rol_DEG_ADM)	// I am an administrator
      /***** Show alert and button to enrol students *****/
      Ale_ShowAlertAndButton (ActReqMdfOneTch,NULL,NULL,
                              NULL,NULL,
                              Btn_CREATE_BUTTON,Txt_Register_teacher,
			      Ale_WARNING,Txt_No_users_found[Rol_TCH]);
   else
      /***** Show alert *****/
      Ale_ShowAlert (Ale_INFO,Txt_No_users_found[Role]);
  }

/*****************************************************************************/
/****************** Get total number of users in platform ********************/
/*****************************************************************************/

unsigned Usr_GetTotalNumberOfUsers (void)
  {
   long Cod;
   unsigned Roles;

   /***** Get number of users with events from database *****/
   switch (Gbl.Scope.Current)
     {
      case HieLvl_SYS:
	 return (unsigned) DB_GetNumRowsTable ("usr_data");			// All users in platform
      case HieLvl_CTY:
      case HieLvl_INS:
      case HieLvl_CTR:
      case HieLvl_DEG:
      case HieLvl_CRS:
         Cod = (Gbl.Scope.Current == HieLvl_CTY ? Gbl.Hierarchy.Cty.CtyCod :
	       (Gbl.Scope.Current == HieLvl_INS ? Gbl.Hierarchy.Ins.InsCod :
	       (Gbl.Scope.Current == HieLvl_CTR ? Gbl.Hierarchy.Ctr.CtrCod :
	       (Gbl.Scope.Current == HieLvl_DEG ? Gbl.Hierarchy.Deg.DegCod :
	                                           Gbl.Hierarchy.Crs.CrsCod))));
         Roles = (1 << Rol_STD) |
	         (1 << Rol_NET) |
	         (1 << Rol_TCH);
         return Usr_GetCachedNumUsrsInCrss (Gbl.Scope.Current,Cod,Roles);	// All users in courses
      default:
	 Err_WrongScopeExit ();
	 return 0;	// Not reached
     }
  }

/*****************************************************************************/
/******* Get total number of users of one or several roles in courses ********/
/*****************************************************************************/

#define Usr_MAX_BYTES_SUBQUERY_ROLES (Rol_NUM_ROLES * (10 + 1) - 1)

unsigned Usr_GetNumUsrsInCrss (HieLvl_Level_t Scope,long Cod,unsigned Roles)
  {
   char UnsignedStr[Cns_MAX_DECIMAL_DIGITS_UINT + 1];
   char SubQueryRoles[Usr_MAX_BYTES_SUBQUERY_ROLES + 1];
   bool AnyUserInCourses;
   Rol_Role_t Role;
   Rol_Role_t FirstRoleRequested;
   bool MoreThanOneRole;
   bool FirstRole;
   unsigned NumUsrs;

   /***** Reset roles that can not belong to courses.
          Only
          - students,
          - non-editing teachers,
          - teachers
          can belong to a course *****/
   Roles &= ((1 << Rol_STD) |
	     (1 << Rol_NET) |
	     (1 << Rol_TCH));

   /***** Check if no roles requested *****/
   if (Roles == 0)
      return 0;

   /***** Check if any user in courses is requested *****/
   AnyUserInCourses = (Roles == ((1 << Rol_STD) |
	                         (1 << Rol_NET) |
	                         (1 << Rol_TCH)));

   /***** Get first role requested *****/
   FirstRoleRequested = Rol_UNK;
   for (Role  = Rol_STD;
        Role <= Rol_TCH;
        Role++)
      if (Roles & (1 << Role))
	{
	 FirstRoleRequested = Role;
	 break;
	}

   /***** Check if more than one role is requested *****/
   MoreThanOneRole = false;
   if (FirstRoleRequested != Rol_UNK)
      for (Role = FirstRoleRequested + 1;
	   Role <= Rol_TCH;
	   Role++)
	 if (Roles & (1 << Role))
	   {
	    MoreThanOneRole = true;
	    break;
	   }

   /***** Build subquery for roles *****/
   if (MoreThanOneRole)
     {
      Str_Copy (SubQueryRoles," IN (",sizeof (SubQueryRoles) - 1);
      for (Role  = Rol_STD, FirstRole = true;
	   Role <= Rol_TCH;
	   Role++)
	 if (Roles & (1 << Role))
	   {
	    snprintf (UnsignedStr,sizeof (UnsignedStr),"%u",(unsigned) Role);
	    if (FirstRole)	// Not the first role
	       FirstRole = false;
	    else
	       Str_Concat (SubQueryRoles,",",sizeof (SubQueryRoles) - 1);
	    Str_Concat (SubQueryRoles,UnsignedStr,sizeof (SubQueryRoles) - 1);
	   }
      Str_Concat (SubQueryRoles,")",sizeof (SubQueryRoles) - 1);
     }
   else	// Only one role
      sprintf (SubQueryRoles,"=%u",FirstRoleRequested);

   /***** Get number of users from database *****/
   switch (Scope)
     {
      case HieLvl_SYS:
         if (AnyUserInCourses)	// Any user
            NumUsrs = (unsigned)
            DB_QueryCOUNT ("can not get number of users",
			   "SELECT COUNT(DISTINCT UsrCod)"
			    " FROM crs_users");
         else
            NumUsrs = (unsigned)
            DB_QueryCOUNT ("can not get number of users",
			   "SELECT COUNT(DISTINCT UsrCod)"
			    " FROM crs_users"
			   " WHERE Role"
			     "%s",
			   SubQueryRoles);
         break;
      case HieLvl_CTY:
         if (AnyUserInCourses)	// Any user
            NumUsrs = (unsigned)
            DB_QueryCOUNT ("can not get number of users",
			   "SELECT COUNT(DISTINCT crs_users.UsrCod)"
			    " FROM ins_instits,"
				  "ctr_centers,"
				  "deg_degrees,"
				  "crs_courses,"
				  "crs_users"
			   " WHERE ins_instits.CtyCod=%ld"
			     " AND ins_instits.InsCod=ctr_centers.InsCod"
			     " AND ctr_centers.CtrCod=deg_degrees.CtrCod"
			     " AND deg_degrees.DegCod=crs_courses.DegCod"
			     " AND crs_courses.CrsCod=crs_users.CrsCod",
			   Cod);
         else
            NumUsrs = (unsigned)
            DB_QueryCOUNT ("can not get number of users",
			   "SELECT COUNT(DISTINCT crs_users.UsrCod)"
			    " FROM ins_instits,"
				  "ctr_centers,"
				  "deg_degrees,"
				  "crs_courses,"
				  "crs_users"
			   " WHERE ins_instits.CtyCod=%ld"
			     " AND ins_instits.InsCod=ctr_centers.InsCod"
			     " AND ctr_centers.CtrCod=deg_degrees.CtrCod"
			     " AND deg_degrees.DegCod=crs_courses.DegCod"
			     " AND crs_courses.CrsCod=crs_users.CrsCod"
			     " AND crs_users.Role"
			     "%s",
			   Cod,
			   SubQueryRoles);
         break;
      case HieLvl_INS:
         if (AnyUserInCourses)	// Any user
            NumUsrs = (unsigned)
            DB_QueryCOUNT ("can not get number of users",
			   "SELECT COUNT(DISTINCT crs_users.UsrCod)"
			    " FROM ctr_centers,"
				  "deg_degrees,"
				  "crs_courses,"
				  "crs_users"
			   " WHERE ctr_centers.InsCod=%ld"
			     " AND ctr_centers.CtrCod=deg_degrees.CtrCod"
			     " AND deg_degrees.DegCod=crs_courses.DegCod"
			     " AND crs_courses.CrsCod=crs_users.CrsCod",
			   Cod);
         else
            NumUsrs = (unsigned)
            DB_QueryCOUNT ("can not get number of users",
			   "SELECT COUNT(DISTINCT crs_users.UsrCod)"
			    " FROM ctr_centers,"
				  "deg_degrees,"
				  "crs_courses,"
				  "crs_users"
			   " WHERE ctr_centers.InsCod=%ld"
			     " AND ctr_centers.CtrCod=deg_degrees.CtrCod"
			     " AND deg_degrees.DegCod=crs_courses.DegCod"
			     " AND crs_courses.CrsCod=crs_users.CrsCod"
			     " AND crs_users.Role"
			     "%s",
			   Cod,
			   SubQueryRoles);
         break;
      case HieLvl_CTR:
         if (AnyUserInCourses)	// Any user
            NumUsrs = (unsigned)
            DB_QueryCOUNT ("can not get number of users",
			   "SELECT COUNT(DISTINCT crs_users.UsrCod)"
			    " FROM deg_degrees,"
				  "crs_courses,"
				  "crs_users"
			   " WHERE deg_degrees.CtrCod=%ld"
			     " AND deg_degrees.DegCod=crs_courses.DegCod"
			     " AND crs_courses.CrsCod=crs_users.CrsCod",
			   Cod);
         else
            NumUsrs = (unsigned)
            DB_QueryCOUNT ("can not get number of users",
			   "SELECT COUNT(DISTINCT crs_users.UsrCod)"
			    " FROM deg_degrees,"
				  "crs_courses,"
				  "crs_users"
			   " WHERE deg_degrees.CtrCod=%ld"
			     " AND deg_degrees.DegCod=crs_courses.DegCod"
			     " AND crs_courses.CrsCod=crs_users.CrsCod"
			     " AND crs_users.Role"
			     "%s",
			   Cod,
			   SubQueryRoles);
         break;
      case HieLvl_DEG:
         if (AnyUserInCourses)	// Any user
            NumUsrs = (unsigned)
            DB_QueryCOUNT ("can not get number of users",
			   "SELECT COUNT(DISTINCT crs_users.UsrCod)"
			    " FROM crs_courses,"
			  	  "crs_users"
			   " WHERE crs_courses.DegCod=%ld"
			     " AND crs_courses.CrsCod=crs_users.CrsCod",
			   Cod);
         else
            NumUsrs = (unsigned)
            DB_QueryCOUNT ("can not get number of users",
			   "SELECT COUNT(DISTINCT crs_users.UsrCod)"
			    " FROM crs_courses,"
				  "crs_users"
			   " WHERE crs_courses.DegCod=%ld"
			     " AND crs_courses.CrsCod=crs_users.CrsCod"
			     " AND crs_users.Role%s",
			  Cod,SubQueryRoles);
         break;
      case HieLvl_CRS:
         if (AnyUserInCourses)	// Any user
            NumUsrs = (unsigned)
            DB_QueryCOUNT ("can not get number of users",
			   "SELECT COUNT(DISTINCT UsrCod)"
			    " FROM crs_users"
			   " WHERE CrsCod=%ld",
			   Cod);
         else
            NumUsrs = (unsigned)
            DB_QueryCOUNT ("can not get number of users",
			   "SELECT COUNT(DISTINCT UsrCod)"
			    " FROM crs_users"
			   " WHERE CrsCod=%ld"
			     " AND Role"
			     "%s",
			   Cod,
			   SubQueryRoles);
         break;
      default:
	 Err_WrongScopeExit ();
	 NumUsrs = 0;	// Not reached. Initialized to avoid warning.
	 break;
     }

   FigCch_UpdateFigureIntoCache (Usr_GetFigureNumUsrsInCrss (Roles),Scope,Cod,
				 FigCch_UNSIGNED,&NumUsrs);
   return NumUsrs;
  }

unsigned Usr_GetCachedNumUsrsInCrss (HieLvl_Level_t Scope,long Cod,unsigned Roles)
  {
   unsigned NumUsrsInCrss;

   /***** Get number of users in courses from cache *****/
   if (!FigCch_GetFigureFromCache (Usr_GetFigureNumUsrsInCrss (Roles),Scope,Cod,
                                   FigCch_UNSIGNED,&NumUsrsInCrss))
      /***** Get current number of users in courses from database and update cache *****/
      NumUsrsInCrss = Usr_GetNumUsrsInCrss (Scope,Cod,Roles);

   return NumUsrsInCrss;
  }

static FigCch_FigureCached_t Usr_GetFigureNumUsrsInCrss (unsigned Roles)
  {
   switch (Roles)
     {
      case 1 << Rol_STD:	// Students
	 return FigCch_NUM_STDS_IN_CRSS;
      case 1 << Rol_NET:	// Non-editing teachers
	 return FigCch_NUM_NETS_IN_CRSS;
      case 1 << Rol_TCH:	// Teachers
	 return FigCch_NUM_TCHS_IN_CRSS;
      case 1 << Rol_NET |
	   1 << Rol_TCH:	// Any teacher in courses
	 return FigCch_NUM_ALLT_IN_CRSS;
      case 1 << Rol_STD |
	   1 << Rol_NET |
	   1 << Rol_TCH:	// Any user in courses
	 return FigCch_NUM_USRS_IN_CRSS;
      default:
	 Err_WrongRoleExit ();
	 return FigCch_UNKNOWN;	// Not reached
     }
  }

/*****************************************************************************/
/******** Get total number of users who do not belong to any course **********/
/*****************************************************************************/

unsigned Usr_GetCachedNumUsrsNotBelongingToAnyCrs (void)
  {
   unsigned NumGsts;

   /***** Get number of guests from cache *****/
   if (!FigCch_GetFigureFromCache (FigCch_NUM_GSTS,HieLvl_SYS,-1L,
                                   FigCch_UNSIGNED,&NumGsts))
     {
      /***** Get current number of guests from database and update cache *****/
      NumGsts = (unsigned)
      DB_QueryCOUNT ("can not get number of users"
		     " who do not belong to any course",
		     "SELECT COUNT(*)"
		      " FROM usr_data"
		     " WHERE UsrCod NOT IN"
			   " (SELECT DISTINCT(UsrCod)"
			      " FROM crs_users)");
      FigCch_UpdateFigureIntoCache (FigCch_NUM_GSTS,HieLvl_SYS,-1L,
                                    FigCch_UNSIGNED,&NumGsts);
     }

   return NumGsts;
  }

/*****************************************************************************/
/************ Get average number of courses with users of a role *************/
/*****************************************************************************/

static double Usr_GetNumCrssPerUsr (HieLvl_Level_t Scope,long Cod,Rol_Role_t Role)
  {
   /***** Get number of courses per user from database *****/
   switch (Scope)
     {
      case HieLvl_SYS:
	 if (Role == Rol_UNK)	// Any user
	    return DB_QuerySELECTDouble ("can not get number of courses per user",
					 "SELECT AVG(NumCrss)"
					  " FROM (SELECT COUNT(CrsCod) AS NumCrss"
						  " FROM crs_users"
						 " GROUP BY UsrCod) AS NumCrssTable");
	 else
	    return DB_QuerySELECTDouble ("can not get number of courses per user",
					 "SELECT AVG(NumCrss)"
					  " FROM (SELECT COUNT(CrsCod) AS NumCrss"
						  " FROM crs_users"
						 " WHERE Role=%u"
						 " GROUP BY UsrCod) AS NumCrssTable",
					 (unsigned) Role);
      case HieLvl_CTY:
	 if (Role == Rol_UNK)	// Any user
	    return DB_QuerySELECTDouble ("can not get number of courses per user",
					 "SELECT AVG(NumCrss)"
					  " FROM (SELECT COUNT(crs_users.CrsCod) AS NumCrss"
						  " FROM ins_instits,"
						        "ctr_centers,"
						        "deg_degrees,"
						        "crs_courses,"
						        "crs_users"
						 " WHERE ins_instits.CtyCod=%ld"
						   " AND ins_instits.InsCod=ctr_centers.InsCod"
						   " AND ctr_centers.CtrCod=deg_degrees.CtrCod"
						   " AND deg_degrees.DegCod=crs_courses.DegCod"
						   " AND crs_courses.CrsCod=crs_users.CrsCod"
						 " GROUP BY crs_users.UsrCod) AS NumCrssTable",
					 Cod);
	 else
	    return DB_QuerySELECTDouble ("can not get number of courses per user",
					 "SELECT AVG(NumCrss)"
					  " FROM (SELECT COUNT(crs_users.CrsCod) AS NumCrss"
						  " FROM ins_instits,"
						        "ctr_centers,"
						        "deg_degrees,"
						        "crs_courses,"
						        "crs_users"
						 " WHERE ins_instits.CtyCod=%ld"
						   " AND ins_instits.InsCod=ctr_centers.InsCod"
						   " AND ctr_centers.CtrCod=deg_degrees.CtrCod"
						   " AND deg_degrees.DegCod=crs_courses.DegCod"
						   " AND crs_courses.CrsCod=crs_users.CrsCod"
						   " AND crs_users.Role=%u"
						 " GROUP BY crs_users.UsrCod) AS NumCrssTable",
					 Cod,
					 (unsigned) Role);
      case HieLvl_INS:
	 if (Role == Rol_UNK)	// Any user
	    return DB_QuerySELECTDouble ("can not get number of courses per user",
					 "SELECT AVG(NumCrss)"
					  " FROM (SELECT COUNT(crs_users.CrsCod) AS NumCrss"
						  " FROM ctr_centers,"
						        "deg_degrees,"
						        "crs_courses,"
						        "crs_users"
						 " WHERE ctr_centers.InsCod=%ld"
						   " AND ctr_centers.CtrCod=deg_degrees.CtrCod"
						   " AND deg_degrees.DegCod=crs_courses.DegCod"
						   " AND crs_courses.CrsCod=crs_users.CrsCod"
						 " GROUP BY crs_users.UsrCod) AS NumCrssTable",
					 Cod);
	 else
	    return DB_QuerySELECTDouble ("can not get number of courses per user",
					 "SELECT AVG(NumCrss)"
					  " FROM (SELECT COUNT(crs_users.CrsCod) AS NumCrss"
						  " FROM ctr_centers,"
						        "deg_degrees,"
						        "crs_courses,"
						        "crs_users"
						  " WHERE ctr_centers.InsCod=%ld"
						    " AND ctr_centers.CtrCod=deg_degrees.CtrCod"
						    " AND deg_degrees.DegCod=crs_courses.DegCod"
						    " AND crs_courses.CrsCod=crs_users.CrsCod"
						    " AND crs_users.Role=%u"
						  " GROUP BY crs_users.UsrCod) AS NumCrssTable",
					 Cod,
					 (unsigned) Role);
      case HieLvl_CTR:
	 if (Role == Rol_UNK)	// Any user
	    return DB_QuerySELECTDouble ("can not get number of courses per user",
					 "SELECT AVG(NumCrss)"
					  " FROM (SELECT COUNT(crs_users.CrsCod) AS NumCrss"
						  " FROM deg_degrees,"
						        "crs_courses,"
						        "crs_users"
						 " WHERE deg_degrees.CtrCod=%ld"
						   " AND deg_degrees.DegCod=crs_courses.DegCod"
						   " AND crs_courses.CrsCod=crs_users.CrsCod"
						 " GROUP BY crs_users.UsrCod) AS NumCrssTable",
					 Cod);
	 else
	    return DB_QuerySELECTDouble ("can not get number of courses per user",
					 "SELECT AVG(NumCrss)"
					  " FROM (SELECT COUNT(crs_users.CrsCod) AS NumCrss"
						  " FROM deg_degrees,"
						        "crs_courses,"
						        "crs_users"
						 " WHERE deg_degrees.CtrCod=%ld"
						   " AND deg_degrees.DegCod=crs_courses.DegCod"
						   " AND crs_courses.CrsCod=crs_users.CrsCod"
						   " AND crs_users.Role=%u"
						 " GROUP BY crs_users.UsrCod) AS NumCrssTable",
					 Cod,
					 (unsigned) Role);
      case HieLvl_DEG:
	 if (Role == Rol_UNK)	// Any user
	    return DB_QuerySELECTDouble ("can not get number of courses per user",
					 "SELECT AVG(NumCrss)"
					  " FROM (SELECT COUNT(crs_users.CrsCod) AS NumCrss"
						  " FROM crs_courses,"
						        "crs_users"
						 " WHERE crs_courses.DegCod=%ld"
						   " AND crs_courses.CrsCod=crs_users.CrsCod"
						 " GROUP BY crs_users.UsrCod) AS NumCrssTable",
					 Cod);
	 else
	    return DB_QuerySELECTDouble ("can not get number of courses per user",
					 "SELECT AVG(NumCrss)"
					  " FROM (SELECT COUNT(crs_users.CrsCod) AS NumCrss"
						  " FROM crs_courses,"
						        "crs_users"
						 " WHERE crs_courses.DegCod=%ld"
						   " AND crs_courses.CrsCod=crs_users.CrsCod"
						   " AND crs_users.Role=%u"
						 " GROUP BY crs_users.UsrCod) AS NumCrssTable",
					 Cod,
					 (unsigned) Role);
      case HieLvl_CRS:
         return 1.0;
      default:
         Err_WrongScopeExit ();
         return 0.0;	// Not reached
     }
  }

double Usr_GetCachedNumCrssPerUsr (HieLvl_Level_t Scope,long Cod,Rol_Role_t Role)
  {
   static const FigCch_FigureCached_t FigureNumCrssPerUsr[Rol_NUM_ROLES] =
     {
      [Rol_UNK] = FigCch_NUM_CRSS_PER_USR,	// Number of courses per user
      [Rol_STD] = FigCch_NUM_CRSS_PER_STD,	// Number of courses per student
      [Rol_NET] = FigCch_NUM_CRSS_PER_NET,	// Number of courses per non-editing teacher
      [Rol_TCH] = FigCch_NUM_CRSS_PER_TCH,	// Number of courses per teacher
     };
   double NumCrssPerUsr;

   /***** Get number of courses per user from cache *****/
   if (!FigCch_GetFigureFromCache (FigureNumCrssPerUsr[Role],Scope,Cod,
                                   FigCch_DOUBLE,&NumCrssPerUsr))
     {
      /***** Get current number of courses per user from database and update cache *****/
      NumCrssPerUsr = Usr_GetNumCrssPerUsr (Scope,Cod,Role);
      FigCch_UpdateFigureIntoCache (FigureNumCrssPerUsr[Role],Scope,Cod,
                                    FigCch_DOUBLE,&NumCrssPerUsr);
     }

   return NumCrssPerUsr;
  }

/*****************************************************************************/
/************ Get average number of courses with users of a type *************/
/*****************************************************************************/

static double Usr_GetNumUsrsPerCrs (HieLvl_Level_t Scope,long Cod,Rol_Role_t Role)
  {
   /***** Get number of users per course from database *****/
   switch (Scope)
     {
      case HieLvl_SYS:
	 if (Role == Rol_UNK)	// Any user
	    return DB_QuerySELECTDouble ("can not get number of users per course",
					 "SELECT AVG(NumUsrs)"
					  " FROM (SELECT COUNT(UsrCod) AS NumUsrs"
						  " FROM crs_users"
						 " GROUP BY CrsCod) AS NumUsrsTable");
	 else
	    return DB_QuerySELECTDouble ("can not get number of users per course",
					 "SELECT AVG(NumUsrs)"
					  " FROM (SELECT COUNT(UsrCod) AS NumUsrs"
						  " FROM crs_users"
						 " WHERE Role=%u GROUP BY CrsCod) AS NumUsrsTable",
					 (unsigned) Role);
      case HieLvl_CTY:
	 if (Role == Rol_UNK)	// Any user
	    return DB_QuerySELECTDouble ("can not get number of users per course",
					 "SELECT AVG(NumUsrs)"
					  " FROM (SELECT COUNT(crs_users.UsrCod) AS NumUsrs"
						  " FROM ins_instits,"
						        "ctr_centers,"
						        "deg_degrees,"
						        "crs_courses,"
						        "crs_users"
						 " WHERE ins_instits.CtyCod=%ld"
						   " AND ins_instits.InsCod=ctr_centers.InsCod"
						   " AND ctr_centers.CtrCod=deg_degrees.CtrCod"
						   " AND deg_degrees.DegCod=crs_courses.DegCod"
						   " AND crs_courses.CrsCod=crs_users.CrsCod"
						 " GROUP BY crs_users.CrsCod) AS NumUsrsTable",
					 Cod);
	 else
	    return DB_QuerySELECTDouble ("can not get number of users per course",
					 "SELECT AVG(NumUsrs)"
					  " FROM (SELECT COUNT(crs_users.UsrCod) AS NumUsrs"
						  " FROM ins_instits,"
						        "ctr_centers,"
						        "deg_degrees,"
						        "crs_courses,"
						        "crs_users"
						 " WHERE ins_instits.CtyCod=%ld"
						   " AND ins_instits.InsCod=ctr_centers.InsCod"
						   " AND ctr_centers.CtrCod=deg_degrees.CtrCod"
						   " AND deg_degrees.DegCod=crs_courses.DegCod"
						   " AND crs_courses.CrsCod=crs_users.CrsCod"
						   " AND crs_users.Role=%u"
						 " GROUP BY crs_users.CrsCod) AS NumUsrsTable",
					 Cod,
					 (unsigned) Role);
      case HieLvl_INS:
	 if (Role == Rol_UNK)	// Any user
	    return DB_QuerySELECTDouble ("can not get number of users per course",
					 "SELECT AVG(NumUsrs)"
					  " FROM (SELECT COUNT(crs_users.UsrCod) AS NumUsrs"
						  " FROM ctr_centers,"
						        "deg_degrees,"
						        "crs_courses,"
						        "crs_users"
						 " WHERE ctr_centers.InsCod=%ld"
						   " AND ctr_centers.CtrCod=deg_degrees.CtrCod"
						   " AND deg_degrees.DegCod=crs_courses.DegCod"
						   " AND crs_courses.CrsCod=crs_users.CrsCod"
						 " GROUP BY crs_users.CrsCod) AS NumUsrsTable",
					 Cod);
	 else
	    return DB_QuerySELECTDouble ("can not get number of users per course",
					 "SELECT AVG(NumUsrs)"
					  " FROM (SELECT COUNT(crs_users.UsrCod) AS NumUsrs"
						  " FROM ctr_centers,"
						        "deg_degrees,"
						        "crs_courses,"
						        "crs_users"
						 " WHERE ctr_centers.InsCod=%ld"
						   " AND ctr_centers.CtrCod=deg_degrees.CtrCod"
						   " AND deg_degrees.DegCod=crs_courses.DegCod"
						   " AND crs_courses.CrsCod=crs_users.CrsCod"
						   " AND crs_users.Role=%u"
						 " GROUP BY crs_users.CrsCod) AS NumUsrsTable",
					 Cod,
					 (unsigned) Role);
      case HieLvl_CTR:
	 if (Role == Rol_UNK)	// Any user
	    return DB_QuerySELECTDouble ("can not get number of users per course",
					 "SELECT AVG(NumUsrs)"
					  " FROM (SELECT COUNT(crs_users.UsrCod) AS NumUsrs"
						  " FROM deg_degrees,"
						        "crs_courses,"
						        "crs_users"
						 " WHERE deg_degrees.CtrCod=%ld"
						   " AND deg_degrees.DegCod=crs_courses.DegCod"
						   " AND crs_courses.CrsCod=crs_users.CrsCod"
						 " GROUP BY crs_users.CrsCod) AS NumUsrsTable",
					 Cod);
	 else
	    return DB_QuerySELECTDouble ("can not get number of users per course",
					 "SELECT AVG(NumUsrs)"
					  " FROM (SELECT COUNT(crs_users.UsrCod) AS NumUsrs"
						  " FROM deg_degrees,"
						        "crs_courses,"
						        "crs_users"
						 " WHERE deg_degrees.CtrCod=%ld"
						   " AND deg_degrees.DegCod=crs_courses.DegCod"
						   " AND crs_courses.CrsCod=crs_users.CrsCod"
						   " AND crs_users.Role=%u"
						 " GROUP BY crs_users.CrsCod) AS NumUsrsTable",
					 Cod,
					 (unsigned) Role);
      case HieLvl_DEG:
	 if (Role == Rol_UNK)	// Any user
	    return DB_QuerySELECTDouble ("can not get number of users per course",
					 "SELECT AVG(NumUsrs)"
					  " FROM (SELECT COUNT(crs_users.UsrCod) AS NumUsrs"
						  " FROM crs_courses,"
						        "crs_users"
						 " WHERE crs_courses.DegCod=%ld"
						   " AND crs_courses.CrsCod=crs_users.CrsCod"
						 " GROUP BY crs_users.CrsCod) AS NumUsrsTable",
					 Cod);
	 else
	    return DB_QuerySELECTDouble ("can not get number of users per course",
					 "SELECT AVG(NumUsrs)"
					  " FROM (SELECT COUNT(crs_users.UsrCod) AS NumUsrs"
						  " FROM crs_courses,"
						        "crs_users"
						 " WHERE crs_courses.DegCod=%ld"
						   " AND crs_courses.CrsCod=crs_users.CrsCod"
						   " AND crs_users.Role=%u"
						 " GROUP BY crs_users.CrsCod) AS NumUsrsTable",
					 Cod,
					 (unsigned) Role);
      case HieLvl_CRS:
	 return (double) Usr_GetNumUsrsInCrss (HieLvl_CRS,Cod,
				               Role == Rol_UNK ? 1 << Rol_STD |
							         1 << Rol_NET |
							         1 << Rol_TCH :	// Any user
							         1 << Role);

      default:
         Err_WrongScopeExit ();
         return 0.0;	// Not reached
     }
  }

double Usr_GetCachedNumUsrsPerCrs (HieLvl_Level_t Scope,long Cod,Rol_Role_t Role)
  {
   static const FigCch_FigureCached_t FigureNumUsrsPerCrs[Rol_NUM_ROLES] =
     {
      [Rol_UNK] = FigCch_NUM_USRS_PER_CRS,	// Number of users per course
      [Rol_STD] = FigCch_NUM_STDS_PER_CRS,	// Number of students per course
      [Rol_NET] = FigCch_NUM_NETS_PER_CRS,	// Number of non-editing teachers per course
      [Rol_TCH] = FigCch_NUM_TCHS_PER_CRS,	// Number of teachers per course
     };
   double NumUsrsPerCrs;

   /***** Get number of users per course from cache *****/
   if (!FigCch_GetFigureFromCache (FigureNumUsrsPerCrs[Role],Scope,Cod,
                                   FigCch_DOUBLE,&NumUsrsPerCrs))
     {
      /***** Get current number of users per course from database and update cache *****/
      NumUsrsPerCrs = Usr_GetNumUsrsPerCrs (Scope,Cod,Role);
      FigCch_UpdateFigureIntoCache (FigureNumUsrsPerCrs[Role],Scope,Cod,
                                    FigCch_DOUBLE,&NumUsrsPerCrs);
     }

   return NumUsrsPerCrs;
  }

/*****************************************************************************/
/****************** Check if a user is banned in ranking *********************/
/*****************************************************************************/

bool Usr_DB_CheckIfUsrBanned (long UsrCod)
  {
   return (DB_QueryCOUNT ("can not check if user is banned",
			  "SELECT COUNT(*)"
			   " FROM usr_banned"
			  " WHERE UsrCod=%ld",
			  UsrCod) != 0);
  }

/*****************************************************************************/
/**************** Remove user from banned users in ranking *******************/
/*****************************************************************************/

void Usr_DB_RemoveUsrFromBanned (long UsrCod)
  {
   DB_QueryDELETE ("can not remove user from users banned",
		   "DELETE FROM usr_banned"
		   " WHERE UsrCod=%ld",
		   UsrCod);
  }

/*****************************************************************************/
/**************************** Show a user QR code ****************************/
/*****************************************************************************/

void Usr_PrintUsrQRCode (void)
  {
   char NewNickWithArr[Nck_MAX_BYTES_NICK_FROM_FORM + 1];

   if (Usr_GetParamOtherUsrCodEncryptedAndGetUsrData ())
     {
      /***** Begin box *****/
      Box_BoxBegin (NULL,Gbl.Usrs.Other.UsrDat.FullName,
                    NULL,NULL,
                    NULL,Box_NOT_CLOSABLE);

      /***** Show QR code *****/
      if (Gbl.Usrs.Other.UsrDat.Nickname[0])
	{
	 snprintf (NewNickWithArr,sizeof (NewNickWithArr),"@%s",
		   Gbl.Usrs.Other.UsrDat.Nickname);
	 QR_ImageQRCode (NewNickWithArr);
	}

      /***** End box *****/
      Box_BoxEnd ();
     }
   else
      Ale_ShowAlertUserNotFoundOrYouDoNotHavePermission ();
  }

/*****************************************************************************/
/********************* Write the author of an assignment *********************/
/*****************************************************************************/

void Usr_WriteAuthor1Line (long UsrCod,bool Hidden)
  {
   bool ShowPhoto = false;
   char PhotoURL[PATH_MAX + 1];
   struct UsrData UsrDat;

   /***** Initialize structure with user's data *****/
   Usr_UsrDataConstructor (&UsrDat);

   /***** Get data of author *****/
   UsrDat.UsrCod = UsrCod;
   if (Usr_ChkUsrCodAndGetAllUsrDataFromUsrCod (&UsrDat,
                                                Usr_DONT_GET_PREFS,
                                                Usr_DONT_GET_ROLE_IN_CURRENT_CRS))
      ShowPhoto = Pho_ShowingUsrPhotoIsAllowed (&UsrDat,PhotoURL);

   /***** Show photo *****/
   Pho_ShowUsrPhoto (&UsrDat,ShowPhoto ? PhotoURL :
                	                 NULL,
	             "PHOTO15x20",Pho_ZOOM,false);

   /***** Write name *****/
   HTM_DIV_Begin ("class=\"AUTHOR_1_LINE %s\"",Hidden ? "AUTHOR_TXT_LIGHT" :
        	                                        "AUTHOR_TXT");
   HTM_Txt (UsrDat.FullName);
   HTM_DIV_End ();

   /***** Free memory used for user's data *****/
   Usr_UsrDataDestructor (&UsrDat);
  }

/*****************************************************************************/
/*************** Show a table cell with the data of a user *******************/
/*****************************************************************************/

void Usr_ShowTableCellWithUsrData (struct UsrData *UsrDat,unsigned NumRows)
  {
   Act_Action_t NextAction;

   /***** Show user's photo *****/
   if (NumRows)
      HTM_TD_Begin ("rowspan=\"%u\" class=\"LT LINE_BOTTOM COLOR%u\"",
	            NumRows + 1,Gbl.RowEvenOdd);
   else
      HTM_TD_Begin ("class=\"LT LINE_BOTTOM COLOR%u\"",Gbl.RowEvenOdd);
   Pho_ShowUsrPhotoIfAllowed (UsrDat,"PHOTO45x60",Pho_ZOOM,false);
   HTM_TD_End ();

   /***** User's IDs and name *****/
   /* Begin cell */
   if (NumRows)
      HTM_TD_Begin ("rowspan=\"%u\" class=\"LT LINE_BOTTOM COLOR%u\"",
	            NumRows + 1,Gbl.RowEvenOdd);
   else
      HTM_TD_Begin ("class=\"LT LINE_BOTTOM COLOR%u\"",Gbl.RowEvenOdd);

   /* Action to go to user's record depending on role in course */
   switch (UsrDat->Roles.InCurrentCrs)
     {
      case Rol_STD:
	 NextAction = ActSeeRecOneStd;
	 break;
      case Rol_NET:
      case Rol_TCH:
	 NextAction = ActSeeRecOneTch;
	 break;
      default:
	 NextAction = ActUnk;
	 break;
     }

   if (NextAction == ActUnk)
      /* Begin div */
      HTM_DIV_Begin ("class=\"LT AUTHOR_TXT\"");
   else
     {
      /* Begin form to go to user's record card */
      Frm_BeginForm (NextAction);
      Usr_PutParamUsrCodEncrypted (UsrDat->EnUsrCod);
      HTM_BUTTON_SUBMIT_Begin (UsrDat->FullName,"BT_LINK LT AUTHOR_TXT",NULL);
     }

   /* User's ID */
   ID_WriteUsrIDs (UsrDat,NULL);

   /* User's name */
   HTM_BR ();
   HTM_Txt (UsrDat->Surname1);
   if (UsrDat->Surname2[0])
      HTM_TxtF ("&nbsp;%s",UsrDat->Surname2);
   if (UsrDat->FrstName[0])
     {
      HTM_Comma ();
      HTM_BR ();
      HTM_Txt (UsrDat->FrstName);
     }

   if (NextAction == ActUnk)
      /* End div */
      HTM_DIV_End ();
   else
     {
      /* End form */
      HTM_BUTTON_End ();
      Frm_EndForm ();
     }

   /* End cell */
   HTM_TD_End ();
  }

/*****************************************************************************/
/********** Show a setting selector for me / selected users / all ************/
/*****************************************************************************/

void Usr_PutWhoIcon (Usr_Who_t Who)
  {
   extern const char *Txt_WHO[Usr_NUM_WHO];

   switch (Who)
     {
      case Usr_WHO_UNKNOWN:
	 break;
      case Usr_WHO_ME:
         HTM_INPUT_IMAGE (Gbl.Usrs.Me.PhotoURL[0] ? Gbl.Usrs.Me.PhotoURL :
						    Cfg_URL_ICON_PUBLIC,
			  Gbl.Usrs.Me.PhotoURL[0] ? NULL :
						    "usr_bl.jpg",
		          Txt_WHO[Who],
	                  "ICO_HIGHLIGHT PHOTO15x20");
	 break;
      case Usr_WHO_SELECTED:
      case Usr_WHO_FOLLOWED:
         HTM_INPUT_IMAGE (Cfg_URL_ICON_PUBLIC,"user-check.svg",
			  Txt_WHO[Who],
			  "ICO_HIGHLIGHT ICOx20");
	 break;
      case Usr_WHO_ALL:
         HTM_INPUT_IMAGE (Cfg_URL_ICON_PUBLIC,"users.svg",
			  Txt_WHO[Who],
			  "ICO_HIGHLIGHT ICOx20");
	 break;
      default:
	Err_WrongWhoExit ();
	break;
     }
  }

/*****************************************************************************/
/*************** Put hidden param for which users are involved ***************/
/*****************************************************************************/

void Usr_PutHiddenParamWho (Usr_Who_t Who)
  {
   Par_PutHiddenParamUnsigned (NULL,"Who",(unsigned) Who);
  }

/*****************************************************************************/
/*************** Get hidden param for which users are involved ***************/
/*****************************************************************************/

Usr_Who_t Usr_GetHiddenParamWho (void)
  {
   return (Usr_Who_t) Par_GetParToUnsignedLong ("Who",
                                                1,
                                                Usr_NUM_WHO - 1,
                                                Usr_WHO_UNKNOWN);
  }

/*****************************************************************************/
/************************* Get old users from database ***********************/
/*****************************************************************************/

unsigned Usr_DB_GetOldUsrs (MYSQL_RES **mysql_res,time_t SecondsWithoutAccess)
  {
   return (unsigned)
   DB_QuerySELECT (mysql_res,"can not get old users",
		   "SELECT UsrCod"
		    " FROM (SELECT UsrCod"
			    " FROM usr_last"
			   " WHERE LastTime<FROM_UNIXTIME(UNIX_TIMESTAMP()-%llu)"
			   " UNION "
			   "SELECT UsrCod"
			    " FROM usr_data"
			   " WHERE UsrCod NOT IN"
			         " (SELECT UsrCod"
				    " FROM usr_last)"
			  ") AS candidate_usrs"
		   " WHERE UsrCod NOT IN"
		         " (SELECT DISTINCT UsrCod"
			    " FROM crs_users)",
		   (unsigned long long) SecondsWithoutAccess);
  }

/*****************************************************************************/
/************** Get number of users who have chosen an option ****************/
/*****************************************************************************/

unsigned Usr_DB_GetNumUsrsWhoChoseAnOption (const char *SubQuery)
  {
   switch (Gbl.Scope.Current)
     {
      case HieLvl_SYS:
	 return (unsigned)
	 DB_QueryCOUNT ("can not get the number of users who have chosen an option",
		        "SELECT COUNT(*)"
		         " FROM usr_data WHERE %s",
		        SubQuery);
      case HieLvl_CTY:
	 return (unsigned)
	 DB_QueryCOUNT ("can not get the number of users who have chosen an option",
		        "SELECT COUNT(DISTINCT usr_data.UsrCod)"
		         " FROM ins_instits,"
			       "ctr_centers,"
			       "deg_degrees,"
			       "crs_courses,"
			       "crs_users,"
			       "usr_data"
		        " WHERE ins_instits.CtyCod=%ld"
		          " AND ins_instits.InsCod=ctr_centers.InsCod"
		          " AND ctr_centers.CtrCod=deg_degrees.CtrCod"
		          " AND deg_degrees.DegCod=crs_courses.DegCod"
		          " AND crs_courses.CrsCod=crs_users.CrsCod"
		          " AND crs_users.UsrCod=usr_data.UsrCod"
		          " AND %s",
		        Gbl.Hierarchy.Cty.CtyCod,SubQuery);
      case HieLvl_INS:
	 return (unsigned)
	 DB_QueryCOUNT ("can not get the number of users who have chosen an option",
		        "SELECT COUNT(DISTINCT usr_data.UsrCod)"
		         " FROM ctr_centers,"
		               "deg_degrees,"
		               "crs_courses,"
		               "crs_users,"
		               "usr_data"
		        " WHERE ctr_centers.InsCod=%ld"
		          " AND ctr_centers.CtrCod=deg_degrees.CtrCod"
		          " AND deg_degrees.DegCod=crs_courses.DegCod"
		          " AND crs_courses.CrsCod=crs_users.CrsCod"
		          " AND crs_users.UsrCod=usr_data.UsrCod"
		          " AND %s",
		        Gbl.Hierarchy.Ins.InsCod,SubQuery);
      case HieLvl_CTR:
	 return (unsigned)
	 DB_QueryCOUNT ("can not get the number of users who have chosen an option",
		        "SELECT COUNT(DISTINCT usr_data.UsrCod)"
		         " FROM deg_degrees,"
		               "crs_courses,"
		               "crs_users,"
		               "usr_data"
		        " WHERE deg_degrees.CtrCod=%ld"
		          " AND deg_degrees.DegCod=crs_courses.DegCod"
		          " AND crs_courses.CrsCod=crs_users.CrsCod"
		          " AND crs_users.UsrCod=usr_data.UsrCod"
		          " AND %s",
		        Gbl.Hierarchy.Ctr.CtrCod,SubQuery);
      case HieLvl_DEG:
	 return (unsigned)
	 DB_QueryCOUNT ("can not get the number of users who have chosen an option",
		        "SELECT COUNT(DISTINCT usr_data.UsrCod)"
		         " FROM crs_courses,"
		               "crs_users,"
		               "usr_data"
		        " WHERE crs_courses.DegCod=%ld"
		          " AND crs_courses.CrsCod=crs_users.CrsCod"
		          " AND crs_users.UsrCod=usr_data.UsrCod"
		          " AND %s",
		        Gbl.Hierarchy.Deg.DegCod,SubQuery);
      case HieLvl_CRS:
	 return (unsigned)
	 DB_QueryCOUNT ("can not get the number of users who have chosen an option",
		        "SELECT COUNT(DISTINCT usr_data.UsrCod)"
		         " FROM crs_users,"
		               "usr_data"
		        " WHERE crs_users.CrsCod=%ld"
		          " AND crs_users.UsrCod=usr_data.UsrCod"
		          " AND %s",
		        Gbl.Hierarchy.Crs.CrsCod,SubQuery);
      default:
	 Err_WrongScopeExit ();
	 return 0;	// Not reached
     }

   return 0;	// Not reached
  }
