// swad_exam.c: exams

/*
    SWAD (Shared Workspace At a Distance),
    is a web platform developed at the University of Granada (Spain),
    and used to support university teaching.

    This file is part of SWAD core.
    Copyright (C) 1999-2023 Antonio Ca�as Vargas

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
#include <float.h>		// For DBL_MAX
#include <linux/limits.h>	// For PATH_MAX
#include <stddef.h>		// For NULL
#include <stdio.h>		// For asprintf
#include <stdlib.h>		// For free
#include <string.h>		// For string functions

#include "swad_action_list.h"
#include "swad_autolink.h"
#include "swad_box.h"
#include "swad_database.h"
#include "swad_error.h"
#include "swad_exam.h"
#include "swad_exam_database.h"
#include "swad_exam_print.h"
#include "swad_exam_result.h"
#include "swad_exam_session.h"
#include "swad_exam_set.h"
#include "swad_exam_type.h"
#include "swad_figure.h"
#include "swad_form.h"
#include "swad_global.h"
#include "swad_hierarchy_level.h"
#include "swad_HTML.h"
#include "swad_match.h"
#include "swad_match_result.h"
#include "swad_pagination.h"
#include "swad_parameter.h"
#include "swad_parameter_code.h"
#include "swad_program_database.h"
#include "swad_role.h"
#include "swad_test.h"
#include "swad_test_visibility.h"

/*****************************************************************************/
/************** External global variables from others modules ****************/
/*****************************************************************************/

extern struct Globals Gbl;

/*****************************************************************************/
/***************************** Private constants *****************************/
/*****************************************************************************/

#define Exa_MAX_CHARS_ANSWER	(1024 - 1)	// 1023
#define Exa_MAX_BYTES_ANSWER	((Exa_MAX_CHARS_ANSWER + 1) * Str_MAX_BYTES_PER_CHAR - 1)	// 16383

#define Exa_MAX_ANSWERS_PER_QUESTION	10

/* Score range [0...max.score]
   will be converted to
   grade range [0...max.grade]
   Example: Exam with 5 questions, unique-choice, 4 options per question
            max.score = 5 *   1     =  5
            min.score = 5 * (-0.33) = -1,67
            max.grade given by teacher = 0.2 ==> min.grade = -0,067

              grade
                ^
                |          /
   max.grade--> +---------+
                |        /|
                |       / |
                |      /  |
                |     /   |
                |    /    |
                |   /     |
                |  /      |
                | /       |
                |/        |
    ------+---0-+---------+---------> score
          ^    /0         ^
     min.score/ |      max.score
          |  /  |   (num.questions)
          | /   |
          |/    |
          +-----+ <--min.grade
         /      |
*/
#define Exa_MAX_GRADE_DEFAULT 1.0

/*****************************************************************************/
/***************************** Private prototypes ****************************/
/*****************************************************************************/

static void Exa_PutIconsListExams (void *Exams);
static void Exa_PutIconToCreateNewExam (struct Exa_Exams *Exams);
static void Exa_PutButtonToCreateNewExam (struct Exa_Exams *Exams);
static void Exa_PutParsToCreateNewExam (void *Exams);

static void Exa_ShowOneExam (struct Exa_Exams *Exams,bool ShowOnlyThisExam);

static void Exa_PutIconsOneExam (void *Exams);
static void Exa_WriteAuthor (struct Exa_Exam *Exam);

static void Exa_PutParExamOrder (Exa_Order_t SelectedOrder);

static void Exa_PutIconsToRemEditOneExam (struct Exa_Exams *Exams,
					  const char *Anchor);

static void Exa_PutParOrder (Exa_Order_t SelectedOrder);
static Exa_Order_t Exa_GetParOrder (void);

static void Exa_RemoveExamFromAllTables (long ExaCod);

static void Exa_RemoveAllMedFilesFromStemOfAllQstsFromExam (long ExaCod);
static void Exa_RemoveAllMedFilesFromStemOfAllQstsFromCrs (long CrsCod);
static void Exa_RemoveAllMedFilesFromAnsOfAllQstsFromExam (long ExaCod);
static void Exa_RemoveAllMedFilesFromAnsOfAllQstsFromCrs (long CrsCod);

static void Exa_HideUnhideExam (bool Hide);

static void Exa_ReceiveExamFieldsFromForm (struct Exa_Exam *Exam,
				           char Txt[Cns_MAX_BYTES_TEXT + 1]);
static bool Exa_CheckExamFieldsReceivedFromForm (const struct Exa_Exam *Exam);

static void Exa_CreateExam (struct Exa_Exam *Exam,const char *Txt);
static void Exa_UpdateExam (struct Exa_Exam *Exam,const char *Txt);

/*****************************************************************************/
/******************************* Reset exams *********************************/
/*****************************************************************************/

void Exa_ResetExams (struct Exa_Exams *Exams)
  {
   Exams->LstIsRead         = false;	// List not read from database...
   Exams->Num               = 0;	// Total number of exams
   Exams->NumSelected       = 0;	// Number of exams selected
   Exams->Lst               = NULL;	// List of exams
   Exams->SelectedOrder     = Exa_ORDER_DEFAULT;
   Exams->CurrentPage       = 0;
   Exams->ListQuestions     = NULL;
   Exams->ExaCodsSelected   = NULL;	// String with selected exam codes separated by separator multiple
   Exams->Exam.ExaCod       = -1L;	// Selected/current exam code
   Exams->SesCod            = -1L;	// Selected/current session code
   Exams->SetInd            = 0;	// Current set index
   Exams->QstCod            = -1L;	// Current question code
  }

/*****************************************************************************/
/*************************** Initialize exam to empty ************************/
/*****************************************************************************/

void Exa_ResetExam (struct Exa_Exam *Exam)
  {
   /***** Initialize to empty exam *****/
   Exam->ExaCod                  = -1L;
   Exam->CrsCod                  = -1L;
   Exam->UsrCod                  = -1L;
   Exam->MaxGrade                = Exa_MAX_GRADE_DEFAULT;
   Exam->Visibility              = TstVis_VISIBILITY_DEFAULT;
   Exam->TimeUTC[Dat_STR_TIME] = (time_t) 0;
   Exam->TimeUTC[Dat_END_TIME] = (time_t) 0;
   Exam->Title[0]                = '\0';
   Exam->Hidden                  = false;
   Exam->NumSets                 = 0;
   Exam->NumQsts                 = 0;
   Exam->NumSess                 = 0;
   Exam->NumOpenSess             = 0;
  }

/*****************************************************************************/
/***************************** List all exams ********************************/
/*****************************************************************************/

void Exa_SeeAllExams (void)
  {
   struct Exa_Exams Exams;

   /***** Reset exams context *****/
   Exa_ResetExams (&Exams);

   /***** Get parameters *****/
   Exa_GetPars (&Exams,Exa_DONT_CHECK_EXA_COD);

   /***** Show all exams *****/
   Exa_ListAllExams (&Exams);
  }

/*****************************************************************************/
/******************************* Show all exams ******************************/
/*****************************************************************************/

void Exa_ListAllExams (struct Exa_Exams *Exams)
  {
   extern const char *Hlp_ASSESSMENT_Exams;
   extern const char *Txt_Exams;
   extern const char *Txt_EXAMS_ORDER_HELP[Exa_NUM_ORDERS];
   extern const char *Txt_EXAMS_ORDER[Exa_NUM_ORDERS];
   extern const char *Txt_Sessions;
   extern const char *Txt_No_exams;
   Exa_Order_t Order;
   struct Pag_Pagination Pagination;
   unsigned NumExam;

   /***** Get number of groups in current course *****/
   if (!Gbl.Crs.Grps.NumGrps)
      Gbl.Crs.Grps.WhichGrps = Grp_ALL_GROUPS;

   /***** Get list of exams *****/
   Exa_GetListExams (Exams,Exams->SelectedOrder);

   /***** Compute variables related to pagination *****/
   Pagination.NumItems = Exams->Num;
   Pagination.CurrentPage = (int) Exams->CurrentPage;
   Pag_CalculatePagination (&Pagination);
   Exams->CurrentPage = (unsigned) Pagination.CurrentPage;

   /***** Begin box *****/
   Box_BoxBegin ("100%",Txt_Exams,
                 Exa_PutIconsListExams,Exams,
                 Hlp_ASSESSMENT_Exams,Box_NOT_CLOSABLE);

      /***** Write links to pages *****/
      Pag_WriteLinksToPagesCentered (Pag_EXAMS,&Pagination,
				     Exams,-1L);

      if (Exams->Num)
	{
	 /***** Table head *****/
	 HTM_TABLE_BeginWideMarginPadding (5);
	    HTM_TR_Begin (NULL);
               HTM_TH_Span (NULL,HTM_HEAD_CENTER,1,1,"CONTEXT_COL");	// Column for contextual icons

	       for (Order  = (Exa_Order_t) 0;
		    Order <= (Exa_Order_t) (Exa_NUM_ORDERS - 1);
		    Order++)
		 {
                  HTM_TH_Begin (HTM_HEAD_LEFT);

		     /* Form to change order */
		     Frm_BeginForm (ActSeeAllExa);
			Pag_PutParPagNum (Pag_EXAMS,Exams->CurrentPage);
			Par_PutParUnsigned (NULL,"Order",(unsigned) Order);
			HTM_BUTTON_Submit_Begin (Txt_EXAMS_ORDER_HELP[Order],
			                         "class=\"BT_LINK\"");
			   if (Order == Exams->SelectedOrder)
			      HTM_U_Begin ();
			   HTM_Txt (Txt_EXAMS_ORDER[Order]);
			   if (Order == Exams->SelectedOrder)
			      HTM_U_End ();
			HTM_BUTTON_End ();
		     Frm_EndForm ();

		  HTM_TH_End ();
		 }

	       HTM_TH (Txt_Sessions,HTM_HEAD_RIGHT);

	    HTM_TR_End ();

	    /***** Write all exams *****/
	    for (NumExam  = Pagination.FirstItemVisible;
		 NumExam <= Pagination.LastItemVisible;
		 NumExam++)
	      {
	       /* Get data of this exam */
	       Exams->Exam.ExaCod = Exams->Lst[NumExam - 1].ExaCod;
	       Exa_GetExamDataByCod (&Exams->Exam);

	       /* Show exam */
	       Exa_ShowOneExam (Exams,
				false);	// Do not show only this exam
	      }

	 /***** End table *****/
	 HTM_TABLE_End ();
	}
      else	// No exams created
	 Ale_ShowAlert (Ale_INFO,Txt_No_exams);

      /***** Write again links to pages *****/
      Pag_WriteLinksToPagesCentered (Pag_EXAMS,&Pagination,
				     Exams,-1L);

      /***** Button to create a new exam *****/
      if (Exa_CheckIfICanEditExams ())
	 Exa_PutButtonToCreateNewExam (Exams);

   /***** End box *****/
   Box_BoxEnd ();

   /***** Free list of exams *****/
   Exa_FreeListExams (Exams);
  }

/*****************************************************************************/
/************************ Check if I can edit exams **************************/
/*****************************************************************************/

bool Exa_CheckIfICanEditExams (void)
  {
   static const bool ICanEditExams[Rol_NUM_ROLES] =
     {
      [Rol_TCH    ] = true,
      [Rol_SYS_ADM] = true,
     };

   return ICanEditExams[Gbl.Usrs.Me.Role.Logged];
  }

/*****************************************************************************/
/***************** Put contextual icons in list of exams *******************/
/*****************************************************************************/

static void Exa_PutIconsListExams (void *Exams)
  {
   static const Act_Action_t NextAction[Rol_NUM_ROLES] =
     {
      [Rol_STD    ] = ActSeeMyExaResCrs,
      [Rol_NET    ] = ActReqSeeUsrExaRes,
      [Rol_TCH    ] = ActReqSeeUsrExaRes,
      [Rol_SYS_ADM] = ActReqSeeUsrExaRes,
     };

   if (Exams)
     {
      /***** Put icon to create a new exam *****/
      if (Exa_CheckIfICanEditExams ())
	 Exa_PutIconToCreateNewExam ((struct Exa_Exams *) Exams);

      /***** Put icon to view sessions results *****/
      if (NextAction[Gbl.Usrs.Me.Role.Logged])
	 Ico_PutContextualIconToShowResults (NextAction[Gbl.Usrs.Me.Role.Logged],NULL,
					     NULL,NULL);

      /***** Link to get resource link *****/
      if (Rsc_CheckIfICanGetLink ())
	 Ico_PutContextualIconToGetLink (ActReqLnkExa,NULL,
					 Exa_PutPars,Exams);

      /***** Put icon to show a figure *****/
      Fig_PutIconToShowFigure (Fig_EXAMS);
     }
  }

/*****************************************************************************/
/********************** Put icon to create a new exam **********************/
/*****************************************************************************/

static void Exa_PutIconToCreateNewExam (struct Exa_Exams *Exams)
  {
   Ico_PutContextualIconToAdd (ActFrmNewExa,NULL,
                               Exa_PutParsToCreateNewExam,Exams);
  }

/*****************************************************************************/
/********************* Put button to create a new exam *********************/
/*****************************************************************************/

static void Exa_PutButtonToCreateNewExam (struct Exa_Exams *Exams)
  {
   extern const char *Txt_New_exam;

   Frm_BeginForm (ActFrmNewExa);
      Exa_PutParsToCreateNewExam (Exams);
      Btn_PutConfirmButton (Txt_New_exam);
   Frm_EndForm ();
  }

/*****************************************************************************/
/******************* Put parameters to create a new exam *******************/
/*****************************************************************************/

static void Exa_PutParsToCreateNewExam (void *Exams)
  {
   if (Exams)
     {
      Exa_PutParExamOrder (((struct Exa_Exams *) Exams)->SelectedOrder);
      Pag_PutParPagNum (Pag_EXAMS,((struct Exa_Exams *) Exams)->CurrentPage);
     }
  }

/*****************************************************************************/
/****************************** Show one exam ******************************/
/*****************************************************************************/

void Exa_SeeOneExam (void)
  {
   struct Exa_Exams Exams;
   struct ExaSes_Session Session;

   /***** Reset exams context *****/
   Exa_ResetExams (&Exams);
   Exa_ResetExam (&Exams.Exam);
   ExaSes_ResetSession (&Session);

   /***** Get parameters *****/
   Exa_GetPars (&Exams,Exa_CHECK_EXA_COD);

   /***** Get exam data *****/
   Exa_GetExamDataByCod (&Exams.Exam);

   /***** Show exam *****/
   Exa_ShowOnlyOneExam (&Exams,&Session,
	                false);	// Do not put form for session
  }

/*****************************************************************************/
/******************************* Show one exam *******************************/
/*****************************************************************************/

void Exa_ShowOnlyOneExam (struct Exa_Exams *Exams,
			  struct ExaSes_Session *Session,
			  bool PutFormSession)
  {
   Exa_ShowOnlyOneExamBegin (Exams,Session,PutFormSession);
   Exa_ShowOnlyOneExamEnd ();
  }

void Exa_ShowOnlyOneExamBegin (struct Exa_Exams *Exams,
			       struct ExaSes_Session *Session,
			       bool PutFormSession)
  {
   extern const char *Hlp_ASSESSMENT_Exams;
   extern const char *Txt_Exam;

   /***** Begin box *****/
   Box_BoxBegin (NULL,Exams->Exam.Title[0] ? Exams->Exam.Title :
					     Txt_Exam,
                 Exa_PutIconsOneExam,Exams,
		 Hlp_ASSESSMENT_Exams,Box_NOT_CLOSABLE);

   /***** Show exam *****/
   Exa_ShowOneExam (Exams,
		    true);	// Show only this exam

   /***** List sessions *****/
   ExaSes_ListSessions (Exams,Session,PutFormSession);
  }

void Exa_ShowOnlyOneExamEnd (void)
  {
   /***** End box *****/
   Box_BoxEnd ();
  }

static void Exa_ShowOneExam (struct Exa_Exams *Exams,bool ShowOnlyThisExam)
  {
   extern const char *Txt_View_exam;
   extern const char *Txt_Sets_of_questions;
   extern const char *Txt_Maximum_grade;
   extern const char *Txt_Result_visibility;
   extern const char *Txt_Sessions;
   char *Anchor;
   static unsigned UniqueId = 0;
   char *Id;
   Dat_StartEndTime_t StartEndTime;
   const char *Color;
   char Txt[Cns_MAX_BYTES_TEXT + 1];

   /***** Build anchor string *****/
   Frm_SetAnchorStr (Exams->Exam.ExaCod,&Anchor);

   /***** Begin box and table *****/
   if (ShowOnlyThisExam)
      HTM_TABLE_BeginWidePadding (2);

   /***** Begin first row of this exam *****/
   HTM_TR_Begin (NULL);

      /***** Icons related to this exam *****/
      if (!ShowOnlyThisExam)
	{
	 HTM_TD_Begin ("rowspan=\"2\" class=\"CONTEXT_COL %s\"",
		       The_GetColorRows ());
	    Exa_PutIconsToRemEditOneExam (Exams,Anchor);
	 HTM_TD_End ();
	}

      /***** Start/end date/time *****/
      UniqueId++;
      for (StartEndTime  = (Dat_StartEndTime_t) 0;
	   StartEndTime <= (Dat_StartEndTime_t) (Dat_NUM_START_END_TIME - 1);
	   StartEndTime++)
	{
	 if (asprintf (&Id,"exa_date_%u_%u",(unsigned) StartEndTime,UniqueId) < 0)
	    Err_NotEnoughMemoryExit ();
	 Color = Exams->Exam.NumOpenSess ? (Exams->Exam.Hidden ? "DATE_GREEN_LIGHT":
								 "DATE_GREEN") :
					   (Exams->Exam.Hidden ? "DATE_RED_LIGHT":
								 "DATE_RED");
	 if (ShowOnlyThisExam)
	    HTM_TD_Begin ("id=\"%s\" class=\"LT %s_%s\"",
			  Id,Color,The_GetSuffix ());
	 else
	    HTM_TD_Begin ("id=\"%s\" class=\"LT %s_%s %s\"",
			  Id,Color,The_GetSuffix (),The_GetColorRows ());
	 if (Exams->Exam.TimeUTC[Dat_STR_TIME])
	    Dat_WriteLocalDateHMSFromUTC (Id,Exams->Exam.TimeUTC[StartEndTime],
					  Gbl.Prefs.DateFormat,Dat_SEPARATOR_BREAK,
					  true,true,true,0x6);
	 HTM_TD_End ();
	 free (Id);
	}

      /***** Exam title and main data *****/
      if (ShowOnlyThisExam)
	 HTM_TD_Begin ("class=\"LT\"");
      else
	 HTM_TD_Begin ("class=\"LT %s\"",The_GetColorRows ());

      /* Exam title */
      HTM_ARTICLE_Begin (Anchor);
	 Frm_BeginForm (ActSeeOneExa);
	    Exa_PutPars (Exams);
	    HTM_BUTTON_Submit_Begin (Txt_View_exam,"class=\"LT BT_LINK %s_%s\"",
				     Exams->Exam.Hidden ? "ASG_TITLE_LIGHT":
							  "ASG_TITLE",
				     The_GetSuffix ());
	       HTM_Txt (Exams->Exam.Title);
	    HTM_BUTTON_End ();
	 Frm_EndForm ();
      HTM_ARTICLE_End ();

      /* Number of questions, maximum grade, visibility of results */
      HTM_DIV_Begin ("class=\"%s_%s\"",
                     Exams->Exam.Hidden ? "ASG_GRP_LIGHT" :
					  "ASG_GRP",
		     The_GetSuffix ());
	 HTM_TxtColonNBSP (Txt_Sets_of_questions);
	 HTM_Unsigned (Exams->Exam.NumSets);
	 HTM_BR ();
	 HTM_TxtColonNBSP (Txt_Maximum_grade);
	 HTM_Double (Exams->Exam.MaxGrade);
	 HTM_BR ();
	 HTM_TxtColonNBSP (Txt_Result_visibility);
	 TstVis_ShowVisibilityIcons (Exams->Exam.Visibility,Exams->Exam.Hidden);
      HTM_DIV_End ();

      /***** Number of sessions in exam *****/
      if (ShowOnlyThisExam)
	 HTM_TD_Begin ("class=\"RT\"");
      else
	 HTM_TD_Begin ("class=\"RT %s\"",The_GetColorRows ());

      Frm_BeginForm (ActSeeOneExa);
	 Exa_PutPars (Exams);
	 HTM_BUTTON_Submit_Begin (Txt_Sessions,"class=\"LT BT_LINK %s_%s\"",
				  Exams->Exam.Hidden ? "ASG_TITLE_LIGHT":
						       "ASG_TITLE",
				  The_GetSuffix ());
	    if (ShowOnlyThisExam)
	       HTM_TxtColonNBSP (Txt_Sessions);
	    HTM_Unsigned (Exams->Exam.NumSess);
	 HTM_BUTTON_End ();
      Frm_EndForm ();

      HTM_TD_End ();

   /***** End 1st row of this exam *****/
   HTM_TR_End ();

   /***** Begin 2nd row of this exam *****/
   HTM_TR_Begin (NULL);

      /***** Author of the exam *****/
      if (ShowOnlyThisExam)
	 HTM_TD_Begin ("colspan=\"2\" class=\"LT\"");
      else
	 HTM_TD_Begin ("colspan=\"2\" class=\"LT %s\"",
	               The_GetColorRows ());
      Exa_WriteAuthor (&Exams->Exam);
      HTM_TD_End ();

      /***** Text of the exam *****/
      if (ShowOnlyThisExam)
	 HTM_TD_Begin ("colspan=\"2\" class=\"LT\"");
      else
	 HTM_TD_Begin ("colspan=\"2\" class=\"LT %s\"",
	               The_GetColorRows ());
      Exa_DB_GetExamTxt (Exams->Exam.ExaCod,Txt);
      Str_ChangeFormat (Str_FROM_HTML,Str_TO_RIGOROUS_HTML,
			Txt,Cns_MAX_BYTES_TEXT,false);	// Convert from HTML to rigorous HTML
      ALn_InsertLinks (Txt,Cns_MAX_BYTES_TEXT,60);	// Insert links
      HTM_DIV_Begin ("class=\"PAR %s_%s\"",
                     Exams->Exam.Hidden ? "DAT_LIGHT" :
					  "DAT",
		     The_GetSuffix ());
	 HTM_Txt (Txt);
      HTM_DIV_End ();
      HTM_TD_End ();

   /***** End 2nd row of this exam *****/
   HTM_TR_End ();

   /***** End table *****/
   if (ShowOnlyThisExam)
      HTM_TABLE_End ();
   else
      The_ChangeRowColor ();

   /***** Free anchor string *****/
   Frm_FreeAnchorStr (&Anchor);
  }

/*****************************************************************************/
/************ Put icon to show results of sessions in an exam ****************/
/*****************************************************************************/

static void Exa_PutIconsOneExam (void *Exams)
  {
   char *Anchor;

   if (Exams)
     {
      /***** Build anchor string *****/
      Frm_SetAnchorStr (((struct Exa_Exams *) Exams)->Exam.ExaCod,&Anchor);

      /***** Icons to remove/edit this exam *****/
      Exa_PutIconsToRemEditOneExam ((struct Exa_Exams *) Exams,Anchor);

      /***** Free anchor string *****/
      Frm_FreeAnchorStr (&Anchor);
     }
  }

/*****************************************************************************/
/*********************** Write the author of an exam ************************/
/*****************************************************************************/

static void Exa_WriteAuthor (struct Exa_Exam *Exam)
  {
   Usr_WriteAuthor1Line (Exam->UsrCod,Exam->Hidden);
  }

/*****************************************************************************/
/****** Put a hidden parameter with the type of order in list of exams *******/
/*****************************************************************************/

static void Exa_PutParExamOrder (Exa_Order_t SelectedOrder)
  {
   Par_PutParUnsigned (NULL,"Order",(unsigned) SelectedOrder);
  }

/*****************************************************************************/
/******************** Put a link (form) to edit one exam *********************/
/*****************************************************************************/

static void Exa_PutIconsToRemEditOneExam (struct Exa_Exams *Exams,
					  const char *Anchor)
  {
   static Act_Action_t ActionHideUnhide[2] =
     {
      [false] = ActHidExa,	// Visible ==> action to hide
      [true ] = ActUnhExa,	// Hidden ==> action to unhide
     };
   static const Act_Action_t ActionShowResults[Rol_NUM_ROLES] =
     {
      [Rol_STD    ] = ActSeeMyExaResExa,
      [Rol_NET    ] = ActSeeUsrExaResExa,
      [Rol_TCH    ] = ActSeeUsrExaResExa,
      [Rol_SYS_ADM] = ActSeeUsrExaResExa,
     };

   if (Exa_CheckIfICanEditExams ())
     {
      /***** Icon to remove exam *****/
      Ico_PutContextualIconToRemove (ActReqRemExa,NULL,
				     Exa_PutPars,Exams);

      /***** Icon to hide/unhide exam *****/
      Ico_PutContextualIconToHideUnhide (ActionHideUnhide,Anchor,
					 Exa_PutPars,Exams,
					 Exams->Exam.Hidden);

      /***** Icon to edit exam *****/
      Ico_PutContextualIconToEdit (ActEdiOneExa,NULL,
				   Exa_PutPars,Exams);
     }

   /***** Put icon to view results of sessions in exam *****/
   if (ActionShowResults[Gbl.Usrs.Me.Role.Logged])
      Ico_PutContextualIconToShowResults (ActionShowResults[Gbl.Usrs.Me.Role.Logged],ExaRes_RESULTS_BOX_ID,
					  Exa_PutPars,Exams);

   /***** Link to get resource link *****/
   if (Rsc_CheckIfICanGetLink ())
      Ico_PutContextualIconToGetLink (ActReqLnkExa,NULL,
				      Exa_PutPars,Exams);
  }

/*****************************************************************************/
/*********************** Params used to edit an exam **************************/
/*****************************************************************************/

void Exa_PutPars (void *Exams)
  {
   Grp_WhichGroups_t WhichGroups;

   if (Exams)
     {
      ParCod_PutPar (ParCod_Exa,((struct Exa_Exams *) Exams)->Exam.ExaCod);
      Exa_PutParOrder (((struct Exa_Exams *) Exams)->SelectedOrder);
      WhichGroups = Grp_GetParWhichGroups ();
      Grp_PutParWhichGroups (&WhichGroups);
      Pag_PutParPagNum (Pag_EXAMS,((struct Exa_Exams *) Exams)->CurrentPage);
     }
  }

/*****************************************************************************/
/******************* Get parameters used to edit an exam **********************/
/*****************************************************************************/

void Exa_GetPars (struct Exa_Exams *Exams,Exa_CheckExaCod_t CheckExaCod)
  {
   long (*GetExaCo[2]) (ParCod_Param_t ParCode) =
     {
      [Exa_DONT_CHECK_EXA_COD] = ParCod_GetPar,
      [Exa_CHECK_EXA_COD     ] = ParCod_GetAndCheckPar,
     };

   /***** Get other parameters *****/
   Exams->SelectedOrder = Exa_GetParOrder ();
   Exams->CurrentPage = Pag_GetParPagNum (Pag_EXAMS);

   /***** Get exam code *****/
   Exams->Exam.ExaCod = GetExaCo[CheckExaCod] (ParCod_Exa);
  }

/*****************************************************************************/
/****** Put a hidden parameter with the type of order in list of exams *******/
/*****************************************************************************/

static void Exa_PutParOrder (Exa_Order_t SelectedOrder)
  {
   if (SelectedOrder != Exa_ORDER_DEFAULT)
      Par_PutParUnsigned (NULL,"Order",(unsigned) SelectedOrder);
  }

/*****************************************************************************/
/********** Get parameter with the type or order in list of exams ************/
/*****************************************************************************/

static Exa_Order_t Exa_GetParOrder (void)
  {
   return (Exa_Order_t) Par_GetParUnsignedLong ("Order",
						0,
						Exa_NUM_ORDERS - 1,
						(unsigned long) Exa_ORDER_DEFAULT);
  }

/*****************************************************************************/
/************************** Get list of all exams ****************************/
/*****************************************************************************/

void Exa_GetListExams (struct Exa_Exams *Exams,Exa_Order_t SelectedOrder)
  {
   MYSQL_RES *mysql_res;
   MYSQL_ROW row;
   unsigned NumExam;

   /***** Free list of exams *****/
   if (Exams->LstIsRead)
      Exa_FreeListExams (Exams);

   /***** Get list of exams from database *****/
   if ((Exams->Num = Exa_DB_GetListExams (&mysql_res,SelectedOrder))) // Exams found...
     {
      /***** Create list of exams *****/
      if ((Exams->Lst = malloc ((size_t) Exams->Num *
                                sizeof (*Exams->Lst))) == NULL)
         Err_NotEnoughMemoryExit ();

      /***** Get the exams codes *****/
      for (NumExam = 0;
	   NumExam < Exams->Num;
	   NumExam++)
        {
         /* Get next exam code (row[0]) */
         row = mysql_fetch_row (mysql_res);
         if ((Exams->Lst[NumExam].ExaCod = Str_ConvertStrCodToLongCod (row[0])) <= 0)
            Err_WrongExamExit ();
        }
     }

   /***** Free structure that stores the query result *****/
   DB_FreeMySQLResult (&mysql_res);

   Exams->LstIsRead = true;
  }

/*****************************************************************************/
/******************** Get list of exam sessions selected *********************/
/*****************************************************************************/

void Exa_GetListSelectedExaCods (struct Exa_Exams *Exams)
  {
   extern const char *Par_CodeStr[];
   unsigned MaxSizeListExaCodsSelected;
   unsigned NumExam;
   const char *Ptr;
   long ExaCod;
   char LongStr[Cns_MAX_DECIMAL_DIGITS_LONG + 1];

   /***** Allocate memory for list of exams selected *****/
   MaxSizeListExaCodsSelected = Exams->Num * (Cns_MAX_DECIMAL_DIGITS_LONG + 1);
   if ((Exams->ExaCodsSelected = malloc (MaxSizeListExaCodsSelected + 1)) == NULL)
      Err_NotEnoughMemoryExit ();

   /***** Get parameter multiple with list of exams selected *****/
   Par_GetParMultiToText (Par_CodeStr[ParCod_Exa],Exams->ExaCodsSelected,
                          MaxSizeListExaCodsSelected);

   /***** Set which exams will be shown as selected (checkboxes on) *****/
   if (Exams->ExaCodsSelected[0])	// Some exams selected
     {
      /* Reset selection */
      for (NumExam = 0;
	   NumExam < Exams->Num;
	   NumExam++)
	 Exams->Lst[NumExam].Selected = false;
      Exams->NumSelected = 0;

      /* Set some exams as selected */
      for (Ptr = Exams->ExaCodsSelected;
	   *Ptr;
	   )
	{
	 /* Get next exam selected */
	 Par_GetNextStrUntilSeparParMult (&Ptr,LongStr,Cns_MAX_DECIMAL_DIGITS_LONG);
	 ExaCod = Str_ConvertStrCodToLongCod (LongStr);

	 /* Set each exam in *StrExaCodsSelected as selected */
	 for (NumExam = 0;
	      NumExam < Exams->Num;
	      NumExam++)
	    if (Exams->Lst[NumExam].ExaCod == ExaCod)
	      {
	       Exams->Lst[NumExam].Selected = true;
	       Exams->NumSelected++;
	       break;
	      }
	}
     }
   else					// No exams selected
     {
      /***** Set all exams as selected *****/
      for (NumExam = 0;
	   NumExam < Exams->Num;
	   NumExam++)
	 Exams->Lst[NumExam].Selected = true;
      Exams->NumSelected = Exams->Num;
     }
  }

/*****************************************************************************/
/********************** Get exam data using its code *************************/
/*****************************************************************************/

void Exa_GetExamDataByCod (struct Exa_Exam *Exam)
  {
   MYSQL_RES *mysql_res;
   MYSQL_ROW row;

   /***** Trivial check *****/
   if (Exam->ExaCod <= 0)
     {
      /* Initialize to empty exam */
      Exa_ResetExam (Exam);
      return;
     }

   /***** Get exam data from database *****/
   if (Exa_DB_GetExamDataByCod (&mysql_res,Exam->ExaCod)) // Exam found...
     {
      /* Get row */
      row = mysql_fetch_row (mysql_res);

      /* Get code of the exam (row[0])
         and code of the course (row[1]) */
      Exam->ExaCod = Str_ConvertStrCodToLongCod (row[0]);
      Exam->CrsCod = Str_ConvertStrCodToLongCod (row[1]);

      /* Get whether the exam is hidden (row[2]) */
      Exam->Hidden = (row[2][0] == 'Y');

      /* Get author of the exam (row[3]) */
      Exam->UsrCod = Str_ConvertStrCodToLongCod (row[3]);

      /* Get maximum grade (row[4]) */
      Exam->MaxGrade = Str_GetDoubleFromStr (row[4]);
      if (Exam->MaxGrade < 0.0)	// Only positive values allowed
	 Exam->MaxGrade = 0.0;

      /* Get visibility (row[5]) */
      Exam->Visibility = TstVis_GetVisibilityFromStr (row[5]);

      /* Get the title of the exam (row[6]) */
      Str_Copy (Exam->Title,row[6],sizeof (Exam->Title) - 1);

      /* Get number of sets,
             number of questions,
             number of sessions
         and number of open sessions */
      Exam->NumSets     = Exa_DB_GetNumSetsExam (Exam->ExaCod);
      Exam->NumQsts     = Exa_DB_GetNumQstsExam (Exam->ExaCod);
      Exam->NumSess     = Exa_DB_GetNumSessionsInExam (Exam->ExaCod);
      Exam->NumOpenSess = Exa_DB_GetNumOpenSessionsInExam (Exam->ExaCod);
     }
   else
      /* Initialize to empty exam */
      Exa_ResetExam (Exam);

   /* Free structure that stores the query result */
   DB_FreeMySQLResult (&mysql_res);

   if (Exam->ExaCod > 0)
     {
      /***** Get start and end times from database *****/
      if (Exa_DB_GetExamStartEnd (&mysql_res,Exam->ExaCod))
	{
	 /* Get row */
	 row = mysql_fetch_row (mysql_res);

	 /* Get start date (row[0] holds the start UTC time)
	    and end   date (row[1] holds the end   UTC time) */
	 Exam->TimeUTC[Dat_STR_TIME] = Dat_GetUNIXTimeFromStr (row[0]);
	 Exam->TimeUTC[Dat_END_TIME] = Dat_GetUNIXTimeFromStr (row[1]);
	}

      /* Free structure that stores the query result */
      DB_FreeMySQLResult (&mysql_res);
     }
   else
     {
      Exam->TimeUTC[Dat_STR_TIME] =
      Exam->TimeUTC[Dat_END_TIME] = (time_t) 0;
     }
  }

/*****************************************************************************/
/***************************** Free list of exams ****************************/
/*****************************************************************************/

void Exa_FreeListExams (struct Exa_Exams *Exams)
  {
   if (Exams->LstIsRead && Exams->Lst)
     {
      /***** Free memory used by the list of exams *****/
      free (Exams->Lst);
      Exams->Lst       = NULL;
      Exams->Num       = 0;
      Exams->LstIsRead = false;
     }
  }

/*****************************************************************************/
/*************** Ask for confirmation of removing of an exam *****************/
/*****************************************************************************/

void Exa_AskRemExam (void)
  {
   extern const char *Txt_Do_you_really_want_to_remove_the_exam_X;
   extern const char *Txt_Remove_exam;
   struct Exa_Exams Exams;

   /***** Check if I can edit exams *****/
   if (!Exa_CheckIfICanEditExams ())
      Err_NoPermissionExit ();

   /***** Reset exams context *****/
   Exa_ResetExams (&Exams);
   Exa_ResetExam (&Exams.Exam);

   /***** Get parameters *****/
   Exa_GetPars (&Exams,Exa_CHECK_EXA_COD);

   /***** Get data of the exam from database *****/
   Exa_GetExamDataByCod (&Exams.Exam);

   /***** Show question and button to remove exam *****/
   Ale_ShowAlertAndButton (ActRemExa,NULL,NULL,
                           Exa_PutPars,&Exams,
			   Btn_REMOVE_BUTTON,Txt_Remove_exam,
			   Ale_QUESTION,Txt_Do_you_really_want_to_remove_the_exam_X,
                           Exams.Exam.Title);

   /***** Show exams again *****/
   Exa_ListAllExams (&Exams);
  }

/*****************************************************************************/
/******************************* Remove an exam ******************************/
/*****************************************************************************/

void Exa_RemoveExam (void)
  {
   extern const char *Txt_Exam_X_removed;
   struct Exa_Exams Exams;

   /***** Check if I can edit exams *****/
   if (!Exa_CheckIfICanEditExams ())
      Err_NoPermissionExit ();

   /***** Reset exams context *****/
   Exa_ResetExams (&Exams);
   Exa_ResetExam (&Exams.Exam);

   /***** Get exam code *****/
   Exams.Exam.ExaCod = ParCod_GetAndCheckPar (ParCod_Exa);

   /***** Get data of the exam from database *****/
   Exa_GetExamDataByCod (&Exams.Exam);

   /***** Remove exam from all tables *****/
   Exa_RemoveExamFromAllTables (Exams.Exam.ExaCod);

   /***** Write message to show the change made *****/
   Ale_ShowAlert (Ale_SUCCESS,Txt_Exam_X_removed,
                  Exams.Exam.Title);

   /***** Show exams again *****/
   Exa_ListAllExams (&Exams);
  }

/*****************************************************************************/
/*********************** Remove exam from all tables *************************/
/*****************************************************************************/

static void Exa_RemoveExamFromAllTables (long ExaCod)
  {
   /***** Remove questions of exams prints, and exam prints, in this exam *****/
   // TODO: DO NOT REMOVE EXAMS PRINTS. Instead move them to tables of deleted prints
   Exa_DB_RemovePrintQstsFromExa (ExaCod);
   Exa_DB_RemoveAllPrintsFromExa (ExaCod);

   /***** Remove groups associated to sessions, and sessions, in this exam *****/
   Exa_DB_RemoveAllGrpsFromExa (ExaCod);
   Exa_DB_RemoveAllSessionsFromExam (ExaCod);

   /***** Remove media associated to exam questions in the exam *****/
   Exa_RemoveAllMedFilesFromStemOfAllQstsFromExam (ExaCod);
   Exa_RemoveAllMedFilesFromAnsOfAllQstsFromExam (ExaCod);

   /***** Remove the answers and the questions in sets of questions *****/
   Exa_DB_RemoveAllSetAnswersFromExam (ExaCod);
   Exa_DB_RemoveAllSetQuestionsFromExam (ExaCod);

   /***** Remove exam sets *****/
   Exa_DB_RemoveAllSetsFromExam (ExaCod);

   /***** Remove exam *****/
   Exa_DB_RemoveExam (ExaCod);
  }

/*****************************************************************************/
/*********************** Remove all exams of a course ************************/
/*****************************************************************************/

void Exa_RemoveCrsExams (long CrsCod)
  {
   /***** Remove questions of exams prints, and exam prints,
          made in the given course *****/
   // TODO: DO NOT REMOVE EXAMS PRINTS. Instead move them to tables of deleted prints
   Exa_DB_RemovePrintQstsFromCrs (CrsCod);
   Exa_DB_RemoveAllPrintsFromCrs (CrsCod);

   /***** Remove groups associated to sessions, and sessions, in this course *****/
   Exa_DB_RemoveAllGrpsFromCrs (CrsCod);
   Exa_DB_RemoveAllSessionsFromCrs (CrsCod);

   /***** Remove media associated to test questions in the course *****/
   Exa_RemoveAllMedFilesFromStemOfAllQstsFromCrs (CrsCod);
   Exa_RemoveAllMedFilesFromAnsOfAllQstsFromCrs (CrsCod);

   /***** Remove the answers and the questions in sets of questions *****/
   Exa_DB_RemoveAllSetAnswersFromCrs (CrsCod);
   Exa_DB_RemoveAllSetQuestionsFromCrs (CrsCod);

   /***** Remove the sets of questions in exams *****/
   Exa_DB_RemoveAllSetsFromCrs (CrsCod);

   /***** Remove the exams *****/
   Exa_DB_RemoveAllExamsFromCrs (CrsCod);
  }

/*****************************************************************************/
/********* Remove all media associated to stems of an exam questions *********/
/*****************************************************************************/

static void Exa_RemoveAllMedFilesFromStemOfAllQstsFromExam (long ExaCod)
  {
   MYSQL_RES *mysql_res;
   unsigned NumMedia;

   /***** Get media codes associated to stems of an exam questions *****/
   NumMedia = Exa_DB_GetAllMediaFomStemOfAllQstsFromExam (&mysql_res,ExaCod);

   /***** Go over result removing media files *****/
   Med_RemoveMediaFromAllRows (NumMedia,mysql_res);

   /***** Free structure that stores the query result *****/
   DB_FreeMySQLResult (&mysql_res);
  }

/*****************************************************************************/
/** Remove all media associated to stems of all exam questions in a course ***/
/*****************************************************************************/

static void Exa_RemoveAllMedFilesFromStemOfAllQstsFromCrs (long CrsCod)
  {
   MYSQL_RES *mysql_res;
   unsigned NumMedia;

   /***** Get media codes associated to stems of exam questions from database *****/
   NumMedia = Exa_DB_GetAllMediaFomStemOfAllQstsFromCrs (&mysql_res,CrsCod);

   /***** Go over result removing media files *****/
   Med_RemoveMediaFromAllRows (NumMedia,mysql_res);

   /***** Free structure that stores the query result *****/
   DB_FreeMySQLResult (&mysql_res);
  }

/*****************************************************************************/
/** Remove media associated to all answers of all exam questions in an exam **/
/*****************************************************************************/

static void Exa_RemoveAllMedFilesFromAnsOfAllQstsFromExam (long ExaCod)
  {
   MYSQL_RES *mysql_res;
   unsigned NumMedia;

   /***** Get names of media files associated to answers of exam questions from database *****/
   NumMedia = Exa_DB_GetAllMediaFromAnsOfAllQstsFromExam (&mysql_res,ExaCod);

   /***** Go over result removing media files *****/
   Med_RemoveMediaFromAllRows (NumMedia,mysql_res);

   /***** Free structure that stores the query result *****/
   DB_FreeMySQLResult (&mysql_res);
  }

/*****************************************************************************/
/* Remove media associated to all answers of all exam questions in a course **/
/*****************************************************************************/

static void Exa_RemoveAllMedFilesFromAnsOfAllQstsFromCrs (long CrsCod)
  {
   MYSQL_RES *mysql_res;
   unsigned NumMedia;

   /***** Get names of media files associated to answers of exam questions from database *****/
   NumMedia = Exa_DB_GetAllMediaFromAnsOfAllQstsFromCrs (&mysql_res,CrsCod);

   /***** Go over result removing media files *****/
   Med_RemoveMediaFromAllRows (NumMedia,mysql_res);

   /***** Free structure that stores the query result *****/
   DB_FreeMySQLResult (&mysql_res);
  }

/*****************************************************************************/
/******************************** Hide an exam ******************************/
/*****************************************************************************/

void Exa_HideExam (void)
  {
   Exa_HideUnhideExam (true);
  }

/*****************************************************************************/
/******************************* Unhide an exam ******************************/
/*****************************************************************************/

void Exa_UnhideExam (void)
  {
   Exa_HideUnhideExam (false);
  }

/*****************************************************************************/
/******************************* Unhide an exam ******************************/
/*****************************************************************************/

static void Exa_HideUnhideExam (bool Hide)
  {
   struct Exa_Exams Exams;

   /***** Check if I can edit exams *****/
   if (!Exa_CheckIfICanEditExams ())
      Err_NoPermissionExit ();

   /***** Reset exams context *****/
   Exa_ResetExams (&Exams);
   Exa_ResetExam (&Exams.Exam);

   /***** Get parameters *****/
   Exa_GetPars (&Exams,Exa_CHECK_EXA_COD);

   /***** Get data of the exam from database *****/
   Exa_GetExamDataByCod (&Exams.Exam);

   /***** Unhide exam *****/
   Exa_DB_HideOrUnhideExam (Exams.Exam.ExaCod,Hide);

   /***** Show exams again *****/
   Exa_ListAllExams (&Exams);
  }

/*****************************************************************************/
/*************** Request the creation or edition of an exam ******************/
/*****************************************************************************/

void Exa_ReqCreatOrEditExam (void)
  {
   struct Exa_Exams Exams;
   struct ExaSet_Set Set;
   bool ItsANewExam;

   /***** Check if I can edit exams *****/
   if (!Exa_CheckIfICanEditExams ())
      Err_NoPermissionExit ();

   /***** Reset exams context *****/
   Exa_ResetExams (&Exams);
   Exa_ResetExam (&Exams.Exam);
   ExaSet_ResetSet (&Set);

   /***** Get parameters *****/
   Exa_GetPars (&Exams,Exa_DONT_CHECK_EXA_COD);
   ItsANewExam = (Exams.Exam.ExaCod <= 0);

   /***** Get exam data *****/
   if (ItsANewExam)
      /* Initialize to empty exam */
      Exa_ResetExam (&Exams.Exam);
   else
      /* Get exam data from database */
      Exa_GetExamDataByCod (&Exams.Exam);

   /***** Put form to create/edit an exam and show sets *****/
   Exa_PutFormsOneExam (&Exams,&Set,ItsANewExam);
  }

/*****************************************************************************/
/******************** Put forms to create/edit an exam ***********************/
/*****************************************************************************/

void Exa_PutFormsOneExam (struct Exa_Exams *Exams,
			  struct ExaSet_Set *Set,
			  bool ItsANewExam)
  {
   char Txt[Cns_MAX_BYTES_TEXT + 1];

   /***** Initialize text / get text from database *****/
   if (ItsANewExam)
      Txt[0] = '\0';
   else
      Exa_DB_GetExamTxt (Exams->Exam.ExaCod,Txt);

   /***** Put form to create/edit an exam *****/
   Exa_PutFormEditionExam (Exams,Txt,ItsANewExam);

   /***** Show other lists *****/
   if (ItsANewExam)
      /* Show exams again */
      Exa_ListAllExams (Exams);
   else
      /* Show list of sets */
      ExaSet_ListExamSets (Exams,Set);
  }

/*****************************************************************************/
/********************* Put a form to create/edit an exam **********************/
/*****************************************************************************/

void Exa_PutFormEditionExam (struct Exa_Exams *Exams,
			     char Txt[Cns_MAX_BYTES_TEXT + 1],
			     bool ItsANewExam)
  {
   extern const char *Hlp_ASSESSMENT_Exams_new_exam;
   extern const char *Hlp_ASSESSMENT_Exams_edit_exam;
   extern const char *Txt_New_exam;
   extern const char *Txt_Edit_exam;
   extern const char *Txt_Title;
   extern const char *Txt_Maximum_grade;
   extern const char *Txt_Result_visibility;
   extern const char *Txt_Description;
   extern const char *Txt_Create_exam;
   extern const char *Txt_Save_changes;

   /***** Begin form *****/
   Frm_BeginForm (ItsANewExam ? ActNewExa :
				ActChgExa);
      Exa_PutPars (Exams);

      /***** Begin box and table *****/
      if (ItsANewExam)
	 Box_BoxTableBegin (NULL,Txt_New_exam,
			    NULL,NULL,
			    Hlp_ASSESSMENT_Exams_new_exam,Box_NOT_CLOSABLE,2);
      else
	 Box_BoxTableBegin (NULL,
			    Exams->Exam.Title[0] ? Exams->Exam.Title :
						   Txt_Edit_exam,
			    NULL,NULL,
			    Hlp_ASSESSMENT_Exams_edit_exam,Box_NOT_CLOSABLE,2);

      /***** Exam title *****/
      HTM_TR_Begin (NULL);

	 /* Label */
	 Frm_LabelColumn ("RT","Title",Txt_Title);

	 /* Data */
	 HTM_TD_Begin ("class=\"LT\"");
	    HTM_INPUT_TEXT ("Title",Exa_MAX_CHARS_TITLE,Exams->Exam.Title,
			    HTM_DONT_SUBMIT_ON_CHANGE,
			    "id=\"Title\""
			    " class=\"TITLE_DESCRIPTION_WIDTH INPUT_%s\""
			    " required=\"required\"",
			    The_GetSuffix ());
	 HTM_TD_End ();

      HTM_TR_End ();

      /***** Maximum grade *****/
      HTM_TR_Begin (NULL);

	 HTM_TD_Begin ("class=\"RM FORM_IN_%s\"",The_GetSuffix ());
	    HTM_TxtColon (Txt_Maximum_grade);
	 HTM_TD_End ();

	 HTM_TD_Begin ("class=\"LM\"");
	    HTM_INPUT_FLOAT ("MaxGrade",0.0,DBL_MAX,0.01,Exams->Exam.MaxGrade,false,
			     " class=\"INPUT_%s\" required=\"required\"",
			     The_GetSuffix ());
	 HTM_TD_End ();

      HTM_TR_End ();

      /***** Visibility of results *****/
      HTM_TR_Begin (NULL);

	 HTM_TD_Begin ("class=\"RT %s\"",The_GetSuffix ());
	    HTM_TxtColon (Txt_Result_visibility);
	 HTM_TD_End ();

	 HTM_TD_Begin ("class=\"LB\"");
	    TstVis_PutVisibilityCheckboxes (Exams->Exam.Visibility);
	 HTM_TD_End ();

      HTM_TR_End ();

      /***** Exam text *****/
      HTM_TR_Begin (NULL);

      /* Label */
      Frm_LabelColumn ("RT","Txt",Txt_Description);

      /* Data */
      HTM_TD_Begin ("class=\"LT\"");
	 HTM_TEXTAREA_Begin ("id=\"Txt\" name=\"Txt\" rows=\"5\""
			     " class=\"TITLE_DESCRIPTION_WIDTH INPUT_%s\"",
			     The_GetSuffix ());
	    HTM_Txt (Txt);
	 HTM_TEXTAREA_End ();
      HTM_TD_End ();

      HTM_TR_End ();

      /***** End table, send button and end box *****/
      if (ItsANewExam)
	 Box_BoxTableWithButtonEnd (Btn_CREATE_BUTTON,Txt_Create_exam);
      else
	 Box_BoxTableWithButtonEnd (Btn_CONFIRM_BUTTON,Txt_Save_changes);

   /***** End form *****/
   Frm_EndForm ();
  }

/*****************************************************************************/
/********************** Receive form to create a new exam ********************/
/*****************************************************************************/

void Exa_ReceiveFormExam (void)
  {
   struct Exa_Exams Exams;
   struct ExaSet_Set Set;
   bool ItsANewExam;
   char Txt[Cns_MAX_BYTES_TEXT + 1];

   /***** Check if I can edit exams *****/
   if (!Exa_CheckIfICanEditExams ())
      Err_NoPermissionExit ();

   /***** Reset exams context *****/
   Exa_ResetExams (&Exams);
   Exa_ResetExam (&Exams.Exam);
   ExaSet_ResetSet (&Set);

   /***** Get parameters *****/
   Exa_GetPars (&Exams,Exa_DONT_CHECK_EXA_COD);
   ItsANewExam = (Exams.Exam.ExaCod <= 0);

   /***** Get all current exam data from database *****/
   // Some data, not received from form,
   // are necessary to show exam and sets of questions again
   if (!ItsANewExam)
      Exa_GetExamDataByCod (&Exams.Exam);

   /***** Overwrite some exam data with the data received from form *****/
   Exa_ReceiveExamFieldsFromForm (&Exams.Exam,Txt);
   if (Exa_CheckExamFieldsReceivedFromForm (&Exams.Exam))
     {
      /***** Create a new exam or update an existing one *****/
      if (ItsANewExam)
	{
	 Exa_CreateExam (&Exams.Exam,Txt);	// Add new exam to database
	 ItsANewExam = false;
	}
      else
	 Exa_UpdateExam (&Exams.Exam,Txt);	// Update exam data in database
     }

   /***** Show current exam and its sets *****/
   Exa_PutFormsOneExam (&Exams,&Set,
			ItsANewExam);
  }

static void Exa_ReceiveExamFieldsFromForm (struct Exa_Exam *Exam,
				           char Txt[Cns_MAX_BYTES_TEXT + 1])
  {
   char MaxGradeStr[64];

   /***** Get exam title *****/
   Par_GetParText ("Title",Exam->Title,Exa_MAX_BYTES_TITLE);

   /***** Get maximum grade *****/
   Par_GetParText ("MaxGrade",MaxGradeStr,sizeof (MaxGradeStr) - 1);
   Exam->MaxGrade = Str_GetDoubleFromStr (MaxGradeStr);
   if (Exam->MaxGrade < 0.0)	// Only positive values allowed
      Exam->MaxGrade = 0.0;

   /***** Get visibility *****/
   Exam->Visibility = TstVis_GetVisibilityFromForm ();

   /***** Get exam text *****/
   Par_GetParHTML ("Txt",Txt,Cns_MAX_BYTES_TEXT);	// Store in HTML format (not rigorous)
  }

static bool Exa_CheckExamFieldsReceivedFromForm (const struct Exa_Exam *Exam)
  {
   extern const char *Txt_Already_existed_an_exam_with_the_title_X;
   bool NewExamIsCorrect;

   /***** Check if title is correct *****/
   NewExamIsCorrect = true;
   if (Exam->Title[0])	// If there's an exam title
     {
      /* If title of exam was in database... */
      if (Exa_DB_CheckIfSimilarExamExists (Gbl.Hierarchy.Crs.CrsCod,
                                           Exam->ExaCod,Exam->Title))
	{
	 NewExamIsCorrect = false;
	 Ale_ShowAlert (Ale_WARNING,Txt_Already_existed_an_exam_with_the_title_X,
			Exam->Title);
	}
     }
   else	// If there is not an exam title
     {
      NewExamIsCorrect = false;
      Ale_CreateAlertYouMustSpecifyTheTitle ();
     }

   return NewExamIsCorrect;
  }

/*****************************************************************************/
/**************************** Create a new exam ******************************/
/*****************************************************************************/

static void Exa_CreateExam (struct Exa_Exam *Exam,const char *Txt)
  {
   extern const char *Txt_Created_new_exam_X;

   /***** Create a new exam *****/
   Exam->ExaCod = Exa_DB_CreateExam (Exam,Txt);

   /***** Write success message *****/
   Ale_ShowAlert (Ale_SUCCESS,Txt_Created_new_exam_X,
                  Exam->Title);
  }

/*****************************************************************************/
/************************* Update an existing exam *************************/
/*****************************************************************************/

static void Exa_UpdateExam (struct Exa_Exam *Exam,const char *Txt)
  {
   extern const char *Txt_The_exam_has_been_modified;

   /***** Update the data of the exam *****/
   Exa_DB_UpdateExam (Exam,Txt);

   /***** Write success message *****/
   Ale_ShowAlert (Ale_SUCCESS,Txt_The_exam_has_been_modified);
  }

/*****************************************************************************/
/********** Get number of sessions and check is edition is possible **********/
/*****************************************************************************/
// Before calling this function, number of sessions must be calculated

bool Exa_CheckIfEditable (const struct Exa_Exam *Exam)
  {
   if (Exa_CheckIfICanEditExams ())
      /***** Questions are editable only if exam has no sessions *****/
      return Exam->NumSess == 0;	// Exams with sessions should not be edited
   else
      return false;	// Questions are not editable
  }

/*****************************************************************************/
/*************************** Show stats about exams **************************/
/*****************************************************************************/

void Exa_GetAndShowExamsStats (void)
  {
   extern const char *Hlp_ANALYTICS_Figures_exams;
   extern const char *Txt_FIGURE_TYPES[Fig_NUM_FIGURES];
   extern const char *Txt_Number_of_BR_exams;
   extern const char *Txt_Number_of_BR_courses_with_BR_exams;
   extern const char *Txt_Average_number_BR_of_exams_BR_per_course;
   unsigned NumExams;
   unsigned NumCoursesWithExams = 0;
   double NumExamsPerCourse = 0.0;

   /***** Get the number of exams from this location *****/
   if ((NumExams = Exa_DB_GetNumExams (Gbl.Scope.Current)))
      if ((NumCoursesWithExams = Exa_DB_GetNumCoursesWithExams (Gbl.Scope.Current)) != 0)
         NumExamsPerCourse = (double) NumExams / (double) NumCoursesWithExams;

   /***** Begin box and table *****/
   Box_BoxTableBegin (NULL,Txt_FIGURE_TYPES[Fig_EXAMS],
                      NULL,NULL,
                      Hlp_ANALYTICS_Figures_exams,Box_NOT_CLOSABLE,2);

      /***** Write table heading *****/
      HTM_TR_Begin (NULL);
	 HTM_TH (Txt_Number_of_BR_exams                      ,HTM_HEAD_RIGHT);
	 HTM_TH (Txt_Number_of_BR_courses_with_BR_exams      ,HTM_HEAD_RIGHT);
	 HTM_TH (Txt_Average_number_BR_of_exams_BR_per_course,HTM_HEAD_RIGHT);
      HTM_TR_End ();

      /***** Write number of exams *****/
      HTM_TR_Begin (NULL);

	 HTM_TD_Begin ("class=\"RM DAT_%s\"",The_GetSuffix ());
	    HTM_Unsigned (NumExams);
	 HTM_TD_End ();

	 HTM_TD_Begin ("class=\"RM DAT_%s\"",The_GetSuffix ());
	    HTM_Unsigned (NumCoursesWithExams);
	 HTM_TD_End ();

	 HTM_TD_Begin ("class=\"RM DAT_%s\"",The_GetSuffix ());
	    HTM_Double2Decimals (NumExamsPerCourse);
	 HTM_TD_End ();

      HTM_TR_End ();

   /***** End table and box *****/
   Box_BoxTableEnd ();
  }
