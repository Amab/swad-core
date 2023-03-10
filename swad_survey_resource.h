// swad_survey_resource.c: links to surveys as program resources

#ifndef _SWAD_SVY_RSC
#define _SWAD_SVY_RSC
/*
    SWAD (Shared Workspace At a Distance),
    is a web platform developed at the University of Granada (Spain),
    and used to support university teaching.

    This file is part of SWAD core.
    Copyright (C) 1999-2023 Antonio Ca�as Vargas

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

#include <stddef.h>		// For size_t

/*****************************************************************************/
/***************************** Public prototypes *****************************/
/*****************************************************************************/

void SvyRsc_GetLinkToSurvey (void);
void SvyRsc_WriteSurveyInCrsProgram (long SvyCod,bool PutFormToGo,
                                     const char *Icon,const char *IconTitle);
void SvyRsc_GetTitleFromSvyCod (long SvyCod,char *Title,size_t TitleSize);

#endif
