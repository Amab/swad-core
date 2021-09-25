// swad_match_database.h: matches in games using remote control, operations woth database

#ifndef _SWAD_MCH_DB
#define _SWAD_MCH_DB
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

#include "swad_match.h"

/*****************************************************************************/
/************************** Public types and constants ***********************/
/*****************************************************************************/

/*****************************************************************************/
/***************************** Public prototypes *****************************/
/*****************************************************************************/

//------------------------------- Matches -------------------------------------
long Mch_DB_CreateMatch (long GamCod,char Title[Mch_MAX_BYTES_TITLE + 1]);
void Mch_DB_UpdateMatchStatus (const struct Mch_Match *Match);
void Mch_DB_UpdateMatchTitle (long MchCod,
                              const char Title[Gam_MAX_BYTES_TITLE + 1]);
void Mch_DB_UpdateVisResultsMchUsr (long MchCod,bool ShowUsrResults);

unsigned Mch_DB_GetDataOfMatchByCod (MYSQL_RES **mysql_res,long MchCod);
unsigned Mch_DB_GetStartEndMatchesInGame (MYSQL_RES **mysql_res,long GamCod);
unsigned Mch_DB_GetMatches (MYSQL_RES **mysql_res,long GamCod);
Mch_Showing_t Mch_DB_GetShowingFromStr (const char *Str);
unsigned Mch_DB_GetNumMchsInGame (long GamCod);
unsigned Mch_DB_GetNumUnfinishedMchsInGame (long GamCod);

void Mch_DB_RemoveMatchFromTable (long MchCod,const char *TableName);
void Mch_DB_RemoveMatchesInGameFromMainTable (long GamCod);
void Mch_DB_RemoveMatchesInGameFromOtherTable (long GamCod,const char *TableName);
void Mch_DB_RemoveMatchesInCrsFromMainTable (long CrsCod);
void Mch_DB_RemoveMatchesInCrsFromOtherTable (long CrsCod,const char *TableName);
void Mch_DB_RemoveMatchesMadeByUsrFromTable (long UsrCod,const char *TableName);
void Mch_DB_RemoveMatchesMadeByUsrInCrsFromTable (long UsrCod,long CrsCod,
                                                  const char *TableName);

//---------------------------------Groups -------------------------------------
void Mch_DB_AssociateGroupToMatch (long MchCod,long GrpCod);

unsigned Mch_DB_GetGrpsAssociatedToMatch (MYSQL_RES **mysql_res,long MchCod);
bool Mch_DB_CheckIfICanPlayThisMatchBasedOnGrps (long MchCod);

void Mch_DB_RemoveGroup (long GrpCod);
void Mch_DB_RemoveGroupsOfType (long GrpTypCod);

//---------------------------- Match questions --------------------------------
unsigned Mch_DB_GetMatchQuestions (MYSQL_RES **mysql_res,long MchCod);

//------------------------------ User answers ---------------------------------
void Mch_DB_UpdateMyAnswerToMatchQuestion (const struct Mch_Match *Match,
                                           const struct Mch_UsrAnswer *UsrAnswer);
void Mch_DB_UpdateIndexesOfQstsGreaterThan (long GamCod,unsigned QstInd);

unsigned Mch_DB_GetUsrAnsToQst (MYSQL_RES **mysql_res,
                                long MchCod,long UsrCod,unsigned QstInd);
unsigned Mch_DB_GetNumUsrsWhoAnsweredQst (long MchCod,unsigned QstInd);
unsigned Mch_DB_GetNumUsrsWhoHaveChosenAns (long MchCod,unsigned QstInd,unsigned AnsInd);

void Mch_DB_RemoveMyAnswerToMatchQuestion (const struct Mch_Match *Match);
void Mch_DB_RemUsrAnswersOfAQuestion (long GamCod,unsigned QstInd);

//----------------------------- Answers indexes -------------------------------
void Mch_DB_CreateQstIndexes (long MchCod,unsigned QstInd,
                              const char StrAnswersOneQst[Tst_MAX_BYTES_ANSWERS_ONE_QST + 1]);
void Mch_DB_GetIndexes (long MchCod,unsigned QstInd,
		        char StrIndexesOneQst[Tst_MAX_BYTES_INDEXES_ONE_QST + 1]);

//-------------------------- Matches being played -----------------------------
void Mch_DB_UpdateMatchAsBeingPlayed (long MchCod);
void Mch_DB_RegisterMeAsPlayerInMatch (long MchCod);

bool Mch_DB_GetIfMatchIsBeingPlayed (long MchCod);
unsigned Mch_DB_GetNumPlayers (long MchCod);

void Mch_DB_RemoveMatchFromBeingPlayed (long MchCod);
void Mch_DB_RemoveOldPlaying (void);

//----------------------------- Elapsed times ---------------------------------
void Mch_DB_UpdateElapsedTimeInQuestion (long MchCod,long QstInd);

unsigned Mch_DB_GetElapsedTimeInQuestion (MYSQL_RES **mysql_res,
					  long MchCod,unsigned QstInd);
unsigned Mch_DB_GetElapsedTimeInMatch (MYSQL_RES **mysql_res,long MchCod);

//----------------------------- Match results ---------------------------------
unsigned Mch_DB_GetNumUsrsWhoHavePlayedMch (long MchCod);
unsigned Mch_DB_GetMaxUsrs (long MchCod);
unsigned Mch_DB_GetUsrsPerScore (MYSQL_RES **mysql_res,long MchCod);


#endif