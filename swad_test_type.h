// swad_test_type.h: definition of types for tests

#ifndef _SWAD_TST_TYP
#define _SWAD_TST_TYP
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

#include "swad_media.h"
#include "swad_tag.h"

/*****************************************************************************/
/***************************** Public constants ******************************/
/*****************************************************************************/

#define Qst_MAX_BYTES_FLOAT_ANSWER	30	// Maximum length of the strings that store an floating point answer

#define Qst_MAX_OPTIONS_PER_QUESTION	10

#define Qst_MAX_BYTES_INDEXES_ONE_QST	(Qst_MAX_OPTIONS_PER_QUESTION * (3 + 1))

#define Qst_MAX_CHARS_ANSWERS_ONE_QST	(128 - 1)	// 127
#define Qst_MAX_BYTES_ANSWERS_ONE_QST	((Qst_MAX_CHARS_ANSWERS_ONE_QST + 1) * Str_MAX_BYTES_PER_CHAR - 1)	// 2047

#define Tst_SCORE_MAX	10	// Maximum score of a test (10 in Spain). Must be unsigned! // TODO: Make this configurable by teachers

/*****************************************************************************/
/******************************* Public types ********************************/
/*****************************************************************************/

#define Qst_NUM_VALIDITIES 2
typedef enum
  {
   Qst_INVALID_QUESTION,
   Qst_VALID_QUESTION,
  } Qst_Validity_t;

#define Qst_NUM_ANS_TYPES	6
#define Qst_MAX_BYTES_LIST_ANSWER_TYPES	(Qst_NUM_ANS_TYPES * (Cns_MAX_DECIMAL_DIGITS_UINT + 1))
typedef enum
  {
   Qst_ANS_INT             = 0,
   Qst_ANS_FLOAT           = 1,
   Qst_ANS_TRUE_FALSE      = 2,
   Qst_ANS_UNIQUE_CHOICE   = 3,
   Qst_ANS_MULTIPLE_CHOICE = 4,
   Qst_ANS_TEXT            = 5,
   Qst_ANS_UNKNOWN         = 6,	// Unknown/all/any type of answer
  } Qst_AnswerType_t;

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

/*****************************************************************************/
/***************************** Public prototypes *****************************/
/*****************************************************************************/

#endif
