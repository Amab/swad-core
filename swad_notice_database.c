// swad_notice_database.c: notices (yellow notes) operations with database

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
#include "swad_error.h"
#include "swad_global.h"
#include "swad_notice.h"

/*****************************************************************************/
/************** External global variables from others modules ****************/
/*****************************************************************************/

extern struct Globals Gbl;

/*****************************************************************************/
/***************************** Private constants *****************************/
/*****************************************************************************/

/*****************************************************************************/
/***************************** Private prototypes ****************************/
/*****************************************************************************/

/*****************************************************************************/
/******************* Insert a notice in the table of notices *****************/
/*****************************************************************************/
// Return the code of the new inserted notice

long Not_DB_InsertNotice (const char *Content)
  {
   return
   DB_QueryINSERTandReturnCode ("can not create notice",
				"INSERT INTO not_notices"
				" (CrsCod,UsrCod,CreatTime,Content,Status)"
				" VALUES"
				" (%ld,%ld,NOW(),'%s',%u)",
				Gbl.Hierarchy.Crs.CrsCod,
				Gbl.Usrs.Me.UsrDat.UsrCod,
				Content,
				(unsigned) Not_ACTIVE_NOTICE);
  }

/*****************************************************************************/
/********************** Mark as active/hidden a notice ***********************/
/*****************************************************************************/

void Not_DB_ChangeNoticeStatus (long NotCod,Not_Status_t Status)
  {
   DB_QueryUPDATE ("can not reveal notice",
		   "UPDATE not_notices"
		     " SET Status=%u"
		   " WHERE NotCod=%ld"
		     " AND CrsCod=%ld",
	           (unsigned) Status,
	           NotCod,
	           Gbl.Hierarchy.Crs.CrsCod);
  }

/*****************************************************************************/
/****************** Copy notice to table of deleted notices ******************/
/*****************************************************************************/

void Not_DB_CopyNoticeToDeleted (long NotCod)
  {
   DB_QueryINSERT ("can not remove notice",
		   "INSERT IGNORE INTO not_deleted"
		   " (NotCod,CrsCod,UsrCod,CreatTime,Content,NumNotif)"
		   " SELECT NotCod,"
		           "CrsCod,"
		           "UsrCod,"
		           "CreatTime,"
		           "Content,"
		           "NumNotif"
		     " FROM not_notices"
		    " WHERE NotCod=%ld"
		      " AND CrsCod=%ld",	// Extra check
                   NotCod,
                   Gbl.Hierarchy.Crs.CrsCod);
  }

/*****************************************************************************/
/*********** Update number of users notified in table of notices *************/
/*****************************************************************************/

void Not_DB_UpdateNumUsrsNotifiedByEMailAboutNotice (long NotCod,
                                                     unsigned NumUsrsToBeNotifiedByEMail)
  {
   DB_QueryUPDATE ("can not update the number of notifications of a notice",
		   "UPDATE not_notices"
		     " SET NumNotif=%u"
		   " WHERE NotCod=%ld",
	           NumUsrsToBeNotifiedByEMail,
	           NotCod);
  }

/*****************************************************************************/
/**************************** Get data of a notice ***************************/
/*****************************************************************************/

unsigned Not_DB_GetDataOfNotice (MYSQL_RES **mysql_res,long NotCod)
  {
   return (unsigned)
   DB_QuerySELECT (mysql_res,"can not get notice from database",
		   "SELECT UNIX_TIMESTAMP(CreatTime) AS F,"	// row[0]
			  "UsrCod,"				// row[1]
			  "Content,"				// row[2]
			  "Status"				// row[3]
		    " FROM not_notices"
		   " WHERE NotCod=%ld"
		     " AND CrsCod=%ld",	// Extra check
		   NotCod,
		   Gbl.Hierarchy.Crs.CrsCod);
  }

/*****************************************************************************/
/********************* Get content of message from database ******************/
/*****************************************************************************/

unsigned Not_DB_ContentNotice (MYSQL_RES **mysql_res,long NotCod)
  {
   return (unsigned)
   DB_QuerySELECT (mysql_res,"can not get content of notice",
		   "SELECT Content"		// row[0]
		    " FROM not_notices"
		   " WHERE NotCod=%ld",
		   NotCod);
  }

/*****************************************************************************/
/******************************* Get all notices *****************************/
/*****************************************************************************/

unsigned Not_DB_GetAllNotices (MYSQL_RES **mysql_res)
  {
   return (unsigned)
   DB_QuerySELECT (mysql_res,"can not get notices from database",
		   "SELECT NotCod,"				// row[0]
			  "UNIX_TIMESTAMP(CreatTime) AS F,"	// row[1]
			  "UsrCod,"				// row[2]
			  "Content,"				// row[3]
			  "Status"				// row[4]
		    " FROM not_notices"
		   " WHERE CrsCod=%ld"
		   " ORDER BY CreatTime DESC",
		   Gbl.Hierarchy.Crs.CrsCod);
  }

/*****************************************************************************/
/***************************** Get active notices ****************************/
/*****************************************************************************/

unsigned Not_DB_GetActiveNotices (MYSQL_RES **mysql_res,long CrsCod)
  {
   return (unsigned)
   DB_QuerySELECT (mysql_res,"can not get notices from database",
		   "SELECT NotCod,"			// row[0]
			  "UNIX_TIMESTAMP(CreatTime),"	// row[1]
			  "UsrCod,"			// row[2]
			  "Content,"			// row[3]
			  "Status"			// row[4]
		    " FROM not_notices"
		   " WHERE CrsCod=%ld"
		     " AND Status=%u"
		   " ORDER BY CreatTime DESC",
		   CrsCod,
		   (unsigned) Not_ACTIVE_NOTICE);
  }

/*****************************************************************************/
/*************************** Get number of notices ***************************/
/*****************************************************************************/

unsigned Not_DB_GetNumNotices (MYSQL_RES **mysql_res,
                               HieLvl_Level_t Scope,Not_Status_t Status)
  {
   switch (Scope)
     {
      case HieLvl_SYS:
	 return (unsigned)
         DB_QuerySELECT (mysql_res,"can not get number of notices",
			 "SELECT COUNT(*),"			// row[0]
			        "SUM(NumNotif)"			// row[1]
			  " FROM not_notices"
			 " WHERE Status=%u",
                         Status);
      case HieLvl_CTY:
	 return (unsigned)
         DB_QuerySELECT (mysql_res,"can not get number of notices",
			 "SELECT COUNT(*),"			// row[0]
			        "SUM(not_notices.NumNotif)"	// row[1]
			  " FROM ins_instits,"
			        "ctr_centers,"
			        "deg_degrees,"
			        "crs_courses,"
			        "not_notices"
			 " WHERE ins_instits.CtyCod=%ld"
			   " AND ins_instits.InsCod=ctr_centers.InsCod"
			   " AND ctr_centers.CtrCod=deg_degrees.CtrCod"
			   " AND deg_degrees.DegCod=crs_courses.DegCod"
			   " AND crs_courses.CrsCod=not_notices.CrsCod"
			   " AND not_notices.Status=%u",
                         Gbl.Hierarchy.Cty.CtyCod,
                         Status);
      case HieLvl_INS:
	 return (unsigned)
         DB_QuerySELECT (mysql_res,"can not get number of notices",
			 "SELECT COUNT(*),"			// row[0]
			        "SUM(not_notices.NumNotif)"	// row[1]
			  " FROM ctr_centers,"
			        "deg_degrees,"
			        "crs_courses,"
			        "not_notices"
			 " WHERE ctr_centers.InsCod=%ld"
			   " AND ctr_centers.CtrCod=deg_degrees.CtrCod"
			   " AND deg_degrees.DegCod=crs_courses.DegCod"
			   " AND crs_courses.CrsCod=not_notices.CrsCod"
			   " AND not_notices.Status=%u",
                         Gbl.Hierarchy.Ins.InsCod,
                         Status);
      case HieLvl_CTR:
	 return (unsigned)
         DB_QuerySELECT (mysql_res,"can not get number of notices",
			 "SELECT COUNT(*),"			// row[0]
			        "SUM(not_notices.NumNotif)"	// row[1]
			  " FROM deg_degrees,"
			        "crs_courses,"
			        "not_notices"
			 " WHERE deg_degrees.CtrCod=%ld"
			   " AND deg_degrees.DegCod=crs_courses.DegCod"
			   " AND crs_courses.CrsCod=not_notices.CrsCod"
			   " AND not_notices.Status=%u",
                         Gbl.Hierarchy.Ctr.CtrCod,
                         Status);
      case HieLvl_DEG:
	 return (unsigned)
         DB_QuerySELECT (mysql_res,"can not get number of notices",
			 "SELECT COUNT(*),"			// row[0]
			        "SUM(not_notices.NumNotif)"	// row[1]
			  " FROM crs_courses,"
			        "not_notices"
			 " WHERE crs_courses.DegCod=%ld"
			   " AND crs_courses.CrsCod=not_notices.CrsCod"
			   " AND not_notices.Status=%u",
                         Gbl.Hierarchy.Deg.DegCod,
                         Status);
      case HieLvl_CRS:
	 return (unsigned)
         DB_QuerySELECT (mysql_res,"can not get number of notices",
			 "SELECT COUNT(*),"			// row[0]
			        "SUM(NumNotif)"			// row[1]
			  " FROM not_notices"
			 " WHERE CrsCod=%ld"
			   " AND Status=%u",
                         Gbl.Hierarchy.Crs.CrsCod,
                         Status);
      default:
	 Err_WrongScopeExit ();
	 return 0;	// Not reached
     }
  }

/*****************************************************************************/
/********************** Get number of deleted notices ************************/
/*****************************************************************************/

unsigned Not_DB_GetNumNoticesDeleted (MYSQL_RES **mysql_res,
                                      HieLvl_Level_t Scope)
  {
   switch (Scope)
     {
      case HieLvl_SYS:
	 return (unsigned)
         DB_QuerySELECT (mysql_res,"can not get number of deleted notices",
			 "SELECT COUNT(*),"			// row[0]
			        "SUM(NumNotif)"			// row[1]
			  " FROM not_deleted");
      case HieLvl_CTY:
	 return (unsigned)
         DB_QuerySELECT (mysql_res,"can not get number of deleted notices",
			 "SELECT COUNT(*),"			// row[0]
			        "SUM(not_deleted.NumNotif)"	// row[1]
			  " FROM ins_instits,"
			        "ctr_centers,"
			        "deg_degrees,"
			        "crs_courses,"
			        "not_deleted"
			 " WHERE ins_instits.CtyCod=%ld"
			   " AND ins_instits.InsCod=ctr_centers.InsCod"
			   " AND ctr_centers.CtrCod=deg_degrees.CtrCod"
			   " AND deg_degrees.DegCod=crs_courses.DegCod"
			   " AND crs_courses.CrsCod=not_deleted.CrsCod",
                         Gbl.Hierarchy.Cty.CtyCod);
      case HieLvl_INS:
	 return (unsigned)
         DB_QuerySELECT (mysql_res,"can not get number of deleted notices",
			 "SELECT COUNT(*),"			// row[0]
			        "SUM(not_deleted.NumNotif)"	// row[1]
			  " FROM ctr_centers,"
			        "deg_degrees,"
			        "crs_courses,"
			        "not_deleted"
			 " WHERE ctr_centers.InsCod=%ld"
			   " AND ctr_centers.CtrCod=deg_degrees.CtrCod"
			   " AND deg_degrees.DegCod=crs_courses.DegCod"
			   " AND crs_courses.CrsCod=not_deleted.CrsCod",
                         Gbl.Hierarchy.Ins.InsCod);
      case HieLvl_CTR:
	 return (unsigned)
         DB_QuerySELECT (mysql_res,"can not get number of deleted notices",
			 "SELECT COUNT(*),"			// row[0]
			        "SUM(not_deleted.NumNotif)"	// row[1]
			  " FROM deg_degrees,"
			        "crs_courses,"
			        "not_deleted"
			 " WHERE deg_degrees.CtrCod=%ld"
			 " AND deg_degrees.DegCod=crs_courses.DegCod"
			 " AND crs_courses.CrsCod=not_deleted.CrsCod",
                         Gbl.Hierarchy.Ctr.CtrCod);
      case HieLvl_DEG:
	 return (unsigned)
         DB_QuerySELECT (mysql_res,"can not get number of deleted notices",
			 "SELECT COUNT(*),"			// row[0]
			        "SUM(not_deleted.NumNotif)"	// row[1]
			  " FROM crs_courses,"
			        "not_deleted"
			 " WHERE crs_courses.DegCod=%ld"
			   " AND crs_courses.CrsCod=not_deleted.CrsCod",
                         Gbl.Hierarchy.Deg.DegCod);
      case HieLvl_CRS:
	 return (unsigned)
         DB_QuerySELECT (mysql_res,"can not get number of deleted notices",
			 "SELECT COUNT(*),"			// row[0]
			        "SUM(NumNotif)"			// row[1]
			  " FROM not_deleted"
			 " WHERE CrsCod=%ld",
                         Gbl.Hierarchy.Crs.CrsCod);
      default:
	 Err_WrongScopeExit ();
	 return 0;	// Not reached
     }
  }

/*****************************************************************************/
/******************************** Remove notice ******************************/
/*****************************************************************************/

void Not_DB_RemoveNotice (long NotCod)
  {
   DB_QueryDELETE ("can not remove notice",
		   "DELETE FROM not_notices"
		   " WHERE NotCod=%ld"
		     " AND CrsCod=%ld",	// Extra check
                   NotCod,
                   Gbl.Hierarchy.Crs.CrsCod);
  }

/*****************************************************************************/
/************************* Remove notices in a course ************************/
/*****************************************************************************/

void Not_DB_RemoveCrsNotices (long CrsCod)
  {
   /***** Copy all notices from the course to table of deleted notices *****/
   DB_QueryINSERT ("can not remove notices in a course",
		   "INSERT INTO not_deleted"
		   " (NotCod,CrsCod,UsrCod,CreatTime,Content,NumNotif)"
		   " SELECT NotCod,"
			   "CrsCod,"
			   "UsrCod,"
			   "CreatTime,"
			   "Content,"
			   "NumNotif"
		    " FROM not_notices"
		   " WHERE CrsCod=%ld",
		   CrsCod);

   /***** Remove all notices from the course *****/
   DB_QueryDELETE ("can not remove notices in a course",
		   "DELETE FROM not_notices"
		   " WHERE CrsCod=%ld",
		   CrsCod);
  }
