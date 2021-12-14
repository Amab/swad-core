// swad_ID.c: Users' IDs

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

#include <ctype.h>		// For isalnum, isdigit, etc.
#include <stdbool.h>		// For boolean type
#include <stdlib.h>		// For exit, system, malloc, free, rand, etc.
#include <string.h>		// For string functions

#include "swad_account.h"
#include "swad_box.h"
#include "swad_database.h"
#include "swad_error.h"
#include "swad_form.h"
#include "swad_global.h"
#include "swad_hierarchy_level.h"
#include "swad_HTML.h"
#include "swad_ID.h"
#include "swad_ID_database.h"
#include "swad_parameter.h"
#include "swad_QR.h"
#include "swad_user.h"

/*****************************************************************************/
/************** External global variables from others modules ****************/
/*****************************************************************************/

extern struct Globals Gbl;

/*****************************************************************************/
/***************************** Private constants *****************************/
/*****************************************************************************/

#define ID_MAX_IDS_PER_USER	3	// Maximum number of IDs per user

/*****************************************************************************/
/***************************** Private variables *****************************/
/*****************************************************************************/

const char *ID_ID_SECTION_ID = "id_section";

/*****************************************************************************/
/***************************** Private prototypes ****************************/
/*****************************************************************************/

static bool ID_CheckIfUsrIDIsValidUsingMinDigits (const char *UsrID,unsigned MinDigits);

static void ID_PutLinkToConfirmID (struct UsrData *UsrDat,unsigned NumID,
                                   const char *Anchor);

static void ID_ShowFormChangeUsrID (bool ItsMe,bool IShouldFillInID);

static void ID_PutParamsRemoveMyID (void *ID);
static void ID_PutParamsRemoveOtherID (void *ID);

static void ID_RemoveUsrID (const struct UsrData *UsrDat,bool ItsMe);
static void ID_NewUsrID (const struct UsrData *UsrDat,bool ItsMe);

/*****************************************************************************/
/********************** Get list of IDs of a user ****************************/
/*****************************************************************************/

void ID_GetListIDsFromUsrCod (struct UsrData *UsrDat)
  {
   MYSQL_RES *mysql_res;
   MYSQL_ROW row;
   unsigned NumIDs;
   unsigned NumID;

   /***** Initialize list of IDs to an empty list *****/
   ID_FreeListIDs (UsrDat);

   if (UsrDat->UsrCod > 0)
     {
      /***** Get user's IDs from database *****/
      // First the confirmed  (Confirmed == 'Y')
      // Then the unconfirmed (Confirmed == 'N')
      if ((NumIDs = ID_DB_GetIDsFromUsrCod (&mysql_res,UsrDat->UsrCod)))
	{
	 /***** Allocate space for the list *****/
         ID_ReallocateListIDs (UsrDat,NumIDs);

         /***** Get list of IDs *****/
	 for (NumID = 0;
	      NumID < NumIDs;
	      NumID++)
	   {
            row = mysql_fetch_row (mysql_res);

	    /* Get ID from row[0] */
            Str_Copy (UsrDat->IDs.List[NumID].ID,row[0],
                      sizeof (UsrDat->IDs.List[NumID].ID) - 1);

            /* Get if ID is confirmed from row[1] */
            UsrDat->IDs.List[NumID].Confirmed = (row[1][0] == 'Y');
	   }
        }

      /***** Free structure that stores the query result *****/
      DB_FreeMySQLResult (&mysql_res);
     }
  }

/*****************************************************************************/
/***************** Free memory allocated for list of IDs *********************/
/*****************************************************************************/

void ID_ReallocateListIDs (struct UsrData *UsrDat,unsigned NumIDs)
  {
   /***** Free list of IDs if used *****/
   ID_FreeListIDs (UsrDat);

   /***** Assign number of IDs in list *****/
   UsrDat->IDs.Num = NumIDs;

   /***** Allocate space for the list *****/
   if ((UsrDat->IDs.List = malloc (NumIDs * sizeof (*UsrDat->IDs.List))) == NULL)
      Err_NotEnoughMemoryExit ();
  }

/*****************************************************************************/
/***************** Free memory allocated for list of IDs *********************/
/*****************************************************************************/

void ID_FreeListIDs (struct UsrData *UsrDat)
  {
   /***** Free list *****/
   if (UsrDat->IDs.Num && UsrDat->IDs.List)
      free (UsrDat->IDs.List);

   /***** Reset list *****/
   UsrDat->IDs.List = NULL;
   UsrDat->IDs.Num = 0;
  }

/*****************************************************************************/
/***************** Get list of user codes from user's IDs ********************/
/*****************************************************************************/
// Returns the number of users with any of these IDs
// The list of users' codes is allocated inside this function and should be freed by caller

unsigned ID_GetListUsrCodsFromUsrID (struct UsrData *UsrDat,
                                     const char *EncryptedPassword,	// If NULL or empty ==> do not check password
                                     struct ListUsrCods *ListUsrCods,
                                     bool OnlyConfirmedIDs)
  {
   MYSQL_RES *mysql_res;
   unsigned NumUsr;

   if (UsrDat->IDs.Num)
     {
      if ((ListUsrCods->NumUsrs = ID_DB_GetUsrCodsFromUsrID (&mysql_res,
                                                             UsrDat,
                                                             EncryptedPassword,
                                                             OnlyConfirmedIDs)))
        {
	 /***** Allocate space for the list of users' codes *****/
	 Usr_AllocateListUsrCods (ListUsrCods);
	 // The list should be freed by caller

	 /***** Fill the list *****/
	 for (NumUsr = 0;
	      NumUsr < ListUsrCods->NumUsrs;
	      NumUsr++)
	    /* Get user's code */
	    ListUsrCods->Lst[NumUsr] = DB_GetNextCode (mysql_res);
	 UsrDat->UsrCod = ListUsrCods->Lst[0];	// The first user found
        }
      else	// ListUsrCods->NumUsrs == 0
        {
         ListUsrCods->Lst = NULL;
         UsrDat->UsrCod = -1L;
	}

      /***** Free structure that stores the query result *****/
      DB_FreeMySQLResult (&mysql_res);
     }
   else	// List of user's IDs is empty
     {
      ListUsrCods->NumUsrs = 0;
      ListUsrCods->Lst = NULL;
      UsrDat->UsrCod = -1L;
     }

   return ListUsrCods->NumUsrs;
  }

/*****************************************************************************/
/******* Put hidden parameter with the plain user's ID of other user *********/
/*****************************************************************************/

void ID_PutParamOtherUsrIDPlain (void)
  {
   Par_PutHiddenParamString (NULL,"OtherUsrID",
	                     (Gbl.Usrs.Other.UsrDat.IDs.Num &&
	                      Gbl.Usrs.Other.UsrDat.IDs.List) ? Gbl.Usrs.Other.UsrDat.IDs.List[0].ID :
	                                                        "");
  }

/*****************************************************************************/
/********* Get parameter plain user's ID of other user from a form ***********/
/*****************************************************************************/

void ID_GetParamOtherUsrIDPlain (void)
  {
   /***** Allocate space for the list *****/
   ID_ReallocateListIDs (&Gbl.Usrs.Other.UsrDat,1);

   /***** Get parameter *****/
   Par_GetParToText ("OtherUsrID",Gbl.Usrs.Other.UsrDat.IDs.List[0].ID,
                     ID_MAX_BYTES_USR_ID);
   // Users' IDs are always stored internally in capitals and without leading zeros
   Str_RemoveLeadingZeros (Gbl.Usrs.Other.UsrDat.IDs.List[0].ID);
   Str_ConvertToUpperText (Gbl.Usrs.Other.UsrDat.IDs.List[0].ID);

   Gbl.Usrs.Other.UsrDat.IDs.List[0].Confirmed = true;
  }

/*****************************************************************************/
/****** Check whether a user's ID without the ending letter is valid *********/
/*****************************************************************************/
// Returns true if the user's ID string is valid, or false if not
// A valid user's ID must...:
// 1. Must be ID_MIN_BYTES_USR_ID <= characters <= ID_MAX_BYTES_USR_ID.
// 2. All characters must be digits or letters
// 3. Must have a minimum number of digits

// Wrapper function to avoid passing extra parameters
bool ID_CheckIfUsrIDIsValid (const char *UsrID)
  {
   if (UsrID)
      if (UsrID[0])
         return ID_CheckIfUsrIDIsValidUsingMinDigits (UsrID,ID_MIN_DIGITS_USR_ID);

   return false;
  }

// Wrapper function to avoid passing extra parameters
bool ID_CheckIfUsrIDSeemsAValidID (const char *UsrID)
  {
   if (UsrID)
      if (UsrID[0])
         return ID_CheckIfUsrIDIsValidUsingMinDigits (UsrID,ID_MIN_DIGITS_AUTOMATIC_DETECT_USR_ID);

   return false;
  }

static bool ID_CheckIfUsrIDIsValidUsingMinDigits (const char *UsrID,unsigned MinDigits)
  {
   const char *Ptr;
   unsigned NumDigits = 0;
   unsigned Length;

   /***** Check length *****/
   if (!UsrID)
      return false;
   if (!UsrID[0])
      return false;
   Length = strlen (UsrID);
   if (Length < ID_MIN_BYTES_USR_ID ||
       Length > ID_MAX_BYTES_USR_ID)
      return false;			// 1. Must be ID_MIN_BYTES_USR_ID <= characters <= ID_MAX_BYTES_USR_ID

   /**** Loop through user's ID *****/
   for (Ptr = UsrID;
        *Ptr;
        Ptr++)
      if (isdigit ((int) *Ptr))                        // If character is digit
         NumDigits++;
      else if (!((*Ptr >= 'A' && *Ptr <= 'Z') ||
                 (*Ptr >= 'a' && *Ptr <= 'z')))        // If character is not alpha
         return false;			// 2. All characters must be digits or letters

   return (NumDigits >= MinDigits);	// 3. Must have MinDigits digits at least
  }

/*****************************************************************************/
/*************************** Write list of user's ID *************************/
/*****************************************************************************/

void ID_WriteUsrIDs (struct UsrData *UsrDat,const char *Anchor)
  {
   unsigned NumID;
   bool ICanSeeUsrID;
   bool ICanConfirmUsrID;

   ICanSeeUsrID = ID_ICanSeeOtherUsrIDs (UsrDat);
   ICanConfirmUsrID = ICanSeeUsrID &&
	              (UsrDat->UsrCod != Gbl.Usrs.Me.UsrDat.UsrCod) &&		// Not me
	              !Gbl.Form.Inside &&					// Not inside another form
                      Act_GetBrowserTab (Gbl.Action.Act) == Act_BRW_1ST_TAB;	// Only in main browser tab

   for (NumID = 0;
	NumID < UsrDat->IDs.Num;
	NumID++)
     {
      if (NumID)
	 HTM_BR ();

      HTM_SPAN_Begin ("class=\"%s\"",
	              UsrDat->IDs.List[NumID].Confirmed ? "USR_ID_C" :
						          "USR_ID_NC");
      if (ICanSeeUsrID)
	 HTM_Txt (UsrDat->IDs.List[NumID].ID);
      else
	 HTM_Txt ("********");
      HTM_SPAN_End ();

      if (ICanConfirmUsrID &&
	  !UsrDat->IDs.List[NumID].Confirmed)
	 ID_PutLinkToConfirmID (UsrDat,NumID,Anchor);
     }
  }

/*****************************************************************************/
/***************** Check if I can see another user's IDs *********************/
/*****************************************************************************/

bool ID_ICanSeeOtherUsrIDs (const struct UsrData *UsrDat)
  {
   if (Usr_ItsMe (UsrDat->UsrCod))
      return true;

   /***** Check if I have permission to see another user's IDs *****/
   switch (Gbl.Usrs.Me.Role.Logged)
     {
      case Rol_NET:
      case Rol_TCH:
	 /* Check 1: I can see the IDs of users who do not exist in database */
         if (UsrDat->UsrCod <= 0)	// User does not exist (when creating a new user)
            return true;

	 /* Check 2: I can see the IDs of confirmed students */
         if (UsrDat->Roles.InCurrentCrs == Rol_STD &&	// A student
	     UsrDat->Accepted)				// who accepted registration
            return true;

         /* Check 3: I can see the IDs of users with user's data empty */
         // This check is made to not view simultaneously:
         // - an ID
         // - a name or an email
         if (!UsrDat->Password[0] &&	// User has no password (never logged)
	     !UsrDat->Surname1[0] &&	// and who has no surname 1 (nobody filled user's surname 1)
	     !UsrDat->Surname2[0] &&	// and who has no surname 2 (nobody filled user's surname 2)
	     !UsrDat->FrstName[0] &&	// and who has no first name (nobody filled user's first name)
             !UsrDat->Email[0])		// and who has no email (nobody filled user's email)
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
/****************** Put a link to confirm another user's ID ******************/
/*****************************************************************************/

static void ID_PutLinkToConfirmID (struct UsrData *UsrDat,unsigned NumID,
                                   const char *Anchor)
  {
   extern const char *The_ClassFormLinkOutBoxBold[The_NUM_THEMES];
   extern const char *Txt_Confirm_ID;
   static const Act_Action_t NextAction[Rol_NUM_ROLES] =
     {
      [Rol_UNK	  ] = ActCnfID_Oth,
      [Rol_GST	  ] = ActCnfID_Oth,
      [Rol_USR    ] = ActCnfID_Oth,
      [Rol_STD	  ] = ActCnfID_Std,
      [Rol_NET	  ] = ActCnfID_Tch,
      [Rol_TCH	  ] = ActCnfID_Tch,
      [Rol_DEG_ADM] = ActCnfID_Oth,
      [Rol_CTR_ADM] = ActCnfID_Oth,
      [Rol_INS_ADM] = ActCnfID_Oth,
      [Rol_SYS_ADM] = ActCnfID_Oth,
     };

   /***** Begin form *****/
   Frm_BeginFormAnchor (NextAction[UsrDat->Roles.InCurrentCrs],Anchor);
      if (Gbl.Action.Original != ActUnk)
	{
	 Par_PutHiddenParamLong (NULL,"OriginalActCod",
				 Act_GetActCod (Gbl.Action.Original));	// Original action, used to know where we came from
	 switch (Gbl.Action.Original)
	   {
	    case ActSeeRecSevGst:
	    case ActSeeRecSevStd:
	    case ActSeeRecSevTch:
	       Usr_PutHiddenParSelectedUsrsCods (&Gbl.Usrs.Selected);
	       break;
	    default:
	       break;
	   }
	}
      Usr_PutParamUsrCodEncrypted (UsrDat->EnUsrCod);
      Par_PutHiddenParamString (NULL,"UsrID",UsrDat->IDs.List[NumID].ID);

      /***** Put link *****/
      HTM_BUTTON_SUBMIT_Begin (Txt_Confirm_ID,The_ClassFormLinkOutBoxBold[Gbl.Prefs.Theme],NULL);
	 Ico_PutIconTextLink ("check.svg",Txt_Confirm_ID);
      HTM_BUTTON_End ();

   /***** End form *****/
   Frm_EndForm ();
  }

/*****************************************************************************/
/*********************** Show form to change my user's ID ********************/
/*****************************************************************************/

void ID_ShowFormChangeMyID (bool IShouldFillInID)
  {
   extern const char *Hlp_PROFILE_Account;
   extern const char *Txt_ID;
   char StrRecordWidth[Cns_MAX_DECIMAL_DIGITS_UINT + 2 + 1];

   /***** Begin section *****/
   HTM_SECTION_Begin (ID_ID_SECTION_ID);

      /***** Begin box *****/
      snprintf (StrRecordWidth,sizeof (StrRecordWidth),"%upx",Rec_RECORD_WIDTH);
      Box_BoxBegin (StrRecordWidth,Txt_ID,
		    Acc_PutLinkToRemoveMyAccount,NULL,
		    Hlp_PROFILE_Account,Box_NOT_CLOSABLE);

	 /***** Show form to change ID *****/
	 ID_ShowFormChangeUsrID (true,	// ItsMe
				 IShouldFillInID);

      /***** End box *****/
      Box_BoxEnd ();

   /***** End section *****/
   HTM_SECTION_End ();
  }

/*****************************************************************************/
/*********************** Show form to change my user's ID ********************/
/*****************************************************************************/

void ID_ShowFormChangeOtherUsrID (void)
  {
   extern const char *Hlp_PROFILE_Account;
   extern const char *Txt_ID;
   char StrRecordWidth[Cns_MAX_DECIMAL_DIGITS_UINT + 2 + 1];

   /***** Begin section *****/
   HTM_SECTION_Begin (ID_ID_SECTION_ID);

      /***** Begin box *****/
      snprintf (StrRecordWidth,sizeof (StrRecordWidth),"%upx",Rec_RECORD_WIDTH);
      Box_BoxBegin (StrRecordWidth,Txt_ID,
		    NULL,NULL,
		    Hlp_PROFILE_Account,Box_NOT_CLOSABLE);

	 /***** Show form to change ID *****/
	 ID_ShowFormChangeUsrID (false,	// ItsMe
				 false);	// IShouldFillInID

      /***** End box *****/
      Box_BoxEnd ();

   /***** End section *****/
   HTM_SECTION_End ();
  }

/*****************************************************************************/
/*********************** Show form to change my user's ID ********************/
/*****************************************************************************/

static void ID_ShowFormChangeUsrID (bool ItsMe,bool IShouldFillInID)
  {
   extern const char *Hlp_PROFILE_Account;
   extern const char *The_ClassDat[The_NUM_THEMES];
   extern const char *Txt_Please_fill_in_your_ID;
   extern const char *Txt_ID_X_confirmed;
   extern const char *Txt_ID_X_not_confirmed;
   extern const char *Txt_ID;
   extern const char *Txt_Another_ID;
   extern const char *Txt_Add_this_ID;
   extern const char *Txt_The_ID_is_used_in_order_to_facilitate_;
   unsigned NumID;
   static const struct
     {
      Act_Action_t Remove;
      Act_Action_t New;
     } NextAction[Rol_NUM_ROLES] =
     {
      [Rol_UNK	  ] = {ActRemID_Oth,ActNewID_Oth},
      [Rol_GST	  ] = {ActRemID_Oth,ActNewID_Oth},
      [Rol_USR    ] = {ActRemID_Oth,ActNewID_Oth},
      [Rol_STD	  ] = {ActRemID_Std,ActNewID_Std},
      [Rol_NET	  ] = {ActRemID_Tch,ActNewID_Tch},
      [Rol_TCH	  ] = {ActRemID_Tch,ActNewID_Tch},
      [Rol_DEG_ADM] = {ActRemID_Oth,ActNewID_Oth},
      [Rol_CTR_ADM] = {ActRemID_Oth,ActNewID_Oth},
      [Rol_INS_ADM] = {ActRemID_Oth,ActNewID_Oth},
      [Rol_SYS_ADM] = {ActRemID_Oth,ActNewID_Oth},
     };
   const struct UsrData *UsrDat = (ItsMe ? &Gbl.Usrs.Me.UsrDat :
	                                   &Gbl.Usrs.Other.UsrDat);

   /***** Show possible alerts *****/
   Ale_ShowAlerts (ID_ID_SECTION_ID);

   /***** Help message *****/
   if (IShouldFillInID)
      Ale_ShowAlert (Ale_WARNING,Txt_Please_fill_in_your_ID);

   /***** Begin table *****/
   HTM_TABLE_BeginWidePadding (2);

      /***** List existing user's IDs *****/
      for (NumID = 0;
	   NumID < UsrDat->IDs.Num;
	   NumID++)
	{
	 if (NumID == 0)
	   {
	    HTM_TR_Begin (NULL);

	       /* Label */
	       Frm_LabelColumn ("REC_C1_BOT RT",NULL,Txt_ID);

	       /* Data */
	       HTM_TD_Begin ("class=\"REC_C2_BOT LT USR_ID\"");
	   }
	 else	// NumID >= 1
	    HTM_BR ();

	 if (UsrDat->IDs.Num > 1)	// I have two or more IDs
	   {
	    if (ItsMe && UsrDat->IDs.List[NumID].Confirmed)	// I can not remove my confirmed IDs
	       /* Put disabled icon to remove user's ID */
	       Ico_PutIconRemovalNotAllowed ();
	    else							// I can remove
	      {
	       /* Form to remove user's ID */
	       if (ItsMe)
		  Ico_PutContextualIconToRemove (ActRemMyID,ID_ID_SECTION_ID,
						 ID_PutParamsRemoveMyID,UsrDat->IDs.List[NumID].ID);
	       else
		  Ico_PutContextualIconToRemove (NextAction[UsrDat->Roles.InCurrentCrs].Remove,ID_ID_SECTION_ID,
						 ID_PutParamsRemoveOtherID,UsrDat->IDs.List[NumID].ID);
	      }
	   }

	    /* User's ID */
	    HTM_SPAN_Begin ("class=\"%s\" title=\"%s\"",
			    UsrDat->IDs.List[NumID].Confirmed ? "USR_ID_C" :
								"USR_ID_NC",
			    Str_BuildStringStr (UsrDat->IDs.List[NumID].Confirmed ? Txt_ID_X_confirmed :
										    Txt_ID_X_not_confirmed,
						UsrDat->IDs.List[NumID].ID));
	    Str_FreeStrings ();
	       HTM_Txt (UsrDat->IDs.List[NumID].ID);
	       HTM_Txt (UsrDat->IDs.List[NumID].Confirmed ? "&check;" :
							     "");
	    HTM_SPAN_End ();

	 if (NumID == UsrDat->IDs.Num - 1)
	   {
	       HTM_TD_End ();
	    HTM_TR_End ();
	   }
	}

      if (UsrDat->IDs.Num < ID_MAX_IDS_PER_USER)
	{
	 /***** Write help text *****/
	 HTM_TR_Begin (NULL);

	    HTM_TD_Begin ("colspan=\"2\" class=\"%s CM\"",
	                  The_ClassDat[Gbl.Prefs.Theme]);
	       Ale_ShowAlert (Ale_INFO,Txt_The_ID_is_used_in_order_to_facilitate_);
	    HTM_TD_End ();

	 HTM_TR_End ();

	 /***** Form to enter new user's ID *****/
	 HTM_TR_Begin (NULL);

	    /* Label */
	    Frm_LabelColumn ("REC_C1_BOT RT","NewID",
			     UsrDat->IDs.Num ? Txt_Another_ID :	// A new user's ID
					       Txt_ID);		// The first user's ID

	    /* Data */
	    HTM_TD_Begin ("class=\"REC_C2_BOT LT %s\"",
	                  The_ClassDat[Gbl.Prefs.Theme]);
	       if (ItsMe)
		  Frm_BeginFormAnchor (ActChgMyID,ID_ID_SECTION_ID);
	       else
		 {
		  Frm_BeginFormAnchor (NextAction[UsrDat->Roles.InCurrentCrs].New,ID_ID_SECTION_ID);
		     Usr_PutParamUsrCodEncrypted (UsrDat->EnUsrCod);
		 }
		  HTM_INPUT_TEXT ("NewID",ID_MAX_BYTES_USR_ID,
				  UsrDat->IDs.Num ? UsrDat->IDs.List[UsrDat->IDs.Num - 1].ID :
						    "",	// Show the most recent ID
				  HTM_DONT_SUBMIT_ON_CHANGE,
				  "id=\"NewID\" size=\"18\"");
		  HTM_BR ();
		  Btn_PutCreateButtonInline (Txt_Add_this_ID);
	       Frm_EndForm ();
	    HTM_TD_End ();

	 HTM_TR_End ();
	}

   /***** End table *****/
   HTM_TABLE_End ();
  }

static void ID_PutParamsRemoveMyID (void *ID)
  {
   if (ID)
      Par_PutHiddenParamString (NULL,"UsrID",(char *) ID);
  }

static void ID_PutParamsRemoveOtherID (void *ID)
  {
   if (ID)
     {
      Usr_PutParamUsrCodEncrypted (Gbl.Usrs.Other.UsrDat.EnUsrCod);
      Par_PutHiddenParamString (NULL,"UsrID",(char *) ID);
     }
  }

/*****************************************************************************/
/********************** Remove one of my user's IDs **************************/
/*****************************************************************************/

void ID_RemoveMyUsrID (void)
  {
   /***** Remove user's ID *****/
   ID_RemoveUsrID (&Gbl.Usrs.Me.UsrDat,
		   true);	// It's me

   /***** Update list of IDs *****/
   ID_GetListIDsFromUsrCod (&Gbl.Usrs.Me.UsrDat);

   /***** Show my account again *****/
   Acc_ShowFormChgMyAccount ();
  }

/*****************************************************************************/
/**************** Remove one of the user's IDs of another user ***************/
/*****************************************************************************/

void ID_RemoveOtherUsrID (void)
  {
   /***** Get other user's code from form and get user's data *****/
   if (Usr_GetParamOtherUsrCodEncryptedAndGetUsrData ())
     {
      if (Usr_ICanEditOtherUsr (&Gbl.Usrs.Other.UsrDat))
	{
	 /***** Remove user's ID *****/
	 ID_RemoveUsrID (&Gbl.Usrs.Other.UsrDat,
	                 Usr_ItsMe (Gbl.Usrs.Other.UsrDat.UsrCod));

	 /***** Update list of IDs *****/
	 ID_GetListIDsFromUsrCod (&Gbl.Usrs.Other.UsrDat);

	 /***** Show form again *****/
	 Acc_ShowFormChgOtherUsrAccount ();
	}
      else
	 Ale_ShowAlertUserNotFoundOrYouDoNotHavePermission ();
     }
   else		// User not found
      Ale_ShowAlertUserNotFoundOrYouDoNotHavePermission ();
  }

/*****************************************************************************/
/***************************** Remove user's ID ******************************/
/*****************************************************************************/

static void ID_RemoveUsrID (const struct UsrData *UsrDat,bool ItsMe)
  {
   extern const char *Txt_ID_X_removed;
   extern const char *Txt_You_can_not_delete_this_ID;
   char UsrID[ID_MAX_BYTES_USR_ID + 1];
   bool ICanRemove;

   if (Usr_ICanEditOtherUsr (UsrDat))
     {
      /***** Get user's ID from form *****/
      Par_GetParToText ("UsrID",UsrID,ID_MAX_BYTES_USR_ID);
      // Users' IDs are always stored internally in capitals and without leading zeros
      Str_RemoveLeadingZeros (UsrID);
      Str_ConvertToUpperText (UsrID);

      if (UsrDat->IDs.Num < 2)	// One unique ID
	 ICanRemove = false;
      else if (ItsMe)
	 // I can remove my ID only if it is not confirmed
	 ICanRemove = !ID_DB_CheckIfConfirmed (UsrDat->UsrCod,UsrID);
      else
	 ICanRemove = true;

      if (ICanRemove)
	{
	 /***** Remove one of the user's IDs *****/
	 ID_DB_RemoveUsrID (UsrDat->UsrCod,UsrID);

	 /***** Show message *****/
	 Ale_CreateAlert (Ale_SUCCESS,ID_ID_SECTION_ID,
	                  Txt_ID_X_removed,
		          UsrID);
	}
      else
	 Ale_CreateAlert (Ale_WARNING,ID_ID_SECTION_ID,
	                  Txt_You_can_not_delete_this_ID);
     }
   else
      Ale_CreateAlertUserNotFoundOrYouDoNotHavePermission ();
  }

/*****************************************************************************/
/************************* New user's ID for me ******************************/
/*****************************************************************************/

void ID_NewMyUsrID (void)
  {
   /***** New user's ID *****/
   ID_NewUsrID (&Gbl.Usrs.Me.UsrDat,
		true);	// It's me

   /***** Update list of IDs *****/
   ID_GetListIDsFromUsrCod (&Gbl.Usrs.Me.UsrDat);

   /***** Show my account again *****/
   Acc_ShowFormChgMyAccount ();
  }

/*****************************************************************************/
/********************* New user's ID for another user ************************/
/*****************************************************************************/

void ID_NewOtherUsrID (void)
  {
   /***** Get other user's code from form and get user's data *****/
   if (Usr_GetParamOtherUsrCodEncryptedAndGetUsrData ())
     {
      if (Usr_ICanEditOtherUsr (&Gbl.Usrs.Other.UsrDat))
	{
	 /***** New user's ID *****/
	 ID_NewUsrID (&Gbl.Usrs.Other.UsrDat,
	              Usr_ItsMe (Gbl.Usrs.Other.UsrDat.UsrCod));

	 /***** Update list of IDs *****/
	 ID_GetListIDsFromUsrCod (&Gbl.Usrs.Other.UsrDat);

	 /***** Show form again *****/
	 Acc_ShowFormChgOtherUsrAccount ();
	}
      else
	 Ale_ShowAlertUserNotFoundOrYouDoNotHavePermission ();
     }
   else		// User not found
      Ale_ShowAlertUserNotFoundOrYouDoNotHavePermission ();
  }

/*****************************************************************************/
/***************************** New user's ID *********************************/
/*****************************************************************************/

static void ID_NewUsrID (const struct UsrData *UsrDat,bool ItsMe)
  {
   extern const char *Txt_The_ID_X_matches_one_of_the_existing;
   extern const char *Txt_The_ID_X_has_been_confirmed;
   extern const char *Txt_A_user_can_not_have_more_than_X_IDs;
   extern const char *Txt_The_ID_X_has_been_registered_successfully;
   extern const char *Txt_The_ID_X_is_not_valid;
   char NewID[ID_MAX_BYTES_USR_ID + 1];
   unsigned NumID;
   bool AlreadyExists;
   unsigned NumIDFound = 0;	// Initialized to avoid warning

   if (Usr_ICanEditOtherUsr (UsrDat))
     {
      /***** Get new user's ID from form *****/
      Par_GetParToText ("NewID",NewID,ID_MAX_BYTES_USR_ID);
      // Users' IDs are always stored internally in capitals and without leading zeros
      Str_RemoveLeadingZeros (NewID);
      Str_ConvertToUpperText (NewID);

      if (ID_CheckIfUsrIDIsValid (NewID))        // If new ID is valid
	{
	 /***** Check if the new ID matches any of the old IDs *****/
	 for (NumID = 0, AlreadyExists = false;
	      NumID < UsrDat->IDs.Num && !AlreadyExists;
	      NumID++)
	    if (!strcasecmp (UsrDat->IDs.List[NumID].ID,NewID))
	      {
	       AlreadyExists = true;
	       NumIDFound = NumID;
	      }

	 if (AlreadyExists)	// This new ID was already associated to this user
	   {
	    if (ItsMe || UsrDat->IDs.List[NumIDFound].Confirmed)
	       Ale_CreateAlert (Ale_WARNING,ID_ID_SECTION_ID,
		                Txt_The_ID_X_matches_one_of_the_existing,
			        NewID);
	    else	// It's not me && !Confirmed
	      {
	       /***** Mark this ID as confirmed *****/
	       ID_DB_ConfirmUsrID (UsrDat->UsrCod,NewID);

	       Ale_CreateAlert (Ale_SUCCESS,ID_ID_SECTION_ID,
		                Txt_The_ID_X_has_been_confirmed,
			        NewID);
	      }
	   }
	 else if (UsrDat->IDs.Num >= ID_MAX_IDS_PER_USER)
	    Ale_CreateAlert (Ale_WARNING,ID_ID_SECTION_ID,
		             Txt_A_user_can_not_have_more_than_X_IDs,
		             ID_MAX_IDS_PER_USER);
	 else	// OK ==> add this new ID to my list of IDs
	   {
	    /***** Save this new ID *****/
	    // It's me ==> ID not confirmed
	    // Not me  ==> ID confirmed
	    ID_DB_InsertANewUsrID (UsrDat->UsrCod,NewID,!ItsMe);

	    Ale_CreateAlert (Ale_SUCCESS,ID_ID_SECTION_ID,
		             Txt_The_ID_X_has_been_registered_successfully,
		             NewID);
	   }
	}
      else        // New ID is not valid
	 Ale_CreateAlert (Ale_WARNING,ID_ID_SECTION_ID,
	                  Txt_The_ID_X_is_not_valid,
		          NewID);
     }
   else
      Ale_CreateAlertUserNotFoundOrYouDoNotHavePermission ();
  }

/*****************************************************************************/
/************************ Confirm another user's ID **************************/
/*****************************************************************************/

void ID_ConfirmOtherUsrID (void)
  {
   extern const char *Txt_ID_X_had_already_been_confirmed;
   extern const char *Txt_The_ID_X_has_been_confirmed;
   long OriginalActCod;
   char UsrID[ID_MAX_BYTES_USR_ID + 1];
   bool ICanConfirm;
   bool Found;
   unsigned NumID;
   unsigned NumIDFound = 0;	// Initialized to avoid warning

   /***** Get where we came from *****/
   OriginalActCod = Par_GetParToLong ("OriginalActCod");
   Gbl.Action.Original = Act_GetActionFromActCod (OriginalActCod);

   /***** Get other user's code from form and get user's data *****/
   ICanConfirm = false;
   if (Usr_GetParamOtherUsrCodEncryptedAndGetUsrData ())
      if (!Usr_ItsMe (Gbl.Usrs.Other.UsrDat.UsrCod))	// Not me
        {
	 /* If user is a student in current course,
	    check if he/she has accepted */
	 if (Gbl.Hierarchy.Level == HieLvl_CRS)
	    if (Gbl.Usrs.Other.UsrDat.Roles.InCurrentCrs == Rol_STD)
	       Gbl.Usrs.Other.UsrDat.Accepted = Enr_CheckIfUsrHasAcceptedInCurrentCrs (&Gbl.Usrs.Other.UsrDat);

	 if (ID_ICanSeeOtherUsrIDs (&Gbl.Usrs.Other.UsrDat))
	    ICanConfirm = true;
        }

   if (ICanConfirm)
     {
      /***** Get user's ID from form *****/
      Par_GetParToText ("UsrID",UsrID,ID_MAX_BYTES_USR_ID);
      // Users' IDs are always stored internally in capitals and without leading zeros
      Str_RemoveLeadingZeros (UsrID);
      Str_ConvertToUpperText (UsrID);

      for (NumID = 0, Found = false;
	   NumID < Gbl.Usrs.Other.UsrDat.IDs.Num && !Found;
	   NumID++)
	 if (!strcasecmp (UsrID,Gbl.Usrs.Other.UsrDat.IDs.List[NumID].ID))
	   {
	    Found = true;
	    NumIDFound = NumID;
	   }

      if (Found)	// Found
	{
	 if (Gbl.Usrs.Other.UsrDat.IDs.List[NumIDFound].Confirmed)
	    /***** ID found and already confirmed *****/
            Ale_CreateAlert (Ale_INFO,ID_ID_SECTION_ID,
        	             Txt_ID_X_had_already_been_confirmed,
		             Gbl.Usrs.Other.UsrDat.IDs.List[NumIDFound].ID);
	 else
	   {
	    /***** Mark this ID as confirmed *****/
	    ID_DB_ConfirmUsrID (Gbl.Usrs.Other.UsrDat.UsrCod,
			        Gbl.Usrs.Other.UsrDat.IDs.List[NumIDFound].ID);
	    Gbl.Usrs.Other.UsrDat.IDs.List[NumIDFound].Confirmed = true;

	    /***** Write success message *****/
	    Ale_CreateAlert (Ale_SUCCESS,ID_ID_SECTION_ID,
		             Txt_The_ID_X_has_been_confirmed,
		             Gbl.Usrs.Other.UsrDat.IDs.List[NumIDFound].ID);
	   }
	}
      else	// User's ID not found
	 Ale_CreateAlertUserNotFoundOrYouDoNotHavePermission ();
     }
   else	// I can not confirm
      Ale_CreateAlertUserNotFoundOrYouDoNotHavePermission ();

   /***** Show one or multiple records *****/
   switch (Gbl.Action.Original)
     {
      case ActSeeRecSevGst:
	 /* Show multiple records of guests again (including the updated one) */
	 Rec_ListRecordsGstsShow ();
	 break;
      case ActSeeRecSevStd:
	 /* Show multiple records of students again (including the updated one) */
	 Rec_ListRecordsStdsShow ();
	 break;
      case ActSeeRecSevTch:
	 /* Show multiple records of teachers again (including the updated one) */
	 Rec_ListRecordsTchsShow ();
	 break;
      default:
	 /* Show optional alert */
	 Ale_ShowAlerts (NULL);

	 /* Show only the updated record of this user */
	 Rec_ShowSharedUsrRecord (Rec_SHA_RECORD_LIST,
				  &Gbl.Usrs.Other.UsrDat,NULL);
	 break;
     }
  }
