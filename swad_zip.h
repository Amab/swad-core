// swad_zip.h: compress files in file browsers

#ifndef _SWAD_ZIP
#define _SWAD_ZIP
/*
    SWAD (Shared Workspace At a Distance in Spanish),
    is a web platform developed at the University of Granada (Spain),
    and used to support university teaching.

    This file is part of SWAD core.
    Copyright (C) 1999-2014 Antonio Ca�as Vargas

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

// #include "swad_group.h"

/*****************************************************************************/
/******************************* Public types ********************************/
/*****************************************************************************/

/*****************************************************************************/
/****************************** Public constants *****************************/
/*****************************************************************************/

/*****************************************************************************/
/***************************** Public prototypes *****************************/
/*****************************************************************************/

void ZIP_PutButtonToCreateZIPAsgWrk (void);
bool ZIP_GetCreateZIPFromForm (void);
void ZIP_CreateZIPAsgWrk (void);
void ZIP_CreateTmpDirForCompression (void);
void ZIP_CreateDirCompressionUsr (struct UsrData *UsrDat);

void ZIP_PutButtonToDownloadZIPOfAFolder (const char *PathInTree,const char *FileName);
void ZIP_CompressFileTree (void);

#endif
