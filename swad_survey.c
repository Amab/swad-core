// swad_survey.c: surveys

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
#include <stdlib.h>		// For calloc
#include <string.h>		// For string functions

#include "swad_attendance.h"
#include "swad_autolink.h"
#include "swad_box.h"
#include "swad_database.h"
#include "swad_error.h"
#include "swad_figure.h"
#include "swad_form.h"
#include "swad_global.h"
#include "swad_group.h"
#include "swad_group_database.h"
#include "swad_HTML.h"
#include "swad_notification.h"
#include "swad_notification_database.h"
#include "swad_pagination.h"
#include "swad_parameter.h"
#include "swad_role.h"
#include "swad_setting.h"
#include "swad_survey.h"
#include "swad_survey_database.h"

/*****************************************************************************/
/************** External global variables from others modules ****************/
/*****************************************************************************/

extern struct Globals Gbl;

/*****************************************************************************/
/***************************** Private constants *****************************/
/*****************************************************************************/

#define Svy_MAX_CHARS_ANSWER	(1024 - 1)	// 1023
#define Svy_MAX_BYTES_ANSWER	((Svy_MAX_CHARS_ANSWER + 1) * Str_MAX_BYTES_PER_CHAR - 1)	// 16383

#define Svy_MAX_BYTES_LIST_ANSWER_TYPES	(Svy_NUM_ANS_TYPES * (Cns_MAX_DECIMAL_DIGITS_UINT + 1))

#define Svy_MAX_ANSWERS_PER_QUESTION	10

struct Svy_Question	// Must be initialized to 0 before using it
  {
   long QstCod;
   unsigned QstInd;
   Svy_AnswerType_t AnswerType;
   struct
     {
      char *Text;
     } AnsChoice[Svy_MAX_ANSWERS_PER_QUESTION];
   bool AllAnsTypes;
   char ListAnsTypes[Svy_MAX_BYTES_LIST_ANSWER_TYPES + 1];
  };

/*****************************************************************************/
/***************************** Private prototypes ****************************/
/*****************************************************************************/

static void Svy_ResetSurveys (struct Svy_Surveys *Surveys);

static void Svy_ListAllSurveys (struct Svy_Surveys *Surveys);
static bool Svy_CheckIfICanCreateSvy (void);
static void Svy_PutIconsListSurveys (void *Surveys);
static void Svy_PutIconToCreateNewSvy (struct Svy_Surveys *Surveys);
static void Svy_PutButtonToCreateNewSvy (struct Svy_Surveys *Surveys);
static void Svy_PutParamsToCreateNewSvy (void *Surveys);
static void Svy_ParamsWhichGroupsToShow (void *Surveys);
static void Svy_ShowOneSurvey (struct Svy_Surveys *Surveys,
                               long SvyCod,bool ShowOnlyThisSvyComplete);
static void Svy_WriteAuthor (struct Svy_Survey *Svy);
static void Svy_WriteStatus (struct Svy_Survey *Svy);
static Dat_StartEndTime_t Svy_GetParamSvyOrder (void);

static void Svy_PutFormsToRemEditOneSvy (struct Svy_Surveys *Surveys,
					 const struct Svy_Survey *Svy,
                                         const char *Anchor);
static void Svy_PutParams (void *Surveys);

static void Svy_GetListSurveys (struct Svy_Surveys *Surveys);

static void Svy_SetAllowedAndHiddenScopes (unsigned *ScopesAllowed,
                                           unsigned *HiddenAllowed);

static void Svy_PutParamSvyCod (long SvyCod);
static long Svy_GetParamSvyCod (void);

static void Svy_PutButtonToResetSurvey (struct Svy_Surveys *Surveys);

static void Svy_SetDefaultAndAllowedScope (struct Svy_Survey *Svy);
static void Svy_ShowLstGrpsToEditSurvey (long SvyCod);
static void Svy_CreateSurvey (struct Svy_Survey *Svy,const char *Txt);
static void Svy_UpdateSurvey (struct Svy_Survey *Svy,const char *Txt);
static void Svy_CreateGrps (long SvyCod);
static void Svy_GetAndWriteNamesOfGrpsAssociatedToSvy (struct Svy_Survey *Svy);

static void Svy_ShowFormEditOneQst (struct Svy_Surveys *Surveys,
                                    long SvyCod,struct Svy_Question *SvyQst,
                                    char Stem[Cns_MAX_BYTES_TEXT + 1]);
static void Svy_InitQst (struct Svy_Question *SvyQst);
static void Svy_PutParamQstCod (long QstCod);
static long Svy_GetParamQstCod (void);
static bool Svy_AllocateTextChoiceAnswer (struct Svy_Question *SvyQst,unsigned NumAns);
static void Svy_FreeTextChoiceAnswers (struct Svy_Question *SvyQst,unsigned NumAnswers);
static void Svy_FreeTextChoiceAnswer (struct Svy_Question *SvyQst,unsigned NumAns);

static unsigned Svy_GetNextQuestionIndexInSvy (long SvyCod);
static void Svy_ListSvyQuestions (struct Svy_Surveys *Surveys,
                                  struct Svy_Survey *Svy);
static void Svy_GetDataOfQstFromRow (struct Svy_Question *SvyQst,
                                     char Stem[Cns_MAX_BYTES_TEXT + 1],
                                     MYSQL_ROW row);
static void Svy_PutParamsToEditQuestion (void *Surveys);
static void Svy_PutIconToAddNewQuestion (void *Surveys);
static void Svy_PutButtonToCreateNewQuestion (struct Svy_Surveys *Surveys);
static void Svy_WriteQstStem (const char *Stem);
static void Svy_WriteAnswersOfAQst (struct Svy_Survey *Svy,
                                    struct Svy_Question *SvyQst,
                                    bool PutFormAnswerSurvey);
static void Svy_DrawBarNumUsrs (unsigned NumUsrs,unsigned MaxUsrs);

static void Svy_PutIconToRemoveOneQst (void *Surveys);
static void Svy_PutParamsRemoveOneQst (void *Surveys);

static void Svy_ReceiveAndStoreUserAnswersToASurvey (long SvyCod);

/*****************************************************************************/
/*************************** Reset surveys context ***************************/
/*****************************************************************************/

static void Svy_ResetSurveys (struct Svy_Surveys *Surveys)
  {
   Surveys->LstIsRead     = false;	// Is the list already read from database, or it needs to be read?
   Surveys->Num           = 0;		// Number of surveys
   Surveys->LstSvyCods    = NULL;	// List of survey codes
   Surveys->SelectedOrder = Svy_ORDER_DEFAULT;
   Surveys->CurrentPage   = 0;
   Surveys->SvyCod        = -1L;
   Surveys->QstCod        = -1L;
  }

/*****************************************************************************/
/***************************** List all surveys ******************************/
/*****************************************************************************/

void Svy_SeeAllSurveys (void)
  {
   struct Svy_Surveys Surveys;

   /***** Reset surveys *****/
   Svy_ResetSurveys (&Surveys);

   /***** Get parameters *****/
   Surveys.SelectedOrder = Svy_GetParamSvyOrder ();
   Grp_GetParamWhichGroups ();
   Surveys.CurrentPage = Pag_GetParamPagNum (Pag_SURVEYS);

   /***** Show all surveys *****/
   Svy_ListAllSurveys (&Surveys);
  }

/*****************************************************************************/
/***************************** Show all surveys ******************************/
/*****************************************************************************/

static void Svy_ListAllSurveys (struct Svy_Surveys *Surveys)
  {
   extern const char *Hlp_ASSESSMENT_Surveys;
   extern const char *Txt_Surveys;
   extern const char *Txt_START_END_TIME_HELP[Dat_NUM_START_END_TIME];
   extern const char *Txt_START_END_TIME[Dat_NUM_START_END_TIME];
   extern const char *Txt_Survey;
   extern const char *Txt_Status;
   extern const char *Txt_No_surveys;
   Dat_StartEndTime_t Order;
   Grp_WhichGroups_t WhichGroups;
   struct Pagination Pagination;
   unsigned NumSvy;

   /***** Get number of groups in current course *****/
   if (!Gbl.Crs.Grps.NumGrps)
      Gbl.Crs.Grps.WhichGrps = Grp_ALL_GROUPS;

   /***** Get list of surveys *****/
   Svy_GetListSurveys (Surveys);

   /***** Compute variables related to pagination *****/
   Pagination.NumItems = Surveys->Num;
   Pagination.CurrentPage = (int) Surveys->CurrentPage;
   Pag_CalculatePagination (&Pagination);
   Surveys->CurrentPage = (unsigned) Pagination.CurrentPage;

   /***** Begin box *****/
   Box_BoxBegin ("100%",Txt_Surveys,
                 Svy_PutIconsListSurveys,Surveys,
                 Hlp_ASSESSMENT_Surveys,Box_NOT_CLOSABLE);

      /***** Select whether show only my groups or all groups *****/
      if (Gbl.Crs.Grps.NumGrps)
	{
	 Set_BeginSettingsHead ();
	    Grp_ShowFormToSelWhichGrps (ActSeeAllSvy,
					Svy_ParamsWhichGroupsToShow,Surveys);
	 Set_EndSettingsHead ();
	}

      /***** Write links to pages *****/
      Pag_WriteLinksToPagesCentered (Pag_SURVEYS,&Pagination,
				     Surveys,-1L);

      if (Surveys->Num)
	{
	 /***** Table head *****/
	 HTM_TABLE_BeginWideMarginPadding (5);
	    HTM_TR_Begin (NULL);

               HTM_TH_Span (NULL,HTM_HEAD_CENTER,1,1,"CONTEXT_COL");	// Column for contextual icons

	       for (Order  = (Dat_StartEndTime_t) 0;
		    Order <= (Dat_StartEndTime_t) (Dat_NUM_START_END_TIME - 1);
		    Order++)
		 {
                  HTM_TH_Begin (HTM_HEAD_LEFT);

		     /* Form to change order */
		     Frm_BeginForm (ActSeeAllSvy);
			WhichGroups = Grp_GetParamWhichGroups ();
			Grp_PutParamWhichGroups (&WhichGroups);
			Pag_PutHiddenParamPagNum (Pag_SURVEYS,Surveys->CurrentPage);
			Dat_PutHiddenParamOrder (Order);
			HTM_BUTTON_Submit_Begin (Txt_START_END_TIME_HELP[Order],
			                         "class=\"BT_LINK\"");
			   if (Order == Surveys->SelectedOrder)
			      HTM_U_Begin ();
			   HTM_Txt (Txt_START_END_TIME[Order]);
			   if (Order == Surveys->SelectedOrder)
			      HTM_U_End ();
			HTM_BUTTON_End ();
		     Frm_EndForm ();

		  HTM_TH_End ();
		 }

	       HTM_TH (Txt_Survey,HTM_HEAD_LEFT  );
	       HTM_TH (Txt_Status,HTM_HEAD_CENTER);

	    HTM_TR_End ();

	    /***** Write all surveys *****/
	    for (NumSvy  = Pagination.FirstItemVisible;
		 NumSvy <= Pagination.LastItemVisible;
		 NumSvy++)
	       Svy_ShowOneSurvey (Surveys,Surveys->LstSvyCods[NumSvy - 1],false);

	 /***** End table *****/
	 HTM_TABLE_End ();
	}
      else	// No surveys created
	 Ale_ShowAlert (Ale_INFO,Txt_No_surveys);

      /***** Write again links to pages *****/
      Pag_WriteLinksToPagesCentered (Pag_SURVEYS,&Pagination,
				     Surveys,-1L);

      /***** Button to create a new survey *****/
      if (Svy_CheckIfICanCreateSvy ())
	 Svy_PutButtonToCreateNewSvy (Surveys);

   /***** End box *****/
   Box_BoxEnd ();

   /***** Free list of surveys *****/
   Svy_FreeListSurveys (Surveys);
  }

/*****************************************************************************/
/******************* Check if I can create a new survey **********************/
/*****************************************************************************/

static bool Svy_CheckIfICanCreateSvy (void)
  {
   static const bool ICanCreateSvy[Rol_NUM_ROLES] =
     {
      [Rol_TCH	  ] = true,
      [Rol_DEG_ADM] = true,
      [Rol_CTR_ADM] = true,
      [Rol_INS_ADM] = true,
      [Rol_SYS_ADM] = true,
     };

   return ICanCreateSvy[Gbl.Usrs.Me.Role.Logged];
  }

/*****************************************************************************/
/***************** Put contextual icons in list of surveys *******************/
/*****************************************************************************/

static void Svy_PutIconsListSurveys (void *Surveys)
  {
   /***** Put icon to create a new survey *****/
   if (Svy_CheckIfICanCreateSvy ())
      Svy_PutIconToCreateNewSvy ((struct Svy_Surveys *) Surveys);

   /***** Put icon to show a figure *****/
   Fig_PutIconToShowFigure (Fig_SURVEYS);
  }

/*****************************************************************************/
/********************** Put icon to create a new survey **********************/
/*****************************************************************************/

static void Svy_PutIconToCreateNewSvy (struct Svy_Surveys *Surveys)
  {
   Ico_PutContextualIconToAdd (ActFrmNewSvy,NULL,
                               Svy_PutParamsToCreateNewSvy,Surveys);
  }

/*****************************************************************************/
/********************* Put button to create a new survey *********************/
/*****************************************************************************/

static void Svy_PutButtonToCreateNewSvy (struct Svy_Surveys *Surveys)
  {
   extern const char *Txt_New_survey;

   Frm_BeginForm (ActFrmNewSvy);
      Svy_PutParamsToCreateNewSvy (Surveys);
      Btn_PutConfirmButton (Txt_New_survey);
   Frm_EndForm ();
  }

/*****************************************************************************/
/******************* Put parameters to create a new survey *******************/
/*****************************************************************************/

static void Svy_PutParamsToCreateNewSvy (void *Surveys)
  {
   Grp_WhichGroups_t WhichGroups;

   if (Surveys)
     {
      Svy_PutHiddenParamSvyOrder (((struct Svy_Surveys *) Surveys)->SelectedOrder);
      WhichGroups = Grp_GetParamWhichGroups ();
      Grp_PutParamWhichGroups (&WhichGroups);
      Pag_PutHiddenParamPagNum (Pag_SURVEYS,((struct Svy_Surveys *) Surveys)->CurrentPage);
     }
  }

/*****************************************************************************/
/**************** Put params to select which groups to show ******************/
/*****************************************************************************/

static void Svy_ParamsWhichGroupsToShow (void *Surveys)
  {
   if (Surveys)
     {
      Svy_PutHiddenParamSvyOrder (((struct Svy_Surveys *) Surveys)->SelectedOrder);
      Pag_PutHiddenParamPagNum (Pag_SURVEYS,((struct Svy_Surveys *) Surveys)->CurrentPage);
     }
  }

/*****************************************************************************/
/****************************** Show one survey ******************************/
/*****************************************************************************/

void Svy_SeeOneSurvey (void)
  {
   struct Svy_Surveys Surveys;
   struct Svy_Survey Svy;

   /***** Reset surveys *****/
   Svy_ResetSurveys (&Surveys);

   /***** Get parameters *****/
   Surveys.SelectedOrder = Svy_GetParamSvyOrder ();
   Grp_GetParamWhichGroups ();
   Surveys.CurrentPage = Pag_GetParamPagNum (Pag_SURVEYS);

   /***** Get survey code *****/
   if ((Svy.SvyCod = Svy_GetParamSvyCod ()) <= 0)
      Err_WrongSurveyExit ();

   /***** Show survey *****/
   Svy_ShowOneSurvey (&Surveys,Svy.SvyCod,true);
  }

/*****************************************************************************/
/****************************** Show one survey ******************************/
/*****************************************************************************/

static void Svy_ShowOneSurvey (struct Svy_Surveys *Surveys,
                               long SvyCod,bool ShowOnlyThisSvyComplete)
  {
   extern const char *Hlp_ASSESSMENT_Surveys;
   extern const char *Txt_Survey;
   extern const char *Txt_View_survey;
   extern const char *Txt_Number_of_questions;
   extern const char *Txt_Number_of_users;
   extern const char *Txt_Scope;
   extern const char *Txt_Country;
   extern const char *Txt_Institution;
   extern const char *Txt_Center;
   extern const char *Txt_Degree;
   extern const char *Txt_Course;
   extern const char *Txt_Users;
   extern const char *Txt_Answer_survey;
   extern const char *Txt_View_results;
   char *Anchor = NULL;
   static unsigned UniqueId = 0;
   char *Id;
   Grp_WhichGroups_t WhichGroups;
   struct Svy_Survey Svy;
   char Txt[Cns_MAX_BYTES_TEXT + 1];

   /***** Begin box *****/
   if (ShowOnlyThisSvyComplete)
      Box_BoxBegin (NULL,Txt_Survey,
                    NULL,NULL,
                    Hlp_ASSESSMENT_Surveys,Box_NOT_CLOSABLE);

   /***** Get data of this survey *****/
   Svy.SvyCod = SvyCod;
   Svy_GetDataOfSurveyByCod (&Svy);

   /***** Set anchor string *****/
   Frm_SetAnchorStr (Svy.SvyCod,&Anchor);

   /***** Begin table *****/
   if (ShowOnlyThisSvyComplete)
      HTM_TABLE_BeginWidePadding (2);

   /***** Write first row of data of this assignment *****/
   HTM_TR_Begin (NULL);

      /* Forms to remove/edit this assignment */
      if (ShowOnlyThisSvyComplete)
	 HTM_TD_Begin ("rowspan=\"2\" class=\"CONTEXT_COL\"");
      else
	 HTM_TD_Begin ("rowspan=\"2\" class=\"CONTEXT_COL %s\"",
	               The_GetColorRows ());
      if (Svy.Status.ICanEdit)
	 Svy_PutFormsToRemEditOneSvy (Surveys,&Svy,Anchor);
      HTM_TD_End ();

      /* Start date/time */
      UniqueId++;
      if (asprintf (&Id,"svy_date_start_%u",UniqueId) < 0)
	 Err_NotEnoughMemoryExit ();
      if (ShowOnlyThisSvyComplete)
	 HTM_TD_Begin ("id=\"%s\" class=\"LT %s_%s\"",
		       Id,
		       Svy.Status.Hidden ? (Svy.Status.Open ? "DATE_GREEN_LIGHT" :
							      "DATE_RED_LIGHT") :
					   (Svy.Status.Open ? "DATE_GREEN" :
							      "DATE_RED"),
		       The_GetSuffix ());
      else
	 HTM_TD_Begin ("id=\"%s\" class=\"LT %s_%s %s\"",
		       Id,
		       Svy.Status.Hidden ? (Svy.Status.Open ? "DATE_GREEN_LIGHT" :
							      "DATE_RED_LIGHT") :
					   (Svy.Status.Open ? "DATE_GREEN" :
							      "DATE_RED"),
		       The_GetSuffix (),
		       The_GetColorRows ());
      Dat_WriteLocalDateHMSFromUTC (Id,Svy.TimeUTC[Dat_STR_TIME],
				    Gbl.Prefs.DateFormat,Dat_SEPARATOR_BREAK,
				    true,true,true,0x7);
      HTM_TD_End ();
      free (Id);

      /* End date/time */
      if (asprintf (&Id,"svy_date_end_%u",UniqueId) < 0)
	 Err_NotEnoughMemoryExit ();
      if (ShowOnlyThisSvyComplete)
	 HTM_TD_Begin ("id=\"%s\" class=\"LT %s_%s\"",
		       Id,
		       Svy.Status.Hidden ? (Svy.Status.Open ? "DATE_GREEN_LIGHT" :
							      "DATE_RED_LIGHT") :
					   (Svy.Status.Open ? "DATE_GREEN" :
							      "DATE_RED"),
		       The_GetSuffix ());
      else
	 HTM_TD_Begin ("id=\"%s\" class=\"LT %s_%s %s\"",
		       Id,
		       Svy.Status.Hidden ? (Svy.Status.Open ? "DATE_GREEN_LIGHT" :
							      "DATE_RED_LIGHT") :
					   (Svy.Status.Open ? "DATE_GREEN" :
							      "DATE_RED"),
		       The_GetSuffix (),
		       The_GetColorRows ());
      Dat_WriteLocalDateHMSFromUTC (Id,Svy.TimeUTC[Dat_END_TIME],
				    Gbl.Prefs.DateFormat,Dat_SEPARATOR_BREAK,
				    true,false,true,0x7);
      HTM_TD_End ();
      free (Id);

      /* Survey title */
      if (ShowOnlyThisSvyComplete)
	 HTM_TD_Begin ("class=\"LT\"");
      else
	 HTM_TD_Begin ("class=\"LT %s\"",The_GetColorRows ());

      HTM_ARTICLE_Begin (Anchor);
	 Frm_BeginForm (ActSeeSvy);
	    Svy_PutParamSvyCod (SvyCod);
	    Svy_PutHiddenParamSvyOrder (Surveys->SelectedOrder);
	    WhichGroups = Grp_GetParamWhichGroups ();
	    Grp_PutParamWhichGroups (&WhichGroups);
	    Pag_PutHiddenParamPagNum (Pag_SURVEYS,Surveys->CurrentPage);
	    HTM_BUTTON_Submit_Begin (Txt_View_survey,"class=\"LT BT_LINK %s_%s\"",
				     Svy.Status.Hidden ? "ASG_TITLE_LIGHT" :
							 "ASG_TITLE",
				     The_GetSuffix ());
	       HTM_Txt (Svy.Title);
	    HTM_BUTTON_End ();
	 Frm_EndForm ();
      HTM_ARTICLE_End ();

      /* Number of questions and number of distinct users who have already answered this survey */
      HTM_DIV_Begin ("class=\"%s_%s\"",
		     Svy.Status.Hidden ? "ASG_GRP_LIGHT" :
					 "ASG_GRP",
		     The_GetSuffix ());
	 HTM_TxtColonNBSP (Txt_Number_of_questions);
	 HTM_Unsigned (Svy.NumQsts);
	 HTM_Txt ("; ");
	 HTM_TxtColonNBSP (Txt_Number_of_users);
	 HTM_Unsigned (Svy.NumUsrs);
      HTM_DIV_End ();

      HTM_TD_End ();

      /* Status of the survey */
      if (ShowOnlyThisSvyComplete)
	 HTM_TD_Begin ("rowspan=\"2\" class=\"LT\"");
      else
	 HTM_TD_Begin ("rowspan=\"2\" class=\"LT %s\"",
	               The_GetColorRows ());
      Svy_WriteStatus (&Svy);

      if (!ShowOnlyThisSvyComplete)
	{
	 /* Possible button to answer this survey */
	 if (Svy.Status.ICanAnswer)
	   {
	    HTM_DIV_Begin ("class=\"BUTTONS_AFTER_ALERT\"");

	       Frm_BeginForm (ActSeeSvy);
		  Svy_PutParamSvyCod (Svy.SvyCod);
		  Svy_PutHiddenParamSvyOrder (Surveys->SelectedOrder);
		  WhichGroups = Grp_GetParamWhichGroups ();
		  Grp_PutParamWhichGroups (&WhichGroups);
		  Pag_PutHiddenParamPagNum (Pag_SURVEYS,Surveys->CurrentPage);
		     Btn_PutCreateButtonInline (Txt_Answer_survey);
	       Frm_EndForm ();

	    HTM_DIV_End ();
	   }
	 /* Possible button to see the result of the survey */
	 else if (Svy.Status.ICanViewResults)
	   {
	    HTM_DIV_Begin ("class=\"BUTTONS_AFTER_ALERT\"");

	       Frm_BeginForm (ActSeeSvy);
		  Svy_PutParamSvyCod (Svy.SvyCod);
		  Svy_PutHiddenParamSvyOrder (Surveys->SelectedOrder);
		  WhichGroups = Grp_GetParamWhichGroups ();
		  Grp_PutParamWhichGroups (&WhichGroups);
		  Pag_PutHiddenParamPagNum (Pag_SURVEYS,Surveys->CurrentPage);
		  Btn_PutConfirmButtonInline (Txt_View_results);
	       Frm_EndForm ();

	    HTM_DIV_End ();
	   }
	}

      HTM_TD_End ();
   HTM_TR_End ();

   /***** Write second row of data of this survey *****/
   HTM_TR_Begin (NULL);

      /* 1st column: Author of the survey */
      if (ShowOnlyThisSvyComplete)
	 HTM_TD_Begin ("colspan=\"2\" class=\"LT\"");
      else
	 HTM_TD_Begin ("colspan=\"2\" class=\"LT %s\"",
	               The_GetColorRows ());
      Svy_WriteAuthor (&Svy);
      HTM_TD_End ();

      /* 2nd column: Scope, Users, Groups and Text */
      if (ShowOnlyThisSvyComplete)
	 HTM_TD_Begin ("class=\"LT\"");
      else
	 HTM_TD_Begin ("class=\"LT %s\"",The_GetColorRows ());

      /* Scope of the survey */
      HTM_DIV_Begin ("class=\"%s_%s\"",
                     Svy.Status.Hidden ? "ASG_GRP_LIGHT" :
					 "ASG_GRP",
		     The_GetSuffix ());
	 HTM_TxtColonNBSP (Txt_Scope);
	 switch (Svy.Scope)
	   {
	    case HieLvl_UNK:	// Unknown
	       Err_WrongScopeExit ();
	       break;
	    case HieLvl_SYS:	// System
	       HTM_Txt (Cfg_PLATFORM_SHORT_NAME);
	       break;
	    case HieLvl_CTY:	// Country
	       HTM_TxtF ("%s&nbsp;%s",Txt_Country,Gbl.Hierarchy.Cty.Name[Gbl.Prefs.Language]);
	       break;
	    case HieLvl_INS:	// Institution
	       HTM_TxtF ("%s&nbsp;%s",Txt_Institution,Gbl.Hierarchy.Ins.ShrtName);
	       break;
	    case HieLvl_CTR:	// Center
	       HTM_TxtF ("%s&nbsp;%s",Txt_Center,Gbl.Hierarchy.Ctr.ShrtName);
	       break;
	    case HieLvl_DEG:	// Degree
	       HTM_TxtF ("%s&nbsp;%s",Txt_Degree,Gbl.Hierarchy.Deg.ShrtName);
	       break;
	    case HieLvl_CRS:	// Course
	       HTM_TxtF ("%s&nbsp;%s",Txt_Course,Gbl.Hierarchy.Crs.ShrtName);
	       break;
	   }
      HTM_DIV_End ();

      /* Users' roles who can answer the survey */
      HTM_DIV_Begin ("class=\"%s_%s\"",
                     Svy.Status.Hidden ? "ASG_GRP_LIGHT" :
                	                 "ASG_GRP",
		     The_GetSuffix ());
	 HTM_TxtColon (Txt_Users);
	 HTM_BR ();
	 Rol_WriteSelectorRoles (1 << Rol_STD |
				 1 << Rol_NET |
				 1 << Rol_TCH,
				 Svy.Roles,
				 true,false);
      HTM_DIV_End ();

      /* Groups whose users can answer this survey */
      if (Svy.Scope == HieLvl_CRS)
	 if (Gbl.Crs.Grps.NumGrps)
	    Svy_GetAndWriteNamesOfGrpsAssociatedToSvy (&Svy);

      /* Text of the survey */
      Svy_DB_GetSurveyTxt (Svy.SvyCod,Txt);
      Str_ChangeFormat (Str_FROM_HTML,Str_TO_RIGOROUS_HTML,
			Txt,Cns_MAX_BYTES_TEXT,false);	// Convert from HTML to rigorous HTML
      ALn_InsertLinks (Txt,Cns_MAX_BYTES_TEXT,60);	// Insert links
      HTM_DIV_Begin ("class=\"PAR %s_%s\"",
                     Svy.Status.Hidden ? "DAT_LIGHT" :
					 "DAT",
		     The_GetSuffix ());
	 HTM_Txt (Txt);
      HTM_DIV_End ();
      HTM_TD_End ();

   HTM_TR_End ();

   /***** Write questions of this survey *****/
   if (ShowOnlyThisSvyComplete)
     {
      HTM_TR_Begin (NULL);
	 HTM_TD_Begin ("colspan=\"5\"");
	    Svy_ListSvyQuestions (Surveys,&Svy);
	 HTM_TD_End ();
      HTM_TR_End ();
     }

   The_ChangeRowColor ();

   /***** Mark possible notification as seen *****/
   if (Svy.Scope == HieLvl_CRS)	// Only course surveys are notified
      Ntf_DB_MarkNotifAsSeenUsingCod (Ntf_EVENT_SURVEY,Svy.Cod);

   if (ShowOnlyThisSvyComplete)
     {
	 /***** End table *****/
	 HTM_TABLE_End ();

      /***** End box *****/
      Box_BoxEnd ();
     }

   /***** Free anchor string *****/
   Frm_FreeAnchorStr (Anchor);
  }

/*****************************************************************************/
/*********************** Write the author of a survey ************************/
/*****************************************************************************/

static void Svy_WriteAuthor (struct Svy_Survey *Svy)
  {
   Usr_WriteAuthor1Line (Svy->UsrCod,Svy->Status.Hidden);
  }

/*****************************************************************************/
/************************ Write status of a survey ***************************/
/*****************************************************************************/

static void Svy_WriteStatus (struct Svy_Survey *Svy)
  {
   extern const char *Txt_Hidden_survey;
   extern const char *Txt_Visible_survey;
   extern const char *Txt_Closed_survey;
   extern const char *Txt_Open_survey;
   extern const char *Txt_SURVEY_Type_of_user_not_allowed;
   extern const char *Txt_SURVEY_Type_of_user_allowed;
   extern const char *Txt_SURVEY_You_belong_to_the_scope_of_the_survey;
   extern const char *Txt_SURVEY_You_dont_belong_to_the_scope_of_the_survey;
   extern const char *Txt_SURVEY_You_have_already_answered;
   extern const char *Txt_SURVEY_You_have_not_answered;

   /***** Begin list with items of status *****/
   HTM_UL_Begin (NULL);

      /* Write whether survey is visible or hidden */
      if (Svy->Status.Hidden)
	{
	 HTM_LI_Begin ("class=\"STATUS_RED_LIGHT_%s\"",The_GetSuffix ());
	    HTM_Txt (Txt_Hidden_survey);
	}
      else
	{
	 HTM_LI_Begin ("class=\"STATUS_GREEN_%s\"",The_GetSuffix ());
	    HTM_Txt (Txt_Visible_survey);
	}
      HTM_LI_End ();

      /* Write whether survey is open or closed */
      if (Svy->Status.Open)
	{
	 HTM_LI_Begin ("class=\"%s_%s\"",
		       Svy->Status.Hidden ? "STATUS_GREEN_LIGHT" :
				            "STATUS_GREEN",
		       The_GetSuffix ());
	    HTM_Txt (Txt_Open_survey);
	}
      else
	{
	 HTM_LI_Begin ("class=\"%s_%s\"",
		       Svy->Status.Hidden ? "STATUS_RED_LIGHT" :
				            "STATUS_RED",
		       The_GetSuffix ());
	    HTM_Txt (Txt_Closed_survey);
	}
      HTM_LI_End ();

      /* Write whether survey can be answered by me or not depending on user type */
      if (Svy->Status.IAmLoggedWithAValidRoleToAnswer)
	{
	 HTM_LI_Begin ("class=\"%s_%s\"",
		       Svy->Status.Hidden ? "STATUS_GREEN_LIGHT" :
				            "STATUS_GREEN",
		       The_GetSuffix ());
	    HTM_Txt (Txt_SURVEY_Type_of_user_allowed);
	}
      else
	{
	 HTM_LI_Begin ("class=\"%s_%s\"",
		       Svy->Status.Hidden ? "STATUS_RED_LIGHT" :
				            "STATUS_RED",
		       The_GetSuffix ());
	    HTM_Txt (Txt_SURVEY_Type_of_user_not_allowed);
	}
      HTM_LI_End ();

      /* Write whether survey can be answered by me or not depending on groups */
      if (Svy->Status.IBelongToScope)
	{
	 HTM_LI_Begin ("class=\"%s_%s\"",
		       Svy->Status.Hidden ? "STATUS_GREEN_LIGHT" :
				            "STATUS_GREEN",
		       The_GetSuffix ());
	    HTM_Txt (Txt_SURVEY_You_belong_to_the_scope_of_the_survey);
	}
      else
	{
	 HTM_LI_Begin ("class=\"%s_%s\"",
		       Svy->Status.Hidden ? "STATUS_RED_LIGHT" :
					    "STATUS_RED",
		       The_GetSuffix ());
	    HTM_Txt (Txt_SURVEY_You_dont_belong_to_the_scope_of_the_survey);
	}
      HTM_LI_End ();

      /* Write whether survey has been already answered by me or not */
      if (Svy->Status.IHaveAnswered)
	{
	 HTM_LI_Begin ("class=\"%s_%s\"",
		       Svy->Status.Hidden ? "STATUS_GREEN_LIGHT" :
					    "STATUS_GREEN",
		       The_GetSuffix ());
	    HTM_Txt (Txt_SURVEY_You_have_already_answered);
	}
      else
	{
	 HTM_LI_Begin ("class=\"%s_%s\"",
		       Svy->Status.Hidden ? "STATUS_RED_LIGHT" :
				            "STATUS_RED",
		       The_GetSuffix ());
	    HTM_Txt (Txt_SURVEY_You_have_not_answered);
	}
      HTM_LI_End ();

   /***** End list with items of status *****/
   HTM_UL_End ();
  }

/*****************************************************************************/
/********* Get parameter with the type or order in list of surveys ***********/
/*****************************************************************************/

static Dat_StartEndTime_t Svy_GetParamSvyOrder (void)
  {
   return (Dat_StartEndTime_t)
	  Par_GetParToUnsignedLong ("Order",
				    0,
				    Dat_NUM_START_END_TIME - 1,
				    (unsigned long) Svy_ORDER_DEFAULT);
  }

/*****************************************************************************/
/***** Put a hidden parameter with the type of order in list of surveys ******/
/*****************************************************************************/

void Svy_PutHiddenParamSvyOrder (Dat_StartEndTime_t SelectedOrder)
  {
   Dat_PutHiddenParamOrder (SelectedOrder);
  }

/*****************************************************************************/
/******************* Put a link (form) to edit one survey ********************/
/*****************************************************************************/

static void Svy_PutFormsToRemEditOneSvy (struct Svy_Surveys *Surveys,
					 const struct Svy_Survey *Svy,
                                         const char *Anchor)
  {
   static Act_Action_t ActionHideUnhide[2] =
     {
      [false] = ActHidSvy,	// Visible ==> action to hide
      [true ] = ActUnhSvy,	// Hidden ==> action to unhide
     };

   Surveys->SvyCod = Svy->SvyCod;	// Used as parameters in contextual links

   /***** Icon to remove survey *****/
   Ico_PutContextualIconToRemove (ActReqRemSvy,NULL,
                                  Svy_PutParams,Surveys);

   /***** Icon to reset survey *****/
   Lay_PutContextualLinkOnlyIcon (ActReqRstSvy,NULL,
                                  Svy_PutParams,Surveys,
				  "recycle.svg",Ico_RED);

   /***** Icon to hide/unhide survey *****/
   Ico_PutContextualIconToHideUnhide (ActionHideUnhide,Anchor,
				      Svy_PutParams,Surveys,
				      Svy->Status.Hidden);

   /***** Icon to edit survey *****/
   Ico_PutContextualIconToEdit (ActEdiOneSvy,NULL,
                                Svy_PutParams,Surveys);
  }

/*****************************************************************************/
/********************** Params used to edit a survey *************************/
/*****************************************************************************/

static void Svy_PutParams (void *Surveys)
  {
   Grp_WhichGroups_t WhichGroups;

   if (Surveys)
     {
      if (((struct Svy_Surveys *) Surveys)->SvyCod > 0)
	 Svy_PutParamSvyCod (((struct Svy_Surveys *) Surveys)->SvyCod);
      Dat_PutHiddenParamOrder (((struct Svy_Surveys *) Surveys)->SelectedOrder);
      WhichGroups = Grp_GetParamWhichGroups ();
      Grp_PutParamWhichGroups (&WhichGroups);
      Pag_PutHiddenParamPagNum (Pag_SURVEYS,((struct Svy_Surveys *) Surveys)->CurrentPage);
     }
  }

/*****************************************************************************/
/************************** Get list of all surveys **************************/
/*****************************************************************************/

static void Svy_GetListSurveys (struct Svy_Surveys *Surveys)
  {
   MYSQL_RES *mysql_res;
   unsigned NumSvy;
   unsigned ScopesAllowed = 0;
   unsigned HiddenAllowed = 0;

   /***** Free list of surveys *****/
   if (Surveys->LstIsRead)
      Svy_FreeListSurveys (Surveys);

   /***** Set allowed and hidden scopes to get list depending on my user's role *****/
   Svy_SetAllowedAndHiddenScopes (&ScopesAllowed,&HiddenAllowed);

   /***** Get list of surveys from database *****/
   if ((Surveys->Num = Svy_DB_GetListSurveys (&mysql_res,
                                              ScopesAllowed,
                                              HiddenAllowed,
                                              Surveys->SelectedOrder))) // Surveys found...
     {
      /***** Create list of surveys *****/
      if ((Surveys->LstSvyCods = calloc ((size_t) Surveys->Num,
                                         sizeof (*Surveys->LstSvyCods))) == NULL)
         Err_NotEnoughMemoryExit ();

      /***** Get the surveys codes *****/
      for (NumSvy = 0;
	   NumSvy < Surveys->Num;
	   NumSvy++)
         /* Get next survey code */
         if ((Surveys->LstSvyCods[NumSvy] = DB_GetNextCode (mysql_res)) < 0)
            Err_WrongSurveyExit ();
     }

   /***** Free structure that stores the query result *****/
   DB_FreeMySQLResult (&mysql_res);

   Surveys->LstIsRead = true;
  }

/*****************************************************************************/
/*** Set allowed and hidden scopes to get list depending on my user's role ***/
/*****************************************************************************/

static void Svy_SetAllowedAndHiddenScopes (unsigned *ScopesAllowed,
                                           unsigned *HiddenAllowed)
  {
   switch (Gbl.Usrs.Me.Role.Logged)
     {
      case Rol_UNK:	// User not logged in *********************************
	 *ScopesAllowed = 0;
	 *HiddenAllowed = 0;
         break;
      case Rol_GST:	// User not belonging to any course *******************
	 *ScopesAllowed = 1 << HieLvl_SYS;
	 *HiddenAllowed = 0;
	 break;
      case Rol_USR:	// Student or teacher in other courses...
   	   	   	// ...but not belonging to the current course *********
	 *ScopesAllowed = 1 << HieLvl_SYS;
	 *HiddenAllowed = 0;
	 if (Cty_CheckIfIBelongToCty (Gbl.Hierarchy.Cty.CtyCod))
	   {
	    *ScopesAllowed |= 1 << HieLvl_CTY;
	    if (Ins_CheckIfIBelongToIns (Gbl.Hierarchy.Ins.InsCod))
	      {
	       *ScopesAllowed |= 1 << HieLvl_INS;
	       if (Ctr_CheckIfIBelongToCtr (Gbl.Hierarchy.Ctr.CtrCod))
		 {
		  *ScopesAllowed |= 1 << HieLvl_CTR;
		  if (Deg_CheckIfIBelongToDeg (Gbl.Hierarchy.Deg.DegCod))
		     *ScopesAllowed |= 1 << HieLvl_DEG;
		 }
	      }
	   }
         break;
      case Rol_STD:	// Student in current course **************************
	 *ScopesAllowed = 1 << HieLvl_SYS;
	 *HiddenAllowed = 0;
	 if (Cty_CheckIfIBelongToCty (Gbl.Hierarchy.Cty.CtyCod))
	   {
	    *ScopesAllowed |= 1 << HieLvl_CTY;
	    if (Ins_CheckIfIBelongToIns (Gbl.Hierarchy.Ins.InsCod))
	      {
	       *ScopesAllowed |= 1 << HieLvl_INS;
	       if (Ctr_CheckIfIBelongToCtr (Gbl.Hierarchy.Ctr.CtrCod))
		 {
		  *ScopesAllowed |= 1 << HieLvl_CTR;
		  if (Deg_CheckIfIBelongToDeg (Gbl.Hierarchy.Deg.DegCod))
		    {
		     *ScopesAllowed |= 1 << HieLvl_DEG;
		     if (Gbl.Usrs.Me.IBelongToCurrentCrs)
			*ScopesAllowed |= 1 << HieLvl_CRS;
		    }
		 }
	      }
	   }
         break;
      case Rol_NET:	// Non-editing teacher in current course **************
      case Rol_TCH:	// Teacher in current course **************************
	 *ScopesAllowed = 1 << HieLvl_SYS;
	 *HiddenAllowed = 0;
	 if (Cty_CheckIfIBelongToCty (Gbl.Hierarchy.Cty.CtyCod))
	   {
	    *ScopesAllowed |= 1 << HieLvl_CTY;
	    if (Ins_CheckIfIBelongToIns (Gbl.Hierarchy.Ins.InsCod))
	      {
	       *ScopesAllowed |= 1 << HieLvl_INS;
	       if (Ctr_CheckIfIBelongToCtr (Gbl.Hierarchy.Ctr.CtrCod))
		 {
		  *ScopesAllowed |= 1 << HieLvl_CTR;
		  if (Deg_CheckIfIBelongToDeg (Gbl.Hierarchy.Deg.DegCod))
		    {
		     *ScopesAllowed |= 1 << HieLvl_DEG;
		     if (Gbl.Usrs.Me.IBelongToCurrentCrs)
		       {
			*ScopesAllowed |= 1 << HieLvl_CRS;
			*HiddenAllowed |= 1 << HieLvl_CRS;	// A non-editing teacher or teacher can view hidden course surveys
		       }
		    }
		 }
	      }
	   }
         break;
      case Rol_DEG_ADM:	// Degree administrator *******************************
	 *ScopesAllowed = 1 << HieLvl_SYS;
	 *HiddenAllowed = 0;
	 if (Gbl.Hierarchy.Cty.CtyCod > 0)			// Country selected
	   {
	    *ScopesAllowed |= 1 << HieLvl_CTY;
	    if (Gbl.Hierarchy.Ins.InsCod > 0)			// Institution selected
	      {
	       *ScopesAllowed |= 1 << HieLvl_INS;
	       if (Gbl.Hierarchy.Ctr.CtrCod > 0)		// Center selected
		 {
		  *ScopesAllowed |= 1 << HieLvl_CTR;
		  if (Gbl.Hierarchy.Deg.DegCod > 0)		// Degree selected
		    {
		     *ScopesAllowed |= 1 << HieLvl_DEG;
		     *HiddenAllowed |= 1 << HieLvl_DEG;	// A degree admin can view hidden degree surveys
		    }
	         }
	      }
	   }
	 break;
      case Rol_CTR_ADM:	// Center administrator *******************************
	 *ScopesAllowed = 1 << HieLvl_SYS;
	 *HiddenAllowed = 0;
	 if (Gbl.Hierarchy.Cty.CtyCod > 0)			// Country selected
	   {
	    *ScopesAllowed |= 1 << HieLvl_CTY;
	    if (Gbl.Hierarchy.Ins.InsCod > 0)			// Institution selected
	      {
	       *ScopesAllowed |= 1 << HieLvl_INS;
	       if (Gbl.Hierarchy.Ctr.CtrCod > 0)		// Center selected
		 {
		  *ScopesAllowed |= 1 << HieLvl_CTR;
		  *HiddenAllowed |= 1 << HieLvl_CTR;		// A center admin can view hidden center surveys
		 }
	      }
	   }
	 break;
      case Rol_INS_ADM:	// Institution administrator **************************
	 *ScopesAllowed = 1 << HieLvl_SYS;
	 *HiddenAllowed = 0;
	 if (Gbl.Hierarchy.Cty.CtyCod > 0)			// Country selected
	   {
	    *ScopesAllowed |= 1 << HieLvl_CTY;
	    if (Gbl.Hierarchy.Ins.InsCod > 0)			// Institution selected
	      {
	       *ScopesAllowed |= 1 << HieLvl_INS;
	       *HiddenAllowed |= 1 << HieLvl_INS;		// An institution admin can view hidden institution surveys
	      }
	   }
	 break;
      case Rol_SYS_ADM:	// System administrator (superuser) *******************
	 *ScopesAllowed = 1 << HieLvl_SYS;
	 *HiddenAllowed = 1 << HieLvl_SYS;			// A system admin can view hidden system surveys
	 if (Gbl.Hierarchy.Cty.CtyCod > 0)			// Country selected
	   {
	    *ScopesAllowed |= 1 << HieLvl_CTY;
	    *HiddenAllowed |= 1 << HieLvl_CTY;			// A system admin can view hidden country surveys
	    if (Gbl.Hierarchy.Ins.InsCod > 0)			// Institution selected
	      {
	       *ScopesAllowed |= 1 << HieLvl_INS;
	       *HiddenAllowed |= 1 << HieLvl_INS;		// A system admin can view hidden institution surveys
	       if (Gbl.Hierarchy.Ctr.CtrCod > 0)		// Center selected
		 {
		  *ScopesAllowed |= 1 << HieLvl_CTR;
	          *HiddenAllowed |= 1 << HieLvl_CTR;		// A system admin can view hidden center surveys
		  if (Gbl.Hierarchy.Deg.DegCod > 0)		// Degree selected
		    {
		     *ScopesAllowed |= 1 << HieLvl_DEG;
	             *HiddenAllowed |= 1 << HieLvl_DEG;		// A system admin can view hidden degree surveys
		     if (Gbl.Hierarchy.Level == HieLvl_CRS)	// Course selected
		       {
			*ScopesAllowed |= 1 << HieLvl_CRS;
	                *HiddenAllowed |= 1 << HieLvl_CRS;	// A system admin can view hidden course surveys
		       }
		    }
		 }
	      }
	   }
	 break;
     }
  }

/*****************************************************************************/
/********************* Get survey data using its code ************************/
/*****************************************************************************/

void Svy_GetDataOfSurveyByCod (struct Svy_Survey *Svy)
  {
   MYSQL_RES *mysql_res;
   MYSQL_ROW row;

   /***** Get data of survey from database *****/
   if (Svy_DB_GetDataOfSurveyByCod (&mysql_res,Svy->SvyCod)) // Survey found...
     {
      /* Get row */
      row = mysql_fetch_row (mysql_res);

      /* Get code of the survey (row[0]) */
      Svy->SvyCod = Str_ConvertStrCodToLongCod (row[0]);

      /* Get survey scope (row[1]) */
      if ((Svy->Scope = Sco_GetScopeFromDBStr (row[1])) == HieLvl_UNK)
         Err_WrongScopeExit ();

      /* Get code of the country, institution, center, degree or course (row[2]) */
      Svy->Cod = Str_ConvertStrCodToLongCod (row[2]);

      /* Get whether the survey is hidden (row[3]) */
      Svy->Status.Hidden = (row[3][0] == 'Y');

      /* Get roles (row[4]) */
      if (sscanf (row[4],"%u",&Svy->Roles) != 1)
      	 Err_ShowErrorAndExit ("Error when reading roles of survey.");

      /* Get author of the survey (row[5]) */
      Svy->UsrCod = Str_ConvertStrCodToLongCod (row[5]);

      /* Get start date (row[6] holds the start UTC time)
         and   end date (row[7] holds the end   UTC time) */
      Svy->TimeUTC[Dat_STR_TIME] = Dat_GetUNIXTimeFromStr (row[6]);
      Svy->TimeUTC[Dat_END_TIME] = Dat_GetUNIXTimeFromStr (row[7]);

      /* Get whether the survey is open or closed (row(8)) */
      Svy->Status.Open = (row[8][0] == '1');

      /* Get the title of the survey (row[9]) */
      Str_Copy (Svy->Title,row[9],sizeof (Svy->Title) - 1);

      /* Get number of questions and number of users who have already answer this survey */
      Svy->NumQsts = Svy_DB_GetNumQstsSvy (Svy->SvyCod);
      Svy->NumUsrs = Svy_DB_GetNumUsrsWhoHaveAnsweredSvy (Svy->SvyCod);

      /* Am I logged with a valid role to answer this survey? */
      Svy->Status.IAmLoggedWithAValidRoleToAnswer = (Svy->Roles & (1 << Gbl.Usrs.Me.Role.Logged));

      /* Do I belong to valid groups to answer this survey? */
      switch (Svy->Scope)
        {
	 case HieLvl_UNK:	// Unknown
            Err_WrongScopeExit ();
	    break;
	 case HieLvl_SYS:	// System
            Svy->Status.IBelongToScope = Gbl.Usrs.Me.Logged;
	    break;
	 case HieLvl_CTY:	// Country
            Svy->Status.IBelongToScope = Cty_CheckIfIBelongToCty (Svy->Cod);
	    break;
	 case HieLvl_INS:	// Institution
            Svy->Status.IBelongToScope = Ins_CheckIfIBelongToIns (Svy->Cod);
	    break;
	 case HieLvl_CTR:	// Center
            Svy->Status.IBelongToScope = Ctr_CheckIfIBelongToCtr (Svy->Cod);
	    break;
	 case HieLvl_DEG:	// Degree
            Svy->Status.IBelongToScope = Deg_CheckIfIBelongToDeg (Svy->Cod);
	    break;
	 case HieLvl_CRS:	// Course
	    Svy->Status.IBelongToScope = Enr_CheckIfIBelongToCrs (Svy->Cod) &&
					 Svy_DB_CheckIfICanDoThisSurveyBasedOnGrps (Svy->SvyCod);
	    break;
        }

      /* Have I answered this survey? */
      Svy->Status.IHaveAnswered = Svy_DB_CheckIfIHaveAnsweredSvy (Svy->SvyCod);

      /* Can I answer survey? */
      Svy->Status.ICanAnswer = ( Svy->NumQsts != 0) &&
                                !Svy->Status.Hidden &&
                                 Svy->Status.Open &&
                                 Svy->Status.IAmLoggedWithAValidRoleToAnswer &&
                                 Svy->Status.IBelongToScope &&
                                !Svy->Status.IHaveAnswered;

      /* Can I view results of the survey?
         Can I edit survey? */
      switch (Gbl.Usrs.Me.Role.Logged)
        {
         case Rol_STD:
            Svy->Status.ICanViewResults = ( Svy->Scope == HieLvl_CRS ||
        	                            Svy->Scope == HieLvl_DEG ||
        	                            Svy->Scope == HieLvl_CTR ||
        	                            Svy->Scope == HieLvl_INS ||
        	                            Svy->Scope == HieLvl_CTY ||
        	                            Svy->Scope == HieLvl_SYS) &&
        	                          ( Svy->NumQsts != 0) &&
                                           !Svy->Status.Hidden &&
                                            Svy->Status.Open &&
                                            Svy->Status.IAmLoggedWithAValidRoleToAnswer &&
                                            Svy->Status.IBelongToScope &&
                                            Svy->Status.IHaveAnswered;
            Svy->Status.ICanEdit         = false;
            break;
         case Rol_NET:
            Svy->Status.ICanViewResults = (Svy->Scope == HieLvl_CRS ||
        	                           Svy->Scope == HieLvl_DEG ||
        	                           Svy->Scope == HieLvl_CTR ||
        	                           Svy->Scope == HieLvl_INS ||
        	                           Svy->Scope == HieLvl_CTY ||
        	                           Svy->Scope == HieLvl_SYS) &&
        	                           Svy->NumQsts != 0 &&
                                          !Svy->Status.ICanAnswer;
            Svy->Status.ICanEdit        = false;
            break;
         case Rol_TCH:
            Svy->Status.ICanViewResults = (Svy->Scope == HieLvl_CRS ||
        	                           Svy->Scope == HieLvl_DEG ||
        	                           Svy->Scope == HieLvl_CTR ||
        	                           Svy->Scope == HieLvl_INS ||
        	                           Svy->Scope == HieLvl_CTY ||
        	                           Svy->Scope == HieLvl_SYS) &&
        	                           Svy->NumQsts != 0 &&
                                          !Svy->Status.ICanAnswer;
            Svy->Status.ICanEdit        =  Svy->Scope == HieLvl_CRS &&
                                           Svy->Status.IBelongToScope;
            break;
         case Rol_DEG_ADM:
            Svy->Status.ICanViewResults = (Svy->Scope == HieLvl_DEG ||
        	                           Svy->Scope == HieLvl_CTR ||
        	                           Svy->Scope == HieLvl_INS ||
        	                           Svy->Scope == HieLvl_CTY ||
        	                           Svy->Scope == HieLvl_SYS) &&
        	                          (Svy->NumQsts != 0) &&
                                          !Svy->Status.ICanAnswer;
            Svy->Status.ICanEdit        =  Svy->Scope == HieLvl_DEG &&
                                           Svy->Status.IBelongToScope;
            break;
         case Rol_CTR_ADM:
            Svy->Status.ICanViewResults = (Svy->Scope == HieLvl_CTR ||
        	                           Svy->Scope == HieLvl_INS ||
        	                           Svy->Scope == HieLvl_CTY ||
        	                           Svy->Scope == HieLvl_SYS) &&
        	                          (Svy->NumQsts != 0) &&
                                          !Svy->Status.ICanAnswer;
            Svy->Status.ICanEdit        =  Svy->Scope == HieLvl_CTR &&
                                           Svy->Status.IBelongToScope;
            break;
         case Rol_INS_ADM:
            Svy->Status.ICanViewResults = (Svy->Scope == HieLvl_INS ||
        	                           Svy->Scope == HieLvl_CTY ||
        	                           Svy->Scope == HieLvl_SYS) &&
        	                          (Svy->NumQsts != 0) &&
                                          !Svy->Status.ICanAnswer;
            Svy->Status.ICanEdit        =  Svy->Scope == HieLvl_INS &&
                                           Svy->Status.IBelongToScope;
            break;
         case Rol_SYS_ADM:
            Svy->Status.ICanViewResults = (Svy->NumQsts != 0);
            Svy->Status.ICanEdit        = true;
            break;
         default:
            Svy->Status.ICanViewResults = false;
            Svy->Status.ICanEdit        = false;
            break;
        }
     }
   else
     {
      /* Initialize to empty survey */
      Svy->SvyCod                = -1L;
      Svy->Scope                 = HieLvl_UNK;
      Svy->Roles                 = 0;
      Svy->UsrCod                = -1L;
      Svy->TimeUTC[Dat_STR_TIME] =
      Svy->TimeUTC[Dat_END_TIME] = (time_t) 0;
      Svy->Title[0]              = '\0';
      Svy->NumQsts               = 0;
      Svy->NumUsrs               = 0;
      Svy->Status.Hidden                          = false;
      Svy->Status.Open                            = false;
      Svy->Status.IAmLoggedWithAValidRoleToAnswer = false;
      Svy->Status.IBelongToScope                  = false;
      Svy->Status.IHaveAnswered                   = false;
      Svy->Status.ICanAnswer                      = false;
      Svy->Status.ICanViewResults                 = false;
      Svy->Status.ICanEdit                        = false;
     }

   /***** Free structure that stores the query result *****/
   DB_FreeMySQLResult (&mysql_res);
  }

/*****************************************************************************/
/**************************** Free list of surveys ***************************/
/*****************************************************************************/

void Svy_FreeListSurveys (struct Svy_Surveys *Surveys)
  {
   if (Surveys->LstIsRead && Surveys->LstSvyCods)
     {
      /***** Free memory used by the list of surveys *****/
      free (Surveys->LstSvyCods);
      Surveys->LstSvyCods = NULL;
      Surveys->Num = 0;
      Surveys->LstIsRead = false;
     }
  }

/*****************************************************************************/
/******************** Get summary and content of a survey  *******************/
/*****************************************************************************/

void Svy_GetNotifSurvey (char SummaryStr[Ntf_MAX_BYTES_SUMMARY + 1],
                         char **ContentStr,
                         long SvyCod,bool GetContent)
  {
   MYSQL_RES *mysql_res;
   MYSQL_ROW row;
   size_t Length;

   SummaryStr[0] = '\0';	// Return nothing on error

   /***** Get title and text *****/
   if (Svy_DB_GetSurveyTitleAndText (&mysql_res,SvyCod))
     {
      /***** Get row *****/
      row = mysql_fetch_row (mysql_res);

      /***** Get summary *****/
      Str_Copy (SummaryStr,row[0],Ntf_MAX_BYTES_SUMMARY);

      /***** Get content *****/
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
/******************* Write parameter with code of survey *********************/
/*****************************************************************************/

static void Svy_PutParamSvyCod (long SvyCod)
  {
   Par_PutHiddenParamLong (NULL,"SvyCod",SvyCod);
  }

/*****************************************************************************/
/******************** Get parameter with code of survey **********************/
/*****************************************************************************/

static long Svy_GetParamSvyCod (void)
  {
   /***** Get code of survey *****/
   return Par_GetParToLong ("SvyCod");
  }

/*****************************************************************************/
/*************** Ask for confirmation of removing of a survey ****************/
/*****************************************************************************/

void Svy_AskRemSurvey (void)
  {
   extern const char *Txt_Do_you_really_want_to_remove_the_survey_X;
   extern const char *Txt_Remove_survey;
   struct Svy_Surveys Surveys;
   struct Svy_Survey Svy;

   /***** Reset surveys *****/
   Svy_ResetSurveys (&Surveys);

   /***** Get parameters *****/
   Surveys.SelectedOrder = Svy_GetParamSvyOrder ();
   Grp_GetParamWhichGroups ();
   Surveys.CurrentPage = Pag_GetParamPagNum (Pag_SURVEYS);

   /***** Get survey code *****/
   if ((Svy.SvyCod = Svy_GetParamSvyCod ()) <= 0)
      Err_WrongSurveyExit ();

   /***** Get data of the survey from database *****/
   Svy_GetDataOfSurveyByCod (&Svy);
   if (!Svy.Status.ICanEdit)
      Err_NoPermissionExit ();

   /***** Show question and button to remove survey *****/
   Surveys.SvyCod = Svy.SvyCod;
   Ale_ShowAlertAndButton (ActRemSvy,NULL,NULL,
                           Svy_PutParams,&Surveys,
			   Btn_REMOVE_BUTTON,Txt_Remove_survey,
			   Ale_QUESTION,Txt_Do_you_really_want_to_remove_the_survey_X,
			   Svy.Title);

   /***** Show surveys again *****/
   Svy_ListAllSurveys (&Surveys);
  }

/*****************************************************************************/
/****************************** Remove a survey ******************************/
/*****************************************************************************/

void Svy_RemoveSurvey (void)
  {
   extern const char *Txt_Survey_X_removed;
   struct Svy_Surveys Surveys;
   struct Svy_Survey Svy;

   /***** Reset surveys *****/
   Svy_ResetSurveys (&Surveys);

   /***** Get parameters *****/
   Surveys.SelectedOrder = Svy_GetParamSvyOrder ();
   Grp_GetParamWhichGroups ();
   Surveys.CurrentPage = Pag_GetParamPagNum (Pag_SURVEYS);

   /***** Get survey code *****/
   if ((Svy.SvyCod = Svy_GetParamSvyCod ()) <= 0)
      Err_WrongSurveyExit ();

   /***** Get data of the survey from database *****/
   Svy_GetDataOfSurveyByCod (&Svy);
   if (!Svy.Status.ICanEdit)
      Err_NoPermissionExit ();

   /***** Remove all users in this survey *****/
   Svy_DB_RemoveUsrsWhoHaveAnsweredSvy (Svy.SvyCod);

   /***** Remove all answers in this survey *****/
   Svy_DB_RemoveAnswersSvy (Svy.SvyCod);

   /***** Remove all questions in this survey *****/
   Svy_DB_RemoveQstsSvy (Svy.SvyCod);

   /***** Remove all groups of this survey *****/
   Svy_DB_RemoveGrpsAssociatedToSurvey (Svy.SvyCod);

   /***** Remove survey *****/
   Svy_DB_RemoveSvy (Svy.SvyCod);

   /***** Mark possible notifications as removed *****/
   Ntf_DB_MarkNotifAsRemoved (Ntf_EVENT_SURVEY,Svy.SvyCod);

   /***** Write message to show the change made *****/
   Ale_ShowAlert (Ale_SUCCESS,Txt_Survey_X_removed,
                  Svy.Title);

   /***** Show surveys again *****/
   Svy_ListAllSurveys (&Surveys);
  }

/*****************************************************************************/
/***************** Ask for confirmation of reset of a survey *****************/
/*****************************************************************************/

void Svy_AskResetSurvey (void)
  {
   extern const char *Txt_Do_you_really_want_to_reset_the_survey_X;
   struct Svy_Surveys Surveys;
   struct Svy_Survey Svy;

   /***** Reset surveys *****/
   Svy_ResetSurveys (&Surveys);

   /***** Get parameters *****/
   Surveys.SelectedOrder = Svy_GetParamSvyOrder ();
   Grp_GetParamWhichGroups ();
   Surveys.CurrentPage = Pag_GetParamPagNum (Pag_SURVEYS);

   /***** Get survey code *****/
   if ((Svy.SvyCod = Svy_GetParamSvyCod ()) <= 0)
      Err_WrongSurveyExit ();

   /***** Get data of the survey from database *****/
   Svy_GetDataOfSurveyByCod (&Svy);
   if (!Svy.Status.ICanEdit)
      Err_NoPermissionExit ();

   /***** Ask for confirmation of reset *****/
   Ale_ShowAlert (Ale_WARNING,Txt_Do_you_really_want_to_reset_the_survey_X,
                  Svy.Title);

   /***** Button of confirmation of reset *****/
   Surveys.SvyCod = Svy.SvyCod;
   Svy_PutButtonToResetSurvey (&Surveys);

   /***** Show surveys again *****/
   Svy_ListAllSurveys (&Surveys);
  }

/*****************************************************************************/
/************************* Put button to reset survey ************************/
/*****************************************************************************/

static void Svy_PutButtonToResetSurvey (struct Svy_Surveys *Surveys)
  {
   extern const char *Txt_Reset_survey;

   Frm_BeginForm (ActRstSvy);
      Svy_PutParams (Surveys);
      Btn_PutRemoveButton (Txt_Reset_survey);
   Frm_EndForm ();
  }

/*****************************************************************************/
/******************************* Reset a survey ******************************/
/*****************************************************************************/

void Svy_ResetSurvey (void)
  {
   extern const char *Txt_Survey_X_reset;
   struct Svy_Surveys Surveys;
   struct Svy_Survey Svy;

   /***** Reset surveys *****/
   Svy_ResetSurveys (&Surveys);

   /***** Get parameters *****/
   Surveys.SelectedOrder = Svy_GetParamSvyOrder ();
   Grp_GetParamWhichGroups ();
   Surveys.CurrentPage = Pag_GetParamPagNum (Pag_SURVEYS);

   /***** Get survey code *****/
   if ((Svy.SvyCod = Svy_GetParamSvyCod ()) <= 0)
      Err_WrongSurveyExit ();

   /***** Get data of the survey from database *****/
   Svy_GetDataOfSurveyByCod (&Svy);
   if (!Svy.Status.ICanEdit)
      Err_NoPermissionExit ();

   /***** Remove all users in this survey *****/
   Svy_DB_RemoveUsrsWhoHaveAnsweredSvy (Svy.SvyCod);

   /***** Reset all answers in this survey *****/
   Svy_DB_ResetAnswersSvy (Svy.SvyCod);

   /***** Write message to show the change made *****/
   Ale_ShowAlert (Ale_SUCCESS,Txt_Survey_X_reset,
                  Svy.Title);

   /***** Show surveys again *****/
   Svy_ListAllSurveys (&Surveys);
  }

/*****************************************************************************/
/******************************** Hide a survey ******************************/
/*****************************************************************************/

void Svy_HideSurvey (void)
  {
   struct Svy_Surveys Surveys;
   struct Svy_Survey Svy;

   /***** Reset surveys *****/
   Svy_ResetSurveys (&Surveys);

   /***** Get parameters *****/
   Surveys.SelectedOrder = Svy_GetParamSvyOrder ();
   Grp_GetParamWhichGroups ();
   Surveys.CurrentPage = Pag_GetParamPagNum (Pag_SURVEYS);

   /***** Get survey code *****/
   if ((Svy.SvyCod = Svy_GetParamSvyCod ()) <= 0)
      Err_WrongSurveyExit ();

   /***** Get data of the survey from database *****/
   Svy_GetDataOfSurveyByCod (&Svy);
   if (!Svy.Status.ICanEdit)
      Err_NoPermissionExit ();

   /***** Hide survey *****/
   Svy_DB_HideOrUnhideSurvey (Svy.SvyCod,true);

   /***** Show surveys again *****/
   Svy_ListAllSurveys (&Surveys);
  }

/*****************************************************************************/
/******************************** Show a survey ******************************/
/*****************************************************************************/

void Svy_UnhideSurvey (void)
  {
   struct Svy_Surveys Surveys;
   struct Svy_Survey Svy;

   /***** Reset surveys *****/
   Svy_ResetSurveys (&Surveys);

   /***** Get parameters *****/
   Surveys.SelectedOrder = Svy_GetParamSvyOrder ();
   Grp_GetParamWhichGroups ();
   Surveys.CurrentPage = Pag_GetParamPagNum (Pag_SURVEYS);

   /***** Get survey code *****/
   if ((Svy.SvyCod = Svy_GetParamSvyCod ()) <= 0)
      Err_WrongSurveyExit ();

   /***** Get data of the survey from database *****/
   Svy_GetDataOfSurveyByCod (&Svy);
   if (!Svy.Status.ICanEdit)
      Err_NoPermissionExit ();

   /***** Show survey *****/
   Svy_DB_HideOrUnhideSurvey (Svy.SvyCod,false);

   /***** Show surveys again *****/
   Svy_ListAllSurveys (&Surveys);
  }

/*****************************************************************************/
/********************* Put a form to create a new survey *********************/
/*****************************************************************************/

void Svy_RequestCreatOrEditSvy (void)
  {
   extern const char *Hlp_ASSESSMENT_Surveys_new_survey;
   extern const char *Hlp_ASSESSMENT_Surveys_edit_survey;
   extern const char *Txt_New_survey;
   extern const char *Txt_Scope;
   extern const char *Txt_Edit_survey;
   extern const char *Txt_Title;
   extern const char *Txt_Description;
   extern const char *Txt_Users;
   extern const char *Txt_Create_survey;
   extern const char *Txt_Save_changes;
   struct Svy_Surveys Surveys;
   struct Svy_Survey Svy;
   bool ItsANewSurvey;
   char Txt[Cns_MAX_BYTES_TEXT + 1];
   static const Dat_SetHMS SetHMS[Dat_NUM_START_END_TIME] =
     {
      [Dat_STR_TIME] = Dat_HMS_TO_000000,
      [Dat_END_TIME] = Dat_HMS_TO_235959
     };

   /***** Reset surveys *****/
   Svy_ResetSurveys (&Surveys);

   /***** Get parameters *****/
   Surveys.SelectedOrder = Svy_GetParamSvyOrder ();
   Grp_GetParamWhichGroups ();
   Surveys.CurrentPage = Pag_GetParamPagNum (Pag_SURVEYS);

   /***** Get the code of the survey *****/
   ItsANewSurvey = ((Svy.SvyCod = Svy_GetParamSvyCod ()) <= 0);

   /***** Get from the database the data of the survey *****/
   if (ItsANewSurvey)
     {
      /***** Put link (form) to create new survey *****/
      if (!Svy_CheckIfICanCreateSvy ())
         Err_NoPermissionExit ();

      /* Initialize to empty survey */
      Svy.SvyCod = -1L;
      Svy.Scope  = HieLvl_UNK;
      Svy.Roles  = (1 << Rol_STD);
      Svy.UsrCod = Gbl.Usrs.Me.UsrDat.UsrCod;
      Svy.TimeUTC[Dat_STR_TIME] = Gbl.StartExecutionTimeUTC;
      Svy.TimeUTC[Dat_END_TIME] = Gbl.StartExecutionTimeUTC + (24 * 60 * 60);	// +24 hours
      Svy.Title[0] = '\0';
      Svy.NumQsts = 0;
      Svy.NumUsrs = 0;
      Svy.Status.Hidden = false;
      Svy.Status.Open = true;
      Svy.Status.IAmLoggedWithAValidRoleToAnswer = false;
      Svy.Status.IBelongToScope = false;
      Svy.Status.IHaveAnswered = false;
      Svy.Status.ICanAnswer = false;
      Svy.Status.ICanViewResults = false;
     }
   else
     {
      /* Get data of the survey from database */
      Svy_GetDataOfSurveyByCod (&Svy);
      if (!Svy.Status.ICanEdit)
         Err_NoPermissionExit ();

      /* Get text of the survey from database */
      Svy_DB_GetSurveyTxt (Svy.SvyCod,Txt);
     }

   /***** Begin form *****/
   Surveys.SvyCod = Svy.SvyCod;
   Frm_BeginForm (ItsANewSurvey ? ActNewSvy :
	                          ActChgSvy);
      Svy_PutParams (&Surveys);

      /***** Begin box and table *****/
      if (ItsANewSurvey)
	 Box_BoxTableBegin (NULL,Txt_New_survey,
			    NULL,NULL,
			    Hlp_ASSESSMENT_Surveys_new_survey,Box_NOT_CLOSABLE,2);
      else
	 Box_BoxTableBegin (NULL,
			    Svy.Title[0] ? Svy.Title :
					   Txt_Edit_survey,
			    NULL,NULL,
			    Hlp_ASSESSMENT_Surveys_edit_survey,Box_NOT_CLOSABLE,2);

      /***** Scope of the survey *****/
      HTM_TR_Begin (NULL);

	 /* Label */
	 Frm_LabelColumn ("RT","ScopeSvy",Txt_Scope);

	 /* Data */
	 HTM_TD_Begin ("class=\"LT\"");
	    Svy_SetDefaultAndAllowedScope (&Svy);
	    Sco_GetScope ("ScopeSvy");
	    Sco_PutSelectorScope ("ScopeSvy",HTM_DONT_SUBMIT_ON_CHANGE);
	 HTM_TD_End ();

      HTM_TR_End ();

      /***** Survey title *****/
      HTM_TR_Begin (NULL);

	 /* Label */
	 Frm_LabelColumn ("RT","Title",Txt_Title);

	 /* Data */
	 HTM_TD_Begin ("class=\"LT\"");
	    HTM_INPUT_TEXT ("Title",Svy_MAX_CHARS_SURVEY_TITLE,Svy.Title,
			    HTM_DONT_SUBMIT_ON_CHANGE,
			    "id=\"Title\""
			    " class=\"TITLE_DESCRIPTION_WIDTH INPUT_%s\""
			    " required=\"required\"",
			    The_GetSuffix ());
	 HTM_TD_End ();

      HTM_TR_End ();

      /***** Survey start and end dates *****/
      Dat_PutFormStartEndClientLocalDateTimes (Svy.TimeUTC,
					       Dat_FORM_SECONDS_ON,
					       SetHMS);

      /***** Survey text *****/
      HTM_TR_Begin (NULL);

	 /* Label */
	 Frm_LabelColumn ("RT","Txt",Txt_Description);

	 /* Data */
	 HTM_TD_Begin ("class=\"LT\"");
	    HTM_TEXTAREA_Begin ("id=\"Txt\" name=\"Txt\" rows=\"5\""
				" class=\"TITLE_DESCRIPTION_WIDTH INPUT_%s\"",
				The_GetSuffix ());
	       if (!ItsANewSurvey)
		  HTM_Txt (Txt);
	    HTM_TEXTAREA_End ();
	 HTM_TD_End ();

      HTM_TR_End ();

      /***** Users' roles who can answer the survey *****/
      HTM_TR_Begin (NULL);

	 HTM_TD_Begin ("class=\"RT FORM_IN_%s\"",The_GetSuffix ());
	    HTM_TxtColon (Txt_Users);
	 HTM_TD_End ();

	 HTM_TD_Begin ("class=\"LM DAT_%s\"",The_GetSuffix ());
	    Rol_WriteSelectorRoles (1 << Rol_STD |
				    1 << Rol_NET |
				    1 << Rol_TCH,
				    Svy.Roles,
				    false,false);
	 HTM_TD_End ();

      HTM_TR_End ();

      /***** Groups *****/
      Svy_ShowLstGrpsToEditSurvey (Svy.SvyCod);

      /***** End table, send button and end box *****/
      if (ItsANewSurvey)
	 Box_BoxTableWithButtonEnd (Btn_CREATE_BUTTON,Txt_Create_survey);
      else
	 Box_BoxTableWithButtonEnd (Btn_CONFIRM_BUTTON,Txt_Save_changes);

   /***** End form *****/
   Frm_EndForm ();

   /***** Show questions of the survey ready to be edited *****/
   if (!ItsANewSurvey)
      Svy_ListSvyQuestions (&Surveys,&Svy);

   /***** Show all surveys *****/
   Svy_ListAllSurveys (&Surveys);
  }

/*****************************************************************************/
/****** Set default and allowed scopes depending on logged user's role *******/
/*****************************************************************************/

static void Svy_SetDefaultAndAllowedScope (struct Svy_Survey *Svy)
  {
   bool ICanEdit = false;

   /***** Set default scope *****/
   Gbl.Scope.Default = HieLvl_UNK;
   Gbl.Scope.Allowed = 0;

   switch (Gbl.Usrs.Me.Role.Logged)
     {
      case Rol_TCH:	// Teachers only can edit course surveys
	 if (Gbl.Hierarchy.Level == HieLvl_CRS)	// Course selected
	   {
	    if (Svy->Scope == HieLvl_UNK)		// Scope not defined
	       Svy->Scope = HieLvl_CRS;
	    if (Svy->Scope == HieLvl_CRS)
	      {
               Gbl.Scope.Default = Svy->Scope;
	       Gbl.Scope.Allowed = 1 << HieLvl_CRS;
	       ICanEdit = true;
	      }
	   }
         break;
      case Rol_DEG_ADM:	// Degree admins only can edit degree surveys
	 if (Svy->Scope == HieLvl_UNK)		// Scope not defined
	    Svy->Scope = HieLvl_DEG;
	 if (Svy->Scope == HieLvl_DEG)
	   {
	    Gbl.Scope.Default = Svy->Scope;
	    Gbl.Scope.Allowed = 1 << HieLvl_DEG;
	    ICanEdit = true;
	   }
         break;
      case Rol_CTR_ADM:	// Center admins only can edit center surveys
	 if (Svy->Scope == HieLvl_UNK)		// Scope not defined
	    Svy->Scope = HieLvl_CTR;
	 if (Svy->Scope == HieLvl_CTR)
	   {
	    Gbl.Scope.Default = Svy->Scope;
	    Gbl.Scope.Allowed = 1 << HieLvl_CTR;
	    ICanEdit = true;
	   }
         break;
      case Rol_INS_ADM:	// Institution admins only can edit institution surveys
	 if (Svy->Scope == HieLvl_UNK)		// Scope not defined
	    Svy->Scope = HieLvl_INS;
	 if (Svy->Scope == HieLvl_INS)
	   {
	    Gbl.Scope.Default = Svy->Scope;
	    Gbl.Scope.Allowed = 1 << HieLvl_INS;
	    ICanEdit = true;
	   }
         break;
      case Rol_SYS_ADM:// System admins can edit any survey
	 if (Svy->Scope == HieLvl_UNK)	// Scope not defined
	    Svy->Scope = (Gbl.Hierarchy.Level < HieLvl_NUM_LEVELS &&
		          Gbl.Hierarchy.Level != HieLvl_UNK) ? Gbl.Hierarchy.Level :
		        	                            HieLvl_SYS;
         Gbl.Scope.Default = Svy->Scope;
         Gbl.Scope.Allowed = 1 << HieLvl_SYS |
	                     1 << HieLvl_CTY |
	                     1 << HieLvl_INS |
	                     1 << HieLvl_CTR |
                             1 << HieLvl_DEG |
                             1 << HieLvl_CRS;
	 ICanEdit = true;
	 break;
      default:
	 break;
     }

   if (!ICanEdit)
      Err_NoPermissionExit ();
  }

/*****************************************************************************/
/******************** Show list of groups to edit a survey *******************/
/*****************************************************************************/

static void Svy_ShowLstGrpsToEditSurvey (long SvyCod)
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
	    Box_BoxTableBegin ("95%",NULL,
			       NULL,NULL,
			       NULL,Box_NOT_CLOSABLE,0);

	    /***** First row: checkbox to select the whole course *****/
	    HTM_TR_Begin (NULL);

	       HTM_TD_Begin ("colspan=\"7\" class=\"LM DAT_%s\"",
	                     The_GetSuffix ());
		  HTM_LABEL_Begin (NULL);
		     HTM_INPUT_CHECKBOX ("WholeCrs",HTM_DONT_SUBMIT_ON_CHANGE,
					 "id=\"WholeCrs\" value=\"Y\"%s onclick=\"uncheckChildren(this,'GrpCods')\"",
					 Grp_DB_CheckIfAssociatedToGrps ("svy_groups","SvyCod",SvyCod) ? "" :
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
						     Grp_SURVEY,SvyCod);

	    /***** End table and box *****/
	    Box_BoxTableEnd ();
	 HTM_TD_End ();
      HTM_TR_End ();
     }

   /***** Free list of groups types and groups in this course *****/
   Grp_FreeListGrpTypesAndGrps ();
  }

/*****************************************************************************/
/********************* Receive form to create a new survey *******************/
/*****************************************************************************/

void Svy_ReceiveFormSurvey (void)
  {
   extern const char *Txt_Already_existed_a_survey_with_the_title_X;
   struct Svy_Surveys Surveys;
   struct Svy_Survey OldSvy;
   struct Svy_Survey NewSvy;
   bool ItsANewSurvey;
   bool NewSurveyIsCorrect = true;
   unsigned NumUsrsToBeNotifiedByEMail;
   char Txt[Cns_MAX_BYTES_TEXT + 1];

   /***** Reset surveys *****/
   Svy_ResetSurveys (&Surveys);

   /***** Get parameters *****/
   Surveys.SelectedOrder = Svy_GetParamSvyOrder ();
   Grp_GetParamWhichGroups ();
   Surveys.CurrentPage = Pag_GetParamPagNum (Pag_SURVEYS);

   /***** Get the code of the survey *****/
   ItsANewSurvey = ((NewSvy.SvyCod = Svy_GetParamSvyCod ()) <= 0);

   if (ItsANewSurvey)
      NewSvy.Scope = HieLvl_UNK;
   else
     {
      /* Get data of the old (current) survey from database */
      OldSvy.SvyCod = NewSvy.SvyCod;
      Svy_GetDataOfSurveyByCod (&OldSvy);
      if (!OldSvy.Status.ICanEdit)
         Err_NoPermissionExit ();
      NewSvy.Scope = OldSvy.Scope;
     }

   /***** Get scope *****/
   Svy_SetDefaultAndAllowedScope (&NewSvy);
   Sco_GetScope ("ScopeSvy");
   switch (Gbl.Scope.Current)
     {
      case HieLvl_SYS:
	 if (Gbl.Usrs.Me.Role.Logged != Rol_SYS_ADM)
	    Err_WrongScopeExit ();
         NewSvy.Scope = HieLvl_SYS;
         NewSvy.Cod = -1L;
         break;
      case HieLvl_CTY:
	 if (Gbl.Usrs.Me.Role.Logged != Rol_SYS_ADM)
	    Err_WrongScopeExit ();
	 NewSvy.Scope = HieLvl_CTY;
	 NewSvy.Cod = Gbl.Hierarchy.Cty.CtyCod;
         break;
      case HieLvl_INS:
	 if (Gbl.Usrs.Me.Role.Logged != Rol_SYS_ADM &&
	     Gbl.Usrs.Me.Role.Logged != Rol_INS_ADM)
	    Err_WrongScopeExit ();
	 NewSvy.Scope = HieLvl_INS;
	 NewSvy.Cod = Gbl.Hierarchy.Ins.InsCod;
         break;
      case HieLvl_CTR:
	 if (Gbl.Usrs.Me.Role.Logged != Rol_SYS_ADM &&
	     Gbl.Usrs.Me.Role.Logged != Rol_CTR_ADM)
	    Err_WrongScopeExit ();
	 NewSvy.Scope = HieLvl_CTR;
	 NewSvy.Cod = Gbl.Hierarchy.Ctr.CtrCod;
         break;
      case HieLvl_DEG:
	 if (Gbl.Usrs.Me.Role.Logged != Rol_SYS_ADM &&
	     Gbl.Usrs.Me.Role.Logged != Rol_DEG_ADM)
	    Err_WrongScopeExit ();
	 NewSvy.Scope = HieLvl_DEG;
	 NewSvy.Cod = Gbl.Hierarchy.Deg.DegCod;
         break;
      case HieLvl_CRS:
	 if (Gbl.Usrs.Me.Role.Logged != Rol_SYS_ADM &&
	     Gbl.Usrs.Me.Role.Logged != Rol_TCH)
	    Err_WrongScopeExit ();
	 NewSvy.Scope = HieLvl_CRS;
	 NewSvy.Cod = Gbl.Hierarchy.Crs.CrsCod;
         break;
      default:
	 Err_WrongScopeExit ();
	 break;
     }

   /***** Get start/end date-times *****/
   NewSvy.TimeUTC[Dat_STR_TIME] = Dat_GetTimeUTCFromForm ("StartTimeUTC");
   NewSvy.TimeUTC[Dat_END_TIME] = Dat_GetTimeUTCFromForm ("EndTimeUTC"  );

   /***** Get survey title *****/
   Par_GetParToText ("Title",NewSvy.Title,Svy_MAX_BYTES_SURVEY_TITLE);

   /***** Get survey text and insert links *****/
   Par_GetParToHTML ("Txt",Txt,Cns_MAX_BYTES_TEXT);	// Store in HTML format (not rigorous)

   /***** Adjust dates *****/
   if (NewSvy.TimeUTC[Dat_STR_TIME] == 0)
      NewSvy.TimeUTC[Dat_STR_TIME] = Gbl.StartExecutionTimeUTC;
   if (NewSvy.TimeUTC[Dat_END_TIME] == 0)
      NewSvy.TimeUTC[Dat_END_TIME] = NewSvy.TimeUTC[Dat_STR_TIME] + 24 * 60 * 60;	// +24 hours

   /***** Get users who can answer this survey *****/
   NewSvy.Roles = Rol_GetSelectedRoles ();

   /***** Check if title is correct *****/
   if (NewSvy.Title[0])	// If there's a survey title
     {
      /* If title of survey was in database... */
      if (Svy_DB_CheckIfSimilarSurveyExists (&NewSvy))
        {
         NewSurveyIsCorrect = false;
         Ale_ShowAlert (Ale_WARNING,Txt_Already_existed_a_survey_with_the_title_X,
                        NewSvy.Title);
        }
     }
   else	// If there is not a survey title
     {
      NewSurveyIsCorrect = false;
      Ale_ShowAlertYouMustSpecifyTheTitle ();
     }

   /***** Create a new survey or update an existing one *****/
   if (NewSurveyIsCorrect)
     {
      /* Get groups for this surveys */
      Grp_GetParCodsSeveralGrps ();

      if (ItsANewSurvey)
         Svy_CreateSurvey (&NewSvy,Txt);	// Add new survey to database
      else
         Svy_UpdateSurvey (&NewSvy,Txt);

      /* Free memory for list of selected groups */
      Grp_FreeListCodSelectedGrps ();
     }
   else
      Svy_RequestCreatOrEditSvy ();

   /***** Notify by email about the new survey *****/
   if (NewSvy.Scope == HieLvl_CRS)	// Notify only the surveys for a course, not for a degree or global
      if ((NumUsrsToBeNotifiedByEMail = Ntf_StoreNotifyEventsToAllUsrs (Ntf_EVENT_SURVEY,NewSvy.SvyCod)))
         Svy_DB_UpdateNumUsrsNotifiedByEMailAboutSurvey (NewSvy.SvyCod,NumUsrsToBeNotifiedByEMail);

   /***** Show surveys again *****/
   Svy_ListAllSurveys (&Surveys);
  }

/*****************************************************************************/
/*************************** Create a new survey *****************************/
/*****************************************************************************/

static void Svy_CreateSurvey (struct Svy_Survey *Svy,const char *Txt)
  {
   extern const char *Txt_Created_new_survey_X;

   /***** Create a new survey *****/
   Svy->SvyCod = Svy_DB_CreateSurvey (Svy,Txt);

   /***** Create groups *****/
   if (Gbl.Crs.Grps.LstGrpsSel.NumGrps)
      Svy_CreateGrps (Svy->SvyCod);

   /***** Write success message *****/
   Ale_ShowAlert (Ale_SUCCESS,Txt_Created_new_survey_X,
                  Svy->Title);
  }

/*****************************************************************************/
/************************* Update an existing survey *************************/
/*****************************************************************************/

static void Svy_UpdateSurvey (struct Svy_Survey *Svy,const char *Txt)
  {
   extern const char *Txt_The_survey_has_been_modified;

   /***** Update the data of the survey *****/
   Svy_DB_UpdateSurvey (Svy,Txt);

   /***** Update groups *****/
   /* Remove old groups */
   Svy_DB_RemoveGrpsAssociatedToSurvey (Svy->SvyCod);

   /* Create new groups */
   if (Gbl.Crs.Grps.LstGrpsSel.NumGrps)
      Svy_CreateGrps (Svy->SvyCod);

   /***** Write success message *****/
   Ale_ShowAlert (Ale_SUCCESS,Txt_The_survey_has_been_modified);
  }

/*****************************************************************************/
/************************ Create groups of a survey **************************/
/*****************************************************************************/

static void Svy_CreateGrps (long SvyCod)
  {
   unsigned NumGrpSel;

   /***** Create groups of the survey *****/
   for (NumGrpSel = 0;
	NumGrpSel < Gbl.Crs.Grps.LstGrpsSel.NumGrps;
	NumGrpSel++)
      Svy_DB_CreateGrp (SvyCod,Gbl.Crs.Grps.LstGrpsSel.GrpCods[NumGrpSel]);
  }

/*****************************************************************************/
/************ Get and write the names of the groups of a survey **************/
/*****************************************************************************/

static void Svy_GetAndWriteNamesOfGrpsAssociatedToSvy (struct Svy_Survey *Svy)
  {
   extern const char *Txt_Group;
   extern const char *Txt_Groups;
   extern const char *Txt_and;
   extern const char *Txt_The_whole_course;
   MYSQL_RES *mysql_res;
   MYSQL_ROW row;
   unsigned NumGrps;
   unsigned NumGrp;

   /***** Get groups associated to a survey from database *****/
   NumGrps = Svy_DB_GetGrpNamesAssociatedToSvy (&mysql_res,Svy->SvyCod);

   /***** Write heading *****/
   HTM_DIV_Begin ("class=\"%s_%s\"",
                  Svy->Status.Hidden ? "ASG_GRP_LIGHT" :
                	               "ASG_GRP",
        	  The_GetSuffix ());
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

	    /* Write group type name and group name */
	    HTM_TxtF ("%s&nbsp;%s",row[0],row[1]);

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
	 HTM_TxtF ("%s&nbsp;%s",Txt_The_whole_course,Gbl.Hierarchy.Crs.ShrtName);

   HTM_DIV_End ();

   /***** Free structure that stores the query result *****/
   DB_FreeMySQLResult (&mysql_res);
  }

/*****************************************************************************/
/************* Remove all surveys in a place on the hierarchy   **************/
/************* (country, institution, center, degree or course) **************/
/*****************************************************************************/

void Svy_RemoveSurveys (HieLvl_Level_t Scope,long Cod)
  {
   /***** Remove all users in surveys *****/
   Svy_DB_RemoveUsrsWhoHaveAnsweredSvysIn (Scope,Cod);

   /***** Remove all answers in surveys *****/
   Svy_DB_RemoveAnswersSvysIn (Scope,Cod);

   /***** Remove all questions in surveys *****/
   Svy_DB_RemoveQstsSvysIn (Scope,Cod);

   /***** Remove all groups *****/
   Svy_DB_RemoveGrpsSvysIn (Scope,Cod);

   /***** Remove all surveys *****/
   Svy_DB_RemoveSvysIn (Scope,Cod);
  }

/*****************************************************************************/
/*********** Put a form to edit/create a question in survey  *****************/
/*****************************************************************************/

void Svy_RequestEditQuestion (void)
  {
   struct Svy_Surveys Surveys;
   struct Svy_Question SvyQst;
   long SvyCod;
   char Stem[Cns_MAX_BYTES_TEXT + 1];

   /***** Reset surveys *****/
   Svy_ResetSurveys (&Surveys);

   /***** Initialize question to zero *****/
   Svy_InitQst (&SvyQst);

   /***** Initialize text to empty string *****/
   Stem[0] = '\0';

   /***** Get survey code *****/
   if ((SvyCod = Svy_GetParamSvyCod ()) <= 0)
      Err_WrongSurveyExit ();

   /* Get the question code */
   SvyQst.QstCod = Svy_GetParamQstCod ();

   /***** Get other parameters *****/
   Surveys.SelectedOrder = Svy_GetParamSvyOrder ();
   Grp_GetParamWhichGroups ();
   Surveys.CurrentPage = Pag_GetParamPagNum (Pag_SURVEYS);

   /***** Show form to create a new question in this survey *****/
   Svy_ShowFormEditOneQst (&Surveys,SvyCod,&SvyQst,Stem);

   /***** Show current survey *****/
   Svy_ShowOneSurvey (&Surveys,SvyCod,true);
  }

/*****************************************************************************/
/******************* Show form to edit one survey question *******************/
/*****************************************************************************/

static void Svy_ShowFormEditOneQst (struct Svy_Surveys *Surveys,
                                    long SvyCod,struct Svy_Question *SvyQst,
                                    char Stem[Cns_MAX_BYTES_TEXT + 1])
  {
   extern const char *Hlp_ASSESSMENT_Surveys_questions;
   extern const char *Txt_Question;
   extern const char *Txt_New_question;
   extern const char *Txt_Wording;
   extern const char *Txt_Type;
   extern const char *Txt_SURVEY_STR_ANSWER_TYPES[Svy_NUM_ANS_TYPES];
   extern const char *Txt_Save_changes;
   extern const char *Txt_Create_question;
   MYSQL_RES *mysql_res;
   MYSQL_ROW row;
   unsigned NumAns;
   unsigned NumAnswers = 0;
   char *Title;
   Svy_AnswerType_t AnsType;

   if (Gbl.Action.Act == ActEdiOneSvyQst) // If no receiving the question, but editing a new or existing question
     {
      if ((SvyQst->QstCod > 0))	// If parameter QstCod received ==> question already exists in the database
        {
         /***** Get question data from database *****/
         if (Svy_DB_GetQstDataByCod (&mysql_res,SvyQst->QstCod,SvyCod))
           {
	    row = mysql_fetch_row (mysql_res);
            Svy_GetDataOfQstFromRow (SvyQst,Stem,row);
           }
         else
            Err_WrongQuestionExit ();

         /* Free structure that stores the query result */
	 DB_FreeMySQLResult (&mysql_res);

         /***** Get the answers from the database *****/
         NumAnswers = Svy_DB_GetAnswersQst (&mysql_res,SvyQst->QstCod);	// Result: AnsInd,NumUsrs,Answer
         for (NumAns = 0;
              NumAns < NumAnswers;
              NumAns++)
           {
            row = mysql_fetch_row (mysql_res);

            if (NumAnswers > Svy_MAX_ANSWERS_PER_QUESTION)
               Err_WrongAnswerExit ();
            if (!Svy_AllocateTextChoiceAnswer (SvyQst,NumAns))
	       /* Abort on error */
	       Ale_ShowAlertsAndExit ();

            Str_Copy (SvyQst->AnsChoice[NumAns].Text,row[2],Svy_MAX_BYTES_ANSWER);
           }
         /* Free structure that stores the query result */
	 DB_FreeMySQLResult (&mysql_res);
        }
     }

   /***** Begin box *****/
   if (SvyQst->QstCod > 0)	// If the question already has assigned a code
     {
      /* Parameters for contextual icon */
      Surveys->SvyCod = SvyCod;
      Surveys->QstCod = SvyQst->QstCod;

      if (asprintf (&Title,"%s %u",Txt_Question,SvyQst->QstInd + 1) < 0)	// Question index may be 0, 1, 2, 3,...
	 Err_NotEnoughMemoryExit ();
      Box_BoxBegin (NULL,Title,
                    Svy_PutIconToRemoveOneQst,Surveys,
                    NULL,Box_NOT_CLOSABLE);
      free (Title);
     }
   else
      Box_BoxBegin (NULL,Txt_New_question,
                    NULL,NULL,
                    Hlp_ASSESSMENT_Surveys_questions,Box_NOT_CLOSABLE);

   /***** Begin form *****/
   Frm_BeginForm (ActRcvSvyQst);
      Svy_PutParamSvyCod (SvyCod);
      if (SvyQst->QstCod > 0)	// If the question already has assigned a code
	 Svy_PutParamQstCod (SvyQst->QstCod);

      /***** Begin table *****/
      HTM_TABLE_BeginWidePadding (2);

	 /***** Stem *****/
	 HTM_TR_Begin (NULL);

	    /* Label */
	    Frm_LabelColumn ("RT","Txt",Txt_Wording);

	    /* Data */
	    HTM_TD_Begin ("class=\"LT\"");
	       HTM_TEXTAREA_Begin ("id=\"Txt\" name=\"Txt\""
			           " cols=\"60\" rows=\"4\""
			           " class=\"INPUT_%s\"",
			           The_GetSuffix ());
		  HTM_Txt (Stem);
	       HTM_TEXTAREA_End ();
	    HTM_TD_End ();

	 HTM_TR_End ();

	 /***** Type of answer *****/
	 HTM_TR_Begin (NULL);

	    HTM_TD_Begin ("class=\"RT FORM_IN_%s\"",The_GetSuffix ());
	       HTM_TxtColon (Txt_Type);
	    HTM_TD_End ();

	    HTM_TD_Begin ("class=\"LT FORM_IN_%s\"",The_GetSuffix ());
	       for (AnsType  = (Svy_AnswerType_t) 0;
		    AnsType <= (Svy_AnswerType_t) (Svy_NUM_ANS_TYPES - 1);
		    AnsType++)
		 {
		  HTM_LABEL_Begin (NULL);
		     HTM_INPUT_RADIO ("AnswerType",false,
				      "value=\"%u\"%s",
				      (unsigned) AnsType,
				      AnsType == SvyQst->AnswerType ? " checked=\"checked\"" : "");
		     HTM_Txt (Txt_SURVEY_STR_ANSWER_TYPES[AnsType]);
		  HTM_LABEL_End ();
		  HTM_BR ();
		 }
	    HTM_TD_End ();

	 HTM_TR_End ();

	 /***** Answers *****/
	 HTM_TR_Begin (NULL);

	    HTM_TD_Empty (1);

	    /* Unique or multiple choice answers */
	    HTM_TD_Begin ("class=\"LT\"");
	       HTM_TABLE_BeginPadding (2);
		  for (NumAns = 0;
		       NumAns < Svy_MAX_ANSWERS_PER_QUESTION;
		       NumAns++)
		    {
		     HTM_TR_Begin (NULL);

			/* Label with the number of the answer */
			HTM_TD_Begin ("class=\"RT\"");
			   HTM_LABEL_Begin ("for=\"AnsStr%u\" class=\"FORM_IN_%s\"",
					    NumAns,The_GetSuffix ());
			      HTM_TxtF ("%u)",NumAns + 1);
			   HTM_LABEL_End ();
			HTM_TD_End ();

			/* Answer text */
			HTM_TD_Begin ("class=\"RT\"");
			   HTM_TEXTAREA_Begin ("id=\"AnsStr%u\" name=\"AnsStr%u\" cols=\"50\" rows=\"1\"",
					       NumAns,NumAns);
			      if (SvyQst->AnsChoice[NumAns].Text)
				 HTM_Txt (SvyQst->AnsChoice[NumAns].Text);
			   HTM_TEXTAREA_End ();
			HTM_TD_End ();

		     HTM_TR_End ();
		    }
	       HTM_TABLE_End ();
	    HTM_TD_End ();

	 HTM_TR_End ();

      /***** End table *****/
      HTM_TABLE_End ();

      /***** Send button *****/
      if (SvyQst->QstCod > 0)	// If the question already has assigned a code
	 Btn_PutConfirmButton (Txt_Save_changes);
      else
	 Btn_PutCreateButton (Txt_Create_question);

   /***** End form *****/
   Frm_EndForm ();

   /***** End box *****/
   Box_BoxEnd ();

   /***** Free memory for answers *****/
   Svy_FreeTextChoiceAnswers (SvyQst,NumAnswers);
  }

/*****************************************************************************/
/********************* Initialize a new question to zero *********************/
/*****************************************************************************/

static void Svy_InitQst (struct Svy_Question *SvyQst)
  {
   unsigned NumAns;

   SvyQst->QstCod = -1L;
   SvyQst->QstInd = 0;
   SvyQst->AnswerType = Svy_ANS_UNIQUE_CHOICE;
   for (NumAns = 0;
	NumAns < Svy_MAX_ANSWERS_PER_QUESTION;
	NumAns++)
      SvyQst->AnsChoice[NumAns].Text = NULL;
  }

/*****************************************************************************/
/****************** Write parameter with code of question ********************/
/*****************************************************************************/

static void Svy_PutParamQstCod (long QstCod)
  {
   Par_PutHiddenParamLong (NULL,"QstCod",QstCod);
  }

/*****************************************************************************/
/******************* Get parameter with code of question *********************/
/*****************************************************************************/

static long Svy_GetParamQstCod (void)
  {
   /***** Get code of question *****/
   return Par_GetParToLong ("QstCod");
  }

/*****************************************************************************/
/******************* Allocate memory for a choice answer *********************/
/*****************************************************************************/

static bool Svy_AllocateTextChoiceAnswer (struct Svy_Question *SvyQst,
                                          unsigned NumAns)
  {
   Svy_FreeTextChoiceAnswer (SvyQst,NumAns);
   if ((SvyQst->AnsChoice[NumAns].Text = malloc (Svy_MAX_BYTES_ANSWER + 1)) == NULL)
     {
      Ale_CreateAlert (Ale_ERROR,NULL,
	               "Not enough memory to store answer.");
      return false;
     }
   SvyQst->AnsChoice[NumAns].Text[0] = '\0';
   return true;
  }

/*****************************************************************************/
/******************** Free memory of all choice answers **********************/
/*****************************************************************************/

static void Svy_FreeTextChoiceAnswers (struct Svy_Question *SvyQst,unsigned NumAnswers)
  {
   unsigned NumAns;

   for (NumAns = 0;
	NumAns < NumAnswers;
	NumAns++)
      Svy_FreeTextChoiceAnswer (SvyQst,NumAns);
  }

/*****************************************************************************/
/********************** Free memory of a choice answer ***********************/
/*****************************************************************************/

static void Svy_FreeTextChoiceAnswer (struct Svy_Question *SvyQst,unsigned NumAns)
  {
   if (SvyQst->AnsChoice[NumAns].Text)
     {
      free (SvyQst->AnsChoice[NumAns].Text);
      SvyQst->AnsChoice[NumAns].Text = NULL;
     }
  }

/*****************************************************************************/
/*********************** Receive a question of a survey **********************/
/*****************************************************************************/

void Svy_ReceiveQst (void)
  {
   extern const char *Txt_You_must_type_the_question_stem;
   extern const char *Txt_You_can_not_leave_empty_intermediate_answers;
   extern const char *Txt_You_must_type_at_least_the_first_two_answers;
   extern const char *Txt_The_survey_has_been_modified;
   struct Svy_Surveys Surveys;
   struct Svy_Question SvyQst;
   long SvyCod;
   char Stem[Cns_MAX_BYTES_TEXT + 1];
   unsigned NumAns;
   char AnsStr[8 + 10 + 1];
   unsigned NumLastAns;
   bool ThereIsEndOfAnswers;
   bool Error = false;

   /***** Reset surveys *****/
   Svy_ResetSurveys (&Surveys);

   /***** Initialize question to zero *****/
   Svy_InitQst (&SvyQst);

   /***** Get parameters from form *****/
   /* Get survey code */
   if ((SvyCod = Svy_GetParamSvyCod ()) <= 0)
      Err_WrongSurveyExit ();

   /* Get question code */
   SvyQst.QstCod = Svy_GetParamQstCod ();

   /* Get answer type */
   SvyQst.AnswerType = (Svy_AnswerType_t)
	               Par_GetParToUnsignedLong ("AnswerType",
	                                         0,
	                                         Svy_NUM_ANS_TYPES - 1,
                                                 (unsigned long) Svy_ANSWER_TYPE_DEFAULT);

   /* Get question text */
   Par_GetParToHTML ("Txt",Stem,Cns_MAX_BYTES_TEXT);

   /* Get the texts of the answers */
   for (NumAns = 0;
	NumAns < Svy_MAX_ANSWERS_PER_QUESTION;
	NumAns++)
     {
      if (!Svy_AllocateTextChoiceAnswer (&SvyQst,NumAns))
	 /* Abort on error */
	 Ale_ShowAlertsAndExit ();
      snprintf (AnsStr,sizeof (AnsStr),"AnsStr%u",NumAns);
      Par_GetParToHTML (AnsStr,SvyQst.AnsChoice[NumAns].Text,Svy_MAX_BYTES_ANSWER);
     }

   /***** Make sure that stem and answer are not empty *****/
   if (Stem[0])
     {
      if (SvyQst.AnsChoice[0].Text[0])	// If the first answer has been filled
        {
         for (NumAns = 0, NumLastAns = 0, ThereIsEndOfAnswers = false;
              !Error && NumAns < Svy_MAX_ANSWERS_PER_QUESTION;
              NumAns++)
            if (SvyQst.AnsChoice[NumAns].Text[0])
              {
               if (ThereIsEndOfAnswers)
                 {
                  Ale_ShowAlert (Ale_WARNING,Txt_You_can_not_leave_empty_intermediate_answers);
                  Error = true;
                 }
               else
                  NumLastAns = NumAns;
              }
            else
               ThereIsEndOfAnswers = true;
         if (!Error)
           {
            if (NumLastAns < 1)
              {
               Ale_ShowAlert (Ale_WARNING,Txt_You_must_type_at_least_the_first_two_answers);
               Error = true;
              }
           }
        }
      else	// If first answer is empty
        {
         Ale_ShowAlert (Ale_WARNING,Txt_You_must_type_at_least_the_first_two_answers);
         Error = true;
        }
     }
   else
     {
      Ale_ShowAlert (Ale_WARNING,Txt_You_must_type_the_question_stem);
      Error = true;
     }

   if (Error)
      Svy_ShowFormEditOneQst (&Surveys,SvyCod,&SvyQst,Stem);
   else
     {
      /***** Form is received OK ==> insert question and answer in the database *****/
      if (SvyQst.QstCod < 0)	// It's a new question
        {
         SvyQst.QstInd = Svy_GetNextQuestionIndexInSvy (SvyCod);
         SvyQst.QstCod = Svy_DB_CreateQuestion (SvyCod,SvyQst.QstInd,
				                SvyQst.AnswerType,Stem);
        }
      else			// It's an existing question
         /* Update question */
         Svy_DB_UpdateQuestion (SvyCod,SvyQst.QstCod,SvyQst.AnswerType,Stem);

      /* Insert, update or delete answers in the answers table */
      for (NumAns = 0;
	   NumAns < Svy_MAX_ANSWERS_PER_QUESTION;
	   NumAns++)
         if (Svy_DB_CheckIfAnswerExists (SvyQst.QstCod,NumAns))	// If this answer exists...
           {
            if (SvyQst.AnsChoice[NumAns].Text[0])	// Answer is not empty
               /* Update answer text */
               Svy_DB_UpdateAnswerText (SvyQst.QstCod,NumAns,
                                        SvyQst.AnsChoice[NumAns].Text);
            else					// Answer is empty
               /* Delete answer from database */
               Svy_DB_RemoveAnswerQst (SvyQst.QstCod,NumAns);
           }
         else	// If this answer does not exist...
           {
            if (SvyQst.AnsChoice[NumAns].Text[0])	// Answer is not empty
               /* Create answer into database */
               Svy_DB_CreateAnswer (SvyQst.QstCod,NumAns,
                                    SvyQst.AnsChoice[NumAns].Text);
           }

      /***** List the questions of this survey, including the new one just inserted into the database *****/
      Ale_ShowAlert (Ale_SUCCESS,Txt_The_survey_has_been_modified);
     }

   /***** Free answers *****/
   Svy_FreeTextChoiceAnswers (&SvyQst,Svy_MAX_ANSWERS_PER_QUESTION);

   /***** Show current survey *****/
   Svy_ShowOneSurvey (&Surveys,SvyCod,true);
  }

/*****************************************************************************/
/******************* Get next question index in a survey *********************/
/*****************************************************************************/

static unsigned Svy_GetNextQuestionIndexInSvy (long SvyCod)
  {
   MYSQL_RES *mysql_res;
   MYSQL_ROW row;
   unsigned QstInd = 0;

   /***** Get last question index *****/
   if (Svy_DB_GetLastQstInd (&mysql_res,SvyCod))
     {
      row = mysql_fetch_row (mysql_res);
      if (row[0])	// There are questions
	{
	 if (sscanf (row[0],"%u",&QstInd) != 1)
	    Err_WrongQuestionIndexExit ();
	 QstInd++;
	}
     }

   /***** Free structure that stores the query result *****/
   DB_FreeMySQLResult (&mysql_res);

   return QstInd;
  }

/*****************************************************************************/
/************************ List the questions of a survey *********************/
/*****************************************************************************/

static void Svy_ListSvyQuestions (struct Svy_Surveys *Surveys,
                                  struct Svy_Survey *Svy)
  {
   extern const char *Hlp_ASSESSMENT_Surveys_questions;
   extern const char *Txt_Questions;
   extern const char *Txt_No_INDEX;
   extern const char *Txt_Type;
   extern const char *Txt_Question;
   extern const char *Txt_SURVEY_STR_ANSWER_TYPES[Svy_NUM_ANS_TYPES];
   extern const char *Txt_This_survey_has_no_questions;
   extern const char *Txt_Done;
   MYSQL_RES *mysql_res;
   MYSQL_ROW row;
   unsigned NumQsts;
   unsigned NumQst;
   struct Svy_Question SvyQst;
   char Stem[Cns_MAX_BYTES_TEXT + 1];
   bool Editing = (Gbl.Action.Act == ActEdiOneSvy    ||
	           Gbl.Action.Act == ActEdiOneSvyQst ||
	           Gbl.Action.Act == ActRcvSvyQst);
   bool PutFormAnswerSurvey = Svy->Status.ICanAnswer && !Editing;

   /***** Begin box *****/
   Surveys->SvyCod = Svy->SvyCod;
   if (Svy->Status.ICanEdit)
      Box_BoxBegin (NULL,Txt_Questions,
		    Svy_PutIconToAddNewQuestion,Surveys,
		    Hlp_ASSESSMENT_Surveys_questions,Box_NOT_CLOSABLE);
   else
      Box_BoxBegin (NULL,Txt_Questions,
		    NULL,NULL,
		    Hlp_ASSESSMENT_Surveys_questions,Box_NOT_CLOSABLE);

   /***** Get data of questions from database *****/
   if ((NumQsts = Svy_DB_GetSurveyQsts (&mysql_res,Svy->SvyCod)))
     {
      if (PutFormAnswerSurvey)
	{
	 /***** Begin form to send answers to survey *****/
	 Frm_BeginForm (ActAnsSvy);
	    Svy_PutParamSvyCod (Svy->SvyCod);
	}

      /***** Write the heading *****/
      HTM_TABLE_BeginWideMarginPadding (5);

	 HTM_TR_Begin (NULL);
	    if (Svy->Status.ICanEdit)
	       HTM_TH_Empty (1);
	    HTM_TH (Txt_No_INDEX,HTM_HEAD_CENTER);
	    HTM_TH (Txt_Type    ,HTM_HEAD_CENTER);
	    HTM_TH (Txt_Question,HTM_HEAD_LEFT  );
	 HTM_TR_End ();

	 /***** Write questions one by one *****/
	 for (NumQst = 0, The_ResetRowColor ();
	      NumQst < NumQsts;
	      NumQst++, The_ChangeRowColor ())
	   {
	    /* Initialize question to zero */
	    Svy_InitQst (&SvyQst);

	    /* Get question data from row */
	    row = mysql_fetch_row (mysql_res);
	    Svy_GetDataOfQstFromRow (&SvyQst,Stem,row);

	    HTM_TR_Begin (NULL);

	       if (Svy->Status.ICanEdit)
		 {
		  HTM_TD_Begin ("class=\"BT %s\"",The_GetColorRows ());

		     /* Initialize context */
		     Surveys->SvyCod = Svy->SvyCod;
		     Surveys->QstCod = SvyQst.QstCod;

		     /* Write icon to remove the question */
		     Ico_PutContextualIconToRemove (ActReqRemSvyQst,NULL,
						    Svy_PutParamsToEditQuestion,Surveys);

		     /* Write icon to edit the question */
		     Ico_PutContextualIconToEdit (ActEdiOneSvyQst,NULL,
						  Svy_PutParamsToEditQuestion,Surveys);

		  HTM_TD_End ();
		 }

	       /* Write index of question inside survey */
	       HTM_TD_Begin ("class=\"CT DAT_SMALL_%s %s\"",
	                     The_GetSuffix (),
	                     The_GetColorRows ());
		  HTM_Unsigned (SvyQst.QstInd + 1);
	       HTM_TD_End ();

	       /* Write the question type (row[2]) */
	       HTM_TD_Begin ("class=\"CT DAT_SMALL_%s %s\"",
	                     The_GetSuffix (),
	                     The_GetColorRows ());
		  HTM_Txt (Txt_SURVEY_STR_ANSWER_TYPES[SvyQst.AnswerType]);
	       HTM_TD_End ();

	       /* Write the stem and the answers of this question */
	       HTM_TD_Begin ("class=\"LT DAT_%s %s\"",
	                     The_GetSuffix (),
	                     The_GetColorRows ());
		  Svy_WriteQstStem (Stem);
		  Svy_WriteAnswersOfAQst (Svy,&SvyQst,PutFormAnswerSurvey);
	       HTM_TD_End ();

	    HTM_TR_End ();
	   }

      HTM_TABLE_End ();

      if (PutFormAnswerSurvey)
	{
	 /***** Button to create/modify survey *****/
	 Btn_PutConfirmButton (Txt_Done);

	 /***** End form *****/
	 Frm_EndForm ();
	}
     }
   else	// This survey has no questions
      Ale_ShowAlert (Ale_INFO,Txt_This_survey_has_no_questions);

   if (Svy->Status.ICanEdit &&	// I can edit
       (!NumQsts ||		// This survey has no questions
	Editing))		// I am editing
      /***** Put button to add a new question in this survey *****/
      Svy_PutButtonToCreateNewQuestion (Surveys);

   /***** Free structure that stores the query result *****/
   DB_FreeMySQLResult (&mysql_res);

   /***** End box *****/
   Box_BoxEnd ();
  }

/*****************************************************************************/
/************************* Get question data from row ************************/
/*****************************************************************************/

static void Svy_GetDataOfQstFromRow (struct Svy_Question *SvyQst,
                                     char Stem[Cns_MAX_BYTES_TEXT + 1],
                                     MYSQL_ROW row)
  {
   /***** Get the code of the question (row[0]) *****/
   if (sscanf (row[0],"%ld",&(SvyQst->QstCod)) != 1)
      Err_WrongQuestionExit ();

   /***** Get the index of the question inside the survey (row[1]) *****/
   SvyQst->QstInd = Str_ConvertStrToUnsigned (row[1]);

   /***** Get the answer type (row[2]) *****/
   SvyQst->AnswerType = Svy_DB_ConvertFromStrAnsTypDBToAnsTyp (row[2]);

   /***** Get the stem of the question from the database (row[3]) *****/
   Str_Copy (Stem,row[3],Cns_MAX_BYTES_TEXT);
  }

/*****************************************************************************/
/******************** Put parameters to edit a question **********************/
/*****************************************************************************/

static void Svy_PutParamsToEditQuestion (void *Surveys)
  {
   if (Surveys)
     {
      Svy_PutParamSvyCod (((struct Svy_Surveys *) Surveys)->SvyCod);
      Svy_PutParamQstCod (((struct Svy_Surveys *) Surveys)->QstCod);
     }
  }

/*****************************************************************************/
/***************** Put icon to add a new question to survey ******************/
/*****************************************************************************/

static void Svy_PutIconToAddNewQuestion (void *Surveys)
  {
   Ico_PutContextualIconToAdd (ActEdiOneSvyQst,NULL,Svy_PutParams,Surveys);
  }

/*****************************************************************************/
/**************** Put button to add a new question to survey *****************/
/*****************************************************************************/

static void Svy_PutButtonToCreateNewQuestion (struct Svy_Surveys *Surveys)
  {
   extern const char *Txt_New_question;

   Frm_BeginForm (ActEdiOneSvyQst);
      Svy_PutParams (Surveys);
      Btn_PutConfirmButton (Txt_New_question);
   Frm_EndForm ();
  }

/*****************************************************************************/
/****************** Write the heading of a survey question *******************/
/*****************************************************************************/

static void Svy_WriteQstStem (const char *Stem)
  {
   char *HeadingRigorousHTML;
   size_t Length;

   /* Convert the stem, that is in HTML, to rigorous HTML */
   Length = strlen (Stem) * Str_MAX_BYTES_PER_CHAR;
   if ((HeadingRigorousHTML = malloc (Length + 1)) == NULL)
      Err_NotEnoughMemoryExit ();
   Str_Copy (HeadingRigorousHTML,Stem,Length);
   Str_ChangeFormat (Str_FROM_HTML,Str_TO_RIGOROUS_HTML,
                     HeadingRigorousHTML,Length,false);

   /* Write the stem */
   HTM_Txt (HeadingRigorousHTML);

   /* Free memory allocated for the stem */
   free (HeadingRigorousHTML);
  }

/*****************************************************************************/
/************** Get and write the answers of a survey question ***************/
/*****************************************************************************/

static void Svy_WriteAnswersOfAQst (struct Svy_Survey *Svy,
                                    struct Svy_Question *SvyQst,
                                    bool PutFormAnswerSurvey)
  {
   unsigned NumAnswers;
   unsigned NumAns;
   MYSQL_RES *mysql_res;
   MYSQL_ROW row;
   unsigned NumUsrsThisAnswer;
   char StrAns[32];

   /***** Get answers of this question *****/
   NumAnswers = Svy_DB_GetAnswersQst (&mysql_res,SvyQst->QstCod);	// Result: AnsInd,NumUsrs,Answer

   /***** Write the answers *****/
   if (NumAnswers)
     {
      /* Check number of answers */
      if (NumAnswers > Svy_MAX_ANSWERS_PER_QUESTION)
         Err_WrongAnswerExit ();

      /* Write one row for each answer */
      HTM_TABLE_BeginPadding (5);

      for (NumAns = 0;
	   NumAns < NumAnswers;
	   NumAns++)
	{
	 row = mysql_fetch_row (mysql_res);

	 /* Get number of users who have marked this answer (row[1]) */
	 if (sscanf (row[1],"%u",&NumUsrsThisAnswer) != 1)
	    Err_ShowErrorAndExit ("Error when getting number of users who have marked an answer.");

	 /* Convert the answer (row[2]), that is in HTML, to rigorous HTML */
	 if (!Svy_AllocateTextChoiceAnswer (SvyQst,NumAns))
	    /* Abort on error */
	    Ale_ShowAlertsAndExit ();

	 Str_Copy (SvyQst->AnsChoice[NumAns].Text,row[2],Svy_MAX_BYTES_ANSWER);
	 Str_ChangeFormat (Str_FROM_HTML,Str_TO_RIGOROUS_HTML,
			   SvyQst->AnsChoice[NumAns].Text,Svy_MAX_BYTES_ANSWER,false);

	 /* Selectors and label with the letter of the answer */
	 HTM_TR_Begin (NULL);

	    if (PutFormAnswerSurvey)
	      {
	       /* Write selector to choice this answer */
	       HTM_TD_Begin ("class=\"LT\"");
		  snprintf (StrAns,sizeof (StrAns),"Ans%010u",
			    (unsigned) SvyQst->QstCod);
		  if (SvyQst->AnswerType == Svy_ANS_UNIQUE_CHOICE)
		     HTM_INPUT_RADIO (StrAns,false,
				      "id=\"Ans%010u_%010u\" value=\"%u\""
				      " onclick=\"selectUnselectRadio(this,this.form.Ans%010u,%u)\"",
				      (unsigned) SvyQst->QstCod,NumAns,NumAns,
				      NumAns,
				      (unsigned) SvyQst->QstCod,NumAnswers);
		  else // SvyQst->AnswerType == Svy_ANS_MULTIPLE_CHOICE
		     HTM_INPUT_CHECKBOX (StrAns,HTM_DONT_SUBMIT_ON_CHANGE,
					 "id=\"Ans%010u_%010u\" value=\"%u\"",
					 (unsigned) SvyQst->QstCod,NumAns,NumAns,
					 NumAns);
	       HTM_TD_End ();
	      }

	    /* Write the number of option */
	    HTM_TD_Begin ("class=\"SVY_OPT LT\"");
	       HTM_LABEL_Begin ("for=\"Ans%010u_%010u\" class=\"DAT_%s\"",
				(unsigned) SvyQst->QstCod,NumAns,
				The_GetSuffix ());
		  HTM_TxtF ("%u)",NumAns + 1);
	       HTM_LABEL_End ();
	    HTM_TD_End ();

	    /* Write the text of the answer */
	    HTM_TD_Begin ("class=\"LT\"");
	       HTM_LABEL_Begin ("for=\"Ans%010u_%010u\" class=\"DAT_%s\"",
				(unsigned) SvyQst->QstCod,NumAns,
				The_GetSuffix ());
		  HTM_Txt (SvyQst->AnsChoice[NumAns].Text);
	       HTM_LABEL_End ();
	    HTM_TD_End ();

	    /* Show stats of this answer */
	    if (Svy->Status.ICanViewResults)
	       Svy_DrawBarNumUsrs (NumUsrsThisAnswer,Svy->NumUsrs);

	 HTM_TR_End ();

	 /* Free memory allocated for the answer */
	 Svy_FreeTextChoiceAnswer (SvyQst,NumAns);
	}

      HTM_TABLE_End ();
     }

   /***** Free structure that stores the query result *****/
   DB_FreeMySQLResult (&mysql_res);
  }

/*****************************************************************************/
/***************** Draw a bar with the percentage of answers *****************/
/*****************************************************************************/

#define Svy_MAX_BAR_WIDTH 125

static void Svy_DrawBarNumUsrs (unsigned NumUsrs,unsigned MaxUsrs)
  {
   extern const char *Txt_of_PART_OF_A_TOTAL;
   unsigned BarWidth = 0;
   char *Title;

   /***** Build string with the number of users *****/
   if (MaxUsrs)
     {
      if (asprintf (&Title,"%u&nbsp;(%u%%&nbsp;%s&nbsp;%u)",
                    NumUsrs,
                    (unsigned) ((((double) NumUsrs * 100.0) / (double) MaxUsrs) + 0.5),
                    Txt_of_PART_OF_A_TOTAL,MaxUsrs) < 0)
         Err_NotEnoughMemoryExit ();
     }
   else
     {
      if (asprintf (&Title,"0&nbsp;(0%%&nbsp;%s&nbsp;%u)",
                    Txt_of_PART_OF_A_TOTAL,MaxUsrs) < 0)
         Err_NotEnoughMemoryExit ();
     }

   HTM_TD_Begin ("class=\"LT DAT_%s\" style=\"width:%upx;\"",
                 The_GetSuffix (),Svy_MAX_BAR_WIDTH + 125);

      /***** Draw bar with a with proportional to the number of clicks *****/
      if (NumUsrs && MaxUsrs)
	 BarWidth = (unsigned) ((((double) NumUsrs * (double) Svy_MAX_BAR_WIDTH) /
				  (double) MaxUsrs) + 0.5);
      if (BarWidth < 2)
	 BarWidth = 2;
      HTM_IMG (Cfg_URL_ICON_PUBLIC,"o1x1.png",Title,
	       "class=\"LT\" style=\"width:%upx; height:20px;\"",BarWidth);

      /***** Write the number of users *****/
      HTM_TxtF ("&nbsp;%s",Title);

      /***** Free string with the number of users *****/
      free (Title);

   HTM_TD_End ();
  }

/*****************************************************************************/
/********************* Put icon to remove one question ***********************/
/*****************************************************************************/

static void Svy_PutIconToRemoveOneQst (void *Surveys)
  {
   Ico_PutContextualIconToRemove (ActReqRemSvyQst,NULL,
				  Svy_PutParamsRemoveOneQst,Surveys);
  }

/*****************************************************************************/
/****************** Put parameter to remove one question *********************/
/*****************************************************************************/

static void Svy_PutParamsRemoveOneQst (void *Surveys)
  {
   if (Surveys)
     {
      Svy_PutParamSvyCod (((struct Svy_Surveys *) Surveys)->SvyCod);
      Svy_PutParamQstCod (((struct Svy_Surveys *) Surveys)->QstCod);
     }
  }

/*****************************************************************************/
/********************** Request the removal of a question ********************/
/*****************************************************************************/

void Svy_RequestRemoveQst (void)
  {
   extern const char *Txt_Do_you_really_want_to_remove_the_question_X;
   extern const char *Txt_Remove_question;
   struct Svy_Surveys Surveys;
   struct Svy_Question SvyQst;
   long SvyCod;

   /***** Reset surveys *****/
   Svy_ResetSurveys (&Surveys);

   /***** Initialize question to zero *****/
   Svy_InitQst (&SvyQst);

   /***** Get parameters from form *****/
   /* Get survey code */
   if ((SvyCod = Svy_GetParamSvyCod ()) <= 0)
      Err_WrongSurveyExit ();

   /* Get question code */
   if ((SvyQst.QstCod = Svy_GetParamQstCod ()) < 0)
      Err_WrongQuestionExit ();

   /* Get question index */
   SvyQst.QstInd = Svy_DB_GetQstIndFromQstCod (SvyQst.QstCod);

   /***** Show question and button to remove question *****/
   Surveys.SvyCod = SvyCod;
   Surveys.QstCod = SvyQst.QstCod;
   Ale_ShowAlertAndButton (ActRemSvyQst,NULL,NULL,
                           Svy_PutParamsRemoveOneQst,&Surveys,
			   Btn_REMOVE_BUTTON,Txt_Remove_question,
			   Ale_QUESTION,Txt_Do_you_really_want_to_remove_the_question_X,
	                   (long) SvyQst.QstInd + 1);

   /***** Show current survey *****/
   Svy_ShowOneSurvey (&Surveys,SvyCod,true);
  }

/*****************************************************************************/
/****************************** Remove a question ****************************/
/*****************************************************************************/

void Svy_RemoveQst (void)
  {
   extern const char *Txt_Question_removed;
   struct Svy_Surveys Surveys;
   struct Svy_Question SvyQst;
   long SvyCod;

   /***** Reset surveys *****/
   Svy_ResetSurveys (&Surveys);

   /***** Initialize question to zero *****/
   Svy_InitQst (&SvyQst);

   /***** Get parameters from form *****/
   /* Get survey code */
   if ((SvyCod = Svy_GetParamSvyCod ()) <= 0)
      Err_WrongSurveyExit ();

   /* Get question code */
   if ((SvyQst.QstCod = Svy_GetParamQstCod ()) <= 0)
      Err_WrongQuestionExit ();

   /* Get question index */
   SvyQst.QstInd = Svy_DB_GetQstIndFromQstCod (SvyQst.QstCod);

   /***** Remove the question from all tables *****/
   /* Remove answers from this test question */
   Svy_DB_RemoveAnswersQst (SvyQst.QstCod);

   /* Remove the question itself */
   Svy_DB_RemoveQst (SvyQst.QstCod);

   /* Change index of questions greater than this */
   Svy_DB_ChangeIndexesQsts (SvyCod,SvyQst.QstInd);

   /***** Write message *****/
   Ale_ShowAlert (Ale_SUCCESS,Txt_Question_removed);

   /***** Show current survey *****/
   Svy_ShowOneSurvey (&Surveys,SvyCod,true);
  }

/*****************************************************************************/
/************************ Receive answers of a survey ************************/
/*****************************************************************************/

void Svy_ReceiveSurveyAnswers (void)
  {
   extern const char *Txt_You_already_answered_this_survey_before;
   extern const char *Txt_Thanks_for_answering_the_survey;
   struct Svy_Surveys Surveys;
   struct Svy_Survey Svy;

   /***** Reset surveys *****/
   Svy_ResetSurveys (&Surveys);

   /***** Get survey code *****/
   if ((Svy.SvyCod = Svy_GetParamSvyCod ()) <= 0)
      Err_WrongSurveyExit ();

   /***** Get data of the survey from database *****/
   Svy_GetDataOfSurveyByCod (&Svy);

   /***** Check if I have no answered this survey formerly *****/
   if (Svy.Status.IHaveAnswered)
      Ale_ShowAlert (Ale_WARNING,Txt_You_already_answered_this_survey_before);
   else
     {
      /***** Receive and store user's answers *****/
      Svy_ReceiveAndStoreUserAnswersToASurvey (Svy.SvyCod);
      Ale_ShowAlert (Ale_SUCCESS,Txt_Thanks_for_answering_the_survey);
     }

   /***** Show current survey *****/
   Svy_ShowOneSurvey (&Surveys,Svy.SvyCod,true);
  }

/*****************************************************************************/
/**************** Get and store user's answers to a survey *******************/
/*****************************************************************************/

static void Svy_ReceiveAndStoreUserAnswersToASurvey (long SvyCod)
  {
   MYSQL_RES *mysql_res;
   unsigned NumQst;
   unsigned NumQsts;
   long QstCod;
   char ParamName[3 + 10 + 6 + 1];
   char StrAnswersIndexes[Svy_MAX_ANSWERS_PER_QUESTION * (Cns_MAX_DECIMAL_DIGITS_UINT + 1)];
   const char *Ptr;
   char UnsignedStr[Cns_MAX_DECIMAL_DIGITS_UINT + 1];
   unsigned AnsInd;

   /***** Get questions of this survey from database *****/
   if ((NumQsts = Svy_DB_GetSurveyQstsCodes (&mysql_res,SvyCod)))
     {
      // This survey has questions
      /***** Get questions *****/
      for (NumQst = 0;
	   NumQst < NumQsts;
	   NumQst++)
        {
         /* Get next question */
         if ((QstCod = DB_GetNextCode (mysql_res)) <= 0)
            Err_WrongQuestionExit ();

         /* Get possible parameter with the user's answer */
         snprintf (ParamName,sizeof (ParamName),"Ans%010u",(unsigned) QstCod);
         // Lay_ShowAlert (Lay_INFO,ParamName);
         Par_GetParMultiToText (ParamName,StrAnswersIndexes,
                                Svy_MAX_ANSWERS_PER_QUESTION * (Cns_MAX_DECIMAL_DIGITS_UINT + 1));
         Ptr = StrAnswersIndexes;
         while (*Ptr)
           {
            Par_GetNextStrUntilSeparParamMult (&Ptr,UnsignedStr,Cns_MAX_DECIMAL_DIGITS_UINT);
            if (sscanf (UnsignedStr,"%u",&AnsInd) == 1)
               // Parameter exists ==> user has checked this answer
               // 		   ==> store it in database
               Svy_DB_IncreaseAnswer (QstCod,AnsInd);
           }
        }
     }
   else		// This survey has no questions
      Err_ShowErrorAndExit ("Error: this survey has no questions.");

   /***** Free structure that stores the query result *****/
   DB_FreeMySQLResult (&mysql_res);

   /***** Register that you have answered this survey *****/
   Svy_DB_RegisterIHaveAnsweredSvy (SvyCod);
  }

/*****************************************************************************/
/******************** Get number of surveys for courses **********************/
/*****************************************************************************/
// Returns the number of surveys for courses
// in this location (all the platform, current degree or current course)

unsigned Svy_GetNumCrsSurveys (HieLvl_Level_t Scope,unsigned *NumNotif)
  {
   MYSQL_RES *mysql_res;
   MYSQL_ROW row;
   unsigned NumSurveys;

   /***** Get number of surveys from database *****/
   if (Svy_DB_GetNumCrsSurveys (&mysql_res,Scope))
     {
      /***** Get number of surveys *****/
      row = mysql_fetch_row (mysql_res);
      if (sscanf (row[0],"%u",&NumSurveys) != 1)
	 Err_ShowErrorAndExit ("Error when getting number of surveys.");

      /***** Get number of notifications by email *****/
      if (row[1])
	{
	 if (sscanf (row[1],"%u",NumNotif) != 1)
	    Err_ShowErrorAndExit ("Error when getting number of notifications of surveys.");
	}
      else
	 *NumNotif = 0;
     }
   else
      NumSurveys = 0;

   /***** Free structure that stores the query result *****/
   DB_FreeMySQLResult (&mysql_res);

   return NumSurveys;
  }

/*****************************************************************************/
/***************************** Show stats of surveys *************************/
/*****************************************************************************/

void Svy_GetAndShowSurveysStats (void)
  {
   extern const char *Hlp_ANALYTICS_Figures_surveys;
   extern const char *Txt_FIGURE_TYPES[Fig_NUM_FIGURES];
   extern const char *Txt_Number_of_BR_surveys;
   extern const char *Txt_Number_of_BR_courses_with_BR_surveys;
   extern const char *Txt_Average_number_BR_of_surveys_BR_per_course;
   extern const char *Txt_Average_number_BR_of_questions_BR_per_survey;
   extern const char *Txt_Number_of_BR_notifications;
   unsigned NumSurveys;
   unsigned NumNotif;
   unsigned NumCoursesWithSurveys = 0;
   double NumSurveysPerCourse = 0.0;
   double NumQstsPerSurvey = 0.0;

   /***** Get the number of surveys and the average number of questions per survey from this location *****/
   if ((NumSurveys = Svy_GetNumCrsSurveys (Gbl.Scope.Current,&NumNotif)))
     {
      if ((NumCoursesWithSurveys = Svy_DB_GetNumCrssWithCrsSurveys (Gbl.Scope.Current)) != 0)
         NumSurveysPerCourse = (double) NumSurveys /
	                       (double) NumCoursesWithSurveys;
      NumQstsPerSurvey = Svy_DB_GetNumQstsPerCrsSurvey (Gbl.Scope.Current);
     }

   /***** Begin box and table *****/
   Box_BoxTableBegin (NULL,Txt_FIGURE_TYPES[Fig_SURVEYS],
                      NULL,NULL,
                      Hlp_ANALYTICS_Figures_surveys,Box_NOT_CLOSABLE,2);

      /***** Write table heading *****/
      HTM_TR_Begin (NULL);
	 HTM_TH (Txt_Number_of_BR_surveys                        ,HTM_HEAD_RIGHT);
	 HTM_TH (Txt_Number_of_BR_courses_with_BR_surveys        ,HTM_HEAD_RIGHT);
	 HTM_TH (Txt_Average_number_BR_of_surveys_BR_per_course  ,HTM_HEAD_RIGHT);
	 HTM_TH (Txt_Average_number_BR_of_questions_BR_per_survey,HTM_HEAD_RIGHT);
	 HTM_TH (Txt_Number_of_BR_notifications                  ,HTM_HEAD_RIGHT);
      HTM_TR_End ();

      /***** Write number of surveys *****/
      HTM_TR_Begin (NULL);

	 HTM_TD_Begin ("class=\"RM DAT_%s\"",The_GetSuffix ());
	    HTM_Unsigned (NumSurveys);
	 HTM_TD_End ();

	 HTM_TD_Begin ("class=\"RM DAT_%s\"",The_GetSuffix ());
	    HTM_Unsigned (NumCoursesWithSurveys);
	 HTM_TD_End ();

	 HTM_TD_Begin ("class=\"RM DAT_%s\"",The_GetSuffix ());
	    HTM_Double2Decimals (NumSurveysPerCourse);
	 HTM_TD_End ();

	 HTM_TD_Begin ("class=\"RM DAT_%s\"",The_GetSuffix ());
	    HTM_Double2Decimals (NumQstsPerSurvey);
	 HTM_TD_End ();

	 HTM_TD_Begin ("class=\"RM DAT_%s\"",The_GetSuffix ());
	    HTM_Unsigned (NumNotif);
	 HTM_TD_End ();

      HTM_TR_End ();

   /***** End table and box *****/
   Box_BoxTableEnd ();
  }
