// swad_question_import.h: import and export questions using XML files

#ifndef _SWAD_QST_IMP
#define _SWAD_QST_IMP
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

/*****************************************************************************/
/***************************** Public constants ******************************/
/*****************************************************************************/

/*****************************************************************************/
/******************************* Public types ********************************/
/*****************************************************************************/

/*****************************************************************************/
/***************************** Public prototypes *****************************/
/*****************************************************************************/

void QstImp_PutIconToExportQuestions (struct Tst_Test *Test);
bool QstImp_GetCreateXMLParamFromForm (void);
void QstImp_PutIconToImportQuestions (void);
void QstImp_CreateXML (unsigned long NumRows,MYSQL_RES *mysql_res);
void QstImp_ShowFormImportQstsFromXML (void);
void QstImp_ImportQstsFromXML (void);

#endif
