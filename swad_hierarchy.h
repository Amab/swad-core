// swad_hierarchy.h: hierarchy (system, institution, center, degree, course)

#ifndef _SWAD_HIE
#define _SWAD_HIE
/*
    SWAD (Shared Workspace At a Distance in Spanish),
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

#include "swad_center.h"
#include "swad_country.h"
#include "swad_course.h"
#include "swad_degree.h"
#include "swad_institution.h"

/*****************************************************************************/
/***************************** Public prototypes *****************************/
/*****************************************************************************/

void Hie_SeePending (void);

void Hie_WriteMenuHierarchy (void);
void Hie_WriteHierarchyInBreadcrumb (void);
void Hie_WriteBigNameCtyInsCtrDegCrs (void);

void Hie_SetHierarchyFromUsrLastHierarchy (void);
void Hie_InitHierarchy (void);
void Hie_ResetHierarchy (void);

void Hie_GetAndWriteInsCtrDegAdminBy (long UsrCod,unsigned ColSpan);

bool Hie_CheckIfICanEdit (void);

void Hie_WriteStatusCell (Hie_Status_t Status,
			  const char *Class,const char *BgColor,
			  const char *Txt[Hie_NUM_STATUS_TXT]);
void Hie_WriteStatusCellEditable (bool ICanEdit,Hie_Status_t Status,
                                  Act_Action_t NextAction,long HieCod,
                                  const char *Txt[Hie_NUM_STATUS_TXT]);
Hie_Status_t Hie_GetParStatus (void);
void Hie_PutParOtherHieCod (void *HieCod);

Hie_Order_t Hie_GetParHieOrder (void);

void Hie_FreeList (HieLvl_Level_t Level);

void Hie_FlushCacheUsrBelongsTo (HieLvl_Level_t Level);
bool Hie_CheckIfUsrBelongsTo (HieLvl_Level_t Level,long UsrCod,long HieCod,
                              bool CountOnlyAcceptedCourses);

//-------------------------------- Figures ------------------------------------
void Hie_GetAndShowHierarchyStats (void);

unsigned Hie_GetCachedNumNodesInSys (FigCch_FigureCached_t Figure,
				     const char *Table);

void Hie_FlushCachedFigureInHieLvl (FigCch_FigureCached_t Figure,
			            HieLvl_Level_t Level);
unsigned Hie_GetCachedFigureInHieLvl (FigCch_FigureCached_t Figure,
				      HieLvl_Level_t Level,long HieCod);
unsigned Hie_GetFigureInHieLvl (FigCch_FigureCached_t Figure,
		      	        HieLvl_Level_t Level,long HieCod);

#endif
