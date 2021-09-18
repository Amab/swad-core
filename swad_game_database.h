// swad_game_database.h: games using remote control, operations with database

#ifndef _SWAD_GAM_DB
#define _SWAD_GAM_DB
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

// #include "swad_date.h"
#include "swad_hierarchy_level.h"
// #include "swad_scope.h"

/*****************************************************************************/
/************************** Public types and constants ***********************/
/*****************************************************************************/

/*****************************************************************************/
/***************************** Public prototypes *****************************/
/*****************************************************************************/

unsigned Gam_DB_GetListGames (MYSQL_RES **mysql_res,Gam_Order_t SelectedOrder);
unsigned Gam_DB_GetDataOfGameByCod (MYSQL_RES **mysql_res,long GamCod);
void Gam_DB_GetGameTxt (long GamCod,char Txt[Cns_MAX_BYTES_TEXT + 1]);
bool Gam_DB_CheckIfSimilarGameExists (const struct Gam_Game *Game);

unsigned Gam_DB_GetNumQstsGame (long GamCod);
unsigned Gam_DB_GetQstIndFromQstCod (long GamCod,long QstCod);
unsigned Gam_DB_GetMaxQuestionIndexInGame (long GamCod);

unsigned Gam_DB_GetPrevQuestionIndexInGame (long GamCod,unsigned QstInd);
unsigned Gam_DB_GetNextQuestionIndexInGame (long GamCod,unsigned QstInd);

unsigned Gam_DB_GetNumCoursesWithGames (HieLvl_Level_t Scope);
unsigned Gam_DB_GetNumGames (HieLvl_Level_t Scope);
double Gam_DB_GetNumQstsPerGame (HieLvl_Level_t Scope);

#endif
