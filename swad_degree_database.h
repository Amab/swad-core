// swad_degree_database.h: degrees operations with database

#ifndef _SWAD_DEG_DB
#define _SWAD_DEG_DB
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

#include "swad_degree.h"
#include "swad_degree_type.h"
#include "swad_search.h"

/*****************************************************************************/
/***************************** Public constants ******************************/
/*****************************************************************************/

/*****************************************************************************/
/******************************* Public types ********************************/
/*****************************************************************************/

/*****************************************************************************/
/***************************** Public prototypes *****************************/
/*****************************************************************************/

void Deg_DB_CreateDegreeType (const char DegTypName[DegTyp_MAX_BYTES_DEGREE_TYPE_NAME + 1]);
void Deg_DB_CreateDegree (struct Deg_Degree *Deg,unsigned Status);

unsigned Deg_DB_GetDegreeTypes (MYSQL_RES **mysql_res,
                                HieLvl_Level_t Scope,DegTyp_Order_t Order);
void Deg_DB_GetDegTypeNameByCod (struct DegreeType *DegTyp);
unsigned Deg_DB_GetDataOfDegreeByCod (MYSQL_RES **mysql_res,long DegCod);
long Deg_DB_GetInsCodOfDegreeByCod (long DegCod);
long Deg_DB_GetCtrCodOfDegreeByCod (long DegCod);
void Deg_DB_GetShortNameOfDegreeByCod (long DegCod,char ShrtName[Cns_HIERARCHY_MAX_BYTES_SHRT_NAME + 1]);
bool Deg_DB_CheckIfDegreeTypeNameExists (const char *DegTypName,long DegTypCod);
unsigned Deg_DB_GetNumDegsOfType (long DegTypCod);
unsigned Deg_DB_GetDegsOfType (MYSQL_RES **mysql_res,long DegTypCod);
unsigned Deg_DB_GetDegsOfCurrentCtrBasic (MYSQL_RES **mysql_res);
unsigned Deg_DB_GetDegsOfCurrentCtrFull (MYSQL_RES **mysql_res);
unsigned Deg_DB_GetDegsWithPendingCrss (MYSQL_RES **mysql_res);
unsigned Deg_DB_GetDegsWithStds (MYSQL_RES **mysql_res);
bool Deg_DB_CheckIfDegNameExistsInCtr (const char *FieldName,const char *Name,
                                       long DegCod,long CtrCod);

unsigned Deg_DB_SearchDegs (MYSQL_RES **mysql_res,
                            const char SearchQuery[Sch_MAX_BYTES_SEARCH_QUERY + 1],
                            const char *RangeQuery);

unsigned Deg_DB_GetNumDegsWithCrss (HieLvl_Level_t Scope,long Cod);
unsigned Deg_DB_GetNumDegsWithUsrs (Rol_Role_t Role,
                                    HieLvl_Level_t Scope,long Cod);

unsigned Deg_DB_GetDegsFromUsr (MYSQL_RES **mysql_res,long UsrCod,long CtrCod);

unsigned Deg_DB_GetNumDegsInCty (long CtyCod);
unsigned Deg_DB_GetNumDegsInIns (long InsCod);
unsigned Deg_DB_GetNumDegsInCtr (long CtrCod);

void Deg_DB_UpdateDegTypName (long DegTypCod,
                              const char NewNameDegTyp[DegTyp_MAX_BYTES_DEGREE_TYPE_NAME + 1]);
void Deg_DB_UpdateDegNameDB (long DegCod,const char *FieldName,
                             const char NewDegName[Cns_HIERARCHY_MAX_BYTES_FULL_NAME + 1]);
void Deg_DB_UpdateDegCtr (long DegCod,long NewCtrCod);
void Deg_DB_UpdateDegTyp (long DegCod,long NewDegTypCod);
void Deg_DB_UpdateDegWWW (long DegCod,const char NewWWW[Cns_MAX_BYTES_WWW + 1]);
void Deg_DB_UpdateDegStatus (long DegCod,Deg_Status_t NewStatus);

void Deg_DB_RemoveDegTyp (long DegTypCod);
void Deg_DB_RemoveDeg (long DegCod);

#endif
