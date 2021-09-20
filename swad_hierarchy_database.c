// swad_hierarchy_database.c: hierarchy (system, institution, center, degree, course), operations with database

/*
    SWAD (Shared Workspace At a Distance),
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

#include "swad_database.h"
#include "swad_hierarchy_database.h"
#include "swad_hierarchy_level.h"
#include "swad_scope.h"

/*****************************************************************************/
/************** External global variables from others modules ****************/
/*****************************************************************************/

/*****************************************************************************/
/*************************** Public constants ********************************/
/*****************************************************************************/

/*****************************************************************************/
/***************************** Private types *********************************/
/*****************************************************************************/

/*****************************************************************************/
/*************************** Private constants *******************************/
/*****************************************************************************/

/*****************************************************************************/
/*************************** Private prototypes ******************************/
/*****************************************************************************/

/*****************************************************************************/
/****** Get institutions, centers and degrees administrated by an admin *****/
/*****************************************************************************/

unsigned Hie_DB_GetInsCtrDegAdminBy (MYSQL_RES **mysql_res,long UsrCod)
  {
   return (unsigned)
   DB_QuerySELECT (mysql_res,"can not get institutions, centers, degrees"
			     " admin by a user",
		   "(SELECT %u AS S,"			// row[0]
		           "-1 AS Cod,"			// row[1]
		           "'' AS FullName"		// row[2]
		     " FROM usr_admins"
		    " WHERE UsrCod=%ld"
		      " AND Scope='%s')"
		   " UNION "
		   "(SELECT %u AS S,"			// row[0]
			   "usr_admins.Cod,"		// row[1]
			   "ins_instits.FullName"	// row[2]
		     " FROM usr_admins,"
			   "ins_instits"
		    " WHERE usr_admins.UsrCod=%ld"
		      " AND usr_admins.Scope='%s'"
		      " AND usr_admins.Cod=ins_instits.InsCod)"
		   " UNION "
		   "(SELECT %u AS S,"			// row[0]
			   "usr_admins.Cod,"		// row[1]
			   "ctr_centers.FullName"	// row[2]
		     " FROM usr_admins,"
			   "ctr_centers"
		    " WHERE usr_admins.UsrCod=%ld"
		      " AND usr_admins.Scope='%s'"
		      " AND usr_admins.Cod=ctr_centers.CtrCod)"
		   " UNION "
		   "(SELECT %u AS S,"			// row[0]
			   "usr_admins.Cod,"		// row[1]
			   "deg_degrees.FullName"	// row[2]
		     " FROM usr_admins,"
		           "deg_degrees"
		    " WHERE usr_admins.UsrCod=%ld"
		      " AND usr_admins.Scope='%s'"
		      " AND usr_admins.Cod=deg_degrees.DegCod)"
		   " ORDER BY S,"
		             "FullName",
		   (unsigned) HieLvl_SYS,UsrCod,Sco_GetDBStrFromScope (HieLvl_SYS),
		   (unsigned) HieLvl_INS,UsrCod,Sco_GetDBStrFromScope (HieLvl_INS),
		   (unsigned) HieLvl_CTR,UsrCod,Sco_GetDBStrFromScope (HieLvl_CTR),
		   (unsigned) HieLvl_DEG,UsrCod,Sco_GetDBStrFromScope (HieLvl_DEG));
  }
