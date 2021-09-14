// swad_browser_database.h: file browsers operations with database

#ifndef _SWAD_BROWSER_DATABASE
#define _SWAD_BROWSER_DATABASE
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

// #include <linux/limits.h>	// For PATH_MAX
#include <mysql/mysql.h>	// To access MySQL databases

#include "swad_browser.h"
// #include "swad_course.h"
// #include "swad_group.h"
// #include "swad_notification.h"

/*****************************************************************************/
/************************ Public types and constants *************************/
/*****************************************************************************/

/*****************************************************************************/
/****************************** Public constants *****************************/
/*****************************************************************************/

/*****************************************************************************/
/***************************** Public prototypes *****************************/
/*****************************************************************************/

void Brw_DB_StoreSizeOfFileZone (void);

void Brw_DB_UpdateDateMyLastAccFileBrowser (Brw_FileBrowser_t FileBrowser);
unsigned Brw_DB_GetDateMyLastAccFileBrowser (MYSQL_RES **mysql_res,
                                             Brw_FileBrowser_t FileBrowser);
unsigned Brw_DB_GetGrpLastAccFileBrowser (MYSQL_RES **mysql_res,const char *FieldNameDB);

void Brw_DB_AddPathToClipboards (void);
void Brw_DB_UpdatePathInClipboard (void);
unsigned Brw_DB_GetMyClipboard (MYSQL_RES **mysql_res);

unsigned Brw_DB_GetFoldersAssignments (MYSQL_RES **mysql_res,long ZoneUsrCod);

void Brw_DB_GetSizeOfFileBrowser (MYSQL_RES **mysql_res,
			          Brw_FileBrowser_t FileBrowser);
unsigned Brw_DB_GetNumberOfOERs (MYSQL_RES **mysql_res,Brw_License_t License);

void Brw_DB_RemoveInsFiles (long InsCod);
void Brw_DB_RemoveCtrFiles (long CtrCod);
void Brw_DB_RemoveDegFiles (long DegCod);
void Brw_DB_RemoveCrsFiles (long CrsCod);
void Brw_DB_RemoveGrpFiles (long GrpCod);
void Brw_DB_RemovePrjFiles (long PrjCod);
void Brw_DB_RemoveSomeInfoAboutCrsUsrFiles (long UsrCod,long CrsCod);
void Brw_DB_RemoveWrkFiles (long CrsCod,long UsrCod);
void Brw_DB_RemoveUsrFiles (long UsrCod);

#endif
