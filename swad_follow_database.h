// swad_follow_database.h: user's followers and followed operations with database

#ifndef _SWAD_FOL_DB
#define _SWAD_FOL_DB
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
/********************************** Headers **********************************/
/*****************************************************************************/

#include "swad_follow.h"

/*****************************************************************************/
/****************************** Public constants *****************************/
/*****************************************************************************/

/*****************************************************************************/
/******************************** Public types *******************************/
/*****************************************************************************/

/*****************************************************************************/
/****************************** Public prototypes ****************************/
/*****************************************************************************/

unsigned Fol_DB_GetUsrsToFollow (unsigned MaxUsrsToShow,
				 Fol_WhichUsersSuggestToFollowThem_t WhichUsersSuggestToFollowThem,
				 MYSQL_RES **mysql_res);

bool Fol_DB_CheckUsrIsFollowerOf (long FollowerCod,long FollowedCod);

unsigned Fol_DB_GetNumFollowing (long UsrCod);
unsigned Fol_DB_GetNumFollowers (long UsrCod);

unsigned Fol_DB_GetListFollowing (long UsrCod,MYSQL_RES **mysql_res);
unsigned Fol_DB_GetListFollowers (long UsrCod,MYSQL_RES **mysql_res);

void Fol_DB_FollowUsr (long UsrCod);
void Fol_DB_UnfollowUsr (long UsrCod);

unsigned Fol_DB_GetRankingFollowers (MYSQL_RES **mysql_res);

void Fol_DB_RemoveUsrFromUsrFollow (long UsrCod);

void Fol_DB_CreateTmpTableMeAndUsrsIFollow (void);
void Fol_DB_DropTmpTableMeAndUsrsIFollow (void);

#endif
