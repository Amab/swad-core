// swad_link_database.c: institutional links, operations with database

/*
    SWAD (Shared Workspace At a Distance),
    is a web platform developed at the University of Granada (Spain),
    and used to support university teaching.

    This file is part of SWAD core.
    Copyright (C) 1999-2022 Antonio Ca�as Vargas

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

#include "swad_database.h"
#include "swad_link_database.h"

/*****************************************************************************/
/**************************** Create a new link ******************************/
/*****************************************************************************/

void Lnk_DB_CreateLink (const struct Lnk_Link *Lnk)
  {
   DB_QueryINSERT ("can not create institutional link",
		   "INSERT INTO lnk_links"
		   " (ShortName,FullName,WWW)"
		   " VALUES"
		   " ('%s','%s','%s')",
                   Lnk->ShrtName,
                   Lnk->FullName,
                   Lnk->WWW);
  }

/*****************************************************************************/
/************ Update link name in table of institutional links ***************/
/*****************************************************************************/

void Lnk_DB_UpdateLnkName (long LnkCod,const char *FldName,const char *NewLnkName)
  {
   DB_QueryUPDATE ("can not update the name of an institutional link",
		   "UPDATE lnk_links"
		     " SET %s='%s'"
		   " WHERE LnkCod=%ld",
	           FldName,NewLnkName,
	           LnkCod);
  }


/*****************************************************************************/
/****************** Update link changing old WWW by new WWW ******************/
/*****************************************************************************/

void Lnk_DB_UpdateLnkWWW (long LnkCod,const char NewWWW[Cns_MAX_BYTES_WWW + 1])
  {
   DB_QueryUPDATE ("can not update the web of an institutional link",
		   "UPDATE lnk_links"
		     " SET WWW='%s'"
		   " WHERE LnkCod=%ld",
		   NewWWW,
		   LnkCod);
  }

/*****************************************************************************/
/************************** Get institutional links **************************/
/*****************************************************************************/

unsigned Lnk_DB_GetLinks (MYSQL_RES **mysql_res)
  {
   return (unsigned)
   DB_QuerySELECT (mysql_res,"can not get institutional links",
		   "SELECT LnkCod,"	// row[0]
			  "ShortName,"	// row[1]
			  "FullName,"	// row[2]
			  "WWW"		// row[3]
		    " FROM lnk_links"
		   " ORDER BY ShortName");
  }

/*****************************************************************************/
/**************************** Get link full name *****************************/
/*****************************************************************************/

unsigned Lnk_DB_GetDataOfLinkByCod (MYSQL_RES **mysql_res,long LnkCod)
  {
   return (unsigned)
   DB_QuerySELECT (mysql_res,"can not get data of an institutional link",
		   "SELECT LnkCod,"	// row[0]
		          "ShortName,"	// row[1]
			  "FullName,"	// row[2]
			  "WWW"		// row[3]
		    " FROM lnk_links"
		   " WHERE LnkCod=%ld",
		   LnkCod);
  }

/*****************************************************************************/
/********************** Check if the name of link exists *********************/
/*****************************************************************************/

bool Lnk_DB_CheckIfLinkNameExists (const char *FldName,const char *Name,long LnkCod)
  {
   return
   DB_QueryEXISTS ("can not check if the name of an institutional link already existed",
		   "SELECT EXISTS"
		   "(SELECT *"
		     " FROM lnk_links"
		    " WHERE %s='%s'"
		      " AND LnkCod<>%ld)",
		   FldName,Name,
		   LnkCod);
  }

/*****************************************************************************/
/******************************* Remove a link *******************************/
/*****************************************************************************/

void Lnk_DB_RemoveLink (long LnkCod)
  {
   DB_QueryDELETE ("can not remove an institutional link",
		   "DELETE FROM lnk_links"
		   " WHERE LnkCod=%ld",
		   LnkCod);
  }
