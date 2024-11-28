// swad_link_database.h: course links, operations with database

#ifndef _SWAD_Bib_DB
#define _SWAD_Bib_DB
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

#include <mysql/mysql.h>	// To access MySQL databases

#include "swad_link_type.h"
#include "swad_hidden_visible.h"
#include "swad_tree.h"

/*****************************************************************************/
/***************************** Public prototypes *****************************/
/*****************************************************************************/

long Bib_DB_CreateBibRef (const struct Tre_Node *Node);
unsigned Bib_DB_GetListBibRefs (MYSQL_RES **mysql_res,long NodCod,
                                bool ShowHiddenBibRefs);
unsigned Bib_DB_GetBibRefDataByCod (MYSQL_RES **mysql_res,long BibCod);
unsigned Bib_DB_GetBibIndBefore (const struct Tre_Node *Node);
unsigned Bib_DB_GetBibIndAfter (const struct Tre_Node *Node);
long Bib_DB_GetBibCodFromBibInd (long NodCod,unsigned BibInd);
void Bib_DB_RemoveBibRef (const struct Tre_Node *Node);
void Bib_DB_HideOrUnhideBibRef (const struct Tre_Node *Node,
			        HidVis_HiddenOrVisible_t HiddenOrVisible);
void Bib_DB_LockTableBibRefs (void);
void Bib_DB_UpdateBibInd (const struct Tre_Node *Node,long BibCod,int BibInd);
void Bib_DB_UpdateBibRef (const struct Tre_Node *Node);

#endif