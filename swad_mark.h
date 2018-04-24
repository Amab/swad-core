// swad_marks.h: marks

#ifndef _SWAD_MRK
#define _SWAD_MRK
/*
    SWAD (Shared Workspace At a Distance in Spanish),
    is a web platform developed at the University of Granada (Spain),
    and used to support university teaching.

    This file is part of SWAD core.
    Copyright (C) 1999-2018 Antonio Ca�as Vargas

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

#include <stdbool.h>		// For boolean type

#include "swad_file_browser.h"

/*****************************************************************************/
/******************************* Public types ********************************/
/*****************************************************************************/

struct MarksProperties
  {
   unsigned Header;
   unsigned Footer;
  };

/*****************************************************************************/
/***************************** Public prototypes *****************************/
/*****************************************************************************/

void Mrk_AddMarksToDB (long FilCod,struct MarksProperties *Marks);
void Mrk_GetAndWriteNumRowsHeaderAndFooter (Brw_FileType_t FileType,const char *PathInTree,const char *FileName);
void Mrk_ChangeNumRowsHeader (void);
void Mrk_ChangeNumRowsFooter (void);

bool Mrk_CheckFileOfMarks (const char *Path,struct MarksProperties *Marks);
void Mrk_ShowMyMarks (void);
void Mrk_GetNotifMyMarks (char SummaryStr[Ntf_MAX_BYTES_SUMMARY + 1],
                          char **ContentStr,
                          long MrkCod,long UsrCod,bool GetContent);

#endif
