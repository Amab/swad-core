// swad_attendance.c: control of attendance

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

#define _GNU_SOURCE 		// For asprintf
#include <linux/limits.h>	// For PATH_MAX
#include <mysql/mysql.h>	// To access MySQL databases
#include <stddef.h>		// For NULL
#include <stdio.h>		// For asprintf
#include <stdlib.h>		// For calloc
#include <string.h>		// For string functions

#include "swad_attendance.h"
#include "swad_attendance_database.h"
#include "swad_autolink.h"
#include "swad_box.h"
#include "swad_database.h"
#include "swad_error.h"
#include "swad_form.h"
#include "swad_global.h"
#include "swad_group.h"
#include "swad_group_database.h"
#include "swad_hierarchy_level.h"
#include "swad_HTML.h"
#include "swad_ID.h"
#include "swad_pagination.h"
#include "swad_parameter.h"
#include "swad_photo.h"
#include "swad_program_database.h"
#include "swad_QR.h"
#include "swad_setting.h"

/*****************************************************************************/
/*************** External global variables from others modules ***************/
/*****************************************************************************/

extern struct Globals Gbl;

/*****************************************************************************/
/****************************** Private constants ****************************/
/*****************************************************************************/

#define Att_ATTENDANCE_TABLE_ID		"att_table"
#define Att_ATTENDANCE_DETAILS_ID	"att_details"

/*****************************************************************************/
/******************************** Private types ******************************/
/*****************************************************************************/

typedef enum
  {
   Att_VIEW_ONLY_ME,	// View only me
   Att_VIEW_SEL_USR,	// View selected users
   Att_PRNT_ONLY_ME,	// Print only me
   Att_PRNT_SEL_USR,	// Print selected users
  } Att_TypeOfView_t;

/*****************************************************************************/
/****************************** Private prototypes ***************************/
/*****************************************************************************/

static void Att_ResetEvents (struct Att_Events *Events);

static void Att_ShowAllAttEvents (struct Att_Events *Events);
static void Att_ParamsWhichGroupsToShow (void *Events);
static void Att_PutIconsInListOfAttEvents (void *Events);
static void Att_PutIconToCreateNewAttEvent (struct Att_Events *Events);
static void Att_PutButtonToCreateNewAttEvent (struct Att_Events *Events);
static void Att_PutParamsToCreateNewAttEvent (void *Events);
static void Att_PutParamsToListUsrsAttendance (void *Events);

static void Att_ShowOneAttEvent (struct Att_Events *Events,
                                 struct Att_Event *Event,
                                 bool ShowOnlyThisAttEventComplete);
static void Att_WriteAttEventAuthor (struct Att_Event *Event);
static Dat_StartEndTime_t Att_GetParamAttOrder (void);

static void Att_PutFormsToRemEditOneAttEvent (struct Att_Events *Events,
					      const struct Att_Event *Event,
                                              const char *Anchor);
static void Att_PutParams (void *Events);
static void Att_GetListAttEvents (struct Att_Events *Events,
                                  Att_OrderNewestOldest_t OrderNewestOldest);
static void Att_GetDataOfAttEventByCodAndCheckCrs (struct Att_Event *Event);
static void Att_ResetAttendanceEvent (struct Att_Event *Event);
static void Att_FreeListAttEvents (struct Att_Events *Events);

static void Att_PutParamSelectedAttCod (void *Events);
static void Att_PutParamAttCod (long AttCod);
static long Att_GetParamAttCod (void);

static void Att_ShowLstGrpsToEditAttEvent (long AttCod);
static void Att_CreateGroups (long AttCod);
static void Att_GetAndWriteNamesOfGrpsAssociatedToAttEvent (struct Att_Event *Event);

static void Att_ShowEvent (struct Att_Events *Events);
static void Att_PutIconsOneAtt (void *Events);

static void Att_ListAttOnlyMeAsStudent (struct Att_Event *Event);
static void Att_ListAttStudents (struct Att_Events *Events,
	                         struct Att_Event *Event);
static void Att_WriteRowUsrToCallTheRoll (unsigned NumUsr,
                                          struct UsrData *UsrDat,
                                          struct Att_Event *Event);
static void Att_PutLinkAttEvent (struct Att_Event *Event,
				 const char *Title,const char *Txt);
static void Att_PutParamsCodGrps (long AttCod);
static unsigned Att_GetNumUsrsFromAListWhoAreInAttEvent (long AttCod,
						         long LstSelectedUsrCods[],
						         unsigned NumUsrsInList);
static bool Att_CheckIfUsrIsPresentInAttEvent (long AttCod,long UsrCod);
static bool Att_CheckIfUsrIsPresentInAttEventAndGetComments (long AttCod,long UsrCod,
                                                             char CommentStd[Cns_MAX_BYTES_TEXT + 1],
                                                             char CommentTch[Cns_MAX_BYTES_TEXT + 1]);

static void Att_ReqListOrPrintUsrsAttendanceCrs (void *TypeOfView);
static void Att_ListOrPrintMyAttendanceCrs (Att_TypeOfView_t TypeOfView);
static void Att_GetUsrsAndListOrPrintAttendanceCrs (Att_TypeOfView_t TypeOfView);
static void Att_ListOrPrintUsrsAttendanceCrs (void *TypeOfView);

static void Att_GetListSelectedAttCods (struct Att_Events *Events);

static void Att_PutIconsMyAttList (void *Events);
static void Att_PutFormToPrintMyListParams (void *Events);
static void Att_PutIconsStdsAttList (void *Events);
static void Att_PutParamsToPrintStdsList (void *Events);

static void Att_PutButtonToShowDetails (const struct Att_Events *Events);
static void Att_ListEventsToSelect (const struct Att_Events *Events,
                                    Att_TypeOfView_t TypeOfView);
static void Att_PutIconToViewAttEvents (__attribute__((unused)) void *Args);
static void Att_PutIconToEditAttEvents (__attribute__((unused)) void *Args);
static void Att_ListUsrsAttendanceTable (const struct Att_Events *Events,
                                         Att_TypeOfView_t TypeOfView,
	                                 unsigned NumUsrsInList,
                                         long *LstSelectedUsrCods);
static void Att_WriteTableHeadSeveralAttEvents (const struct Att_Events *Events);
static void Att_WriteRowUsrSeveralAttEvents (const struct Att_Events *Events,
                                             unsigned NumUsr,struct UsrData *UsrDat);
static void Att_PutCheckOrCross (bool Present);
static void Att_ListStdsWithAttEventsDetails (const struct Att_Events *Events,
                                              unsigned NumUsrsInList,
                                              long *LstSelectedUsrCods);
static void Att_ListAttEventsForAStd (const struct Att_Events *Events,
                                      unsigned NumUsr,struct UsrData *UsrDat);

/*****************************************************************************/
/************************** Reset attendance events **************************/
/*****************************************************************************/

static void Att_ResetEvents (struct Att_Events *Events)
  {
   Events->LstIsRead          = false;		// List is not read from database
   Events->Num                = 0;		// Number of attendance events
   Events->Lst                = NULL;		// List of attendance events
   Events->SelectedOrder      = Att_ORDER_DEFAULT;
   Events->AttCod             = -1L;
   Events->ShowDetails        = false;
   Events->StrAttCodsSelected = NULL;
   Events->CurrentPage        = 0;
  }

/*****************************************************************************/
/************************ List all attendance events *************************/
/*****************************************************************************/

void Att_SeeAttEvents (void)
  {
   struct Att_Events Events;

   /***** Reset attendance events *****/
   Att_ResetEvents (&Events);

   /***** Get parameters *****/
   Events.SelectedOrder = Att_GetParamAttOrder ();
   Grp_GetParamWhichGroups ();
   Events.CurrentPage = Pag_GetParamPagNum (Pag_ATT_EVENTS);

   /***** Get list of attendance events *****/
   Att_GetListAttEvents (&Events,Att_NEWEST_FIRST);

   /***** Show all attendance events *****/
   Att_ShowAllAttEvents (&Events);
  }

/*****************************************************************************/
/************************* Show all attendance events ************************/
/*****************************************************************************/

static void Att_ShowAllAttEvents (struct Att_Events *Events)
  {
   extern const char *Hlp_USERS_Attendance;
   extern const char *Txt_Events;
   extern const char *Txt_START_END_TIME_HELP[Dat_NUM_START_END_TIME];
   extern const char *Txt_START_END_TIME[Dat_NUM_START_END_TIME];
   extern const char *Txt_Event;
   extern const char *Txt_ROLES_PLURAL_Abc[Rol_NUM_ROLES][Usr_NUM_SEXS];
   extern const char *Txt_No_events;
   struct Pagination Pagination;
   Dat_StartEndTime_t Order;
   Grp_WhichGroups_t WhichGroups;
   unsigned NumAttEvent;
   bool ICanEdit = (Gbl.Usrs.Me.Role.Logged == Rol_TCH ||
		    Gbl.Usrs.Me.Role.Logged == Rol_SYS_ADM);

   /***** Compute variables related to pagination *****/
   Pagination.NumItems = Events->Num;
   Pagination.CurrentPage = (int) Events->CurrentPage;
   Pag_CalculatePagination (&Pagination);
   Events->CurrentPage = (unsigned) Pagination.CurrentPage;

   /***** Begin box *****/
   Box_BoxBegin ("100%",Txt_Events,
                 Att_PutIconsInListOfAttEvents,Events,
		 Hlp_USERS_Attendance,Box_NOT_CLOSABLE);

      /***** Select whether show only my groups or all groups *****/
      if (Gbl.Crs.Grps.NumGrps)
	{
	 Set_BeginSettingsHead ();
	 Grp_ShowFormToSelWhichGrps (ActSeeAtt,
				     Att_ParamsWhichGroupsToShow,&Events);
	 Set_EndSettingsHead ();
	}

      /***** Write links to pages *****/
      Pag_WriteLinksToPagesCentered (Pag_ATT_EVENTS,&Pagination,
				     Events,-1L);

      if (Events->Num)
	{
	 /***** Begin table *****/
	 HTM_TABLE_BeginWideMarginPadding (2);

	    /***** Table head *****/
	    HTM_TR_Begin (NULL);

	       HTM_TH_Span (NULL,HTM_HEAD_CENTER,1,1,"CONTEXT_COL");	// Column for contextual icons
	       for (Order  = (Dat_StartEndTime_t) 0;
		    Order <= (Dat_StartEndTime_t) (Dat_NUM_START_END_TIME - 1);
		    Order++)
		 {
                  HTM_TH_Begin (HTM_HEAD_LEFT);

		     Frm_BeginForm (ActSeeAtt);
			WhichGroups = Grp_GetParamWhichGroups ();
			Grp_PutParamWhichGroups (&WhichGroups);
			Pag_PutHiddenParamPagNum (Pag_ATT_EVENTS,Events->CurrentPage);
			Dat_PutHiddenParamOrder (Order);

			HTM_BUTTON_Submit_Begin (Txt_START_END_TIME_HELP[Order],
			                         "class=\"BT_LINK\"");

			   if (Order == Events->SelectedOrder)
			      HTM_U_Begin ();
			   HTM_Txt (Txt_START_END_TIME[Order]);
			   if (Order == Events->SelectedOrder)
			      HTM_U_End ();

			HTM_BUTTON_End ();

		     Frm_EndForm ();

		  HTM_TH_End ();
		 }
	       HTM_TH (Txt_Event                                     ,HTM_HEAD_LEFT );
	       HTM_TH (Txt_ROLES_PLURAL_Abc[Rol_STD][Usr_SEX_UNKNOWN],HTM_HEAD_RIGHT);

	    HTM_TR_End ();

	    /***** Write all attendance events *****/
	    for (NumAttEvent  = Pagination.FirstItemVisible, The_ResetRowColor ();
		 NumAttEvent <= Pagination.LastItemVisible;
		 NumAttEvent++, The_ChangeRowColor ())
	       Att_ShowOneAttEvent (Events,
				    &Events->Lst[NumAttEvent - 1],
				    false);

	 /***** End table *****/
	 HTM_TABLE_End ();
	}
      else	// No events created
	 Ale_ShowAlert (Ale_INFO,Txt_No_events);

      /***** Write again links to pages *****/
      Pag_WriteLinksToPagesCentered (Pag_ATT_EVENTS,&Pagination,
				     Events,-1L);

      /***** Button to create a new attendance event *****/
      if (ICanEdit)
	 Att_PutButtonToCreateNewAttEvent (Events);

   /***** End box *****/
   Box_BoxEnd ();

   /***** Free list of attendance events *****/
   Att_FreeListAttEvents (Events);
  }

/*****************************************************************************/
/***************** Put params to select which groups to show *****************/
/*****************************************************************************/

static void Att_ParamsWhichGroupsToShow (void *Events)
  {
   if (Events)
     {
      Dat_PutHiddenParamOrder (((struct Att_Events *) Events)->SelectedOrder);
      Pag_PutHiddenParamPagNum (Pag_ATT_EVENTS,((struct Att_Events *) Events)->CurrentPage);
     }
  }

/*****************************************************************************/
/************* Put contextual icons in list of attendance events *************/
/*****************************************************************************/

static void Att_PutIconsInListOfAttEvents (void *Events)
  {
   bool ICanEdit;

   if (Events)
     {
      /***** Put icon to create a new attendance event *****/
      ICanEdit = (Gbl.Usrs.Me.Role.Logged == Rol_TCH ||
		  Gbl.Usrs.Me.Role.Logged == Rol_SYS_ADM);
      if (ICanEdit)
	 Att_PutIconToCreateNewAttEvent ((struct Att_Events *) Events);

      /***** Put icon to show attendance list *****/
      if (((struct Att_Events *) Events)->Num)
	 switch (Gbl.Usrs.Me.Role.Logged)
	   {
	    case Rol_STD:
	       Ico_PutContextualIconToShowAttendanceList (ActSeeLstMyAtt,
	                                                  NULL,NULL);
	       break;
	    case Rol_NET:
	    case Rol_TCH:
	    case Rol_SYS_ADM:
	       Ico_PutContextualIconToShowAttendanceList (ActReqLstUsrAtt,
							  Att_PutParamsToListUsrsAttendance,Events);
	       break;
	    default:
	       break;
	   }

      /***** Put icon to print my QR code *****/
      QR_PutLinkToPrintQRCode (ActPrnUsrQR,
			       Usr_PutParamMyUsrCodEncrypted,Gbl.Usrs.Me.UsrDat.EnUsrCod);
     }
  }

/*****************************************************************************/
/**************** Put icon to create a new attendance event ******************/
/*****************************************************************************/

static void Att_PutIconToCreateNewAttEvent (struct Att_Events *Events)
  {
   Ico_PutContextualIconToAdd (ActFrmNewAtt,NULL,
                               Att_PutParamsToCreateNewAttEvent,Events);
  }

/*****************************************************************************/
/**************** Put button to create a new attendance event ****************/
/*****************************************************************************/

static void Att_PutButtonToCreateNewAttEvent (struct Att_Events *Events)
  {
   extern const char *Txt_New_event;

   /***** Begin form *****/
   Frm_BeginForm (ActFrmNewAtt);
      Att_PutParamsToCreateNewAttEvent (Events);

      /***** Button to create new event *****/
      Btn_PutConfirmButton (Txt_New_event);

   /***** End form *****/
   Frm_EndForm ();
  }

/*****************************************************************************/
/************** Put parameters to create a new attendance event **************/
/*****************************************************************************/

static void Att_PutParamsToCreateNewAttEvent (void *Events)
  {
   Grp_WhichGroups_t WhichGroups;

   if (Events)
     {
      Dat_PutHiddenParamOrder (((struct Att_Events *) Events)->SelectedOrder);
      WhichGroups = Grp_GetParamWhichGroups ();
      Grp_PutParamWhichGroups (&WhichGroups);
      Pag_PutHiddenParamPagNum (Pag_ATT_EVENTS,((struct Att_Events *) Events)->CurrentPage);
     }
  }

/*****************************************************************************/
/***************** Put parameters to list users attendance *******************/
/*****************************************************************************/

static void Att_PutParamsToListUsrsAttendance (void *Events)
  {
   Grp_WhichGroups_t WhichGroups;

   if (Events)
     {
      Dat_PutHiddenParamOrder (((struct Att_Events *) Events)->SelectedOrder);
      WhichGroups = Grp_GetParamWhichGroups ();
      Grp_PutParamWhichGroups (&WhichGroups);
      Pag_PutHiddenParamPagNum (Pag_ATT_EVENTS,((struct Att_Events *) Events)->CurrentPage);
     }
  }

/*****************************************************************************/
/************************* Show one attendance event *************************/
/*****************************************************************************/
// Only Event->AttCod must be filled

static void Att_ShowOneAttEvent (struct Att_Events *Events,
                                 struct Att_Event *Event,
                                 bool ShowOnlyThisAttEventComplete)
  {
   extern const char *Txt_View_event;
   char *Anchor = NULL;
   static unsigned UniqueId = 0;
   char *Id;
   Dat_StartEndTime_t StartEndTime;
   char Description[Cns_MAX_BYTES_TEXT + 1];

   /***** Get data of this attendance event *****/
   Att_GetDataOfAttEventByCodAndCheckCrs (Event);
   Event->NumStdsTotal = Att_DB_GetNumStdsTotalWhoAreInAttEvent (Event->AttCod);

   /***** Set anchor string *****/
   Frm_SetAnchorStr (Event->AttCod,&Anchor);

   /***** Write first row of data of this attendance event *****/
   /* Forms to remove/edit this attendance event */
   HTM_TR_Begin (NULL);

      if (ShowOnlyThisAttEventComplete)
	 HTM_TD_Begin ("rowspan=\"2\" class=\"CONTEXT_COL\"");
      else
	 HTM_TD_Begin ("rowspan=\"2\" class=\"CONTEXT_COL %s\"",
	               The_GetColorRows ());
      switch (Gbl.Usrs.Me.Role.Logged)
	{
	 case Rol_TCH:
	 case Rol_SYS_ADM:
	    Att_PutFormsToRemEditOneAttEvent (Events,Event,Anchor);
	    break;
	 default:
	    break;
	}
      HTM_TD_End ();

      /* Start/end date/time */
      UniqueId++;
      for (StartEndTime  = (Dat_StartEndTime_t) 0;
	   StartEndTime <= (Dat_StartEndTime_t) (Dat_NUM_START_END_TIME - 1);
	   StartEndTime++)
	{
	 if (asprintf (&Id,"att_date_%u_%u",(unsigned) StartEndTime,UniqueId) < 0)
	    Err_NotEnoughMemoryExit ();
	 if (ShowOnlyThisAttEventComplete)
	    HTM_TD_Begin ("id=\"%s\" class=\"LB %s_%s\"",
			  Id,
			  Event->Hidden ? (Event->Open ? "DATE_GREEN_LIGHT" :
							 "DATE_RED_LIGHT") :
					  (Event->Open ? "DATE_GREEN" :
							 "DATE_RED"),
			  The_GetSuffix ());
	 else
	    HTM_TD_Begin ("id=\"%s\" class=\"LB %s_%s %s\"",
			  Id,
			  Event->Hidden ? (Event->Open ? "DATE_GREEN_LIGHT" :
							 "DATE_RED_LIGHT") :
					  (Event->Open ? "DATE_GREEN" :
							 "DATE_RED"),
			  The_GetSuffix (),
			  The_GetColorRows ());
	 Dat_WriteLocalDateHMSFromUTC (Id,Event->TimeUTC[StartEndTime],
				       Gbl.Prefs.DateFormat,Dat_SEPARATOR_BREAK,
				       true,true,true,0x7);
	 HTM_TD_End ();
	 free (Id);
	}

      /* Attendance event title */
      if (ShowOnlyThisAttEventComplete)
	 HTM_TD_Begin ("class=\"LT\"");
      else
	 HTM_TD_Begin ("class=\"LT %s\"",The_GetColorRows ());
      HTM_ARTICLE_Begin (Anchor);
	 Att_PutLinkAttEvent (Event,Txt_View_event,Event->Title);
      HTM_ARTICLE_End ();
      HTM_TD_End ();

      /* Number of students in this event */
      if (ShowOnlyThisAttEventComplete)
	 HTM_TD_Begin ("class=\"RT\"");
      else
	 HTM_TD_Begin ("class=\"RT %s\"",The_GetColorRows ());
      HTM_SPAN_Begin ("class=\"%s_%s\"",
		       Event->Hidden ? "ASG_TITLE_LIGHT" :
				       "ASG_TITLE",
		       The_GetSuffix ());
         HTM_Unsigned (Event->NumStdsTotal);
      HTM_SPAN_End ();
      HTM_TD_End ();

   HTM_TR_End ();

   /***** Write second row of data of this attendance event *****/
   HTM_TR_Begin (NULL);

      /* Author of the attendance event */
      if (ShowOnlyThisAttEventComplete)
	 HTM_TD_Begin ("colspan=\"2\" class=\"LT\"");
      else
	 HTM_TD_Begin ("colspan=\"2\" class=\"LT %s\"",The_GetColorRows ());
      Att_WriteAttEventAuthor (Event);
      HTM_TD_End ();

      /* Text of the attendance event */
      Att_DB_GetAttEventDescription (Event->AttCod,Description);
      Str_ChangeFormat (Str_FROM_HTML,Str_TO_RIGOROUS_HTML,
			Description,Cns_MAX_BYTES_TEXT,false);	// Convert from HTML to recpectful HTML
      ALn_InsertLinks (Description,Cns_MAX_BYTES_TEXT,60);	// Insert links
      if (ShowOnlyThisAttEventComplete)
	 HTM_TD_Begin ("colspan=\"2\" class=\"LT\"");
      else
	 HTM_TD_Begin ("colspan=\"2\" class=\"LT %s\"",The_GetColorRows ());
      if (Gbl.Crs.Grps.NumGrps)
	 Att_GetAndWriteNamesOfGrpsAssociatedToAttEvent (Event);

      HTM_DIV_Begin ("class=\"%s_%s\"",
                     Event->Hidden ? "DAT_LIGHT" :
				     "DAT",
		     The_GetSuffix ());
	 HTM_Txt (Description);
      HTM_DIV_End ();

      HTM_TD_End ();

   HTM_TR_End ();

   /***** Free anchor string *****/
   Frm_FreeAnchorStr (Anchor);
  }

/*****************************************************************************/
/****************** Write the author of an attendance event ******************/
/*****************************************************************************/

static void Att_WriteAttEventAuthor (struct Att_Event *Event)
  {
   Usr_WriteAuthor1Line (Event->UsrCod,Event->Hidden);
  }

/*****************************************************************************/
/**** Get parameter with the type or order in list of attendance events ******/
/*****************************************************************************/

static Dat_StartEndTime_t Att_GetParamAttOrder (void)
  {
   return (Dat_StartEndTime_t)
	  Par_GetParToUnsignedLong ("Order",
				    0,
				    Dat_NUM_START_END_TIME - 1,
				    (unsigned long) Att_ORDER_DEFAULT);
  }

/*****************************************************************************/
/************** Put a link (form) to edit one attendance event ***************/
/*****************************************************************************/

static void Att_PutFormsToRemEditOneAttEvent (struct Att_Events *Events,
					      const struct Att_Event *Event,
                                              const char *Anchor)
  {
   static Act_Action_t ActionHideUnhide[2] =
     {
      [false] = ActHidAtt,	// Visible ==> action to hide
      [true ] = ActUnhAtt,	// Hidden ==> action to unhide
     };

   Events->AttCod = Event->AttCod;

   /***** Icon to remove attendance event *****/
   Ico_PutContextualIconToRemove (ActReqRemAtt,NULL,
                                  Att_PutParams,Events);

   /***** Icon to hide/unhide attendance event *****/
   Ico_PutContextualIconToHideUnhide (ActionHideUnhide,Anchor,
				      Att_PutParams,Events,
				      Event->Hidden);

   /***** Icon to edit attendance event *****/
   Ico_PutContextualIconToEdit (ActEdiOneAtt,NULL,
                                Att_PutParams,Events);
  }

/*****************************************************************************/
/***************** Params used to edit an attendance event *******************/
/*****************************************************************************/

static void Att_PutParams (void *Events)
  {
   Grp_WhichGroups_t WhichGroups;

   if (Events)
     {
      Att_PutParamAttCod (((struct Att_Events *) Events)->AttCod);
      Dat_PutHiddenParamOrder (((struct Att_Events *) Events)->SelectedOrder);
      WhichGroups = Grp_GetParamWhichGroups ();
      Grp_PutParamWhichGroups (&WhichGroups);
      Pag_PutHiddenParamPagNum (Pag_ATT_EVENTS,((struct Att_Events *) Events)->CurrentPage);
     }
  }

/*****************************************************************************/
/************************ List all attendance events *************************/
/*****************************************************************************/

static void Att_GetListAttEvents (struct Att_Events *Events,
                                  Att_OrderNewestOldest_t OrderNewestOldest)
  {
   extern unsigned (*Att_DB_GetListAttEvents[Grp_NUM_WHICH_GROUPS]) (MYSQL_RES **mysql_res,
							             Dat_StartEndTime_t SelectedOrder,
							             Att_OrderNewestOldest_t OrderNewestOldest);
   MYSQL_RES *mysql_res;
   unsigned NumAttEvent;

   if (Events->LstIsRead)
      Att_FreeListAttEvents (Events);

   /***** Get list of attendance events from database *****/
   Events->Num = Att_DB_GetListAttEvents[Gbl.Crs.Grps.WhichGrps] (&mysql_res,
	                                                          Events->SelectedOrder,
	                                                          OrderNewestOldest);
   if (Events->Num) // Attendance events found...
     {
      /***** Create list of attendance events *****/
      if ((Events->Lst = calloc ((size_t) Events->Num,
                                 sizeof (*Events->Lst))) == NULL)
         Err_NotEnoughMemoryExit ();

      /***** Get the attendance events codes *****/
      for (NumAttEvent = 0;
	   NumAttEvent < Events->Num;
	   NumAttEvent++)
        {
         /* Get next attendance event code */
         if ((Events->Lst[NumAttEvent].AttCod = DB_GetNextCode (mysql_res)) < 0)
            Err_WrongEventExit ();
        }
     }

   /***** Free structure that stores the query result *****/
   DB_FreeMySQLResult (&mysql_res);

   Events->LstIsRead = true;
  }

/*****************************************************************************/
/********* Get attendance event data using its code and check course *********/
/*****************************************************************************/

static void Att_GetDataOfAttEventByCodAndCheckCrs (struct Att_Event *Event)
  {
   if (Att_GetDataOfAttEventByCod (Event))
     {
      if (Event->CrsCod != Gbl.Hierarchy.Crs.CrsCod)
         Err_WrongEventExit ();
     }
   else	// Attendance event not found
      Err_WrongEventExit ();
  }

/*****************************************************************************/
/**************** Get attendance event data using its code *******************/
/*****************************************************************************/
// Returns true if attendance event exists
// This function can be called from web service, so do not display messages

bool Att_GetDataOfAttEventByCod (struct Att_Event *Event)
  {
   MYSQL_RES *mysql_res;
   MYSQL_ROW row;
   unsigned NumAttEvents;
   bool Found = false;

   /***** Reset attendance event data *****/
   Att_ResetAttendanceEvent (Event);

   if (Event->AttCod > 0)
     {
      /***** Build query *****/
      NumAttEvents = Att_DB_GetDataOfAttEventByCod (&mysql_res,Event->AttCod);

      /***** Get data of attendance event from database *****/
      if ((Found = (NumAttEvents != 0))) // Attendance event found...
	{
	 /* Get row */
	 row = mysql_fetch_row (mysql_res);

	 /* Get code of attendance event (row[0]) and code of course (row[1]) */
	 Event->AttCod = Str_ConvertStrCodToLongCod (row[0]);
	 Event->CrsCod = Str_ConvertStrCodToLongCod (row[1]);

	 /* Get whether the attendance event is hidden or not (row[2]) */
	 Event->Hidden = (row[2][0] == 'Y');

	 /* Get author of the attendance event (row[3]) */
	 Event->UsrCod = Str_ConvertStrCodToLongCod (row[3]);

	 /* Get start date (row[4]) and end date (row[5]) in UTC time */
	 Event->TimeUTC[Dat_STR_TIME] = Dat_GetUNIXTimeFromStr (row[4]);
	 Event->TimeUTC[Dat_END_TIME] = Dat_GetUNIXTimeFromStr (row[5]);

	 /* Get whether the attendance event is open or closed (row(6)) */
	 Event->Open = (row[6][0] == '1');

	 /* Get whether the attendance event is visible or not (row[7]) */
	 Event->CommentTchVisible = (row[7][0] == 'Y');

	 /* Get the title of the attendance event (row[8]) */
	 Str_Copy (Event->Title,row[8],sizeof (Event->Title) - 1);
	}

      /***** Free structure that stores the query result *****/
      DB_FreeMySQLResult (&mysql_res);
     }

   return Found;
  }

/*****************************************************************************/
/********************** Clear all attendance event data **********************/
/*****************************************************************************/

static void Att_ResetAttendanceEvent (struct Att_Event *Event)
  {
   if (Event->AttCod <= 0)	// If > 0 ==> keep values of AttCod and Selected
     {
      Event->AttCod = -1L;
      Event->NumStdsTotal = 0;
      Event->NumStdsFromList = 0;
      Event->Selected = false;
     }
   Event->CrsCod = -1L;
   Event->Hidden = false;
   Event->UsrCod = -1L;
   Event->TimeUTC[Dat_STR_TIME] =
   Event->TimeUTC[Dat_END_TIME] = (time_t) 0;
   Event->Open = false;
   Event->Title[0] = '\0';
   Event->CommentTchVisible = false;
  }

/*****************************************************************************/
/********************** Free list of attendance events ***********************/
/*****************************************************************************/

static void Att_FreeListAttEvents (struct Att_Events *Events)
  {
   if (Events->LstIsRead && Events->Lst)
     {
      /***** Free memory used by the list of attendance events *****/
      free (Events->Lst);
      Events->Lst       = NULL;
      Events->Num       = 0;
      Events->LstIsRead = false;
     }
  }

/*****************************************************************************/
/************** Write parameter with code of attendance event ****************/
/*****************************************************************************/

static void Att_PutParamSelectedAttCod (void *Events)
  {
   if (Events)
      Att_PutParamAttCod (((struct Att_Events *) Events)->AttCod);
  }

static void Att_PutParamAttCod (long AttCod)
  {
   Par_PutHiddenParamLong (NULL,"AttCod",AttCod);
  }

/*****************************************************************************/
/*************** Get parameter with code of attendance event *****************/
/*****************************************************************************/

static long Att_GetParamAttCod (void)
  {
   /***** Get code of attendance event *****/
   return Par_GetParToLong ("AttCod");
  }

/*****************************************************************************/
/********* Ask for confirmation of removing of an attendance event ***********/
/*****************************************************************************/

void Att_AskRemAttEvent (void)
  {
   extern const char *Txt_Do_you_really_want_to_remove_the_event_X;
   extern const char *Txt_Remove_event;
   struct Att_Events Events;
   struct Att_Event Event;
   Grp_WhichGroups_t WhichGroups;

   /***** Reset attendance events *****/
   Att_ResetEvents (&Events);

   /***** Get parameters *****/
   Events.SelectedOrder = Att_GetParamAttOrder ();
   Grp_GetParamWhichGroups ();
   Events.CurrentPage = Pag_GetParamPagNum (Pag_ATT_EVENTS);

   /***** Get attendance event code *****/
   if ((Event.AttCod = Att_GetParamAttCod ()) < 0)
      Err_WrongEventExit ();

   /***** Get data of the attendance event from database *****/
   Att_GetDataOfAttEventByCodAndCheckCrs (&Event);

   /***** Button of confirmation of removing *****/
   Frm_BeginForm (ActRemAtt);
      Att_PutParamAttCod (Event.AttCod);
      Dat_PutHiddenParamOrder (Events.SelectedOrder);
      WhichGroups = Grp_GetParamWhichGroups ();
      Grp_PutParamWhichGroups (&WhichGroups);
      Pag_PutHiddenParamPagNum (Pag_ATT_EVENTS,Events.CurrentPage);

      /* Ask for confirmation of removing */
      Ale_ShowAlert (Ale_WARNING,Txt_Do_you_really_want_to_remove_the_event_X,
		     Event.Title);

      Btn_PutRemoveButton (Txt_Remove_event);
   Frm_EndForm ();

   /***** Show attendance events again *****/
   Att_SeeAttEvents ();
  }

/*****************************************************************************/
/** Get param., remove an attendance event and show attendance events again **/
/*****************************************************************************/

void Att_GetAndRemAttEvent (void)
  {
   extern const char *Txt_Event_X_removed;
   struct Att_Event Event;

   /***** Get attendance event code *****/
   if ((Event.AttCod = Att_GetParamAttCod ()) < 0)
      Err_WrongEventExit ();

   /***** Get data of the attendance event from database *****/
   // Inside this function, the course is checked to be the current one
   Att_GetDataOfAttEventByCodAndCheckCrs (&Event);

   /***** Remove the attendance event from database *****/
   Att_RemoveAttEventFromDB (Event.AttCod);

   /***** Write message to show the change made *****/
   Ale_ShowAlert (Ale_SUCCESS,Txt_Event_X_removed,
	          Event.Title);

   /***** Show attendance events again *****/
   Att_SeeAttEvents ();
  }

/*****************************************************************************/
/**************** Remove an attendance event from database *******************/
/*****************************************************************************/

void Att_RemoveAttEventFromDB (long AttCod)
  {
   /***** Remove users registered in the attendance event *****/
   Att_DB_RemoveAllUsrsFromAnAttEvent (AttCod);

   /***** Remove all groups of this attendance event *****/
   Att_DB_RemoveGrpsAssociatedToAnAttEvent (AttCod);

   /***** Remove attendance event *****/
   Att_DB_RemoveAttEventFromCurrentCrs (AttCod);
  }

/*****************************************************************************/
/************************* Hide an attendance event **************************/
/*****************************************************************************/

void Att_HideAttEvent (void)
  {
   struct Att_Event Event;

   /***** Get attendance event code *****/
   if ((Event.AttCod = Att_GetParamAttCod ()) < 0)
      Err_WrongEventExit ();

   /***** Get data of the attendance event from database *****/
   Att_GetDataOfAttEventByCodAndCheckCrs (&Event);

   /***** Hide attendance event *****/
   Att_DB_HideOrUnhideAttEvent (Event.AttCod,true);

   /***** Show attendance events again *****/
   Att_SeeAttEvents ();
  }

/*****************************************************************************/
/************************ Unhide an attendance event *************************/
/*****************************************************************************/

void Att_UnhideAttEvent (void)
  {
   struct Att_Event Event;

   /***** Get attendance event code *****/
   if ((Event.AttCod = Att_GetParamAttCod ()) < 0)
      Err_WrongEventExit ();

   /***** Get data of the attendance event from database *****/
   Att_GetDataOfAttEventByCodAndCheckCrs (&Event);

   /***** Unhide attendance event *****/
   Att_DB_HideOrUnhideAttEvent (Event.AttCod,false);

   /***** Show attendance events again *****/
   Att_SeeAttEvents ();
  }

/*****************************************************************************/
/*************** Put a form to create a new attendance event *****************/
/*****************************************************************************/

void Att_RequestCreatOrEditAttEvent (void)
  {
   extern const char *Hlp_USERS_Attendance_new_event;
   extern const char *Hlp_USERS_Attendance_edit_event;
   extern const char *Txt_New_event;
   extern const char *Txt_Edit_event;
   extern const char *Txt_Teachers_comment;
   extern const char *Txt_Title;
   extern const char *Txt_Hidden_MALE_PLURAL;
   extern const char *Txt_Visible_MALE_PLURAL;
   extern const char *Txt_Description;
   extern const char *Txt_Create_event;
   extern const char *Txt_Save_changes;
   struct Att_Events Events;
   struct Att_Event Event;
   bool ItsANewAttEvent;
   Grp_WhichGroups_t WhichGroups;
   char Description[Cns_MAX_BYTES_TEXT + 1];
   static const Dat_SetHMS SetHMS[Dat_NUM_START_END_TIME] =
     {
      [Dat_STR_TIME] = Dat_HMS_DO_NOT_SET,
      [Dat_END_TIME] = Dat_HMS_DO_NOT_SET
     };

   /***** Reset attendance events *****/
   Att_ResetEvents (&Events);

   /***** Get parameters *****/
   Events.SelectedOrder = Att_GetParamAttOrder ();
   Grp_GetParamWhichGroups ();
   Events.CurrentPage = Pag_GetParamPagNum (Pag_ATT_EVENTS);

   /***** Get the code of the attendance event *****/
   Event.AttCod = Att_GetParamAttCod ();
   ItsANewAttEvent = (Event.AttCod <= 0);

   /***** Get from the database the data of the attendance event *****/
   if (ItsANewAttEvent)
     {
      /* Reset attendance event data */
      Event.AttCod = -1L;
      Att_ResetAttendanceEvent (&Event);

      /* Initialize some fields */
      Event.CrsCod = Gbl.Hierarchy.Crs.CrsCod;
      Event.UsrCod = Gbl.Usrs.Me.UsrDat.UsrCod;
      Event.TimeUTC[Dat_STR_TIME] = Gbl.StartExecutionTimeUTC;
      Event.TimeUTC[Dat_END_TIME] = Gbl.StartExecutionTimeUTC + (2 * 60 * 60);	// +2 hours
      Event.Open = true;
     }
   else
     {
      /* Get data of the attendance event from database */
      Att_GetDataOfAttEventByCodAndCheckCrs (&Event);

      /* Get text of the attendance event from database */
      Att_DB_GetAttEventDescription (Event.AttCod,Description);
     }

   /***** Begin form *****/
   if (ItsANewAttEvent)
      Frm_BeginForm (ActNewAtt);
   else
     {
      Frm_BeginForm (ActChgAtt);
	 Att_PutParamAttCod (Event.AttCod);
     }
      Dat_PutHiddenParamOrder (Events.SelectedOrder);
      WhichGroups = Grp_GetParamWhichGroups ();
      Grp_PutParamWhichGroups (&WhichGroups);
      Pag_PutHiddenParamPagNum (Pag_ATT_EVENTS,Events.CurrentPage);

      /***** Begin box and table *****/
      if (ItsANewAttEvent)
	 Box_BoxTableBegin (NULL,Txt_New_event,
			    NULL,NULL,
			    Hlp_USERS_Attendance_new_event,Box_NOT_CLOSABLE,2);
      else
	 Box_BoxTableBegin (NULL,
			    Event.Title[0] ? Event.Title :
					     Txt_Edit_event,
			    NULL,NULL,
			    Hlp_USERS_Attendance_edit_event,Box_NOT_CLOSABLE,2);

      /***** Attendance event title *****/
      HTM_TR_Begin (NULL);

	 /* Label */
	 Frm_LabelColumn ("RT","Title",Txt_Title);

	 /* Data */
	 HTM_TD_Begin ("class=\"LT\"");
	    HTM_INPUT_TEXT ("Title",Att_MAX_CHARS_ATTENDANCE_EVENT_TITLE,Event.Title,
			    HTM_DONT_SUBMIT_ON_CHANGE,
			    "id=\"Title\" class=\"TITLE_DESCRIPTION_WIDTH INPUT_%s\""
			    " required=\"required\"",
			    The_GetSuffix ());
	 HTM_TD_End ();

      HTM_TR_End ();

      /***** Assignment start and end dates *****/
      Dat_PutFormStartEndClientLocalDateTimes (Event.TimeUTC,
					       Dat_FORM_SECONDS_ON,
					       SetHMS);

      /***** Visibility of comments *****/
      HTM_TR_Begin (NULL);

	 /* Label */
	 Frm_LabelColumn ("RT","ComTchVisible",Txt_Teachers_comment);

	 /* Data */
	 HTM_TD_Begin ("class=\"LT\"");
	    HTM_SELECT_Begin (HTM_DONT_SUBMIT_ON_CHANGE,
			      "id=\"ComTchVisible\" name=\"ComTchVisible\""
			      " class=\"INPUT_%s\"",
			      The_GetSuffix ());
	       HTM_OPTION (HTM_Type_STRING,"N",!Event.CommentTchVisible,false,
			   "%s",Txt_Hidden_MALE_PLURAL);
	       HTM_OPTION (HTM_Type_STRING,"Y",Event.CommentTchVisible,false,
			   "%s",Txt_Visible_MALE_PLURAL);
	    HTM_SELECT_End ();
	 HTM_TD_End ();

      HTM_TR_End ();

      /***** Attendance event description *****/
      HTM_TR_Begin (NULL);

	 /* Label */
	 Frm_LabelColumn ("RT","Txt",Txt_Description);

	 /* Data */
	 HTM_TD_Begin ("class=\"LT\"");
	    HTM_TEXTAREA_Begin ("id=\"Txt\" name=\"Txt\" rows=\"5\""
				" class=\"TITLE_DESCRIPTION_WIDTH INPUT_%s\"",
				The_GetSuffix ());
	       if (!ItsANewAttEvent)
		  HTM_Txt (Description);
	    HTM_TEXTAREA_End ();
	 HTM_TD_End ();

      HTM_TR_End ();

      /***** Groups *****/
      Att_ShowLstGrpsToEditAttEvent (Event.AttCod);

      /***** End table, send button and end box *****/
      if (ItsANewAttEvent)
	 Box_BoxTableWithButtonEnd (Btn_CREATE_BUTTON,Txt_Create_event);
      else
	 Box_BoxTableWithButtonEnd (Btn_CONFIRM_BUTTON,Txt_Save_changes);

   /***** End form *****/
   Frm_EndForm ();

   /***** Show current attendance events *****/
   Att_GetListAttEvents (&Events,Att_NEWEST_FIRST);
   Att_ShowAllAttEvents (&Events);
  }

/*****************************************************************************/
/************* Show list of groups to edit and attendance event **************/
/*****************************************************************************/

static void Att_ShowLstGrpsToEditAttEvent (long AttCod)
  {
   extern const char *Txt_Groups;
   extern const char *Txt_The_whole_course;
   unsigned NumGrpTyp;

   /***** Get list of groups types and groups in this course *****/
   Grp_GetListGrpTypesAndGrpsInThisCrs (Grp_ONLY_GROUP_TYPES_WITH_GROUPS);

   if (Gbl.Crs.Grps.GrpTypes.NumGrpTypes)
     {
      /***** Begin box and table *****/
      HTM_TR_Begin (NULL);

	 HTM_TD_Begin ("class=\"RT FORM_IN_%s\"",The_GetSuffix ());
	    HTM_TxtColon (Txt_Groups);
	 HTM_TD_End ();

	 HTM_TD_Begin ("class=\"LT\"");
	    Box_BoxTableBegin ("100%",NULL,
			       NULL,NULL,
			       NULL,Box_NOT_CLOSABLE,0);

	       /***** First row: checkbox to select the whole course *****/
	       HTM_TR_Begin (NULL);

		  HTM_TD_Begin ("colspan=\"7\" class=\"LM DAT_%s\"",
		                The_GetSuffix ());
		     HTM_LABEL_Begin (NULL);
			HTM_INPUT_CHECKBOX ("WholeCrs",HTM_DONT_SUBMIT_ON_CHANGE,
					    "id=\"WholeCrs\" value=\"Y\"%s"
					    " onclick=\"uncheckChildren(this,'GrpCods')\"",
					    Grp_DB_CheckIfAssociatedToGrps ("att_groups","AttCod",AttCod) ? "" :
													 " checked=\"checked\"");
			HTM_TxtF ("%s&nbsp;%s",Txt_The_whole_course,Gbl.Hierarchy.Crs.ShrtName);
		     HTM_LABEL_End ();
		  HTM_TD_End ();

	       HTM_TR_End ();

	       /***** List the groups for each group type *****/
	       for (NumGrpTyp = 0;
		    NumGrpTyp < Gbl.Crs.Grps.GrpTypes.NumGrpTypes;
		    NumGrpTyp++)
		  if (Gbl.Crs.Grps.GrpTypes.LstGrpTypes[NumGrpTyp].NumGrps)
		     Grp_ListGrpsToEditAsgAttSvyEvtMch (&Gbl.Crs.Grps.GrpTypes.LstGrpTypes[NumGrpTyp],
							Grp_ATT_EVENT,AttCod);

	    /***** End table and box *****/
	    Box_BoxTableEnd ();
	 HTM_TD_End ();

      HTM_TR_End ();
     }

   /***** Free list of groups types and groups in this course *****/
   Grp_FreeListGrpTypesAndGrps ();
  }

/*****************************************************************************/
/*************** Receive form to create a new attendance event ***************/
/*****************************************************************************/

void Att_ReceiveFormAttEvent (void)
  {
   extern const char *Txt_Already_existed_an_event_with_the_title_X;
   extern const char *Txt_Created_new_event_X;
   extern const char *Txt_The_event_has_been_modified;
   struct Att_Event OldAtt;
   struct Att_Event ReceivedAtt;
   bool ItsANewAttEvent;
   bool ReceivedAttEventIsCorrect = true;
   char Description[Cns_MAX_BYTES_TEXT + 1];

   /***** Get the code of the attendance event *****/
   ItsANewAttEvent = ((ReceivedAtt.AttCod = Att_GetParamAttCod ()) <= 0);

   if (!ItsANewAttEvent)
     {
      /* Get data of the old (current) attendance event from database */
      OldAtt.AttCod = ReceivedAtt.AttCod;
      Att_GetDataOfAttEventByCodAndCheckCrs (&OldAtt);
      ReceivedAtt.Hidden = OldAtt.Hidden;
     }

   /***** Get start/end date-times *****/
   ReceivedAtt.TimeUTC[Dat_STR_TIME] = Dat_GetTimeUTCFromForm ("StartTimeUTC");
   ReceivedAtt.TimeUTC[Dat_END_TIME] = Dat_GetTimeUTCFromForm ("EndTimeUTC"  );

   /***** Get boolean parameter that indicates if teacher's comments are visible by students *****/
   ReceivedAtt.CommentTchVisible = Par_GetParToBool ("ComTchVisible");

   /***** Get attendance event title *****/
   Par_GetParToText ("Title",ReceivedAtt.Title,Att_MAX_BYTES_ATTENDANCE_EVENT_TITLE);

   /***** Get attendance event description *****/
   Par_GetParToHTML ("Txt",Description,Cns_MAX_BYTES_TEXT);	// Store in HTML format (not rigorous)

   /***** Adjust dates *****/
   if (ReceivedAtt.TimeUTC[Dat_STR_TIME] == 0)
      ReceivedAtt.TimeUTC[Dat_STR_TIME] = Gbl.StartExecutionTimeUTC;
   if (ReceivedAtt.TimeUTC[Dat_END_TIME] == 0)
      ReceivedAtt.TimeUTC[Dat_END_TIME] = ReceivedAtt.TimeUTC[Dat_STR_TIME] + 2 * 60 * 60;	// +2 hours // TODO: 2 * 60 * 60 should be in a #define in swad_config.h

   /***** Check if title is correct *****/
   if (ReceivedAtt.Title[0])	// If there's an attendance event title
     {
      /* If title of attendance event was in database... */
      if (Att_DB_CheckIfSimilarAttEventExists ("Title",ReceivedAtt.Title,ReceivedAtt.AttCod))
        {
         ReceivedAttEventIsCorrect = false;

	 Ale_ShowAlert (Ale_WARNING,Txt_Already_existed_an_event_with_the_title_X,
                        ReceivedAtt.Title);
        }
     }
   else	// If there is not an attendance event title
     {
      ReceivedAttEventIsCorrect = false;
      Ale_ShowAlertYouMustSpecifyTheTitle ();
     }

   /***** Create a new attendance event or update an existing one *****/
   if (ReceivedAttEventIsCorrect)
     {
      /* Get groups for this attendance events */
      Grp_GetParCodsSeveralGrps ();

      if (ItsANewAttEvent)
	{
	 ReceivedAtt.Hidden = false;	// New attendance events are visible by default
         Att_CreateAttEvent (&ReceivedAtt,Description);	// Add new attendance event to database

         /***** Write success message *****/
	 Ale_ShowAlert (Ale_SUCCESS,Txt_Created_new_event_X,
		        ReceivedAtt.Title);
	}
      else
	{
         Att_UpdateAttEvent (&ReceivedAtt,Description);

	 /***** Write success message *****/
	 Ale_ShowAlert (Ale_SUCCESS,Txt_The_event_has_been_modified);
	}

      /* Free memory for list of selected groups */
      Grp_FreeListCodSelectedGrps ();
     }
   else
      Att_RequestCreatOrEditAttEvent ();

   /***** Show attendance events again *****/
   Att_SeeAttEvents ();
  }

/*****************************************************************************/
/********************* Create a new attendance event *************************/
/*****************************************************************************/

void Att_CreateAttEvent (struct Att_Event *Event,const char *Description)
  {
   /***** Create a new attendance event *****/
   Event->AttCod = Att_DB_CreateAttEvent (Event,Description);

   /***** Create groups *****/
   if (Gbl.Crs.Grps.LstGrpsSel.NumGrps)
      Att_CreateGroups (Event->AttCod);
  }

/*****************************************************************************/
/****************** Update an existing attendance event **********************/
/*****************************************************************************/

void Att_UpdateAttEvent (struct Att_Event *Event,const char *Description)
  {
   /***** Update the data of the attendance event *****/
   Att_DB_UpdateAttEvent (Event,Description);

   /***** Update groups *****/
   /* Remove old groups */
   Att_DB_RemoveGrpsAssociatedToAnAttEvent (Event->AttCod);

   /* Create new groups */
   if (Gbl.Crs.Grps.LstGrpsSel.NumGrps)
      Att_CreateGroups (Event->AttCod);
  }

/*****************************************************************************/
/***************** Create groups of an attendance event **********************/
/*****************************************************************************/

static void Att_CreateGroups (long AttCod)
  {
   unsigned NumGrp;

   /***** Create groups of the attendance event *****/
   for (NumGrp = 0;
	NumGrp < Gbl.Crs.Grps.LstGrpsSel.NumGrps;
	NumGrp++)
      Att_DB_CreateGroup (AttCod,Gbl.Crs.Grps.LstGrpsSel.GrpCods[NumGrp]);
  }

/*****************************************************************************/
/****** Get and write the names of the groups of an attendance event *********/
/*****************************************************************************/

static void Att_GetAndWriteNamesOfGrpsAssociatedToAttEvent (struct Att_Event *Event)
  {
   extern const char *Txt_Group;
   extern const char *Txt_Groups;
   extern const char *Txt_and;
   extern const char *Txt_The_whole_course;
   MYSQL_RES *mysql_res;
   MYSQL_ROW row;
   unsigned NumGrps;
   unsigned NumGrp;

   /***** Get groups associated to an attendance event from database *****/
   NumGrps = Att_DB_GetGroupsAssociatedToEvent (&mysql_res,Event->AttCod);

   /***** Begin container *****/
   HTM_DIV_Begin ("class=\"%s_%s\"",
                  Event->Hidden ? "ASG_GRP_LIGHT" :
        	                  "ASG_GRP",
        	  The_GetSuffix ());

      /***** Write heading *****/
      HTM_TxtColonNBSP (NumGrps == 1 ? Txt_Group  :
				       Txt_Groups);

      /***** Write groups *****/
      if (NumGrps) // Groups found...
	{
	 /* Get and write the group types and names */
	 for (NumGrp = 0;
	      NumGrp < NumGrps;
	      NumGrp++)
	   {
	    /* Get next group */
	    row = mysql_fetch_row (mysql_res);

	    /* Write group type name (row[0]) and group name (row[1]) */
	    HTM_TxtF ("%s&nbsp;%s",row[0],row[1]);

	    /* Write the name of the room (row[2]) */
	    if (row[2])	// May be NULL because of LEFT JOIN
	       if (row[2][0])
		  HTM_TxtF ("&nbsp;(%s)",row[2]);

	    /* Write separator */
	    if (NumGrps >= 2)
	      {
	       if (NumGrp == NumGrps - 2)
		  HTM_TxtF (" %s ",Txt_and);
	       if (NumGrps >= 3)
		 if (NumGrp < NumGrps - 2)
		     HTM_Txt (", ");
	      }
	   }
	}
      else
	 HTM_TxtF ("%s&nbsp;%s",
	           Txt_The_whole_course,Gbl.Hierarchy.Crs.ShrtName);

   /***** End container *****/
   HTM_DIV_End ();

   /***** Free structure that stores the query result *****/
   DB_FreeMySQLResult (&mysql_res);
  }

/*****************************************************************************/
/***************** Remove all attendance events of a course ******************/
/*****************************************************************************/

void Att_RemoveCrsAttEvents (long CrsCod)
  {
   /***** Remove students *****/
   Att_DB_RemoveUsrsFromCrsAttEvents (CrsCod);

   /***** Remove groups *****/
   Att_DB_RemoveGrpsAssociatedToCrsAttEvents (CrsCod);

   /***** Remove attendance events *****/
   Att_DB_RemoveCrsAttEvents (CrsCod);
  }

/*****************************************************************************/
/********************* Get number of attendance events ***********************/
/*****************************************************************************/
// Returns the number of attendance events
// in this location (all the platform, the current degree or the current course)

unsigned Att_GetNumAttEvents (HieLvl_Level_t Scope,unsigned *NumNotif)
  {
   MYSQL_RES *mysql_res;
   MYSQL_ROW row;
   unsigned NumAttEvents;

   /***** Set default values *****/
   NumAttEvents = 0;
   *NumNotif = 0;

   /***** Get number of attendance events from database *****/
   if (Att_DB_GetNumAttEvents (&mysql_res,Scope))
     {
      /***** Get number of attendance events *****/
      row = mysql_fetch_row (mysql_res);
      if (sscanf (row[0],"%u",&NumAttEvents) != 1)
	 Err_ShowErrorAndExit ("Error when getting number of attendance events.");

      /***** Get number of notifications by email *****/
      if (row[1])
	 if (sscanf (row[1],"%u",NumNotif) != 1)
	    Err_ShowErrorAndExit ("Error when getting number of notifications of attendance events.");
     }

   /***** Free structure that stores the query result *****/
   DB_FreeMySQLResult (&mysql_res);

   return NumAttEvents;
  }

/*****************************************************************************/
/************************ Show one attendance event **************************/
/*****************************************************************************/

void Att_SeeOneAttEvent (void)
  {
   struct Att_Events Events;

   /***** Reset attendance events *****/
   Att_ResetEvents (&Events);

   /***** Get attendance event code *****/
   if ((Events.AttCod = Att_GetParamAttCod ()) < 0)
      Err_WrongEventExit ();

   /***** Show event *****/
   Att_ShowEvent (&Events);
  }

static void Att_ShowEvent (struct Att_Events *Events)
  {
   extern const char *Hlp_USERS_Attendance;
   extern const char *Txt_Event;
   struct Att_Event Event;

   /***** Get parameters *****/
   Events->SelectedOrder = Att_GetParamAttOrder ();
   Grp_GetParamWhichGroups ();
   Events->CurrentPage = Pag_GetParamPagNum (Pag_ATT_EVENTS);

   /***** Begin box and table *****/
   Box_BoxTableBegin (NULL,Txt_Event,
                      Att_PutIconsOneAtt,Events,
                      Hlp_USERS_Attendance,Box_NOT_CLOSABLE,2);

      /***** Show attendance event *****/
      Event.AttCod = Events->AttCod;
      Att_ShowOneAttEvent (Events,&Event,true);

   /***** End table and box *****/
   Box_BoxTableEnd ();

   switch (Gbl.Usrs.Me.Role.Logged)
     {
      case Rol_STD:
	 Att_ListAttOnlyMeAsStudent (&Event);
	 break;
      case Rol_NET:
      case Rol_TCH:
      case Rol_SYS_ADM:
	 /***** Show list of students *****/
         Att_ListAttStudents (Events,&Event);
         break;
      default:
         break;
     }
  }

/*****************************************************************************/
/*************** Put contextual icons in an attendance event *****************/
/*****************************************************************************/

static void Att_PutIconsOneAtt (void *Events)
  {
   /***** Put icon to get resource link *****/
   if (Gbl.Usrs.Me.Role.Logged == Rol_SYS_ADM)		// Only if I am superuser // TODO: Include teachers
      Ico_PutContextualIconToGetLink (ActReqLnkAtt,NULL,
                                      Att_PutParams,Events);
  }

/*****************************************************************************/
/*********************** List me as student in one event *********************/
/*****************************************************************************/
// Event must be filled before calling this function

static void Att_ListAttOnlyMeAsStudent (struct Att_Event *Event)
  {
   extern const char *Hlp_USERS_Attendance;
   extern const char *Txt_Attendance;
   extern const char *Txt_Student_comment;
   extern const char *Txt_Teachers_comment;
   extern const char *Txt_ROLES_SINGUL_Abc[Rol_NUM_ROLES][Usr_NUM_SEXS];
   extern const char *Txt_Save_changes;

   /***** Get my setting about photos in users' list for current course *****/
   Set_GetMyPrefAboutListWithPhotosFromDB ();

   /***** List students (only me) *****/
   /* Begin box */
   Box_BoxBegin (NULL,Txt_Attendance,
                 NULL,NULL,
                 Hlp_USERS_Attendance,Box_NOT_CLOSABLE);

      /***** Begin form *****/
      if (Event->Open)
	{
	 Frm_BeginForm (ActRecAttMe);
	    Att_PutParamAttCod (Event->AttCod);
	}

	 /***** List students (only me) *****/
	 /* Begin table */
	 HTM_TABLE_BeginWideMarginPadding (2);

	    /* Header */
	    HTM_TR_Begin (NULL);

	       HTM_TH_Empty (3);
	       if (Gbl.Usrs.Listing.WithPhotos)
		  HTM_TH_Empty (1);
	       HTM_TH_Span (Txt_ROLES_SINGUL_Abc[Rol_STD][Usr_SEX_UNKNOWN],HTM_HEAD_LEFT,1,2,NULL);
	       HTM_TH      (Txt_Student_comment                           ,HTM_HEAD_LEFT);
	       HTM_TH      (Txt_Teachers_comment                          ,HTM_HEAD_LEFT);

	    HTM_TR_End ();

	    /* List of students (only me) */
	    Att_WriteRowUsrToCallTheRoll (1,&Gbl.Usrs.Me.UsrDat,Event);

	 /* End table */
	 HTM_TABLE_End ();

      /* Send button */
      if (Event->Open)
	{
	 Btn_PutConfirmButton (Txt_Save_changes);
	 Frm_EndForm ();
	}

   /* End box */
   Box_BoxEnd ();
  }

/*****************************************************************************/
/*************** List students who attended to one event *********************/
/*****************************************************************************/
// Event must be filled before calling this function

static void Att_ListAttStudents (struct Att_Events *Events,
	                         struct Att_Event *Event)
  {
   extern const char *Hlp_USERS_Attendance;
   extern const char *Txt_Attendance;
   extern const char *Txt_Student_comment;
   extern const char *Txt_Teachers_comment;
   extern const char *Txt_ROLES_SINGUL_Abc[Rol_NUM_ROLES][Usr_NUM_SEXS];
   extern const char *Txt_Save_changes;
   unsigned NumUsr;
   struct UsrData UsrDat;

   /***** Get groups to show ******/
   Grp_GetParCodsSeveralGrpsToShowUsrs ();

   /***** Get and order list of students in this course *****/
   Usr_GetListUsrs (HieLvl_CRS,Rol_STD);

   /***** Begin box *****/
   Box_BoxBegin (NULL,Txt_Attendance,
                 NULL,NULL,
                 Hlp_USERS_Attendance,Box_NOT_CLOSABLE);

      /***** Form to select groups *****/
      Grp_ShowFormToSelectSeveralGroups (Att_PutParamSelectedAttCod,Events,
					 Grp_MY_GROUPS);

      /***** Begin section with user list *****/
      HTM_SECTION_Begin (Usr_USER_LIST_SECTION_ID);

	 if (Gbl.Usrs.LstUsrs[Rol_STD].NumUsrs)
	   {
	    /***** Get my preference about photos in users' list for current course *****/
	    Set_GetMyPrefAboutListWithPhotosFromDB ();

	    /***** Initialize structure with user's data *****/
	    Usr_UsrDataConstructor (&UsrDat);

	    /* Begin form */
	    Frm_BeginForm (ActRecAttStd);
	       Att_PutParamAttCod (Event->AttCod);
	       Grp_PutParamsCodGrps ();

	       /* Begin table */
	       HTM_TABLE_BeginWideMarginPadding (2);

		  /* Header */
		  HTM_TR_Begin (NULL);

		     HTM_TH_Empty (3);
		     if (Gbl.Usrs.Listing.WithPhotos)
			HTM_TH_Empty (1);
		     HTM_TH_Span (Txt_ROLES_SINGUL_Abc[Rol_STD][Usr_SEX_UNKNOWN],HTM_HEAD_LEFT,1,2,NULL);
		     HTM_TH      (Txt_Student_comment                           ,HTM_HEAD_LEFT);
		     HTM_TH      (Txt_Teachers_comment                          ,HTM_HEAD_LEFT);

		  HTM_TR_End ();

		  /* List of students */
		  for (NumUsr = 0;
		       NumUsr < Gbl.Usrs.LstUsrs[Rol_STD].NumUsrs;
		       NumUsr++)
		    {
		     /* Copy user's basic data from list */
		     Usr_CopyBasicUsrDataFromList (&UsrDat,&Gbl.Usrs.LstUsrs[Rol_STD].Lst[NumUsr]);

		     /* Get list of user's IDs */
		     ID_GetListIDsFromUsrCod (&UsrDat);

		     Att_WriteRowUsrToCallTheRoll (NumUsr + 1,&UsrDat,Event);
		    }

	       /* End table */
	       HTM_TABLE_End ();

	       /* Send button */
	       Btn_PutConfirmButton (Txt_Save_changes);

	    /***** End form *****/
	    Frm_EndForm ();

	    /***** Free memory used for user's data *****/
	    Usr_UsrDataDestructor (&UsrDat);
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
/************** Write a row of a table with the data of a user ***************/
/*****************************************************************************/

static void Att_WriteRowUsrToCallTheRoll (unsigned NumUsr,
                                          struct UsrData *UsrDat,
                                          struct Att_Event *Event)
  {
   static const char *ClassPhoto[PhoSha_NUM_SHAPES] =
     {
      [PhoSha_SHAPE_CIRCLE   ] = "PHOTOC45x60",
      [PhoSha_SHAPE_ELLIPSE  ] = "PHOTOE45x60",
      [PhoSha_SHAPE_OVAL     ] = "PHOTOO45x60",
      [PhoSha_SHAPE_RECTANGLE] = "PHOTOR45x60",
     };
   bool Present;
   char CommentStd[Cns_MAX_BYTES_TEXT + 1];
   char CommentTch[Cns_MAX_BYTES_TEXT + 1];
   bool ICanChangeStdAttendance;
   bool ICanEditStdComment;
   bool ICanEditTchComment;

   /***** Set who can edit *****/
   switch (Gbl.Usrs.Me.Role.Logged)
     {
      case Rol_STD:
	 // A student can see only her/his attendance
	 if (!Usr_ItsMe (UsrDat->UsrCod))
	    Err_ShowErrorAndExit ("Wrong call.");
	 ICanChangeStdAttendance = false;
	 ICanEditStdComment = Event->Open;	// Attendance event is open
	 ICanEditTchComment = false;
	 break;
      case Rol_TCH:
	 ICanChangeStdAttendance = true;
	 ICanEditStdComment = false;
	 ICanEditTchComment = true;
	 break;
      case Rol_SYS_ADM:
	 ICanChangeStdAttendance = true;
	 ICanEditStdComment = false;
	 ICanEditTchComment = false;
	 break;
      default:
	 ICanChangeStdAttendance = false;
	 ICanEditStdComment = false;
	 ICanEditTchComment = false;
	 break;
     }

   /***** Check if this student is already present in the current event *****/
   Present = Att_CheckIfUsrIsPresentInAttEventAndGetComments (Event->AttCod,UsrDat->UsrCod,CommentStd,CommentTch);

   /***** Begin table row *****/
   HTM_TR_Begin (NULL);

      /***** Icon to show if the user is already present *****/
      HTM_TD_Begin ("class=\"BT %s\"",The_GetColorRows ());
	 HTM_LABEL_Begin ("for=\"Std%u\"",NumUsr);
	    Att_PutCheckOrCross (Present);
	 HTM_LABEL_End ();
      HTM_TD_End ();

      /***** Checkbox to select user *****/
      HTM_TD_Begin ("class=\"CT %s\"",The_GetColorRows ());
	 HTM_INPUT_CHECKBOX ("UsrCodStd",HTM_DONT_SUBMIT_ON_CHANGE,
			     "id=\"Std%u\" value=\"%s\"%s%s",
			     NumUsr,UsrDat->EnUsrCod,
			     Present ? " checked=\"checked\"" : "",
			     ICanChangeStdAttendance ? "" :
				                       " disabled=\"disabled\"");
      HTM_TD_End ();

      /***** Write number of student in the list *****/
      HTM_TD_Begin ("class=\"RT %s_%s %s\"",
		    UsrDat->Accepted ? "DAT_STRONG" :
				       "DAT",
		    The_GetSuffix (),
		    The_GetColorRows ());
	 HTM_Unsigned (NumUsr);
      HTM_TD_End ();

      /***** Show student's photo *****/
      if (Gbl.Usrs.Listing.WithPhotos)
	{
	 HTM_TD_Begin ("class=\"%s LT\"",The_GetColorRows ());
	    Pho_ShowUsrPhotoIfAllowed (UsrDat,
	                               ClassPhoto[Gbl.Prefs.PhotoShape],Pho_ZOOM,
	                               false);
	 HTM_TD_End ();
	}

      /***** Write user's ID ******/
      HTM_TD_Begin ("class=\"LT %s_%s %s\"",
		    UsrDat->Accepted ? "DAT_SMALL_STRONG" :
				       "DAT_SMALL",
		    The_GetSuffix (),
		    The_GetColorRows ());
	 ID_WriteUsrIDs (UsrDat,NULL);
      HTM_TD_End ();

      /***** Write student's name *****/
      HTM_TD_Begin ("class=\"LT %s_%s %s\"",
		    UsrDat->Accepted ? "DAT_SMALL_STRONG" :
				       "DAT_SMALL",
		    The_GetSuffix (),
		    The_GetColorRows ());
	 HTM_Txt (UsrDat->Surname1);
	 if (UsrDat->Surname2[0])
	    HTM_TxtF ("&nbsp;%s",UsrDat->Surname2);
	 HTM_TxtF (", %s",UsrDat->FrstName);
      HTM_TD_End ();

      /***** Student's comment: write form or text */
      HTM_TD_Begin ("class=\"LT %s_%s %s\"",
		    UsrDat->Accepted ? "DAT_SMALL_STRONG" :
				       "DAT_SMALL",
		    The_GetSuffix (),
		    The_GetColorRows ());
	 if (ICanEditStdComment)	// Show with form
	   {
	    HTM_TEXTAREA_Begin ("name=\"CommentStd%s\" cols=\"40\" rows=\"3\""
				" class=\"INPUT_%s\"",
				UsrDat->EnUsrCod,
				The_GetSuffix ());
	       HTM_Txt (CommentStd);
	    HTM_TEXTAREA_End ();
	   }
	 else				// Show without form
	   {
	    Str_ChangeFormat (Str_FROM_HTML,Str_TO_RIGOROUS_HTML,
			      CommentStd,Cns_MAX_BYTES_TEXT,false);
	    HTM_Txt (CommentStd);
	   }
      HTM_TD_End ();

      /***** Teacher's comment: write form, text or nothing */
      HTM_TD_Begin ("class=\"LT %s_%s %s\"",
		    UsrDat->Accepted ? "DAT_SMALL_STRONG" :
				       "DAT_SMALL",
		    The_GetSuffix (),
		    The_GetColorRows ());
	 if (ICanEditTchComment)		// Show with form
	   {
	    HTM_TEXTAREA_Begin ("name=\"CommentTch%s\" cols=\"40\" rows=\"3\""
				" class=\"INPUT_%s\"",
				UsrDat->EnUsrCod,
				The_GetSuffix ());
	       HTM_Txt (CommentTch);
	    HTM_TEXTAREA_End ();
	   }
	 else	if (Event->CommentTchVisible)	// Show without form
	   {
	    Str_ChangeFormat (Str_FROM_HTML,Str_TO_RIGOROUS_HTML,
			      CommentTch,Cns_MAX_BYTES_TEXT,false);
	    HTM_Txt (CommentTch);
	   }
      HTM_TD_End ();

   /***** End table row *****/
   HTM_TR_End ();

   The_ChangeRowColor ();
  }

/*****************************************************************************/
/**************** Put link to view one attendance event **********************/
/*****************************************************************************/

static void Att_PutLinkAttEvent (struct Att_Event *Event,
				 const char *Title,const char *Txt)
  {
   /***** Begin form *****/
   Frm_BeginForm (ActSeeOneAtt);
      Att_PutParamAttCod (Event->AttCod);
      Att_PutParamsCodGrps (Event->AttCod);

      /***** Link to view attendance event *****/
      HTM_BUTTON_Submit_Begin (Title,"class=\"LT BT_LINK %s_%s\"",
			       Event->Hidden ? "ASG_TITLE_LIGHT" :
					       "ASG_TITLE",
			       The_GetSuffix ());
	 HTM_Txt (Txt);
      HTM_BUTTON_End ();

   /***** End form *****/
   Frm_EndForm ();
  }

/*****************************************************************************/
/****** Put parameters with the default groups in an attendance event ********/
/*****************************************************************************/

static void Att_PutParamsCodGrps (long AttCod)
  {
   extern const char *Par_SEPARATOR_PARAM_MULTIPLE;
   MYSQL_RES *mysql_res;
   MYSQL_ROW row;
   unsigned NumGrp;
   unsigned NumGrps;
   size_t MaxLengthGrpCods;
   char *GrpCods;

   /***** Get groups associated to an attendance event from database *****/
   if (Gbl.Crs.Grps.NumGrps)
      NumGrps = Att_DB_GetGrpCodsAssociatedToEvent (&mysql_res,AttCod);
   else
      NumGrps = 0;

   /***** Get groups *****/
   if (NumGrps) // Groups found...
     {
      MaxLengthGrpCods = NumGrps * (1 + 20) - 1;
      if ((GrpCods = malloc (MaxLengthGrpCods + 1)) == NULL)
	 Err_NotEnoughMemoryExit ();
      GrpCods[0] = '\0';

      /* Get groups */
      for (NumGrp = 0;
	   NumGrp < NumGrps;
	   NumGrp++)
        {
         /* Get next group */
         row = mysql_fetch_row (mysql_res);

         /* Append group code to list */
         if (NumGrp)
            Str_Concat (GrpCods,Par_SEPARATOR_PARAM_MULTIPLE,MaxLengthGrpCods);
         Str_Concat (GrpCods,row[0],MaxLengthGrpCods);
        }

      Par_PutHiddenParamString (NULL,"GrpCods",GrpCods);
      free (GrpCods);
     }
   else
      /***** Write the boolean parameter that indicates
             if all groups should be listed *****/
      Par_PutHiddenParamChar ("AllGroups",'Y');

   /***** Free structure that stores the query result *****/
   if (Gbl.Crs.Grps.NumGrps)
      DB_FreeMySQLResult (&mysql_res);
  }

/*****************************************************************************/
/*************** Save me as students who attended to an event ****************/
/*****************************************************************************/

void Att_RegisterMeAsStdInAttEvent (void)
  {
   extern const char *Txt_Your_comment_has_been_updated;
   struct Att_Events Events;
   struct Att_Event Event;
   bool Present;
   char *ParamName;
   char CommentStd[Cns_MAX_BYTES_TEXT + 1];
   char CommentTch[Cns_MAX_BYTES_TEXT + 1];

   /***** Reset attendance events *****/
   Att_ResetEvents (&Events);

   /***** Get attendance event code *****/
   if ((Event.AttCod = Att_GetParamAttCod ()) < 0)
      Err_WrongEventExit ();
   Att_GetDataOfAttEventByCodAndCheckCrs (&Event);	// This checks that event belong to current course

   if (Event.Open)
     {
      /***** Get comments for this student *****/
      Present = Att_CheckIfUsrIsPresentInAttEventAndGetComments (Event.AttCod,Gbl.Usrs.Me.UsrDat.UsrCod,
	                                                         CommentStd,CommentTch);
      if (asprintf (&ParamName,"CommentStd%s",Gbl.Usrs.Me.UsrDat.EnUsrCod) < 0)
         Err_NotEnoughMemoryExit ();
      Par_GetParToHTML (ParamName,CommentStd,Cns_MAX_BYTES_TEXT);
      free (ParamName);

      if (Present ||
	  CommentStd[0] ||
	  CommentTch[0])
	 /***** Register student *****/
	 Att_DB_RegUsrInAttEventChangingComments (Event.AttCod,Gbl.Usrs.Me.UsrDat.UsrCod,
	                                          Present,CommentStd,CommentTch);
      else
	 /***** Remove student *****/
	 Att_DB_RemoveUsrFromAttEvent (Event.AttCod,Gbl.Usrs.Me.UsrDat.UsrCod);

      /***** Write final message *****/
      Ale_ShowAlert (Ale_SUCCESS,Txt_Your_comment_has_been_updated);
     }

   /***** Show the attendance event again *****/
   Events.AttCod = Event.AttCod;
   Att_ShowEvent (&Events);
  }

/*****************************************************************************/
/***************** Save students who attended to an event ********************/
/*****************************************************************************/
/* Algorithm:
   1. Get list of students in the groups selected: Gbl.Usrs.LstUsrs[Rol_STD]
   2. Mark all students in the groups selected setting Remove=true
   3. Get list of students marked as present by me: Gbl.Usrs.Selected.List[Rol_STD]
   4. Loop over the list Gbl.Usrs.Selected.List[Rol_STD],
      that holds the list of the students marked as present,
      marking the students in Gbl.Usrs.LstUsrs[Rol_STD].Lst as Remove=false
   5. Delete from att_users all students marked as Remove=true
   6. Replace (insert without duplicated) into att_users all students marked as Remove=false
 */
void Att_RegisterStudentsInAttEvent (void)
  {
   extern const char *Txt_Presents;
   extern const char *Txt_Absents;
   struct Att_Events Events;
   struct Att_Event Event;
   unsigned NumUsr;
   const char *Ptr;
   bool Present;
   char *ParamName;
   unsigned NumStdsPresent;
   unsigned NumStdsAbsent;
   struct UsrData UsrData;
   char CommentStd[Cns_MAX_BYTES_TEXT + 1];
   char CommentTch[Cns_MAX_BYTES_TEXT + 1];

   /***** Reset attendance events *****/
   Att_ResetEvents (&Events);

   /***** Get attendance event code *****/
   if ((Event.AttCod = Att_GetParamAttCod ()) < 0)
      Err_WrongEventExit ();
   Att_GetDataOfAttEventByCodAndCheckCrs (&Event);	// This checks that event belong to current course

   /***** Get groups selected *****/
   Grp_GetParCodsSeveralGrpsToShowUsrs ();

   /***** 1. Get list of students in the groups selected: Gbl.Usrs.LstUsrs[Rol_STD] *****/
   /* Get list of students in the groups selected */
   Usr_GetListUsrs (HieLvl_CRS,Rol_STD);

   if (Gbl.Usrs.LstUsrs[Rol_STD].NumUsrs)	// If there are students in the groups selected...
     {
      /***** 2. Mark all students in the groups selected setting Remove=true *****/
      for (NumUsr = 0;
           NumUsr < Gbl.Usrs.LstUsrs[Rol_STD].NumUsrs;
           NumUsr++)
         Gbl.Usrs.LstUsrs[Rol_STD].Lst[NumUsr].Remove = true;

      /***** 3. Get list of students marked as present by me: Gbl.Usrs.Selected.List[Rol_STD] *****/
      Usr_GetListsSelectedEncryptedUsrsCods (&Gbl.Usrs.Selected);

      /***** Initialize structure with user's data *****/
      Usr_UsrDataConstructor (&UsrData);

      /***** 4. Loop over the list Gbl.Usrs.Selected.List[Rol_STD],
                that holds the list of the students marked as present,
                marking the students in Gbl.Usrs.LstUsrs[Rol_STD].Lst as Remove=false *****/
      Ptr = Gbl.Usrs.Selected.List[Rol_STD];
      while (*Ptr)
	{
	 Par_GetNextStrUntilSeparParamMult (&Ptr,UsrData.EnUsrCod,
	                                    Cry_BYTES_ENCRYPTED_STR_SHA256_BASE64);
	 Usr_GetUsrCodFromEncryptedUsrCod (&UsrData);
	 if (UsrData.UsrCod > 0)	// Student exists in database
	    /***** Mark student to not be removed *****/
	    for (NumUsr = 0;
		 NumUsr < Gbl.Usrs.LstUsrs[Rol_STD].NumUsrs;
		 NumUsr++)
	       if (Gbl.Usrs.LstUsrs[Rol_STD].Lst[NumUsr].UsrCod == UsrData.UsrCod)
		 {
		  Gbl.Usrs.LstUsrs[Rol_STD].Lst[NumUsr].Remove = false;
	          break;	// Found! Exit loop
	         }
	}

      /***** Free memory used for user's data *****/
      Usr_UsrDataDestructor (&UsrData);

      /***** Free memory *****/
      /* Free memory used by list of selected students' codes */
      Usr_FreeListsSelectedEncryptedUsrsCods (&Gbl.Usrs.Selected);

      // 5. Delete from att_users all students marked as Remove=true
      // 6. Replace (insert without duplicated) into att_users all students marked as Remove=false
      for (NumUsr = 0, NumStdsAbsent = NumStdsPresent = 0;
	   NumUsr < Gbl.Usrs.LstUsrs[Rol_STD].NumUsrs;
	   NumUsr++)
	{
	 /***** Get comments for this student *****/
	 Att_CheckIfUsrIsPresentInAttEventAndGetComments (Event.AttCod,
	                                                  Gbl.Usrs.LstUsrs[Rol_STD].Lst[NumUsr].UsrCod,
	                                                  CommentStd,
	                                                  CommentTch);
	 if (asprintf (&ParamName,"CommentTch%s",
	               Gbl.Usrs.LstUsrs[Rol_STD].Lst[NumUsr].EnUsrCod) < 0)
	    Err_NotEnoughMemoryExit ();
	 Par_GetParToHTML (ParamName,CommentTch,Cns_MAX_BYTES_TEXT);
	 free (ParamName);

	 Present = !Gbl.Usrs.LstUsrs[Rol_STD].Lst[NumUsr].Remove;

	 if (Present ||
	     CommentStd[0] ||
	     CommentTch[0])
	    /***** Register student *****/
	    Att_DB_RegUsrInAttEventChangingComments (Event.AttCod,Gbl.Usrs.LstUsrs[Rol_STD].Lst[NumUsr].UsrCod,
					             Present,CommentStd,CommentTch);
	 else
	    /***** Remove student *****/
	    Att_DB_RemoveUsrFromAttEvent (Event.AttCod,Gbl.Usrs.LstUsrs[Rol_STD].Lst[NumUsr].UsrCod);

	 if (Present)
            NumStdsPresent++;
      	 else
	    NumStdsAbsent++;
	}

      /***** Free memory for students list *****/
      Usr_FreeUsrsList (Rol_STD);

      /***** Write final message *****/
      Ale_ShowAlert (Ale_INFO,"%s: %u<br />"
		              "%s: %u",
		     Txt_Presents,NumStdsPresent,
		     Txt_Absents ,NumStdsAbsent );
     }
   else	// Gbl.Usrs.LstUsrs[Rol_STD].NumUsrs == 0
      /***** Show warning indicating no students found *****/
      Usr_ShowWarningNoUsersFound (Rol_STD);

   /***** Show the attendance event again *****/
   Events.AttCod = Event.AttCod;
   Att_ShowEvent (&Events);

   /***** Free memory for list of groups selected *****/
   Grp_FreeListCodSelectedGrps ();
  }

/*****************************************************************************/
/********* Get number of users from a list who attended to an event **********/
/*****************************************************************************/

static unsigned Att_GetNumUsrsFromAListWhoAreInAttEvent (long AttCod,
						         long LstSelectedUsrCods[],
						         unsigned NumUsrsInList)
  {
   char *SubQueryUsrs;
   unsigned NumUsrsInAttEvent;

   if (NumUsrsInList)
     {
      /***** Create subquery string *****/
      Usr_CreateSubqueryUsrCods (LstSelectedUsrCods,NumUsrsInList,
				 &SubQueryUsrs);

      /***** Get number of users from list in attendance event from database ****/
      NumUsrsInAttEvent = Att_DB_GetNumStdsFromListWhoAreInAttEvent (AttCod,SubQueryUsrs);

      /***** Free memory for subquery string *****/
      Usr_FreeSubqueryUsrCods (SubQueryUsrs);
     }
   else
      NumUsrsInAttEvent = 0;

   return NumUsrsInAttEvent;
  }

/*****************************************************************************/
/***************** Check if a student attended to an event *******************/
/*****************************************************************************/

static bool Att_CheckIfUsrIsPresentInAttEvent (long AttCod,long UsrCod)
  {
   bool Present;

   Att_DB_CheckIfUsrIsInTableAttUsr (AttCod,UsrCod,&Present);

   return Present;
  }

/*****************************************************************************/
/***************** Check if a student attended to an event *******************/
/*****************************************************************************/

static bool Att_CheckIfUsrIsPresentInAttEventAndGetComments (long AttCod,long UsrCod,
                                                             char CommentStd[Cns_MAX_BYTES_TEXT + 1],
                                                             char CommentTch[Cns_MAX_BYTES_TEXT + 1])
  {
   MYSQL_RES *mysql_res;
   MYSQL_ROW row;
   bool Present;

   /***** Check if a students is registered in an event in database *****/
   if (Att_DB_GetPresentAndComments (&mysql_res,AttCod,UsrCod))
     {
      /* Get row */
      row = mysql_fetch_row (mysql_res);

      /* Get if present (row[0]) */
      Present = (row[0][0] == 'Y');

      /* Get student's (row[1]) and teacher's (row[2]) comment */
      Str_Copy (CommentStd,row[1],Cns_MAX_BYTES_TEXT);
      Str_Copy (CommentTch,row[2],Cns_MAX_BYTES_TEXT);
     }
   else	// User is not present
     {
      Present = false;
      CommentStd[0] =
      CommentTch[0] = '\0';
     }

   /***** Free structure that stores the query result *****/
   DB_FreeMySQLResult (&mysql_res);

   return Present;
  }

/*****************************************************************************/
/********** Request listing attendance of users to several events ************/
/*****************************************************************************/

void Att_ReqListUsrsAttendanceCrs (void)
  {
   Att_TypeOfView_t TypeOfView = Att_VIEW_SEL_USR;

   Att_ReqListOrPrintUsrsAttendanceCrs (&TypeOfView);
  }

static void Att_ReqListOrPrintUsrsAttendanceCrs (void *TypeOfView)
  {
   extern const char *Hlp_USERS_Attendance_attendance_list;
   extern const char *Txt_Attendance_list;
   extern const char *Txt_View_attendance;
   struct Att_Events Events;

   switch (*((Att_TypeOfView_t *) TypeOfView))
     {
      case Att_VIEW_SEL_USR:
      case Att_PRNT_SEL_USR:
	 /***** Reset attendance events *****/
	 Att_ResetEvents (&Events);

	 /***** Get list of attendance events *****/
	 Att_GetListAttEvents (&Events,Att_OLDEST_FIRST);

	 /***** List users to select some of them *****/
	 Usr_PutFormToSelectUsrsToGoToAct (&Gbl.Usrs.Selected,
					   ActSeeLstUsrAtt,
					   NULL,NULL,
					   Txt_Attendance_list,
					   Hlp_USERS_Attendance_attendance_list,
					   Txt_View_attendance,
					   false);	// Do not put form with date range

	 /***** Free list of attendance events *****/
	 Att_FreeListAttEvents (&Events);
	 break;
      default:
	 Err_WrongTypeExit ();
	 break;
     }
  }

/*****************************************************************************/
/********** List my attendance (I am a student) to several events ************/
/*****************************************************************************/

void Att_ListMyAttendanceCrs (void)
  {
   Att_ListOrPrintMyAttendanceCrs (Att_VIEW_ONLY_ME);
  }

void Att_PrintMyAttendanceCrs (void)
  {
   Att_ListOrPrintMyAttendanceCrs (Att_PRNT_ONLY_ME);
  }

static void Att_ListOrPrintMyAttendanceCrs (Att_TypeOfView_t TypeOfView)
  {
   extern const char *Hlp_USERS_Attendance_attendance_list;
   extern const char *Txt_Attendance;
   struct Att_Events Events;
   unsigned NumAttEvent;

   switch (TypeOfView)
     {
      case Att_VIEW_ONLY_ME:
      case Att_PRNT_ONLY_ME:
	 /***** Reset attendance events *****/
	 Att_ResetEvents (&Events);

	 /***** Get list of attendance events *****/
	 Att_GetListAttEvents (&Events,Att_OLDEST_FIRST);

	 /***** Get boolean parameter that indicates if details must be shown *****/
	 Events.ShowDetails = Par_GetParToBool ("ShowDetails");

	 /***** Get list of groups selected ******/
	 Grp_GetParCodsSeveralGrpsToShowUsrs ();

	 /***** Get number of students in each event *****/
	 for (NumAttEvent = 0;
	      NumAttEvent < Events.Num;
	      NumAttEvent++)
	    /* Get number of students in this event */
	    Events.Lst[NumAttEvent].NumStdsFromList =
	    Att_GetNumUsrsFromAListWhoAreInAttEvent (Events.Lst[NumAttEvent].AttCod,
						     &Gbl.Usrs.Me.UsrDat.UsrCod,1);

	 /***** Get list of attendance events selected *****/
	 Att_GetListSelectedAttCods (&Events);

	 /***** Begin box *****/
	 switch (TypeOfView)
	   {
	    case Att_VIEW_ONLY_ME:
	       Box_BoxBegin (NULL,Txt_Attendance,
			     Att_PutIconsMyAttList,&Events,
			     Hlp_USERS_Attendance_attendance_list,Box_NOT_CLOSABLE);
	       break;
	    case Att_PRNT_ONLY_ME:
	       Box_BoxBegin (NULL,Txt_Attendance,
			     NULL,NULL,
			     NULL,Box_NOT_CLOSABLE);
	       break;
	    default:
	       Err_WrongTypeExit ();
	       break;
	   }

	 /***** List events to select *****/
	 Att_ListEventsToSelect (&Events,TypeOfView);

	 /***** Get my preference about photos in users' list for current course *****/
	 Set_GetMyPrefAboutListWithPhotosFromDB ();

	 /***** Show table with attendances for every student in list *****/
	 Att_ListUsrsAttendanceTable (&Events,TypeOfView,1,&Gbl.Usrs.Me.UsrDat.UsrCod);

	 /***** Show details or put button to show details *****/
	 if (Events.ShowDetails)
	    Att_ListStdsWithAttEventsDetails (&Events,1,&Gbl.Usrs.Me.UsrDat.UsrCod);

	 /***** End box *****/
	 Box_BoxEnd ();

	 /***** Free memory for list of attendance events selected *****/
	 free (Events.StrAttCodsSelected);

	 /***** Free list of groups selected *****/
	 Grp_FreeListCodSelectedGrps ();

	 /***** Free list of attendance events *****/
	 Att_FreeListAttEvents (&Events);
	 break;
      default:
	 Err_WrongTypeExit ();
	 break;
     }
  }

/*****************************************************************************/
/*************** List attendance of users to several events ******************/
/*****************************************************************************/

void Att_ListUsrsAttendanceCrs (void)
  {
   Att_GetUsrsAndListOrPrintAttendanceCrs (Att_VIEW_SEL_USR);
  }

void Att_PrintUsrsAttendanceCrs (void)
  {
   Att_GetUsrsAndListOrPrintAttendanceCrs (Att_PRNT_SEL_USR);
  }

static void Att_GetUsrsAndListOrPrintAttendanceCrs (Att_TypeOfView_t TypeOfView)
  {
   Usr_GetSelectedUsrsAndGoToAct (&Gbl.Usrs.Selected,
				  Att_ListOrPrintUsrsAttendanceCrs,&TypeOfView,
                                  Att_ReqListOrPrintUsrsAttendanceCrs,&TypeOfView);
  }

static void Att_ListOrPrintUsrsAttendanceCrs (void *TypeOfView)
  {
   extern const char *Hlp_USERS_Attendance_attendance_list;
   extern const char *Txt_Attendance_list;
   struct Att_Events Events;
   unsigned NumUsrsInList;
   long *LstSelectedUsrCods;
   unsigned NumAttEvent;

   switch (*((Att_TypeOfView_t *) TypeOfView))
     {
      case Att_VIEW_SEL_USR:
      case Att_PRNT_SEL_USR:
	 /***** Reset attendance events *****/
	 Att_ResetEvents (&Events);

	 /***** Get parameters *****/
	 /* Get boolean parameter that indicates if details must be shown */
	 Events.ShowDetails = Par_GetParToBool ("ShowDetails");

	 /* Get list of groups selected */
	 Grp_GetParCodsSeveralGrpsToShowUsrs ();

	 /***** Count number of valid users in list of encrypted user codes *****/
	 NumUsrsInList = Usr_CountNumUsrsInListOfSelectedEncryptedUsrCods (&Gbl.Usrs.Selected);

	 if (NumUsrsInList)
	   {
	    /***** Get list of students selected to show their attendances *****/
	    Usr_GetListSelectedUsrCods (&Gbl.Usrs.Selected,NumUsrsInList,&LstSelectedUsrCods);

	    /***** Get list of attendance events *****/
	    Att_GetListAttEvents (&Events,Att_OLDEST_FIRST);

	    /***** Get number of students in each event *****/
	    for (NumAttEvent = 0;
		 NumAttEvent < Events.Num;
		 NumAttEvent++)
	       /* Get number of students in this event */
	       Events.Lst[NumAttEvent].NumStdsFromList =
	       Att_GetNumUsrsFromAListWhoAreInAttEvent (Events.Lst[NumAttEvent].AttCod,
							LstSelectedUsrCods,NumUsrsInList);

	    /***** Get list of attendance events selected *****/
	    Att_GetListSelectedAttCods (&Events);

	    /***** Begin box *****/
	    switch (*((Att_TypeOfView_t *) TypeOfView))
	      {
	       case Att_VIEW_SEL_USR:
		  Box_BoxBegin (NULL,Txt_Attendance_list,
				Att_PutIconsStdsAttList,&Events,
				Hlp_USERS_Attendance_attendance_list,Box_NOT_CLOSABLE);
		  break;
	       case Att_PRNT_SEL_USR:
		  Box_BoxBegin (NULL,Txt_Attendance_list,
				NULL,NULL,
				NULL,Box_NOT_CLOSABLE);
		  break;
	       default:
		  Err_WrongTypeExit ();
	      }

	    /***** List events to select *****/
	    Att_ListEventsToSelect (&Events,*((Att_TypeOfView_t *) TypeOfView));

	    /***** Get my preference about photos in users' list for current course *****/
	    Set_GetMyPrefAboutListWithPhotosFromDB ();

	    /***** Show table with attendances for every student in list *****/
	    Att_ListUsrsAttendanceTable (&Events,*((Att_TypeOfView_t *) TypeOfView),
	                                 NumUsrsInList,LstSelectedUsrCods);

	    /***** Show details or put button to show details *****/
	    if (Events.ShowDetails)
	       Att_ListStdsWithAttEventsDetails (&Events,NumUsrsInList,LstSelectedUsrCods);

	    /***** End box *****/
	    Box_BoxEnd ();

	    /***** Free memory for list of attendance events selected *****/
	    free (Events.StrAttCodsSelected);

	    /***** Free list of attendance events *****/
	    Att_FreeListAttEvents (&Events);

	    /***** Free list of user codes *****/
	    Usr_FreeListSelectedUsrCods (LstSelectedUsrCods);
	   }

	 /***** Free list of groups selected *****/
	 Grp_FreeListCodSelectedGrps ();
	 break;
      default:
	 Err_WrongTypeExit ();
	 break;
     }
  }

/*****************************************************************************/
/****************** Get list of attendance events selected *******************/
/*****************************************************************************/

static void Att_GetListSelectedAttCods (struct Att_Events *Events)
  {
   size_t MaxSizeListAttCodsSelected;
   unsigned NumAttEvent;
   const char *Ptr;
   long AttCod;
   char LongStr[Cns_MAX_DECIMAL_DIGITS_LONG + 1];
   MYSQL_RES *mysql_res;
   unsigned NumGrpsInThisEvent;
   unsigned NumGrpInThisEvent;
   long GrpCodInThisEvent;
   unsigned NumGrpSel;

   /***** Allocate memory for list of attendance events selected *****/
   MaxSizeListAttCodsSelected = (size_t) Events->Num * (Cns_MAX_DECIMAL_DIGITS_LONG + 1);
   if ((Events->StrAttCodsSelected = malloc (MaxSizeListAttCodsSelected + 1)) == NULL)
      Err_NotEnoughMemoryExit ();

   /***** Get parameter multiple with list of attendance events selected *****/
   Par_GetParMultiToText ("AttCods",Events->StrAttCodsSelected,MaxSizeListAttCodsSelected);

   /***** Set which attendance events will be shown as selected (checkboxes on) *****/
   if (Events->StrAttCodsSelected[0])	// There are events selected
     {
      /* Reset selection */
      for (NumAttEvent = 0;
	   NumAttEvent < Events->Num;
	   NumAttEvent++)
	 Events->Lst[NumAttEvent].Selected = false;

      /* Set some events as selected */
      for (Ptr = Events->StrAttCodsSelected;
	   *Ptr;
	  )
	{
	 /* Get next attendance event selected */
	 Par_GetNextStrUntilSeparParamMult (&Ptr,LongStr,Cns_MAX_DECIMAL_DIGITS_LONG);
	 AttCod = Str_ConvertStrCodToLongCod (LongStr);

	 /* Set each event in *StrAttCodsSelected as selected */
	 for (NumAttEvent = 0;
	      NumAttEvent < Events->Num;
	      NumAttEvent++)
	    if (Events->Lst[NumAttEvent].AttCod == AttCod)
	      {
	       Events->Lst[NumAttEvent].Selected = true;
	       break;
	      }
	}
     }
   else				// No events selected
     {
      /***** Set which events will be marked as selected by default *****/
      if (!Gbl.Crs.Grps.NumGrps ||	// Course has no groups
          Gbl.Usrs.ClassPhoto.AllGroups)	// All groups selected
	 /* Set all events as selected */
	 for (NumAttEvent = 0;
	      NumAttEvent < Events->Num;
	      NumAttEvent++)
	    Events->Lst[NumAttEvent].Selected = true;
      else					// Course has groups and not all of them are selected
	 for (NumAttEvent = 0;
	      NumAttEvent < Events->Num;
	      NumAttEvent++)
	   {
	    /* Reset selection */
	    Events->Lst[NumAttEvent].Selected = false;

	    /* Set this event as selected? */
	    if (Events->Lst[NumAttEvent].NumStdsFromList)	// Some students attended to this event
	       Events->Lst[NumAttEvent].Selected = true;
	    else						// No students attended to this event
	      {
	       /***** Get groups associated to an attendance event from database *****/
	       if ((NumGrpsInThisEvent = Att_DB_GetGrpCodsAssociatedToEvent (&mysql_res,
	                                                                     Events->Lst[NumAttEvent].AttCod)))	// This event is associated to groups
		  /* Get groups associated to this event */
		  for (NumGrpInThisEvent = 0;
		       NumGrpInThisEvent < NumGrpsInThisEvent &&
		       !Events->Lst[NumAttEvent].Selected;
		       NumGrpInThisEvent++)
		    {
		     /* Get next group associated to this event */
		     if ((GrpCodInThisEvent = DB_GetNextCode (mysql_res)) > 0)
			/* Check if this group is selected */
			for (NumGrpSel = 0;
			     NumGrpSel < Gbl.Crs.Grps.LstGrpsSel.NumGrps &&
			     !Events->Lst[NumAttEvent].Selected;
			     NumGrpSel++)
			   if (Gbl.Crs.Grps.LstGrpsSel.GrpCods[NumGrpSel] == GrpCodInThisEvent)
			      Events->Lst[NumAttEvent].Selected = true;
		    }
	       else			// This event is not associated to groups
		  Events->Lst[NumAttEvent].Selected = true;

	       /***** Free structure that stores the query result *****/
	       DB_FreeMySQLResult (&mysql_res);
	      }
	   }
     }
  }

/*****************************************************************************/
/******* Put contextual icons when listing my assistance (as student) ********/
/*****************************************************************************/

static void Att_PutIconsMyAttList (void *Events)
  {
   if (Events)
     {
      /***** Put icon to print my assistance (as student) to several events *****/
      Ico_PutContextualIconToPrint (ActPrnLstMyAtt,
				    Att_PutFormToPrintMyListParams,Events);

      /***** Put icon to print my QR code *****/
      QR_PutLinkToPrintQRCode (ActPrnUsrQR,
			       Usr_PutParamMyUsrCodEncrypted,Gbl.Usrs.Me.UsrDat.EnUsrCod);
     }
  }

static void Att_PutFormToPrintMyListParams (void *Events)
  {
   if (Events)
     {
      if (((struct Att_Events *) Events)->ShowDetails)
	 Par_PutHiddenParamChar ("ShowDetails",'Y');
      if (((struct Att_Events *) Events)->StrAttCodsSelected)
	 if (((struct Att_Events *) Events)->StrAttCodsSelected[0])
	    Par_PutHiddenParamString (NULL,"AttCods",((struct Att_Events *) Events)->StrAttCodsSelected);
     }
  }

/*****************************************************************************/
/******** Put icon to print assistance of students to several events *********/
/*****************************************************************************/

static void Att_PutIconsStdsAttList (void *Events)
  {
   if (Events)
     {
      /***** Put icon to print assistance of students to several events *****/
      Ico_PutContextualIconToPrint (ActPrnLstUsrAtt,
				    Att_PutParamsToPrintStdsList,Events);

      /***** Put icon to print my QR code *****/
      QR_PutLinkToPrintQRCode (ActPrnUsrQR,
			       Usr_PutParamMyUsrCodEncrypted,Gbl.Usrs.Me.UsrDat.EnUsrCod);
     }
  }

static void Att_PutParamsToPrintStdsList (void *Events)
  {
   if (Events)
     {
      if (((struct Att_Events *) Events)->ShowDetails)
	 Par_PutHiddenParamChar ("ShowDetails",'Y');
      Grp_PutParamsCodGrps ();
      Usr_PutHiddenParSelectedUsrsCods (&Gbl.Usrs.Selected);
      if (((struct Att_Events *) Events)->StrAttCodsSelected)
	 if (((struct Att_Events *) Events)->StrAttCodsSelected[0])
	    Par_PutHiddenParamString (NULL,"AttCods",((struct Att_Events *) Events)->StrAttCodsSelected);
     }
  }

/*****************************************************************************/
/**** Put a link (form) to list assistance of students to several events *****/
/*****************************************************************************/

static void Att_PutButtonToShowDetails (const struct Att_Events *Events)
  {
   extern const char *Txt_Show_more_details;

   /***** Button to show more details *****/
   /* Begin form */
   Frm_BeginFormAnchor (Gbl.Action.Act,Att_ATTENDANCE_DETAILS_ID);
      Par_PutHiddenParamChar ("ShowDetails",'Y');
      Grp_PutParamsCodGrps ();
      Usr_PutHiddenParSelectedUsrsCods (&Gbl.Usrs.Selected);
      if (Events->StrAttCodsSelected)
	 if (Events->StrAttCodsSelected[0])
	    Par_PutHiddenParamString (NULL,"AttCods",Events->StrAttCodsSelected);

      /* Button */
      Btn_PutConfirmButton (Txt_Show_more_details);

   /* End form */
   Frm_EndForm ();
  }

/*****************************************************************************/
/********** Write list of those attendance events that have students *********/
/*****************************************************************************/

static void Att_ListEventsToSelect (const struct Att_Events *Events,
                                    Att_TypeOfView_t TypeOfView)
  {
   extern const char *Txt_Events;
   extern const char *Txt_Event;
   extern const char *Txt_ROLES_PLURAL_Abc[Rol_NUM_ROLES][Usr_NUM_SEXS];
   extern const char *Txt_Update_attendance;
   unsigned UniqueId;
   char *Id;
   unsigned NumAttEvent;
   bool NormalView = (TypeOfView == Att_VIEW_ONLY_ME ||
                      TypeOfView == Att_VIEW_SEL_USR);

   /***** Begin box *****/
   switch (TypeOfView)
     {
      case Att_VIEW_ONLY_ME:
	 Box_BoxBegin (NULL,Txt_Events,
		       Att_PutIconToViewAttEvents,NULL,
		       NULL,Box_NOT_CLOSABLE);
	 break;
      case Att_VIEW_SEL_USR:
	 Box_BoxBegin (NULL,Txt_Events,
		       Att_PutIconToEditAttEvents,NULL,
		       NULL,Box_NOT_CLOSABLE);
	 break;
      case Att_PRNT_ONLY_ME:
      case Att_PRNT_SEL_USR:
	 Box_BoxBegin (NULL,Txt_Events,
		       NULL,NULL,
		       NULL,Box_NOT_CLOSABLE);
	 break;
     }

      /***** Begin form to update the attendance
	     depending on the events selected *****/
      if (NormalView)
	{
	 Frm_BeginFormAnchor (Gbl.Action.Act,Att_ATTENDANCE_TABLE_ID);
	    Grp_PutParamsCodGrps ();
	    Usr_PutHiddenParSelectedUsrsCods (&Gbl.Usrs.Selected);
	}

      /***** Begin table *****/
      HTM_TABLE_BeginWidePadding (2);

	 /***** Heading row *****/
	 HTM_TR_Begin (NULL);
	    HTM_TH_Span (Txt_Event                                     ,HTM_HEAD_LEFT ,1,4,NULL);
	    HTM_TH      (Txt_ROLES_PLURAL_Abc[Rol_STD][Usr_SEX_UNKNOWN],HTM_HEAD_RIGHT);
	 HTM_TR_End ();

	 /***** List the events *****/
	 for (NumAttEvent = 0, UniqueId = 1;
	      NumAttEvent < Events->Num;
	      NumAttEvent++, UniqueId++, The_ChangeRowColor ())
	   {
	    /* Get data of the attendance event from database */
	    Att_GetDataOfAttEventByCodAndCheckCrs (&Events->Lst[NumAttEvent]);
	    Events->Lst[NumAttEvent].NumStdsTotal = Att_DB_GetNumStdsTotalWhoAreInAttEvent (Events->Lst[NumAttEvent].AttCod);

	    /* Write a row for this event */
	    HTM_TR_Begin (NULL);

	       HTM_TD_Begin ("class=\"CT DAT_%s %s\"",
	                     The_GetSuffix (),
	                     The_GetColorRows ());
		  HTM_INPUT_CHECKBOX ("AttCods",HTM_DONT_SUBMIT_ON_CHANGE,
				      "id=\"Event%u\" value=\"%ld\"%s",
				      NumAttEvent,Events->Lst[NumAttEvent].AttCod,
				      Events->Lst[NumAttEvent].Selected ? " checked=\"checked\"" :
									  "");
	       HTM_TD_End ();

	       HTM_TD_Begin ("class=\"RT DAT_%s %s\"",
	                     The_GetSuffix (),
	                     The_GetColorRows ());
		  HTM_LABEL_Begin ("for=\"Event%u\"",NumAttEvent);
		     HTM_TxtF ("%u:",NumAttEvent + 1);
		  HTM_LABEL_End ();
	       HTM_TD_End ();

	       HTM_TD_Begin ("class=\"LT DAT_%s %s\"",
	                     The_GetSuffix (),
	                     The_GetColorRows ());
		  if (asprintf (&Id,"att_date_start_%u",UniqueId) < 0)
		     Err_NotEnoughMemoryExit ();
		  HTM_LABEL_Begin ("for=\"Event%u\"",NumAttEvent);
		     HTM_SPAN_Begin ("id=\"%s\"",Id);
		     HTM_SPAN_End ();
		  HTM_LABEL_End ();
		  Dat_WriteLocalDateHMSFromUTC (Id,Events->Lst[NumAttEvent].TimeUTC[Dat_STR_TIME],
						Gbl.Prefs.DateFormat,Dat_SEPARATOR_COMMA,
						true,true,true,0x7);
		  free (Id);
	       HTM_TD_End ();

	       HTM_TD_Begin ("class=\"LT DAT_%s %s\"",
	                     The_GetSuffix (),
	                     The_GetColorRows ());
		  HTM_Txt (Events->Lst[NumAttEvent].Title);
	       HTM_TD_End ();

	       HTM_TD_Begin ("class=\"RT DAT_%s %s\"",
	                     The_GetSuffix (),
	                     The_GetColorRows ());
		  HTM_Unsigned (Events->Lst[NumAttEvent].NumStdsTotal);
	       HTM_TD_End ();

	    HTM_TR_End ();
	   }

      /***** End table *****/
      HTM_TABLE_End ();

      /***** Put button to refresh *****/
      if (NormalView)
	{
	 HTM_DIV_Begin ("class=\"UPD\"");
	    HTM_BUTTON_Submit_Begin (Txt_Update_attendance,
				     "class=\"BT_LINK FORM_IN_%s BOLD\"",
				     The_GetSuffix ());
	       Ico_PutIconTextLink ("recycle.svg",Ico_BLACK,Txt_Update_attendance);
	    HTM_BUTTON_End ();
	 HTM_DIV_End ();
	}

      /***** End form *****/
      if (NormalView)
	 Frm_EndForm ();

   /***** End box *****/
   Box_BoxEnd ();
  }

/*****************************************************************************/
/*********** Put icon to list (without edition) attendance events ************/
/*****************************************************************************/

static void Att_PutIconToViewAttEvents (__attribute__((unused)) void *Args)
  {
   Ico_PutContextualIconToView (ActSeeAtt,NULL,
				NULL,NULL);
  }

/*****************************************************************************/
/************ Put icon to list (with edition) attendance events **************/
/*****************************************************************************/

static void Att_PutIconToEditAttEvents (__attribute__((unused)) void *Args)
  {
   Ico_PutContextualIconToEdit (ActSeeAtt,NULL,
				NULL,NULL);
  }

/*****************************************************************************/
/************ Show table with attendances for every user in list *************/
/*****************************************************************************/

static void Att_ListUsrsAttendanceTable (const struct Att_Events *Events,
                                         Att_TypeOfView_t TypeOfView,
	                                 unsigned NumUsrsInList,
                                         long *LstSelectedUsrCods)
  {
   extern const char *Txt_Number_of_users;
   struct UsrData UsrDat;
   unsigned NumUsr;
   unsigned NumAttEvent;
   unsigned Total;
   bool PutButtonShowDetails = (TypeOfView == Att_VIEW_ONLY_ME ||
	                        TypeOfView == Att_VIEW_SEL_USR) &&
	                        !Events->ShowDetails;

   /***** Initialize structure with user's data *****/
   Usr_UsrDataConstructor (&UsrDat);

   /***** Begin section with attendance table *****/
   HTM_SECTION_Begin (Att_ATTENDANCE_TABLE_ID);

      /***** Begin table *****/
      HTM_TABLE_BeginCenterPadding (2);

	 /***** Heading row *****/
	 Att_WriteTableHeadSeveralAttEvents (Events);

	 /***** List the users *****/
	 for (NumUsr = 0;
	      NumUsr < NumUsrsInList;
	      NumUsr++)
	   {
	    UsrDat.UsrCod = LstSelectedUsrCods[NumUsr];
	    if (Usr_ChkUsrCodAndGetAllUsrDataFromUsrCod (&UsrDat,		// Get from the database the data of the student
							 Usr_DONT_GET_PREFS,
							 Usr_DONT_GET_ROLE_IN_CURRENT_CRS))
	       if (Usr_CheckIfICanViewAtt (&UsrDat))
		 {
		  UsrDat.Accepted = Enr_CheckIfUsrHasAcceptedInCurrentCrs (&UsrDat);
		  Att_WriteRowUsrSeveralAttEvents (Events,NumUsr,&UsrDat);
		 }
	   }

	 /***** Last row with the total of users present in each event *****/
	 if (NumUsrsInList > 1)
	   {
	    HTM_TR_Begin (NULL);

	       HTM_TD_Begin ("colspan=\"%u\" class=\"RM DAT_STRONG_%s LINE_TOP\"",
			     Gbl.Usrs.Listing.WithPhotos ? 4 :
							   3,
			     The_GetSuffix ());
		  HTM_TxtColon (Txt_Number_of_users);
	       HTM_TD_End ();

	       for (NumAttEvent = 0, Total = 0;
		    NumAttEvent < Events->Num;
		    NumAttEvent++)
		  if (Events->Lst[NumAttEvent].Selected)
		    {
		     HTM_TD_Begin ("class=\"RM DAT_STRONG_%s LINE_TOP\"",
		                   The_GetSuffix ());
			HTM_Unsigned (Events->Lst[NumAttEvent].NumStdsFromList);
		     HTM_TD_End ();

		     Total += Events->Lst[NumAttEvent].NumStdsFromList;
		    }

	       HTM_TD_Begin ("class=\"RM DAT_STRONG_%s LINE_TOP\"",
	                     The_GetSuffix ());
		  HTM_Unsigned (Total);
	       HTM_TD_End ();

	    HTM_TR_End ();
	   }

      /***** End table *****/
      HTM_TABLE_End ();

      /***** Button to show more details *****/
      if (PutButtonShowDetails)
	 Att_PutButtonToShowDetails (Events);

   /***** End section with attendance table *****/
   HTM_SECTION_End ();

   /***** Free memory used for user's data *****/
   Usr_UsrDataDestructor (&UsrDat);
  }

/*****************************************************************************/
/* Write table heading for listing of students in several attendance events **/
/*****************************************************************************/

static void Att_WriteTableHeadSeveralAttEvents (const struct Att_Events *Events)
  {
   extern const char *Txt_ROLES_SINGUL_Abc[Rol_NUM_ROLES][Usr_NUM_SEXS];
   extern const char *Txt_Attendance;
   unsigned NumAttEvent;
   char StrNumAttEvent[Cns_MAX_DECIMAL_DIGITS_UINT + 1];

   HTM_TR_Begin (NULL);

      HTM_TH_Span (Txt_ROLES_SINGUL_Abc[Rol_USR][Usr_SEX_UNKNOWN],HTM_HEAD_LEFT,
	           1,Gbl.Usrs.Listing.WithPhotos ? 4 :
					           3,NULL);

      for (NumAttEvent = 0;
	   NumAttEvent < Events->Num;
	   NumAttEvent++)
	 if (Events->Lst[NumAttEvent].Selected)
	   {
	    /***** Get data of this attendance event *****/
	    Att_GetDataOfAttEventByCodAndCheckCrs (&Events->Lst[NumAttEvent]);

	    /***** Put link to this attendance event *****/
            HTM_TH_Begin (HTM_HEAD_CENTER);
	       snprintf (StrNumAttEvent,sizeof (StrNumAttEvent),"%u",NumAttEvent + 1);
	       Att_PutLinkAttEvent (&Events->Lst[NumAttEvent],
				    Events->Lst[NumAttEvent].Title,
				    StrNumAttEvent);
	    HTM_TH_End ();
	   }

      HTM_TH (Txt_Attendance ,HTM_HEAD_RIGHT);

   HTM_TR_End ();
  }

/*****************************************************************************/
/************** Write a row of a table with the data of a user ***************/
/*****************************************************************************/

static void Att_WriteRowUsrSeveralAttEvents (const struct Att_Events *Events,
                                             unsigned NumUsr,struct UsrData *UsrDat)
  {
   static const char *ClassPhoto[PhoSha_NUM_SHAPES] =
     {
      [PhoSha_SHAPE_CIRCLE   ] = "PHOTOC21x28",
      [PhoSha_SHAPE_ELLIPSE  ] = "PHOTOE21x28",
      [PhoSha_SHAPE_OVAL     ] = "PHOTOO21x28",
      [PhoSha_SHAPE_RECTANGLE] = "PHOTOR21x28",
     };
   unsigned NumAttEvent;
   bool Present;
   unsigned NumTimesPresent;

   /***** Write number of user in the list *****/
   HTM_TR_Begin (NULL);

      HTM_TD_Begin ("class=\"RM %s_%s %s\"",
		    UsrDat->Accepted ? "DAT_STRONG" :
				       "DAT",
		    The_GetSuffix (),
		    The_GetColorRows ());
	 HTM_Unsigned (NumUsr + 1);
      HTM_TD_End ();

      /***** Show user's photo *****/
      if (Gbl.Usrs.Listing.WithPhotos)
	{
	 HTM_TD_Begin ("class=\"LM %s\"",The_GetColorRows ());
	    Pho_ShowUsrPhotoIfAllowed (UsrDat,
	                               ClassPhoto[Gbl.Prefs.PhotoShape],Pho_ZOOM,
	                               false);
	 HTM_TD_End ();
	}

      /***** Write user's ID ******/
      HTM_TD_Begin ("class=\"LM %s_%s %s\"",
		    UsrDat->Accepted ? "DAT_SMALL_STRONG" :
				       "DAT_SMALL",
		    The_GetSuffix (),
		    The_GetColorRows ());
	 ID_WriteUsrIDs (UsrDat,NULL);
      HTM_TD_End ();

      /***** Write user's name *****/
      HTM_TD_Begin ("class=\"LM %s_%s %s\"",
		    UsrDat->Accepted ? "DAT_SMALL_STRONG" :
				       "DAT_SMALL",
		    The_GetSuffix (),
		    The_GetColorRows ());
	 HTM_Txt (UsrDat->Surname1);
	 if (UsrDat->Surname2[0])
	    HTM_TxtF ("&nbsp;%s",UsrDat->Surname2);
	 HTM_TxtF (", %s",UsrDat->FrstName);
      HTM_TD_End ();

      /***** Check/cross to show if the user is present/absent *****/
      for (NumAttEvent = 0, NumTimesPresent = 0;
	   NumAttEvent < Events->Num;
	   NumAttEvent++)
	 if (Events->Lst[NumAttEvent].Selected)
	   {
	    /* Check if this student is already registered in the current event */
	    // Here it is not necessary to get comments
	    Present = Att_CheckIfUsrIsPresentInAttEvent (Events->Lst[NumAttEvent].AttCod,
							 UsrDat->UsrCod);

	    /* Write check or cross */
	    HTM_TD_Begin ("class=\"BM %s\"",The_GetColorRows ());
	       Att_PutCheckOrCross (Present);
	    HTM_TD_End ();

	    if (Present)
	       NumTimesPresent++;
	   }

      /***** Last column with the number of times this user is present *****/
      HTM_TD_Begin ("class=\"RM DAT_STRONG_%s %s\"",
                    The_GetSuffix (),The_GetColorRows ());
	 HTM_Unsigned (NumTimesPresent);
      HTM_TD_End ();

   HTM_TR_End ();

   The_ChangeRowColor ();
  }

/*****************************************************************************/
/*********************** Put check or cross character ************************/
/*****************************************************************************/

static void Att_PutCheckOrCross (bool Present)
  {
   extern const char *Txt_Present;
   extern const char *Txt_Absent;

   if (Present)
     {
      HTM_DIV_Begin ("class=\"ATT_CHECK ATT_CHECK_%s\" title=\"%s\"",
                     The_GetSuffix (),Txt_Present);
	 HTM_Txt ("&check;");
     }
   else
     {
      HTM_DIV_Begin ("class=\"ATT_CROSS ATT_CROSS_%s\" title=\"%s\"",
                     The_GetSuffix (),Txt_Absent);
	 HTM_Txt ("&cross;");
     }

   HTM_DIV_End ();
  }

/*****************************************************************************/
/**************** List the students with details and comments ****************/
/*****************************************************************************/

static void Att_ListStdsWithAttEventsDetails (const struct Att_Events *Events,
                                              unsigned NumUsrsInList,
                                              long *LstSelectedUsrCods)
  {
   extern const char *Txt_Details;
   struct UsrData UsrDat;
   unsigned NumUsr;

   /***** Initialize structure with user's data *****/
   Usr_UsrDataConstructor (&UsrDat);

   /***** Begin section with attendance details *****/
   HTM_SECTION_Begin (Att_ATTENDANCE_DETAILS_ID);

      /***** Begin box and table *****/
      Box_BoxTableBegin (NULL,Txt_Details,
			 NULL,NULL,
			 NULL,Box_NOT_CLOSABLE,2);

	 /***** List students with attendance details *****/
	 for (NumUsr = 0;
	      NumUsr < NumUsrsInList;
	      NumUsr++)
	   {
	    UsrDat.UsrCod = LstSelectedUsrCods[NumUsr];
	    if (Usr_ChkUsrCodAndGetAllUsrDataFromUsrCod (&UsrDat,	// Get from the database the data of the student
							 Usr_DONT_GET_PREFS,
							 Usr_DONT_GET_ROLE_IN_CURRENT_CRS))
	       if (Usr_CheckIfICanViewAtt (&UsrDat))
		 {
		  UsrDat.Accepted = Enr_CheckIfUsrHasAcceptedInCurrentCrs (&UsrDat);
		  Att_ListAttEventsForAStd (Events,NumUsr,&UsrDat);
		 }
	   }

      /***** End table and box *****/
      Box_BoxTableEnd ();

   /***** End section with attendance details *****/
   HTM_SECTION_End ();

   /***** Free memory used for user's data *****/
   Usr_UsrDataDestructor (&UsrDat);
  }

/*****************************************************************************/
/*************** Write list of attendance events for a student ***************/
/*****************************************************************************/

static void Att_ListAttEventsForAStd (const struct Att_Events *Events,
                                      unsigned NumUsr,struct UsrData *UsrDat)
  {
   extern const char *Txt_Student_comment;
   extern const char *Txt_Teachers_comment;
   static const char *ClassPhoto[PhoSha_NUM_SHAPES] =
     {
      [PhoSha_SHAPE_CIRCLE   ] = "PHOTOC21x28",
      [PhoSha_SHAPE_ELLIPSE  ] = "PHOTOE21x28",
      [PhoSha_SHAPE_OVAL     ] = "PHOTOO21x28",
      [PhoSha_SHAPE_RECTANGLE] = "PHOTOR21x28",
     };
   unsigned NumAttEvent;
   unsigned UniqueId;
   char *Id;
   bool Present;
   bool ShowCommentStd;
   bool ShowCommentTch;
   char CommentStd[Cns_MAX_BYTES_TEXT + 1];
   char CommentTch[Cns_MAX_BYTES_TEXT + 1];

   /***** Write number of student in the list *****/
   NumUsr++;
   HTM_TR_Begin (NULL);

      HTM_TD_Begin ("class=\"RM %s_%s %s\"",
		    UsrDat->Accepted ? "DAT_STRONG" :
				       "DAT",
		    The_GetSuffix (),
		    The_GetColorRows ());
	 HTM_TxtF ("%u:",NumUsr);
      HTM_TD_End ();

      /***** Show student's photo *****/
      HTM_TD_Begin ("colspan=\"2\" class=\"RM %s\"",
                    The_GetColorRows ());
	 Pho_ShowUsrPhotoIfAllowed (UsrDat,
	                            ClassPhoto[Gbl.Prefs.PhotoShape],Pho_ZOOM,
	                            false);
      HTM_TD_End ();

      HTM_TD_Begin ("class=\"LM %s\"",The_GetColorRows ());

	 HTM_TABLE_Begin (NULL);
	    HTM_TR_Begin (NULL);

	       /***** Write user's ID ******/
	       HTM_TD_Begin ("class=\"LM %s_%s\"",
			     UsrDat->Accepted ? "DAT_SMALL_STRONG" :
						"DAT_SMALL",
			     The_GetSuffix ());
		  ID_WriteUsrIDs (UsrDat,NULL);
	       HTM_TD_End ();

	       /***** Write student's name *****/
	       HTM_TD_Begin ("class=\"LM %s_%s\"",
			     UsrDat->Accepted ? "DAT_SMALL_STRONG" :
						"DAT_SMALL",
			     The_GetSuffix ());
		  HTM_Txt (UsrDat->Surname1);
		  if (UsrDat->Surname2[0])
		     HTM_TxtF ("&nbsp;%s",UsrDat->Surname2);
		  HTM_TxtF (", %s",UsrDat->FrstName);
	       HTM_TD_End ();

	    HTM_TR_End ();
	 HTM_TABLE_End ();

      HTM_TD_End ();

   HTM_TR_End ();

   /***** List the events with students *****/
   for (NumAttEvent = 0, UniqueId = 1;
	NumAttEvent < Events->Num;
	NumAttEvent++, UniqueId++)
      if (Events->Lst[NumAttEvent].Selected)
	{
	 /***** Get data of the attendance event from database *****/
	 Att_GetDataOfAttEventByCodAndCheckCrs (&Events->Lst[NumAttEvent]);
         Events->Lst[NumAttEvent].NumStdsTotal = Att_DB_GetNumStdsTotalWhoAreInAttEvent (Events->Lst[NumAttEvent].AttCod);

	 /***** Get comments for this student *****/
	 Present = Att_CheckIfUsrIsPresentInAttEventAndGetComments (Events->Lst[NumAttEvent].AttCod,UsrDat->UsrCod,CommentStd,CommentTch);
         ShowCommentStd = CommentStd[0];
	 ShowCommentTch = CommentTch[0] &&
	                  (Gbl.Usrs.Me.Role.Logged == Rol_TCH ||
	                   Events->Lst[NumAttEvent].CommentTchVisible);

	 /***** Write a row for this event *****/
	 HTM_TR_Begin (NULL);

	    HTM_TD_ColouredEmpty (1);

	    HTM_TD_Begin ("class=\"RT %s_%s %s\"",
			  Present ? "DAT_GREEN" :
				    "DAT_RED",
			  The_GetSuffix (),
			  The_GetColorRows ());
	       HTM_TxtF ("%u:",NumAttEvent + 1);
	    HTM_TD_End ();

	    HTM_TD_Begin ("class=\"BT %s\"",The_GetColorRows ());
	       Att_PutCheckOrCross (Present);
	    HTM_TD_End ();

	    HTM_TD_Begin ("class=\"LT DAT_%s %s\"",
	                  The_GetSuffix (),
	                  The_GetColorRows ());
	       if (asprintf (&Id,"att_date_start_%u_%u",NumUsr,UniqueId) < 0)
		  Err_NotEnoughMemoryExit ();
	       HTM_SPAN_Begin ("id=\"%s\"",Id);
	       HTM_SPAN_End ();
	       HTM_BR ();
	       HTM_Txt (Events->Lst[NumAttEvent].Title);
	       Dat_WriteLocalDateHMSFromUTC (Id,Events->Lst[NumAttEvent].TimeUTC[Dat_STR_TIME],
					     Gbl.Prefs.DateFormat,Dat_SEPARATOR_COMMA,
					     true,true,true,0x7);
	       free (Id);
	    HTM_TD_End ();

	 HTM_TR_End ();

	 /***** Write comments for this student *****/
	 if (ShowCommentStd || ShowCommentTch)
	   {
	    HTM_TR_Begin (NULL);

	       HTM_TD_ColouredEmpty (2);

	       HTM_TD_Begin ("class=\"BT %s\"",The_GetColorRows ());
	       HTM_TD_End ();

	       HTM_TD_Begin ("class=\"LM DAT_%s %s\"",
	                     The_GetSuffix (),The_GetColorRows ());

		  HTM_DL_Begin ();
		     if (ShowCommentStd)
		       {
			Str_ChangeFormat (Str_FROM_HTML,Str_TO_RIGOROUS_HTML,
					  CommentStd,Cns_MAX_BYTES_TEXT,false);
			HTM_DT_Begin ();
			   HTM_TxtColon (Txt_Student_comment);
			HTM_DT_End ();
			HTM_DD_Begin ();
			   HTM_Txt (CommentStd);
			HTM_DD_End ();
		       }
		     if (ShowCommentTch)
		       {
			Str_ChangeFormat (Str_FROM_HTML,Str_TO_RIGOROUS_HTML,
					  CommentTch,Cns_MAX_BYTES_TEXT,false);
			HTM_DT_Begin ();
			   HTM_TxtColon (Txt_Teachers_comment);
			HTM_DT_End ();
			HTM_DD_Begin ();
			   HTM_Txt (CommentTch);
			HTM_DD_End ();
		       }
		  HTM_DL_End ();

	       HTM_TD_End ();

	    HTM_TR_End ();
	   }
	}

   The_ChangeRowColor ();
  }

/*****************************************************************************/
/************************ Get link to attendance event ***********************/
/*****************************************************************************/

void Att_GetLinkToEvent (void)
  {
   extern const char *Txt_Link_to_resource_X_copied_into_clipboard;
   long AttCod;
   char Title[Att_MAX_BYTES_ATTENDANCE_EVENT_TITLE + 1];

   /***** Get attendance event code *****/
   if ((AttCod = Att_GetParamAttCod ()) < 0)
      Err_WrongEventExit ();

   /***** Get attendance event title *****/
   Att_DB_GetAttEventTitle (AttCod,Title);

   /***** Copy link to attendance event into resource clipboard *****/
   Prg_DB_CopyToClipboard (PrgRsc_ATTENDANCE_EVENT,AttCod);

   /***** Write sucess message *****/
   Ale_ShowAlert (Ale_SUCCESS,Txt_Link_to_resource_X_copied_into_clipboard,
   		  Title);

   /***** Show attendance events again *****/
   Att_SeeAttEvents ();
  }

/*****************************************************************************/
/**************** Write attendance event in course program *******************/
/*****************************************************************************/

void Att_WriteAttEventInCrsProgram (long AttCod,bool PutFormToGo)
  {
   extern const char *Txt_Actions[Act_NUM_ACTIONS];
   char Title[Att_MAX_BYTES_ATTENDANCE_EVENT_TITLE + 1];

   /***** Get game title *****/
   Att_DB_GetAttEventTitle (AttCod,Title);

   /***** Begin form to go to game *****/
   if (PutFormToGo)
     {
      Frm_BeginForm (ActSeeOneAtt);
         Att_PutParamAttCod (AttCod);
	 HTM_BUTTON_Submit_Begin (Txt_Actions[ActSeeOneAtt],
	                          "class=\"LM BT_LINK PRG_RSC_%s\"",
	                          The_GetSuffix ());
     }

   /***** Write attendance event title *****/
   HTM_Txt (Title);

   /***** End form to download file *****/
   if (PutFormToGo)
     {
      /* End form */
         HTM_BUTTON_End ();

      Frm_EndForm ();
     }
  }

/*****************************************************************************/
/*************** Get attendance event title from game code *******************/
/*****************************************************************************/

void Att_GetTitleFromAttCod (long AttCod,char *Title,size_t TitleSize)
  {
   char TitleFromDB[Att_MAX_BYTES_ATTENDANCE_EVENT_TITLE + 1];

   /***** Get attendance event title *****/
   Att_DB_GetAttEventTitle (AttCod,TitleFromDB);
   Str_Copy (Title,TitleFromDB,TitleSize);
  }
