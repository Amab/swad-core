// swad_match.h: matches in games using remote control

#ifndef _SWAD_MCH
#define _SWAD_MCH
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

#include "swad_game.h"
#include "swad_match_print.h"
#include "swad_scope.h"
#include "swad_test.h"

/*****************************************************************************/
/************************** Public types and constants ***********************/
/*****************************************************************************/

#define Mch_MAX_CHARS_TITLE	Gam_MAX_CHARS_TITLE	// 127
#define Mch_MAX_BYTES_TITLE	((Mch_MAX_CHARS_TITLE + 1) * Str_MAX_BYTES_PER_CHAR - 1)	// 2047

#define Mch_NEW_MATCH_SECTION_ID	"new_match"

#define Mch_NUM_SHOWING 5
typedef enum
  {
   Mch_START,	// Start: don't show anything
   Mch_STEM,	// Showing only the question stem
   Mch_ANSWERS,	// Showing the question stem and the answers
   Mch_RESULTS,	// Showing the results
   Mch_END,	// End: don't show anything
  } Mch_Showing_t;
#define Mch_SHOWING_DEFAULT Mch_START

struct Mch_Match
  {
   long MchCod;
   long GamCod;
   long UsrCod;
   time_t TimeUTC[Dat_NUM_START_END_TIME];
   char Title[Gam_MAX_BYTES_TITLE + 1];
   struct
     {
      unsigned QstInd;	// 0 means that the game has not started. First question has index 1.
      long QstCod;
      time_t QstStartTimeUTC;
      Mch_Showing_t Showing;	// What is shown on teacher's screen
      long Countdown;		// > 0 ==> countdown in progress
				// = 0 ==> countdown over ==> go to next step
				// < 0 ==> no countdown at this time
      unsigned NumCols;		// Number of columns for answers on teacher's screen
      bool ShowQstResults;	// Show global results of current question while playing
      bool ShowUsrResults;	// Show exam with results of all questions for the student
      bool Playing;		// Is being played now?
      unsigned NumPlayers;
     } Status;			// Status related to match playing
  };

struct Mch_UsrAnswer
  {
   int NumOpt;	// < 0 ==> no answer selected
   int AnsInd;	// < 0 ==> no answer selected
  };

/*****************************************************************************/
/***************************** Public prototypes *****************************/
/*****************************************************************************/

long Mch_GetMchCodBeingPlayed (void);

void Mch_ResetMatch (struct Mch_Match *Match);

void Mch_ListMatches (struct Gam_Games *Games,
                      struct Gam_Game *Game,
                      bool PutFormNewMatch);
void Mch_GetDataOfMatchByCod (struct Mch_Match *Match);

void Mch_ToggleVisResultsMchUsr (void);

void Mch_RequestRemoveMatch (void);
void Mch_RemoveMatch (void);

void Mch_RemoveMatchesInGameFromAllTables (long GamCod);
void Mch_RemoveMatchesInCourseFromAllTables (long CrsCod);
void Mch_RemoveMatchesMadeByUsrInAllCrss (long UsrCod);
void Mch_RemoveMatchesMadeByUsrInCrs (long UsrCod,long CrsCod);

void Mch_EditMatch (void);

void Mch_PutParamsEdit (void *Games);
void Mch_GetAndCheckParameters (struct Gam_Games *Games,
                                struct Gam_Game *Game,
                                struct Mch_Match *Match);
long Mch_GetParamMchCod (void);

void Mch_CreateNewMatch (void);
void Mch_ChangeMatch (void);
void Mch_ResumeMatch (void);
void Mch_GetIndexes (long MchCod,unsigned QstInd,
		     unsigned Indexes[Tst_MAX_OPTIONS_PER_QUESTION]);

void Mch_DB_RemoveGroup (long GrpCod);
void Mch_DB_RemoveGroupsOfType (long GrpTypCod);

void Mch_PlayPauseMatch (void);
void Mch_ChangeNumColsMch (void);
void Mch_ToggleVisResultsMchQst (void);
void Mch_BackMatch (void);
void Mch_ForwardMatch (void);

unsigned Mch_DB_GetNumMchsInGame (long GamCod);
unsigned Mch_DB_GetNumUnfinishedMchsInGame (long GamCod);

bool Mch_CheckIfICanPlayThisMatchBasedOnGrps (const struct Mch_Match *Match);

bool Mch_RegisterMeAsPlayerInMatch (struct Mch_Match *Match);

void Mch_GetMatchBeingPlayed (void);
void Mch_JoinMatchAsStd (void);
void Mch_RemMyQstAnsAndShowMchStatus (void);
void Mch_RemoveMyQuestionAnswer (const struct Mch_Match *Match,unsigned QstInd);

void Mch_StartCountdown (void);
void Mch_RefreshMatchTch (void);
void Mch_RefreshMatchStd (void);

void Mch_GetQstAnsFromDB (long MchCod,long UsrCod,unsigned QstInd,
		          struct Mch_UsrAnswer *UsrAnswer);
void Mch_ReceiveQuestionAnswer (void);
void Mch_StoreQuestionAnswer (const struct Mch_Match *Match,unsigned QstInd,
                              struct Mch_UsrAnswer *UsrAnswer);

void Mch_GetMatchQuestionsFromDB (struct MchPrn_Print *Print);

void Mch_ComputeScore (struct MchPrn_Print *Print);

unsigned Mch_DB_GetNumUsrsWhoAnsweredQst (long MchCod,unsigned QstInd);
unsigned Mch_DB_GetNumUsrsWhoHaveChosenAns (long MchCod,unsigned QstInd,unsigned AnsInd);
void Mch_DrawBarNumUsrs (unsigned NumRespondersAns,unsigned NumRespondersQst,bool Correct);

void Mch_DB_UpdateIndexesOfQstsGreaterThan (long GamCod,unsigned QstInd);

unsigned Mch_DB_GetStartEndMatchesInGame (MYSQL_RES **mysql_res,long GamCod);
void Mch_DB_RemAnswersOfAQuestion (long GamCod,unsigned QstInd);

#endif
