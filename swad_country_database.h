// swad_country_database.h: countries operations with database

#ifndef _SWAD_CTY_DB
#define _SWAD_CTY_DB
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

#include "swad_country.h"
#include "swad_map.h"
#include "swad_media.h"

/*****************************************************************************/
/************************** Public types and constants ***********************/
/*****************************************************************************/

/*****************************************************************************/
/***************************** Public prototypes *****************************/
/*****************************************************************************/

void Cty_DB_CreateCountry (const struct Cty_Countr *Cty);

unsigned Cty_DB_GetCtysBasic (MYSQL_RES **mysql_res);
unsigned Cty_DB_GetCtysWithPendingInss (MYSQL_RES **mysql_res);
unsigned Cty_DB_GetCtysFull (MYSQL_RES **mysql_res);
unsigned Cty_DB_GetDataOfCountryByCod (MYSQL_RES **mysql_res,long CtyCod);
void Cty_DB_GetCountryName (long CtyCod,Lan_Language_t Language,
			    char CtyName[Cty_MAX_BYTES_NAME + 1]);

unsigned Cty_DB_GetNumCtysWithInss (void);
unsigned Cty_DB_GetNumCtysWithCtrs (void);
unsigned Cty_DB_GetNumCtysWithDegs (void);
unsigned Cty_DB_GetNumCtysWithCrss (void);
unsigned Cty_DB_GetNumCtysWithUsrs (Rol_Role_t Role,const char *SubQuery);

bool Cty_DB_CheckIfNumericCountryCodeExists (long CtyCod);
bool Cty_DB_CheckIfAlpha2CountryCodeExists (const char Alpha2[2 + 1]);
bool Cty_DB_CheckIfCountryNameExists (Lan_Language_t Language,const char *Name,long CtyCod);

void Cty_DB_GetCoordAndZoom (struct Map_Coordinates *Coord,unsigned *Zoom);
unsigned Cty_DB_GetCtrsWithCoordsInCurrentCty (MYSQL_RES **mysql_res);
unsigned Cty_DB_GetMapAttr (MYSQL_RES **mysql_res,long CtyCod);
bool Cty_DB_GetIfMapIsAvailable (long CtyCod);

void Cty_DB_UpdateCtyField (long CtyCod,const char *FieldName,const char *FieldValue);
void Cty_DB_UpdateCtyMapAttr (const char NewMapAttribution[Med_MAX_BYTES_ATTRIBUTION + 1]);

void Cty_DB_RemoveCty (long CtyCod);

#endif
