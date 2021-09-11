// swad_institution_database.h: institutions operations with database

#ifndef _SWAD_INS_DB
#define _SWAD_INS_DB
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
/********************************** Headers **********************************/
/*****************************************************************************/

#include "swad_constant.h"
#include "swad_hierarchy_level.h"
#include "swad_map.h"
#include "swad_role_type.h"

/*****************************************************************************/
/************************ Public types and constants *************************/
/*****************************************************************************/

/*****************************************************************************/
/***************************** Public prototypes *****************************/
/*****************************************************************************/

long Ins_DB_CreateInstitution (const struct Ins_Instit *Ins,unsigned Status);
void Ins_DB_UpdateInsCty (long InsCod,long CtyCod);
void Ins_DB_UpdateInsStatus (long InsCod,Ins_Status_t Status);
void Ins_DB_UpdateInsName (long InsCod,const char *FieldName,const char *NewInsName);
void Ins_DB_UpdateInsWWW (long InsCod,const char NewWWW[Cns_MAX_BYTES_WWW + 1]);

unsigned Ins_DB_GetDataOfInstitutionByCod (MYSQL_RES **mysql_res,long InsCod);
void Ins_DB_GetShortNameOfInstitution (long InsCod,char ShrtName[Cns_HIERARCHY_MAX_BYTES_SHRT_NAME + 1]);
unsigned Ins_DB_GetShrtNameAndCtyOfIns (MYSQL_RES **mysql_res,long InsCod);
bool Ins_DB_CheckIfInsNameExistsInCty (const char *FieldName,
                                       const char *Name,
				       long InsCod,
				       long CtyCod);

unsigned Ins_DB_GetAllInsWithPendingCtr (MYSQL_RES **mysql_res);
unsigned Ins_DB_GetInsWithPendingCtrsAdminByMe (MYSQL_RES **mysql_res);

unsigned Ins_DB_GetInssInCtyOrderedByShrtName (MYSQL_RES **mysql_res,long CtyCod);
unsigned Ins_DB_GetInssInCtyOrderedByFullName (MYSQL_RES **mysql_res,long CtyCod);
unsigned Ins_DB_GetFullListOfInssInCty (MYSQL_RES **mysql_res,long CtyCod);

unsigned Ins_DB_GetInssOrderedByNumCtrs (MYSQL_RES **mysql_res);
unsigned Ins_DB_GetInssOrderedByNumDegs (MYSQL_RES **mysql_res);
unsigned Ins_DB_GetInssOrderedByNumCrss (MYSQL_RES **mysql_res);
unsigned Ins_DB_GetInssOrderedByNumUsrsInCrss (MYSQL_RES **mysql_res);
unsigned Ins_DB_GetInssOrderedByNumUsrsWhoClaimToBelongToThem (MYSQL_RES **mysql_res);

unsigned Ins_DB_GetNumInssInCty (long CtyCod);

unsigned Ins_DB_GetNumInssWithCtrs (HieLvl_Level_t Scope,long Cod);
unsigned Ins_DB_GetNumInssWithDegs (HieLvl_Level_t Scope,long Cod);
unsigned Ins_DB_GetNumInssWithCrss (HieLvl_Level_t Scope,long Cod);
unsigned Ins_DB_GetNumInnsWithUsrs (Rol_Role_t Role,
                                    HieLvl_Level_t Scope,long Cod);

bool Ins_DB_GetIfMapIsAvailable (long InsCod);
void Ins_DB_GetCoordAndZoom (struct Map_Coordinates *Coord,unsigned *Zoom);
unsigned Ins_DB_GetCtrsWithCoordsInCurrentIns (MYSQL_RES **mysql_res);

void Ins_DB_RemoveInstitution (long InsCod);

#endif
