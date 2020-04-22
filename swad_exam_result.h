// swad_exam_result.h: exams results

#ifndef _SWAD_EXA_RES
#define _SWAD_EXA_RES
/*
    SWAD (Shared Workspace At a Distance in Spanish),
    is a web platform developed at the University of Granada (Spain),
    and used to support university teaching.

    This file is part of SWAD core.
    Copyright (C) 1999-2020 Antonio Ca�as Vargas

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

#include "swad_test_exam.h"

/*****************************************************************************/
/************************** Public types and constants ***********************/
/*****************************************************************************/

#define ExaRes_RESULTS_BOX_ID		"exares_box"
#define ExaRes_RESULTS_TABLE_ID		"exares_table"

/*****************************************************************************/
/***************************** Public prototypes *****************************/
/*****************************************************************************/

void ExaRes_ShowMyExaResultsInCrs (void);
void ExaRes_ShowMyExaResultsInExa (void);
void ExaRes_ShowMyExaResultsInEvt (void);

void ExaRes_ShowAllExaResultsInCrs (void);
void ExaRes_SelUsrsToViewExaResults (void);
void ExaRes_ShowAllExaResultsInExa (void);
void ExaRes_ShowAllExaResultsInEvt (void);

void ExaRes_ShowOneExaResult (void);
void ExaRes_GetExamResultQuestionsFromDB (long EvtCod,long UsrCod,
				          struct TstRes_Result *Result);

#endif