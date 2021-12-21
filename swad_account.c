// swad_account.c: user's account

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

#include <string.h>		// For string functions

#include "swad_account.h"
#include "swad_account_database.h"
#include "swad_admin_database.h"
#include "swad_agenda_database.h"
#include "swad_announcement_database.h"
#include "swad_attendance_database.h"
#include "swad_box.h"
#include "swad_browser_database.h"
#include "swad_calendar.h"
#include "swad_connected_database.h"
#include "swad_database.h"
#include "swad_duplicate_database.h"
#include "swad_enrolment_database.h"
#include "swad_error.h"
#include "swad_exam_database.h"
#include "swad_follow.h"
#include "swad_form.h"
#include "swad_forum_database.h"
#include "swad_global.h"
#include "swad_HTML.h"
#include "swad_ID.h"
#include "swad_ID_database.h"
#include "swad_language.h"
#include "swad_mail_database.h"
#include "swad_match.h"
#include "swad_message.h"
#include "swad_message_database.h"
#include "swad_network.h"
#include "swad_nickname.h"
#include "swad_nickname_database.h"
#include "swad_notification.h"
#include "swad_notification_database.h"
#include "swad_parameter.h"
#include "swad_photo_database.h"
#include "swad_profile.h"
#include "swad_profile_database.h"
#include "swad_project.h"
#include "swad_record_database.h"
#include "swad_report.h"
#include "swad_session_database.h"
#include "swad_setting.h"
#include "swad_setting_database.h"
#include "swad_test_print.h"
#include "swad_timeline.h"
#include "swad_user_database.h"

/*****************************************************************************/
/************** External global variables from others modules ****************/
/*****************************************************************************/

extern struct Globals Gbl;

/*****************************************************************************/
/***************************** Private prototypes ****************************/
/*****************************************************************************/

static void Acc_ShowFormCheckIfIHaveAccount (const char *Title);
static void Acc_WriteRowEmptyAccount (unsigned NumUsr,const char *ID,struct UsrData *UsrDat);
static void Acc_ShowFormRequestNewAccountWithParams (const char NewNickWithoutArr[Nck_MAX_BYTES_NICK_WITHOUT_ARROBA + 1],
                                                     const char *NewEmail);
static bool Acc_GetParamsNewAccount (char NewNickWithoutArr[Nck_MAX_BYTES_NICK_WITHOUT_ARROBA + 1],
                                     char NewEmail[Cns_MAX_BYTES_EMAIL_ADDRESS + 1],
                                     char *NewEncryptedPassword);
static void Acc_CreateNewEncryptedUsrCod (struct UsrData *UsrDat);

static void Acc_PutParamsToRemoveMyAccount (void *EncryptedUsrCod);

static void Acc_AskIfRemoveUsrAccount (bool ItsMe);
static void Acc_AskIfRemoveOtherUsrAccount (void);

static void Acc_RemoveUsrBriefcase (struct UsrData *UsrDat);

/*****************************************************************************/
/******************** Put link to create a new account ***********************/
/*****************************************************************************/

void Acc_PutLinkToCreateAccount (void)
  {
   extern const char *Txt_Create_account;

   Lay_PutContextualLinkIconText (ActFrmMyAcc,NULL,
                                  NULL,NULL,
				  "at.svg",Ico_BLACK,
				  Txt_Create_account);
  }

/*****************************************************************************/
/******** Show form to change my account or to create a new account **********/
/*****************************************************************************/

void Acc_ShowFormMyAccount (void)
  {
   extern const char *Txt_Before_creating_a_new_account_check_if_you_have_been_already_registered;

   if (Gbl.Usrs.Me.Logged)
      Acc_ShowFormChgMyAccount ();
   else	// Not logged
     {
      /***** Contextual menu *****/
      Mnu_ContextMenuBegin ();
	 Usr_PutLinkToLogin ();
	 Pwd_PutLinkToSendNewPasswd ();
	 Lan_PutLinkToChangeLanguage ();
      Mnu_ContextMenuEnd ();

      /**** Show form to check if I have an account *****/
      Acc_ShowFormCheckIfIHaveAccount (Txt_Before_creating_a_new_account_check_if_you_have_been_already_registered);
     }
  }

/*****************************************************************************/
/***************** Show form to check if I have an account *******************/
/*****************************************************************************/

static void Acc_ShowFormCheckIfIHaveAccount (const char *Title)
  {
   extern const char *Hlp_PROFILE_SignUp;
   extern const char *The_ClassFormInBox[The_NUM_THEMES];
   extern const char *Txt_If_you_think_you_may_have_been_registered_;
   extern const char *Txt_ID;
   extern const char *Txt_Check;
   extern const char *Txt_Skip_this_step;

   /***** Begin box *****/
   Box_BoxBegin (NULL,Title,
                 NULL,NULL,
                 Hlp_PROFILE_SignUp,Box_NOT_CLOSABLE);

      /***** Help alert *****/
      Ale_ShowAlert (Ale_INFO,Txt_If_you_think_you_may_have_been_registered_);

      /***** Form to request user's ID for possible account already created *****/
      Frm_BeginForm (ActChkUsrAcc);
	 HTM_LABEL_Begin ("class=\"%s\"",The_ClassFormInBox[Gbl.Prefs.Theme]);
	    HTM_TxtColonNBSP (Txt_ID);
	    HTM_INPUT_TEXT ("ID",ID_MAX_CHARS_USR_ID,"",HTM_DONT_SUBMIT_ON_CHANGE,
			    "size=\"18\" required=\"required\"");
	 HTM_LABEL_End ();
	 Btn_PutCreateButtonInline (Txt_Check);
      Frm_EndForm ();

      /***** Form to skip this step *****/
      Frm_BeginForm (ActCreMyAcc);
	 Btn_PutConfirmButton (Txt_Skip_this_step);
      Frm_EndForm ();

   /***** End box *****/
   Box_BoxEnd ();
  }

/*****************************************************************************/
/* Check if already exists a new account without password associated to a ID */
/*****************************************************************************/

void Acc_CheckIfEmptyAccountExists (void)
  {
   extern const char *Txt_Do_you_think_you_are_this_user;
   extern const char *Txt_Do_you_think_you_are_one_of_these_users;
   extern const char *Txt_There_is_no_empty_account_associated_with_your_ID_X;
   extern const char *Txt_Check_another_ID;
   extern const char *Txt_Please_enter_your_ID;
   extern const char *Txt_Before_creating_a_new_account_check_if_you_have_been_already_registered;
   char ID[ID_MAX_BYTES_USR_ID + 1];
   unsigned NumUsrs;
   unsigned NumUsr;
   struct UsrData UsrDat;
   MYSQL_RES *mysql_res;

   /***** Contextual menu *****/
   Mnu_ContextMenuBegin ();
      Usr_PutLinkToLogin ();
      Pwd_PutLinkToSendNewPasswd ();
      Lan_PutLinkToChangeLanguage ();
   Mnu_ContextMenuEnd ();

   /***** Get new user's ID from form *****/
   Par_GetParToText ("ID",ID,ID_MAX_BYTES_USR_ID);
   // Users' IDs are always stored internally in capitals and without leading zeros
   Str_RemoveLeadingZeros (ID);
   Str_ConvertToUpperText (ID);

   /***** Check if there are users with this user's ID *****/
   if (ID_CheckIfUsrIDIsValid (ID))
     {
      if ((NumUsrs = Acc_DB_GetUsrsWithID (&mysql_res,ID)))
	{
         /***** Begin box and table *****/
	 Box_BoxTableBegin (NULL,
	                    (NumUsrs == 1) ? Txt_Do_you_think_you_are_this_user :
					     Txt_Do_you_think_you_are_one_of_these_users,
			    NULL,NULL,
			    NULL,Box_CLOSABLE,5);

	 /***** Initialize structure with user's data *****/
	 Usr_UsrDataConstructor (&UsrDat);

	 /***** List users found *****/
	 for (NumUsr  = 1, Gbl.RowEvenOdd = 0;
	      NumUsr <= NumUsrs;
	      NumUsr++, Gbl.RowEvenOdd = 1 - Gbl.RowEvenOdd)
	   {
	    /***** Get user's data from query result *****/
	    /* Get user's code */
	    UsrDat.UsrCod = DB_GetNextCode (mysql_res);

	    /* Get user's data */
            Usr_GetAllUsrDataFromUsrCod (&UsrDat,
                                         Usr_DONT_GET_PREFS,
                                         Usr_DONT_GET_ROLE_IN_CURRENT_CRS);

            /***** Write row with data of empty account *****/
            Acc_WriteRowEmptyAccount (NumUsr,ID,&UsrDat);
	   }

	 /***** Free memory used for user's data *****/
	 Usr_UsrDataDestructor (&UsrDat);

	 /***** End table and box *****/
	 Box_BoxTableEnd ();
	}
      else
	 Ale_ShowAlert (Ale_INFO,Txt_There_is_no_empty_account_associated_with_your_ID_X,
		        ID);

      /***** Free structure that stores the query result *****/
      DB_FreeMySQLResult (&mysql_res);

      /**** Show form to check if I have an account *****/
      Acc_ShowFormCheckIfIHaveAccount (Txt_Check_another_ID);
     }
   else	// ID not valid
     {
      /**** Show again form to check if I have an account *****/
      Ale_ShowAlert (Ale_WARNING,Txt_Please_enter_your_ID);

      Acc_ShowFormCheckIfIHaveAccount (Txt_Before_creating_a_new_account_check_if_you_have_been_already_registered);
     }
  }

/*****************************************************************************/
/************************ Write data of empty account ************************/
/*****************************************************************************/

static void Acc_WriteRowEmptyAccount (unsigned NumUsr,const char *ID,struct UsrData *UsrDat)
  {
   extern const char *The_ClassDatStrong[The_NUM_THEMES];
   extern const char *Txt_ID;
   extern const char *Txt_Name;
   extern const char *Txt_yet_unnamed;
   extern const char *Txt_Its_me;

   /***** Begin 1st table row *****/
   HTM_TR_Begin (NULL);

      /***** Write number of user in the list *****/
      HTM_TD_Begin ("rowspan=\"2\" class=\"USR_LIST_NUM_N RT %s\"",
                    Gbl.ColorRows[Gbl.RowEvenOdd]);
	 HTM_Unsigned (NumUsr);
      HTM_TD_End ();

      /***** Write user's ID and name *****/
      HTM_TD_Begin ("class=\"%s LT %s\"",
                    The_ClassDatStrong[Gbl.Prefs.Theme],
                    Gbl.ColorRows[Gbl.RowEvenOdd]);
	 HTM_TxtF ("%s:&nbsp;%s",Txt_ID,ID);
	 HTM_BR ();
	 HTM_TxtColonNBSP (Txt_Name);
	 if (UsrDat->FullName[0])
	   {
	    HTM_STRONG_Begin ();
	       HTM_Txt (UsrDat->FullName);
	    HTM_STRONG_End ();
	   }
	 else
	   {
	    HTM_EM_Begin ();
	       HTM_Txt (Txt_yet_unnamed);
	    HTM_EM_End ();
	   }
      HTM_TD_End ();

      /***** Button to login with this account *****/
      HTM_TD_Begin ("class=\"RT %s\"",Gbl.ColorRows[Gbl.RowEvenOdd]);
	 Frm_BeginForm (ActLogInNew);
	    Usr_PutParamUsrCodEncrypted (UsrDat->EnUsrCod);
	    Btn_PutCreateButtonInline (Txt_Its_me);
	 Frm_EndForm ();
      HTM_TD_End ();

   /***** End 1st table row *****/
   HTM_TR_End ();

   /***** Begin 2nd table row *****/
   HTM_TR_Begin (NULL);

   /***** Courses of this user *****/
   HTM_TD_Begin ("colspan=\"2\" class=\"LT %s\"",Gbl.ColorRows[Gbl.RowEvenOdd]);
      UsrDat->Sex = Usr_SEX_UNKNOWN;
      Crs_GetAndWriteCrssOfAUsr (UsrDat,Rol_TCH);
      Crs_GetAndWriteCrssOfAUsr (UsrDat,Rol_NET);
      Crs_GetAndWriteCrssOfAUsr (UsrDat,Rol_STD);
   HTM_TD_End ();

   /***** End 2nd table row *****/
   HTM_TR_End ();
  }

/*****************************************************************************/
/********************* Show form to create a new account *********************/
/*****************************************************************************/

void Acc_ShowFormCreateMyAccount (void)
  {
   /***** Contextual menu *****/
   Mnu_ContextMenuBegin ();
      Usr_PutLinkToLogin ();
      Pwd_PutLinkToSendNewPasswd ();
      Lan_PutLinkToChangeLanguage ();
   Mnu_ContextMenuEnd ();

   /**** Show form to create a new account *****/
   Acc_ShowFormRequestNewAccountWithParams ("","");
  }

/*****************************************************************************/
/************ Show form to create a new account using parameters *************/
/*****************************************************************************/

static void Acc_ShowFormRequestNewAccountWithParams (const char NewNickWithoutArr[Nck_MAX_BYTES_NICK_WITHOUT_ARROBA + 1],
                                                     const char *NewEmail)
  {
   extern const char *Hlp_PROFILE_SignUp;
   extern const char *The_ClassFormInBox[The_NUM_THEMES];
   extern const char *Txt_Create_account;
   extern const char *Txt_Nickname;
   extern const char *Txt_HELP_nickname;
   extern const char *Txt_HELP_email;
   extern const char *Txt_Email;
   char NewNickWithArr[Nck_MAX_BYTES_NICK_WITH_ARROBA + 1];

   /***** Begin form to enter some data of the new user *****/
   Frm_BeginForm (ActCreUsrAcc);

      /***** Begin box and table *****/
      Box_BoxTableBegin (NULL,Txt_Create_account,
			 NULL,NULL,
			 Hlp_PROFILE_SignUp,Box_NOT_CLOSABLE,2);

	 /***** Nickname *****/
	 /* Begin table row */
	 HTM_TR_Begin (NULL);

	    /* Label */
	    Frm_LabelColumn ("RT","NewNick",Txt_Nickname);

	    /* Data */
	    HTM_TD_Begin ("class=\"LT\"");
	       if (NewNickWithoutArr[0])
		  snprintf (NewNickWithArr,sizeof (NewNickWithArr),"@%s",
			    NewNickWithoutArr);
	       else
		  NewNickWithArr[0] = '\0';
	       HTM_INPUT_TEXT ("NewNick",1 + Nck_MAX_CHARS_NICK_WITHOUT_ARROBA,
			       NewNickWithArr,HTM_DONT_SUBMIT_ON_CHANGE,
			       "id=\"NewNick\" size=\"18\" placeholder=\"%s\""
			       " required=\"required\"",
			       Txt_HELP_nickname);
	    HTM_TD_End ();

	 /* End table row */
	 HTM_TR_End ();

	 /***** Email *****/
	 /* Begin table row */
	 HTM_TR_Begin (NULL);

	    /* Label */
	    Frm_LabelColumn ("RT","NewEmail",Txt_Email);

	    /* Data */
	    HTM_TD_Begin ("class=\"LT\"");
	       HTM_INPUT_EMAIL ("NewEmail",Cns_MAX_CHARS_EMAIL_ADDRESS,NewEmail,
				"id=\"NewEmail\" size=\"18\" placeholder=\"%s\""
				" required=\"required\"",
				Txt_HELP_email);
	    HTM_TD_End ();

	 /* End table row */
	 HTM_TR_End ();

	 /***** Password *****/
	 Pwd_PutFormToGetNewPasswordOnce ();

      /***** End table, send button and end box *****/
      Box_BoxTableWithButtonEnd (Btn_CREATE_BUTTON,Txt_Create_account);

   /***** End form *****/
   Frm_EndForm ();
  }

/*****************************************************************************/
/********* Show form to go to request the creation of a new account **********/
/*****************************************************************************/

void Acc_ShowFormGoToRequestNewAccount (void)
  {
   extern const char *Hlp_PROFILE_SignUp;
   extern const char *Txt_New_on_PLATFORM_Sign_up;
   extern const char *Txt_Create_account;

   /***** Begin box *****/
   Box_BoxBegin (NULL,Str_BuildString (Txt_New_on_PLATFORM_Sign_up,
				          Cfg_PLATFORM_SHORT_NAME),
                 NULL,NULL,
                 Hlp_PROFILE_SignUp,Box_NOT_CLOSABLE);
   Str_FreeStrings ();

   /***** Button to go to request the creation of a new account *****/
   Frm_BeginForm (ActFrmMyAcc);
      Btn_PutCreateButton (Txt_Create_account);
   Frm_EndForm ();

   /***** End box *****/
   Box_BoxEnd ();
  }

/*****************************************************************************/
/*********************** Show form to change my account **********************/
/*****************************************************************************/

void Acc_ShowFormChgMyAccount (void)
  {
   extern const char *Txt_Before_going_to_any_other_option_you_must_create_your_password;
   extern const char *Txt_Before_going_to_any_other_option_you_must_fill_your_nickname;
   extern const char *Txt_Before_going_to_any_other_option_you_must_fill_in_your_email_address;
   bool IMustCreateMyPasswordNow = false;
   bool IMustCreateMyNicknameNow = false;
   bool IMustFillInMyEmailNow    = false;
   bool IShouldConfirmMyEmailNow = false;
   bool IShouldFillInMyIDNow     = false;

   /***** Get current user's nickname and email address
          It's necessary because current nickname or email could be just updated *****/
   Nck_DB_GetNicknameFromUsrCod (Gbl.Usrs.Me.UsrDat.UsrCod,Gbl.Usrs.Me.UsrDat.Nickname);
   Mai_GetEmailFromUsrCod (&Gbl.Usrs.Me.UsrDat);

   /***** Check nickname, email and ID *****/
   IMustCreateMyPasswordNow = (Gbl.Usrs.Me.UsrDat.Password[0] == '\0');
   if (IMustCreateMyPasswordNow)
      Ale_ShowAlert (Ale_WARNING,Txt_Before_going_to_any_other_option_you_must_create_your_password);
   else
     {
      IMustCreateMyNicknameNow = (Gbl.Usrs.Me.UsrDat.Nickname[0] == '\0');
      if (IMustCreateMyNicknameNow)
	 Ale_ShowAlert (Ale_WARNING,Txt_Before_going_to_any_other_option_you_must_fill_your_nickname);
      else
        {
	 IMustFillInMyEmailNow = (Gbl.Usrs.Me.UsrDat.Email[0] == '\0');
	 if (IMustFillInMyEmailNow)
	    Ale_ShowAlert (Ale_WARNING,Txt_Before_going_to_any_other_option_you_must_fill_in_your_email_address);
	 else
	   {
	    IShouldConfirmMyEmailNow = (!Gbl.Usrs.Me.UsrDat.EmailConfirmed &&	// Email not yet confirmed
	                                !Gbl.Usrs.Me.ConfirmEmailJustSent);		// Do not ask for email confirmation when confirmation email is just sent
            IShouldFillInMyIDNow = (Gbl.Usrs.Me.UsrDat.IDs.Num == 0);
	   }
        }
     }

   /***** Begin container for this user *****/
   HTM_DIV_Begin ("class=\"REC_USR\"");

      /***** Show form to change my password and my nickname ****/
      HTM_DIV_Begin ("class=\"REC_LEFT\"");
	 Pwd_ShowFormChgMyPwd ();
	 Nck_ShowFormChangeMyNickname (IMustCreateMyNicknameNow);
      HTM_DIV_End ();

      /***** Show form to change my email and my ID *****/
      HTM_DIV_Begin ("class=\"REC_RIGHT\"");
	 Mai_ShowFormChangeMyEmail (IMustFillInMyEmailNow,IShouldConfirmMyEmailNow);
	 ID_ShowFormChangeMyID (IShouldFillInMyIDNow);
      HTM_DIV_End ();

   /***** Begin container for this user *****/
   HTM_DIV_End ();
  }

/*****************************************************************************/
/***************** Show form to change another user's account ****************/
/*****************************************************************************/

void Acc_ShowFormChgOtherUsrAccount (void)
  {
   /***** Get user whose account must be changed *****/
   if (Usr_GetParamOtherUsrCodEncryptedAndGetUsrData ())
     {
      if (Usr_ICanEditOtherUsr (&Gbl.Usrs.Other.UsrDat))
	{
	 /***** Get user's nickname and email address
		It's necessary because nickname or email could be just updated *****/
	 Nck_DB_GetNicknameFromUsrCod (Gbl.Usrs.Other.UsrDat.UsrCod,Gbl.Usrs.Other.UsrDat.Nickname);
	 Mai_GetEmailFromUsrCod (&Gbl.Usrs.Other.UsrDat);

	 /***** Show user's record *****/
	 Rec_ShowSharedUsrRecord (Rec_SHA_RECORD_LIST,
				  &Gbl.Usrs.Other.UsrDat,NULL);

	 /***** Begin container for this user *****/
	 HTM_DIV_Begin ("class=\"REC_USR\"");

	    /***** Show form to change password and nickname *****/
	    HTM_DIV_Begin ("class=\"REC_LEFT\"");
	       Pwd_ShowFormChgOtherUsrPwd ();
	       Nck_ShowFormChangeOtherUsrNickname ();
	    HTM_DIV_End ();

	    /***** Show form to change email and ID *****/
	    HTM_DIV_Begin ("class=\"REC_RIGHT\"");
	       Mai_ShowFormChangeOtherUsrEmail ();
	       ID_ShowFormChangeOtherUsrID ();
	    HTM_DIV_End ();

	 /***** Begin container for this user *****/
	 HTM_DIV_End ();
	}
      else
	 Ale_ShowAlertUserNotFoundOrYouDoNotHavePermission ();
     }
   else		// User not found
      Ale_ShowAlertUserNotFoundOrYouDoNotHavePermission ();
  }

/*****************************************************************************/
/************* Put an icon (form) to request removing my account *************/
/*****************************************************************************/

void Acc_PutLinkToRemoveMyAccount (__attribute__((unused)) void *Args)
  {
   extern const char *Txt_Remove_account;

   if (Acc_CheckIfICanEliminateAccount (Gbl.Usrs.Me.UsrDat.UsrCod))
      Lay_PutContextualLinkOnlyIcon (ActReqRemMyAcc,NULL,
				     Acc_PutParamsToRemoveMyAccount,Gbl.Usrs.Me.UsrDat.EnUsrCod,
				     "trash.svg",Ico_RED,
				     Txt_Remove_account);
  }

static void Acc_PutParamsToRemoveMyAccount (void *EncryptedUsrCod)
  {
   Usr_PutParamMyUsrCodEncrypted (EncryptedUsrCod);
   Par_PutHiddenParamUnsigned (NULL,"RegRemAction",
                               (unsigned) Enr_ELIMINATE_ONE_USR_FROM_PLATFORM);
  }

/*****************************************************************************/
/*************** Create new user account with an ID and login ****************/
/*****************************************************************************/
// Return true if no error and user can be logged in
// Return false on error

bool Acc_CreateMyNewAccountAndLogIn (void)
  {
   char NewNickWithoutArr[Nck_MAX_BYTES_NICK_WITHOUT_ARROBA + 1];
   char NewEmail[Cns_MAX_BYTES_EMAIL_ADDRESS + 1];
   char NewEncryptedPassword[Pwd_BYTES_ENCRYPTED_PASSWORD + 1];

   if (Acc_GetParamsNewAccount (NewNickWithoutArr,NewEmail,NewEncryptedPassword))
     {
      /***** User's has no ID *****/
      Gbl.Usrs.Me.UsrDat.IDs.Num = 0;
      Gbl.Usrs.Me.UsrDat.IDs.List = NULL;

      /***** Set password to the password typed by the user *****/
      Str_Copy (Gbl.Usrs.Me.UsrDat.Password,NewEncryptedPassword,
                sizeof (Gbl.Usrs.Me.UsrDat.Password) - 1);

      /***** User does not exist in the platform, so create him/her! *****/
      Acc_CreateNewUsr (&Gbl.Usrs.Me.UsrDat,
                        true);	// I am creating my own account

      /***** Save nickname *****/
      Nck_DB_UpdateNick (Gbl.Usrs.Me.UsrDat.UsrCod,NewNickWithoutArr);
      Str_Copy (Gbl.Usrs.Me.UsrDat.Nickname,NewNickWithoutArr,
                sizeof (Gbl.Usrs.Me.UsrDat.Nickname) - 1);

      /***** Save email *****/
      if (Mai_UpdateEmailInDB (&Gbl.Usrs.Me.UsrDat,NewEmail))
	{
	 /* Email updated sucessfully */
	 Str_Copy (Gbl.Usrs.Me.UsrDat.Email,NewEmail,
	           sizeof (Gbl.Usrs.Me.UsrDat.Email) - 1);

	 Gbl.Usrs.Me.UsrDat.EmailConfirmed = false;
	}

      return true;
     }
   else
     {
      /***** Show form again ******/
      Acc_ShowFormRequestNewAccountWithParams (NewNickWithoutArr,NewEmail);
      return false;
     }
  }

/*****************************************************************************/
/************* Get parameters for the creation of a new account **************/
/*****************************************************************************/
// Return false on error

static bool Acc_GetParamsNewAccount (char NewNickWithoutArr[Nck_MAX_BYTES_NICK_WITHOUT_ARROBA + 1],
                                     char NewEmail[Cns_MAX_BYTES_EMAIL_ADDRESS + 1],
                                     char NewEncryptedPassword[Pwd_BYTES_ENCRYPTED_PASSWORD + 1])
  {
   extern const char *Txt_The_nickname_had_been_registered_by_another_user;
   extern const char *Txt_The_nickname_is_not_valid_;
   extern const char *Txt_The_email_address_X_had_been_registered_by_another_user;
   extern const char *Txt_The_email_address_entered_X_is_not_valid;
   char NewNick[Nck_MAX_BYTES_NICK_WITH_ARROBA + 1];
   char NewPlainPassword[Pwd_MAX_BYTES_PLAIN_PASSWORD + 1];
   bool Error = false;

   /***** Step 1/3: Get new nickname from form *****/
   Par_GetParToText ("NewNick",NewNick,sizeof (NewNick) - 1);

   /* Remove arrobas at the beginning */
   Str_RemoveLeadingArrobas (NewNick);
   Str_Copy (NewNickWithoutArr,NewNick,sizeof (NewNick) - 1);

   /* Create a new version of the nickname with arroba */
   snprintf (NewNick,sizeof (NewNick),"@%s",
	     NewNickWithoutArr);

   if (Nck_CheckIfNickWithArrIsValid (NewNick))        // If new nickname is valid
     {
      /* Check if the new nickname
         matches any of the nicknames of other users */
      if (Acc_DB_CheckIfNicknameAlreadyExists (NewNickWithoutArr))
	{
	 Error = true;
	 Ale_ShowAlert (Ale_WARNING,Txt_The_nickname_had_been_registered_by_another_user);
	}
     }
   else        // New nickname is not valid
     {
      Error = true;
      Ale_ShowAlert (Ale_WARNING,Txt_The_nickname_is_not_valid_,
		     Nck_MIN_CHARS_NICK_WITHOUT_ARROBA,
		     Nck_MAX_CHARS_NICK_WITHOUT_ARROBA);
     }

   /***** Step 2/3: Get new email from form *****/
   Par_GetParToText ("NewEmail",NewEmail,Cns_MAX_BYTES_EMAIL_ADDRESS);

   if (Mai_CheckIfEmailIsValid (NewEmail))	// New email is valid
     {
      /* Check if the new email matches
         any of the confirmed emails of other users */
      if (Acc_DB_CheckIfEmailAlreadyExists (NewEmail))	// An email of another user is the same that my email
	{
	 Error = true;
	 Ale_ShowAlert (Ale_WARNING,Txt_The_email_address_X_had_been_registered_by_another_user,
		        NewEmail);
	}
     }
   else	// New email is not valid
     {
      Error = true;
      Ale_ShowAlert (Ale_WARNING,Txt_The_email_address_entered_X_is_not_valid,
                     NewEmail);
     }

   /***** Step 3/3: Get new password from form *****/
   Par_GetParToText ("Paswd",NewPlainPassword,Pwd_MAX_BYTES_PLAIN_PASSWORD);
   Cry_EncryptSHA512Base64 (NewPlainPassword,NewEncryptedPassword);
   if (!Pwd_SlowCheckIfPasswordIsGood (NewPlainPassword,NewEncryptedPassword,-1L))        // New password is good?
     {
      Error = true;
      Ale_ShowAlerts (NULL);	// Error message is set in Pwd_SlowCheckIfPasswordIsGood
     }

   return !Error;
  }

/*****************************************************************************/
/****************************** Create new user ******************************/
/*****************************************************************************/
// UsrDat->UsrCod must be <= 0
// UsrDat->UsrDat.IDs must contain a list of IDs for the new user

void Acc_CreateNewUsr (struct UsrData *UsrDat,bool CreatingMyOwnAccount)
  {
   char PathRelUsr[PATH_MAX + 1];
   unsigned NumID;

   /***** Trivial check: user's code should be <= 0 *****/
   if (UsrDat->UsrCod > 0)
      Err_ShowErrorAndExit ("Can not create new user.");

   /***** Create encrypted user's code *****/
   Acc_CreateNewEncryptedUsrCod (UsrDat);

   /***** Filter some user's data before inserting */
   Usr_FilterUsrBirthday (&UsrDat->Birthday);

   /***** Insert new user in database *****/
   /* Insert user's data */
   UsrDat->UsrCod = Acc_DB_CreateNewUsr (UsrDat);

   /* Insert user's IDs as confirmed */
   for (NumID = 0;
	NumID < UsrDat->IDs.Num;
	NumID++)
     {
      Str_ConvertToUpperText (UsrDat->IDs.List[NumID].ID);
      ID_DB_InsertANewUsrID (UsrDat->UsrCod,
		             UsrDat->IDs.List[NumID].ID,
		             UsrDat->IDs.List[NumID].Confirmed);
     }

   /***** Create directory for the user, if not exists *****/
   Usr_ConstructPathUsr (UsrDat->UsrCod,PathRelUsr);
   Fil_CreateDirIfNotExists (PathRelUsr);

   /***** Create user's figures *****/
   Prf_CreateNewUsrFigures (UsrDat->UsrCod,CreatingMyOwnAccount);
  }

/*****************************************************************************/
/******************** Create a new encrypted user's code *********************/
/*****************************************************************************/

#define LENGTH_RANDOM_STR 32
#define MAX_TRY 10

static void Acc_CreateNewEncryptedUsrCod (struct UsrData *UsrDat)
  {
   char RandomStr[LENGTH_RANDOM_STR + 1];
   unsigned NumTry;

   for (NumTry = 0;
        NumTry < MAX_TRY;
        NumTry++)
     {
      Str_CreateRandomAlphanumStr (RandomStr,LENGTH_RANDOM_STR);
      Cry_EncryptSHA256Base64 (RandomStr,UsrDat->EnUsrCod);
      if (Usr_DB_GetUsrCodFromEncryptedUsrCod (UsrDat->EnUsrCod) <= 0)
         break;
     }
   if (NumTry == MAX_TRY)
      Err_ShowErrorAndExit ("Can not create a new encrypted user's code.");
   }

/*****************************************************************************/
/***************** Message after creation of a new account *******************/
/*****************************************************************************/

void Acc_AfterCreationNewAccount (void)
  {
   extern const char *Txt_Congratulations_You_have_created_your_account_X_Now_Y_will_request_you_;

   if (Gbl.Usrs.Me.Logged)	// If account has been created without problem, I am logged
     {
      /***** Show message of success *****/
      Ale_ShowAlert (Ale_SUCCESS,Txt_Congratulations_You_have_created_your_account_X_Now_Y_will_request_you_,
	             Gbl.Usrs.Me.UsrDat.Nickname,
	             Cfg_PLATFORM_SHORT_NAME);

      /***** Show form with account data *****/
      Acc_ShowFormChgMyAccount ();
     }
  }

/*****************************************************************************/
/************** Definite removing of a user from the platform ****************/
/*****************************************************************************/

void Acc_GetUsrCodAndRemUsrGbl (void)
  {
   bool Error = false;

   if (Usr_GetParamOtherUsrCodEncryptedAndGetUsrData ())
     {
      if (Acc_CheckIfICanEliminateAccount (Gbl.Usrs.Other.UsrDat.UsrCod))
         Acc_ReqRemAccountOrRemAccount (Acc_REMOVE_USR);
      else
         Error = true;
     }
   else
      Error = true;

   if (Error)
      Ale_ShowAlertUserNotFoundOrYouDoNotHavePermission ();
  }

/*****************************************************************************/
/*************************** Remove a user account ***************************/
/*****************************************************************************/

void Acc_ReqRemAccountOrRemAccount (Acc_ReqOrRemUsr_t RequestOrRemove)
  {
   bool ItsMe = Usr_ItsMe (Gbl.Usrs.Other.UsrDat.UsrCod);

   switch (RequestOrRemove)
     {
      case Acc_REQUEST_REMOVE_USR:	// Ask if eliminate completely the user from the platform
	 Acc_AskIfRemoveUsrAccount (ItsMe);
	 break;
      case Acc_REMOVE_USR:		// Eliminate completely the user from the platform
	 if (Pwd_GetConfirmationOnDangerousAction ())
	   {
	    Acc_CompletelyEliminateAccount (&Gbl.Usrs.Other.UsrDat,Cns_VERBOSE);

	    /***** Move unused contents of messages to table of deleted contents of messages *****/
	    Msg_DB_MoveUnusedMsgsContentToDeleted ();
	   }
	 else
	    Acc_AskIfRemoveUsrAccount (ItsMe);
	 break;
     }
  }

/*****************************************************************************/
/******** Check if I can eliminate completely another user's account *********/
/*****************************************************************************/

bool Acc_CheckIfICanEliminateAccount (long UsrCod)
  {
   bool ItsMe = Usr_ItsMe (UsrCod);

   // A user logged as superuser can eliminate any user except her/him
   // Other users only can eliminate themselves
   return (( ItsMe &&							// It's me
	    (Gbl.Usrs.Me.Role.Available & (1 << Rol_SYS_ADM)) == 0)	// I can not be system admin
	   ||
           (!ItsMe &&							// It's not me
             Gbl.Usrs.Me.Role.Logged == Rol_SYS_ADM));			// I am logged as system admin
  }

/*****************************************************************************/
/*********** Ask if really wanted to eliminate completely a user *************/
/*****************************************************************************/

static void Acc_AskIfRemoveUsrAccount (bool ItsMe)
  {
   if (ItsMe)
      Acc_AskIfRemoveMyAccount ();
   else
      Acc_AskIfRemoveOtherUsrAccount ();
  }

void Acc_AskIfRemoveMyAccount (void)
  {
   extern const char *Txt_Do_you_really_want_to_completely_eliminate_your_user_account;
   extern const char *Txt_Eliminate_my_user_account;

   /***** Show question and button to remove my user account *****/
   /* Begin alert */
   Ale_ShowAlertAndButton1 (Ale_QUESTION,Txt_Do_you_really_want_to_completely_eliminate_your_user_account);

   /* Show my record */
   Rec_ShowSharedRecordUnmodifiable (&Gbl.Usrs.Me.UsrDat);

   /* Show form to request confirmation */
   Frm_BeginForm (ActRemMyAcc);
      Pwd_AskForConfirmationOnDangerousAction ();
      Btn_PutRemoveButton (Txt_Eliminate_my_user_account);
   Frm_EndForm ();

   /* End alert */
   Ale_ShowAlertAndButton2 (ActUnk,NULL,NULL,
                            NULL,NULL,
                            Btn_NO_BUTTON,NULL);

   /***** Show forms to change my account *****/
   Acc_ShowFormChgMyAccount ();
  }

static void Acc_AskIfRemoveOtherUsrAccount (void)
  {
   extern const char *Txt_Do_you_really_want_to_completely_eliminate_the_following_user;
   extern const char *Txt_Eliminate_user_account;

   if (Usr_DB_ChkIfUsrCodExists (Gbl.Usrs.Other.UsrDat.UsrCod))
     {
      /***** Show question and button to remove user account *****/
      /* Begin alert */
      Ale_ShowAlertAndButton1 (Ale_QUESTION,Txt_Do_you_really_want_to_completely_eliminate_the_following_user);

      /* Show user's record */
      Rec_ShowSharedRecordUnmodifiable (&Gbl.Usrs.Other.UsrDat);

      /* Show form to request confirmation */
      Frm_BeginForm (ActRemUsrGbl);
	 Usr_PutParamOtherUsrCodEncrypted (Gbl.Usrs.Other.UsrDat.EnUsrCod);
	 Pwd_AskForConfirmationOnDangerousAction ();
	 Btn_PutRemoveButton (Txt_Eliminate_user_account);
      Frm_EndForm ();

      /* End alert */
      Ale_ShowAlertAndButton2 (ActUnk,NULL,NULL,
                               NULL,NULL,
                               Btn_NO_BUTTON,NULL);
     }
   else
      Ale_ShowAlertUserNotFoundOrYouDoNotHavePermission ();
  }

/*****************************************************************************/
/************* Remove completely a user from the whole platform **************/
/*****************************************************************************/

void Acc_RemoveMyAccount (void)
  {
   if (Pwd_GetConfirmationOnDangerousAction ())
     {
      Acc_CompletelyEliminateAccount (&Gbl.Usrs.Me.UsrDat,Cns_VERBOSE);

      /***** Move unused contents of messages to table of deleted contents of messages *****/
      Msg_DB_MoveUnusedMsgsContentToDeleted ();
     }
   else
      Acc_AskIfRemoveUsrAccount (true);
  }

void Acc_CompletelyEliminateAccount (struct UsrData *UsrDat,
                                     Cns_QuietOrVerbose_t QuietOrVerbose)
  {
   extern const char *Txt_THE_USER_X_has_been_removed_from_all_his_her_courses;
   extern const char *Txt_THE_USER_X_has_been_removed_as_administrator;
   extern const char *Txt_Messages_of_THE_USER_X_have_been_deleted;
   extern const char *Txt_Briefcase_of_THE_USER_X_has_been_removed;
   extern const char *Txt_Photo_of_THE_USER_X_has_been_removed;
   extern const char *Txt_Record_card_of_THE_USER_X_has_been_removed;
   bool PhotoRemoved = false;

   /***** Remove the works zones of the user in all courses *****/
   Brw_RemoveUsrWorksInAllCrss (UsrDat);        // Make this before of removing the user from the courses

   /***** Remove the fields of course record in all courses *****/
   Rec_DB_RemoveAllFieldContentsFromUsrRecords (UsrDat->UsrCod);

   /***** Remove user from all his/her projects *****/
   Prj_RemoveUsrFromProjects (UsrDat->UsrCod);

   /***** Remove user from all attendance events *****/
   Att_DB_RemoveUsrFromAllAttEvents (UsrDat->UsrCod);

   /***** Remove user from all groups of all courses *****/
   Grp_RemUsrFromAllGrps (UsrDat->UsrCod);

   /***** Remove user's requests for enrolment *****/
   Enr_DB_RemUsrRequests (UsrDat->UsrCod);

   /***** Remove user from possible duplicate users *****/
   Dup_DB_RemoveUsrFromDuplicated (UsrDat->UsrCod);

   /***** Remove user from the tables of courses and users *****/
   Set_DB_RemUsrFromAllCrssSettings (UsrDat->UsrCod);
   Enr_DB_RemUsrFromAllCrss (UsrDat->UsrCod);

   if (QuietOrVerbose == Cns_VERBOSE)
      Ale_ShowAlert (Ale_SUCCESS,Txt_THE_USER_X_has_been_removed_from_all_his_her_courses,
                     UsrDat->FullName);

   /***** Remove user as administrator of any degree/center/institution *****/
   Adm_DB_RemUsrAsAdmin (UsrDat->UsrCod);

   if (QuietOrVerbose == Cns_VERBOSE)
      Ale_ShowAlert (Ale_SUCCESS,Txt_THE_USER_X_has_been_removed_as_administrator,
                     UsrDat->FullName);

   /***** Remove user's clipboard in forums *****/
   For_DB_RemoveUsrFromClipboard (UsrDat->UsrCod);

   /***** Remove some files of the user's from database *****/
   Brw_DB_RemoveUsrFiles (UsrDat->UsrCod);

   /***** Remove the file tree of a user *****/
   Acc_RemoveUsrBriefcase (UsrDat);
   if (QuietOrVerbose == Cns_VERBOSE)
      Ale_ShowAlert (Ale_SUCCESS,Txt_Briefcase_of_THE_USER_X_has_been_removed,
                     UsrDat->FullName);

   /***** Remove test, exams and matches made by user in all courses *****/
   TstPrn_RemovePrintsMadeByUsrInAllCrss (UsrDat->UsrCod);
   Exa_DB_RemovePrintQstsMadeByUsrInAllCrss (UsrDat->UsrCod);
   Exa_DB_RemoveAllPrintsMadeByUsrInAllCrss (UsrDat->UsrCod);
   Mch_RemoveMatchesMadeByUsrInAllCrss (UsrDat->UsrCod);

   /***** Remove user's notifications *****/
   Ntf_DB_RemoveUsrNtfs (UsrDat->UsrCod);

   /***** Delete user's messages sent and received *****/
   Msg_DB_RemoveAllRecAndSntMsgsUsr (UsrDat->UsrCod);
   if (QuietOrVerbose == Cns_VERBOSE)
      Ale_ShowAlert (Ale_SUCCESS,Txt_Messages_of_THE_USER_X_have_been_deleted,
                     UsrDat->FullName);

   /***** Remove user from tables of banned users *****/
   Prf_DB_RemoveUsrFromBanned (UsrDat->UsrCod);
   Msg_DB_RemoveUsrFromBanned (UsrDat->UsrCod);

   /***** Delete thread read status for this user *****/
   For_DB_RemoveUsrFromReadThrs (UsrDat->UsrCod);

   /***** Remove user from table of seen announcements *****/
   Ann_DB_RemoveUsrFromSeenAnnouncements (UsrDat->UsrCod);

   /***** Remove user from table of connected users *****/
   Con_DB_RemoveUsrFromConnected (UsrDat->UsrCod);

   /***** Remove all sessions of this user *****/
   Ses_DB_RemoveUsrSessions (UsrDat->UsrCod);

   /***** Remove social content associated to the user *****/
   Tml_Usr_RemoveUsrContent (UsrDat->UsrCod);

   /***** Remove user's figures *****/
   Prf_DB_RemoveUsrFigures (UsrDat->UsrCod);

   /***** Remove user from table of followers *****/
   Fol_RemoveUsrFromUsrFollow (UsrDat->UsrCod);

   /***** Remove user's usage reports *****/
   Rep_RemoveUsrUsageReports (UsrDat->UsrCod);

   /***** Remove user's agenda *****/
   Agd_DB_RemoveUsrEvents (UsrDat->UsrCod);

   /***** Remove the user from the list of users without photo *****/
   Pho_DB_RemoveUsrFromTableClicksWithoutPhoto (UsrDat->UsrCod);

   /***** Remove user's photo *****/
   PhotoRemoved = Pho_RemovePhoto (UsrDat);
   if (PhotoRemoved && QuietOrVerbose == Cns_VERBOSE)
      Ale_ShowAlert (Ale_SUCCESS,Txt_Photo_of_THE_USER_X_has_been_removed,
                     UsrDat->FullName);

   /***** Remove user's webs / social networks *****/
   Net_DB_RemoveUsrWebs (UsrDat->UsrCod);

   /***** Remove user's nicknames *****/
   Nck_DB_RemoveUsrNicknames (UsrDat->UsrCod);

   /***** Remove user's emails *****/
   Mai_DB_RemoveUsrPendingEmails (UsrDat->UsrCod);
   Mai_DB_RemoveUsrEmails (UsrDat->UsrCod);

   /***** Remove user's IDs *****/
   ID_DB_RemoveUsrIDs (UsrDat->UsrCod);

   /***** Remove user's last data *****/
   Usr_DB_RemoveUsrLastData (UsrDat->UsrCod);

   /***** Remove user's data *****/
   Usr_DB_RemoveUsrData (UsrDat->UsrCod);

   if (QuietOrVerbose == Cns_VERBOSE)
      Ale_ShowAlert (Ale_SUCCESS,Txt_Record_card_of_THE_USER_X_has_been_removed,
                     UsrDat->FullName);
  }

/*****************************************************************************/
/********************** Remove the briefcase of a user ***********************/
/*****************************************************************************/

static void Acc_RemoveUsrBriefcase (struct UsrData *UsrDat)
  {
   char PathRelUsr[PATH_MAX + 1];

   /***** Remove files of the user's briefcase from disc *****/
   Usr_ConstructPathUsr (UsrDat->UsrCod,PathRelUsr);
   Fil_RemoveTree (PathRelUsr);
  }

/*****************************************************************************/
/********* Put an icon to the action used to manage user's account ***********/
/*****************************************************************************/

void Acc_PutIconToChangeUsrAccount (void)
  {
   extern const char *Txt_Change_account;
   static const Act_Action_t NextAction[Rol_NUM_ROLES] =
     {
      [Rol_UNK	  ] = ActFrmAccOth,
      [Rol_GST	  ] = ActFrmAccOth,
      [Rol_USR	  ] = ActFrmAccOth,
      [Rol_STD	  ] = ActFrmAccStd,
      [Rol_NET	  ] = ActFrmAccTch,
      [Rol_TCH	  ] = ActFrmAccTch,
      [Rol_DEG_ADM] = ActFrmAccOth,
      [Rol_CTR_ADM] = ActFrmAccOth,
      [Rol_INS_ADM] = ActFrmAccOth,
      [Rol_SYS_ADM] = ActFrmAccOth,
     };

   /***** Link for changing the account *****/
   if (Usr_ItsMe (Gbl.Record.UsrDat->UsrCod))
      Lay_PutContextualLinkOnlyIcon (ActFrmMyAcc,NULL,
                                     NULL,NULL,
			             "at.svg",Ico_BLACK,
			             Txt_Change_account);
   else	// Not me
      if (Usr_ICanEditOtherUsr (Gbl.Record.UsrDat))
	 Lay_PutContextualLinkOnlyIcon (NextAction[Gbl.Record.UsrDat->Roles.InCurrentCrs],NULL,
	                                Rec_PutParamUsrCodEncrypted,NULL,
	                                "at.svg",Ico_BLACK,
				        Txt_Change_account);
  }
