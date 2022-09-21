// swad_notice_database.h: notices (yellow notes) operations with database

#ifndef _SWAD_NOT_DB
#define _SWAD_NOT_DB
/*
    SWAD (Shared Workspace At a Distance in Spanish),
    is a web platform developed at the University of Granada (Spain),
    and used to support university teaching.

    This file is part of SWAD core.
    Copyright (C) 1999-2022 Antonio Ca�as Vargas

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

#include <mysql/mysql.h>	// To access MySQL databases

/*****************************************************************************/
/***************************** Public prototypes *****************************/
/*****************************************************************************/

long Not_DB_InsertNotice (const char *Content);
void Not_DB_ChangeNoticeStatus (long NotCod,Not_Status_t Status);
void Not_DB_CopyNoticeToDeleted (long NotCod);
void Not_DB_UpdateNumUsrsNotifiedByEMailAboutNotice (long NotCod,
                                                     unsigned NumUsrsToBeNotifiedByEMail);

unsigned Not_DB_GetDataOfNotice (MYSQL_RES **mysql_res,long NotCod);
unsigned Not_DB_ContentNotice (MYSQL_RES **mysql_res,long NotCod);
unsigned Not_DB_GetAllNotices (MYSQL_RES **mysql_res);
unsigned Not_DB_GetActiveNotices (MYSQL_RES **mysql_res,long CrsCod);
unsigned Not_DB_GetNumNotices (MYSQL_RES **mysql_res,
                               HieLvl_Level_t Scope,Not_Status_t Status);
unsigned Not_DB_GetNumNoticesDeleted (MYSQL_RES **mysql_res,
                                      HieLvl_Level_t Scope);

void Not_DB_RemoveNotice (long NotCod);
void Not_DB_RemoveCrsNotices (long CrsCod);

#endif
