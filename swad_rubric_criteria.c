// swad_rubric_criteria.c: criteria in assessment rubrics
/*
    SWAD (Shared Workspace At a Distance),
    is a web platform developed at the University of Granada (Spain),
    and used to support university teaching.

    This file is part of SWAD core.
    Copyright (C) 1999-2023 Antonio Ca�as Vargas

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

#include <float.h>		// For DBL_MAX
#include <mysql/mysql.h>	// To access MySQL databases
#include <string.h>		// For string functions

#include "swad_action_list.h"
#include "swad_alert.h"
#include "swad_assignment_resource.h"
#include "swad_box.h"
#include "swad_database.h"
#include "swad_error.h"
#include "swad_exam_resource.h"
#include "swad_form.h"
#include "swad_game_resource.h"
#include "swad_global.h"
#include "swad_parameter.h"
#include "swad_parameter_code.h"
#include "swad_resource_database.h"
#include "swad_rubric.h"
#include "swad_rubric_criteria.h"
#include "swad_rubric_database.h"

/*****************************************************************************/
/************** External global variables from others modules ****************/
/*****************************************************************************/

extern struct Globals Gbl;

/*****************************************************************************/
/***************************** Private constants *****************************/
/*****************************************************************************/

// Form parameters for minimum/maximum criterion values
static const char *RubCri_ParValues[RubCri_NUM_VALUES] =
  {
   [RubCri_MIN] = "MinVal",
   [RubCri_MAX] = "MaxVal",
  };

#define RubCri_WEIGHT_MIN	0.0
#define RubCri_WEIGHT_MAX	1.0
#define RubCri_WEIGHT_STEP	0.000001

/*****************************************************************************/
/***************************** Private prototypes ****************************/
/*****************************************************************************/

static void RubCri_PutParsOneCriterion (void *Rubrics);

static void RubCri_PutFormNewCriterion (struct Rub_Rubrics *Rubrics,
				        struct RubCri_Criterion *Criterion,
				        unsigned MaxCriInd);
static void RubCri_ReceiveCriterionFieldsFromForm (struct RubCri_Criterion *Criterion);
static bool RubCri_CheckCriterionTitleReceivedFromForm (const struct RubCri_Criterion *Criterion,
                                                        const char NewTitle[RubCri_MAX_BYTES_TITLE + 1]);

static void RubCri_ChangeValueCriterion (RubCri_ValueRange_t ValueRange);

static void RubCri_CreateCriterion (struct RubCri_Criterion *Criterion);

static void RubCri_ListOneOrMoreCriteriaForEdition (struct Rub_Rubrics *Rubrics,
					            unsigned MaxCriInd,
					            unsigned NumCriteria,
                                                    MYSQL_RES *mysql_res,
                                                    bool ICanEditCriteria);

static void RubCri_GetCriterionDataFromRow (MYSQL_RES *mysql_res,
                                            struct RubCri_Criterion *Criterion);

static void RubCri_PutTableHeadingForCriteria (void);

static void RubCri_GetAndCheckPars (struct Rub_Rubrics *Rubrics,
                                    struct RubCri_Criterion *Criterion);

static void RubCri_ExchangeCriteria (long RubCod,
                                     unsigned CriIndTop,unsigned CriIndBottom);

/*****************************************************************************/
/*************** Put parameter to edit one rubric criterion ******************/
/*****************************************************************************/

static void RubCri_PutParsOneCriterion (void *Rubrics)
  {
   if (Rubrics)
     {
      Rub_PutPars (Rubrics);
      ParCod_PutPar (ParCod_Cri,((struct Rub_Rubrics *) Rubrics)->CriCod);
     }
  }

/*****************************************************************************/
/******************** Get criterion data using its code **********************/
/*****************************************************************************/

void RubCri_GetCriterionDataByCod (struct RubCri_Criterion *Criterion)
  {
   MYSQL_RES *mysql_res;

   /***** Trivial check *****/
   if (Criterion->CriCod <= 0)
     {
      /* Initialize to empty criterion */
      RubCri_ResetCriterion (Criterion);
      return;
     }

   /***** Get data of rubric criterion from database *****/
   if (Rub_DB_GetCriterionDataByCod (&mysql_res,Criterion->CriCod)) // Criterion found...
      RubCri_GetCriterionDataFromRow (mysql_res,Criterion);
   else
      /* Initialize to empty criterion */
      RubCri_ResetCriterion (Criterion);

   /* Free structure that stores the query result */
   DB_FreeMySQLResult (&mysql_res);
  }

/*****************************************************************************/
/*************** Put a form to create/edit a rubric criterion ****************/
/*****************************************************************************/

static void RubCri_PutFormNewCriterion (struct Rub_Rubrics *Rubrics,
				        struct RubCri_Criterion *Criterion,
				        unsigned MaxCriInd)
  {
   extern const char *Txt_New_criterion;
   extern const char *Txt_Create_criterion;
   RubCri_ValueRange_t ValueRange;

   /***** Begin form *****/
   Frm_BeginForm (ActNewRubCri);
      Rub_PutPars (Rubrics);

      /***** Begin box and table *****/
      Box_BoxTableBegin (NULL,Txt_New_criterion,
			 NULL,NULL,
			 NULL,Box_NOT_CLOSABLE,2);

	 /***** Table heading *****/
	 RubCri_PutTableHeadingForCriteria ();

	 /***** Begin row *****/
	 HTM_TR_Begin (NULL);

	    /***** Empty column for buttons *****/
	    HTM_TD_Begin ("class=\"BT\"");
	    HTM_TD_End ();

	    /***** Index *****/
	    HTM_TD_Begin ("class=\"RT\"");
	       Lay_WriteIndex (MaxCriInd + 1,"BIG_INDEX");
	    HTM_TD_End ();

	    /***** Title *****/
	    HTM_TD_Begin ("class=\"LT\"");
	       HTM_INPUT_TEXT ("Title",RubCri_MAX_CHARS_TITLE,Criterion->Title,
			       HTM_DONT_SUBMIT_ON_CHANGE,
			       "id=\"Title\""
			       " class=\"TITLE_DESCRIPTION_WIDTH INPUT_%s\""
			       " required=\"required\"",
			       The_GetSuffix ());
	    HTM_TD_End ();

	    /***** Link *****/
	    HTM_TD_Begin ("class=\"LT\"");
	       Rsc_ShowClipboardToChangeLink (NULL);
	    HTM_TD_End ();

	    /***** Minimum and maximum values of the criterion *****/
	    for (ValueRange  = (RubCri_ValueRange_t) 0;
		 ValueRange <= (RubCri_ValueRange_t) (RubCri_NUM_VALUES - 1);
		 ValueRange++)
	      {
	       HTM_TD_Begin ("class=\"RM\"");
		  HTM_INPUT_FLOAT (RubCri_ParValues[ValueRange],0.0,DBL_MAX,0.1,
		                   Criterion->Values[ValueRange],false,
				   " class=\"INPUT_FLOAT INPUT_%s\" required=\"required\"",
				   The_GetSuffix ());
	       HTM_TD_End ();
	      }

	    /***** Weight *****/
	    HTM_TD_Begin ("class=\"RM\"");
	       HTM_INPUT_FLOAT ("Weight",
	                        RubCri_WEIGHT_MIN,
	                        RubCri_WEIGHT_MAX,
	                        RubCri_WEIGHT_STEP,
				Criterion->Weight,false,
				" class=\"INPUT_FLOAT INPUT_%s\" required=\"required\"",
				The_GetSuffix ());
	    HTM_TD_End ();

	 /***** End row *****/
	 HTM_TR_End ();

      /***** End table, send button and end box *****/
      Box_BoxTableWithButtonEnd (Btn_CREATE_BUTTON,Txt_Create_criterion);

   /***** End form *****/
   Frm_EndForm ();
  }

/*****************************************************************************/
/**************** Receive form to create a new rubric criterion **************/
/*****************************************************************************/

void RubCri_ReceiveFormCriterion (void)
  {
   struct Rub_Rubrics Rubrics;
   struct RubCri_Criterion Criterion;

   /***** Reset rubrics context *****/
   Rub_ResetRubrics (&Rubrics);
   Rub_ResetRubric (&Rubrics.Rubric);
   RubCri_ResetCriterion (&Criterion);

   /***** Get parameters *****/
   Rub_GetPars (&Rubrics,true);
   Criterion.RubCod = Rubrics.Rubric.RubCod;

   /***** Get rubric data from database *****/
   Rub_GetRubricDataByCod (&Rubrics.Rubric);

   /***** Check if rubric is editable *****/
   if (!Rub_CheckIfEditable (&Rubrics.Rubric))
      Err_NoPermissionExit ();

   /***** If I can edit rubrics ==> receive criterion from form *****/
   RubCri_ReceiveCriterionFieldsFromForm (&Criterion);

   if (RubCri_CheckCriterionTitleReceivedFromForm (&Criterion,Criterion.Title))
      RubCri_CreateCriterion (&Criterion);	// Add new criterion to database

   /***** Show current rubric and its criteria *****/
   Rub_PutFormsOneRubric (&Rubrics,&Criterion,
                          Rub_EXISTING_RUBRIC);	// It's not a new rubric
  }

static void RubCri_ReceiveCriterionFieldsFromForm (struct RubCri_Criterion *Criterion)
  {
   RubCri_ValueRange_t ValueRange;
   char ValueStr[64];
   char WeightStr[64];

   /***** Get criterion title *****/
   Par_GetParText ("Title",Criterion->Title,RubCri_MAX_BYTES_TITLE);

   /***** Get criterion link to resource *****/
   if (Rsc_GetParLink (&Criterion->Link))
      /***** Remove link from clipboard *****/
      Rsc_DB_RemoveLinkFromClipboard (&Criterion->Link);

   /***** Get minimum and maximum values of criterion *****/
   for (ValueRange  = (RubCri_ValueRange_t) 0;
	ValueRange <= (RubCri_ValueRange_t) (RubCri_NUM_VALUES - 1);
	ValueRange++)
     {
      Par_GetParText (RubCri_ParValues[ValueRange],ValueStr,sizeof (ValueStr) - 1);
      Criterion->Values[ValueRange] = Str_GetDoubleFromStr (ValueStr);
     }

   /***** Get criterion weight *****/
   Par_GetParText ("Weight",WeightStr,sizeof (WeightStr) - 1);
   Criterion->Weight = Str_GetDoubleFromStr (WeightStr);
  }

static bool RubCri_CheckCriterionTitleReceivedFromForm (const struct RubCri_Criterion *Criterion,
                                                        const char NewTitle[RubCri_MAX_BYTES_TITLE + 1])
  {
   extern const char *Txt_Already_existed_a_criterion_in_this_rubric_with_the_title_X;
   bool NewTitleIsCorrect;

   /***** Check if title is correct *****/
   NewTitleIsCorrect = true;
   if (NewTitle[0])	// If there's an criterion title
     {
      /***** Check if old and new titles are the same
	     (this happens when return is pressed without changes) *****/
      if (strcmp (Criterion->Title,NewTitle))	// Different titles
	{
	 /* If title of criterion was in database... */
	 if (Rub_DB_CheckIfSimilarCriterionExists (Criterion,NewTitle))
	   {
	    NewTitleIsCorrect = false;
	    Ale_ShowAlert (Ale_WARNING,Txt_Already_existed_a_criterion_in_this_rubric_with_the_title_X,
			   Criterion->Title);
	   }
	}
     }
   else	// If there is not a criterion title
     {
      NewTitleIsCorrect = false;
      Ale_ShowAlertYouMustSpecifyTheTitle ();
     }

   return NewTitleIsCorrect;
  }

/*****************************************************************************/
/************* Receive form to change title of rubric criterion **************/
/*****************************************************************************/

void RubCri_ChangeTitle (void)
  {
   struct Rub_Rubrics Rubrics;
   struct RubCri_Criterion Criterion;
   char NewTitle[RubCri_MAX_BYTES_TITLE + 1];

   /***** Check if I can edit rubrics *****/
   if (!Rub_CheckIfICanEditRubrics ())
      Err_NoPermissionExit ();

   /***** Reset rubrics context *****/
   Rub_ResetRubrics (&Rubrics);
   Rub_ResetRubric (&Rubrics.Rubric);
   RubCri_ResetCriterion (&Criterion);

   /***** Get and check parameters *****/
   RubCri_GetAndCheckPars (&Rubrics,&Criterion);

   /***** Check if rubric is editable *****/
   if (!Rub_CheckIfEditable (&Rubrics.Rubric))
      Err_NoPermissionExit ();

   /***** Receive new title from form *****/
   Par_GetParText ("Title",NewTitle,RubCri_MAX_BYTES_TITLE);

   /***** Check if title should be changed *****/
   if (RubCri_CheckCriterionTitleReceivedFromForm (&Criterion,NewTitle))
     {
      /* Update title and database table */
      Str_Copy (Criterion.Title,NewTitle,sizeof (Criterion.Title) - 1);
      Rub_DB_UpdateCriterionTitle (&Criterion);
     }

   /***** Show current rubric and its criteria *****/
   Rub_PutFormsOneRubric (&Rubrics,&Criterion,
                          Rub_EXISTING_RUBRIC);	// It's not a new rubric
  }

/*****************************************************************************/
/********* Receive form to change minimum/maximum value of criterion *********/
/*****************************************************************************/

void RubCri_ChangeMinValue (void)
  {
   RubCri_ChangeValueCriterion (RubCri_MIN);
  }

void RubCri_ChangeMaxValue (void)
  {
   RubCri_ChangeValueCriterion (RubCri_MAX);
  }

static void RubCri_ChangeValueCriterion (RubCri_ValueRange_t ValueRange)
  {
   struct Rub_Rubrics Rubrics;
   struct RubCri_Criterion Criterion;
   char ValueStr[64];

   /***** Check if I can edit rubrics *****/
   if (!Rub_CheckIfICanEditRubrics ())
      Err_NoPermissionExit ();

   /***** Reset rubrics context *****/
   Rub_ResetRubrics (&Rubrics);
   Rub_ResetRubric (&Rubrics.Rubric);
   RubCri_ResetCriterion (&Criterion);

   /***** Get parameters *****/
   Rub_GetPars (&Rubrics,true);
   Criterion.RubCod = Rubrics.Rubric.RubCod;
   Rubrics.CriCod = Criterion.CriCod = ParCod_GetAndCheckPar (ParCod_Cri);

   /***** Get and check parameters *****/
   RubCri_GetAndCheckPars (&Rubrics,&Criterion);

   /***** Check if rubric is editable *****/
   if (!Rub_CheckIfEditable (&Rubrics.Rubric))
      Err_NoPermissionExit ();

   /***** Receive new value from form *****/
   Par_GetParText (RubCri_ParValues[ValueRange],ValueStr,sizeof (ValueStr) - 1);
   Criterion.Values[ValueRange] = Str_GetDoubleFromStr (ValueStr);

   /***** Change value *****/
   /* Update the table changing old value by new value */
   Rub_DB_UpdateCriterionValue (&Criterion,ValueRange);

   /***** Show current rubric and its criteria *****/
   Rub_PutFormsOneRubric (&Rubrics,&Criterion,
                          Rub_EXISTING_RUBRIC);	// It's not a new rubric
  }

/*****************************************************************************/
/****************** Receive form to change criterion weight ******************/
/*****************************************************************************/

void RubCri_ChangeWeight (void)
  {
   struct Rub_Rubrics Rubrics;
   struct RubCri_Criterion Criterion;
   char WeightStr[64];

   /***** Check if I can edit rubrics *****/
   if (!Rub_CheckIfICanEditRubrics ())
      Err_NoPermissionExit ();

   /***** Reset rubrics context *****/
   Rub_ResetRubrics (&Rubrics);
   Rub_ResetRubric (&Rubrics.Rubric);
   RubCri_ResetCriterion (&Criterion);

   /***** Get parameters *****/
   Rub_GetPars (&Rubrics,true);
   Criterion.RubCod = Rubrics.Rubric.RubCod;
   Rubrics.CriCod = Criterion.CriCod = ParCod_GetAndCheckPar (ParCod_Cri);

   /***** Get and check parameters *****/
   RubCri_GetAndCheckPars (&Rubrics,&Criterion);

   /***** Check if rubric is editable *****/
   if (!Rub_CheckIfEditable (&Rubrics.Rubric))
      Err_NoPermissionExit ();

   /***** Receive new weight from form *****/
   Par_GetParText ("Weight",WeightStr,sizeof (WeightStr) - 1);
   Criterion.Weight = Str_GetDoubleFromStr (WeightStr);

   /***** Change value *****/
   /* Update the table changing old weight by new weight */
   Rub_DB_UpdateCriterionWeight (&Criterion);

   /***** Show current rubric and its criteria *****/
   Rub_PutFormsOneRubric (&Rubrics,&Criterion,
                          Rub_EXISTING_RUBRIC);	// It's not a new rubric
  }

/*****************************************************************************/
/************************ Create a new rubric criterion **********************/
/*****************************************************************************/

static void RubCri_CreateCriterion (struct RubCri_Criterion *Criterion)
  {
   extern const char *Txt_Created_new_criterion_X;
   unsigned MaxCriInd;

   /***** Get maximum criterion index *****/
   MaxCriInd = Rub_DB_GetMaxCriterionIndexInRubric (Criterion->RubCod);

   /***** Create a new criterion *****/
   Criterion->CriInd = MaxCriInd + 1;
   Criterion->CriCod = Rub_DB_CreateCriterion (Criterion);

   /***** Write success message *****/
   Ale_ShowAlert (Ale_SUCCESS,Txt_Created_new_criterion_X,
                  Criterion->Title);
  }

/*****************************************************************************/
/********************** List the criteria of a rubric ************************/
/*****************************************************************************/

void RubCri_ListCriteria (struct Rub_Rubrics *Rubrics,
			  struct RubCri_Criterion *Criterion)
  {
   extern const char *Hlp_ASSESSMENT_Rubrics_criteria;
   extern const char *Txt_Criteria;
   MYSQL_RES *mysql_res;
   unsigned MaxCriInd;
   unsigned NumCriteria;
   bool ICanEditCriteria = Rub_CheckIfEditable (&Rubrics->Rubric);

   /***** Get maximum criterion index *****/
   MaxCriInd = Rub_DB_GetMaxCriterionIndexInRubric (Rubrics->Rubric.RubCod);

   /***** Get data of rubric criteria from database *****/
   NumCriteria = Rub_DB_GetCriteria (&mysql_res,Rubrics->Rubric.RubCod);

   /***** Begin box *****/
   Box_BoxBegin (NULL,Txt_Criteria,
		 NULL,NULL,
		 Hlp_ASSESSMENT_Rubrics_criteria,Box_NOT_CLOSABLE);

      /***** Show table with rubric criteria *****/
      if (NumCriteria)
	 RubCri_ListOneOrMoreCriteriaForEdition (Rubrics,
						 MaxCriInd,
						 NumCriteria,mysql_res,
						 ICanEditCriteria);

      /***** Free structure that stores the query result *****/
      DB_FreeMySQLResult (&mysql_res);

      /***** Put forms to create/edit a criterion *****/
      if (ICanEditCriteria)
	 RubCri_PutFormNewCriterion (Rubrics,Criterion,MaxCriInd);

   /***** End box *****/
   Box_BoxEnd ();
  }

/*****************************************************************************/
/********************** List rubric criteria for edition *********************/
/*****************************************************************************/

static void RubCri_ListOneOrMoreCriteriaForEdition (struct Rub_Rubrics *Rubrics,
					            unsigned MaxCriInd,
					            unsigned NumCriteria,
                                                    MYSQL_RES *mysql_res,
                                                    bool ICanEditCriteria)
  {
   extern const char *Txt_Criteria;
   extern const char *Txt_Movement_not_allowed;
   // Actions to change minimum/maximum criterion values
   static Act_Action_t RubCri_ActionsValues[RubCri_NUM_VALUES] =
     {
      [RubCri_MIN] = ActChgMinRubCri,
      [RubCri_MAX] = ActChgMaxRubCri,
     };
   unsigned NumCriterion;
   struct RubCri_Criterion Criterion;
   char *Anchor;
   RubCri_ValueRange_t ValueRange;

   /***** Trivial check *****/
   if (!NumCriteria)
      return;

   /***** Begin table *****/
   HTM_TABLE_BeginWideMarginPadding (5);

      /***** Write the heading *****/
      RubCri_PutTableHeadingForCriteria ();

      /***** Write rows *****/
      for (NumCriterion = 0, The_ResetRowColor ();
	   NumCriterion < NumCriteria;
	   NumCriterion++, The_ChangeRowColor ())
	{
	 /***** Create criterion of questions *****/
	 RubCri_ResetCriterion (&Criterion);

	 /***** Get criterion data *****/
	 RubCri_GetCriterionDataFromRow (mysql_res,&Criterion);

	 /* Initialize context */
	 Rubrics->CriCod = Criterion.CriCod;
	 Rubrics->CriInd = Criterion.CriInd;

	 /***** Build anchor string *****/
	 Frm_SetAnchorStr (Criterion.CriCod,&Anchor);

	 /***** Begin first row *****/
	 HTM_TR_Begin (NULL);

	    /***** Icons *****/
	    HTM_TD_Begin ("rowspan=\"2\" class=\"BT %s\"",
	                  The_GetColorRows ());

	       /* Put icon to remove the criterion */
	       if (ICanEditCriteria)
		  Ico_PutContextualIconToRemove (ActReqRemRubCri,NULL,
						 RubCri_PutParsOneCriterion,Rubrics);
	       else
		  Ico_PutIconRemovalNotAllowed ();

	       /* Put icon to move up the question */
	       if (ICanEditCriteria && Criterion.CriInd > 1)
		  Lay_PutContextualLinkOnlyIcon (ActUp_RubCri,Anchor,
						 RubCri_PutParsOneCriterion,Rubrics,
						 "arrow-up.svg",Ico_BLACK);
	       else
		  Ico_PutIconOff ("arrow-up.svg",Ico_BLACK,
		                  Txt_Movement_not_allowed);

	       /* Put icon to move down the criterion */
	       if (ICanEditCriteria && Criterion.CriInd < MaxCriInd)
		  Lay_PutContextualLinkOnlyIcon (ActDwnRubCri,Anchor,
						 RubCri_PutParsOneCriterion,Rubrics,
						 "arrow-down.svg",Ico_BLACK);
	       else
		  Ico_PutIconOff ("arrow-down.svg",Ico_BLACK,
		                  Txt_Movement_not_allowed);

	    HTM_TD_End ();

	    /***** Index *****/
	    HTM_TD_Begin ("rowspan=\"2\" class=\"RT %s\"",
	                  The_GetColorRows ());
	       Lay_WriteIndex (Criterion.CriInd,"BIG_INDEX");
	    HTM_TD_End ();

	    /***** Title *****/
	    HTM_TD_Begin ("class=\"LT %s\"",The_GetColorRows ());
	       HTM_ARTICLE_Begin (Anchor);
		  if (ICanEditCriteria)
		    {
		     Frm_BeginFormAnchor (ActChgTitRubCri,Anchor);
			RubCri_PutParsOneCriterion (Rubrics);
			HTM_INPUT_TEXT ("Title",RubCri_MAX_CHARS_TITLE,Criterion.Title,
					HTM_SUBMIT_ON_CHANGE,
					"id=\"Title\""
					" class=\"TITLE_DESCRIPTION_WIDTH INPUT_%s\""
					" required=\"required\"",
					The_GetSuffix ());
		     Frm_EndForm ();
		    }
		  else
		    {
		     HTM_SPAN_Begin ("class=\"EXA_SET_TITLE\"");
			HTM_Txt (Criterion.Title);
		     HTM_SPAN_End ();
		    }
	       HTM_ARTICLE_End ();
	    HTM_TD_End ();

	    /***** Link to resource *****/
	    HTM_TD_Begin ("class=\"LT %s\"",The_GetColorRows ());
	       Frm_BeginFormAnchor (ActChgLnkRubCri,Anchor);
		  RubCri_PutParsOneCriterion (Rubrics);
		  Rsc_ShowClipboardToChangeLink (&Criterion.Link);
	       Frm_EndForm ();
	    HTM_TD_End ();

	    /***** Minimum and maximum values of criterion *****/
	    for (ValueRange  = (RubCri_ValueRange_t) 0;
		 ValueRange <= (RubCri_ValueRange_t) (RubCri_NUM_VALUES - 1);
		 ValueRange++)
	      {
	       HTM_TD_Begin ("class=\"RT %s\"",The_GetColorRows ());
		  if (ICanEditCriteria)
		    {
		     Frm_BeginFormAnchor (RubCri_ActionsValues[ValueRange],Anchor);
			RubCri_PutParsOneCriterion (Rubrics);
			HTM_INPUT_FLOAT (RubCri_ParValues[ValueRange],0.0,DBL_MAX,0.1,
			                 Criterion.Values[ValueRange],false,
					 " class=\"INPUT_FLOAT INPUT_%s\""
					 " required=\"required\"",
					 The_GetSuffix ());
		     Frm_EndForm ();
		    }
		  else
		    {
		     HTM_SPAN_Begin ("class=\"CRI_VALUE\"");
			HTM_Unsigned (Criterion.Values[ValueRange]);
		     HTM_SPAN_End ();
		    }
	       HTM_TD_End ();
	      }

	    /***** Criterion weight *****/
	    HTM_TD_Begin ("class=\"RT %s\"",The_GetColorRows ());
	       if (ICanEditCriteria)
		 {
		  Frm_BeginFormAnchor (ActChgWeiRubCri,Anchor);
		     RubCri_PutParsOneCriterion (Rubrics);
		     HTM_INPUT_FLOAT ("Weight",
		                      RubCri_WEIGHT_MIN,
		                      RubCri_WEIGHT_MAX,
		                      RubCri_WEIGHT_STEP,
				      Criterion.Weight,false,
				      " class=\"INPUT_FLOAT INPUT_%s\""
				      " required=\"required\"",
				      The_GetSuffix ());
		  Frm_EndForm ();
		 }
	       else
		 {
		  HTM_SPAN_Begin ("class=\"CRI_VALUE\"");
		     HTM_Unsigned (Criterion.Weight);
		  HTM_SPAN_End ();
		 }
	    HTM_TD_End ();

	 /***** End first row *****/
	 HTM_TR_End ();

	 /***** Begin second row *****/
	 HTM_TR_Begin (NULL);

	    /***** Questions *****/
	    HTM_TD_Begin ("colspan=\"5\" class=\"LT %s\"",
	                  The_GetColorRows ());

	       // Description here

	    HTM_TD_End ();

	 /***** End second row *****/
	 HTM_TR_End ();

	 /***** Free anchor string *****/
	 Frm_FreeAnchorStr (Anchor);
	}

   /***** End table *****/
   HTM_TABLE_End ();
  }

/*****************************************************************************/
/************************** Get rubric criteria data *************************/
/*****************************************************************************/

static void RubCri_GetCriterionDataFromRow (MYSQL_RES *mysql_res,
                                            struct RubCri_Criterion *Criterion)
  {
   MYSQL_ROW row;
   RubCri_ValueRange_t ValueRange;

   /***** Get next row from result *****/
   row = mysql_fetch_row (mysql_res);
   /*
   row[0] CriCod
   row[1] RubCod
   row[2] CriInd
   row[3] Source
   row[4] Cod
   row[5] MinVal
   row[6] MaxVal
   row[7] Weight
   row[8] Title
   */
   /***** Get criterion code (row[0]) and rubric code (row[1]) *****/
   if ((Criterion->CriCod = Str_ConvertStrCodToLongCod (row[0])) <= 0)
      Err_WrongCriterionExit ();
   if ((Criterion->RubCod = Str_ConvertStrCodToLongCod (row[1])) <= 0)
      Err_WrongRubricExit ();

   /***** Get criterion index (row[2]) *****/
   Criterion->CriInd = Str_ConvertStrToUnsigned (row[2]);

   /***** Get type (row[3]) and code (row[4]) *****/
   Criterion->Link.Type = Rsc_GetTypeFromString (row[3]);
   Criterion->Link.Cod  = Str_ConvertStrCodToLongCod (row[4]);

   /***** Get criterion minimum and maximum values (row[5], row[6]) *****/
   for (ValueRange  = (RubCri_ValueRange_t) 0;
	ValueRange <= (RubCri_ValueRange_t) (RubCri_NUM_VALUES - 1);
	ValueRange++)
      Criterion->Values[ValueRange] = Str_GetDoubleFromStr (row[5 + ValueRange]);

   /***** Get criterion weight (row[7]) *****/
   Criterion->Weight = Str_GetDoubleFromStr (row[5 + RubCri_NUM_VALUES]);

   /***** Get the title of the criterion (row[8]) *****/
   Str_Copy (Criterion->Title,row[5 + RubCri_NUM_VALUES + 1],
             sizeof (Criterion->Title) - 1);
  }

/*****************************************************************************/
/****************** Put table heading for rubric criteria ********************/
/*****************************************************************************/

static void RubCri_PutTableHeadingForCriteria (void)
  {
   extern const char *Txt_No_INDEX;
   extern const char *Txt_Criterion;
   extern const char *Txt_Source;
   extern const char *Txt_Minimum;
   extern const char *Txt_Maximum;
   extern const char *Txt_Weight;

   /***** Begin row *****/
   HTM_TR_Begin (NULL);

      /***** Header cells *****/
      HTM_TH_Empty (1);
      HTM_TH (Txt_No_INDEX ,HTM_HEAD_RIGHT);
      HTM_TH (Txt_Criterion,HTM_HEAD_LEFT );
      HTM_TH (Txt_Source   ,HTM_HEAD_LEFT );
      HTM_TH (Txt_Minimum  ,HTM_HEAD_LEFT );
      HTM_TH (Txt_Maximum  ,HTM_HEAD_LEFT );
      HTM_TH (Txt_Weight   ,HTM_HEAD_LEFT );

   /***** End row *****/
   HTM_TR_End ();
  }

/*****************************************************************************/
/*************************** Reset rubric criterion **************************/
/*****************************************************************************/

void RubCri_ResetCriterion (struct RubCri_Criterion *Criterion)
  {
   // Default values minimum/maximum criterion values
   static double RubCri_DefaultValues[RubCri_NUM_VALUES] =
     {
      [RubCri_MIN] =  0.0,
      [RubCri_MAX] = 10.0,
     };
   RubCri_ValueRange_t ValueRange;

   Criterion->RubCod = -1L;
   Criterion->CriCod = -1L;
   Criterion->CriInd = 0;
   Criterion->Link.Type = Rsc_NONE;
   Criterion->Link.Cod  = -1L;
   for (ValueRange  = (RubCri_ValueRange_t) 0;
	ValueRange <= (RubCri_ValueRange_t) (RubCri_NUM_VALUES - 1);
	ValueRange++)
      Criterion->Values[ValueRange] = RubCri_DefaultValues[ValueRange];
   Criterion->Weight = RubCri_WEIGHT_MAX;;
   Criterion->Title[0] = '\0';
  }

/*****************************************************************************/
/***************** Request the removal of a rubric criterion *****************/
/*****************************************************************************/

void RubCri_ReqRemCriterion (void)
  {
   extern const char *Txt_Do_you_really_want_to_remove_the_criterion_X;
   extern const char *Txt_Remove_criterion;
   struct Rub_Rubrics Rubrics;
   struct RubCri_Criterion Criterion;

   /***** Reset rubrics context *****/
   Rub_ResetRubrics (&Rubrics);
   Rub_ResetRubric (&Rubrics.Rubric);
   RubCri_ResetCriterion (&Criterion);

   /***** Get and check parameters *****/
   RubCri_GetAndCheckPars (&Rubrics,&Criterion);

   /***** Check if rubric is editable *****/
   if (!Rub_CheckIfEditable (&Rubrics.Rubric))
      Err_NoPermissionExit ();

   /***** Show question and button to remove question *****/
   Ale_ShowAlertAndButton (ActRemRubCri,NULL,NULL,
			   RubCri_PutParsOneCriterion,&Rubrics,
			   Btn_REMOVE_BUTTON,Txt_Remove_criterion,
			   Ale_QUESTION,Txt_Do_you_really_want_to_remove_the_criterion_X,
			   Criterion.Title);

   /***** Show current rubric and its criteria *****/
   Rub_PutFormsOneRubric (&Rubrics,&Criterion,
                          Rub_EXISTING_RUBRIC);	// It's not a new rubric
  }

/*****************************************************************************/
/************************* Remove a rubric criterion *************************/
/*****************************************************************************/

void RubCri_RemoveCriterion (void)
  {
   extern const char *Txt_Criterion_removed;
   struct Rub_Rubrics Rubrics;
   struct RubCri_Criterion Criterion;

   /***** Reset rubrics context *****/
   Rub_ResetRubrics (&Rubrics);
   Rub_ResetRubric (&Rubrics.Rubric);
   RubCri_ResetCriterion (&Criterion);

   /***** Get and check parameters *****/
   RubCri_GetAndCheckPars (&Rubrics,&Criterion);

   /***** Check if rubric is editable *****/
   if (!Rub_CheckIfEditable (&Rubrics.Rubric))
      Err_NoPermissionExit ();

   /***** Remove the criterion from all tables *****/
   /* Remove questions associated to criterion */
   // Exa_DB_RemoveAllSetQuestionsFromSet (Criterion.CriCod,Criterion.RubCod);

   /* Remove the criterion itself */
   Rub_DB_RemoveCriterionFromRubric (&Criterion);

   /* Change indexes of criteria greater than this */
   Rub_DB_UpdateCriteriaIndexesInRubricGreaterThan (&Criterion);

   /***** Write message *****/
   Ale_ShowAlert (Ale_SUCCESS,Txt_Criterion_removed);

   /***** Show current rubric and its criteria *****/
   Rub_PutFormsOneRubric (&Rubrics,&Criterion,
                          Rub_EXISTING_RUBRIC);	// It's not a new rubric
  }

/*****************************************************************************/
/*************** Move up position of a criterion in a rubric *****************/
/*****************************************************************************/

void RubCri_MoveUpCriterion (void)
  {
   extern const char *Txt_Movement_not_allowed;
   struct Rub_Rubrics Rubrics;
   struct RubCri_Criterion Criterion;
   unsigned CriIndTop;
   unsigned CriIndBottom;

   /***** Reset rubrics context *****/
   Rub_ResetRubrics (&Rubrics);
   Rub_ResetRubric (&Rubrics.Rubric);
   RubCri_ResetCriterion (&Criterion);

   /***** Get and check parameters *****/
   RubCri_GetAndCheckPars (&Rubrics,&Criterion);

   /***** Check if rubric is editable *****/
   if (!Rub_CheckIfEditable (&Rubrics.Rubric))
      Err_NoPermissionExit ();

   /***** Get criterion index *****/
   CriIndBottom = Rub_DB_GetCriIndFromCriCod (Rubrics.Rubric.RubCod,Criterion.CriCod);

   /***** Move up criterion *****/
   if (CriIndBottom > 1)
     {
      /* Indexes of criteria to be exchanged */
      CriIndTop = Rub_DB_GetPrevCriterionIndexInRubric (Rubrics.Rubric.RubCod,CriIndBottom);
      if (CriIndTop == 0)
	 Err_ShowErrorAndExit ("Wrong criterion index.");

      /* Exchange criteria */
      RubCri_ExchangeCriteria (Rubrics.Rubric.RubCod,CriIndTop,CriIndBottom);
     }
   else
      Ale_ShowAlert (Ale_WARNING,Txt_Movement_not_allowed);

   /***** Show current rubric and its criteria *****/
   Rub_PutFormsOneRubric (&Rubrics,&Criterion,
                          Rub_EXISTING_RUBRIC);	// It's not a new rubric
  }

/*****************************************************************************/
/************** Move down position of a criterion in a rubric ****************/
/*****************************************************************************/

void RubCri_MoveDownCriterion (void)
  {
   extern const char *Txt_Movement_not_allowed;
   struct Rub_Rubrics Rubrics;
   struct RubCri_Criterion Criterion;
   unsigned CriIndTop;
   unsigned CriIndBottom;
   unsigned MaxCriInd;	// 0 if no criteria

   /***** Reset rubrics context *****/
   Rub_ResetRubrics (&Rubrics);
   Rub_ResetRubric (&Rubrics.Rubric);
   RubCri_ResetCriterion (&Criterion);

   /***** Get and check parameters *****/
   RubCri_GetAndCheckPars (&Rubrics,&Criterion);

   /***** Check if rubric is editable *****/
   if (!Rub_CheckIfEditable (&Rubrics.Rubric))
      Err_NoPermissionExit ();

   /***** Get criterion index *****/
   CriIndTop = Rub_DB_GetCriIndFromCriCod (Rubrics.Rubric.RubCod,Criterion.CriCod);

   /***** Get maximum criterion index *****/
   MaxCriInd = Rub_DB_GetMaxCriterionIndexInRubric (Rubrics.Rubric.RubCod);

   /***** Move down criterion *****/
   if (CriIndTop < MaxCriInd)
     {
      /* Indexes of criteria to be exchanged */
      CriIndBottom = Rub_DB_GetNextCriterionIndexInRubric (Rubrics.Rubric.RubCod,CriIndTop);
      if (CriIndBottom == 0)	// 0 means error reading from database
	 Err_ShowErrorAndExit ("Wrong criterion index.");

      /* Exchange criteria */
      RubCri_ExchangeCriteria (Rubrics.Rubric.RubCod,CriIndTop,CriIndBottom);
     }
   else
      Ale_ShowAlert (Ale_WARNING,Txt_Movement_not_allowed);

   /***** Show current rubric and its criteria *****/
   Rub_PutFormsOneRubric (&Rubrics,&Criterion,
                          Rub_EXISTING_RUBRIC);	// It's not a new trubric
  }

/*****************************************************************************/
/************************** Get and check parameters *************************/
/*****************************************************************************/

static void RubCri_GetAndCheckPars (struct Rub_Rubrics *Rubrics,
                                    struct RubCri_Criterion *Criterion)
  {
   /***** Get parameters *****/
   Rub_GetPars (Rubrics,true);
   Criterion->CriCod = ParCod_GetAndCheckPar (ParCod_Cri);

   /***** Get rubric data from database *****/
   Rub_GetRubricDataByCod (&Rubrics->Rubric);
   if (Rubrics->Rubric.CrsCod != Gbl.Hierarchy.Crs.CrsCod)
      Err_WrongRubricExit ();

   /***** Get criterion data from database *****/
   RubCri_GetCriterionDataByCod (Criterion);
   if (Criterion->RubCod != Rubrics->Rubric.RubCod)
      Err_WrongCriterionExit ();
   Rubrics->CriCod = Criterion->CriCod;
  }

/*****************************************************************************/
/******** Exchange the order of two consecutive criteria in a rubric *********/
/*****************************************************************************/

static void RubCri_ExchangeCriteria (long RubCod,
                                     unsigned CriIndTop,unsigned CriIndBottom)
  {
   long CriCodTop;
   long CriCodBottom;

   /***** Lock table to make the move atomic *****/
   Rub_DB_LockTable ();

   /***** Get criterion codes of the sets to be moved *****/
   CriCodTop    = Rub_DB_GetCriCodFromCriInd (RubCod,CriIndTop   );
   CriCodBottom = Rub_DB_GetCriCodFromCriInd (RubCod,CriIndBottom);

   /***** Exchange indexes of sets *****/
   /*
   Example:
   CriIndTop    = 1; CriCodTop    = 218
   CriIndBottom = 2; CriCodBottom = 220
                     Step 1              Step 2              Step 3
+--------+--------+ +--------+--------+ +--------+--------+ +--------+--------+
| CriInd | CriCod | | CriInd | CriCod | | CriInd | CriCod | | CriInd | CriCod |
+--------+--------+ +--------+--------+ +--------+--------+ +--------+--------+
|      1 |    218 |>|     -2 |    218 |>|     -2 |    218 |>|      2 |    218 |
|      2 |    220 | |      2 |    220 | |      1 |    220 | |      1 |    220 |
|      3 |    232 | |      3 |    232 | |      3 |    232 | |      3 |    232 |
+--------+--------+ +--------+--------+ +--------+--------+ +--------+--------+
   */
   /* Step 1: change temporarily top index to minus bottom index
              in order to not repeat unique index (RubCod,CriInd) */
   Rub_DB_UpdateCriterionIndex (-((long) CriIndBottom),CriCodTop   ,RubCod);

   /* Step 2: change bottom index to old top index  */
   Rub_DB_UpdateCriterionIndex (  (long) CriIndTop    ,CriCodBottom,RubCod);

   /* Step 3: change top index to old bottom index */
   Rub_DB_UpdateCriterionIndex (  (long) CriIndBottom ,CriCodTop   ,RubCod);

   /***** Unlock table *****/
   DB_UnlockTables ();
  }

/*****************************************************************************/
/************************** Show criterion resource **************************/
/*****************************************************************************/
/*
static void RubCri_ShowResource (struct Rub_Rubrics *Rubrics,
                                 const struct RubCri_Criterion *Criterion,
                                 bool Editing,const char *Anchor)
  {
   extern const char *Rsc_ResourceTypesIcons[Rsc_NUM_TYPES];
   extern const char *Txt_RESOURCE_TYPES[Rsc_NUM_TYPES];

   Rsc_WriteLinkName (&Criterion->Link,
		      true,	// Put form to go
                      Rsc_ResourceTypesIcons[Criterion->Link.Type],
	              Txt_RESOURCE_TYPES[Criterion->Link.Type]);
  }
*/

/*****************************************************************************/
/***************** Show clipboard to change resource link ********************/
/*****************************************************************************/

void RubCri_ChangeLink (void)
  {
   struct Rub_Rubrics Rubrics;
   struct RubCri_Criterion Criterion;

   /***** Reset rubrics context *****/
   Rub_ResetRubrics (&Rubrics);
   Rub_ResetRubric (&Rubrics.Rubric);
   RubCri_ResetCriterion (&Criterion);

   /***** Get and check parameters *****/
   RubCri_GetAndCheckPars (&Rubrics,&Criterion);

   /***** Check if rubric is editable *****/
   if (!Rub_CheckIfEditable (&Rubrics.Rubric))
      Err_NoPermissionExit ();

   /***** Get link type and code *****/
   if (Rsc_GetParLink (&Criterion.Link))
     {
      /***** Update link to resource in criterion *****/
      Rub_DB_UpdateCriterionLink (&Criterion);

      /***** Remove link from clipboard *****/
      Rsc_DB_RemoveLinkFromClipboard (&Criterion.Link);
     }

   /***** Show current rubric and its criteria *****/
   Rub_PutFormsOneRubric (&Rubrics,&Criterion,
                          Rub_EXISTING_RUBRIC);	// It's not a new rubric
  }
