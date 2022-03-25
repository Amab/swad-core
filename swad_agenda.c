// swad_agenda.c: user's agenda (personal organizer)

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
/********************************* Headers ***********************************/
/*****************************************************************************/

#define _GNU_SOURCE 		// For asprintf
#include <linux/limits.h>	// For PATH_MAX
#include <stddef.h>		// For NULL
#include <stdio.h>		// For asprintf
#include <stdlib.h>		// For calloc
#include <string.h>		// For string functions

#include "swad_agenda.h"
#include "swad_agenda_database.h"
#include "swad_autolink.h"
#include "swad_box.h"
#include "swad_database.h"
#include "swad_date.h"
#include "swad_error.h"
#include "swad_figure.h"
#include "swad_form.h"
#include "swad_global.h"
#include "swad_group.h"
#include "swad_HTML.h"
#include "swad_notification.h"
#include "swad_pagination.h"
#include "swad_parameter.h"
#include "swad_photo.h"
#include "swad_privacy.h"
#include "swad_QR.h"
#include "swad_setting.h"
#include "swad_string.h"

/*****************************************************************************/
/************** External global variables from others modules ****************/
/*****************************************************************************/

extern struct Globals Gbl;

/*****************************************************************************/
/***************************** Private constants *****************************/
/*****************************************************************************/

static const char *ParamPast__FutureName = "Past__Future";
static const char *ParamPrivatPublicName = "PrivatPublic";
static const char *ParamHiddenVisiblName = "HiddenVisibl";

/*****************************************************************************/
/***************************** Private prototypes ****************************/
/*****************************************************************************/

static void Agd_ResetAgenda (struct Agd_Agenda *Agenda);

static void Agd_ShowMyAgenda (struct Agd_Agenda *Agenda);

static void Agd_ShowFormToSelPast__FutureEvents (const struct Agd_Agenda *Agenda);
static void Agd_ShowFormToSelPrivatPublicEvents (const struct Agd_Agenda *Agenda);
static void Agd_ShowFormToSelHiddenVisiblEvents (const struct Agd_Agenda *Agenda);

static void Agd_PutHiddenParamPast__FutureEvents (unsigned Past__FutureEvents);
static void Agd_PutHiddenParamPrivatPublicEvents (unsigned PrivatPublicEvents);
static void Agd_PutHiddenParamHiddenVisiblEvents (unsigned HiddenVisiblEvents);
static unsigned Agd_GetParamsPast__FutureEvents (void);
static unsigned Agd_GetParamsPrivatPublicEvents (void);
static unsigned Agd_GetParamsHiddenVisiblEvents (void);

static void Agd_ShowEvents (struct Agd_Agenda *Agenda,
                            Agd_AgendaType_t AgendaType);
static void Agd_ShowEventsToday (struct Agd_Agenda *Agenda,
                                 Agd_AgendaType_t AgendaType);
static void Agd_WriteHeaderListEvents (const struct Agd_Agenda *Agenda,
                                       Agd_AgendaType_t AgendaType);

static void Agd_PutIconsMyFullAgenda (void *Agenda);
static void Agd_PutIconsMyPublicAgenda (void *EncryptedUsrCod);
static void Agd_PutIconToCreateNewEvent (void *Agenda);
static void Agd_PutIconToViewEditMyFullAgenda (void *EncryptedUsrCod);
static void Agd_PutIconToShowQR (void);
static void Agd_PutIconsOtherPublicAgenda (void *EncryptedUsrCod);

static void Agd_PutButtonToCreateNewEvent (const struct Agd_Agenda *Agenda);
static void Agd_ShowOneEvent (struct Agd_Agenda *Agenda,
                              Agd_AgendaType_t AgendaType,long AgdCod);
static void Agd_GetParamEventOrder (struct Agd_Agenda *Agenda);
static void Agd_PutFormsToRemEditOneEvent (struct Agd_Agenda *Agenda,
                                           struct Agd_Event *AgdEvent,
                                           const char *Anchor);

static void Agd_PutCurrentParamsMyAgenda (void *Agenda);
static void Agd_GetParams (struct Agd_Agenda *Agenda,
                           Agd_AgendaType_t AgendaType);

static void Agd_GetListEvents (struct Agd_Agenda *Agenda,
                               Agd_AgendaType_t AgendaType);
static void Agd_GetDataOfEventByCod (struct Agd_Event *AgdEvent);

static void Agd_FreeListEvents (struct Agd_Agenda *Agenda);

/*****************************************************************************/
/*************************** Reset agenda context ****************************/
/*****************************************************************************/

static void Agd_ResetAgenda (struct Agd_Agenda *Agenda)
  {
   Agenda->LstIsRead          = false;
   Agenda->Num                = 0;
   Agenda->LstAgdCods         = NULL;
   Agenda->Past__FutureEvents = Agd_DEFAULT_PAST___EVENTS |
	                        Agd_DEFAULT_FUTURE_EVENTS;
   Agenda->PrivatPublicEvents = Agd_DEFAULT_PRIVAT_EVENTS |
	                        Agd_DEFAULT_PUBLIC_EVENTS;
   Agenda->HiddenVisiblEvents = Agd_DEFAULT_HIDDEN_EVENTS |
	                        Agd_DEFAULT_VISIBL_EVENTS;
   Agenda->SelectedOrder      = Agd_ORDER_DEFAULT;
   Agenda->AgdCodToEdit       = -1L;
   Agenda->CurrentPage        = 0;
  }

/*****************************************************************************/
/********** Put form to log in and then show another user's agenda ***********/
/*****************************************************************************/

void Agd_PutFormLogInToShowUsrAgenda (void)
  {
   /***** Form to log in *****/
   Usr_WriteFormLogin (ActLogInUsrAgd,Agd_PutParamAgd);
  }

void Agd_PutParamAgd (void)
  {
   char NickWithArr[Nck_MAX_BYTES_NICK_WITH_ARROBA + 1];

   snprintf (NickWithArr,sizeof (NickWithArr),"@%s",Gbl.Usrs.Other.UsrDat.Nickname);
   Par_PutHiddenParamString (NULL,"agd",NickWithArr);
  }

/*****************************************************************************/
/******************************* Show my agenda ******************************/
/*****************************************************************************/

void Agd_GetParamsAndShowMyAgenda (void)
  {
   struct Agd_Agenda Agenda;

   /***** Reset agenda context *****/
   Agd_ResetAgenda (&Agenda);

   /***** Get parameters *****/
   Agd_GetParams (&Agenda,Agd_MY_AGENDA);

   /***** Show my agenda *****/
   Agd_ShowMyAgenda (&Agenda);
  }

static void Agd_ShowMyAgenda (struct Agd_Agenda *Agenda)
  {
   extern const char *Hlp_PROFILE_Agenda;
   extern const char *Txt_My_agenda;

   /***** Begin box *****/
   Box_BoxBegin ("100%",Txt_My_agenda,
                 Agd_PutIconsMyFullAgenda,Agenda,
		 Hlp_PROFILE_Agenda,Box_NOT_CLOSABLE);

      /***** Put forms to choice which events to show *****/
      Set_BeginSettingsHead ();
      Agd_ShowFormToSelPast__FutureEvents (Agenda);
      Agd_ShowFormToSelPrivatPublicEvents (Agenda);
      Agd_ShowFormToSelHiddenVisiblEvents (Agenda);
      Set_EndSettingsHead ();

      /***** Show the current events in the user's agenda *****/
      Agd_ShowEventsToday (Agenda,Agd_MY_AGENDA_TODAY);

      /***** Show all my events *****/
      Agd_ShowEvents (Agenda,Agd_MY_AGENDA);

   /***** End box *****/
   Box_BoxEnd ();
  }

/*****************************************************************************/
/*************** Show form to select past / future events ********************/
/*****************************************************************************/

static void Agd_ShowFormToSelPast__FutureEvents (const struct Agd_Agenda *Agenda)
  {
   extern const char *The_Colors[The_NUM_THEMES];
   extern const char *Txt_AGENDA_PAST___FUTURE_EVENTS[2];
   Agd_Past__FutureEvents_t PstFut;
   static const char *Icon[2] =
     {
      [Agd_PAST___EVENTS] = "calendar-minus.svg",
      [Agd_FUTURE_EVENTS] = "calendar-plus.svg",
     };

   Set_BeginOneSettingSelector ();
   for (PstFut  = Agd_PAST___EVENTS;
	PstFut <= Agd_FUTURE_EVENTS;
	PstFut++)
     {
      if ((Agenda->Past__FutureEvents & (1 << PstFut)))
	 HTM_DIV_Begin ("class=\"PREF_ON PREF_ON_%s\"",
	                The_Colors[Gbl.Prefs.Theme]);
      else
	 HTM_DIV_Begin ("class=\"PREF_OFF\"");
      Frm_BeginForm (ActSeeMyAgd);
	 Agd_PutParamsMyAgenda (Agenda->Past__FutureEvents ^ (1 << PstFut),	// Toggle
				Agenda->PrivatPublicEvents,
				Agenda->HiddenVisiblEvents,
				Agenda->SelectedOrder,
				Agenda->CurrentPage,
				-1L);
	 Ico_PutSettingIconLink (Icon[PstFut],Ico_BLACK,
				 Txt_AGENDA_PAST___FUTURE_EVENTS[PstFut]);
      Frm_EndForm ();
      HTM_DIV_End ();
     }
   Set_EndOneSettingSelector ();
  }

/*****************************************************************************/
/************** Show form to select private / public events ******************/
/*****************************************************************************/

static void Agd_ShowFormToSelPrivatPublicEvents (const struct Agd_Agenda *Agenda)
  {
   extern const char *The_Colors[The_NUM_THEMES];
   extern const char *Txt_AGENDA_PRIVAT_PUBLIC_EVENTS[2];
   Agd_PrivatPublicEvents_t PrvPub;
   static const struct
     {
      const char *Icon;
      Ico_Color_t Color;
     } Icon[2] =
     {
      [Agd_PRIVAT_EVENTS] = {"lock.svg"  ,Ico_RED  },
      [Agd_PUBLIC_EVENTS] = {"unlock.svg",Ico_GREEN},
     };

   Set_BeginOneSettingSelector ();
   for (PrvPub  = Agd_PRIVAT_EVENTS;
	PrvPub <= Agd_PUBLIC_EVENTS;
	PrvPub++)
     {
      if ((Agenda->PrivatPublicEvents & (1 << PrvPub)))
	 HTM_DIV_Begin ("class=\"PREF_ON PREF_ON_%s\"",
	                The_Colors[Gbl.Prefs.Theme]);
      else
	 HTM_DIV_Begin ("class=\"PREF_OFF\"");
      Frm_BeginForm (ActSeeMyAgd);
	 Agd_PutParamsMyAgenda (Agenda->Past__FutureEvents,
				Agenda->PrivatPublicEvents ^ (1 << PrvPub),	// Toggle
				Agenda->HiddenVisiblEvents,
				Agenda->SelectedOrder,
				Agenda->CurrentPage,
				-1L);
	 Ico_PutSettingIconLink (Icon[PrvPub].Icon,Icon[PrvPub].Color,
				 Txt_AGENDA_PRIVAT_PUBLIC_EVENTS[PrvPub]);
      Frm_EndForm ();
      HTM_DIV_End ();
     }
   Set_EndOneSettingSelector ();
  }

/*****************************************************************************/
/************* Show form to select hidden / visible events *******************/
/*****************************************************************************/

static void Agd_ShowFormToSelHiddenVisiblEvents (const struct Agd_Agenda *Agenda)
  {
   extern const char *The_Colors[The_NUM_THEMES];
   extern const char *Txt_AGENDA_HIDDEN_VISIBL_EVENTS[2];
   Agd_HiddenVisiblEvents_t HidVis;
   static const struct
     {
      const char *Icon;
      Ico_Color_t Color;
     } Icon[2] =
     {
      [Agd_HIDDEN_EVENTS] = {"eye-slash.svg",Ico_RED  },
      [Agd_VISIBL_EVENTS] = {"eye.svg"      ,Ico_GREEN},
     };

   Set_BeginOneSettingSelector ();
   for (HidVis  = Agd_HIDDEN_EVENTS;
	HidVis <= Agd_VISIBL_EVENTS;
	HidVis++)
     {
      if ((Agenda->HiddenVisiblEvents & (1 << HidVis)))
	 HTM_DIV_Begin ("class=\"PREF_ON PREF_ON_%s\"",
	                The_Colors[Gbl.Prefs.Theme]);
      else
	 HTM_DIV_Begin ("class=\"PREF_OFF\"");
      Frm_BeginForm (ActSeeMyAgd);
	 Agd_PutParamsMyAgenda (Agenda->Past__FutureEvents,
				Agenda->PrivatPublicEvents,
				Agenda->HiddenVisiblEvents ^ (1 << HidVis),	// Toggle
				Agenda->SelectedOrder,
				Agenda->CurrentPage,
				-1L);
	 Ico_PutSettingIconLink (Icon[HidVis].Icon,Icon[HidVis].Color,
				 Txt_AGENDA_HIDDEN_VISIBL_EVENTS[HidVis]);
      Frm_EndForm ();
      HTM_DIV_End ();
     }
   Set_EndOneSettingSelector ();
  }

/*****************************************************************************/
/************************ Put hidden params for events ***********************/
/*****************************************************************************/

static void Agd_PutHiddenParamPast__FutureEvents (unsigned Past__FutureEvents)
  {
   Par_PutHiddenParamUnsigned (NULL,ParamPast__FutureName,Past__FutureEvents);
  }

static void Agd_PutHiddenParamPrivatPublicEvents (unsigned PrivatPublicEvents)
  {
   Par_PutHiddenParamUnsigned (NULL,ParamPrivatPublicName,PrivatPublicEvents);
  }

static void Agd_PutHiddenParamHiddenVisiblEvents (unsigned HiddenVisiblEvents)
  {
   Par_PutHiddenParamUnsigned (NULL,ParamHiddenVisiblName,HiddenVisiblEvents);
  }

/*****************************************************************************/
/************************ Get hidden params for events ***********************/
/*****************************************************************************/

static unsigned Agd_GetParamsPast__FutureEvents (void)
  {
   return (unsigned) Par_GetParToUnsignedLong (ParamPast__FutureName,
					       0,
					       (1 << Agd_PAST___EVENTS) |
					       (1 << Agd_FUTURE_EVENTS),
					       Agd_DEFAULT_PAST___EVENTS |
					       Agd_DEFAULT_FUTURE_EVENTS);
  }

static unsigned Agd_GetParamsPrivatPublicEvents (void)
  {
   return (unsigned) Par_GetParToUnsignedLong (ParamPrivatPublicName,
					       0,
					       (1 << Agd_PRIVAT_EVENTS) |
					       (1 << Agd_PUBLIC_EVENTS),
					       Agd_DEFAULT_PRIVAT_EVENTS |
					       Agd_DEFAULT_PUBLIC_EVENTS);
  }

static unsigned Agd_GetParamsHiddenVisiblEvents (void)
  {
   return (unsigned) Par_GetParToUnsignedLong (ParamHiddenVisiblName,
					       0,
					       (1 << Agd_HIDDEN_EVENTS) |
					       (1 << Agd_VISIBL_EVENTS),
					       Agd_DEFAULT_HIDDEN_EVENTS |
					       Agd_DEFAULT_VISIBL_EVENTS);
  }

/*****************************************************************************/
/************************ Show another user's agenda *************************/
/*****************************************************************************/

void Agd_ShowUsrAgenda (void)
  {
   extern const char *Hlp_PROFILE_Agenda_public_agenda;
   extern const char *Txt_Public_agenda_USER;
   struct Agd_Agenda Agenda;
   bool Error = true;
   bool ItsMe;
   char *Title;

   /***** Get user *****/
   if (Usr_GetParamOtherUsrCodEncryptedAndGetUsrData ())
      if (Usr_CheckIfICanViewUsrAgenda (&Gbl.Usrs.Other.UsrDat))
	{
	 Error = false;

	 /***** Reset agenda context *****/
	 Agd_ResetAgenda (&Agenda);

	 /***** Begin box *****/
	 ItsMe = Usr_ItsMe (Gbl.Usrs.Other.UsrDat.UsrCod);
	 if (asprintf (&Title,Txt_Public_agenda_USER,
	               ItsMe ? Gbl.Usrs.Me.UsrDat.FullName :
	        	       Gbl.Usrs.Other.UsrDat.FullName) < 0)
            Err_NotEnoughMemoryExit ();
	 Box_BoxBegin ("100%",Title,
		       ItsMe ? Agd_PutIconsMyPublicAgenda :
			       Agd_PutIconsOtherPublicAgenda,
		       ItsMe ? Gbl.Usrs.Me.UsrDat.EnUsrCod :
			       Gbl.Usrs.Other.UsrDat.EnUsrCod,
		       Hlp_PROFILE_Agenda_public_agenda,Box_NOT_CLOSABLE);
         free (Title);

	    /***** Show the current events in the user's agenda *****/
	    Agd_ShowEventsToday (&Agenda,Agd_ANOTHER_AGENDA_TODAY);

	    /***** Show all visible events in the user's agenda *****/
	    Agd_ShowEvents (&Agenda,Agd_ANOTHER_AGENDA);

	 /***** End box *****/
	 Box_BoxEnd ();
	}

   if (Error)
      Ale_ShowAlertUserNotFoundOrYouDoNotHavePermission ();
  }

/*****************************************************************************/
/***************** Show another user's agenda after log in *******************/
/*****************************************************************************/

void Agd_ShowOtherAgendaAfterLogIn (void)
  {
   extern const char *Hlp_PROFILE_Agenda_public_agenda;
   extern const unsigned Txt_Current_CGI_SWAD_Language;
   extern const char *Txt_Public_agenda_USER;
   extern const char *Txt_Switching_to_LANGUAGE[1 + Lan_NUM_LANGUAGES];
   struct Agd_Agenda Agenda;
   bool ItsMe;
   char *Title;

   if (Gbl.Usrs.Me.Logged)
     {
      if (Gbl.Usrs.Me.UsrDat.Prefs.Language == Txt_Current_CGI_SWAD_Language)
        {
	 /***** Get user *****/
	 /* If nickname is correct, user code is already got from nickname */
	 if (Usr_ChkUsrCodAndGetAllUsrDataFromUsrCod (&Gbl.Usrs.Other.UsrDat,        // Existing user
	                                              Usr_DONT_GET_PREFS,
	                                              Usr_DONT_GET_ROLE_IN_CURRENT_CRS))
	   {
	    /***** Reset agenda context *****/
	    Agd_ResetAgenda (&Agenda);

	    /***** Begin box *****/
	    ItsMe = Usr_ItsMe (Gbl.Usrs.Other.UsrDat.UsrCod);
	    if (asprintf (&Title,Txt_Public_agenda_USER,
			  ItsMe ? Gbl.Usrs.Me.UsrDat.FullName :
				  Gbl.Usrs.Other.UsrDat.FullName) < 0)
	       Err_NotEnoughMemoryExit ();
	    Box_BoxBegin ("100%",Title,
			  ItsMe ? Agd_PutIconToViewEditMyFullAgenda :
				  Agd_PutIconsOtherPublicAgenda,
			  ItsMe ? Gbl.Usrs.Me.UsrDat.EnUsrCod :
				  Gbl.Usrs.Other.UsrDat.EnUsrCod,
			  Hlp_PROFILE_Agenda_public_agenda,Box_NOT_CLOSABLE);
            free (Title);

	       /***** Show the current events in the user's agenda *****/
	       Agd_ShowEventsToday (&Agenda,Agd_ANOTHER_AGENDA_TODAY);

	       /***** Show all visible events in the user's agenda *****/
	       Agd_ShowEvents (&Agenda,Agd_ANOTHER_AGENDA);

	    /***** End box *****/
	    Box_BoxEnd ();
           }
	 else
	    Ale_ShowAlertUserNotFoundOrYouDoNotHavePermission ();
       }
      else
	 /* The current language is not my preferred language
	    ==> change automatically to my language */
         Ale_ShowAlert (Ale_INFO,Txt_Switching_to_LANGUAGE[Gbl.Usrs.Me.UsrDat.Prefs.Language]);
     }
  }

/*****************************************************************************/
/*************************** Show events in agenda ***************************/
/*****************************************************************************/

static void Agd_ShowEvents (struct Agd_Agenda *Agenda,
                            Agd_AgendaType_t AgendaType)
  {
   extern const char *Hlp_PROFILE_Agenda;
   extern const char *Txt_Public_agenda_USER;
   extern const char *Txt_My_agenda;
   extern const char *Txt_No_events;
   struct Pagination Pagination;
   unsigned NumEvent;
   static const Pag_WhatPaginate_t WhatPaginate[Agd_NUM_AGENDA_TYPES] =
     {
      [Agd_MY_AGENDA_TODAY     ] = Pag_MY_AGENDA,	// not used
      [Agd_MY_AGENDA           ] = Pag_MY_AGENDA,
      [Agd_ANOTHER_AGENDA_TODAY] = Pag_ANOTHER_AGENDA,	// not used
      [Agd_ANOTHER_AGENDA      ] = Pag_ANOTHER_AGENDA,
     };

   /***** Get parameters *****/
   Agd_GetParams (Agenda,AgendaType);

   /***** Get list of events *****/
   Agd_GetListEvents (Agenda,AgendaType);

   /***** Compute variables related to pagination *****/
   Pagination.NumItems = Agenda->Num;
   Pagination.CurrentPage = (int) Agenda->CurrentPage;
   Pag_CalculatePagination (&Pagination);
   Agenda->CurrentPage = (unsigned) Pagination.CurrentPage;

   /***** Write links to pages *****/
   Pag_WriteLinksToPagesCentered (WhatPaginate[AgendaType],&Pagination,
				  Agenda,-1L);

   if (Agenda->Num)
     {
      /***** Begin table *****/
      HTM_TABLE_BeginWideMarginPadding (2);

	 /***** Table head *****/
	 Agd_WriteHeaderListEvents (Agenda,AgendaType);

	 /***** Write all events *****/
	 for (NumEvent  = Pagination.FirstItemVisible;
	      NumEvent <= Pagination.LastItemVisible;
	      NumEvent++)
	    Agd_ShowOneEvent (Agenda,AgendaType,Agenda->LstAgdCods[NumEvent - 1]);

      /***** End table *****/
      HTM_TABLE_End ();
     }
   else
      Ale_ShowAlert (Ale_INFO,Txt_No_events);

   /***** Write again links to pages *****/
   Pag_WriteLinksToPagesCentered (WhatPaginate[AgendaType],&Pagination,
				  Agenda,-1L);

   /***** Button to create a new event *****/
   if (AgendaType == Agd_MY_AGENDA)
      Agd_PutButtonToCreateNewEvent (Agenda);

   /***** Free list of events *****/
   Agd_FreeListEvents (Agenda);
  }

/*****************************************************************************/
/************************ Show today events in agenda ************************/
/*****************************************************************************/

static void Agd_ShowEventsToday (struct Agd_Agenda *Agenda,
                                 Agd_AgendaType_t AgendaType)
  {
   extern const char *Hlp_PROFILE_Agenda;
   extern const char *Hlp_PROFILE_Agenda_public_agenda;
   extern const char *Txt_Today;
   extern const char *Txt_Public_agenda_USER;
   extern const char *Txt_My_agenda;
   extern const char *Txt_No_events;
   unsigned NumEvent;

   /***** Get parameters *****/
   Agd_GetParams (Agenda,AgendaType);

   /***** Get list of events *****/
   Agd_GetListEvents (Agenda,AgendaType);

   if (Agenda->Num)
     {
      /***** Begin box and table *****/
      switch (AgendaType)
        {
	 case Agd_MY_AGENDA_TODAY:
	    Box_BoxTableShadowBegin (NULL,Txt_Today,
	                             NULL,NULL,
				     Hlp_PROFILE_Agenda,
				     2);
	    break;
	 case Agd_ANOTHER_AGENDA_TODAY:
	    Box_BoxTableShadowBegin (NULL,Txt_Today,
	                             NULL,NULL,
			             Hlp_PROFILE_Agenda_public_agenda,
				     2);
            break;
	 default:
	    break;
        }

	 /***** Table head *****/
	 Agd_WriteHeaderListEvents (Agenda,AgendaType);

	 /***** Write all events *****/
	 for (NumEvent = 0;
	      NumEvent < Agenda->Num;
	      NumEvent++)
	    Agd_ShowOneEvent (Agenda,AgendaType,Agenda->LstAgdCods[NumEvent]);

      /***** End table and box *****/
      Box_BoxTableEnd ();
     }

   /***** Free list of events *****/
   Agd_FreeListEvents (Agenda);
  }

/*****************************************************************************/
/*************** Put contextual icon to view/edit my agenda ******************/
/*****************************************************************************/

static void Agd_WriteHeaderListEvents (const struct Agd_Agenda *Agenda,
                                       Agd_AgendaType_t AgendaType)
  {
   extern const char *The_Colors[The_NUM_THEMES];
   extern const char *Txt_START_END_TIME_HELP[2];
   extern const char *Txt_START_END_TIME[Dat_NUM_START_END_TIME];
   extern const char *Txt_Event;
   extern const char *Txt_Location;
   Dat_StartEndTime_t Order;

   /***** Table head *****/
   HTM_TR_Begin (NULL);

      for (Order  = (Dat_StartEndTime_t) 0;
	   Order <= (Dat_StartEndTime_t) (Dat_NUM_START_END_TIME - 1);
	   Order++)
	{
         HTM_TH_Begin (HTM_HEAD_LEFT);
	    switch (AgendaType)
	      {
	       case Agd_MY_AGENDA_TODAY:
	       case Agd_MY_AGENDA:
		  Frm_BeginForm (ActSeeMyAgd);
		     Pag_PutHiddenParamPagNum (Pag_MY_AGENDA,Agenda->CurrentPage);
		  break;
	       case Agd_ANOTHER_AGENDA_TODAY:
	       case Agd_ANOTHER_AGENDA:
		  Frm_BeginForm (ActSeeUsrAgd);
		     Usr_PutParamOtherUsrCodEncrypted (Gbl.Usrs.Other.UsrDat.EnUsrCod);
		     Pag_PutHiddenParamPagNum (Pag_ANOTHER_AGENDA,Agenda->CurrentPage);
		  break;
	      }
	    Agd_PutParamsMyAgenda (Agenda->Past__FutureEvents,
				   Agenda->PrivatPublicEvents,
				   Agenda->HiddenVisiblEvents,
				   Order,
				   Agenda->CurrentPage,
				   -1L);

	       HTM_BUTTON_OnSubmit_Begin (Txt_START_END_TIME_HELP[Order],
	                                  "BT_LINK",NULL);
		  if (Order == Agenda->SelectedOrder)
		     HTM_U_Begin ();

		  HTM_Txt (Txt_START_END_TIME[Order]);

		  if (Order == Agenda->SelectedOrder)
		     HTM_U_End ();
	       HTM_BUTTON_End ();

	    Frm_EndForm ();
	 HTM_TH_End ();
	}

      HTM_TH (Txt_Event   ,HTM_HEAD_LEFT);
      HTM_TH (Txt_Location,HTM_HEAD_LEFT);

   HTM_TR_End ();
  }

/*****************************************************************************/
/********************** Put contextual icons in agenda ***********************/
/*****************************************************************************/

static void Agd_PutIconsMyFullAgenda (void *Agenda)
  {
   /***** Put icon to create a new event *****/
   Agd_PutIconToCreateNewEvent (Agenda);

   /***** Put icon to show QR code *****/
   Agd_PutIconToShowQR ();
  }

static void Agd_PutIconsMyPublicAgenda (void *EncryptedUsrCod)
  {
   /***** Put icon to view/edit my full agenda *****/
   Agd_PutIconToViewEditMyFullAgenda (EncryptedUsrCod);

   /***** Put icon to show QR code *****/
   Agd_PutIconToShowQR ();
  }

static void Agd_PutIconToCreateNewEvent (void *Agenda)
  {
   extern const char *Txt_New_event;

   /***** Put form to create a new event *****/
   ((struct Agd_Agenda *) Agenda)->AgdCodToEdit = -1L;
   Ico_PutContextualIconToAdd (ActFrmNewEvtMyAgd,NULL,
			       Agd_PutCurrentParamsMyAgenda,Agenda,
			       Txt_New_event);
  }

static void Agd_PutIconToViewEditMyFullAgenda (void *EncryptedUsrCod)
  {
   Ico_PutContextualIconToEdit (ActSeeMyAgd,NULL,
                                NULL,EncryptedUsrCod);
  }

static void Agd_PutIconToShowQR (void)
  {
   char URL[Cns_MAX_BYTES_WWW + 1];
   extern const char *Lan_STR_LANG_ID[1 + Lan_NUM_LANGUAGES];

   snprintf (URL,sizeof (URL),"%s/%s?agd=@%s",
             Cfg_URL_SWAD_CGI,Lan_STR_LANG_ID[Gbl.Prefs.Language],
             Gbl.Usrs.Me.UsrDat.Nickname);
   QR_PutLinkToPrintQRCode (ActPrnAgdQR,
                            QR_PutParamQRString,URL);
  }

static void Agd_PutIconsOtherPublicAgenda (void *EncryptedUsrCod)
  {
   extern const char *Txt_Another_user_s_profile;
   extern const char *Txt_View_record_for_this_course;
   extern const char *Txt_View_record_and_office_hours;

   /***** Button to view user's public profile *****/
   if (Pri_ShowingIsAllowed (Gbl.Usrs.Other.UsrDat.BaPrfVisibility,
		             &Gbl.Usrs.Other.UsrDat))
      Lay_PutContextualLinkOnlyIcon (ActSeeOthPubPrf,NULL,
                                     Usr_PutParamOtherUsrCodEncrypted,EncryptedUsrCod,
			             "user.svg",Ico_BLACK,
			             Txt_Another_user_s_profile);

   /***** Button to view user's record card *****/
   if (Usr_CheckIfICanViewRecordStd (&Gbl.Usrs.Other.UsrDat))
      /* View student's records: common record card and course record card */
      Lay_PutContextualLinkOnlyIcon (ActSeeRecOneStd,NULL,
                                     Usr_PutParamOtherUsrCodEncrypted,EncryptedUsrCod,
			             "address-card.svg",Ico_BLACK,
			             Txt_View_record_for_this_course);
   else if (Usr_CheckIfICanViewRecordTch (&Gbl.Usrs.Other.UsrDat))
      Lay_PutContextualLinkOnlyIcon (ActSeeRecOneTch,NULL,
			             Usr_PutParamOtherUsrCodEncrypted,EncryptedUsrCod,
			             "address-card.svg",Ico_BLACK,
			             Txt_View_record_and_office_hours);
  }

/*****************************************************************************/
/********************* Put button to create a new event **********************/
/*****************************************************************************/

static void Agd_PutButtonToCreateNewEvent (const struct Agd_Agenda *Agenda)
  {
   extern const char *Txt_New_event;

   /***** Begin form *****/
   Frm_BeginForm (ActFrmNewEvtMyAgd);
      Agd_PutParamsMyAgenda (Agenda->Past__FutureEvents,
			     Agenda->PrivatPublicEvents,
			     Agenda->HiddenVisiblEvents,
			     Agenda->SelectedOrder,
			     Agenda->CurrentPage,
			     -1L);

      /***** Confirm button *****/
      Btn_PutConfirmButton (Txt_New_event);

   /***** End form *****/
   Frm_EndForm ();
  }

/*****************************************************************************/
/******************************* Show one event ******************************/
/*****************************************************************************/

static void Agd_ShowOneEvent (struct Agd_Agenda *Agenda,
                              Agd_AgendaType_t AgendaType,long AgdCod)
  {
   extern const char *Dat_TimeStatusClassVisible[Dat_NUM_TIME_STATUS];
   extern const char *Dat_TimeStatusClassHidden[Dat_NUM_TIME_STATUS];
   extern const char *The_Colors[The_NUM_THEMES];
   char *Anchor = NULL;
   static unsigned UniqueId = 0;
   char *Id;
   struct Agd_Event AgdEvent;
   Dat_StartEndTime_t StartEndTime;
   char Txt[Cns_MAX_BYTES_TEXT + 1];

   /***** Get data of this event *****/
   AgdEvent.AgdCod = AgdCod;
   switch (AgendaType)
     {
      case Agd_MY_AGENDA_TODAY:
      case Agd_MY_AGENDA:
	 AgdEvent.UsrCod = Gbl.Usrs.Me.UsrDat.UsrCod;
         break;
      case Agd_ANOTHER_AGENDA_TODAY:
      case Agd_ANOTHER_AGENDA:
	 AgdEvent.UsrCod = Gbl.Usrs.Other.UsrDat.UsrCod;
         break;
     }
   Agd_GetDataOfEventByCod (&AgdEvent);

   /***** Set anchor string *****/
   Frm_SetAnchorStr (AgdEvent.AgdCod,&Anchor);

   /***** Write first row of data of this event *****/
   HTM_TR_Begin (NULL);

      /* Start/end date/time */
      UniqueId++;
      for (StartEndTime  = (Dat_StartEndTime_t) 0;
	   StartEndTime <= (Dat_StartEndTime_t) (Dat_NUM_START_END_TIME - 1);
	   StartEndTime++)
	{
	 if (asprintf (&Id,"agd_date_%u_%u",(unsigned) StartEndTime,UniqueId) < 0)
	    Err_NotEnoughMemoryExit ();
	 HTM_TD_Begin ("id=\"%s\" class=\"%s LB %s\"",
		       Id,
		       AgdEvent.Hidden ? Dat_TimeStatusClassHidden[AgdEvent.TimeStatus] :
					 Dat_TimeStatusClassVisible[AgdEvent.TimeStatus],
		       The_GetColorRows ());
	    Dat_WriteLocalDateHMSFromUTC (Id,AgdEvent.TimeUTC[StartEndTime],
					  Gbl.Prefs.DateFormat,Dat_SEPARATOR_BREAK,
					  true,true,true,0x6);
	 HTM_TD_End ();
	 free (Id);
	}

      /* Event */
      HTM_TD_Begin ("class=\"%s LT %s\"",
		    AgdEvent.Hidden ? "ASG_TITLE_LIGHT" :
				      "ASG_TITLE",
		    The_GetColorRows ());
	 HTM_ARTICLE_Begin (Anchor);
	    HTM_Txt (AgdEvent.Event);
	 HTM_ARTICLE_End ();
      HTM_TD_End ();

      /* Location */
      HTM_TD_Begin ("class=\"LT %s\"",The_GetColorRows ());
	 HTM_DIV_Begin ("class=\"%s\"",AgdEvent.Hidden ? "ASG_TITLE_LIGHT" :
							 "ASG_TITLE");
	    HTM_Txt (AgdEvent.Location);
	 HTM_DIV_End ();
      HTM_TD_End ();

   HTM_TR_End ();

   /***** Write second row of data of this event *****/
   HTM_TR_Begin (NULL);

      HTM_TD_Begin ("colspan=\"2\" class=\"LT %s\"",The_GetColorRows ());
	 switch (AgendaType)
	   {
	    case Agd_MY_AGENDA_TODAY:
	    case Agd_MY_AGENDA:
	       /* Forms to remove/edit this event */
	       Agd_PutFormsToRemEditOneEvent (Agenda,&AgdEvent,Anchor);
	       break;
	    default:
	       break;
	   }
      HTM_TD_End ();

      /* Text of the event */
      HTM_TD_Begin ("colspan=\"2\" class=\"LT %s\"",The_GetColorRows ());
	 HTM_DIV_Begin ("class=\"PAR %s_%s\"",
	                AgdEvent.Hidden ? "DAT_LIGHT" :
	                	          "DAT",
	                The_Colors[Gbl.Prefs.Theme]);
	    Agd_DB_GetEventTxt (&AgdEvent,Txt);
	    Str_ChangeFormat (Str_FROM_HTML,Str_TO_RIGOROUS_HTML,
			      Txt,Cns_MAX_BYTES_TEXT,false);	// Convert from HTML to recpectful HTML
	    ALn_InsertLinks (Txt,Cns_MAX_BYTES_TEXT,60);	// Insert links
	    HTM_Txt (Txt);
	 HTM_DIV_End ();
      HTM_TD_End ();

   HTM_TR_End ();

   /***** Free anchor string *****/
   Frm_FreeAnchorStr (Anchor);

   The_ChangeRowColor ();
  }

/*****************************************************************************/
/******************* Put a link (form) to edit one event *********************/
/*****************************************************************************/

static void Agd_PutFormsToRemEditOneEvent (struct Agd_Agenda *Agenda,
                                           struct Agd_Event *AgdEvent,
                                           const char *Anchor)
  {
   extern const char *Txt_Event_private_click_to_make_it_visible_to_the_users_of_your_courses;
   extern const char *Txt_Event_visible_to_the_users_of_your_courses_click_to_make_it_private;

   Agenda->AgdCodToEdit = AgdEvent->AgdCod;	// Used as parameter in contextual links

   /***** Put form to remove event *****/
   Ico_PutContextualIconToRemove (ActReqRemEvtMyAgd,NULL,
                                  Agd_PutCurrentParamsMyAgenda,Agenda);

   /***** Put form to hide/show event *****/
   if (AgdEvent->Hidden)
      Ico_PutContextualIconToUnhide (ActShoEvtMyAgd,Anchor,
                                     Agd_PutCurrentParamsMyAgenda,Agenda);
   else
      Ico_PutContextualIconToHide (ActHidEvtMyAgd,Anchor,
                                   Agd_PutCurrentParamsMyAgenda,Agenda);

   /***** Put form to edit event *****/
   Ico_PutContextualIconToEdit (ActEdiOneEvtMyAgd,NULL,
                                Agd_PutCurrentParamsMyAgenda,Agenda);

   /***** Put form to make event public/private *****/
   if (AgdEvent->Public)
      Lay_PutContextualLinkOnlyIcon (ActPrvEvtMyAgd,NULL,
				     Agd_PutCurrentParamsMyAgenda,Agenda,
			             "unlock.svg",Ico_GREEN,
			             Txt_Event_visible_to_the_users_of_your_courses_click_to_make_it_private);
   else
      Lay_PutContextualLinkOnlyIcon (ActPubEvtMyAgd,NULL,
	                             Agd_PutCurrentParamsMyAgenda,Agenda,
			             "lock.svg",Ico_RED,
			             Txt_Event_private_click_to_make_it_visible_to_the_users_of_your_courses);
  }

/*****************************************************************************/
/****************** Parameters passed in my agenda forms *********************/
/*****************************************************************************/

static void Agd_PutCurrentParamsMyAgenda (void *Agenda)
  {
   if (Agenda)
      Agd_PutParamsMyAgenda (((struct Agd_Agenda *) Agenda)->Past__FutureEvents,
			     ((struct Agd_Agenda *) Agenda)->PrivatPublicEvents,
			     ((struct Agd_Agenda *) Agenda)->HiddenVisiblEvents,
			     ((struct Agd_Agenda *) Agenda)->SelectedOrder,
			     ((struct Agd_Agenda *) Agenda)->CurrentPage,
			     ((struct Agd_Agenda *) Agenda)->AgdCodToEdit);
  }

/* The following function is called
   when one or more parameters must be passed explicitely.
   Each parameter is passed only if its value is distinct to default. */

void Agd_PutParamsMyAgenda (unsigned Past__FutureEvents,
                            unsigned PrivatPublicEvents,
                            unsigned HiddenVisiblEvents,
			    Dat_StartEndTime_t Order,
                            unsigned NumPage,
                            long AgdCodToEdit)
  {
   if (Past__FutureEvents != (Agd_DEFAULT_PAST___EVENTS |
	                      Agd_DEFAULT_FUTURE_EVENTS))
      Agd_PutHiddenParamPast__FutureEvents (Past__FutureEvents);

   if (PrivatPublicEvents != (Agd_DEFAULT_PRIVAT_EVENTS |
	                      Agd_DEFAULT_PUBLIC_EVENTS))
      Agd_PutHiddenParamPrivatPublicEvents (PrivatPublicEvents);

   if (HiddenVisiblEvents != (Agd_DEFAULT_HIDDEN_EVENTS |
	                      Agd_DEFAULT_VISIBL_EVENTS))
      Agd_PutHiddenParamHiddenVisiblEvents (HiddenVisiblEvents);

   if (Order != Agd_ORDER_DEFAULT)
      Dat_PutHiddenParamOrder (Order);

   if (NumPage > 1)
      Pag_PutHiddenParamPagNum (Pag_MY_AGENDA,NumPage);

   if (AgdCodToEdit > 0)
      Par_PutHiddenParamLong (NULL,"AgdCod",AgdCodToEdit);
  }

/*****************************************************************************/
/********** Get parameter with the type or order in list of events ***********/
/*****************************************************************************/

static void Agd_GetParams (struct Agd_Agenda *Agenda,
                           Agd_AgendaType_t AgendaType)
  {
   static const Pag_WhatPaginate_t WhatPaginate[Agd_NUM_AGENDA_TYPES] =
     {
      [Agd_MY_AGENDA_TODAY     ] = Pag_MY_AGENDA,	// not used
      [Agd_MY_AGENDA           ] = Pag_MY_AGENDA,
      [Agd_ANOTHER_AGENDA_TODAY] = Pag_ANOTHER_AGENDA,	// not used
      [Agd_ANOTHER_AGENDA      ] = Pag_ANOTHER_AGENDA,
     };

   if (AgendaType == Agd_MY_AGENDA)
     {
      Agenda->Past__FutureEvents = Agd_GetParamsPast__FutureEvents ();
      Agenda->PrivatPublicEvents = Agd_GetParamsPrivatPublicEvents ();
      Agenda->HiddenVisiblEvents = Agd_GetParamsHiddenVisiblEvents ();
     }
   Agd_GetParamEventOrder (Agenda);
   Agenda->CurrentPage = Pag_GetParamPagNum (WhatPaginate[AgendaType]);
  }

/*****************************************************************************/
/****** Put a hidden parameter with the type of order in list of events ******/
/*****************************************************************************/

void Agd_PutHiddenParamEventsOrder (Dat_StartEndTime_t SelectedOrder)
  {
   if (SelectedOrder != Agd_ORDER_DEFAULT)
      Dat_PutHiddenParamOrder (SelectedOrder);
  }

/*****************************************************************************/
/********** Get parameter with the type or order in list of events ***********/
/*****************************************************************************/

static void Agd_GetParamEventOrder (struct Agd_Agenda *Agenda)
  {
   static bool AlreadyGot = false;

   if (!AlreadyGot)
     {
      Agenda->SelectedOrder = (Dat_StartEndTime_t)
			      Par_GetParToUnsignedLong ("Order",
							0,
							Dat_NUM_START_END_TIME - 1,
							(unsigned long) Agd_ORDER_DEFAULT);
      AlreadyGot = true;
     }
  }

/*****************************************************************************/
/************************* Get list of agenda events *************************/
/*****************************************************************************/

#define Agd_MAX_BYTES_SUBQUERY 128

static void Agd_GetListEvents (struct Agd_Agenda *Agenda,
                               Agd_AgendaType_t AgendaType)
  {
   MYSQL_RES *mysql_res;
   unsigned NumEvent;

   /***** Initialize list of events *****/
   Agd_FreeListEvents (Agenda);

   /***** Trivial check: anything to get from database? *****/
   switch (AgendaType)
     {
      case Agd_MY_AGENDA_TODAY:
      case Agd_MY_AGENDA:
	 if (Agenda->Past__FutureEvents == 0 ||
             Agenda->PrivatPublicEvents == 0 ||
             Agenda->HiddenVisiblEvents == 0)	// All selectors are off
	   {
	    // Nothing to get from database
	    Agenda->LstIsRead = true;
	    return;
	   }
	 break;
      default:
	 break;
     }

   /***** Get list of events from database *****/
   if ((Agenda->Num = Agd_DB_GetListEvents (&mysql_res,Agenda,AgendaType))) // Events found...
     {
      /***** Create list of events *****/
      if ((Agenda->LstAgdCods = calloc ((size_t) Agenda->Num,
					sizeof (*Agenda->LstAgdCods))) == NULL)
	 Err_NotEnoughMemoryExit ();

      /***** Get the events codes *****/
      for (NumEvent = 0;
	   NumEvent < Agenda->Num;
	   NumEvent++)
	 /* Get next event code */
	 if ((Agenda->LstAgdCods[NumEvent] = DB_GetNextCode (mysql_res)) < 0)
	    Err_WrongEventExit ();
     }

   /***** Free structure that stores the query result *****/
   DB_FreeMySQLResult (&mysql_res);

   Agenda->LstIsRead = true;
  }

/*****************************************************************************/
/*********************** Get event data using its code ***********************/
/*****************************************************************************/

static void Agd_GetDataOfEventByCod (struct Agd_Event *AgdEvent)
  {
   MYSQL_RES *mysql_res;
   MYSQL_ROW row;

   /***** Get data of event from database *****/
   if (Agd_DB_GetDataOfEventByCod (&mysql_res,AgdEvent))	// Event found...
     {
      /* Get row:
      row[0] AgdCod
      row[1] Public
      row[2] Hidden
      row[3] UNIX_TIMESTAMP(StartTime)
      row[4] UNIX_TIMESTAMP(EndTime)
      row[5] NOW()>EndTime	// Past event?
      row[6] NOW()<StartTime	// Future event?
      row[7] Event
      row[8] Location
      */
      row = mysql_fetch_row (mysql_res);

      /* Get code of the event (row[0]) */
      AgdEvent->AgdCod = Str_ConvertStrCodToLongCod (row[0]);

      /* Get whether the event is public or not (row[1])
         and whether it is hidden or not (row[2])  */
      AgdEvent->Public = (row[1][0] == 'Y');
      AgdEvent->Hidden = (row[2][0] == 'Y');

      /* Get start date (row[3]) and end date (row[4]) in UTC time */
      AgdEvent->TimeUTC[Dat_STR_TIME] = Dat_GetUNIXTimeFromStr (row[3]);
      AgdEvent->TimeUTC[Dat_END_TIME] = Dat_GetUNIXTimeFromStr (row[4]);

      /* Get whether the event is past, present or future (row(5), row[6]) */
      AgdEvent->TimeStatus = ((row[5][0] == '1') ? Dat_PAST :
	                     ((row[6][0] == '1') ? Dat_FUTURE :
	                	                   Dat_PRESENT));

      /* Get the event (row[7]) and its location (row[8]) */
      Str_Copy (AgdEvent->Event   ,row[7],sizeof (AgdEvent->Event   ) - 1);
      Str_Copy (AgdEvent->Location,row[8],sizeof (AgdEvent->Location) - 1);
     }
   else
     {
      /***** Clear all event data *****/
      AgdEvent->AgdCod                = -1L;
      AgdEvent->Public                = false;
      AgdEvent->Hidden                = false;
      AgdEvent->TimeUTC[Dat_STR_TIME] =
      AgdEvent->TimeUTC[Dat_END_TIME] = (time_t) 0;
      AgdEvent->TimeStatus            = Dat_FUTURE;
      AgdEvent->Event[0]              = '\0';
      AgdEvent->Location[0]           = '\0';
     }

   /***** Free structure that stores the query result *****/
   DB_FreeMySQLResult (&mysql_res);
  }

/*****************************************************************************/
/*************************** Free list of events *****************************/
/*****************************************************************************/

static void Agd_FreeListEvents (struct Agd_Agenda *Agenda)
  {
   if (Agenda->LstIsRead && Agenda->LstAgdCods)
     {
      /***** Free memory used by the list of events *****/
      free (Agenda->LstAgdCods);
      Agenda->LstAgdCods = NULL;
      Agenda->Num = 0;
      Agenda->LstIsRead = false;
     }
  }

/*****************************************************************************/
/******************** Get parameter with code of event ***********************/
/*****************************************************************************/

long Agd_GetParamAgdCod (void)
  {
   /***** Get code of event *****/
   return Par_GetParToLong ("AgdCod");
  }

/*****************************************************************************/
/************** Ask for confirmation of removing of an event *****************/
/*****************************************************************************/

void Agd_AskRemEvent (void)
  {
   extern const char *Txt_Do_you_really_want_to_remove_the_event_X;
   extern const char *Txt_Remove_event;
   struct Agd_Agenda Agenda;
   struct Agd_Event AgdEvent;

   /***** Reset agenda context *****/
   Agd_ResetAgenda (&Agenda);

   /***** Get parameters *****/
   Agd_GetParams (&Agenda,Agd_MY_AGENDA);

   /***** Get event code *****/
   if ((AgdEvent.AgdCod = Agd_GetParamAgdCod ()) < 0)
      Err_WrongEventExit ();

   /***** Get data of the event from database *****/
   AgdEvent.UsrCod = Gbl.Usrs.Me.UsrDat.UsrCod;
   Agd_GetDataOfEventByCod (&AgdEvent);

   /***** Show question and button to remove event *****/
   Agenda.AgdCodToEdit = AgdEvent.AgdCod;
   Ale_ShowAlertAndButton (ActRemEvtMyAgd,NULL,NULL,
                           Agd_PutCurrentParamsMyAgenda,&Agenda,
			   Btn_REMOVE_BUTTON,Txt_Remove_event,
			   Ale_QUESTION,Txt_Do_you_really_want_to_remove_the_event_X,
	                   AgdEvent.Event);

   /***** Show events again *****/
   Agd_ShowMyAgenda (&Agenda);
  }

/*****************************************************************************/
/****************************** Remove an event ******************************/
/*****************************************************************************/

void Agd_RemoveEvent (void)
  {
   extern const char *Txt_Event_X_removed;
   struct Agd_Agenda Agenda;
   struct Agd_Event AgdEvent;

   /***** Reset agenda context *****/
   Agd_ResetAgenda (&Agenda);

   /***** Get parameters *****/
   Agd_GetParams (&Agenda,Agd_MY_AGENDA);

   /***** Get event code *****/
   if ((AgdEvent.AgdCod = Agd_GetParamAgdCod ()) < 0)
      Err_WrongEventExit ();

   /***** Get data of the event from database *****/
   AgdEvent.UsrCod = Gbl.Usrs.Me.UsrDat.UsrCod;
   Agd_GetDataOfEventByCod (&AgdEvent);

   /***** Remove event *****/
   Agd_DB_RemoveEvent (&AgdEvent);

   /***** Write message to show the change made *****/
   Ale_ShowAlert (Ale_SUCCESS,Txt_Event_X_removed,
	          AgdEvent.Event);

   /***** Show events again *****/
   Agd_ShowMyAgenda (&Agenda);
  }

/*****************************************************************************/
/********************************* Hide event ********************************/
/*****************************************************************************/

void Agd_HideEvent (void)
  {
   struct Agd_Agenda Agenda;
   struct Agd_Event AgdEvent;

   /***** Reset agenda context *****/
   Agd_ResetAgenda (&Agenda);

   /***** Get parameters *****/
   Agd_GetParams (&Agenda,Agd_MY_AGENDA);

   /***** Get event code *****/
   if ((AgdEvent.AgdCod = Agd_GetParamAgdCod ()) < 0)
      Err_WrongEventExit ();

   /***** Get data of the event from database *****/
   AgdEvent.UsrCod = Gbl.Usrs.Me.UsrDat.UsrCod;
   Agd_GetDataOfEventByCod (&AgdEvent);

   /***** Hide event *****/
   Agd_DB_HideOrUnhideEvent (AgdEvent.AgdCod,AgdEvent.UsrCod,true);

   /***** Show events again *****/
   Agd_ShowMyAgenda (&Agenda);
  }

/*****************************************************************************/
/****************************** Unhide event *********************************/
/*****************************************************************************/

void Agd_UnhideEvent (void)
  {
   struct Agd_Agenda Agenda;
   struct Agd_Event AgdEvent;

   /***** Reset agenda context *****/
   Agd_ResetAgenda (&Agenda);

   /***** Get parameters *****/
   Agd_GetParams (&Agenda,Agd_MY_AGENDA);

   /***** Get event code *****/
   if ((AgdEvent.AgdCod = Agd_GetParamAgdCod ()) < 0)
      Err_WrongEventExit ();

   /***** Get data of the event from database *****/
   AgdEvent.UsrCod = Gbl.Usrs.Me.UsrDat.UsrCod;
   Agd_GetDataOfEventByCod (&AgdEvent);

   /***** Unhide event *****/
   Agd_DB_HideOrUnhideEvent (AgdEvent.AgdCod,AgdEvent.UsrCod,false);

   /***** Show events again *****/
   Agd_ShowMyAgenda (&Agenda);
  }

/*****************************************************************************/
/****************************** Make event private ***************************/
/*****************************************************************************/

void Agd_MakeEventPrivate (void)
  {
   extern const char *Txt_Event_X_is_now_private;
   struct Agd_Agenda Agenda;
   struct Agd_Event AgdEvent;

   /***** Reset agenda context *****/
   Agd_ResetAgenda (&Agenda);

   /***** Get parameters *****/
   Agd_GetParams (&Agenda,Agd_MY_AGENDA);

   /***** Get event code *****/
   if ((AgdEvent.AgdCod = Agd_GetParamAgdCod ()) < 0)
      Err_WrongEventExit ();

   /***** Get data of the event from database *****/
   AgdEvent.UsrCod = Gbl.Usrs.Me.UsrDat.UsrCod;
   Agd_GetDataOfEventByCod (&AgdEvent);

   /***** Make event private *****/
   Agd_DB_MakeEventPrivate (&AgdEvent);

   /***** Write message to show the change made *****/
   Ale_ShowAlert (Ale_SUCCESS,Txt_Event_X_is_now_private,
	          AgdEvent.Event);

   /***** Show events again *****/
   Agd_ShowMyAgenda (&Agenda);
  }

/*****************************************************************************/
/******** Make event public (make it visible to users of my courses) *********/
/*****************************************************************************/

void Agd_MakeEventPublic (void)
  {
   extern const char *Txt_Event_X_is_now_visible_to_users_of_your_courses;
   struct Agd_Agenda Agenda;
   struct Agd_Event AgdEvent;

   /***** Reset agenda context *****/
   Agd_ResetAgenda (&Agenda);

   /***** Get parameters *****/
   Agd_GetParams (&Agenda,Agd_MY_AGENDA);

   /***** Get event code *****/
   if ((AgdEvent.AgdCod = Agd_GetParamAgdCod ()) < 0)
      Err_WrongEventExit ();

   /***** Get data of the event from database *****/
   AgdEvent.UsrCod = Gbl.Usrs.Me.UsrDat.UsrCod;
   Agd_GetDataOfEventByCod (&AgdEvent);

   /***** Make event public *****/
   Agd_DB_MakeEventPublic (&AgdEvent);

   /***** Write message to show the change made *****/
   Ale_ShowAlert (Ale_SUCCESS,Txt_Event_X_is_now_visible_to_users_of_your_courses,
                  AgdEvent.Event);

   /***** Show events again *****/
   Agd_ShowMyAgenda (&Agenda);
  }

/*****************************************************************************/
/******************** Put a form to create a new event ***********************/
/*****************************************************************************/

void Agd_RequestCreatOrEditEvent (void)
  {
   extern const char *Hlp_PROFILE_Agenda_new_event;
   extern const char *Hlp_PROFILE_Agenda_edit_event;
   extern const char *The_ClassInput[The_NUM_THEMES];
   extern const char *Txt_New_event;
   extern const char *Txt_Edit_event;
   extern const char *Txt_Location;
   extern const char *Txt_Event;
   extern const char *Txt_Description;
   extern const char *Txt_Create_event;
   extern const char *Txt_Save_changes;
   struct Agd_Agenda Agenda;
   struct Agd_Event AgdEvent;
   bool ItsANewEvent;
   char Txt[Cns_MAX_BYTES_TEXT + 1];
   static const Dat_SetHMS SetHMS[Dat_NUM_START_END_TIME] =
     {
      [Dat_STR_TIME] = Dat_HMS_DO_NOT_SET,
      [Dat_END_TIME] = Dat_HMS_DO_NOT_SET
     };

   /***** Reset agenda context *****/
   Agd_ResetAgenda (&Agenda);

   /***** Get parameters *****/
   Agd_GetParams (&Agenda,Agd_MY_AGENDA);

   /***** Get the code of the event *****/
   ItsANewEvent = ((AgdEvent.AgdCod = Agd_GetParamAgdCod ()) <= 0);

   /***** Get from the database the data of the event *****/
   AgdEvent.UsrCod = Gbl.Usrs.Me.UsrDat.UsrCod;
   if (ItsANewEvent)
     {
      /* Initialize to empty event */
      AgdEvent.AgdCod = -1L;
      AgdEvent.TimeUTC[Dat_STR_TIME] = Gbl.StartExecutionTimeUTC;
      AgdEvent.TimeUTC[Dat_END_TIME] = Gbl.StartExecutionTimeUTC + (2 * 60 * 60);	// +2 hours
      AgdEvent.TimeStatus = Dat_FUTURE;
      AgdEvent.Event[0]    = '\0';
      AgdEvent.Location[0] = '\0';
     }
   else
     {
      /* Get data of the event from database */
      Agd_GetDataOfEventByCod (&AgdEvent);

      /* Get text of the event from database */
      Agd_DB_GetEventTxt (&AgdEvent,Txt);
     }

   /***** Begin form *****/
   if (ItsANewEvent)
     {
      Frm_BeginForm (ActNewEvtMyAgd);
      Agenda.AgdCodToEdit = -1L;
     }
   else
     {
      Frm_BeginForm (ActChgEvtMyAgd);
      Agenda.AgdCodToEdit = AgdEvent.AgdCod;
     }
   Agd_PutCurrentParamsMyAgenda (&Agenda);

      /***** Begin box and table *****/
      if (ItsANewEvent)
	 Box_BoxTableBegin (NULL,Txt_New_event,
			    NULL,NULL,
			    Hlp_PROFILE_Agenda_new_event,Box_NOT_CLOSABLE,2);
      else
	 Box_BoxTableBegin (NULL,Txt_Edit_event,
			    NULL,NULL,
			    Hlp_PROFILE_Agenda_edit_event,Box_NOT_CLOSABLE,2);

      /***** Event *****/
      /* Begin table row */
      HTM_TR_Begin (NULL);

	 /* Label */
	 Frm_LabelColumn ("RT","Event",Txt_Event);

	 /* Data */
	 HTM_TD_Begin ("class=\"LT\"");
	    HTM_INPUT_TEXT ("Event",Agd_MAX_CHARS_EVENT,AgdEvent.Event,
			    HTM_DONT_SUBMIT_ON_CHANGE,
			    "id=\"Event\" class=\"TITLE_DESCRIPTION_WIDTH %s\""
			    " required=\"required\"",
			    The_ClassInput[Gbl.Prefs.Theme]);
	 HTM_TD_End ();

      /* End table row */
      HTM_TR_End ();

      /***** Location *****/
      /* Begin table row */
      HTM_TR_Begin (NULL);

	 /* Label */
	 Frm_LabelColumn ("RT","Location",Txt_Location);

	 /* Data */
	 HTM_TD_Begin ("class=\"LT\"");
	    HTM_INPUT_TEXT ("Location",Agd_MAX_CHARS_LOCATION,AgdEvent.Location,
			    HTM_DONT_SUBMIT_ON_CHANGE,
			    "id=\"Location\" class=\"TITLE_DESCRIPTION_WIDTH %s\""
			    " required=\"required\"",
			    The_ClassInput[Gbl.Prefs.Theme]);
	 HTM_TD_End ();

      /* End table row */
      HTM_TR_End ();

      /***** Start and end dates *****/
      Dat_PutFormStartEndClientLocalDateTimes (AgdEvent.TimeUTC,
					       Dat_FORM_SECONDS_OFF,
					       SetHMS);

      /***** Text *****/
      /* Begin table row */
      HTM_TR_Begin (NULL);

	 /* Label */
	 Frm_LabelColumn ("RT","Txt",Txt_Description);

	 /* Data */
	 HTM_TD_Begin ("class=\"LT\"");
	    HTM_TEXTAREA_Begin ("id=\"Txt\" name=\"Txt\" rows=\"5\""
				" class=\"TITLE_DESCRIPTION_WIDTH %s\"",
				The_ClassInput[Gbl.Prefs.Theme]);
	       if (!ItsANewEvent)
		  HTM_Txt (Txt);
	    HTM_TEXTAREA_End ();
	 HTM_TD_End ();

      /* End table row */
      HTM_TR_End ();

      /***** End table, send button and end box *****/
      if (ItsANewEvent)
	 Box_BoxTableWithButtonEnd (Btn_CREATE_BUTTON,Txt_Create_event);
      else
	 Box_BoxTableWithButtonEnd (Btn_CONFIRM_BUTTON,Txt_Save_changes);

   /***** End form *****/
   Frm_EndForm ();

   /***** Show current events, if any *****/
   Agd_ShowMyAgenda (&Agenda);
  }

/*****************************************************************************/
/********************* Receive form to create a new event ********************/
/*****************************************************************************/

void Agd_ReceiveFormEvent (void)
  {
   extern const char *Txt_You_must_specify_the_title_of_the_event;
   extern const char *Txt_Created_new_event_X;
   extern const char *Txt_The_event_has_been_modified;
   struct Agd_Agenda Agenda;
   struct Agd_Event AgdEvent;
   bool ItsANewEvent;
   bool NewEventIsCorrect = true;
   char EventTxt[Cns_MAX_BYTES_TEXT + 1];

   /***** Reset agenda context *****/
   Agd_ResetAgenda (&Agenda);

   /***** Get parameters *****/
   Agd_GetParams (&Agenda,Agd_MY_AGENDA);

   /***** Set author of the event *****/
   AgdEvent.UsrCod = Gbl.Usrs.Me.UsrDat.UsrCod;

   /***** Get the code of the event *****/
   ItsANewEvent = ((AgdEvent.AgdCod = Agd_GetParamAgdCod ()) <= 0);

   /***** Get start/end date-times *****/
   AgdEvent.TimeUTC[Dat_STR_TIME] = Dat_GetTimeUTCFromForm ("StartTimeUTC");
   AgdEvent.TimeUTC[Dat_END_TIME] = Dat_GetTimeUTCFromForm ("EndTimeUTC"  );

   /***** Get event location *****/
   Par_GetParToText ("Location",AgdEvent.Location,Agd_MAX_BYTES_LOCATION);

   /***** Get event title *****/
   Par_GetParToText ("Event",AgdEvent.Event,Agd_MAX_BYTES_EVENT);

   /***** Get event description *****/
   Par_GetParToHTML ("Txt",EventTxt,Cns_MAX_BYTES_TEXT);	// Store in HTML format (not rigorous)

   /***** Adjust dates *****/
   if (AgdEvent.TimeUTC[Dat_STR_TIME] == 0)
      AgdEvent.TimeUTC[Dat_STR_TIME] = Gbl.StartExecutionTimeUTC;
   if (AgdEvent.TimeUTC[Dat_END_TIME] == 0)
      AgdEvent.TimeUTC[Dat_END_TIME] = AgdEvent.TimeUTC[Dat_STR_TIME] + 2 * 60 * 60;	// +2 hours

   /***** Check if event is correct *****/
   if (!AgdEvent.Location[0])	// If there is no event
     {
      NewEventIsCorrect = false;
      Ale_ShowAlert (Ale_WARNING,Txt_You_must_specify_the_title_of_the_event);
     }

   /***** Check if event is correct *****/
   if (!AgdEvent.Event[0])	// If there is no event
     {
      NewEventIsCorrect = false;
      Ale_ShowAlert (Ale_WARNING,Txt_You_must_specify_the_title_of_the_event);
     }

   /***** Create a new event or update an existing one *****/
   if (NewEventIsCorrect)
     {
      if (ItsANewEvent)
	{
         AgdEvent.AgdCod = Agd_DB_CreateEvent (&AgdEvent,EventTxt);	// Add new event to database

	 /***** Write success message *****/
	 Ale_ShowAlert (Ale_SUCCESS,Txt_Created_new_event_X,
		        AgdEvent.Event);
	}
      else
        {
	 Agd_DB_UpdateEvent (&AgdEvent,EventTxt);

	 /***** Write success message *****/
	 Ale_ShowAlert (Ale_SUCCESS,Txt_The_event_has_been_modified);
        }

      /* Free memory for list of selected groups */
      Grp_FreeListCodSelectedGrps ();

      /***** Show events again *****/
      Agd_ShowMyAgenda (&Agenda);
     }
   else
      // TODO: The form should be filled with partial data, now is always empty
      Agd_RequestCreatOrEditEvent ();
  }

/*****************************************************************************/
/************************** Show an agenda QR code ***************************/
/*****************************************************************************/

void Agd_PrintAgdQRCode (void)
  {
   extern const char *Txt_Where_s_USER;
   char *Title;

   /***** Begin box *****/
   if (asprintf (&Title,Txt_Where_s_USER,Gbl.Usrs.Me.UsrDat.FullName) < 0)
      Err_NotEnoughMemoryExit ();
   Box_BoxBegin (NULL,Title,NULL,NULL,NULL,Box_NOT_CLOSABLE);
   free (Title);

      /***** Print QR code ****/
      QR_PrintQRCode ();

   /***** End box *****/
   Box_BoxEnd ();
  }

/*****************************************************************************/
/********* Get and show number of users who have chosen a language ***********/
/*****************************************************************************/

void Agd_GetAndShowAgendasStats (void)
  {
   extern const char *Hlp_ANALYTICS_Figures_agendas;
   extern const char *The_ClassDat[The_NUM_THEMES];
   extern const char *Txt_FIGURE_TYPES[Fig_NUM_FIGURES];
   extern const char *Txt_Number_of_events;
   extern const char *Txt_Number_of_users;
   extern const char *Txt_PERCENT_of_users;
   extern const char *Txt_Number_of_events_per_user;
   unsigned NumEvents;
   unsigned NumUsrs;
   unsigned NumUsrsTotal;

   /***** Begin box and table *****/
   Box_BoxTableBegin (NULL,Txt_FIGURE_TYPES[Fig_AGENDAS],
                      NULL,NULL,
                      Hlp_ANALYTICS_Figures_agendas,Box_NOT_CLOSABLE,2);

      /***** Heading row *****/
      HTM_TR_Begin (NULL);
	 HTM_TH (Txt_Number_of_events         ,HTM_HEAD_RIGHT);
	 HTM_TH (Txt_Number_of_users          ,HTM_HEAD_RIGHT);
	 HTM_TH (Txt_PERCENT_of_users         ,HTM_HEAD_RIGHT);
	 HTM_TH (Txt_Number_of_events_per_user,HTM_HEAD_RIGHT);
      HTM_TR_End ();

      /***** Number of agenda events *****/
      NumEvents = Agd_DB_GetNumEvents (Gbl.Scope.Current);

      /***** Number of users with agenda events *****/
      NumUsrs = Agd_DB_GetNumUsrsWithEvents (Gbl.Scope.Current);

      /***** Get total number of users in current scope *****/
      NumUsrsTotal = Usr_GetTotalNumberOfUsers ();

      /***** Write number of users who have chosen each language *****/
      HTM_TR_Begin (NULL);

	 HTM_TD_Begin ("class=\"%s RM\"",The_ClassDat[Gbl.Prefs.Theme]);
	    HTM_Unsigned (NumEvents);
	 HTM_TD_End ();

	 HTM_TD_Begin ("class=\"%s RM\"",The_ClassDat[Gbl.Prefs.Theme]);
	    HTM_Unsigned (NumUsrs);
	 HTM_TD_End ();

	 HTM_TD_Begin ("class=\"%s RM\"",The_ClassDat[Gbl.Prefs.Theme]);
	    HTM_Percentage (NumUsrsTotal ? (double) NumUsrs * 100.0 /
					   (double) NumUsrsTotal :
					   0);
	 HTM_TD_End ();

	 HTM_TD_Begin ("class=\"%s RM\"",The_ClassDat[Gbl.Prefs.Theme]);
	    HTM_Double2Decimals (NumUsrs ? (double) NumEvents /
					   (double) NumUsrs :
					   0);
	 HTM_TD_End ();

      HTM_TR_End ();

   /***** End table and box *****/
   Box_BoxTableEnd ();
  }
