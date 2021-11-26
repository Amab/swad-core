// swad_test.c: self-assessment tests

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
/*********************************** Headers *********************************/
/*****************************************************************************/

#define _GNU_SOURCE 		// For asprintf
#include <limits.h>		// For UINT_MAX
#include <linux/limits.h>	// For PATH_MAX
#include <mysql/mysql.h>	// To access MySQL databases
#include <stdbool.h>		// For boolean type
#include <stddef.h>		// For NULL
#include <stdio.h>		// For asprintf
#include <stdlib.h>		// For exit, system, malloc, free, etc
#include <string.h>		// For string functions
#include <sys/stat.h>		// For mkdir
#include <sys/types.h>		// For mkdir

#include "swad_action.h"
#include "swad_box.h"
#include "swad_database.h"
#include "swad_error.h"
#include "swad_exam_set.h"
#include "swad_figure.h"
#include "swad_form.h"
#include "swad_global.h"
#include "swad_hierarchy_level.h"
#include "swad_HTML.h"
#include "swad_ID.h"
#include "swad_language.h"
#include "swad_match.h"
#include "swad_media.h"
#include "swad_parameter.h"
#include "swad_question.h"
#include "swad_question_database.h"
#include "swad_question_import.h"
#include "swad_tag_database.h"
#include "swad_test.h"
#include "swad_test_config.h"
#include "swad_test_database.h"
#include "swad_test_print.h"
#include "swad_test_visibility.h"
#include "swad_theme.h"
#include "swad_user.h"
#include "swad_xml.h"

/*****************************************************************************/
/************** External global variables from others modules ****************/
/*****************************************************************************/

extern struct Globals Gbl;

/*****************************************************************************/
/***************************** Private prototypes ****************************/
/*****************************************************************************/

static void Tst_ShowFormRequestTest (struct Qst_Questions *Questions);

static bool Tst_CheckIfNextTstAllowed (void);

static void Tst_GetQuestionsForNewTest (struct Qst_Questions *Questions,
                                              struct TstPrn_Print *Print);
static void Tst_GenerateChoiceIndexes (struct TstPrn_PrintedQuestion *PrintedQuestion,
				       bool Shuffle);

static unsigned Tst_GetParamNumTst (void);
static unsigned Tst_GetParamNumQsts (void);

/*****************************************************************************/
/********************* Request a self-assessment test ************************/
/*****************************************************************************/

void Tst_RequestTest (void)
  {
   struct Qst_Questions Questions;

   /***** Create questions *****/
   Qst_Constructor (&Questions);

   /***** Show form to generate a self-assessment test *****/
   Tst_ShowFormRequestTest (&Questions);

   /***** Destroy questions *****/
   Qst_Destructor (&Questions);
  }

/*****************************************************************************/
/*************** Show form to generate a self-assessment test ****************/
/*****************************************************************************/

static void Tst_ShowFormRequestTest (struct Qst_Questions *Questions)
  {
   extern const char *Hlp_ASSESSMENT_Tests;
   extern const char *Txt_Test;
   extern const char *Txt_Number_of_questions;
   extern const char *Txt_Generate_test;
   extern const char *Txt_No_test_questions;
   MYSQL_RES *mysql_res;

   /***** Read test configuration from database *****/
   TstCfg_GetConfig ();

   /***** Begin box *****/
   Box_BoxBegin (NULL,Txt_Test,
                 Tst_PutIconsTests,NULL,
                 Hlp_ASSESSMENT_Tests,Box_NOT_CLOSABLE);

      /***** Get tags *****/
      if ((Questions->Tags.Num = Tag_DB_GetEnabledTagsFromCrs (&mysql_res,
                                                               Gbl.Hierarchy.Crs.CrsCod)) != 0)
	{
	 /***** Check if minimum date-time of next access to test is older than now *****/
	 if (Tst_CheckIfNextTstAllowed ())
	   {
	    Frm_BeginForm (ActSeeTst);

	       HTM_TABLE_BeginPadding (2);

		  /***** Selection of tags *****/
		  Tag_ShowFormSelTags (&Questions->Tags,mysql_res,true);

		  /***** Selection of types of answers *****/
		  Qst_ShowFormAnswerTypes (&Questions->AnswerTypes);

		  /***** Number of questions to generate ****/
		  HTM_TR_Begin (NULL);

		     /* Label */
		     Frm_LabelColumn ("RT","NumQst",Txt_Number_of_questions);

		     /* Data */
		     HTM_TD_Begin ("class=\"LT\"");
			HTM_INPUT_LONG ("NumQst",
					(long) TstCfg_GetConfigMin (),
					(long) TstCfg_GetConfigMax (),
					(long) TstCfg_GetConfigDef (),
					HTM_DONT_SUBMIT_ON_CHANGE,
					TstCfg_GetConfigMin () == TstCfg_GetConfigMax (),
					"id=\"NumQst\"");
		     HTM_TD_End ();

		  HTM_TR_End ();

	       HTM_TABLE_End ();

	       /***** Send button *****/
	       Btn_PutConfirmButton (Txt_Generate_test);
	    Frm_EndForm ();
	   }
	}
      else
	{
	 /***** Warning message *****/
	 Ale_ShowAlert (Ale_INFO,Txt_No_test_questions);

	 /***** Button to create a new question *****/
	 if (Qst_CheckIfICanEditQsts ())
	    Qst_PutButtonToAddQuestion ();
	}

      /***** Free structure that stores the query result *****/
      DB_FreeMySQLResult (&mysql_res);

   /***** End box *****/
   Box_BoxEnd ();
  }

/*****************************************************************************/
/********************** Generate self-assessment test ************************/
/*****************************************************************************/

void Tst_ShowNewTest (void)
  {
   extern const char *Txt_No_questions_found_matching_your_search_criteria;
   struct Qst_Questions Questions;
   struct TstPrn_Print Print;
   unsigned NumPrintsGeneratedByMe;

   /***** Create test *****/
   Qst_Constructor (&Questions);

   /***** Read test configuration from database *****/
   TstCfg_GetConfig ();

   if (Tst_CheckIfNextTstAllowed ())
     {
      /***** Check that all parameters used to generate a test are valid *****/
      if (Tst_GetParamsTst (&Questions,Tst_SHOW_TEST_TO_ANSWER))	// Get parameters from form
        {
         /***** Get questions *****/
	 TstPrn_ResetPrint (&Print);
	 Tst_GetQuestionsForNewTest (&Questions,&Print);
         if (Print.NumQsts.All)
           {
            /***** Increase number of exams generated (answered or not) by me *****/
            Tst_DB_IncreaseNumMyPrints ();
            NumPrintsGeneratedByMe = TstPrn_GetNumPrintsGeneratedByMe ();

	    /***** Create new test print in database *****/
	    Print.PrnCod = Tst_DB_CreatePrint (Print.NumQsts.All);
	    TstPrn_ComputeScoresAndStoreQuestionsOfPrint (&Print,
	                                                  false);	// Don't update question score

            /***** Show test print to be answered *****/
            TstPrn_ShowTestPrintToFillIt (&Print,NumPrintsGeneratedByMe,TstPrn_REQUEST);

            /***** Update date-time of my next allowed access to test *****/
            if (Gbl.Usrs.Me.Role.Logged == Rol_STD)
               Tst_DB_UpdateLastAccTst (Questions.NumQsts);
           }
         else	// No questions found
           {
            Ale_ShowAlert (Ale_INFO,Txt_No_questions_found_matching_your_search_criteria);
            Tst_ShowFormRequestTest (&Questions);	// Show the form again
           }
        }
      else
         Tst_ShowFormRequestTest (&Questions);		// Show the form again
     }

   /***** Destroy test *****/
   Qst_Destructor (&Questions);
  }

/*****************************************************************************/
/** Receive the draft of a test print already (total or partially) answered **/
/*****************************************************************************/

void Tst_ReceiveTestDraft (void)
  {
   extern const char *Txt_The_test_X_has_already_been_assessed_previously;
   extern const char *Txt_Please_review_your_answers_before_submitting_the_exam;
   unsigned NumTst;
   struct TstPrn_Print Print;

   /***** Read test configuration from database *****/
   TstCfg_GetConfig ();

   /***** Get basic parameters of the exam *****/
   /* Get test print code from form */
   TstPrn_ResetPrint (&Print);
   if ((Print.PrnCod = TstPrn_GetParamPrnCod ()) <= 0)
      Err_WrongTestExit ();

   /* Get number of this test from form */
   NumTst = Tst_GetParamNumTst ();

   /***** Get test print from database *****/
   TstPrn_GetPrintDataByPrnCod (&Print);

   /****** Get test status in database for this session-course-num.test *****/
   if (Print.Sent)
      Ale_ShowAlert (Ale_WARNING,Txt_The_test_X_has_already_been_assessed_previously,
	             NumTst);
   else // Print not yet sent
     {
      /***** Get test print questions from database *****/
      if (!TstPrn_GetPrintQuestionsFromDB (&Print))
         Err_WrongExamExit ();

      /***** Get answers from form to assess a test *****/
      TstPrn_GetAnswersFromForm (&Print);

      /***** Update test print in database *****/
      TstPrn_ComputeScoresAndStoreQuestionsOfPrint (&Print,
						    false);	// Don't update question score
      Tst_DB_UpdatePrint (&Print);

      /***** Show question and button to send the test *****/
      /* Begin alert */
      Ale_ShowAlert (Ale_WARNING,Txt_Please_review_your_answers_before_submitting_the_exam);

      /* Show the same test print to be answered */
      TstPrn_ShowTestPrintToFillIt (&Print,NumTst,TstPrn_CONFIRM);
     }
  }

/*****************************************************************************/
/******************************** Assess a test ******************************/
/*****************************************************************************/

void Tst_AssessTest (void)
  {
   extern const char *Hlp_ASSESSMENT_Tests;
   extern const char *Txt_Result;
   extern const char *Txt_Test_No_X_that_you_make_in_this_course;
   extern const char *Txt_Score;
   extern const char *Txt_Grade;
   extern const char *Txt_The_test_X_has_already_been_assessed_previously;
   unsigned NumTst;
   struct TstPrn_Print Print;

   /***** Read test configuration from database *****/
   TstCfg_GetConfig ();

   /***** Get basic parameters of the exam *****/
   /* Get test print code from form */
   TstPrn_ResetPrint (&Print);
   if ((Print.PrnCod = TstPrn_GetParamPrnCod ()) <= 0)
      Err_WrongTestExit ();

   /* Get number of this test from form */
   NumTst = Tst_GetParamNumTst ();

   /***** Get test print from database *****/
   TstPrn_GetPrintDataByPrnCod (&Print);

   /****** Get test status in database for this session-course-num.test *****/
   if (Print.Sent)
      Ale_ShowAlert (Ale_WARNING,Txt_The_test_X_has_already_been_assessed_previously,
		     NumTst);
   else	// Print not yet sent
     {
      /***** Get test print questions from database *****/
      if (!TstPrn_GetPrintQuestionsFromDB (&Print))
         Err_WrongExamExit ();

      /***** Get answers from form to assess a test *****/
      TstPrn_GetAnswersFromForm (&Print);

      /***** Get if test print will be visible by teachers *****/
      Print.Sent          = true;	// The exam has been finished and sent by student
      Print.AllowTeachers = Par_GetParToBool ("AllowTchs");

      /***** Update test print in database *****/
      TstPrn_ComputeScoresAndStoreQuestionsOfPrint (&Print,
						    Gbl.Usrs.Me.Role.Logged == Rol_STD);	// Update question score?
      Tst_DB_UpdatePrint (&Print);

      /***** Begin box *****/
      Box_BoxBegin (NULL,Txt_Result,
		    NULL,NULL,
		    Hlp_ASSESSMENT_Tests,Box_NOT_CLOSABLE);
	 Lay_WriteHeaderClassPhoto (false,false,
				    Gbl.Hierarchy.Ins.InsCod,
				    Gbl.Hierarchy.Deg.DegCod,
				    Gbl.Hierarchy.Crs.CrsCod);

	 /***** Header *****/
	 if (Gbl.Usrs.Me.IBelongToCurrentCrs)
	   {
	    HTM_DIV_Begin ("class=\"TEST_SUBTITLE\"");
	       HTM_TxtF (Txt_Test_No_X_that_you_make_in_this_course,NumTst);
	    HTM_DIV_End ();
	   }

	 /***** Write answers and solutions *****/
	 TstPrn_ShowPrintAfterAssess (&Print);

	 /***** Write total score and grade *****/
	 if (TstVis_IsVisibleTotalScore (TstCfg_GetConfigVisibility ()))
	   {
	    HTM_DIV_Begin ("class=\"DAT_N_BOLD CM\"");
	       HTM_TxtColonNBSP (Txt_Score);
	       HTM_Double2Decimals (Print.Score);
	       HTM_BR ();
	       HTM_TxtColonNBSP (Txt_Grade);
	       TstPrn_ComputeAndShowGrade (Print.NumQsts.All,Print.Score,Tst_SCORE_MAX);
	    HTM_DIV_End ();
	   }

      /***** End box *****/
      Box_BoxEnd ();
     }
  }

/*****************************************************************************/
/************** Check minimum date-time of next access to test ***************/
/*****************************************************************************/
// Return true if allowed date-time of next access to test is older than now

static bool Tst_CheckIfNextTstAllowed (void)
  {
   extern const char *Hlp_ASSESSMENT_Tests;
   extern const char *Txt_You_can_not_take_a_new_test_until;
   MYSQL_RES *mysql_res;
   MYSQL_ROW row;
   long NumSecondsFromNowToNextAccTst = -1L;	// Access allowed when this number <= 0
   time_t TimeNextTestUTC = (time_t) 0;

   /***** Teachers and superusers are allowed to do all tests they want *****/
   if (Gbl.Usrs.Me.Role.Logged == Rol_TCH ||
       Gbl.Usrs.Me.Role.Logged == Rol_SYS_ADM)
      return true;

   /***** Get date of next allowed access to test from database *****/
   if (Tst_DB_GetDateNextTstAllowed (&mysql_res))
     {
      /* Get seconds from now to next access to test (row[0]) */
      row = mysql_fetch_row (mysql_res);
      if (row[0])
         if (sscanf (row[0],"%ld",&NumSecondsFromNowToNextAccTst) == 1)
            /* Time UTC of next access allowed (row[1]) */
            TimeNextTestUTC = Dat_GetUNIXTimeFromStr (row[1]);
     }
   else
      Err_WrongDateExit ();

   /***** Free structure that stores the query result *****/
   DB_FreeMySQLResult (&mysql_res);

   /***** Check if access is allowed *****/
   if (NumSecondsFromNowToNextAccTst > 0)
     {
      /***** Write warning *****/
      Ale_ShowAlert (Ale_WARNING,"%s:<br />"
	                         "<span id=\"date_next_test\"></span>."
				 "<script type=\"text/javascript\">"
				 "writeLocalDateHMSFromUTC('date_next_test',%ld,"
				 "%u,',&nbsp;',%u,true,true,true,0x7);"
				 "</script>",
		     Txt_You_can_not_take_a_new_test_until,
		     (long) TimeNextTestUTC,
		     (unsigned) Gbl.Prefs.DateFormat,
		     (unsigned) Gbl.Prefs.Language);
      return false;
     }
   return true;
  }

/*****************************************************************************/
/********************* Put contextual icons in tests *************************/
/*****************************************************************************/

void Tst_PutIconsTests (__attribute__((unused)) void *Args)
  {
   switch (Gbl.Usrs.Me.Role.Logged)
     {
      case Rol_STD:
         /***** Put icon to view test results *****/
	 Ico_PutContextualIconToShowResults (ActReqSeeMyTstRes,NULL,
					     NULL,NULL);
	 break;
      case Rol_NET:
      case Rol_TCH:
      case Rol_SYS_ADM:
	 /***** Put icon to go to test configuration *****/
	 Ico_PutContextualIconToConfigure (ActCfgTst,
					   NULL,NULL);

	 /***** Put icon to edit tags *****/
	 Tag_PutIconToEditTags ();

         /***** Put icon to view test results *****/
	 Ico_PutContextualIconToShowResults (ActReqSeeUsrTstRes,NULL,
					     NULL,NULL);
	 break;
      default:
	 break;
     }

   /***** Put icon to show a figure *****/
   Fig_PutIconToShowFigure (Fig_TESTS);
  }

/*****************************************************************************/
/************** Get questions for a new test from the database ***************/
/*****************************************************************************/

#define Qst_MAX_BYTES_QUERY_QUESTIONS (16 * 1024 - 1)

static void Tst_GetQuestionsForNewTest (struct Qst_Questions *Questions,
                                        struct TstPrn_Print *Print)
  {
   MYSQL_RES *mysql_res;
   MYSQL_ROW row;
   Qst_AnswerType_t AnswerType;
   bool Shuffle;
   unsigned QstInd;

   /***** Trivial check: number of questions *****/
   if (Questions->NumQsts == 0 ||
       Questions->NumQsts > TstCfg_MAX_QUESTIONS_PER_TEST)
      Err_ShowErrorAndExit ("Wrong number of questions.");

   /***** Get questions and answers from database *****/
   Print->NumQsts.All =
   Questions->NumQsts = Qst_DB_GetQstsForNewTestPrint (&mysql_res,Questions);

   for (QstInd = 0;
	QstInd < Print->NumQsts.All;
	QstInd++)
     {
      /* Get question row */
      row = mysql_fetch_row (mysql_res);
      /*
      QstCod	row[0]
      AnsType	row[1]
      Shuffle	row[2]
      */

      /* Get question code (row[0]) */
      if ((Print->PrintedQuestions[QstInd].QstCod = Str_ConvertStrCodToLongCod (row[0])) <= 0)
	 Err_ShowErrorAndExit ("Wrong code of question.");

      /* Get answer type (row[1]) */
      AnswerType = Qst_ConvertFromStrAnsTypDBToAnsTyp (row[1]);

      /* Get shuffle (row[2]) */
      Shuffle = (row[2][0] == 'Y');

      /* Set indexes of answers */
      switch (AnswerType)
	{
	 case Qst_ANS_INT:
	 case Qst_ANS_FLOAT:
	 case Qst_ANS_TRUE_FALSE:
	 case Qst_ANS_TEXT:
	    Print->PrintedQuestions[QstInd].StrIndexes[0] = '\0';
	    break;
	 case Qst_ANS_UNIQUE_CHOICE:
	 case Qst_ANS_MULTIPLE_CHOICE:
            /* If answer type is unique or multiple option,
               generate indexes of answers depending on shuffle */
	    Tst_GenerateChoiceIndexes (&Print->PrintedQuestions[QstInd],Shuffle);
	    break;
	 default:
	    break;
	}

      /* Reset user's answers.
         Initially user has not answered the question ==> initially all answers will be blank.
         If the user does not confirm the submission of their exam ==>
         ==> the exam may be half filled ==> the answers displayed will be those selected by the user. */
      Print->PrintedQuestions[QstInd].StrAnswers[0] = '\0';
     }

   /***** Get if test print will be visible by teachers *****/
   Print->AllowTeachers = Par_GetParToBool ("AllowTchs");
  }

/*****************************************************************************/
/*************** Generate choice indexes depending on shuffle ****************/
/*****************************************************************************/

static void Tst_GenerateChoiceIndexes (struct TstPrn_PrintedQuestion *PrintedQuestion,
				       bool Shuffle)
  {
   struct Qst_Question Question;
   unsigned NumOpt;
   MYSQL_RES *mysql_res;
   MYSQL_ROW row;
   unsigned Index;
   bool ErrorInIndex;
   char StrInd[1 + Cns_MAX_DECIMAL_DIGITS_UINT + 1];

   /***** Create test question *****/
   Qst_QstConstructor (&Question);
   Question.QstCod = PrintedQuestion->QstCod;

   /***** Get answers of question from database *****/
   Question.Answer.NumOptions = Qst_DB_GetDataOfAnswers (&mysql_res,Question.QstCod,Shuffle);
   /*
   row[0] AnsInd
   row[1] Answer
   row[2] Feedback
   row[3] MedCod
   row[4] Correct
   */

   for (NumOpt = 0;
	NumOpt < Question.Answer.NumOptions;
	NumOpt++)
     {
      /***** Get next answer *****/
      row = mysql_fetch_row (mysql_res);

      /***** Assign index (row[0]).
             Index is 0,1,2,3... if no shuffle
             or 1,3,0,2... (example) if shuffle *****/
      ErrorInIndex = false;
      if (sscanf (row[0],"%u",&Index) == 1)
        {
         if (Index >= Qst_MAX_OPTIONS_PER_QUESTION)
            ErrorInIndex = true;
        }
      else
         ErrorInIndex = true;
      if (ErrorInIndex)
         Err_WrongAnswerIndexExit ();

      snprintf (StrInd,sizeof (StrInd),NumOpt ? ",%u" :
						"%u",Index);
      Str_Concat (PrintedQuestion->StrIndexes,StrInd,
                  sizeof (PrintedQuestion->StrIndexes) - 1);
     }

   /***** Free structure that stores the query result *****/
   DB_FreeMySQLResult (&mysql_res);

   /***** Destroy test question *****/
   Qst_QstDestructor (&Question);
  }

/*****************************************************************************/
/************ Get parameters for the selection of test questions *************/
/*****************************************************************************/
// Return true (OK) if all parameters are found, or false (error) if any necessary parameter is not found

bool Tst_GetParamsTst (struct Qst_Questions *Questions,
                       Tst_ActionToDoWithQuestions_t ActionToDoWithQuestions)
  {
   extern const char *Txt_You_must_select_one_ore_more_tags;
   extern const char *Txt_You_must_select_one_ore_more_types_of_answer;
   extern const char *Txt_The_number_of_questions_must_be_in_the_interval_X;
   bool Error = false;
   char UnsignedStr[Cns_MAX_DECIMAL_DIGITS_UINT + 1];
   unsigned UnsignedNum;

   /***** Tags *****/
   /* Get parameter that indicates whether all tags are selected */
   Questions->Tags.All = Par_GetParToBool ("AllTags");

   /* Get the tags */
   if ((Questions->Tags.List = malloc (Tag_MAX_BYTES_TAGS_LIST + 1)) == NULL)
      Err_NotEnoughMemoryExit ();
   Par_GetParMultiToText ("ChkTag",Questions->Tags.List,Tag_MAX_BYTES_TAGS_LIST);

   /* Check number of tags selected */
   if (Tag_CountNumTagsInList (&Questions->Tags) == 0)	// If no tags selected...
     {						// ...write alert
      Ale_ShowAlert (Ale_WARNING,Txt_You_must_select_one_ore_more_tags);
      Error = true;
     }

   /***** Types of answer *****/
   switch (ActionToDoWithQuestions)
     {
      case Tst_SHOW_TEST_TO_ANSWER:
      case Tst_EDIT_QUESTIONS:
      case Tst_SELECT_QUESTIONS_FOR_EXAM:
	 /* Get parameter that indicates if all types of answer are selected */
	 Questions->AnswerTypes.All = Par_GetParToBool ("AllAnsTypes");

	 /* Get types of answer */
	 Par_GetParMultiToText ("AnswerType",Questions->AnswerTypes.List,Qst_MAX_BYTES_LIST_ANSWER_TYPES);

	 /* Check number of types of answer */
	 if (Qst_CountNumAnswerTypesInList (&Questions->AnswerTypes) == 0)	// If no types of answer selected...
	   {									// ...write warning alert
	    Ale_ShowAlert (Ale_WARNING,Txt_You_must_select_one_ore_more_types_of_answer);
	    Error = true;
	   }
	 break;
      case Tst_SELECT_QUESTIONS_FOR_GAME:
	 /* The unique allowed type of answer in a game is unique choice */
	 Questions->AnswerTypes.All = false;
	 snprintf (Questions->AnswerTypes.List,sizeof (Questions->AnswerTypes.List),"%u",
		   (unsigned) Qst_ANS_UNIQUE_CHOICE);
	 break;
      default:
	 break;
     }

   /***** Get other parameters, depending on action *****/
   switch (ActionToDoWithQuestions)
     {
      case Tst_SHOW_TEST_TO_ANSWER:
	 Questions->NumQsts = Tst_GetParamNumQsts ();
	 if (Questions->NumQsts < TstCfg_GetConfigMin () ||
	     Questions->NumQsts > TstCfg_GetConfigMax ())
	   {
	    Ale_ShowAlert (Ale_WARNING,Txt_The_number_of_questions_must_be_in_the_interval_X,
		           TstCfg_GetConfigMin (),TstCfg_GetConfigMax ());
	    Error = true;
	   }
	 break;
      case Tst_EDIT_QUESTIONS:
	 /* Get starting and ending dates */
	 Dat_GetIniEndDatesFromForm ();

	 /* Get ordering criteria */
	 Par_GetParMultiToText ("Order",UnsignedStr,Cns_MAX_DECIMAL_DIGITS_UINT);
	 if (sscanf (UnsignedStr,"%u",&UnsignedNum) == 1)
	    Questions->SelectedOrder = (Qst_QuestionsOrder_t)
				       ((UnsignedNum < Qst_NUM_TYPES_ORDER_QST) ? UnsignedNum :
										  0);
	 else
	    Questions->SelectedOrder = (Qst_QuestionsOrder_t) 0;
	 break;
      case Tst_SELECT_QUESTIONS_FOR_EXAM:
      case Tst_SELECT_QUESTIONS_FOR_GAME:
	 /* Get starting and ending dates */
	 Dat_GetIniEndDatesFromForm ();

	 /* Order question by stem */
	 Questions->SelectedOrder = Qst_ORDER_STEM;
	 break;
      default:
	 break;
     }

   return !Error;
  }

/*****************************************************************************/
/******** Get parameter with the number of test prints generated by me *******/
/*****************************************************************************/

static unsigned Tst_GetParamNumTst (void)
  {
   return (unsigned) Par_GetParToUnsignedLong ("NumTst",
                                               1,
                                               UINT_MAX,
                                               1);
  }

/*****************************************************************************/
/***** Get parameter with the number of questions to generate in an test *****/
/*****************************************************************************/

static unsigned Tst_GetParamNumQsts (void)
  {
   return (unsigned) Par_GetParToUnsignedLong ("NumQst",
	                                       (unsigned long) TstCfg_GetConfigMin (),
	                                       (unsigned long) TstCfg_GetConfigMax (),
	                                       (unsigned long) TstCfg_GetConfigDef ());
  }
