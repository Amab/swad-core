// swad_holiday_database.c: holidays operations with database

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

// #include <stddef.h>		// For NULL
// #include <stdlib.h>		// For calloc
// #include <string.h>		// For string functions

// #include "swad_box.h"
// #include "swad_calendar.h"
#include "swad_database.h"
// #include "swad_error.h"
// #include "swad_form.h"
#include "swad_global.h"
// #include "swad_holiday.h"
#include "swad_holiday_database.h"
// #include "swad_HTML.h"
// #include "swad_language.h"
// #include "swad_parameter.h"

/*****************************************************************************/
/************** External global variables from others modules ****************/
/*****************************************************************************/

extern struct Globals Gbl;

/*****************************************************************************/
/***************************** Private constants *****************************/
/*****************************************************************************/

/*****************************************************************************/
/******************************* Private types *******************************/
/*****************************************************************************/

/*****************************************************************************/
/***************************** Private variables *****************************/
/*****************************************************************************/

/*****************************************************************************/
/***************************** Private prototypes ****************************/
/*****************************************************************************/

/*****************************************************************************/
/****************** Create a new holiday or no school period *****************/
/*****************************************************************************/

void Hld_DB_CreateHoliday (const struct Hld_Holiday *Hld)
  {
   DB_QueryINSERT ("can not create holiday",
		   "INSERT INTO hld_holidays"
		   " (InsCod,PlcCod,HldTyp,StartDate,EndDate,Name)"
		   " VALUES"
		   " (%ld,%ld,%u,'%04u%02u%02u','%04u%02u%02u','%s')",
	           Gbl.Hierarchy.Ins.InsCod,
	           Hld->PlcCod,
	           (unsigned) Hld->HldTyp,
	           Hld->StartDate.Year,
	           Hld->StartDate.Month,
	           Hld->StartDate.Day,
	           Hld->EndDate.Year,
	           Hld->EndDate.Month,
	           Hld->EndDate.Day,
	           Hld->Name);
  }

/*****************************************************************************/
/*************************** List all the holidays ***************************/
/*****************************************************************************/

unsigned Hld_DB_GetListHolidays (MYSQL_RES **mysql_res,Hld_Order_t SelectedOrder)
  {
   static const char *OrderBySubQuery[Hld_NUM_ORDERS] =
     {
      [Hld_ORDER_BY_PLACE     ] = "Place,"
	                          "StartDate",
      [Hld_ORDER_BY_START_DATE] = "StartDate,"
	                          "Place",
     };

   return (unsigned)
   DB_QuerySELECT (mysql_res,"can not get holidays",
		   "(SELECT hld_holidays.HldCod,"						// row[0]
			   "hld_holidays.PlcCod,"						// row[1]
			   "plc_places.FullName as Place,"					// row[2]
			   "hld_holidays.HldTyp,"						// row[3]
			   "DATE_FORMAT(hld_holidays.StartDate,'%%Y%%m%%d') AS StartDate,"	// row[4]
			   "DATE_FORMAT(hld_holidays.EndDate,'%%Y%%m%%d') AS EndDate,"		// row[5]
			   "hld_holidays.Name"							// row[6]
		     " FROM hld_holidays,"
			   "plc_places"
		    " WHERE hld_holidays.InsCod=%ld"
		      " AND hld_holidays.PlcCod=plc_places.PlcCod"
		      " AND plc_places.InsCod=%ld)"
		   " UNION "
		   "(SELECT HldCod,"								// row[0]
			   "PlcCod,"								// row[1]
			   "'' as Place,"							// row[2]
			   "HldTyp,"								// row[3]
			   "DATE_FORMAT(StartDate,'%%Y%%m%%d') AS StartDate,"		// row[4]
			   "DATE_FORMAT(EndDate,'%%Y%%m%%d') AS EndDate,"			// row[5]
			   "Name"								// row[6]
		     " FROM hld_holidays"
		    " WHERE InsCod=%ld"
		      " AND PlcCod NOT IN"
			   "(SELECT DISTINCT PlcCod"
			     " FROM plc_places"
			    " WHERE InsCod=%ld))"
		   " ORDER BY %s",
		   Gbl.Hierarchy.Ins.InsCod,
		   Gbl.Hierarchy.Ins.InsCod,
		   Gbl.Hierarchy.Ins.InsCod,
		   Gbl.Hierarchy.Ins.InsCod,
		   OrderBySubQuery[SelectedOrder]);
  }

/*****************************************************************************/
/************************* Get holiday data by code **************************/
/*****************************************************************************/

unsigned Hld_DB_GetDataOfHolidayByCod (MYSQL_RES **mysql_res,long HldCod)
  {
   return (unsigned)
   DB_QuerySELECT (mysql_res,"can not get data of a holiday",
		   "(SELECT hld_holidays.PlcCod,"				// row[0]
			   "plc_places.FullName as Place,"			// row[1]
			   "hld_holidays.HldTyp,"				// row[2]
			   "DATE_FORMAT(hld_holidays.StartDate,'%%Y%%m%%d'),"	// row[3]
			   "DATE_FORMAT(hld_holidays.EndDate,'%%Y%%m%%d'),"	// row[4]
			   "hld_holidays.Name"					// row[5]
		     " FROM hld_holidays,"
			   "plc_places"
		    " WHERE hld_holidays.HldCod=%ld"
		      " AND hld_holidays.InsCod=%ld"
		      " AND hld_holidays.PlcCod=plc_places.PlcCod"
		      " AND plc_places.InsCod=%ld)"
		   " UNION "
		   "(SELECT PlcCod,"
			   "'' as Place,"
			   "HldTyp,"
			   "DATE_FORMAT(StartDate,'%%Y%%m%%d'),"
			   "DATE_FORMAT(EndDate,'%%Y%%m%%d'),"
			   "Name"
		     " FROM hld_holidays"
		    " WHERE HldCod=%ld"
		      " AND InsCod=%ld"
		      " AND PlcCod NOT IN"
			   "(SELECT DISTINCT PlcCod"
			     " FROM plc_places"
			    " WHERE InsCod=%ld))",
		   HldCod,
		   Gbl.Hierarchy.Ins.InsCod,
		   Gbl.Hierarchy.Ins.InsCod,
		   HldCod,
		   Gbl.Hierarchy.Ins.InsCod,
		   Gbl.Hierarchy.Ins.InsCod);
  }
