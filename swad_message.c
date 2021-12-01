// swad_message.c: messages between users

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
#include <linux/limits.h>	// For PATH_MAX
#include <stddef.h>		// For NULL
#include <stdio.h>		// For asprintf
#include <stdlib.h>		// For free
#include <string.h>		// For string functions
#include <time.h>		// For time

#include "swad_action.h"
#include "swad_autolink.h"
#include "swad_box.h"
#include "swad_config.h"
#include "swad_course.h"
#include "swad_database.h"
#include "swad_error.h"
#include "swad_figure.h"
#include "swad_form.h"
#include "swad_forum.h"
#include "swad_global.h"
#include "swad_group.h"
#include "swad_hierarchy.h"
#include "swad_hierarchy_level.h"
#include "swad_HTML.h"
#include "swad_ID.h"
#include "swad_message.h"
#include "swad_message_database.h"
#include "swad_nickname_database.h"
#include "swad_notification.h"
#include "swad_notification_database.h"
#include "swad_pagination.h"
#include "swad_parameter.h"
#include "swad_photo.h"
#include "swad_profile.h"
#include "swad_profile_database.h"
#include "swad_session_database.h"
#include "swad_setting.h"
#include "swad_user.h"

/*****************************************************************************/
/************** External global variables from others modules ****************/
/*****************************************************************************/

extern struct Globals Gbl;

/*****************************************************************************/
/***************************** Private constants *****************************/
/*****************************************************************************/

// Forum images will be saved with:
// - maximum width of Msg_IMAGE_SAVED_MAX_HEIGHT
// - maximum height of Msg_IMAGE_SAVED_MAX_HEIGHT
// - maintaining the original aspect ratio (aspect ratio recommended: 3:2)
#define Msg_IMAGE_SAVED_MAX_WIDTH	768
#define Msg_IMAGE_SAVED_MAX_HEIGHT	768
#define Msg_IMAGE_SAVED_QUALITY		 90	// 1 to 100

static const Pag_WhatPaginate_t Msg_WhatPaginate[Msg_NUM_TYPES_OF_MSGS] =
  {
   [Msg_WRITING ] = Pag_NONE,
   [Msg_RECEIVED] = Pag_MESSAGES_RECEIVED,
   [Msg_SENT    ] = Pag_MESSAGES_SENT,
  };

/*****************************************************************************/
/***************************** Private prototypes ****************************/
/*****************************************************************************/

static void Msg_ResetMessages (struct Msg_Messages *Messages);

static void Msg_PutFormMsgUsrs (struct Msg_Messages *Messages,
                                char Content[Cns_MAX_BYTES_LONG_TEXT + 1]);

static void Msg_ShowSntOrRcvMessages (struct Msg_Messages *Messages);
static void Msg_PutLinkToViewBannedUsers(void);

static void Msg_SetNumMsgsStr (const struct Msg_Messages *Messages,
                               char **NumMsgsStr,unsigned NumUnreadMsgs);

static void Msg_PutIconsListMsgs (void *Messages);
static void Msg_PutHiddenParamsOneMsg (void *Messages);

static void Msg_ShowFormSelectCourseSentOrRecMsgs (const struct Msg_Messages *Messages);
static void Msg_ShowFormToFilterMsgs (const struct Msg_Messages *Messages);

static void Msg_ShowFormToShowOnlyUnreadMessages (const struct Msg_Messages *Messages);
static bool Msg_GetParamOnlyUnreadMsgs (void);
static void Msg_ShowASentOrReceivedMessage (struct Msg_Messages *Messages,
                                            long MsgNum,long MsgCod);
static long Msg_GetParamMsgCod (void);
static void Msg_PutLinkToShowMorePotentialRecipients (const struct Msg_Messages *Messages);
static void Msg_PutParamsShowMorePotentialRecipients (const void *Messages);
static void Msg_PutParamsWriteMsg (void *Messages);
static void Msg_PutHiddenParamsSubjectAndContent (void);
static void Msg_ShowOneUniqueRecipient (void);
static void Msg_WriteFormUsrsIDsOrNicksOtherRecipients (void);
static void Msg_WriteFormSubjectAndContentMsgToUsrs (struct Msg_Messages *Messages,
                                                     char Content[Cns_MAX_BYTES_LONG_TEXT + 1]);

static void Msg_PutHiddenParamAnotherRecipient (const struct UsrData *UsrDat);
static void Msg_PutHiddenParamOtherRecipients (void);

static void Msg_ShowNumMsgsDeleted (unsigned NumMsgs);

static void Msg_GetParamMsgsCrsCod (struct Msg_Messages *Messages);
static void Msg_GetParamFilterFromTo (struct Msg_Messages *Messages);
static void Msg_GetParamFilterContent (struct Msg_Messages *Messages);

static long Msg_InsertNewMsg (const char *Subject,const char *Content,
                              struct Med_Media *Media);

static unsigned long Msg_RemoveSomeRecOrSntMsgsUsr (const struct Msg_Messages *Messages,
                                                    long UsrCod,
                                                    const char *FilterFromToSubquery);
static void Msg_MoveRcvMsgToDeleted (long MsgCod,long UsrCod);
static void Msg_MoveSntMsgToDeleted (long MsgCod);

static void Msg_GetMsgSntData (long MsgCod,long *CrsCod,long *UsrCod,
                               time_t *CreatTimeUTC,
                               char Subject[Cns_MAX_BYTES_SUBJECT + 1],
                               bool *Deleted);
static void Msg_GetMsgContent (long MsgCod,
                               char Content[Cns_MAX_BYTES_LONG_TEXT + 1],
                               struct Med_Media *Media);

static void Msg_WriteSentOrReceivedMsgSubject (struct Msg_Messages *Messages,
					       long MsgCod,const char *Subject,
                                               bool Open,bool Expanded);

static bool Msg_WriteCrsOrgMsg (long CrsCod);

static void Msg_WriteFormToReply (long MsgCod,long CrsCod,
                                  bool ThisCrs,bool Replied,
                                  const struct UsrData *UsrDat);
static void Msg_WriteMsgFrom (struct Msg_Messages *Messages,
                              struct UsrData *UsrDat,bool Deleted);
static void Msg_WriteMsgTo (struct Msg_Messages *Messages,long MsgCod);

static void Msg_PutFormToBanSender (struct Msg_Messages *Messages,
                                    struct UsrData *UsrDat);
static void Msg_PutFormToUnbanSender (struct Msg_Messages *Messages,
                                      struct UsrData *UsrDat);
static void Msg_UnbanSender (void);

/*****************************************************************************/
/**************************** Reset messages context *************************/
/*****************************************************************************/

static void Msg_ResetMessages (struct Msg_Messages *Messages)
  {
   Messages->NumMsgs              = 0;
   Messages->Subject[0]           = '\0';
   Messages->FilterCrsCod         = -1L;
   Messages->FilterCrsShrtName[0] = '\0';
   Messages->FilterFromTo[0]      = '\0';
   Messages->FilterContent[0]     = '\0';
   Messages->ShowOnlyUnreadMsgs   = false;
   Messages->ExpandedMsgCod       = -1L;
   Messages->Reply.IsReply        = false;
   Messages->Reply.OriginalMsgCod = -1L;
   Messages->ShowOnlyOneRecipient = false;
   Messages->CurrentPage          = 0;
   Messages->MsgCod               = -1L;
  }

/*****************************************************************************/
/***************** Put a form to write a new message to users ****************/
/*****************************************************************************/

void Msg_FormMsgUsrs (void)
  {
   struct Msg_Messages Messages;
   char Content[Cns_MAX_BYTES_LONG_TEXT + 1];

   /***** Reset messages context *****/
   Msg_ResetMessages (&Messages);

   /***** Get possible hidden subject and content of the message *****/
   Par_GetParToHTML ("HiddenSubject",Messages.Subject,Cns_MAX_BYTES_SUBJECT);
   Par_GetParAndChangeFormat ("HiddenContent",Content,Cns_MAX_BYTES_LONG_TEXT,
                              Str_TO_TEXT,false);

   /***** Show a form to compose a message to users *****/
   Msg_PutFormMsgUsrs (&Messages,Content);
  }

/*****************************************************************************/
/***************** Put a form to write a new message to users ****************/
/*****************************************************************************/

static void Msg_PutFormMsgUsrs (struct Msg_Messages *Messages,
                                char Content[Cns_MAX_BYTES_LONG_TEXT + 1])
  {
   extern const char *Hlp_COMMUNICATION_Messages_write;
   extern const char *The_ClassFormInBox[The_NUM_THEMES];
   extern const char *Txt_Reply_message;
   extern const char *Txt_New_message;
   extern const char *Txt_MSG_To;
   extern const char *Txt_Send_message;
   unsigned NumUsrsInCrs = 0;	// Initialized to avoid warning
   bool ShowUsrsInCrs = false;
   bool GetUsrsInCrs;

   Gbl.Usrs.LstUsrs[Rol_STD].NumUsrs =
   Gbl.Usrs.LstUsrs[Rol_NET].NumUsrs =
   Gbl.Usrs.LstUsrs[Rol_TCH].NumUsrs = 0;

   /***** Get parameter that indicates if the message is a reply to another message *****/
   if ((Messages->Reply.IsReply = Par_GetParToBool ("IsReply")))
      /* Get original message code */
      if ((Messages->Reply.OriginalMsgCod = Msg_GetParamMsgCod ()) <= 0)
         Err_WrongMessageExit ();

   /***** Get user's code of possible preselected recipient *****/
   if (Usr_GetParamOtherUsrCodEncryptedAndGetUsrData ())	// There is a preselected recipient
      /* Get who to show as potential recipients:
         - only the selected recipient
         - any user (default) */
      Messages->ShowOnlyOneRecipient = Par_GetParToBool ("ShowOnlyOneRecipient");
   else
      Messages->ShowOnlyOneRecipient = false;

   GetUsrsInCrs = !Messages->ShowOnlyOneRecipient &&	// Show list of potential recipients
	          (Gbl.Usrs.Me.IBelongToCurrentCrs ||	// If there is a course selected and I belong to it
	           Gbl.Usrs.Me.Role.Logged == Rol_SYS_ADM);
   if (GetUsrsInCrs)
     {
      /***** Get and update type of list,
	     number of columns in class photo
	     and preference about view photos *****/
      Set_GetAndUpdatePrefsAboutUsrList ();

      /***** Get groups to show ******/
      Grp_GetParCodsSeveralGrpsToShowUsrs ();

      /***** Get and order lists of users from this course *****/
      Usr_GetListUsrs (HieLvl_CRS,Rol_STD);
      Usr_GetListUsrs (HieLvl_CRS,Rol_NET);
      Usr_GetListUsrs (HieLvl_CRS,Rol_TCH);
      NumUsrsInCrs = Gbl.Usrs.LstUsrs[Rol_STD].NumUsrs +	// Students
	             Gbl.Usrs.LstUsrs[Rol_NET].NumUsrs +	// Non-editing teachers
		     Gbl.Usrs.LstUsrs[Rol_TCH].NumUsrs;		// Teachers
     }

   /***** Begin box *****/
   Box_BoxBegin (NULL,Messages->Reply.IsReply ? Txt_Reply_message :
					        Txt_New_message,
                 Msg_PutIconsListMsgs,Messages,
		 Hlp_COMMUNICATION_Messages_write,Box_NOT_CLOSABLE);

      if (Messages->ShowOnlyOneRecipient)
	 /***** Form to show several potential recipients *****/
	 Msg_PutLinkToShowMorePotentialRecipients (Messages);
      else
	{
	 /***** Get list of users belonging to the current course *****/
	 if (GetUsrsInCrs)
	   {
	    /***** Form to select groups *****/
	    Grp_ShowFormToSelectSeveralGroups (Msg_PutParamsWriteMsg,Messages,
					       Grp_MY_GROUPS);

	    /***** Begin section with user list *****/
	    HTM_SECTION_Begin (Usr_USER_LIST_SECTION_ID);

	       if (NumUsrsInCrs)
		 {
		  /***** Form to select type of list used for select several users *****/
		  Usr_ShowFormsToSelectUsrListType (Msg_PutParamsWriteMsg,Messages);

		  /***** Put link to register students *****/
		  Enr_CheckStdsAndPutButtonToRegisterStdsInCurrentCrs ();

		  /***** Check if it's a big list *****/
		  ShowUsrsInCrs = Usr_GetIfShowBigList (NumUsrsInCrs,
							Msg_PutParamsWriteMsg,Messages,
							"CopyMessageToHiddenFields();");

		  if (ShowUsrsInCrs)
		     /***** Get lists of selected users *****/
		     Usr_GetListsSelectedEncryptedUsrsCods (&Gbl.Usrs.Selected);
		 }

	    /***** End section with user list *****/
	    HTM_SECTION_End ();
	   }

	 /***** Get list of users' IDs or nicknames written explicitely *****/
	 Usr_GetListMsgRecipientsWrittenExplicitelyBySender (false);
	}

      /***** Begin form to select recipients and write the message *****/
      Frm_BeginForm (ActRcvMsgUsr);
	 if (Messages->Reply.IsReply)
	   {
	    Par_PutHiddenParamChar ("IsReply",'Y');
	    Msg_PutHiddenParamMsgCod (Messages->Reply.OriginalMsgCod);
	   }
	 if (Gbl.Usrs.Other.UsrDat.UsrCod > 0)
	   {
	    Usr_PutParamOtherUsrCodEncrypted (Gbl.Usrs.Other.UsrDat.EnUsrCod);
	    if (Messages->ShowOnlyOneRecipient)
	       Par_PutHiddenParamChar ("ShowOnlyOneRecipient",'Y');
	   }

	 /***** Begin table *****/
	 HTM_TABLE_BeginCenterPadding (2);

	    /***** "To:" section (recipients) *****/
	    HTM_TR_Begin (NULL);

	       HTM_TD_Begin ("class=\"%s RT\"",The_ClassFormInBox[Gbl.Prefs.Theme]);
		  HTM_TxtColon (Txt_MSG_To);
	       HTM_TD_End ();

	       HTM_TD_Begin ("class=\"LT\"");
		  if (Messages->ShowOnlyOneRecipient)
		     /***** Show only one user as recipient *****/
		     Msg_ShowOneUniqueRecipient ();
		  else
		    {
		     /***** Show potential recipients *****/
		     HTM_TABLE_BeginWide ();
			if (ShowUsrsInCrs)
			  {
			   Usr_ListUsersToSelect (Rol_TCH,&Gbl.Usrs.Selected);	// All teachers in course
			   Usr_ListUsersToSelect (Rol_NET,&Gbl.Usrs.Selected);	// All non-editing teachers in course
			   Usr_ListUsersToSelect (Rol_STD,&Gbl.Usrs.Selected);	// All students in selected groups
			  }
			Msg_WriteFormUsrsIDsOrNicksOtherRecipients ();	// Other users (nicknames)
		     HTM_TABLE_End ();
		    }
	       HTM_TD_End ();

	    HTM_TR_End ();

	    /***** Subject and content sections *****/
	    Msg_WriteFormSubjectAndContentMsgToUsrs (Messages,Content);

	 /***** End table *****/
	 HTM_TABLE_End ();

	 /***** Help for text editor and send button *****/
	 Lay_HelpPlainEditor ();

	 /***** Attached image (optional) *****/
	 Med_PutMediaUploader (-1,"MSG_MED_INPUT");

	 /***** Send button *****/
	 Btn_PutCreateButton (Txt_Send_message);

      /***** End form *****/
      Frm_EndForm ();

   /***** End box *****/
   Box_BoxEnd ();

   /***** Free memory used by the list of nicknames *****/
   Usr_FreeListOtherRecipients ();

   /***** Free memory used for by the lists of users *****/
   if (GetUsrsInCrs)
     {
      Usr_FreeUsrsList (Rol_TCH);
      Usr_FreeUsrsList (Rol_NET);
      Usr_FreeUsrsList (Rol_STD);
     }

   /***** Free memory used by list of selected users' codes *****/
   Usr_FreeListsSelectedEncryptedUsrsCods (&Gbl.Usrs.Selected);

   /***** Free memory for list of selected groups *****/
   Grp_FreeListCodSelectedGrps ();
  }

/*****************************************************************************/
/********** Put contextual link to show more potential recipients ************/
/*****************************************************************************/

static void Msg_PutLinkToShowMorePotentialRecipients (const struct Msg_Messages *Messages)
  {
   extern const char *Txt_Show_more_recipients;

   /***** Contextual menu *****/
   Mnu_ContextMenuBegin ();
      Lay_PutContextualLinkIconTextOnSubmit (ActReqMsgUsr,NULL,
					     Msg_PutParamsShowMorePotentialRecipients,Messages,
					     "users.svg",
					     Txt_Show_more_recipients,
					     "CopyMessageToHiddenFields();");	// Shor more potential recipients
   Mnu_ContextMenuEnd ();
  }

/*****************************************************************************/
/************ Put parameters to show more potential recipients ***************/
/*****************************************************************************/

static void Msg_PutParamsShowMorePotentialRecipients (const void *Messages)
  {
   if (((struct Msg_Messages *) Messages)->Reply.IsReply)
     {
      Par_PutHiddenParamChar ("IsReply",'Y');
      Msg_PutHiddenParamMsgCod (((struct Msg_Messages *) Messages)->Reply.OriginalMsgCod);
     }
   if (Gbl.Usrs.Other.UsrDat.UsrCod > 0)
      Usr_PutParamOtherUsrCodEncrypted (Gbl.Usrs.Other.UsrDat.EnUsrCod);

   /***** Hidden params to send subject and content *****/
   Msg_PutHiddenParamsSubjectAndContent ();
  }

/*****************************************************************************/
/***************** Put parameters when writing a message *********************/
/*****************************************************************************/

static void Msg_PutParamsWriteMsg (void *Messages)
  {
   if (Messages)
     {
      Usr_PutHiddenParSelectedUsrsCods (&Gbl.Usrs.Selected);
      Msg_PutHiddenParamOtherRecipients ();
      Msg_PutHiddenParamsSubjectAndContent ();
      if (((struct Msg_Messages *) Messages)->Reply.IsReply)
	{
	 Par_PutHiddenParamChar ("IsReply",'Y');
	 Msg_PutHiddenParamMsgCod (((struct Msg_Messages *) Messages)->Reply.OriginalMsgCod);
	}
      if (Gbl.Usrs.Other.UsrDat.UsrCod > 0)
	{
	 Usr_PutParamOtherUsrCodEncrypted (Gbl.Usrs.Other.UsrDat.EnUsrCod);
	 if (((struct Msg_Messages *) Messages)->ShowOnlyOneRecipient)
	    Par_PutHiddenParamChar ("ShowOnlyOneRecipient",'Y');
	}
     }
  }

/*****************************************************************************/
/********** Put hidden parameters with message subject and content ***********/
/*****************************************************************************/

static void Msg_PutHiddenParamsSubjectAndContent (void)
  {
   /***** Hidden params to send subject and content.
          When the user edit the subject or the content,
          they are copied here. *****/
   Par_PutHiddenParamString (NULL,"HiddenSubject","");
   Par_PutHiddenParamString (NULL,"HiddenContent","");
  }

/*****************************************************************************/
/************ Put parameters to show more potential recipients ***************/
/*****************************************************************************/

static void Msg_ShowOneUniqueRecipient (void)
  {
   static const char *ClassPhoto[Set_NUM_USR_PHOTOS] =
     {
      [Set_USR_PHOTO_CIRCLE   ] = "PHOTOC21x28",
      [Set_USR_PHOTO_ELLIPSE  ] = "PHOTOE21x28",
      [Set_USR_PHOTO_OVAL     ] = "PHOTOO21x28",
      [Set_USR_PHOTO_RECTANGLE] = "PHOTOR21x28",
     };

   /***** Show user's photo *****/
   Pho_ShowUsrPhotoIfAllowed (&Gbl.Usrs.Other.UsrDat,
                              ClassPhoto[Gbl.Prefs.UsrPhotos],Pho_ZOOM,
                              false);

   /****** Write user's IDs ******/
   HTM_DIV_Begin ("class=\"MSG_TO_ONE_RCP %s\"",
		  Gbl.Usrs.Other.UsrDat.Accepted ? "DAT_SMALL_NOBR_N" :
						   "DAT_SMALL_NOBR");
      ID_WriteUsrIDs (&Gbl.Usrs.Other.UsrDat,NULL);
   HTM_DIV_End ();

   /***** Write user's name *****/
   HTM_DIV_Begin ("class=\"MSG_TO_ONE_RCP %s\"",
		  Gbl.Usrs.Other.UsrDat.Accepted ? "DAT_SMALL_NOBR_N" :
						   "DAT_SMALL_NOBR");
      HTM_Txt (Gbl.Usrs.Other.UsrDat.FullName);
   HTM_DIV_End ();

   /***** Hidden parameter with user's nickname *****/
   Msg_PutHiddenParamAnotherRecipient (&Gbl.Usrs.Other.UsrDat);
  }

/*****************************************************************************/
/************** Nicknames of recipients of a message to users ****************/
/*****************************************************************************/

static void Msg_WriteFormUsrsIDsOrNicksOtherRecipients (void)
  {
   extern const char *Txt_Other_recipients;
   extern const char *Txt_Recipients;
   extern const char *Txt_nicks_emails_or_IDs_separated_by_commas;
   char Nickname[Nck_MAX_BYTES_NICK_WITHOUT_ARROBA + 1];
   unsigned ColSpan;
   bool StdsAndTchsWritten = Gbl.Hierarchy.Level == HieLvl_CRS &&		// Course selected
                             (Gbl.Usrs.Me.IBelongToCurrentCrs ||	// I belong to it
                              Gbl.Usrs.Me.Role.Logged == Rol_SYS_ADM);

   /***** How many columns? *****/
   if (StdsAndTchsWritten)
      ColSpan = Usr_GetColumnsForSelectUsrs ();
   else
      ColSpan = 1;

   /***** Title *****/
   HTM_TR_Begin (NULL);
      HTM_TH_Begin (1,ColSpan,"LM LIGHT_BLUE");
	 HTM_LABEL_Begin ("for=\"OtherRecipients\"");
	    HTM_TxtColon (StdsAndTchsWritten ? Txt_Other_recipients :
					       Txt_Recipients);
	 HTM_LABEL_End ();
      HTM_TH_End ();
   HTM_TR_End ();

   /***** Textarea with users' @nicknames, emails or IDs *****/
   HTM_TR_Begin (NULL);
      HTM_TD_Begin ("colspan=\"%u\" class=\"LM\"",ColSpan);
	 HTM_TEXTAREA_Begin ("id=\"OtherRecipients\" name=\"OtherRecipients\""
			     " class=\"MSG_RECIPIENTS\" rows=\"2\" placeholder=\"%s\"",
			     Txt_nicks_emails_or_IDs_separated_by_commas);
	    if (Gbl.Usrs.ListOtherRecipients[0])
	       HTM_Txt (Gbl.Usrs.ListOtherRecipients);
	    else if (Gbl.Usrs.Other.UsrDat.UsrCod > 0)	// If there is a recipient
							 // and there's no list of explicit recipients,
							 // write @nickname of original sender
	      {
	       Nck_DB_GetNicknameFromUsrCod (Gbl.Usrs.Other.UsrDat.UsrCod,Nickname);
	       if (Nickname[0])
		  HTM_TxtF ("@%s",Nickname);
	      }
	 HTM_TEXTAREA_End ();
      HTM_TD_End ();
   HTM_TR_End ();
  }

/*****************************************************************************/
/****** Write form fields with subject and content of a message to users *****/
/*****************************************************************************/

static void Msg_WriteFormSubjectAndContentMsgToUsrs (struct Msg_Messages *Messages,
                                                     char Content[Cns_MAX_BYTES_LONG_TEXT + 1])
  {
   extern const char *The_ClassFormInBox[The_NUM_THEMES];
   extern const char *Txt_MSG_Subject;
   extern const char *Txt_MSG_Content;
   extern const char *Txt_Original_message;
   MYSQL_RES *mysql_res;
   MYSQL_ROW row;
   long MsgCod;
   bool SubjectAndContentComeFromForm = (Messages->Subject[0] || Content[0]);

   /***** Get possible code (of original message if it's a reply) *****/
   MsgCod = Msg_GetParamMsgCod ();

   /***** Message subject *****/
   HTM_TR_Begin (NULL);

      /* Label */
      Frm_LabelColumn ("RT","MsgSubject",Txt_MSG_Subject);

      /* Data */
      HTM_TD_Begin ("class=\"LT\"");
      HTM_TEXTAREA_Begin ("id=\"MsgSubject\" name=\"Subject\""
			  " class=\"MSG_SUBJECT\" rows=\"2\"");

      /* If message is a reply ==> get original message */
      if (MsgCod > 0)	// It's a reply
	{
		  if (!SubjectAndContentComeFromForm)
		    {
		     /* Get subject and content of message from database */
		     if (Msg_DB_GetSubjectAndContent (&mysql_res,MsgCod) != 1)
			Err_WrongMessageExit ();

		     row = mysql_fetch_row (mysql_res);

		     /* Get subject (row[0]) and content (row[1]) */
		     Str_Copy (Messages->Subject,row[0],sizeof (Messages->Subject) - 1);
		     Str_Copy (Content          ,row[1],Cns_MAX_BYTES_LONG_TEXT);

		     /* Free structure that stores the query result */
		     DB_FreeMySQLResult (&mysql_res);
		    }

		  /* Write subject */
		  if (!SubjectAndContentComeFromForm)
		     HTM_Txt ("Re: ");
		  HTM_Txt (Messages->Subject);

	       HTM_TEXTAREA_End ();
	    HTM_TD_End ();
	 HTM_TR_End ();

	 /***** Message content *****/
	 HTM_TR_Begin (NULL);

	    /* Label */
	    Frm_LabelColumn ("RT","MsgContent",Txt_MSG_Content);

	    /* Data */
	    HTM_TD_Begin ("class=\"LT\"");
	       HTM_TEXTAREA_Begin ("id=\"MsgContent\" name=\"Content\""
				   " class=\"MSG_CONTENT\" rows=\"20\"");

		  /* Begin textarea with a '\n', that will be not visible in textarea.
		     When Content is "\nLorem ipsum" (a white line before "Lorem ipsum"),
		     if we don't put the initial '\n' ==> the form will be sent starting
		     by "Lorem", without the white line */
		  HTM_Txt ("\n");

		  if (!SubjectAndContentComeFromForm)
		     HTM_TxtF ("\n\n----- %s -----\n",Txt_Original_message);

		  Msg_WriteMsgContent (Content,false,true);
	       HTM_TEXTAREA_End ();
	    HTM_TD_End ();
      	}
      else	// It's not a reply
	{
		  /* End message subject */
		  HTM_Txt (Messages->Subject);

	       HTM_TEXTAREA_End ();
	    HTM_TD_End ();

	 HTM_TR_End ();

	 /***** Message content *****/
	 HTM_TR_Begin (NULL);

	    /* Label */
	    Frm_LabelColumn ("RT","MsgContent",Txt_MSG_Content);

	    /* Data */
	    HTM_TD_Begin ("class=\"LT\"");
	       HTM_TEXTAREA_Begin ("id=\"MsgContent\" name=\"Content\""
				   " class=\"MSG_CONTENT\" rows=\"20\"");

		  /* Begin textarea with a '\n', that will be not visible in textarea.
		     When Content is "\nLorem ipsum" (a white line before "Lorem ipsum"),
		     if we don't put the initial '\n' ==> the form will be sent starting
		     by "Lorem", without the white line */
		  HTM_TxtF ("\n%s",Content);
	       HTM_TEXTAREA_End ();
	    HTM_TD_End ();
	}

   HTM_TR_End ();
  }

/*****************************************************************************/
/********* Put hidden parameter for another recipient (one nickname) *********/
/*****************************************************************************/

static void Msg_PutHiddenParamAnotherRecipient (const struct UsrData *UsrDat)
  {
   char NickWithArr[Nck_MAX_BYTES_NICK_WITH_ARROBA + 1];

   snprintf (NickWithArr,sizeof (NickWithArr),"@%s",UsrDat->Nickname);
   Par_PutHiddenParamString (NULL,"OtherRecipients",NickWithArr);
  }

/*****************************************************************************/
/********* Put hidden parameter for another recipient (one nickname) *********/
/*****************************************************************************/

static void Msg_PutHiddenParamOtherRecipients (void)
  {
   if (Gbl.Usrs.ListOtherRecipients)
      if (Gbl.Usrs.ListOtherRecipients[0])
         Par_PutHiddenParamString (NULL,"OtherRecipients",Gbl.Usrs.ListOtherRecipients);
  }

/*****************************************************************************/
/********************** Receive a new message from a user ********************/
/*****************************************************************************/

void Msg_RecMsgFromUsr (void)
  {
   extern const char *Txt_You_can_not_send_a_message_to_so_many_recipients_;
   extern const char *Txt_You_must_select_one_ore_more_recipients;
   extern const char *Txt_message_not_sent_to_X;
   extern const char *Txt_message_sent_to_X_notified_by_email;
   extern const char *Txt_message_sent_to_X_not_notified_by_email;
   extern const char *Txt_Error_getting_data_from_a_recipient;
   extern const char *Txt_The_message_has_not_been_sent_to_any_recipient;
   extern const char *Txt_The_message_has_been_sent_to_1_recipient;
   extern const char *Txt_The_message_has_been_sent_to_X_recipients;
   extern const char *Txt_There_have_been_X_errors_in_sending_the_message;
   struct Msg_Messages Messages;
   bool IsReply;
   bool RecipientHasBannedMe;
   bool Replied = false;
   long OriginalMsgCod = -1L;	// Initialized to avoid warning
   const char *Ptr;
   unsigned NumRecipients;
   unsigned NumRecipientsToBeNotifiedByEMail = 0;
   struct UsrData UsrDstData;
   int NumErrors = 0;
   long NewMsgCod = -1L;	// Initiliazed to avoid warning
   bool MsgAlreadyInserted = false;
   bool CreateNotif;
   bool NotifyByEmail;
   char Content[Cns_MAX_BYTES_LONG_TEXT + 1];
   struct Med_Media Media;
   bool Error = false;

   /***** Reset messages context *****/
   Msg_ResetMessages (&Messages);

   /***** Get data from form *****/
   /* Get subject */
   Par_GetParToHTML ("Subject",Messages.Subject,Cns_MAX_BYTES_SUBJECT);

   /* Get body */
   Par_GetParAndChangeFormat ("Content",Content,Cns_MAX_BYTES_LONG_TEXT,
                              Str_DONT_CHANGE,false);

   /* Get parameter that indicates if the message is a reply to a previous message */
   if ((IsReply = Par_GetParToBool ("IsReply")))
      /* Get original message code */
      if ((OriginalMsgCod = Msg_GetParamMsgCod ()) <= 0)
         Err_WrongMessageExit ();

   /* Get user's code of possible preselected recipient */
   Usr_GetParamOtherUsrCodEncryptedAndGetListIDs ();

   /* Get lists of selected users */
   Usr_GetListsSelectedEncryptedUsrsCods (&Gbl.Usrs.Selected);

   /* Get list of users' IDs or nicknames written explicitely */
   Error = Usr_GetListMsgRecipientsWrittenExplicitelyBySender (true);

   /***** Check number of recipients *****/
   if ((NumRecipients = Usr_CountNumUsrsInListOfSelectedEncryptedUsrCods (&Gbl.Usrs.Selected)))
     {
      if (Gbl.Usrs.Me.Role.Logged == Rol_STD &&
          NumRecipients > Cfg_MAX_RECIPIENTS)
        {
         /* Write warning message */
         Ale_ShowAlert (Ale_WARNING,Txt_You_can_not_send_a_message_to_so_many_recipients_);
         Error = true;
        }
     }
   else	// No recipients selected
     {
      /* Write warning message */
      Ale_ShowAlert (Ale_WARNING,Txt_You_must_select_one_ore_more_recipients);
      Error = true;
     }

   /***** If error in list of recipients, show again the form used to write a message *****/
   if (Error)
     {
      /* Show the form again, with the subject and the message filled */
      Str_ChangeFormat (Str_FROM_FORM,Str_TO_TEXT,
                        Content,Cns_MAX_BYTES_LONG_TEXT,true);
      Msg_PutFormMsgUsrs (&Messages,Content);
      return;
     }

   /***** Initialize structure with user's data *****/
   Usr_UsrDataConstructor (&UsrDstData);

   /***** Initialize image *****/
   Med_MediaConstructor (&Media);

   /***** Get attached image (action, file and title) *****/
   Media.Width   = Msg_IMAGE_SAVED_MAX_WIDTH;
   Media.Height  = Msg_IMAGE_SAVED_MAX_HEIGHT;
   Media.Quality = Msg_IMAGE_SAVED_QUALITY;
   Med_GetMediaFromForm (-1L,-1L,-1,&Media,NULL,NULL);
   Ale_ShowAlerts (NULL);

   /***** Loop over the list Gbl.Usrs.Selected.List[Rol_UNK], that holds the list of the
	  recipients, creating a received message for each recipient *****/
   Str_ChangeFormat (Str_FROM_FORM,Str_TO_RIGOROUS_HTML,
                     Content,Cns_MAX_BYTES_LONG_TEXT,false);
   Ptr = Gbl.Usrs.Selected.List[Rol_UNK];
   NumRecipients = 0;
   while (*Ptr)
     {
      Par_GetNextStrUntilSeparParamMult (&Ptr,UsrDstData.EnUsrCod,
                                         Cry_BYTES_ENCRYPTED_STR_SHA256_BASE64);
      Usr_GetUsrCodFromEncryptedUsrCod (&UsrDstData);
      if (Usr_ChkUsrCodAndGetAllUsrDataFromUsrCod (&UsrDstData,	// Get recipient's data from database
                                                   Usr_DONT_GET_PREFS,
                                                   Usr_DONT_GET_ROLE_IN_CURRENT_CRS))
        {
         /***** Check if recipient has banned me *****/
         RecipientHasBannedMe = Msg_DB_CheckIfUsrIsBanned (Gbl.Usrs.Me.UsrDat.UsrCod,UsrDstData.UsrCod);

         if (RecipientHasBannedMe)
            /***** Show an alert indicating that the message has not been sent successfully *****/
            Ale_ShowAlert (Ale_WARNING,Txt_message_not_sent_to_X,
		           UsrDstData.FullName);
         else
           {
            /***** Create message *****/
            if (!MsgAlreadyInserted)
              {
               // The message is inserted only once in the table of messages sent
               NewMsgCod = Msg_InsertNewMsg (Messages.Subject,Content,&Media);
               MsgAlreadyInserted = true;
              }

            /***** If this recipient is the original sender of a message been replied, set Replied to true *****/
            Replied = (IsReply &&
        	       UsrDstData.UsrCod == Gbl.Usrs.Other.UsrDat.UsrCod);

            /***** This received message must be notified by email? *****/
            CreateNotif = (UsrDstData.NtfEvents.CreateNotif & (1 << Ntf_EVENT_MESSAGE));
            NotifyByEmail = CreateNotif &&
        	            (UsrDstData.UsrCod != Gbl.Usrs.Me.UsrDat.UsrCod) &&
                            (UsrDstData.NtfEvents.SendEmail & (1 << Ntf_EVENT_MESSAGE));

            /***** Create the received message for this recipient
                   and increment number of new messages received by this recipient *****/
            Msg_DB_CreateRcvMsg (NewMsgCod,UsrDstData.UsrCod,NotifyByEmail);

            /***** Create notification for this recipient.
                   If this recipient wants to receive notifications by -mail,
                   activate the sending of a notification *****/
            if (CreateNotif)
               Ntf_DB_StoreNotifyEventToUsr (Ntf_EVENT_MESSAGE,UsrDstData.UsrCod,NewMsgCod,
                                             (Ntf_Status_t) (NotifyByEmail ? Ntf_STATUS_BIT_EMAIL :
                                        	                             0),
                                             Gbl.Hierarchy.Ins.InsCod,
                                             Gbl.Hierarchy.Ctr.CtrCod,
                                             Gbl.Hierarchy.Deg.DegCod,
                                             Gbl.Hierarchy.Crs.CrsCod);

            /***** Show an alert indicating that the message has been sent successfully *****/
            Ale_ShowAlert (Ale_SUCCESS,NotifyByEmail ? Txt_message_sent_to_X_notified_by_email :
                                                       Txt_message_sent_to_X_not_notified_by_email,
                           UsrDstData.FullName);

            /***** Increment number of recipients *****/
            if (NotifyByEmail)
               NumRecipientsToBeNotifiedByEMail++;
            NumRecipients++;
           }
        }
      else
        {
         Ale_ShowAlert (Ale_ERROR,Txt_Error_getting_data_from_a_recipient);
         NumErrors++;
        }
     }

   /***** Free image *****/
   Med_MediaDestructor (&Media);

   /***** Free memory used for user's data *****/
   Usr_UsrDataDestructor (&UsrDstData);

   /***** Free memory *****/
   /* Free memory used for list of users */
   Usr_FreeListOtherRecipients ();
   Usr_FreeListsSelectedEncryptedUsrsCods (&Gbl.Usrs.Selected);

   /***** Update received message setting Replied field to true *****/
   if (Replied)
      Msg_DB_SetRcvMsgAsReplied (OriginalMsgCod);

   /***** Write final message *****/
   if (NumRecipients)
     {
      if (NumRecipients == 1)
         Ale_ShowAlert (Ale_SUCCESS,Txt_The_message_has_been_sent_to_1_recipient);
      else
         Ale_ShowAlert (Ale_SUCCESS,Txt_The_message_has_been_sent_to_X_recipients,
                        (unsigned) NumRecipients);
     }
   else
      Ale_ShowAlert (Ale_WARNING,Txt_The_message_has_not_been_sent_to_any_recipient);

   /***** Show alert about errors on sending message *****/
   if (NumErrors > 1)
      Ale_ShowAlert (Ale_ERROR,Txt_There_have_been_X_errors_in_sending_the_message,
                    (unsigned) NumErrors);
  }

/*****************************************************************************/
/***************** Request deletion of all received messages *****************/
/*****************************************************************************/

void Msg_ReqDelAllRecMsgs (void)
  {
   extern const char *Txt_Do_you_really_want_to_delete_the_unread_messages_received_from_USER_X_from_COURSE_Y_related_to_CONTENT_Z;
   extern const char *Txt_any_user;
   extern const char *Txt_Do_you_really_want_to_delete_all_messages_received_from_USER_X_from_COURSE_Y_related_to_CONTENT_Z;
   extern const char *Txt_Do_you_really_want_to_delete_the_unread_messages_received_from_USER_X_from_COURSE_Y;
   extern const char *Txt_Do_you_really_want_to_delete_all_messages_received_from_USER_X_from_COURSE_Y;
   extern const char *Txt_Delete_messages_received;
   struct Msg_Messages Messages;

   /***** Reset messages context *****/
   Msg_ResetMessages (&Messages);

   /***** Get parameters *****/
   Msg_GetParamMsgsCrsCod (&Messages);
   Msg_GetParamFilterFromTo (&Messages);
   Msg_GetParamFilterContent (&Messages);
   Messages.ShowOnlyUnreadMsgs = Msg_GetParamOnlyUnreadMsgs ();

   /***** Show question and button to remove messages received *****/
   /* Begin alert */
   if (Messages.FilterContent[0])
     {
      if (Messages.ShowOnlyUnreadMsgs)
         Ale_ShowAlertAndButton1 (Ale_QUESTION,Txt_Do_you_really_want_to_delete_the_unread_messages_received_from_USER_X_from_COURSE_Y_related_to_CONTENT_Z,
				  Messages.FilterFromTo[0] ? Messages.FilterFromTo :
							     Txt_any_user,
				  Messages.FilterCrsShrtName,Messages.FilterContent);
      else
         Ale_ShowAlertAndButton1 (Ale_QUESTION,Txt_Do_you_really_want_to_delete_all_messages_received_from_USER_X_from_COURSE_Y_related_to_CONTENT_Z,
				  Messages.FilterFromTo[0] ? Messages.FilterFromTo :
							     Txt_any_user,
				  Messages.FilterCrsShrtName,Messages.FilterContent);
     }
   else
     {
      if (Messages.ShowOnlyUnreadMsgs)
         Ale_ShowAlertAndButton1 (Ale_QUESTION,Txt_Do_you_really_want_to_delete_the_unread_messages_received_from_USER_X_from_COURSE_Y,
				  Messages.FilterFromTo[0] ? Messages.FilterFromTo :
							     Txt_any_user,
				  Messages.FilterCrsShrtName);
      else
         Ale_ShowAlertAndButton1 (Ale_QUESTION,Txt_Do_you_really_want_to_delete_all_messages_received_from_USER_X_from_COURSE_Y,
				  Messages.FilterFromTo[0] ? Messages.FilterFromTo :
							     Txt_any_user,
				  Messages.FilterCrsShrtName);
     }

   /* Show received messages again */
   Messages.TypeOfMessages = Msg_RECEIVED;
   Msg_ShowSntOrRcvMessages (&Messages);

   /* End alert */
   Ale_ShowAlertAndButton2 (ActDelAllRcvMsg,NULL,NULL,
                            Msg_PutHiddenParamsMsgsFilters,&Messages,
                            Btn_REMOVE_BUTTON,Txt_Delete_messages_received);
  }

/*****************************************************************************/
/******************* Request deletion of all sent messages *******************/
/*****************************************************************************/

void Msg_ReqDelAllSntMsgs (void)
  {
   extern const char *Txt_Do_you_really_want_to_delete_all_messages_sent_to_USER_X_from_COURSE_Y_related_to_CONTENT_Z;
   extern const char *Txt_any_user;
   extern const char *Txt_Do_you_really_want_to_delete_all_messages_sent_to_USER_X_from_COURSE_Y;
   extern const char *Txt_Delete_messages_sent;
   struct Msg_Messages Messages;

   /***** Reset messages context *****/
   Msg_ResetMessages (&Messages);

   /***** Get parameters *****/
   Msg_GetParamMsgsCrsCod (&Messages);
   Msg_GetParamFilterFromTo (&Messages);
   Msg_GetParamFilterContent (&Messages);

   /***** Show question and button to remove messages received *****/
   /* Begin alert */
   if (Messages.FilterContent[0])
      Ale_ShowAlertAndButton1 (Ale_QUESTION,Txt_Do_you_really_want_to_delete_all_messages_sent_to_USER_X_from_COURSE_Y_related_to_CONTENT_Z,
			       Messages.FilterFromTo[0] ? Messages.FilterFromTo :
							  Txt_any_user,
			       Messages.FilterCrsShrtName,Messages.FilterContent);
   else
      Ale_ShowAlertAndButton1 (Ale_QUESTION,Txt_Do_you_really_want_to_delete_all_messages_sent_to_USER_X_from_COURSE_Y,
			       Messages.FilterFromTo[0] ? Messages.FilterFromTo :
							  Txt_any_user,
			       Messages.FilterCrsShrtName);

   /* Show sent messages again */
   Messages.TypeOfMessages = Msg_SENT;
   Msg_ShowSntOrRcvMessages (&Messages);

   /* End alert */
   Ale_ShowAlertAndButton2 (ActDelAllSntMsg,NULL,NULL,
                            Msg_PutHiddenParamsMsgsFilters,&Messages,
                            Btn_REMOVE_BUTTON,Txt_Delete_messages_sent);
  }

/*****************************************************************************/
/*********************** Delete all received messages ************************/
/*****************************************************************************/

void Msg_DelAllRecMsgs (void)
  {
   struct Msg_Messages Messages;
   char FilterFromToSubquery[Msg_DB_MAX_BYTES_MESSAGES_QUERY + 1];
   unsigned long NumMsgs;

   /***** Reset messages context *****/
   Msg_ResetMessages (&Messages);

   /***** Get parameters *****/
   Msg_GetParamMsgsCrsCod (&Messages);
   Msg_GetParamFilterFromTo (&Messages);
   Msg_GetParamFilterContent (&Messages);
   Messages.ShowOnlyUnreadMsgs = Msg_GetParamOnlyUnreadMsgs ();
   Msg_DB_MakeFilterFromToSubquery (&Messages,FilterFromToSubquery);

   /***** Delete messages *****/
   Messages.TypeOfMessages = Msg_RECEIVED;
   NumMsgs = Msg_RemoveSomeRecOrSntMsgsUsr (&Messages,
                                            Gbl.Usrs.Me.UsrDat.UsrCod,
                                            FilterFromToSubquery);
   Msg_ShowNumMsgsDeleted (NumMsgs);
   Msg_ShowRecMsgs ();
  }

/*****************************************************************************/
/************************* Delete all sent messages **************************/
/*****************************************************************************/

void Msg_DelAllSntMsgs (void)
  {
   struct Msg_Messages Messages;
   char FilterFromToSubquery[Msg_DB_MAX_BYTES_MESSAGES_QUERY + 1];
   unsigned long NumMsgs;

   /***** Reset messages context *****/
   Msg_ResetMessages (&Messages);

   /***** Get parameters *****/
   Msg_GetParamMsgsCrsCod (&Messages);
   Msg_GetParamFilterFromTo (&Messages);
   Msg_GetParamFilterContent (&Messages);
   Msg_DB_MakeFilterFromToSubquery (&Messages,FilterFromToSubquery);

   /***** Delete messages *****/
   Messages.TypeOfMessages = Msg_SENT;
   NumMsgs = Msg_RemoveSomeRecOrSntMsgsUsr (&Messages,
                                            Gbl.Usrs.Me.UsrDat.UsrCod,
                                            FilterFromToSubquery);
   Msg_ShowNumMsgsDeleted (NumMsgs);
   Msg_ShowSntMsgs ();
  }

/*****************************************************************************/
/************* Write number of messages that have been deleted ***************/
/*****************************************************************************/

static void Msg_ShowNumMsgsDeleted (unsigned NumMsgs)
  {
   extern const char *Txt_One_message_has_been_deleted;
   extern const char *Txt_X_messages_have_been_deleted;

   if (NumMsgs == 1)
      Ale_ShowAlert (Ale_SUCCESS,Txt_One_message_has_been_deleted);
   else
      Ale_ShowAlert (Ale_SUCCESS,Txt_X_messages_have_been_deleted,
                     NumMsgs);
  }

/*****************************************************************************/
/**************** Get parameter with course origin of messages ***************/
/*****************************************************************************/

static void Msg_GetParamMsgsCrsCod (struct Msg_Messages *Messages)
  {
   extern const char *Txt_any_course;
   struct Crs_Course Crs;

   if ((Messages->FilterCrsCod = Par_GetParToLong ("FilterCrsCod")) > 0)	// If origin course specified
     {
      /* Get data of course */
      Crs.CrsCod = Messages->FilterCrsCod;
      Crs_GetDataOfCourseByCod (&Crs);

      Str_Copy (Messages->FilterCrsShrtName,Crs.ShrtName,
                sizeof (Messages->FilterCrsShrtName) - 1);
     }
   else
      Str_Copy (Messages->FilterCrsShrtName,Txt_any_course,
                sizeof (Messages->FilterCrsShrtName) - 1);
  }

/*****************************************************************************/
/******************* Get parameter with "from"/"to" filter ********************/
/*****************************************************************************/

static void Msg_GetParamFilterFromTo (struct Msg_Messages *Messages)
  {
   /***** Get "from"/"to" filter *****/
   Par_GetParToText ("FilterFromTo",Messages->FilterFromTo,
                     Usr_MAX_BYTES_FULL_NAME);
  }

/*****************************************************************************/
/********************* Get parameter with content filter *********************/
/*****************************************************************************/

static void Msg_GetParamFilterContent (struct Msg_Messages *Messages)
  {
   /***** Get content filter *****/
   Par_GetParToText ("FilterContent",Messages->FilterContent,
                     Msg_MAX_BYTES_FILTER_CONTENT);
  }

/*****************************************************************************/
/****************************** Delete a sent message ************************/
/*****************************************************************************/

void Msg_DelSntMsg (void)
  {
   extern const char *Txt_Message_deleted;
   long MsgCod;

   /***** Get the code of the message to delete *****/
   if ((MsgCod = Msg_GetParamMsgCod ()) <= 0)
      Err_WrongMessageExit ();

   /***** Delete the message *****/
   /* Delete the sent message */
   Msg_MoveSntMsgToDeleted (MsgCod);
   Ale_ShowAlert (Ale_SUCCESS,Txt_Message_deleted);

   /* Show the remaining messages */
   Msg_ShowSntMsgs ();
  }

/*****************************************************************************/
/************************* Delete a received message *************************/
/*****************************************************************************/

void Msg_DelRecMsg (void)
  {
   extern const char *Txt_Message_deleted;
   long MsgCod;

   /***** Get the code of the message to delete *****/
   if ((MsgCod = Msg_GetParamMsgCod ()) <= 0)
      Err_WrongMessageExit ();

   /***** Delete the message *****/
   /* Delete the received message */
   Msg_MoveRcvMsgToDeleted (MsgCod,Gbl.Usrs.Me.UsrDat.UsrCod);
   Ale_ShowAlert (Ale_SUCCESS,Txt_Message_deleted);

   /* Show the remaining messages */
   Msg_ShowRecMsgs ();
  }

/*****************************************************************************/
/**************************** Expand a sent message **************************/
/*****************************************************************************/

void Msg_ExpSntMsg (void)
  {
   struct Msg_Messages Messages;

   /***** Reset messages context *****/
   Msg_ResetMessages (&Messages);

   /***** Get the code of the message to expand *****/
   if ((Messages.ExpandedMsgCod = Msg_GetParamMsgCod ()) <= 0)
      Err_WrongMessageExit ();

   /***** Expand the message *****/
   Msg_DB_ExpandSntMsg (Messages.ExpandedMsgCod);

   /***** Show again the messages *****/
   Msg_ShowSntMsgs ();
  }

/*****************************************************************************/
/************************** Expand a received message ************************/
/*****************************************************************************/

void Msg_ExpRecMsg (void)
  {
   struct Msg_Messages Messages;

   /***** Reset messages context *****/
   Msg_ResetMessages (&Messages);

   /***** Get the code of the message to expand *****/
   if ((Messages.ExpandedMsgCod = Msg_GetParamMsgCod ()) <= 0)
      Err_WrongMessageExit ();

   /***** Expand the message *****/
   Msg_DB_ExpandRcvMsg (Messages.ExpandedMsgCod);

   /***** Mark possible notification as seen *****/
   Ntf_DB_MarkNotifAsSeenUsingCod (Ntf_EVENT_MESSAGE,Messages.ExpandedMsgCod);

   /***** Show again the messages *****/
   Msg_ShowRecMsgs ();
  }

/*****************************************************************************/
/*************************** Contract a sent message *************************/
/*****************************************************************************/

void Msg_ConSntMsg (void)
  {
   long MsgCod;

   /***** Get the code of the message to contract *****/
   if ((MsgCod = Msg_GetParamMsgCod ()) <= 0)
      Err_WrongMessageExit ();

   /***** Contract the message *****/
   Msg_DB_ContractSntMsg (MsgCod);

   /***** Show again the messages *****/
   Msg_ShowSntMsgs ();
  }

/*****************************************************************************/
/************************* Contract a received message ***********************/
/*****************************************************************************/

void Msg_ConRecMsg (void)
  {
   long MsgCod;

   /***** Get the code of the message to contract *****/
   if ((MsgCod = Msg_GetParamMsgCod ()) <= 0)
      Err_WrongMessageExit ();

   /***** Contract the message *****/
   Msg_DB_ContractRcvMsg (MsgCod);

   /***** Show again the messages *****/
   Msg_ShowRecMsgs ();
  }

/*****************************************************************************/
/********************** Insert a message in the database *********************/
/*****************************************************************************/
// Return the code of the new inserted message

static long Msg_InsertNewMsg (const char *Subject,const char *Content,
                              struct Med_Media *Media)
  {
   long MsgCod;

   /***** Store media in filesystem and database *****/
   Med_RemoveKeepOrStoreMedia (-1L,Media);

   /***** Insert message subject and content in the database *****/
   MsgCod = Msg_DB_CreateNewMsg (Subject,Content,Media->MedCod);

   /***** Insert message in sent messages *****/
   Msg_DB_CreateSntMsg (MsgCod,Gbl.Hierarchy.Crs.CrsCod);

   /***** Increment number of messages sent by me *****/
   Prf_DB_IncrementNumMsgSntUsr (Gbl.Usrs.Me.UsrDat.UsrCod);

   return MsgCod;
  }

/*****************************************************************************/
/************** Delete some received or sent messages of a user **************/
/*****************************************************************************/

static unsigned long Msg_RemoveSomeRecOrSntMsgsUsr (const struct Msg_Messages *Messages,
                                                    long UsrCod,
                                                    const char *FilterFromToSubquery)
  {
   MYSQL_RES *mysql_res;
   unsigned NumMsgs;
   unsigned NumMsg;
   long MsgCod;

   /***** Get some of the messages received or sent by this user from database *****/
   NumMsgs = Msg_DB_GetSntOrRcvMsgs (&mysql_res,
                                     Messages,UsrCod,FilterFromToSubquery);

   /***** Delete each message *****/
   for (NumMsg = 0;
	NumMsg < NumMsgs;
	NumMsg++)
     {
      if ((MsgCod = DB_GetNextCode (mysql_res)) <= 0)
         Err_WrongMessageExit ();

      switch (Messages->TypeOfMessages)
        {
         case Msg_RECEIVED:
            Msg_MoveRcvMsgToDeleted (MsgCod,UsrCod);
            break;
         case Msg_SENT:
            Msg_MoveSntMsgToDeleted (MsgCod);
            break;
         default:
            break;
        }
     }

   /***** Free the MySQL result *****/
   DB_FreeMySQLResult (&mysql_res);

   return NumMsgs;
  }

/*****************************************************************************/
/************ Delete a message from the received message table ***************/
/*****************************************************************************/

static void Msg_MoveRcvMsgToDeleted (long MsgCod,long UsrCod)
  {
   /***** Move message from msg_rcv to msg_rcv_deleted *****/
   /* Insert message into msg_rcv_deleted */
   Msg_DB_CopyRcvMsgToDeleted (MsgCod,UsrCod);

   /* Delete message from msg_rcv *****/
   Msg_DB_RemoveRcvMsg (MsgCod,UsrCod);

   /***** If message content is not longer necessary, move it to msg_content_deleted *****/
   if (Msg_DB_CheckIfSntMsgIsDeleted (MsgCod))
      if (Msg_DB_CheckIfRcvMsgIsDeletedForAllItsRecipients (MsgCod))
         Msg_DB_MoveMsgContentToDeleted (MsgCod);

   /***** Mark possible notifications as removed *****/
   Ntf_DB_MarkNotifToOneUsrAsRemoved (Ntf_EVENT_MESSAGE,MsgCod,UsrCod);
  }

/*****************************************************************************/
/************** Delete a message from the sent message table *****************/
/*****************************************************************************/

static void Msg_MoveSntMsgToDeleted (long MsgCod)
  {
   /***** Move message from msg_snt to msg_snt_deleted *****/
   /* Insert message into msg_snt_deleted */
   Msg_DB_CopySntMsgToDeleted (MsgCod);

   /* Delete message from msg_snt *****/
   Msg_DB_RemoveSntMsg (MsgCod);

   /***** If message content is not longer necessary, move it to msg_content_deleted *****/
   if (Msg_DB_CheckIfRcvMsgIsDeletedForAllItsRecipients (MsgCod))
      Msg_DB_MoveMsgContentToDeleted (MsgCod);
  }

/*****************************************************************************/
/********************* Show messages sent to other users *********************/
/*****************************************************************************/

void Msg_ShowSntMsgs (void)
  {
   struct Msg_Messages Messages;

   /***** Reset messages context *****/
   Msg_ResetMessages (&Messages);

   /***** Show the sent messages *****/
   Messages.TypeOfMessages = Msg_SENT;
   Msg_ShowSntOrRcvMessages (&Messages);
  }

/*****************************************************************************/
/******************* Show messages received from other users *****************/
/*****************************************************************************/

void Msg_ShowRecMsgs (void)
  {
   struct Msg_Messages Messages;

   /***** Reset messages context *****/
   Msg_ResetMessages (&Messages);

   if (Msg_DB_GetNumUsrsBannedBy (Gbl.Usrs.Me.UsrDat.UsrCod))
     {
      /***** Contextual menu *****/
      Mnu_ContextMenuBegin ();
	 Msg_PutLinkToViewBannedUsers ();	// View banned users
      Mnu_ContextMenuEnd ();
     }

   /***** Show the received messages *****/
   Messages.TypeOfMessages = Msg_RECEIVED;
   Msg_ShowSntOrRcvMessages (&Messages);
  }

/*****************************************************************************/
/************************ Show sent or received messages *********************/
/*****************************************************************************/

static void Msg_ShowSntOrRcvMessages (struct Msg_Messages *Messages)
  {
   extern const char *Hlp_COMMUNICATION_Messages_received;
   extern const char *Hlp_COMMUNICATION_Messages_received_filter;
   extern const char *Hlp_COMMUNICATION_Messages_sent;
   extern const char *Hlp_COMMUNICATION_Messages_sent_filter;
   extern const char *The_ClassFormLinkInBoxBold[The_NUM_THEMES];
   extern const char *Txt_Filter;
   extern const char *Txt_Update_messages;
   char FilterFromToSubquery[Msg_DB_MAX_BYTES_MESSAGES_QUERY + 1];
   MYSQL_RES *mysql_res;
   MYSQL_ROW row;
   unsigned NumMsg;
   char *NumMsgsStr;
   unsigned NumUnreadMsgs;
   struct Pagination Pagination;
   long MsgCod;
   static const Act_Action_t ActionSee[Msg_NUM_TYPES_OF_MSGS] =
     {
      [Msg_WRITING ] = ActUnk,
      [Msg_RECEIVED] = ActSeeRcvMsg,
      [Msg_SENT    ] = ActSeeSntMsg,
     };
   static const Pag_WhatPaginate_t WhatPaginate[Msg_NUM_TYPES_OF_MSGS] =
     {
      [Msg_WRITING ] = Pag_NONE,
      [Msg_RECEIVED] = Pag_MESSAGES_RECEIVED,
      [Msg_SENT    ] = Pag_MESSAGES_SENT,
     };
   const char *Help[Msg_NUM_TYPES_OF_MSGS] =
     {
      [Msg_WRITING ] = NULL,
      [Msg_RECEIVED] = Hlp_COMMUNICATION_Messages_received,
      [Msg_SENT    ] = Hlp_COMMUNICATION_Messages_sent,
     };
   const char *HelpFilter[Msg_NUM_TYPES_OF_MSGS] =
     {
      [Msg_WRITING ] = NULL,
      [Msg_RECEIVED] = Hlp_COMMUNICATION_Messages_received_filter,
      [Msg_SENT    ] = Hlp_COMMUNICATION_Messages_sent_filter,
     };

   /***** Get the page number *****/
   Messages->CurrentPage = Pag_GetParamPagNum (WhatPaginate[Messages->TypeOfMessages]);

   /***** Get other parameters *****/
   Msg_GetParamMsgsCrsCod (Messages);
   Msg_GetParamFilterFromTo (Messages);
   Msg_GetParamFilterContent (Messages);
   Msg_DB_MakeFilterFromToSubquery (Messages,FilterFromToSubquery);

   /***** Get number of unread messages *****/
   switch (Messages->TypeOfMessages)
     {
      case Msg_RECEIVED:
         Messages->ShowOnlyUnreadMsgs = Msg_GetParamOnlyUnreadMsgs ();
         NumUnreadMsgs = Msg_DB_GetNumUnreadMsgs (Messages,
                                                  FilterFromToSubquery);
         break;
      case Msg_SENT:
      default:
         NumUnreadMsgs = 0;
         break;
     }

   /***** Get messages from database *****/
   Messages->NumMsgs = Msg_DB_GetSntOrRcvMsgs (&mysql_res,
                                               Messages,Gbl.Usrs.Me.UsrDat.UsrCod,
				               FilterFromToSubquery);

   /***** Begin box with messages *****/
   Msg_SetNumMsgsStr (Messages,&NumMsgsStr,NumUnreadMsgs);
   Box_BoxBegin ("97%",NumMsgsStr,
                 Msg_PutIconsListMsgs,Messages,
                 Help[Messages->TypeOfMessages],Box_NOT_CLOSABLE);
   free (NumMsgsStr);

      /***** Filter messages *****/
      /* Begin box with filter */
      Box_BoxBegin (NULL,Txt_Filter,
		    NULL,NULL,
		    HelpFilter[Messages->TypeOfMessages],Box_CLOSABLE);

	 /* Form to see messages again */
	 Frm_BeginForm (ActionSee[Messages->TypeOfMessages]);

	    HTM_DIV_Begin ("class=\"CM\"");
	       Msg_ShowFormSelectCourseSentOrRecMsgs (Messages);
	       if (Messages->TypeOfMessages == Msg_RECEIVED)
		  Msg_ShowFormToShowOnlyUnreadMessages (Messages);
	    HTM_DIV_End ();

	    Msg_ShowFormToFilterMsgs (Messages);

	    /***** Contextual menu *****/
	    Mnu_ContextMenuBegin ();
	       HTM_BUTTON_Animated_Begin (Txt_Update_messages,
					  The_ClassFormLinkInBoxBold[Gbl.Prefs.Theme],
					  NULL);
		  Ico_PutCalculateIconWithText (Txt_Update_messages);	// Animated icon to update messages
	       HTM_BUTTON_End ();
	    Mnu_ContextMenuEnd ();

	 Frm_EndForm ();

      /* End box */
      Box_BoxEnd ();

      if (Messages->NumMsgs)		// If there are messages...
	{
	 if (Gbl.Action.Act == ActExpRcvMsg)	// Expanding a message, perhaps it is the result of following a link
						   // from a notification of received message, so show the page where the message is inside
	   {
	    /***** Get the page where the expanded message is inside *****/
	    for (NumMsg = 0;
		 NumMsg < Messages->NumMsgs;
		 NumMsg++)
	      {
	       row = mysql_fetch_row (mysql_res);
	       if (sscanf (row[0],"%ld",&MsgCod) != 1)
		  Err_WrongMessageExit ();

	       if (MsgCod == Messages->ExpandedMsgCod)	// Expanded message found
		 {
		  Messages->CurrentPage = NumMsg / Pag_ITEMS_PER_PAGE + 1;
		  break;
		 }
	      }
	   }

	 /***** Compute variables related to pagination *****/
	 Pagination.NumItems = Messages->NumMsgs;
	 Pagination.CurrentPage = (int) Messages->CurrentPage;
	 Pag_CalculatePagination (&Pagination);
	 Messages->CurrentPage = (unsigned) Pagination.CurrentPage;

	 /***** Save my current page in order to show it next time I'll view my received/sent messages *****/
	 Ses_DB_SaveLastPageMsgIntoSession (WhatPaginate[Messages->TypeOfMessages],
					 Messages->CurrentPage);

	 /***** Write links to pages *****/
	 Pag_WriteLinksToPagesCentered (WhatPaginate[Messages->TypeOfMessages],&Pagination,
					Messages,-1L);

	 /***** Show received / sent messages in this page *****/
	 HTM_TABLE_BeginWidePadding (2);

	    mysql_data_seek (mysql_res,(my_ulonglong) (Pagination.FirstItemVisible - 1));
	    for (NumMsg  = Pagination.FirstItemVisible;
		 NumMsg <= Pagination.LastItemVisible;
		 NumMsg++)
	      {
	       row = mysql_fetch_row (mysql_res);

	       if (sscanf (row[0],"%ld",&MsgCod) != 1)
		  Err_WrongMessageExit ();
	       Msg_ShowASentOrReceivedMessage (Messages,
	                                       Messages->NumMsgs - NumMsg + 1,
	                                       MsgCod);
	      }

	 HTM_TABLE_End ();

	 /***** Write again links to pages *****/
	 Pag_WriteLinksToPagesCentered (WhatPaginate[Messages->TypeOfMessages],&Pagination,
					Messages,-1L);
	}

   /***** End box *****/
   Box_BoxEnd ();

   /***** Free structure that stores the query result *****/
   DB_FreeMySQLResult (&mysql_res);
  }

/*****************************************************************************/
/****************** Put a link (form) to view banned users *******************/
/*****************************************************************************/

static void Msg_PutLinkToViewBannedUsers(void)
  {
   extern const char *Txt_Banned_users;

   Lay_PutContextualLinkIconText (ActLstBanUsr,NULL,
                                  NULL,NULL,
				  "lock.svg",
				  Txt_Banned_users);
  }

/*****************************************************************************/
/***** Set string with number of messages and number of unread messages ******/
/*****************************************************************************/
// The string must be deallocated after calling this function

static void Msg_SetNumMsgsStr (const struct Msg_Messages *Messages,
                               char **NumMsgsStr,unsigned NumUnreadMsgs)
  {
   extern const char *Txt_message_received;
   extern const char *Txt_message_sent;
   extern const char *Txt_messages_received;
   extern const char *Txt_messages_sent;
   extern const char *Txt_unread_MESSAGE;
   extern const char *Txt_unread_MESSAGES;

   switch (Messages->TypeOfMessages)
     {
      case Msg_RECEIVED:
	 if (Messages->NumMsgs == 1)
	   {
	    if (NumUnreadMsgs)
	      {
	       if (asprintf (NumMsgsStr,"1 %s, 1 %s",
			     Txt_message_received,Txt_unread_MESSAGE) < 0)
                  Err_NotEnoughMemoryExit ();
	      }
	    else
	      {
	       if (asprintf (NumMsgsStr,"1 %s",Txt_message_received) < 0)
                  Err_NotEnoughMemoryExit ();
	      }
	   }
	 else
	   {
	    if (NumUnreadMsgs == 0)
	      {
	       if (asprintf (NumMsgsStr,"%u %s",
			     Messages->NumMsgs,Txt_messages_received) < 0)
                  Err_NotEnoughMemoryExit ();
	      }
	    else if (NumUnreadMsgs == 1)
	      {
	       if (asprintf (NumMsgsStr,"%u %s, 1 %s",
			     Messages->NumMsgs,Txt_messages_received,
			     Txt_unread_MESSAGE) < 0)
                  Err_NotEnoughMemoryExit ();
	      }
	    else
	      {
	       if (asprintf (NumMsgsStr,"%u %s, %u %s",
			     Messages->NumMsgs,Txt_messages_received,
			     NumUnreadMsgs,Txt_unread_MESSAGES) < 0)
                  Err_NotEnoughMemoryExit ();
	      }
	   }
	 break;
      case Msg_SENT:
	 if (Messages->NumMsgs == 1)
	   {
	    if (asprintf (NumMsgsStr,"1 %s",Txt_message_sent) < 0)
               Err_NotEnoughMemoryExit ();
	   }
	 else
	   {
	    if (asprintf (NumMsgsStr,"%u %s",
			  Messages->NumMsgs,Txt_messages_sent) < 0)
               Err_NotEnoughMemoryExit ();
	   }
	 break;
      default:
	 break;
     }
  }

/*****************************************************************************/
/***************** Put contextual icons in list of messages ******************/
/*****************************************************************************/

static void Msg_PutIconsListMsgs (void *Messages)
  {
   extern const char *Txt_MSGS_Sent;
   extern const char *Txt_MSGS_Received;
   extern const char *Txt_MSGS_Write;
   static const Act_Action_t ActionReqDelAllMsg[Msg_NUM_TYPES_OF_MSGS] =
     {
      [Msg_WRITING ] = ActUnk,
      [Msg_RECEIVED] = ActReqDelAllRcvMsg,
      [Msg_SENT    ] = ActReqDelAllSntMsg,
     };

   if (Messages)
     {
      /***** Put icon to write a new message *****/
      switch (((struct Msg_Messages *) Messages)->TypeOfMessages)
        {
	 case Msg_RECEIVED:
	 case Msg_SENT:
	    Lay_PutContextualLinkOnlyIcon (ActReqMsgUsr,NULL,
					   Msg_PutHiddenParamsMsgsFilters,Messages,
					   "marker.svg",
					   Txt_MSGS_Write);
	    break;
	 default:
	    break;
        }

      /***** Put icon to see received messages *****/
      switch (((struct Msg_Messages *) Messages)->TypeOfMessages)
        {
	 case Msg_WRITING:
	 case Msg_SENT:
	    Lay_PutContextualLinkOnlyIcon (ActSeeRcvMsg,NULL,
					   Msg_PutHiddenParamsMsgsFilters,Messages,
					   "inbox.svg",
					   Txt_MSGS_Received);
	    break;
	 default:
	    break;
        }

      /***** Put icon to see sent messages *****/
      switch (((struct Msg_Messages *) Messages)->TypeOfMessages)
        {
	 case Msg_WRITING:
	 case Msg_RECEIVED:
	    Lay_PutContextualLinkOnlyIcon (ActSeeSntMsg,NULL,
					   Msg_PutHiddenParamsMsgsFilters,Messages,
					   "share.svg",
					   Txt_MSGS_Sent);
	    break;
	 default:
	    break;
        }

      /***** Put icon to remove messages *****/
      switch (((struct Msg_Messages *) Messages)->TypeOfMessages)
        {
	 case Msg_RECEIVED:
	 case Msg_SENT:
	    Ico_PutContextualIconToRemove (ActionReqDelAllMsg[((struct Msg_Messages *) Messages)->TypeOfMessages],NULL,
					   Msg_PutHiddenParamsMsgsFilters,Messages);
	    break;
	 default:
	    break;
        }

      /***** Put icon to show a figure *****/
      Fig_PutIconToShowFigure (Fig_MESSAGES);
     }
  }

/*****************************************************************************/
/******* Put hidden parameters to expand, contract or delete a message *******/
/*****************************************************************************/

static void Msg_PutHiddenParamsOneMsg (void *Messages)
  {
   if (Messages)
     {
      Pag_PutHiddenParamPagNum (Msg_WhatPaginate[((struct Msg_Messages *) Messages)->TypeOfMessages],
				((struct Msg_Messages *) Messages)->CurrentPage);
      Msg_PutHiddenParamMsgCod (((struct Msg_Messages *) Messages)->MsgCod);
      Msg_PutHiddenParamsMsgsFilters (Messages);
     }
  }

/*****************************************************************************/
/****************** Put hidden parameters with filters ***********************/
/*****************************************************************************/

void Msg_PutHiddenParamsMsgsFilters (void *Messages)
  {
   if (Messages)
     {
      if (((struct Msg_Messages *) Messages)->FilterCrsCod >= 0)
	 Par_PutHiddenParamLong (NULL,"FilterCrsCod",((struct Msg_Messages *) Messages)->FilterCrsCod);
      if (((struct Msg_Messages *) Messages)->FilterFromTo[0])
	 Par_PutHiddenParamString (NULL,"FilterFromTo",((struct Msg_Messages *) Messages)->FilterFromTo);
      if (((struct Msg_Messages *) Messages)->FilterContent[0])
	 Par_PutHiddenParamString (NULL,"FilterContent",((struct Msg_Messages *) Messages)->FilterContent);
      if (((struct Msg_Messages *) Messages)->ShowOnlyUnreadMsgs)
	 Par_PutHiddenParamChar ("OnlyUnreadMsgs",'Y');
     }
  }

/*****************************************************************************/
/********* Show form to select course for sent or received messages **********/
/*****************************************************************************/

static void Msg_ShowFormSelectCourseSentOrRecMsgs (const struct Msg_Messages *Messages)
  {
   extern const char *The_ClassFormInBox[The_NUM_THEMES];
   extern const char *Txt_Messages_received_from_A_COURSE;
   extern const char *Txt_Messages_sent_from_A_COURSE;
   extern const char *Txt_any_course;
   static unsigned (*GetDistinctCrssInMyRcvMsgs[Msg_NUM_TYPES_OF_MSGS]) (MYSQL_RES **mysql_res) =
     {
      [Msg_WRITING ] = NULL,
      [Msg_RECEIVED] = Msg_DB_GetDistinctCrssInMyRcvMsgs,
      [Msg_SENT    ] = Msg_DB_GetDistinctCrssInMySntMsgs,
     };
   static const char **TxtSelector[Msg_NUM_TYPES_OF_MSGS] =
     {
      [Msg_WRITING ] = NULL,
      [Msg_RECEIVED] = &Txt_Messages_received_from_A_COURSE,
      [Msg_SENT    ] = &Txt_Messages_sent_from_A_COURSE,
     };
   MYSQL_RES *mysql_res;
   MYSQL_ROW row;
   unsigned NumCrss = 0;	// Initialized to avoid warning
   unsigned NumCrs;
   long CrsCod;

   /***** Get distinct courses in my messages *****/
   if (GetDistinctCrssInMyRcvMsgs[Messages->TypeOfMessages])
      NumCrss = GetDistinctCrssInMyRcvMsgs[Messages->TypeOfMessages] (&mysql_res);

   /***** Course selection *****/
   HTM_LABEL_Begin ("class=\"%s\"",The_ClassFormInBox[Gbl.Prefs.Theme]);
      HTM_TxtF ("%s&nbsp;",*TxtSelector[Messages->TypeOfMessages]);
      HTM_SELECT_Begin (HTM_DONT_SUBMIT_ON_CHANGE,
			"name=\"FilterCrsCod\"");

         /* Write a first option to select any course */
	 HTM_OPTION (HTM_Type_STRING,"",
		     Messages->FilterCrsCod < 0,false,
		     "%s",Txt_any_course);

	 /* Write an option for each origin course */
	 for (NumCrs = 0;
	      NumCrs < NumCrss;
	      NumCrs++)
	   {
	    /* Get next course */
	    row = mysql_fetch_row (mysql_res);

	    if ((CrsCod = Str_ConvertStrCodToLongCod (row[0])) > 0)
	       HTM_OPTION (HTM_Type_LONG,&CrsCod,
			   CrsCod == Messages->FilterCrsCod,false,
			   "%s",row[1]);	// Course short name
	   }

      HTM_SELECT_End ();
   HTM_LABEL_End ();

   /***** Free structure that stores the query result *****/
   DB_FreeMySQLResult (&mysql_res);
  }

/*****************************************************************************/
/***** Show form to filter "from" and "to" for received or sent messages *****/
/*****************************************************************************/

static void Msg_ShowFormToFilterMsgs (const struct Msg_Messages *Messages)
  {
   extern const char *The_ClassFormInBox[The_NUM_THEMES];
   extern const char *Txt_MSG_From;
   extern const char *Txt_MSG_To;
   extern const char *Txt_MSG_Content;
   static const char **TxtFromTo[Msg_NUM_TYPES_OF_MSGS] =
     {
      [Msg_WRITING ] = NULL,
      [Msg_RECEIVED] = &Txt_MSG_From,
      [Msg_SENT    ] = &Txt_MSG_To
     };

   /***** Begin table *****/
   HTM_TABLE_BeginCenterPadding (2);

      HTM_TR_Begin (NULL);

	 /***** Filter authors/recipients *****/
	 HTM_TD_Begin ("class=\"LM\"");
	    HTM_LABEL_Begin ("class=\"%s\"",The_ClassFormInBox[Gbl.Prefs.Theme]);
	       HTM_TxtColonNBSP (*TxtFromTo[Messages->TypeOfMessages]);
	       HTM_INPUT_SEARCH ("FilterFromTo",Usr_MAX_CHARS_FIRSTNAME_OR_SURNAME * 3,
				 Messages->FilterFromTo,
				 "size=\"20\"");
	    HTM_LABEL_End ();
	 HTM_TD_End ();

	 /***** Filter message content *****/
	 HTM_TD_Begin ("class=\"LM\"");
	    HTM_LABEL_Begin ("class=\"%s\"",The_ClassFormInBox[Gbl.Prefs.Theme]);
	       HTM_TxtColonNBSP (Txt_MSG_Content);
	       HTM_INPUT_SEARCH ("FilterContent",Msg_MAX_CHARS_FILTER_CONTENT,
				 Messages->FilterContent,
				 "size=\"20\"");
	    HTM_LABEL_End ();
	 HTM_TD_End ();

      HTM_TR_End ();

   /***** End table *****/
   HTM_TABLE_End ();
  }

/*****************************************************************************/
/**** Show form to select whether to show only unread (received) messages ****/
/*****************************************************************************/

static void Msg_ShowFormToShowOnlyUnreadMessages (const struct Msg_Messages *Messages)
  {
   extern const char *The_ClassFormInBox[The_NUM_THEMES];
   extern const char *Txt_only_unread_messages;

   /***** Put checkbox to select whether to show only unread (received) messages *****/
   HTM_LABEL_Begin ("class=\"%s\"",The_ClassFormInBox[Gbl.Prefs.Theme]);
      HTM_INPUT_CHECKBOX ("OnlyUnreadMsgs",HTM_DONT_SUBMIT_ON_CHANGE,
			  "value=\"Y\"%s",
			  Messages->ShowOnlyUnreadMsgs ? " checked=\"checked\"" :
							 "");
      HTM_Txt (Txt_only_unread_messages);
   HTM_LABEL_End ();
  }

/*****************************************************************************/
/*********** Get parameter to show only unread (received) messages ***********/
/*****************************************************************************/

static bool Msg_GetParamOnlyUnreadMsgs (void)
  {
   /***** Get parameter to show only unread (received) messages *****/
   return Par_GetParToBool ("OnlyUnreadMsgs");
  }

/*****************************************************************************/
/***************************** Get data of a message *************************/
/*****************************************************************************/

static void Msg_GetMsgSntData (long MsgCod,long *CrsCod,long *UsrCod,
                               time_t *CreatTimeUTC,
                               char Subject[Cns_MAX_BYTES_SUBJECT + 1],
                               bool *Deleted)
  {
   MYSQL_RES *mysql_res;
   MYSQL_ROW row;

   /***** Get data of message from table msg_snt *****/
   /* Result should have a unique row */
   if (Msg_DB_GetMsgSntData (&mysql_res,MsgCod,Deleted) != 1)
      Err_WrongMessageExit ();

   /* Get number of rows */
   row = mysql_fetch_row (mysql_res);

   /* Get location (row[0]) */
   *CrsCod = Str_ConvertStrCodToLongCod (row[0]);

   /* Get author code (row[1]) */
   *UsrCod = Str_ConvertStrCodToLongCod (row[1]);

   /* Get creation time (row[2]) */
   *CreatTimeUTC = Dat_GetUNIXTimeFromStr (row[2]);

   /* Free structure that stores the query result */
   DB_FreeMySQLResult (&mysql_res);

   /***** Get subject of message from database *****/
   Msg_DB_GetMsgSubject (MsgCod,Subject);
  }

/*****************************************************************************/
/*************** Get content and optional image of a message *****************/
/*****************************************************************************/

static void Msg_GetMsgContent (long MsgCod,
                               char Content[Cns_MAX_BYTES_LONG_TEXT + 1],
                               struct Med_Media *Media)
  {
   MYSQL_RES *mysql_res;
   MYSQL_ROW row;

   /***** Get content of message from database *****/
   if (Msg_DB_GetMsgContent (&mysql_res,MsgCod) != 1)
      Err_WrongMessageExit ();

   /***** Get number of rows *****/
   row = mysql_fetch_row (mysql_res);

   /****** Get content (row[0]) *****/
   Str_Copy (Content,row[0],Cns_MAX_BYTES_LONG_TEXT);

   /***** Get media (row[1]) *****/
   Media->MedCod = Str_ConvertStrCodToLongCod (row[1]);
   Med_GetMediaDataByCod (Media);

   /***** Free structure that stores the query result *****/
   DB_FreeMySQLResult (&mysql_res);
  }

/*****************************************************************************/
/******** Show a sent or a received message (from a user to another) *********/
/*****************************************************************************/

static void Msg_ShowASentOrReceivedMessage (struct Msg_Messages *Messages,
                                            long MsgNum,long MsgCod)
  {
   extern const char *Txt_MSG_Replied;
   extern const char *Txt_MSG_Not_replied;
   extern const char *Txt_MSG_Unopened;
   extern const char *Txt_MSG_Sent;
   extern const char *Txt_MSG_From;
   extern const char *Txt_MSG_To;
   extern const char *Txt_MSG_Content;
   static const Act_Action_t ActionDelMsg[Msg_NUM_TYPES_OF_MSGS] =
     {
      [Msg_WRITING ] = ActUnk,
      [Msg_RECEIVED] = ActDelRcvMsg,
      [Msg_SENT    ] = ActDelSntMsg,
     };
   struct UsrData UsrDat;
   const char *Title = NULL;	// Initialized to avoid warning
   bool FromThisCrs = false;	// Initialized to avoid warning
   time_t CreatTimeUTC;		// Creation time of a message
   long CrsCod;
   char Subject[Cns_MAX_BYTES_SUBJECT + 1];
   char Content[Cns_MAX_BYTES_LONG_TEXT + 1];
   struct Med_Media Media;
   bool Deleted;
   bool Open = true;
   bool Replied = false;	// Initialized to avoid warning
   bool Expanded = false;

   /***** Initialize structure with user's data *****/
   Usr_UsrDataConstructor (&UsrDat);

   /***** Get data of message *****/
   Msg_GetMsgSntData (MsgCod,&CrsCod,&UsrDat.UsrCod,&CreatTimeUTC,Subject,&Deleted);
   switch (Messages->TypeOfMessages)
     {
      case Msg_RECEIVED:
         Msg_DB_GetStatusOfRcvMsg (MsgCod,&Open,&Replied,&Expanded);
         break;
      case Msg_SENT:
         Expanded = Msg_DB_GetStatusOfSntMsg (MsgCod);
         break;
      default:
	 break;
     }

   /***** Put an icon with message status *****/
   switch (Messages->TypeOfMessages)
     {
      case Msg_RECEIVED:
         Title = (Open ? (Replied ? Txt_MSG_Replied :
                                    Txt_MSG_Not_replied) :
                         Txt_MSG_Unopened);
	 break;
      case Msg_SENT:
	 Title = Txt_MSG_Sent;
	 break;
      default:
	 break;
     }

   HTM_TR_Begin (NULL);

      HTM_TD_Begin ("class=\"CONTEXT_COL %s\"",
		    Messages->TypeOfMessages == Msg_RECEIVED ? (Open ? "BG_MSG_BLUE" :
								       "BG_MSG_GREEN") :
							       "BG_MSG_BLUE");
	 Ico_PutIcon (Messages->TypeOfMessages == Msg_RECEIVED ? (Open ? (Replied ? "reply.svg" :
										    "envelope-open-text.svg") :
									 "envelope.svg") :
								 "share.svg",
		      Title,"ICO16x16");

	 /***** Form to delete message *****/
	 HTM_BR ();
	 Messages->MsgCod = MsgCod;	// Message to be deleted
	 Ico_PutContextualIconToRemove (ActionDelMsg[Messages->TypeOfMessages],NULL,
					Msg_PutHiddenParamsOneMsg,Messages);
      HTM_TD_End ();

      /***** Write message number *****/
      Msg_WriteMsgNumber (MsgNum,!Open);

      /***** Write message author *****/
      HTM_TD_Begin ("class=\"%s LT\"",Open ? "MSG_AUT_BG" :
					     "MSG_AUT_BG_NEW");
	 Usr_ChkUsrCodAndGetAllUsrDataFromUsrCod (&UsrDat,
						  Usr_DONT_GET_PREFS,
						  Usr_DONT_GET_ROLE_IN_CURRENT_CRS);
	 Msg_WriteMsgAuthor (&UsrDat,true,NULL);
      HTM_TD_End ();

      /***** Write subject *****/
      Msg_WriteSentOrReceivedMsgSubject (Messages,MsgCod,Subject,Open,Expanded);

      /***** Write date-time *****/
      Msg_WriteMsgDate (CreatTimeUTC,Open ? "MSG_TIT_BG" :
					    "MSG_TIT_BG_NEW");

   HTM_TR_End ();

   if (Expanded)
     {
      HTM_TR_Begin (NULL);

	 HTM_TD_Begin ("rowspan=\"3\" colspan=\"2\" class=\"LT\"");
	    HTM_TABLE_BeginPadding (2);

	       /***** Write course origin of message *****/
	       HTM_TR_Begin (NULL);
		  HTM_TD_Begin ("class=\"LM\"");
		     FromThisCrs = Msg_WriteCrsOrgMsg (CrsCod);
		  HTM_TD_End ();
	       HTM_TR_End ();

	       /***** Form to reply message *****/
	       HTM_TR_Begin (NULL);
		  HTM_TD_Begin ("class=\"LM\"");
		     if (Messages->TypeOfMessages == Msg_RECEIVED &&
			 Gbl.Usrs.Me.Role.Logged >= Rol_USR)
			// Guests (users without courses) can read messages but not reply them
			Msg_WriteFormToReply (MsgCod,CrsCod,FromThisCrs,Replied,&UsrDat);
		  HTM_TD_End ();
	       HTM_TR_End ();

	    HTM_TABLE_End ();
	 HTM_TD_End ();

	 /***** Write "From:" *****/
	 HTM_TD_Begin ("class=\"RT MSG_TIT\"");
	    HTM_TxtColonNBSP (Txt_MSG_From);
	 HTM_TD_End ();

	 HTM_TD_Begin ("colspan=\"2\" class=\"LT\"");
	    Msg_WriteMsgFrom (Messages,&UsrDat,Deleted);
	 HTM_TD_End ();

      HTM_TR_End ();

      /***** Write "To:" *****/
      HTM_TR_Begin (NULL);

	 HTM_TD_Begin ("class=\"RT MSG_TIT\"");
	    HTM_TxtColonNBSP (Txt_MSG_To);
	 HTM_TD_End ();

	 HTM_TD_Begin ("colspan=\"2\" class=\"LT\"");
	    Msg_WriteMsgTo (Messages,MsgCod);
	 HTM_TD_End ();

      HTM_TR_End ();

      HTM_TR_Begin (NULL);

	 /***** Initialize media *****/
	 Med_MediaConstructor (&Media);

	 /***** Get message content and optional media *****/
	 Msg_GetMsgContent (MsgCod,Content,&Media);

	 /***** Write "Content:" *****/
	 HTM_TD_Begin ("class=\"RT MSG_TIT\"");
	    HTM_TxtColonNBSP (Txt_MSG_Content);
	 HTM_TD_End ();

	 /***** Show content and media *****/
	 HTM_TD_Begin ("colspan=\"2\" class=\"MSG_TXT LT\"");
	    if (Content[0])
	       Msg_WriteMsgContent (Content,true,false);
	    Med_ShowMedia (&Media,"MSG_IMG_CONT","MSG_IMG");
	 HTM_TD_End ();

      HTM_TR_End ();

      /***** Free media *****/
      Med_MediaDestructor (&Media);
     }

   /***** Free memory used for user's data *****/
   Usr_UsrDataDestructor (&UsrDat);
  }

/*****************************************************************************/
/******************** Get subject and content of a message *******************/
/*****************************************************************************/

void Msg_GetNotifMessage (char SummaryStr[Ntf_MAX_BYTES_SUMMARY + 1],
                          char **ContentStr,long MsgCod,bool GetContent)
  {
   MYSQL_RES *mysql_res;
   MYSQL_ROW row;
   size_t Length;

   SummaryStr[0] = '\0';	// Return nothing on error

   /***** Get subject of message from database *****/
   if (Msg_DB_GetSubjectAndContent (&mysql_res,MsgCod) == 1)	// Result should have a unique row
     {
      /***** Get subject and content of the message *****/
      row = mysql_fetch_row (mysql_res);

      /***** Copy subject *****/
      // TODO: Do only direct copy when Subject will be VARCHAR(255)
      if (strlen (row[0]) > Ntf_MAX_BYTES_SUMMARY)
	{
	 strncpy (SummaryStr,row[0],
		  Ntf_MAX_BYTES_SUMMARY);
	 SummaryStr[Ntf_MAX_BYTES_SUMMARY] = '\0';
	}
      else
	 Str_Copy (SummaryStr,row[0],Ntf_MAX_BYTES_SUMMARY);

      /***** Copy subject *****/
      if (GetContent)
	{
	 Length = strlen (row[1]);
	 if ((*ContentStr = malloc (Length + 1)) == NULL)
            Err_NotEnoughMemoryExit ();
	 Str_Copy (*ContentStr,row[1],Length);
	}
     }

   /***** Free structure that stores the query result *****/
   DB_FreeMySQLResult (&mysql_res);
  }

/*****************************************************************************/
/************************** Write number of message **************************/
/*****************************************************************************/

void Msg_WriteMsgNumber (unsigned long MsgNum,bool NewMsg)
  {
   HTM_TD_Begin ("class=\"%s CT\" style=\"width:45px;\"",
		 NewMsg ? "MSG_TIT_BG_NEW" :
			  "MSG_TIT_BG");
      HTM_TxtF ("%lu:",MsgNum);
   HTM_TD_End ();
  }

/*****************************************************************************/
/******************** Write subject of a received message ********************/
/*****************************************************************************/

static void Msg_WriteSentOrReceivedMsgSubject (struct Msg_Messages *Messages,
					       long MsgCod,const char *Subject,
                                               bool Open,bool Expanded)
  {
   extern const char *Txt_Hide_message;
   extern const char *Txt_See_message;
   extern const char *Txt_no_subject;

   /***** Begin cell *****/
   HTM_TD_Begin ("class=\"%s LT\"",Open ? "MSG_TIT_BG" :
        	                          "MSG_TIT_BG_NEW");

      /***** Begin form to expand/contract the message *****/
      Frm_BeginForm (Messages->TypeOfMessages == Msg_RECEIVED ? (Expanded ? ActConRcvMsg :
									    ActExpRcvMsg) :
							        (Expanded ? ActConSntMsg :
									    ActExpSntMsg));
	 Messages->MsgCod = MsgCod;	// Message to be contracted/expanded
	 Msg_PutHiddenParamsOneMsg (Messages);

	 HTM_BUTTON_SUBMIT_Begin (Expanded ? Txt_Hide_message :
					     Txt_See_message,
				  Open ? "BT_LINK LT MSG_TIT" :
					 "BT_LINK LT MSG_TIT_NEW",
				  NULL);

	    /***** Write subject *****/
	    if (Subject[0])
	       HTM_Txt (Subject);
	    else
	       HTM_TxtF ("[%s]",Txt_no_subject);

	 /***** End form to expand the message *****/
	 HTM_BUTTON_End ();

      Frm_EndForm ();

   /***** End cell *****/
   HTM_TD_End ();
  }

/*****************************************************************************/
/************************ Write the author of a message **********************/
/*****************************************************************************/
// Input: UsrDat must hold user's data

void Msg_WriteMsgAuthor (struct UsrData *UsrDat,bool Enabled,const char *BgColor)
  {
   extern const char *Txt_Unknown_or_without_photo;
   static const char *ClassPhoto[Set_NUM_USR_PHOTOS] =
     {
      [Set_USR_PHOTO_CIRCLE   ] = "PHOTOC30x40",
      [Set_USR_PHOTO_ELLIPSE  ] = "PHOTOE30x40",
      [Set_USR_PHOTO_OVAL     ] = "PHOTOO30x40",
      [Set_USR_PHOTO_RECTANGLE] = "PHOTOR30x40",
     };
   bool WriteAuthor;

   /***** Write author name or don't write it? *****/
   WriteAuthor = false;
   if (Enabled)
      if (UsrDat->UsrCod > 0)
         WriteAuthor = true;

   /***** Begin table and row *****/
   HTM_TABLE_BeginPadding (2);
      HTM_TR_Begin (NULL);

	 /***** Begin first column with author's photo
		(if author has a web page, put a link to it) *****/
	 if (BgColor)
	    HTM_TD_Begin ("class=\"CT %s\" style=\"width:30px;\"",BgColor);
	 else
	    HTM_TD_Begin ("class=\"CT\" style=\"width:30px;\"");

	 if (WriteAuthor)
	    Pho_ShowUsrPhotoIfAllowed (UsrDat,
	                               ClassPhoto[Gbl.Prefs.UsrPhotos],Pho_ZOOM,
	                               false);
	 else
	    Ico_PutIcon ("usr_bl.jpg",Txt_Unknown_or_without_photo,
	                 ClassPhoto[Gbl.Prefs.UsrPhotos]);

	 HTM_TD_End ();

	 /***** Second column with user name (if author has a web page, put a link to it) *****/
	 if (WriteAuthor)
	   {
	    if (BgColor)
	       HTM_TD_Begin ("class=\"LT %s\"",BgColor);
	    else
	       HTM_TD_Begin ("class=\"LT\"");

	    HTM_DIV_Begin ("class=\"AUTHOR_2_LINES\"");	// Limited width
	       Usr_WriteFirstNameBRSurnames (UsrDat);
	    HTM_DIV_End ();
	   }
	 else
	   {
	    if (BgColor)
	       HTM_TD_Begin ("class=\"LM %s\"",BgColor);
	    else
	       HTM_TD_Begin ("class=\"LM\"");
	   }

	 /***** End second column *****/
	 HTM_TD_End ();

      /***** End row and table *****/
      HTM_TR_End ();
   HTM_TABLE_End ();
  }

/*****************************************************************************/
/********************* Write course origin of a message **********************/
/*****************************************************************************/
// Returns true if the origin course is the current course

static bool Msg_WriteCrsOrgMsg (long CrsCod)
  {
   extern const char *Txt_from_this_course;
   extern const char *Txt_no_course_of_origin;
   struct Crs_Course Crs;
   bool FromThisCrs = true;
   bool ThereIsOrgCrs = false;

   if (CrsCod > 0)
     {
      /* Get new course code from old course code */
      Crs.CrsCod = CrsCod;

      /* Get data of current degree */
      if (Crs_GetDataOfCourseByCod (&Crs))
        {
         ThereIsOrgCrs = true;
         if ((FromThisCrs = (CrsCod == Gbl.Hierarchy.Crs.CrsCod)))	// Message sent from current course
           {
            HTM_DIV_Begin ("class=\"AUTHOR_TXT\"");
	       HTM_TxtF ("(%s)",Txt_from_this_course);
            HTM_DIV_End ();
           }
         else	// Message sent from another course
           {
            /* Write course, including link */
            Frm_BeginFormGoTo (ActSeeCrsInf);
	       Crs_PutParamCrsCod (Crs.CrsCod);
	       HTM_DIV_Begin ("class=\"AUTHOR_TXT\"");
		  HTM_Txt ("(");
		  HTM_BUTTON_SUBMIT_Begin (Hie_BuildGoToMsg (Crs.FullName),
					   "BT_LINK AUTHOR_TXT",NULL);
		  Hie_FreeGoToMsg ();
		     HTM_Txt (Crs.ShrtName);
		  HTM_BUTTON_End ();
		  HTM_Txt (")");
	       HTM_DIV_End ();
            Frm_EndForm ();
           }
	}
     }
   if (!ThereIsOrgCrs)	// It's an old message without origin source specified, or is a message sent from none course
     {
      HTM_DIV_Begin ("class=\"AUTHOR_TXT\"");
	 HTM_TxtF ("(%s)",Txt_no_course_of_origin);
      HTM_DIV_End ();
     }

   return FromThisCrs;
  }

/*****************************************************************************/
/************************* Write form to reply a message *********************/
/*****************************************************************************/

static void Msg_WriteFormToReply (long MsgCod,long CrsCod,
                                  bool FromThisCrs,bool Replied,
                                  const struct UsrData *UsrDat)
  {
   extern const char *Txt_Reply;
   extern const char *Txt_Reply_again;
   extern const char *Txt_Go_to_course_and_reply;
   extern const char *Txt_Go_to_course_and_reply_again;

   /***** Begin form and parameters *****/
   if (FromThisCrs)
      Frm_BeginForm (ActReqMsgUsr);
   else	// Not the current course ==> go to another course
     {
      Frm_BeginFormGoTo (ActReqMsgUsr);
	 Crs_PutParamCrsCod (CrsCod);
     }
      Grp_PutParamAllGroups ();
      Par_PutHiddenParamChar ("IsReply",'Y');
      Msg_PutHiddenParamMsgCod (MsgCod);
      Usr_PutParamUsrCodEncrypted (UsrDat->EnUsrCod);
      Par_PutHiddenParamChar ("ShowOnlyOneRecipient",'Y');

      /****** Link *****/
      Ico_PutIconLink ("reply.svg",
		       FromThisCrs ? (Replied ? Txt_Reply_again :
						Txt_Reply) :
				     (Replied ? Txt_Go_to_course_and_reply_again :
						Txt_Go_to_course_and_reply));

   /****** End form *****/
   Frm_EndForm ();
  }

/*****************************************************************************/
/************************** Write author of a message ************************/
/*****************************************************************************/

static void Msg_WriteMsgFrom (struct Msg_Messages *Messages,
                              struct UsrData *UsrDat,bool Deleted)
  {
   extern const char *Txt_MSG_Sent;
   extern const char *Txt_MSG_Sent_and_deleted;
   extern const char *Txt_ROLES_SINGUL_abc[Rol_NUM_ROLES][Usr_NUM_SEXS];
   static const char *ClassPhoto[Set_NUM_USR_PHOTOS] =
     {
      [Set_USR_PHOTO_CIRCLE   ] = "PHOTOC21x28",
      [Set_USR_PHOTO_ELLIPSE  ] = "PHOTOE21x28",
      [Set_USR_PHOTO_OVAL     ] = "PHOTOO21x28",
      [Set_USR_PHOTO_RECTANGLE] = "PHOTOR21x28",
     };

   HTM_TABLE_Begin (NULL);
      HTM_TR_Begin (NULL);

	 /***** Put an icon to show if user has read the message *****/
	 HTM_TD_Begin ("class=\"LM\" style=\"width:20px;\"");
	    Ico_PutIcon (Deleted ? "share-red.svg" :
				   "share.svg",
			 Deleted ? Txt_MSG_Sent_and_deleted :
				   Txt_MSG_Sent,
			 "ICO16x16");
	 HTM_TD_End ();

	 /***** Put user's photo *****/
	 HTM_TD_Begin ("class=\"CM\" style=\"width:30px;\"");
	    Pho_ShowUsrPhotoIfAllowed (UsrDat,
	                               ClassPhoto[Gbl.Prefs.UsrPhotos],Pho_ZOOM,
	                               false);
	 HTM_TD_End ();

	 /***** Write user's name *****/
	 HTM_TD_Begin ("class=\"AUTHOR_TXT LM\"");
	    if (UsrDat->UsrCod > 0)
	      {
	       HTM_Txt (UsrDat->FullName);
	       if (Act_GetSuperAction (Gbl.Action.Act) == ActSeeRcvMsg)
		 {
		  HTM_NBSP ();
		  if (Msg_DB_CheckIfUsrIsBanned (UsrDat->UsrCod,Gbl.Usrs.Me.UsrDat.UsrCod))
		     // Sender is banned
		     Msg_PutFormToUnbanSender (Messages,UsrDat);
		  else
		     // Sender is not banned
		     Msg_PutFormToBanSender (Messages,UsrDat);
		 }
	      }
	    else
	       HTM_TxtF ("[%s]",Txt_ROLES_SINGUL_abc[Rol_UNK][Usr_SEX_UNKNOWN]);	// User not found, likely an old user who has been removed
	 HTM_TD_End ();

      HTM_TR_End ();
   HTM_TABLE_End ();
  }

/*****************************************************************************/
/******************** Write list of recipients of a message ******************/
/*****************************************************************************/

#define Msg_MAX_RECIPIENTS_TO_SHOW 10	// If number of recipients <= Msg_MAX_RECIPIENTS_TO_SHOW, show all recipients
#define Msg_DEF_RECIPIENTS_TO_SHOW  5	// If number of recipients  > Msg_MAX_RECIPIENTS_TO_SHOW, show only Msg_DEF_RECIPIENTS_TO_SHOW

static void Msg_WriteMsgTo (struct Msg_Messages *Messages,long MsgCod)
  {
   extern const char *Txt_MSG_Open_and_deleted;
   extern const char *Txt_MSG_Open;
   extern const char *Txt_MSG_Deleted_without_opening;
   extern const char *Txt_MSG_Unopened;
   extern const char *Txt_unknown_recipient;
   extern const char *Txt_unknown_recipients;
   extern const char *Txt_View_all_recipients;
   extern const char *Txt_and_X_other_recipients;
   extern const char *Txt_unknown_recipient;
   extern const char *Txt_unknown_recipients;
   static const Act_Action_t ActionSee[Msg_NUM_TYPES_OF_MSGS] =
     {
      [Msg_WRITING ] = ActUnk,
      [Msg_RECEIVED] = ActSeeRcvMsg,
      [Msg_SENT    ] = ActSeeSntMsg,
     };
   static const char *ClassPhoto[Set_NUM_USR_PHOTOS] =
     {
      [Set_USR_PHOTO_CIRCLE   ] = "PHOTOC21x28",
      [Set_USR_PHOTO_ELLIPSE  ] = "PHOTOE21x28",
      [Set_USR_PHOTO_OVAL     ] = "PHOTOO21x28",
      [Set_USR_PHOTO_RECTANGLE] = "PHOTOR21x28",
     };
   MYSQL_RES *mysql_res;
   MYSQL_ROW row;
   unsigned NumRcp;
   struct
     {
      unsigned Total;
      unsigned Known;
      unsigned Unknown;
      unsigned ToShow;
     } NumRecipients;
   struct UsrData UsrDat;
   bool Deleted;
   bool OpenByDst;
   bool UsrValid;
   bool ShowPhoto;
   const char *Title;
   char PhotoURL[PATH_MAX + 1];

   /***** Get number of recipients of a message from database *****/
   NumRecipients.Total = Msg_DB_GetNumRecipients (MsgCod);

   /***** Get recipients of a message from database *****/
   NumRecipients.Known = Msg_DB_GetKnownRecipients (&mysql_res,MsgCod);

   /***** Check number of recipients *****/
   if (NumRecipients.Total)
     {
      /***** Begin table *****/
      HTM_TABLE_Begin (NULL);

	 /***** How many recipients will be shown? *****/
	 if (NumRecipients.Known <= Msg_MAX_RECIPIENTS_TO_SHOW)
	    NumRecipients.ToShow = NumRecipients.Known;
	 else	// A lot of recipients
	    /***** Get parameter that indicates if I want to see all recipients *****/
	    NumRecipients.ToShow = Par_GetParToBool ("SeeAllRcpts") ? NumRecipients.Known :
								      Msg_DEF_RECIPIENTS_TO_SHOW;

	 /***** Initialize structure with user's data *****/
	 Usr_UsrDataConstructor (&UsrDat);

	 /***** Write known recipients *****/
	 for (NumRcp = 0;
	      NumRcp < NumRecipients.ToShow;
	      NumRcp++)
	   {
	    row = mysql_fetch_row (mysql_res);

	    /* Get user's code (row[0]) */
	    UsrDat.UsrCod = Str_ConvertStrCodToLongCod (row[0]);

	    /* Get if message has been deleted (row[1]) and read (row[2]) by recipient */
	    Deleted   = (row[1][0] == 'Y');
	    OpenByDst = (row[2][0] == 'Y');

	    /* Get user's data */
	    UsrValid = Usr_ChkUsrCodAndGetAllUsrDataFromUsrCod (&UsrDat,
								Usr_DONT_GET_PREFS,
								Usr_DONT_GET_ROLE_IN_CURRENT_CRS);

	    /* Put an icon to show if user has read the message */
	    Title = OpenByDst ? (Deleted ? Txt_MSG_Open_and_deleted :
					   Txt_MSG_Open) :
				(Deleted ? Txt_MSG_Deleted_without_opening :
					   Txt_MSG_Unopened);
	    HTM_TR_Begin (NULL);

	       HTM_TD_Begin ("class=\"LM\" style=\"width:20px;\"");
		  Ico_PutIcon (OpenByDst ? (Deleted ? "envelope-open-text-red.svg"   :
						      "envelope-open-text.svg") :
					   (Deleted ? "envelope-red.svg" :
						      "envelope.svg"),
			       Title,"ICO16x16");
	       HTM_TD_End ();

	       /* Put user's photo */
	       HTM_TD_Begin ("class=\"CT\" style=\"width:30px;\"");
		  ShowPhoto = (UsrValid ? Pho_ShowingUsrPhotoIsAllowed (&UsrDat,PhotoURL) :
					  false);
		  Pho_ShowUsrPhoto (&UsrDat,ShowPhoto ? PhotoURL :
							NULL,
				    ClassPhoto[Gbl.Prefs.UsrPhotos],Pho_ZOOM,
				    false);
	       HTM_TD_End ();

	       /* Write user's name */
	       HTM_TD_Begin ("class=\"%s LM\"",OpenByDst ? "AUTHOR_TXT" :
							   "AUTHOR_TXT_NEW");
		  if (UsrValid)
		     HTM_Txt (UsrDat.FullName);
		  else
		     HTM_TxtF ("[%s]",Txt_unknown_recipient);	// User not found, likely a user who has been removed
	       HTM_TD_End ();

	    HTM_TR_End ();
	   }

	 /***** If any recipients are unknown *****/
	 if ((NumRecipients.Unknown = NumRecipients.Total - NumRecipients.Known))
	   {
	    /***** Begin form to show all users *****/
	    HTM_TR_Begin (NULL);

	       HTM_TD_Begin ("colspan=\"3\" class=\"AUTHOR_TXT LM\"");
		  HTM_TxtF ("[%u %s]",
			    NumRecipients.Unknown,
			    (NumRecipients.Unknown == 1) ? Txt_unknown_recipient :
							   Txt_unknown_recipients);
	       HTM_TD_End ();

	    HTM_TR_End ();
	   }

	 /***** If any known recipient is not listed *****/
	 if (NumRecipients.ToShow < NumRecipients.Known)
	   {
	    /***** Begin form to show all users *****/
	    HTM_TR_Begin (NULL);

	       HTM_TD_Begin ("colspan=\"3\" class=\"AUTHOR_TXT LM\"");
		  Frm_BeginForm (ActionSee[Messages->TypeOfMessages]);
		     Messages->MsgCod = MsgCod;	// Message to be expanded with all recipients visible
		     Msg_PutHiddenParamsOneMsg (Messages);
		     Par_PutHiddenParamChar ("SeeAllRcpts",'Y');
			HTM_BUTTON_SUBMIT_Begin (Txt_View_all_recipients,"BT_LINK AUTHOR_TXT",NULL);
			   HTM_TxtF (Txt_and_X_other_recipients,
				     NumRecipients.Known - NumRecipients.ToShow);
			HTM_BUTTON_End ();
		  Frm_EndForm ();
	       HTM_TD_End ();

	    HTM_TR_End ();
	   }

	 /***** Free memory used for user's data *****/
	 Usr_UsrDataDestructor (&UsrDat);

      /***** End table *****/
      HTM_TABLE_End ();
     }

   /***** Free structure that stores the query result *****/
   DB_FreeMySQLResult (&mysql_res);
  }

/*****************************************************************************/
/******************* Write the date of creation of a message *****************/
/*****************************************************************************/
// TimeUTC holds UTC date and time in UNIX format (seconds since 1970)

void Msg_WriteMsgDate (time_t TimeUTC,const char *ClassBackground)
  {
   static unsigned UniqueId = 0;
   char *Id;

   UniqueId++;
   if (asprintf (&Id,"msg_date_%u",UniqueId) < 0)
      Err_NotEnoughMemoryExit ();

   /***** Begin cell *****/
   HTM_TD_Begin ("id=\"%s\" class=\"%s RT\" style=\"width:106px;\"",
                 Id,ClassBackground);

      /***** Write date and time *****/
      Dat_WriteLocalDateHMSFromUTC (Id,TimeUTC,
				    Gbl.Prefs.DateFormat,Dat_SEPARATOR_COMMA,
				    true,true,false,0x6);

   /***** End cell *****/
   HTM_TD_End ();

   free (Id);
  }

/*****************************************************************************/
/********************* Write the text (content) of a message *****************/
/*****************************************************************************/

void Msg_WriteMsgContent (char Content[Cns_MAX_BYTES_LONG_TEXT + 1],
                          bool InsertLinks,bool ChangeBRToRet)
  {
   /***** Insert links in URLs *****/
   if (InsertLinks)
      ALn_InsertLinks (Content,Cns_MAX_BYTES_LONG_TEXT,60);

   /***** Write message to file *****/
   if (ChangeBRToRet)
      Str_FilePrintStrChangingBRToRetAndNBSPToSpace (Gbl.F.Out,Content);
   else
      HTM_Txt (Content);
  }

/*****************************************************************************/
/*************** Get parameter with the code of a message ********************/
/*****************************************************************************/

void Msg_PutHiddenParamMsgCod (long MsgCod)
  {
   Par_PutHiddenParamLong (NULL,"MsgCod",MsgCod);
  }

/*****************************************************************************/
/*************** Get parameter with the code of a message ********************/
/*****************************************************************************/

static long Msg_GetParamMsgCod (void)
  {
   /***** Get code of message *****/
   return Par_GetParToLong ("MsgCod");
  }

/*****************************************************************************/
/***************** Put a form to ban the sender of a message *****************/
/*****************************************************************************/

static void Msg_PutFormToBanSender (struct Msg_Messages *Messages,
                                    struct UsrData *UsrDat)
  {
   extern const char *Txt_Sender_permitted_click_to_ban_him;

   Frm_BeginForm (ActBanUsrMsg);
      Pag_PutHiddenParamPagNum (Msg_WhatPaginate[Messages->TypeOfMessages],
				Messages->CurrentPage);
      Usr_PutParamUsrCodEncrypted (UsrDat->EnUsrCod);
      Msg_PutHiddenParamsMsgsFilters (Messages);
	 Ico_PutIconLink ("unlock.svg",Txt_Sender_permitted_click_to_ban_him);
   Frm_EndForm ();
  }

/*****************************************************************************/
/**************** Put a form to unban the sender of a message ****************/
/*****************************************************************************/

static void Msg_PutFormToUnbanSender (struct Msg_Messages *Messages,
                                      struct UsrData *UsrDat)
  {
   extern const char *Txt_Sender_banned_click_to_unban_him;

   Frm_BeginForm (ActUnbUsrMsg);
      Pag_PutHiddenParamPagNum (Msg_WhatPaginate[Messages->TypeOfMessages],
				Messages->CurrentPage);
      Usr_PutParamUsrCodEncrypted (UsrDat->EnUsrCod);
      Msg_PutHiddenParamsMsgsFilters (Messages);
	 Ico_PutIconLink ("lock.svg",Txt_Sender_banned_click_to_unban_him);
   Frm_EndForm ();
  }

/*****************************************************************************/
/********* Ban a sender of a message when showing received messages **********/
/*****************************************************************************/

void Msg_BanSenderWhenShowingMsgs (void)
  {
   extern const char *Txt_From_this_time_you_will_not_receive_messages_from_X;

   /***** Get user's code from form *****/
   Usr_GetParamOtherUsrCodEncryptedAndGetListIDs ();

   /***** Get password, user type and user's data from database *****/
   if (!Usr_ChkUsrCodAndGetAllUsrDataFromUsrCod (&Gbl.Usrs.Other.UsrDat,
                                                 Usr_DONT_GET_PREFS,
                                                 Usr_DONT_GET_ROLE_IN_CURRENT_CRS))
      Err_WrongUserExit ();

   /***** Insert pair (sender's code - my code) in table of banned senders if not inserted *****/
   Msg_DB_CreateUsrsPairIntoBanned (Gbl.Usrs.Other.UsrDat.UsrCod,	// From
                                    Gbl.Usrs.Me.UsrDat.UsrCod);		// To

   /***** Show alert with the change made *****/
   Ale_ShowAlert (Ale_SUCCESS,Txt_From_this_time_you_will_not_receive_messages_from_X,
                  Gbl.Usrs.Other.UsrDat.FullName);

   /**** Show received messages again */
   Msg_ShowRecMsgs ();
  }

/*****************************************************************************/
/******** Unban a sender of a message when showing received messages *********/
/*****************************************************************************/

void Msg_UnbanSenderWhenShowingMsgs (void)
  {
   /**** Unban sender *****/
   Msg_UnbanSender ();

   /**** Show received messages again */
   Msg_ShowRecMsgs ();
  }

/*****************************************************************************/
/********** Unban a sender of a message when listing banned users ************/
/*****************************************************************************/

void Msg_UnbanSenderWhenListingUsrs (void)
  {
   /**** Unban sender *****/
   Msg_UnbanSender ();

   /**** List banned users again */
   Msg_ListBannedUsrs ();
  }

/*****************************************************************************/
/************************ Unban a sender of a message ************************/
/*****************************************************************************/

static void Msg_UnbanSender (void)
  {
   extern const char *Txt_From_this_time_you_can_receive_messages_from_X;

   /***** Get user's code from form *****/
   Usr_GetParamOtherUsrCodEncryptedAndGetListIDs ();

   /***** Get password, user type and user's data from database *****/
   if (!Usr_ChkUsrCodAndGetAllUsrDataFromUsrCod (&Gbl.Usrs.Other.UsrDat,
                                                 Usr_DONT_GET_PREFS,
                                                 Usr_DONT_GET_ROLE_IN_CURRENT_CRS))
      Err_WrongUserExit ();

   /***** Remove pair (sender's code - my code) from table of banned senders *****/
   Msg_DB_RemoveUsrsPairFromBanned (Gbl.Usrs.Other.UsrDat.UsrCod,	// From
                                    Gbl.Usrs.Me.UsrDat.UsrCod);		// To

   /***** Show alert with the change made *****/
   Ale_ShowAlert (Ale_SUCCESS,Txt_From_this_time_you_can_receive_messages_from_X,
                  Gbl.Usrs.Other.UsrDat.FullName);
  }

/*****************************************************************************/
/*********** List banned users, who can not sent messages to me **************/
/*****************************************************************************/

void Msg_ListBannedUsrs (void)
  {
   extern const char *Txt_You_have_not_banned_any_sender;
   extern const char *Txt_Banned_users;
   extern const char *Txt_Sender_banned_click_to_unban_him;
   static const char *ClassPhoto[Set_NUM_USR_PHOTOS] =
     {
      [Set_USR_PHOTO_CIRCLE   ] = "PHOTOC21x28",
      [Set_USR_PHOTO_ELLIPSE  ] = "PHOTOE21x28",
      [Set_USR_PHOTO_OVAL     ] = "PHOTOO21x28",
      [Set_USR_PHOTO_RECTANGLE] = "PHOTOR21x28",
     };
   MYSQL_RES *mysql_res;
   unsigned NumUsr;
   unsigned NumUsrs;
   struct UsrData UsrDat;

   /***** Get users banned by me *****/
   if ((NumUsrs = Msg_DB_GetUsrsBannedBy (&mysql_res,Gbl.Usrs.Me.UsrDat.UsrCod)))
     {
      /***** Initialize structure with user's data *****/
      Usr_UsrDataConstructor (&UsrDat);

      /***** Begin box and table *****/
      Box_BoxTableBegin (NULL,Txt_Banned_users,
                         NULL,NULL,
                         NULL,Box_NOT_CLOSABLE,2);

	 /***** List users *****/
	 for (NumUsr  = 1;
	      NumUsr <= NumUsrs;
	      NumUsr++)
	   {
	    /* Get user's code */
	    UsrDat.UsrCod = DB_GetNextCode (mysql_res);

	    /* Get user's data from database */
	    if (Usr_ChkUsrCodAndGetAllUsrDataFromUsrCod (&UsrDat,
							 Usr_DONT_GET_PREFS,
							 Usr_DONT_GET_ROLE_IN_CURRENT_CRS))
	      {
	       HTM_TR_Begin (NULL);

		  /* Put form to unban user */
		  HTM_TD_Begin ("class=\"BM\"");
		     Frm_BeginForm (ActUnbUsrLst);
			Usr_PutParamUsrCodEncrypted (UsrDat.EnUsrCod);
			Ico_PutIconLink ("lock.svg",Txt_Sender_banned_click_to_unban_him);
		     Frm_EndForm ();
		  HTM_TD_End ();

		  /* Show photo */
		  HTM_TD_Begin ("class=\"LM\" style=\"width:30px;\"");
		     Pho_ShowUsrPhotoIfAllowed (&UsrDat,
		                                ClassPhoto[Gbl.Prefs.UsrPhotos],Pho_ZOOM,
		                                false);
		  HTM_TD_End ();

		  /* Write user's full name */
		  HTM_TD_Begin ("class=\"DAT LM\"");
		     HTM_Txt (UsrDat.FullName);
		  HTM_TD_End ();

	       HTM_TR_End ();
	      }
	   }

      /***** End table and box *****/
      Box_BoxTableEnd ();

      /***** Free memory used for user's data *****/
      Usr_UsrDataDestructor (&UsrDat);
     }
   else   // If not result ==> sent message is deleted
      Ale_ShowAlert (Ale_INFO,Txt_You_have_not_banned_any_sender);

   /***** Free structure that stores the query result *****/
   DB_FreeMySQLResult (&mysql_res);
  }
