// swad_question.h: test/exam/game questions

#ifndef _SWAD_QST
#define _SWAD_QST
/*
    SWAD (Shared Workspace At a Distance in Spanish),
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

#include <stdbool.h>		// For boolean type
#include <time.h>		// For time_t

#include "swad_exam.h"
#include "swad_game.h"
#include "swad_question_type.h"
#include "swad_media.h"
#include "swad_string.h"
#include "swad_tag.h"

/*****************************************************************************/
/***************************** Public constants ******************************/
/*****************************************************************************/

#define Qst_MAX_CHARS_ANSWER_OR_FEEDBACK	(1024 - 1)	// 1023
#define Qst_MAX_BYTES_ANSWER_OR_FEEDBACK	((Qst_MAX_CHARS_ANSWER_OR_FEEDBACK + 1) * Str_MAX_BYTES_PER_CHAR - 1)	// 16383

/*****************************************************************************/
/******************************* Public types ********************************/
/*****************************************************************************/

struct Qst_AnswerTypes
  {
   bool All;
   char List[Qst_MAX_BYTES_LIST_ANSWER_TYPES + 1];
  };

#define Qst_NUM_TYPES_ORDER_QST	5
typedef enum
  {
   Qst_ORDER_STEM                    = 0,
   Qst_ORDER_NUM_HITS                = 1,
   Qst_ORDER_AVERAGE_SCORE           = 2,
   Qst_ORDER_NUM_HITS_NOT_BLANK      = 3,
   Qst_ORDER_AVERAGE_SCORE_NOT_BLANK = 4,
  } Qst_QuestionsOrder_t;
#define Qst_DEFAULT_ORDER Qst_ORDER_STEM

#define Qst_NUM_VALIDITIES 2
typedef enum
  {
   Qst_INVALID_QUESTION,
   Qst_VALID_QUESTION,
  } Qst_Validity_t;

struct Qst_Question
  {
   long QstCod;
   struct Tag_Tags Tags;
   time_t EditTime;
   char *Stem;
   char *Feedback;
   struct Med_Media Media;
   struct
     {
      Qst_AnswerType_t Type;
      unsigned NumOptions;
      bool Shuffle;
      char TF;
      struct
	{
	 bool Correct;
	 char *Text;
	 char *Feedback;
	 struct Med_Media Media;
	} Options[Qst_MAX_OPTIONS_PER_QUESTION];
      long Integer;
      double FloatingPoint[2];
     } Answer;
   unsigned long NumHits;
   unsigned long NumHitsNotBlank;
   double Score;
   Qst_Validity_t Validity;	// If a question in an exam has been marked as invalid
  };

struct Qst_Questions
  {
   struct Tag_Tags Tags;		// Selected tags
   struct Qst_AnswerTypes AnswerTypes;	// Selected answer types
   Qst_QuestionsOrder_t SelectedOrder;	// Order for listing questions
   unsigned NumQsts;			// Number of questions
   struct Qst_Question Question;	// Selected / editing question
  };

struct Qst_Stats
  {
   unsigned NumCoursesWithQuestions;
   unsigned NumCoursesWithPluggableQuestions;
   unsigned NumQsts;
   double AvgQstsPerCourse;
   unsigned long NumHits;
   double AvgHitsPerCourse;
   double AvgHitsPerQuestion;
   double TotalScore;
   double AvgScorePerQuestion;
  };

/*****************************************************************************/
/***************************** Public prototypes *****************************/
/*****************************************************************************/

void Qst_Constructor (struct Qst_Questions *Questions);
void Qst_Destructor (struct Qst_Questions *Questions);

void Qst_RequestEditQsts (void);
void Qst_ShowFormRequestEditQsts (struct Qst_Questions *Questions);
void Qst_ShowFormAnswerTypes (const struct Qst_AnswerTypes *AnswerTypes);
void Qst_RequestSelectQstsForExamSet (struct Exa_Exams *Exams);
void Qst_RequestSelectQstsForGame (struct Gam_Games *Games);
void Qst_ShowFormRequestSelectQstsForExamSet (struct Exa_Exams *Exams,
                                              struct Qst_Questions *Questions);
void Qst_ShowFormRequestSelectQstsForGame (struct Gam_Games *Games,
                                           struct Qst_Questions *Questions);

bool Qst_CheckIfICanEditQsts (void);
void Qst_PutIconsRequestBankQsts (__attribute__((unused)) void *Args);
void Qst_PutIconsEditBankQsts (void *Questions);
void Qst_PutButtonToAddQuestion (void);

void Qst_ListQuestionForEdition (struct Qst_Question *Question,
                                 unsigned QstInd,bool QuestionExists,
                                 const char *Anchor);
void Qst_WriteNumQst (unsigned NumQst,const char *Class);
void Qst_WriteAnswerType (Qst_AnswerType_t AnswerType,const char *Class);
void Qst_WriteQstStem (const char *Stem,const char *ClassStem,bool Visible);
void Qst_PutFormToEditQstMedia (const struct Med_Media *Media,int NumMedia,
                                bool OptionsDisabled);
void Qst_WriteQstFeedback (const char *Feedback,const char *ClassFeedback);

void Qst_ListQuestionsToEdit (void);
void Qst_ListQuestionsToSelectForExamSet (struct Exa_Exams *Exams);
void Qst_ListQuestionsToSelectForGame (struct Gam_Games *Games);

void Qst_ListOneOrMoreQstsForEdition (struct Qst_Questions *Questions,
                                      MYSQL_RES *mysql_res);
void Qst_WriteHeadingRowQuestionsForEdition (struct Qst_Questions *Questions);
void Qst_WriteQuestionListing (struct Qst_Questions *Questions,unsigned QstInd);

void Qst_ListOneOrMoreQstsForSelectionForExamSet (struct Exa_Exams *Exams,
						  unsigned NumQsts,
                                                  MYSQL_RES *mysql_res);
void Qst_ListOneOrMoreQstsForSelectionForGame (struct Gam_Games *Games,
					       unsigned NumQsts,
                                               MYSQL_RES *mysql_res);
void Qst_PutCheckboxToSelectAllQuestions (void);
void Qst_WriteQuestionRowForSelection (unsigned QstInd,
                                       struct Qst_Question *Question);

void Qst_PutParamsEditQst (void *Questions);

unsigned Qst_DB_GetNumAnswersQst (long QstCod);
unsigned Qst_DB_GetAnswersQst (MYSQL_RES **mysql_res,
                               const struct Qst_Question *Question,
                               bool Shuffle);

void Qst_WriteAnswersBank (struct Qst_Question *Question,
                           const char *ClassTxt,
                           const char *ClassFeedback);

void Qst_ListOneQstToEdit (struct Qst_Questions *Questions);

//-----------------------------------------------------------------------------

void Qst_WriteIntAnsBank (struct Qst_Question *Question,
                          const char *ClassTxt,
                          __attribute__((unused)) const char *ClassFeedback);
void Qst_WriteFltAnsBank (struct Qst_Question *Question,
                          const char *ClassTxt,
                          __attribute__((unused)) const char *ClassFeedback);
void Qst_WriteTF_AnsBank (struct Qst_Question *Question,
                          const char *ClassTxt,
                          __attribute__((unused)) const char *ClassFeedback);
void Qst_WriteChoAnsBank (struct Qst_Question *Question,
                          const char *ClassTxt,
                          const char *ClassFeedback);
//-----------------------------------------------------------------------------

void Qst_GetCorrectIntAnswerFromDB (struct Qst_Question *Question);
void Qst_GetCorrectFltAnswerFromDB (struct Qst_Question *Question);
void Qst_GetCorrectTF_AnswerFromDB (struct Qst_Question *Question);
void Qst_GetCorrectChoAnswerFromDB (struct Qst_Question *Question);
void Qst_GetCorrectTxtAnswerFromDB (struct Qst_Question *Question);

//-----------------------------------------------------------------------------

void Qst_WriteAnsTF (char AnsTF);

void Qst_WriteParamQstCod (unsigned NumQst,long QstCod);

void Qst_CheckIfNumberOfAnswersIsOne (const struct Qst_Question *Question);

void Qst_GetQuestions (struct Qst_Questions *Questions,MYSQL_RES **mysql_res);

void Qst_ChangeFormatAnswersText (struct Qst_Question *Question);
void Qst_ChangeFormatAnswersFeedback (struct Qst_Question *Question);

Qst_AnswerType_t Qst_ConvertFromStrAnsTypDBToAnsTyp (const char *StrAnsTypeDB);
Qst_AnswerType_t Qst_ConvertFromUnsignedStrToAnsTyp (const char *UnsignedStr);

void Qst_ShowFormEditOneQst (void);
void Qst_PutFormEditOneQst (struct Qst_Question *Question);
void Qst_PutFloatInputField (const char *Label,const char *Field,
                             const struct Qst_Question *Question,
                             unsigned Index);
void Qst_PutTFInputField (const struct Qst_Question *Question,
                          const char *Label,char Value);

void Qst_QstConstructor (struct Qst_Question *Question);
void Qst_QstDestructor (struct Qst_Question *Question);

bool Qst_AllocateTextChoiceAnswer (struct Qst_Question *Question,unsigned NumOpt);
void Qst_FreeTextChoiceAnswers (struct Qst_Question *Question);
void Qst_FreeTextChoiceAnswer (struct Qst_Question *Question,unsigned NumOpt);

void Qst_ResetMediaOfQuestion (struct Qst_Question *Question);
void Qst_FreeMediaOfQuestion (struct Qst_Question *Question);

Qst_AnswerType_t Qst_GetQstAnswerTypeFromDB (long QstCod);
bool Qst_GetQstDataFromDB (struct Qst_Question *Question);
long Qst_GetMedCodFromDB (long CrsCod,long QstCod,int NumOpt);
void Qst_GetMediaFromDB (long CrsCod,long QstCod,int NumOpt,
                         struct Med_Media *Media);
void Qst_ReceiveQst (void);
void Qst_GetQstFromForm (struct Qst_Question *Question);
bool Qst_CheckIfQstFormatIsCorrectAndCountNumOptions (struct Qst_Question *Question);

bool Qst_CheckIfQuestionExistsInDB (struct Qst_Question *Question);

void Qst_MoveMediaToDefinitiveDirectories (struct Qst_Question *Question);

long Qst_GetIntAnsFromStr (char *Str);

void Qst_RequestRemoveSelectedQsts (void);
void Qst_RemoveSelectedQsts (void);
void Qst_PutIconToRemoveOneQst (void *QstCod);
void Qst_RequestRemoveOneQst (void);
void Qst_PutParamsRemoveOnlyThisQst (void *QstCod);
void Qst_RemoveOneQst (void);
void Qst_RemoveOneQstFromDB (long CrsCod,long QstCod);

void Qst_ChangeShuffleQst (void);

long Qst_GetParamQstCod (void);
void Qst_PutParamQstCod (void *QstCod);

void Qst_InsertOrUpdateQstTagsAnsIntoDB (struct Qst_Question *Question);
void Qst_InsertOrUpdateQstIntoDB (struct Qst_Question *Question);
void Qst_InsertAnswersIntoDB (struct Qst_Question *Question);

unsigned Qst_CountNumAnswerTypesInList (const struct Qst_AnswerTypes *AnswerTypes);
unsigned Qst_CountNumQuestionsInList (const char *ListQuestions);

void Qst_RemoveCrsQsts (long CrsCod);
void Qst_DB_RemAnsFromQst (long QstCod);
void Qst_RemoveMediaFromStemOfQst (long CrsCod,long QstCod);
void Qst_RemoveMediaFromAllAnsOfQst (long CrsCod,long QstCod);
void Qst_RemoveAllMedFilesFromStemOfAllQstsInCrs (long CrsCod);
void Qst_RemoveAllMedFilesFromAnsOfAllQstsInCrs (long CrsCod);

unsigned Qst_DB_GetShuffledAnswersIndexes (MYSQL_RES **mysql_res,
                                           const struct Qst_Question *Question);

unsigned Qst_GetNumQuestions (HieLvl_Level_t Scope,Qst_AnswerType_t AnsType,
                              struct Qst_Stats *Stats);

void Qst_GetTestStats (Qst_AnswerType_t AnsType,struct Qst_Stats *Stats);

#endif
