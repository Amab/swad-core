// swad_course_database.c: edition of courses operations with database

#ifndef _SWAD_CRS_DB
#define _SWAD_CRS_DB
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

#include "swad_constant.h"
#include "swad_course.h"
#include "swad_role.h"

/*****************************************************************************/
/***************************** Public prototypes *****************************/
/*****************************************************************************/

void Crs_DB_CreateCourse (struct Crs_Course *Crs,Hie_Status_t Status);

unsigned Crs_DB_GetCrssInDeg (MYSQL_RES **mysql_res,long DegCod);
unsigned Crs_DB_GetCrssInCurrentDegBasic (MYSQL_RES **mysql_res);
unsigned Crs_DB_GetCrssInCurrentDegFull (MYSQL_RES **mysql_res);
unsigned Crs_DB_GetDataOfCourseByCod (MYSQL_RES **mysql_res,long CrsCod);
long Crs_DB_GetCurrentDegCodFromCurrentCrsCod (void);
void Crs_DB_GetShortNamesByCod (long CrsCod,
                                char CrsShortName[Cns_HIERARCHY_MAX_BYTES_SHRT_NAME + 1],
                                char DegShortName[Cns_HIERARCHY_MAX_BYTES_SHRT_NAME + 1]);
bool Crs_DB_CheckIfCrsCodExists (long CrsCod);
bool Crs_DB_CheckIfCrsNameExistsInYearOfDeg (const char *FieldName,const char *Name,long CrsCod,
                                             long DegCod,unsigned Year);
unsigned Crs_DB_GetCrssOfAUsr (MYSQL_RES **mysql_res,long UsrCod,Rol_Role_t Role);
unsigned Crs_DB_GetOldCrss (MYSQL_RES **mysql_res,unsigned long SecondsWithoutAccess);

unsigned Crs_DB_SearchCrss (MYSQL_RES **mysql_res,
                            const char SearchQuery[Sch_MAX_BYTES_SEARCH_QUERY + 1],
                            const char *RangeQuery);

unsigned Crs_DB_GetNumCrssInCty (long CtyCod);
unsigned Crs_DB_GetNumCrssInIns (long InsCod);
unsigned Crs_DB_GetNumCrssInCtr (long CtrCod);
unsigned Crs_DB_GetNumCrssInDeg (long DegCod);
unsigned Crs_DB_GetNumCrssWithUsrs (Rol_Role_t Role,
                                    HieLvl_Level_t Scope,long Cod);

unsigned Crs_DB_GetCrssFromUsr (MYSQL_RES **mysql_res,long UsrCod,long DegCod);

void Crs_DB_UpdateInstitutionalCrsCod (long CrsCod,const char *NewInstitutionalCrsCod);
void Crs_DB_UpdateCrsYear (long CrsCod,unsigned NewYear);
void Crs_DB_UpdateCrsName (long CrsCod,const char *FieldName,const char *NewCrsName);
void Crs_DB_UpdateCrsDeg (long CrsCod,long DegCod);
void Crs_DB_UpdateCrsStatus (long CrsCod,Hie_Status_t Status);

void Crs_DB_UpdateCrsLastClick (void);

void Crs_DB_RemoveCrsInfo (long CrsCod);
void Crs_DB_RemoveCrsTimetable (long CrsCod);
void Crs_DB_RemoveCrsLast (long CrsCod);
void Crs_DB_RemoveCrs (long CrsCod);

#endif
