// swad_duplicate_database.h: duplicate users operations with database

#ifndef _SWAD_DUP_DB
#define _SWAD_DUP_DB
/*
    SWAD (Shared Workspace At a Distance in Spanish),
    is a web platform developed at the University of Granada (Spain),
    and used to support university teaching.

    This file is part of SWAD core.
    Copyright (C) 1999-2025 Antonio Ca�as Vargas

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

#include "swad_user.h"

/*****************************************************************************/
/****************************** Public prototypes ****************************/
/*****************************************************************************/

unsigned Dup_DB_GetListDuplicateUsrs (MYSQL_RES **mysql_res);
unsigned Dup_DB_GetUsrsSimilarTo (MYSQL_RES **mysql_res,const struct Usr_Data *UsrDat);
bool Dup_DB_CheckIfUsrIsDup (long UsrCod);

void Dup_DB_AddUsrToDuplicated (long UsrCod);

void Dup_DB_RemoveUsrFromDuplicated (long UsrCod);

#endif
