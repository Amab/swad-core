// swad_match_result.h: matches results in games using remote control

#ifndef _SWAD_MCH_RES
#define _SWAD_MCH_RES
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
/************************** Public types and constants ***********************/
/*****************************************************************************/

#define MchRes_RESULTS_BOX_ID		"mcr_box"
#define MchRes_RESULTS_TABLE_ID		"mcr_table"

/*****************************************************************************/
/***************************** Public prototypes *****************************/
/*****************************************************************************/

void MchRes_ShowMyMchResultsInCrs (void);
void MchRes_ShowMyMchResultsInGam (void);
void MchRes_ShowMyMchResultsInMch (void);

void MchRes_ShowAllMchResultsInCrs (void);
void MchRes_SelUsrsToViewMchResults (void);
void MchRes_ShowAllMchResultsInGam (void);
void MchRes_ShowAllMchResultsInMch (void);

void MchRes_ShowOneMchResult (void);

#endif
