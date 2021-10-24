// swad_tag_database.h: tags for questions, operations with database

#ifndef _SWAD_TAG_DB
#define _SWAD_TAG_DB
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

#include <mysql/mysql.h>	// To access MySQL databases
#include <stdbool.h>		// For boolean type
// #include "swad_string.h"

/*****************************************************************************/
/***************************** Public constants ******************************/
/*****************************************************************************/

/*****************************************************************************/
/******************************* Public types ********************************/
/*****************************************************************************/

/*****************************************************************************/
/***************************** Public prototypes *****************************/
/*****************************************************************************/

long Tag_DB_CreateNewTag (long CrsCod,const char *TagTxt);
void Tag_DB_AddTagToQst (long QstCod,long TagCod,unsigned TagInd);

void Tag_DB_EnableOrDisableTag (long TagCod,bool TagHidden);

unsigned Tag_DB_GetAllTagsFromCurrentCrs (MYSQL_RES **mysql_res);
unsigned Tag_DB_GetEnabledTagsFromThisCrs (MYSQL_RES **mysql_res);
bool Tag_DB_CheckIfCurrentCrsHasTestTags (void);
long Tag_DB_GetTagCodFromTagTxt (const char *TagTxt);

void Tag_DB_RemTagsFromQst (long QstCod);
void Tag_DB_RemoveUnusedTagsFromCrs (long CrsCod);

#endif
