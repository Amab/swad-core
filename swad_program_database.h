// swad_program_database.h: course program, operations with database

#ifndef _SWAD_PRG_DB
#define _SWAD_PRG_DB
/*
    SWAD (Shared Workspace At a Distance),
    is a web platform developed at the University of Granada (Spain),
    and used to support university teaching.

    This file is part of SWAD core.
    Copyright (C) 1999-2024 Antonio Ca�as Vargas

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

#include "swad_contracted_expanded.h"
#include "swad_hierarchy_type.h"
#include "swad_program.h"
#include "swad_program_resource.h"

/*****************************************************************************/
/***************************** Public prototypes *****************************/
/*****************************************************************************/

//------------------------------ Resources ------------------------------------
long Prg_DB_CreateResource (const struct Tre_Node *Node);

void Prg_DB_UpdateResourceTitle (long NodCod,long RscCod,
                                 const char NewTitle[Rsc_MAX_BYTES_RESOURCE_TITLE + 1]);

unsigned Prg_DB_GetListResources (MYSQL_RES **mysql_res,long NodCod,
                                  bool ShowHiddenResources);
unsigned Prg_DB_GetResourceDataByCod (MYSQL_RES **mysql_res,long RscCod);

unsigned Prg_DB_GetRscIndBefore (long NodCod,unsigned RscInd);
unsigned Prg_DB_GetRscIndAfter (long NodCod,unsigned RscInd);
long Prg_DB_GetRscCodFromRscInd (long NodCod,unsigned RscInd);

void Prg_DB_RemoveResource (const struct Tre_Node *Node);

void Prg_DB_HideOrUnhideResource (long RscCod,
				  HidVis_HiddenOrVisible_t HiddenOrVisible);

void Prg_DB_LockTableResources (void);
void Prg_DB_UpdateRscInd (long RscCod,int RscInd);
void Prg_DB_UpdateRscLink (const struct Tre_Node *Node);

#endif
