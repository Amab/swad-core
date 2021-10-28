// swad_question_database.h: test/exam/game questions, operations with database

#ifndef _SWAD_QST_DB
#define _SWAD_QST_DB
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

// #include <stdbool.h>		// For boolean type
// #include <time.h>		// For time_t

// #include "swad_exam.h"
// #include "swad_game.h"
// #include "swad_question_type.h"
// #include "swad_media.h"
// #include "swad_string.h"
// #include "swad_tag.h"

/*****************************************************************************/
/***************************** Public constants ******************************/
/*****************************************************************************/

/*****************************************************************************/
/******************************* Public types ********************************/
/*****************************************************************************/

/*****************************************************************************/
/***************************** Public prototypes *****************************/
/*****************************************************************************/

void Qst_DB_UpdateQstScore (long QstCod,bool AnswerIsNotBlank,double Score);

unsigned Qst_DB_GetQuestionsForNewTestPrint (MYSQL_RES **mysql_res,
                                        const struct Qst_Questions *Questions);
unsigned Qst_DB_GetNumQsts (MYSQL_RES **mysql_res,
                            HieLvl_Level_t Scope,Qst_AnswerType_t AnsType);
unsigned Qst_DB_GetNumCrssWithQsts (HieLvl_Level_t Scope,
                                    Qst_AnswerType_t AnsType);
unsigned Qst_DB_GetNumCrssWithPluggableQsts (HieLvl_Level_t Scope,
                                             Qst_AnswerType_t AnsType);

unsigned Qst_DB_GetNumAnswersQst (long QstCod);
unsigned Qst_DB_GetDataOfAnswers (MYSQL_RES **mysql_res,long QstCod,bool Shuffle);
unsigned Qst_DB_GetTextOfAnswers (MYSQL_RES **mysql_res,long QstCod);

#endif