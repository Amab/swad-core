// swad_game.h: games using remote control

#ifndef _SWAD_GAM
#define _SWAD_GAM
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

#include "swad_date.h"
#include "swad_scope.h"

/*****************************************************************************/
/************************** Public types and constants ***********************/
/*****************************************************************************/

#define Gam_MAX_CHARS_TITLE	(128 - 1)	// 127
#define Gam_MAX_BYTES_TITLE	((Gam_MAX_CHARS_TITLE + 1) * Str_MAX_BYTES_PER_CHAR - 1)	// 2047

#define Gam_AFTER_LAST_QUESTION	((unsigned)((1UL << 31) - 1))	// 2^31 - 1, don't change this number because it is used in database to indicate that a match is finished

#define Gam_NUM_ORDERS 3
typedef enum
  {
   Gam_ORDER_BY_START_DATE = 0,
   Gam_ORDER_BY_END_DATE   = 1,
   Gam_ORDER_BY_TITLE      = 2,
  } Gam_Order_t;
#define Gam_ORDER_DEFAULT Gam_ORDER_BY_START_DATE

#define Gam_NUM_ANS_TYPES	2
typedef enum
  {
   Gam_ANS_UNIQUE_CHOICE   = 0,
   Gam_ANS_MULTIPLE_CHOICE = 1,
  } Gam_AnswerType_t;
#define Gam_ANSWER_TYPE_DEFAULT Gam_ANS_UNIQUE_CHOICE

struct Gam_GameSelected
  {
   long GamCod;		// Game code
   bool Selected;	// Is this game selected when seeing match results?
  };

struct Gam_Games
  {
   bool LstIsRead;		// Is the list already read from database...
				// ...or it needs to be read?
   unsigned Num;		// Total number of games
   unsigned NumSelected;	// Number of games selected
   struct Gam_GameSelected *Lst;// List of games
   Gam_Order_t SelectedOrder;
   unsigned CurrentPage;
   char *ListQuestions;
   char *GamCodsSelected;	// String with selected game codes separated by separator multiple
   long GamCod;			// Selected/current game code
   struct
     {
      long Selected;		// Current match code
      long Current;
     } MchCod;
   unsigned QstInd;		// Current question index
  };

struct Gam_Game
  {
   long GamCod;			// Game code
   long CrsCod;			// Course code
   long UsrCod;			// Author code
   double MaxGrade;		// Score range [0...max.score]
				// will be converted to
				// grade range [0...max.grade]
   unsigned Visibility;		// Visibility of results
   char Title[Gam_MAX_BYTES_TITLE + 1];
   time_t TimeUTC[Dat_NUM_START_END_TIME];
   bool Hidden;			// Game is hidden
   unsigned NumQsts;		// Number of questions in the game
   unsigned NumMchs;		// Number of matches in the game
   unsigned NumUnfinishedMchs;	// Number of unfinished matches in the game
  };

/*****************************************************************************/
/***************************** Public prototypes *****************************/
/*****************************************************************************/

void Gam_ResetGames (struct Gam_Games *Games);
void Gam_ResetGame (struct Gam_Game *Game);

void Gam_SeeAllGames (void);
void Gam_SeeOneGame (void);
void Gam_ShowOnlyOneGame (struct Gam_Games *Games,
			  struct Gam_Game *Game,
			  bool ListGameQuestions,
			  bool PutFormNewMatch);
void Gam_ShowOnlyOneGameBegin (struct Gam_Games *Games,
			       struct Gam_Game *Game,
			       bool ListGameQuestions,
			       bool PutFormNewMatch);
void Gam_ShowOnlyOneGameEnd (void);

void Gam_SetCurrentGamCod (long GamCod);
void Gam_PutParams (void *Games);
void Gam_PutParamGameCod (long GamCod);
long Gam_GetParamGameCod (void);
long Gam_GetParams (struct Gam_Games *Games);

void Gam_GetListGames (struct Gam_Games *Games,Gam_Order_t SelectedOrder);
void Gam_GetListSelectedGamCods (struct Gam_Games *Games);
void Gam_GetDataOfGameByCod (struct Gam_Game *Game);
void Gam_GetDataOfGameByFolder (struct Gam_Game *Game);
void Gam_FreeListGames (struct Gam_Games *Games);

void Gam_AskRemGame (void);
void Gam_RemoveGame (void);
void Gam_RemoveCrsGames (long CrsCod);

void Gam_HideGame (void);
void Gam_UnhideGame (void);

void Gam_ListGame (void);

void Gam_RequestCreatOrEditGame (void);

void Gam_ReceiveFormGame (void);

void Gam_ReqSelectQstsToAddToGame (void);
void Gam_ListQstsToAddToGame (void);

void Gam_PutParamQstInd (unsigned QstInd);
unsigned Gam_GetParamQstInd (void);
long Gam_GetQstCodFromQstInd (long GamCod,unsigned QstInd);

void Gam_AddQstsToGame (void);

void Gam_RequestRemoveQstFromGame (void);
void Gam_RemoveQstFromGame (void);

void Gam_MoveUpQst (void);
void Gam_MoveDownQst (void);

void Gam_PutButtonNewMatch (struct Gam_Games *Games,long GamCod);
void Gam_RequestNewMatch (void);

void Gam_ShowTstTagsPresentInAGame (long GamCod);

void Gam_GetScoreRange (long GamCod,double *MinScore,double *MaxScore);

#endif
