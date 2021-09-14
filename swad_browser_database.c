// swad_browser_database.c: file browsers operations with database

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

// #define _GNU_SOURCE 		// For asprintf
// #include <dirent.h>		// For scandir, etc.
// #include <errno.h>		// For errno
// #include <linux/limits.h>	// For PATH_MAX
#include <mysql/mysql.h>	// To access MySQL databases
// #include <stddef.h>		// For NULL
// #include <stdio.h>		// For asprintf
// #include <stdlib.h>		// For exit, system, free, etc
// #include <string.h>		// For string functions
// #include <sys/types.h>		// For lstat, time_t
// #include <sys/stat.h>		// For lstat
// #include <time.h>		// For time
// #include <unistd.h>		// For access, lstat, getpid, chdir, symlink

// #include "swad_box.h"
#include "swad_browser.h"
#include "swad_browser_database.h"
// #include "swad_config.h"
#include "swad_database.h"
#include "swad_error.h"
// #include "swad_figure.h"
// #include "swad_file_extension.h"
// #include "swad_file_MIME.h"
// #include "swad_form.h"
#include "swad_global.h"
// #include "swad_group_database.h"
// #include "swad_hierarchy.h"
// #include "swad_hierarchy_level.h"
// #include "swad_HTML.h"
// #include "swad_ID.h"
// #include "swad_logo.h"
// #include "swad_mark.h"
// #include "swad_notification.h"
// #include "swad_parameter.h"
// #include "swad_photo.h"
// #include "swad_profile.h"
// #include "swad_project.h"
// #include "swad_role.h"
// #include "swad_setting.h"
// #include "swad_string.h"
// #include "swad_timeline.h"
// #include "swad_timeline_note.h"
// #include "swad_zip.h"

/*****************************************************************************/
/******************** Global variables from other modules ********************/
/*****************************************************************************/

extern struct Globals Gbl;

/*****************************************************************************/
/******************************* Private types *******************************/
/*****************************************************************************/

/*****************************************************************************/
/***************************** Public constants ******************************/
/*****************************************************************************/

/*****************************************************************************/
/***************************** Private constants *****************************/
/*****************************************************************************/

/*****************************************************************************/
/***************************** Private variables *****************************/
/*****************************************************************************/

/*****************************************************************************/
/**************************** Private prototypes *****************************/
/*****************************************************************************/

/*****************************************************************************/
/*********************** Store size of a file zone ***************************/
/*****************************************************************************/

void Brw_DB_StoreSizeOfFileZone (void)
  {
   extern const Brw_FileBrowser_t Brw_FileBrowserForDB_files[Brw_NUM_TYPES_FILE_BROWSER];
   long Cod = Brw_GetCodForFileBrowser ();
   long ZoneUsrCod = Brw_GetZoneUsrCodForFileBrowser ();

   /***** Update size of the file browser in database *****/
   DB_QueryREPLACE ("can not store the size of a file zone",
		    "REPLACE INTO brw_sizes"
		    " (FileBrowser,Cod,ZoneUsrCod,"
		      "NumLevels,NumFolders,NumFiles,TotalSize)"
		    " VALUES"
		    " (%u,%ld,%ld,"
		      "%u,'%lu','%lu','%llu')",
	            (unsigned) Brw_FileBrowserForDB_files[Gbl.FileBrowser.Type],
		    Cod,ZoneUsrCod,
	            Gbl.FileBrowser.Size.NumLevls,
	            Gbl.FileBrowser.Size.NumFolds,
	            Gbl.FileBrowser.Size.NumFiles,
	            Gbl.FileBrowser.Size.TotalSiz);
  }

/*****************************************************************************/
/************ Update the date of my last access to file browser **************/
/*****************************************************************************/

void Brw_DB_UpdateDateMyLastAccFileBrowser (Brw_FileBrowser_t FileBrowser)
  {
   long Cod = Brw_GetCodForFileBrowser ();

   DB_QueryREPLACE ("can not update date of last access to a file browser",
		    "REPLACE INTO brw_last"
		    " (UsrCod,FileBrowser,Cod,LastClick)"
		    " VALUES"
		    " (%ld,%u,%ld,NOW())",
	            Gbl.Usrs.Me.UsrDat.UsrCod,
	            (unsigned) FileBrowser,
	            Cod);
  }

/*****************************************************************************/
/************** Get the date of my last access to file browser ***************/
/*****************************************************************************/

unsigned Brw_DB_GetDateMyLastAccFileBrowser (MYSQL_RES **mysql_res,
                                             Brw_FileBrowser_t FileBrowser)
  {
   long Cod = Brw_GetCodForFileBrowser ();

   return (unsigned)
   DB_QuerySELECT (mysql_res,"can not get date-time"
			      " of last access to a file browser",
		   "SELECT UNIX_TIMESTAMP(LastClick)"	// row[0]
		    " FROM brw_last"
		   " WHERE UsrCod=%ld"
		     " AND FileBrowser=%u"
		     " AND Cod=%ld",
		   Gbl.Usrs.Me.UsrDat.UsrCod,
		   (unsigned) FileBrowser,
		   Cod);
  }

/*****************************************************************************/
/**************** Get the group of my last access to a zone ******************/
/*****************************************************************************/

unsigned Brw_DB_GetGrpLastAccFileBrowser (MYSQL_RES **mysql_res,const char *FieldNameDB)
  {
   return (unsigned)
   DB_QuerySELECT (mysql_res,"can not get the group of your last access"
			      " to a file browser",
		   "SELECT %s"	// row[0]
		    " FROM crs_user_settings"
		   " WHERE UsrCod=%ld"
		     " AND CrsCod=%ld",
		   FieldNameDB,
		   Gbl.Usrs.Me.UsrDat.UsrCod,
		   Gbl.Hierarchy.Crs.CrsCod);
  }

/*****************************************************************************/
/***************************** Add path to clipboards ************************/
/*****************************************************************************/

void Brw_DB_AddPathToClipboards (void)
  {
   long Cod = Brw_GetCodForFileBrowser ();
   long WorksUsrCod = Brw_GetZoneUsrCodForFileBrowser ();

   /***** Add path to clipboards *****/
   DB_QueryINSERT ("can not add source of copy to clipboard",
		   "INSERT INTO brw_clipboards"
		   " (UsrCod,FileBrowser,Cod,WorksUsrCod,FileType,Path)"
		   " VALUES"
		   " (%ld,%u,%ld,%ld,%u,'%s')",
	           Gbl.Usrs.Me.UsrDat.UsrCod,
	           (unsigned) Gbl.FileBrowser.Type,
	           Cod,
	           WorksUsrCod,
	           (unsigned) Gbl.FileBrowser.FilFolLnk.Type,
	           Gbl.FileBrowser.FilFolLnk.Full);
  }

/*****************************************************************************/
/************************** Update path in my clipboard **********************/
/*****************************************************************************/

void Brw_DB_UpdatePathInClipboard (void)
  {
   long Cod = Brw_GetCodForFileBrowser ();
   long WorksUsrCod = Brw_GetZoneUsrCodForFileBrowser ();

   /***** Update path in my clipboard *****/
   DB_QueryUPDATE ("can not update source of copy in clipboard",
		   "UPDATE brw_clipboards"
		     " SET FileBrowser=%u,"
		          "Cod=%ld,"
		          "WorksUsrCod=%ld,"
		          "FileType=%u,"
		          "Path='%s'"
		   " WHERE UsrCod=%ld",
	           (unsigned) Gbl.FileBrowser.Type,
	           Cod,
	           WorksUsrCod,
	           (unsigned) Gbl.FileBrowser.FilFolLnk.Type,
	           Gbl.FileBrowser.FilFolLnk.Full,
	           Gbl.Usrs.Me.UsrDat.UsrCod);
  }

/*****************************************************************************/
/********************** Get data of my current clipboard *********************/
/*****************************************************************************/

unsigned Brw_DB_GetMyClipboard (MYSQL_RES **mysql_res)
  {
   return (unsigned)
   DB_QuerySELECT (mysql_res,"can not get source of copy from clipboard",
		   "SELECT FileBrowser,"	// row[0]
		          "Cod,"		// row[1]
		          "WorksUsrCod,"	// row[2]
		          "FileType,"		// row[3]
		          "Path"		// row[4]
		    " FROM brw_clipboards"
		   " WHERE UsrCod=%ld",
		   Gbl.Usrs.Me.UsrDat.UsrCod);
  }

/*****************************************************************************/
/*********************** Get folders of assignments **************************/
/*****************************************************************************/
// Get folder of an assignment when:
// 1. The assignment is visible (not hidden)
// 2. ...and the folder name is not empty (the teacher has set that the user must send work(s) for that assignment)
// 3. ...the assignment is not restricted to groups or (if restricted to groups), the owner of zone belong to any of the groups

unsigned Brw_DB_GetFoldersAssignments (MYSQL_RES **mysql_res,long ZoneUsrCod)
  {
   return (unsigned)
   DB_QuerySELECT (mysql_res,"can not get folders of assignments",
		   "SELECT Folder"		// row[0]
		    " FROM asg_assignments"
		   " WHERE CrsCod=%ld"
		     " AND Hidden='N'"
		     " AND Folder<>''"
		     " AND ("
			   "AsgCod NOT IN"
			   " (SELECT AsgCod"
			      " FROM asg_groups)"
			   " OR "
			   "AsgCod IN"
			   " (SELECT asg_groups.AsgCod"
			      " FROM grp_users,"
				    "asg_groups"
			     " WHERE grp_users.UsrCod=%ld"
			       " AND asg_groups.GrpCod=grp_users.GrpCod)"
			  ")",
		   Gbl.Hierarchy.Crs.CrsCod,
		   ZoneUsrCod);
  }

/*****************************************************************************/
/*********************** Get the size of a file zone *************************/
/*****************************************************************************/

void Brw_DB_GetSizeOfFileBrowser (MYSQL_RES **mysql_res,
                                  Brw_FileBrowser_t FileBrowser)
  {
   switch (Gbl.Scope.Current)
     {
      /* Scope = the whole platform */
      case HieLvl_SYS:
	 switch (FileBrowser)
	   {
	    case Brw_UNKNOWN:
	       DB_QuerySELECT (mysql_res,"can not get size of a file browser",
			       "SELECT COUNT(DISTINCT CrsCod),"			// row[0]
				      "COUNT(DISTINCT GrpCod)-1,"		// row[1]
				      "-1,"					// row[2]
				      "MAX(NumLevels),"				// row[3]
				      "SUM(NumFolders),"			// row[4]
				      "SUM(NumFiles),"				// row[5]
				      "SUM(TotalSize)"				// row[6]
			        " FROM "
	                       "("
	                       "SELECT Cod AS CrsCod,"
				      "-1 AS GrpCod,"
				      "NumLevels,"
				      "NumFolders,"
				      "NumFiles,"
				      "TotalSize"
			       " FROM brw_sizes"
			       " WHERE FileBrowser IN (%u,%u,%u,%u,%u,%u)"
	                       " UNION "
	                       "SELECT grp_types.CrsCod,"
				      "brw_sizes.Cod AS GrpCod,"
				      "brw_sizes.NumLevels,"
				      "brw_sizes.NumFolders,"
				      "brw_sizes.NumFiles,"
				      "brw_sizes.TotalSize"
			        " FROM grp_types,"
			              "grp_groups,"
			              "brw_sizes"
			       " WHERE grp_types.GrpTypCod=grp_groups.GrpTypCod"
			         " AND grp_groups.GrpCod=brw_sizes.Cod"
			         " AND brw_sizes.FileBrowser IN (%u,%u,%u,%u)"
			       ") AS sizes",
			       (unsigned) Brw_ADMI_DOC_CRS,
			       (unsigned) Brw_ADMI_TCH_CRS,
			       (unsigned) Brw_ADMI_SHR_CRS,
			       (unsigned) Brw_ADMI_ASG_USR,
			       (unsigned) Brw_ADMI_WRK_USR,
			       (unsigned) Brw_ADMI_MRK_CRS,
			       (unsigned) Brw_ADMI_DOC_GRP,
			       (unsigned) Brw_ADMI_TCH_GRP,
			       (unsigned) Brw_ADMI_SHR_GRP,
			       (unsigned) Brw_ADMI_MRK_GRP);
	       break;
	    case Brw_ADMI_DOC_CRS:
	    case Brw_ADMI_TCH_CRS:
	    case Brw_ADMI_SHR_CRS:
	    case Brw_ADMI_MRK_CRS:
	       DB_QuerySELECT (mysql_res,"can not get size of a file browser",
			       "SELECT COUNT(DISTINCT Cod),"			// row[0]
				      "-1,"					// row[1]
				      "-1,"					// row[2]
				      "MAX(NumLevels),"				// row[3]
				      "SUM(NumFolders),"			// row[4]
				      "SUM(NumFiles),"				// row[5]
				      "SUM(TotalSize)"				// row[6]
			        " FROM brw_sizes"
			       " WHERE FileBrowser=%u",
			       (unsigned) FileBrowser);
	       break;
	    case Brw_ADMI_DOC_GRP:
	    case Brw_ADMI_TCH_GRP:
	    case Brw_ADMI_SHR_GRP:
	    case Brw_ADMI_MRK_GRP:
	       DB_QuerySELECT (mysql_res,"can not get size of a file browser",
			       "SELECT COUNT(DISTINCT grp_types.CrsCod),"	// row[0]
				      "COUNT(DISTINCT brw_sizes.Cod),"		// row[1]
				      "-1,"					// row[2]
				      "MAX(brw_sizes.NumLevels),"		// row[3]
				      "SUM(brw_sizes.NumFolders),"		// row[4]
				      "SUM(brw_sizes.NumFiles),"		// row[5]
				      "SUM(brw_sizes.TotalSize)"		// row[6]
			        " FROM grp_types,"
			              "grp_groups,"
			              "brw_sizes"
			       " WHERE grp_types.GrpTypCod=grp_groups.GrpTypCod"
			         " AND grp_groups.GrpCod=brw_sizes.Cod"
	                         " AND brw_sizes.FileBrowser=%u",
			       (unsigned) FileBrowser);
	       break;
	    case Brw_ADMI_ASG_USR:
	    case Brw_ADMI_WRK_USR:
	       DB_QuerySELECT (mysql_res,"can not get size of a file browser",
			       "SELECT COUNT(DISTINCT Cod),"			// row[0]
				      "-1,"					// row[1]
				      "COUNT(DISTINCT ZoneUsrCod),"		// row[2]
				      "MAX(NumLevels),"				// row[3]
				      "SUM(NumFolders),"			// row[4]
				      "SUM(NumFiles),"				// row[5]
				      "SUM(TotalSize)"				// row[6]
			        " FROM brw_sizes"
			       " WHERE FileBrowser=%u",
			       (unsigned) FileBrowser);
	       break;
	    case Brw_ADMI_BRF_USR:
	       DB_QuerySELECT (mysql_res,"can not get size of a file browser",
			       "SELECT -1,"					// row[0]
				      "-1,"					// row[1]
				      "COUNT(DISTINCT ZoneUsrCod),"		// row[2]
				      "MAX(NumLevels),"				// row[3]
				      "SUM(NumFolders),"			// row[4]
				      "SUM(NumFiles),"				// row[5]
				      "SUM(TotalSize)"				// row[6]
			        " FROM brw_sizes"
			       " WHERE FileBrowser=%u",
			       (unsigned) FileBrowser);
	       break;
	    default:
	       Err_WrongFileBrowserExit ();
	       break;
	   }
         break;
      /* Scope = the current country */
      case HieLvl_CTY:
	 switch (FileBrowser)
	   {
	    case Brw_UNKNOWN:
	       DB_QuerySELECT (mysql_res,"can not get size of a file browser",
			       "SELECT COUNT(DISTINCT CrsCod),"			// row[0]
				      "COUNT(DISTINCT GrpCod)-1,"		// row[1]
				      "-1,"					// row[2]
				      "MAX(NumLevels),"				// row[3]
				      "SUM(NumFolders),"			// row[4]
				      "SUM(NumFiles),"				// row[5]
				      "SUM(TotalSize)"				// row[6]
			        " FROM "
	                       "("
	                       "SELECT brw_sizes.Cod AS CrsCod,"
				      "-1 AS GrpCod,"			// Course zones
				      "brw_sizes.NumLevels,"
				      "brw_sizes.NumFolders,"
				      "brw_sizes.NumFiles,"
				      "brw_sizes.TotalSize"
			        " FROM ins_instits,"
			              "ctr_centers,"
			              "deg_degrees,"
			              "crs_courses,"
			              "brw_sizes"
			       " WHERE ins_instits.CtyCod=%ld"
			         " AND ins_instits.InsCod=ctr_centers.InsCod"
			         " AND ctr_centers.CtrCod=deg_degrees.CtrCod"
			         " AND deg_degrees.DegCod=crs_courses.DegCod"
			         " AND crs_courses.CrsCod=brw_sizes.Cod"
			         " AND brw_sizes.FileBrowser IN (%u,%u,%u,%u,%u,%u)"
	                       " UNION "
	                       "SELECT grp_types.CrsCod,"
				      "brw_sizes.Cod AS GrpCod,"	// Group zones
				      "brw_sizes.NumLevels,"
				      "brw_sizes.NumFolders,"
				      "brw_sizes.NumFiles,"
				      "brw_sizes.TotalSize"
			        " FROM ins_instits,"
			              "ctr_centers,"
			              "deg_degrees,"
			              "crs_courses,"
			              "grp_types,"
			              "grp_groups,"
			              "brw_sizes"
			       " WHERE ins_instits.CtyCod=%ld"
	                         " AND ins_instits.InsCod=ctr_centers.InsCod"
			         " AND ctr_centers.CtrCod=deg_degrees.CtrCod"
			         " AND deg_degrees.DegCod=crs_courses.DegCod"
			         " AND crs_courses.CrsCod=grp_types.CrsCod"
			         " AND grp_types.GrpTypCod=grp_groups.GrpTypCod"
			         " AND grp_groups.GrpCod=brw_sizes.Cod"
			         " AND brw_sizes.FileBrowser IN (%u,%u,%u,%u)"
			       ") AS sizes",
			       Gbl.Hierarchy.Cty.CtyCod,
			       (unsigned) Brw_ADMI_DOC_CRS,
			       (unsigned) Brw_ADMI_TCH_CRS,
			       (unsigned) Brw_ADMI_SHR_CRS,
			       (unsigned) Brw_ADMI_ASG_USR,
			       (unsigned) Brw_ADMI_WRK_USR,
			       (unsigned) Brw_ADMI_MRK_CRS,
			       Gbl.Hierarchy.Cty.CtyCod,
			       (unsigned) Brw_ADMI_DOC_GRP,
			       (unsigned) Brw_ADMI_TCH_GRP,
			       (unsigned) Brw_ADMI_SHR_GRP,
			       (unsigned) Brw_ADMI_MRK_GRP);
	       break;
	    case Brw_ADMI_DOC_CRS:
	    case Brw_ADMI_TCH_CRS:
	    case Brw_ADMI_SHR_CRS:
	    case Brw_ADMI_MRK_CRS:
	       DB_QuerySELECT (mysql_res,"can not get size of a file browser",
			       "SELECT COUNT(DISTINCT brw_sizes.Cod),"		// row[0]
				      "-1,"					// row[1]
				      "-1,"					// row[2]
				      "MAX(brw_sizes.NumLevels),"		// row[3]
				      "SUM(brw_sizes.NumFolders),"		// row[4]
				      "SUM(brw_sizes.NumFiles),"		// row[5]
				      "SUM(brw_sizes.TotalSize)"		// row[6]
			        " FROM ins_instits,"
			              "ctr_centers,"
			              "deg_degrees,"
			              "crs_courses,"
			              "brw_sizes"
			       " WHERE ins_instits.CtyCod=%ld"
	                         " AND ins_instits.InsCod=ctr_centers.InsCod"
			         " AND ctr_centers.CtrCod=deg_degrees.CtrCod"
			         " AND deg_degrees.DegCod=crs_courses.DegCod"
			         " AND crs_courses.CrsCod=brw_sizes.Cod"
			         " AND brw_sizes.FileBrowser=%u",
			       Gbl.Hierarchy.Cty.CtyCod,
			       (unsigned) FileBrowser);
	       break;
	    case Brw_ADMI_DOC_GRP:
	    case Brw_ADMI_TCH_GRP:
	    case Brw_ADMI_SHR_GRP:
	    case Brw_ADMI_MRK_GRP:
	       DB_QuerySELECT (mysql_res,"can not get size of a file browser",
			       "SELECT COUNT(DISTINCT grp_types.CrsCod),"	// row[0]
				      "COUNT(DISTINCT brw_sizes.Cod),"		// row[1]
				      "-1,"					// row[2]
				      "MAX(brw_sizes.NumLevels),"		// row[3]
				      "SUM(brw_sizes.NumFolders),"		// row[4]
				      "SUM(brw_sizes.NumFiles),"		// row[5]
				      "SUM(brw_sizes.TotalSize)"		// row[6]
			        " FROM ins_instits,"
			              "ctr_centers,"
			              "deg_degrees,"
			              "crs_courses,"
			              "grp_types,"
			              "grp_groups,"
			              "brw_sizes"
			       " WHERE ins_instits.CtyCod=%ld"
	                         " AND ins_instits.InsCod=ctr_centers.InsCod"
			         " AND ctr_centers.CtrCod=deg_degrees.CtrCod"
			         " AND deg_degrees.DegCod=crs_courses.DegCod"
			         " AND crs_courses.CrsCod=grp_types.CrsCod"
			         " AND grp_types.GrpTypCod=grp_groups.GrpTypCod"
			         " AND grp_groups.GrpCod=brw_sizes.Cod"
			         " AND brw_sizes.FileBrowser=%u",
			       Gbl.Hierarchy.Cty.CtyCod,
			       (unsigned) FileBrowser);
	       break;
	    case Brw_ADMI_ASG_USR:
	    case Brw_ADMI_WRK_USR:
	       DB_QuerySELECT (mysql_res,"can not get size of a file browser",
			       "SELECT COUNT(DISTINCT brw_sizes.Cod),"		// row[0]
				      "-1,"					// row[1]
				      "COUNT(DISTINCT brw_sizes.ZoneUsrCod),"	// row[2]
				      "MAX(brw_sizes.NumLevels),"		// row[3]
				      "SUM(brw_sizes.NumFolders),"		// row[4]
				      "SUM(brw_sizes.NumFiles),"		// row[5]
				      "SUM(brw_sizes.TotalSize)"		// row[6]
			        " FROM ins_instits,"
			              "ctr_centers,"
			              "deg_degrees,"
			              "crs_courses,"
			              "brw_sizes"
			       " WHERE ins_instits.CtyCod=%ld"
	                         " AND ins_instits.InsCod=ctr_centers.InsCod"
			         " AND ctr_centers.CtrCod=deg_degrees.CtrCod"
			         " AND deg_degrees.DegCod=crs_courses.DegCod"
			         " AND crs_courses.CrsCod=brw_sizes.Cod"
	                         " AND brw_sizes.FileBrowser=%u",
			       Gbl.Hierarchy.Cty.CtyCod,
			       (unsigned) FileBrowser);
	       break;
	    case Brw_ADMI_BRF_USR:
	       DB_QuerySELECT (mysql_res,"can not get size of a file browser",
			       "SELECT -1,"					// row[0]
				      "-1,"					// row[1]
				      "COUNT(DISTINCT brw_sizes.ZoneUsrCod),"	// row[2]
				      "MAX(brw_sizes.NumLevels),"		// row[3]
				      "SUM(brw_sizes.NumFolders),"		// row[4]
				      "SUM(brw_sizes.NumFiles),"		// row[5]
				      "SUM(brw_sizes.TotalSize)"		// row[6]
			        " FROM ins_instits,"
			              "ctr_centers,"
			              "deg_degrees,"
			              "crs_courses,"
			              "crs_users,"
			              "brw_sizes"
			       " WHERE ins_instits.CtyCod=%ld"
	                         " AND ins_instits.InsCod=ctr_centers.InsCod"
			         " AND ctr_centers.CtrCod=deg_degrees.CtrCod"
			         " AND deg_degrees.DegCod=crs_courses.DegCod"
			         " AND crs_courses.CrsCod=crs_users.CrsCod"
			         " AND crs_users.UsrCod=brw_sizes.ZoneUsrCod"
			         " AND brw_sizes.FileBrowser=%u",
			       Gbl.Hierarchy.Cty.CtyCod,
			       (unsigned) FileBrowser);
	       break;
	    default:
	       Err_WrongFileBrowserExit ();
	       break;
	   }
         break;
      /* Scope = the current institution */
      case HieLvl_INS:
	 switch (FileBrowser)
	   {
	    case Brw_UNKNOWN:
	       DB_QuerySELECT (mysql_res,"can not get size of a file browser",
			       "SELECT COUNT(DISTINCT CrsCod),"			// row[0]
				      "COUNT(DISTINCT GrpCod)-1,"		// row[1]
				      "-1,"					// row[2]
				      "MAX(NumLevels),"				// row[3]
				      "SUM(NumFolders),"			// row[4]
				      "SUM(NumFiles),"				// row[5]
				      "SUM(TotalSize)"				// row[6]
			        " FROM "
	                       "("
	                       "SELECT brw_sizes.Cod AS CrsCod,"
				      "-1 AS GrpCod,"			// Course zones
				      "brw_sizes.NumLevels,"
				      "brw_sizes.NumFolders,"
				      "brw_sizes.NumFiles,"
				      "brw_sizes.TotalSize"
			        " FROM ctr_centers,"
			              "deg_degrees,"
			              "crs_courses,"
			              "brw_sizes"
			       " WHERE ctr_centers.InsCod=%ld"
			         " AND ctr_centers.CtrCod=deg_degrees.CtrCod"
			         " AND deg_degrees.DegCod=crs_courses.DegCod"
			         " AND crs_courses.CrsCod=brw_sizes.Cod"
			         " AND brw_sizes.FileBrowser IN (%u,%u,%u,%u,%u,%u)"
	                       " UNION "
	                       "SELECT grp_types.CrsCod,"
				      "brw_sizes.Cod AS GrpCod,"	// Group zones
				      "brw_sizes.NumLevels,"
				      "brw_sizes.NumFolders,"
				      "brw_sizes.NumFiles,"
				      "brw_sizes.TotalSize"
			        " FROM ctr_centers,"
			              "deg_degrees,"
			              "crs_courses,"
			              "grp_types,"
			              "grp_groups,"
			              "brw_sizes"
			       " WHERE ctr_centers.InsCod=%ld"
			         " AND ctr_centers.CtrCod=deg_degrees.CtrCod"
			         " AND deg_degrees.DegCod=crs_courses.DegCod"
			         " AND crs_courses.CrsCod=grp_types.CrsCod"
			         " AND grp_types.GrpTypCod=grp_groups.GrpTypCod"
			         " AND grp_groups.GrpCod=brw_sizes.Cod"
			         " AND brw_sizes.FileBrowser IN (%u,%u,%u,%u)"
			       ") AS sizes",
			       Gbl.Hierarchy.Ins.InsCod,
			       (unsigned) Brw_ADMI_DOC_CRS,
			       (unsigned) Brw_ADMI_TCH_CRS,
			       (unsigned) Brw_ADMI_SHR_CRS,
			       (unsigned) Brw_ADMI_ASG_USR,
			       (unsigned) Brw_ADMI_WRK_USR,
			       (unsigned) Brw_ADMI_MRK_CRS,
			       Gbl.Hierarchy.Ins.InsCod,
			       (unsigned) Brw_ADMI_DOC_GRP,
			       (unsigned) Brw_ADMI_TCH_GRP,
			       (unsigned) Brw_ADMI_SHR_GRP,
			       (unsigned) Brw_ADMI_MRK_GRP);
	       break;
	    case Brw_ADMI_DOC_CRS:
	    case Brw_ADMI_TCH_CRS:
	    case Brw_ADMI_SHR_CRS:
	    case Brw_ADMI_MRK_CRS:
	       DB_QuerySELECT (mysql_res,"can not get size of a file browser",
			       "SELECT COUNT(DISTINCT brw_sizes.Cod),"		// row[0]
				      "-1,"					// row[1]
				      "-1,"					// row[2]
				      "MAX(brw_sizes.NumLevels),"		// row[3]
				      "SUM(brw_sizes.NumFolders),"		// row[4]
				      "SUM(brw_sizes.NumFiles),"		// row[5]
				      "SUM(brw_sizes.TotalSize)"		// row[6]
			        " FROM ctr_centers,"
			              "deg_degrees,"
			              "crs_courses,"
			              "brw_sizes"
			       " WHERE ctr_centers.InsCod=%ld"
			         " AND ctr_centers.CtrCod=deg_degrees.CtrCod"
			         " AND deg_degrees.DegCod=crs_courses.DegCod"
			         " AND crs_courses.CrsCod=brw_sizes.Cod"
			         " AND brw_sizes.FileBrowser=%u",
			       Gbl.Hierarchy.Ins.InsCod,
			       (unsigned) FileBrowser);
	       break;
	    case Brw_ADMI_DOC_GRP:
	    case Brw_ADMI_TCH_GRP:
	    case Brw_ADMI_SHR_GRP:
	    case Brw_ADMI_MRK_GRP:
	       DB_QuerySELECT (mysql_res,"can not get size of a file browser",
			       "SELECT COUNT(DISTINCT grp_types.CrsCod),"	// row[0]
				      "COUNT(DISTINCT brw_sizes.Cod),"		// row[1]
				      "-1,"					// row[2]
				      "MAX(brw_sizes.NumLevels),"		// row[3]
				      "SUM(brw_sizes.NumFolders),"		// row[4]
				      "SUM(brw_sizes.NumFiles),"		// row[5]
				      "SUM(brw_sizes.TotalSize)"		// row[6]
			        " FROM ctr_centers,"
			              "deg_degrees,"
			              "crs_courses,"
			              "grp_types,"
			              "grp_groups,"
			              "brw_sizes"
			       " WHERE ctr_centers.InsCod=%ld"
			         " AND ctr_centers.CtrCod=deg_degrees.CtrCod"
			         " AND deg_degrees.DegCod=crs_courses.DegCod"
			         " AND crs_courses.CrsCod=grp_types.CrsCod"
			         " AND grp_types.GrpTypCod=grp_groups.GrpTypCod"
			         " AND grp_groups.GrpCod=brw_sizes.Cod"
			         " AND brw_sizes.FileBrowser=%u",
			       Gbl.Hierarchy.Ins.InsCod,
			       (unsigned) FileBrowser);
	       break;
	    case Brw_ADMI_ASG_USR:
	    case Brw_ADMI_WRK_USR:
	       DB_QuerySELECT (mysql_res,"can not get size of a file browser",
			       "SELECT COUNT(DISTINCT brw_sizes.Cod),"		// row[0]
				      "-1,"					// row[1]
				      "COUNT(DISTINCT brw_sizes.ZoneUsrCod),"	// row[2]
				      "MAX(brw_sizes.NumLevels),"		// row[3]
				      "SUM(brw_sizes.NumFolders),"		// row[4]
				      "SUM(brw_sizes.NumFiles),"		// row[5]
				      "SUM(brw_sizes.TotalSize)"		// row[6]
			        " FROM ctr_centers,"
			              "deg_degrees,"
			              "crs_courses,"
			              "brw_sizes"
			       " WHERE ctr_centers.InsCod=%ld"
			         " AND ctr_centers.CtrCod=deg_degrees.CtrCod"
			         " AND deg_degrees.DegCod=crs_courses.DegCod"
			         " AND crs_courses.CrsCod=brw_sizes.Cod"
	                         " AND brw_sizes.FileBrowser=%u",
			       Gbl.Hierarchy.Ins.InsCod,
			       (unsigned) FileBrowser);
	       break;
	    case Brw_ADMI_BRF_USR:
	       DB_QuerySELECT (mysql_res,"can not get size of a file browser",
			       "SELECT -1,"					// row[0]
				      "-1,"					// row[1]
				      "COUNT(DISTINCT brw_sizes.ZoneUsrCod),"	// row[2]
				      "MAX(brw_sizes.NumLevels),"		// row[3]
				      "SUM(brw_sizes.NumFolders),"		// row[4]
				      "SUM(brw_sizes.NumFiles),"		// row[5]
				      "SUM(brw_sizes.TotalSize)"		// row[6]
			        " FROM ctr_centers,"
			              "deg_degrees,"
			              "crs_courses,"
			              "crs_users,"
			              "brw_sizes"
			       " WHERE ctr_centers.InsCod=%ld"
			         " AND ctr_centers.CtrCod=deg_degrees.CtrCod"
			         " AND deg_degrees.DegCod=crs_courses.DegCod"
			         " AND crs_courses.CrsCod=crs_users.CrsCod"
			         " AND crs_users.UsrCod=brw_sizes.ZoneUsrCod"
			         " AND brw_sizes.FileBrowser=%u",
			       Gbl.Hierarchy.Ins.InsCod,
			       (unsigned) FileBrowser);
	       break;
	    default:
	       Err_WrongFileBrowserExit ();
	       break;
	   }
         break;
      /* Scope = the current center */
      case HieLvl_CTR:
	 switch (FileBrowser)
	   {
	    case Brw_UNKNOWN:
	       DB_QuerySELECT (mysql_res,"can not get size of a file browser",
			       "SELECT COUNT(DISTINCT CrsCod),"			// row[0]
				      "COUNT(DISTINCT GrpCod)-1,"		// row[1]
				      "-1,"					// row[2]
				      "MAX(NumLevels),"				// row[3]
				      "SUM(NumFolders),"			// row[4]
				      "SUM(NumFiles),"				// row[5]
				      "SUM(TotalSize)"				// row[6]
			        " FROM "
	                       "("
	                       "SELECT brw_sizes.Cod AS CrsCod,"
				      "-1 AS GrpCod,"			// Course zones
				      "brw_sizes.NumLevels,"
				      "brw_sizes.NumFolders,"
				      "brw_sizes.NumFiles,"
				      "brw_sizes.TotalSize"
			        " FROM deg_degrees,"
			              "crs_courses,"
			              "brw_sizes"
			       " WHERE deg_degrees.CtrCod=%ld"
			         " AND deg_degrees.DegCod=crs_courses.DegCod"
			         " AND crs_courses.CrsCod=brw_sizes.Cod"
			         " AND brw_sizes.FileBrowser IN (%u,%u,%u,%u,%u,%u)"
	                       " UNION "
	                       "SELECT grp_types.CrsCod,"
				      "brw_sizes.Cod AS GrpCod,"	// Group zones
				      "brw_sizes.NumLevels,"
				      "brw_sizes.NumFolders,"
				      "brw_sizes.NumFiles,"
				      "brw_sizes.TotalSize"
			        " FROM deg_degrees,"
			              "crs_courses,"
			              "grp_types,"
			              "grp_groups,"
			              "brw_sizes"
			       " WHERE deg_degrees.CtrCod=%ld"
			         " AND deg_degrees.DegCod=crs_courses.DegCod"
			         " AND crs_courses.CrsCod=grp_types.CrsCod"
			         " AND grp_types.GrpTypCod=grp_groups.GrpTypCod"
			         " AND grp_groups.GrpCod=brw_sizes.Cod"
			         " AND brw_sizes.FileBrowser IN (%u,%u,%u,%u)"
			       ") AS sizes",
			       Gbl.Hierarchy.Ctr.CtrCod,
			       (unsigned) Brw_ADMI_DOC_CRS,
			       (unsigned) Brw_ADMI_TCH_CRS,
			       (unsigned) Brw_ADMI_SHR_CRS,
			       (unsigned) Brw_ADMI_ASG_USR,
			       (unsigned) Brw_ADMI_WRK_USR,
			       (unsigned) Brw_ADMI_MRK_CRS,
			       Gbl.Hierarchy.Ctr.CtrCod,
			       (unsigned) Brw_ADMI_DOC_GRP,
			       (unsigned) Brw_ADMI_TCH_GRP,
			       (unsigned) Brw_ADMI_SHR_GRP,
			       (unsigned) Brw_ADMI_MRK_GRP);
	       break;
	    case Brw_ADMI_DOC_CRS:
	    case Brw_ADMI_TCH_CRS:
	    case Brw_ADMI_SHR_CRS:
	    case Brw_ADMI_MRK_CRS:
	       DB_QuerySELECT (mysql_res,"can not get size of a file browser",
			       "SELECT COUNT(DISTINCT brw_sizes.Cod),"		// row[0]
				      "-1,"					// row[1]
				      "-1,"					// row[2]
				      "MAX(brw_sizes.NumLevels),"		// row[3]
				      "SUM(brw_sizes.NumFolders),"		// row[4]
				      "SUM(brw_sizes.NumFiles),"		// row[5]
				      "SUM(brw_sizes.TotalSize)"		// row[6]
			        " FROM deg_degrees,"
			              "crs_courses,"
			              "brw_sizes"
			       " WHERE deg_degrees.CtrCod=%ld"
			         " AND deg_degrees.DegCod=crs_courses.DegCod"
			         " AND crs_courses.CrsCod=brw_sizes.Cod"
			         " AND brw_sizes.FileBrowser=%u",
			       Gbl.Hierarchy.Ctr.CtrCod,
			       (unsigned) FileBrowser);
               break;
	    case Brw_ADMI_DOC_GRP:
	    case Brw_ADMI_TCH_GRP:
	    case Brw_ADMI_SHR_GRP:
	    case Brw_ADMI_MRK_GRP:
	       DB_QuerySELECT (mysql_res,"can not get size of a file browser",
			       "SELECT COUNT(DISTINCT grp_types.CrsCod),"
				      "COUNT(DISTINCT brw_sizes.Cod),"		// row[0]
				      "-1,"					// row[1]
				      "MAX(brw_sizes.NumLevels),"		// row[2]
				      "SUM(brw_sizes.NumFolders),"		// row[3]
				      "SUM(brw_sizes.NumFiles),"		// row[4]
				      "SUM(brw_sizes.TotalSize)"		// row[5]
			        " FROM deg_degrees,"
			              "crs_courses,"
			              "grp_types,"
			              "grp_groups,"
			              "brw_sizes"
			       " WHERE deg_degrees.CtrCod=%ld"
			         " AND deg_degrees.DegCod=crs_courses.DegCod"
			         " AND crs_courses.CrsCod=grp_types.CrsCod"
			         " AND grp_types.GrpTypCod=grp_groups.GrpTypCod"
			         " AND grp_groups.GrpCod=brw_sizes.Cod"
			         " AND brw_sizes.FileBrowser=%u",
			       Gbl.Hierarchy.Ctr.CtrCod,
			       (unsigned) FileBrowser);
               break;
	    case Brw_ADMI_ASG_USR:
	    case Brw_ADMI_WRK_USR:
	       DB_QuerySELECT (mysql_res,"can not get size of a file browser",
			       "SELECT COUNT(DISTINCT brw_sizes.Cod),"		// row[0]
				      "-1,"					// row[1]
				      "COUNT(DISTINCT brw_sizes.ZoneUsrCod),"	// row[2]
				      "MAX(brw_sizes.NumLevels),"		// row[3]
				      "SUM(brw_sizes.NumFolders),"		// row[4]
				      "SUM(brw_sizes.NumFiles),"		// row[5]
				      "SUM(brw_sizes.TotalSize)"		// row[6]
			        " FROM deg_degrees,"
			              "crs_courses,"
			              "brw_sizes"
			       " WHERE deg_degrees.CtrCod=%ld"
			         " AND deg_degrees.DegCod=crs_courses.DegCod"
			         " AND crs_courses.CrsCod=brw_sizes.Cod"
			         " AND brw_sizes.FileBrowser=%u",
			       Gbl.Hierarchy.Ctr.CtrCod,
			       (unsigned) FileBrowser);
	       break;
	    case Brw_ADMI_BRF_USR:
	       DB_QuerySELECT (mysql_res,"can not get size of a file browser",
			       "SELECT -1,"					// row[0]
				      "-1,"					// row[1]
				      "COUNT(DISTINCT brw_sizes.ZoneUsrCod),"	// row[2]
				      "MAX(brw_sizes.NumLevels),"		// row[3]
				      "SUM(brw_sizes.NumFolders),"		// row[4]
				      "SUM(brw_sizes.NumFiles),"		// row[5]
				      "SUM(brw_sizes.TotalSize)"		// row[6]
			        " FROM deg_degrees,"
			              "crs_courses,"
			              "crs_users,"
			              "brw_sizes"
			       " WHERE deg_degrees.CtrCod=%ld"
			         " AND deg_degrees.DegCod=crs_courses.DegCod"
			         " AND crs_courses.CrsCod=crs_users.CrsCod"
			         " AND crs_users.UsrCod=brw_sizes.ZoneUsrCod"
			         " AND brw_sizes.FileBrowser=%u",
			       Gbl.Hierarchy.Ctr.CtrCod,
			       (unsigned) FileBrowser);
	       break;
	    default:
	       Err_WrongFileBrowserExit ();
	       break;
	   }
         break;
      /* Scope = the current degree */
      case HieLvl_DEG:
	 switch (FileBrowser)
	   {
	    case Brw_UNKNOWN:
	       DB_QuerySELECT (mysql_res,"can not get size of a file browser",
			       "SELECT COUNT(DISTINCT CrsCod),"			// row[0]
				      "COUNT(DISTINCT GrpCod)-1,"		// row[1]
				      "-1,"					// row[2]
				      "MAX(NumLevels),"				// row[3]
				      "SUM(NumFolders),"			// row[4]
				      "SUM(NumFiles),"				// row[5]
				      "SUM(TotalSize)"				// row[6]
			        " FROM "
	                       "("
	                       "SELECT brw_sizes.Cod AS CrsCod,"
				      "-1 AS GrpCod,"			// Course zones
				      "brw_sizes.NumLevels,"
				      "brw_sizes.NumFolders,"
				      "brw_sizes.NumFiles,"
				      "brw_sizes.TotalSize"
			        " FROM crs_courses,"
			              "brw_sizes"
			       " WHERE crs_courses.DegCod=%ld"
			         " AND crs_courses.CrsCod=brw_sizes.Cod"
			         " AND brw_sizes.FileBrowser IN (%u,%u,%u,%u,%u,%u)"
	                       " UNION "
	                       "SELECT grp_types.CrsCod,"
	                              "brw_sizes.Cod AS GrpCod,"	// Group zones
			              "brw_sizes.NumLevels,"
				      "brw_sizes.NumFolders,"
				      "brw_sizes.NumFiles,"
				      "brw_sizes.TotalSize"
			        " FROM crs_courses,"
			              "grp_types,"
			              "grp_groups,"
			              "brw_sizes"
			       " WHERE crs_courses.DegCod=%ld"
			         " AND crs_courses.CrsCod=grp_types.CrsCod"
			         " AND grp_types.GrpTypCod=grp_groups.GrpTypCod"
			         " AND grp_groups.GrpCod=brw_sizes.Cod"
			         " AND brw_sizes.FileBrowser IN (%u,%u,%u,%u)"
			       ") AS sizes",
			       Gbl.Hierarchy.Deg.DegCod,
			       (unsigned) Brw_ADMI_DOC_CRS,
			       (unsigned) Brw_ADMI_TCH_CRS,
			       (unsigned) Brw_ADMI_SHR_CRS,
			       (unsigned) Brw_ADMI_ASG_USR,
			       (unsigned) Brw_ADMI_WRK_USR,
			       (unsigned) Brw_ADMI_MRK_CRS,
			       Gbl.Hierarchy.Deg.DegCod,
			       (unsigned) Brw_ADMI_DOC_GRP,
			       (unsigned) Brw_ADMI_TCH_GRP,
			       (unsigned) Brw_ADMI_SHR_GRP,
			       (unsigned) Brw_ADMI_MRK_GRP);
	       break;
	    case Brw_ADMI_DOC_CRS:
	    case Brw_ADMI_TCH_CRS:
	    case Brw_ADMI_SHR_CRS:
	    case Brw_ADMI_MRK_CRS:
	       DB_QuerySELECT (mysql_res,"can not get size of a file browser",
			       "SELECT COUNT(DISTINCT brw_sizes.Cod),"		// row[0]
				      "-1,"					// row[1]
				      "-1,"					// row[2]
				      "MAX(brw_sizes.NumLevels),"		// row[3]
				      "SUM(brw_sizes.NumFolders),"		// row[4]
				      "SUM(brw_sizes.NumFiles),"		// row[5]
				      "SUM(brw_sizes.TotalSize)"		// row[6]
			        " FROM crs_courses,"
			              "brw_sizes"
			       " WHERE crs_courses.DegCod=%ld"
			         " AND crs_courses.CrsCod=brw_sizes.Cod"
			         " AND brw_sizes.FileBrowser=%u",
			       Gbl.Hierarchy.Deg.DegCod,
			       (unsigned) FileBrowser);
	       break;
	    case Brw_ADMI_DOC_GRP:
	    case Brw_ADMI_TCH_GRP:
	    case Brw_ADMI_SHR_GRP:
	    case Brw_ADMI_MRK_GRP:
	       DB_QuerySELECT (mysql_res,"can not get size of a file browser",
			       "SELECT COUNT(DISTINCT grp_types.CrsCod),"	// row[0]
				      "COUNT(DISTINCT brw_sizes.Cod),"		// row[1]
				      "-1,"					// row[2]
				      "MAX(brw_sizes.NumLevels),"		// row[3]
				      "SUM(brw_sizes.NumFolders),"		// row[4]
				      "SUM(brw_sizes.NumFiles),"		// row[5]
				      "SUM(brw_sizes.TotalSize)"		// row[6]
			        " FROM crs_courses,"
			              "grp_types,"
			              "grp_groups,"
			              "brw_sizes"
			       " WHERE crs_courses.DegCod=%ld"
			         " AND crs_courses.CrsCod=grp_types.CrsCod"
			         " AND grp_types.GrpTypCod=grp_groups.GrpTypCod"
			         " AND grp_groups.GrpCod=brw_sizes.Cod"
			         " AND brw_sizes.FileBrowser=%u",
			       Gbl.Hierarchy.Deg.DegCod,
			       (unsigned) FileBrowser);
	       break;
	    case Brw_ADMI_ASG_USR:
	    case Brw_ADMI_WRK_USR:
	       DB_QuerySELECT (mysql_res,"can not get size of a file browser",
			       "SELECT COUNT(DISTINCT brw_sizes.Cod),"		// row[0]
				      "-1,"					// row[1]
				      "COUNT(DISTINCT brw_sizes.ZoneUsrCod),"	// row[2]
				      "MAX(brw_sizes.NumLevels),"		// row[3]
				      "SUM(brw_sizes.NumFolders),"		// row[4]
				      "SUM(brw_sizes.NumFiles),"		// row[5]
				      "SUM(brw_sizes.TotalSize)"		// row[6]
			        " FROM crs_courses,"
			              "brw_sizes"
			       " WHERE crs_courses.DegCod=%ld"
			         " AND crs_courses.CrsCod=brw_sizes.Cod"
			         " AND brw_sizes.FileBrowser=%u",
			       Gbl.Hierarchy.Deg.DegCod,
			       (unsigned) FileBrowser);
	       break;
	    case Brw_ADMI_BRF_USR:
	       DB_QuerySELECT (mysql_res,"can not get size of a file browser",
			       "SELECT -1,"					// row[0]
				      "-1,"					// row[1]
				      "COUNT(DISTINCT brw_sizes.ZoneUsrCod),"	// row[2]
				      "MAX(brw_sizes.NumLevels),"		// row[3]
				      "SUM(brw_sizes.NumFolders),"		// row[4]
				      "SUM(brw_sizes.NumFiles),"		// row[5]
				      "SUM(brw_sizes.TotalSize)"		// row[6]
			        " FROM crs_courses,"
			              "crs_users,"
			              "brw_sizes"
			       " WHERE crs_courses.DegCod=%ld"
			         " AND crs_courses.CrsCod=crs_users.CrsCod"
			         " AND crs_users.UsrCod=brw_sizes.ZoneUsrCod"
			         " AND brw_sizes.FileBrowser=%u",
			       Gbl.Hierarchy.Deg.DegCod,
			       (unsigned) FileBrowser);
	       break;
	    default:
	       Err_WrongFileBrowserExit ();
	       break;
	   }
         break;
      /* Scope = the current course */
      case HieLvl_CRS:
	 switch (FileBrowser)
	   {
	    case Brw_UNKNOWN:
	       DB_QuerySELECT (mysql_res,"can not get size of a file browser",
			       "SELECT COUNT(DISTINCT CrsCod),"			// row[0]
				      "COUNT(DISTINCT GrpCod)-1,"		// row[1]
				      "-1,"					// row[2]
				      "MAX(NumLevels),"				// row[3]
				      "SUM(NumFolders),"			// row[4]
				      "SUM(NumFiles),"				// row[5]
				      "SUM(TotalSize)"				// row[6]
			        " FROM "
	                       "("
	                       "SELECT Cod AS CrsCod,"
				      "-1 AS GrpCod,"			// Course zones
				      "NumLevels,"
				      "NumFolders,"
				      "NumFiles,"
				      "TotalSize"
			        " FROM brw_sizes"
			       " WHERE Cod=%ld"
			        " AND FileBrowser IN (%u,%u,%u,%u,%u,%u)"
	                       " UNION "
	                       "SELECT grp_types.CrsCod,"
				      "brw_sizes.Cod AS GrpCod,"	// Group zones
				      "brw_sizes.NumLevels,"
				      "brw_sizes.NumFolders,"
				      "brw_sizes.NumFiles,"
				      "brw_sizes.TotalSize"
			        " FROM grp_types,"
			              "grp_groups,"
			              "brw_sizes"
			       " WHERE grp_types.CrsCod=%ld"
			         " AND grp_types.GrpTypCod=grp_groups.GrpTypCod"
			         " AND grp_groups.GrpCod=brw_sizes.Cod"
			         " AND brw_sizes.FileBrowser IN (%u,%u,%u,%u)"
			       ") AS sizes",
			       Gbl.Hierarchy.Crs.CrsCod,
			       (unsigned) Brw_ADMI_DOC_CRS,
			       (unsigned) Brw_ADMI_TCH_CRS,
			       (unsigned) Brw_ADMI_SHR_CRS,
			       (unsigned) Brw_ADMI_ASG_USR,
			       (unsigned) Brw_ADMI_WRK_USR,
			       (unsigned) Brw_ADMI_MRK_CRS,
			       Gbl.Hierarchy.Crs.CrsCod,
			       (unsigned) Brw_ADMI_DOC_GRP,
			       (unsigned) Brw_ADMI_TCH_GRP,
			       (unsigned) Brw_ADMI_SHR_GRP,
			       (unsigned) Brw_ADMI_MRK_GRP);
	       break;
	    case Brw_ADMI_DOC_CRS:
	    case Brw_ADMI_TCH_CRS:
	    case Brw_ADMI_SHR_CRS:
	    case Brw_ADMI_MRK_CRS:
	       DB_QuerySELECT (mysql_res,"can not get size of a file browser",
			       "SELECT 1,"					// row[0]
				      "-1,"					// row[1]
				      "-1,"					// row[2]
				      "MAX(NumLevels),"				// row[3]
				      "SUM(NumFolders),"			// row[4]
				      "SUM(NumFiles),"				// row[5]
				      "SUM(TotalSize)"				// row[6]
			        " FROM brw_sizes"
			       " WHERE Cod=%ld"
			       " AND FileBrowser=%u",
			       Gbl.Hierarchy.Crs.CrsCod,
			       (unsigned) FileBrowser);
	       break;
	    case Brw_ADMI_DOC_GRP:
	    case Brw_ADMI_TCH_GRP:
	    case Brw_ADMI_SHR_GRP:
	    case Brw_ADMI_MRK_GRP:
	       DB_QuerySELECT (mysql_res,"can not get size of a file browser",
			       "SELECT COUNT(DISTINCT grp_types.CrsCod),"	// row[0]
				      "COUNT(DISTINCT brw_sizes.Cod),"		// row[1]
				      "-1,"					// row[2]
				      "MAX(brw_sizes.NumLevels),"		// row[3]
				      "SUM(brw_sizes.NumFolders),"		// row[4]
				      "SUM(brw_sizes.NumFiles),"		// row[5]
				      "SUM(brw_sizes.TotalSize)"		// row[6]
			        " FROM grp_types,"
			              "grp_groups,"
			              "brw_sizes"
			       " WHERE grp_types.CrsCod=%ld"
			         " AND grp_types.GrpTypCod=grp_groups.GrpTypCod"
			         " AND grp_groups.GrpCod=brw_sizes.Cod"
			         " AND brw_sizes.FileBrowser=%u",
			       Gbl.Hierarchy.Crs.CrsCod,
			       (unsigned) FileBrowser);
	       break;
	    case Brw_ADMI_ASG_USR:
	    case Brw_ADMI_WRK_USR:
	       DB_QuerySELECT (mysql_res,"can not get size of a file browser",
			       "SELECT 1,"					// row[0]
				      "-1,"					// row[1]
				      "COUNT(DISTINCT ZoneUsrCod),"		// row[2]
				      "MAX(NumLevels),"				// row[3]
				      "SUM(NumFolders),"			// row[4]
				      "SUM(NumFiles),"				// row[5]
				      "SUM(TotalSize)"				// row[6]
			        " FROM brw_sizes"
			       " WHERE Cod=%ld"
			         " AND FileBrowser=%u",
			       Gbl.Hierarchy.Crs.CrsCod,
			       (unsigned) FileBrowser);
	       break;
	    case Brw_ADMI_BRF_USR:
	       DB_QuerySELECT (mysql_res,"can not get size of a file browser",
			       "SELECT -1,"					// row[0]
				      "-1,"					// row[1]
				      "COUNT(DISTINCT brw_sizes.ZoneUsrCod),"	// row[2]
				      "MAX(brw_sizes.NumLevels),"		// row[3]
				      "SUM(brw_sizes.NumFolders),"		// row[4]
				      "SUM(brw_sizes.NumFiles),"		// row[5]
				      "SUM(brw_sizes.TotalSize)"		// row[6]
			        " FROM crs_users,"
			              "brw_sizes"
			       " WHERE crs_users.CrsCod=%ld"
			         " AND crs_users.UsrCod=brw_sizes.ZoneUsrCod"
			         " AND brw_sizes.FileBrowser=%u",
			       Gbl.Hierarchy.Crs.CrsCod,
			       (unsigned) FileBrowser);
	       break;
	    default:
	       Err_WrongFileBrowserExit ();
	       break;
	   }
         break;
      default:
	 Err_WrongScopeExit ();
	 break;
     }
  }

/*****************************************************************************/
/***************** Get number of OERs depending on license *******************/
/*****************************************************************************/

unsigned Brw_DB_GetNumberOfOERs (MYSQL_RES **mysql_res,Brw_License_t License)
  {
   switch (Gbl.Scope.Current)
     {
      case HieLvl_SYS:
         return (unsigned)
         DB_QuerySELECT (mysql_res,"can not get number of OERs",
			 "SELECT Public,"		// row[0]
			        "COUNT(*)"		// row[1]
			  " FROM brw_files"
			 " WHERE License=%u"
			 " GROUP BY Public",
			 (unsigned) License);
      case HieLvl_CTY:
         return (unsigned)
         DB_QuerySELECT (mysql_res,"can not get number of OERs",
			 "SELECT brw_files.Public,"	// row[0]
			        "COUNT(*)"		// row[1]
			  " FROM ins_instits,"
			        "ctr_centers,"
			        "deg_degrees,"
			        "crs_courses,"
			        "brw_files"
			 " WHERE ins_instits.CtyCod=%ld"
			   " AND ins_instits.InsCod=ctr_centers.InsCod"
			   " AND ctr_centers.CtrCod=deg_degrees.CtrCod"
			   " AND deg_degrees.DegCod=crs_courses.DegCod"
			   " AND crs_courses.CrsCod=brw_files.Cod"
			   " AND brw_files.FileBrowser IN (%u,%u)"
			   " AND brw_files.License=%u"
			 " GROUP BY brw_files.Public",
			 Gbl.Hierarchy.Cty.CtyCod,
			 (unsigned) Brw_ADMI_DOC_CRS,
			 (unsigned) Brw_ADMI_SHR_CRS,
			 (unsigned) License);
      case HieLvl_INS:
         return (unsigned)
         DB_QuerySELECT (mysql_res,"can not get number of OERs",
			 "SELECT brw_files.Public,"	// row[0]
			        "COUNT(*)"		// row[1]
			  " FROM ctr_centers,"
			        "deg_degrees,"
			        "crs_courses,"
			        "brw_files"
			 " WHERE ctr_centers.InsCod=%ld"
			   " AND ctr_centers.CtrCod=deg_degrees.CtrCod"
			   " AND deg_degrees.DegCod=crs_courses.DegCod"
			   " AND crs_courses.CrsCod=brw_files.Cod"
			   " AND brw_files.FileBrowser IN (%u,%u)"
			   " AND brw_files.License=%u"
			 " GROUP BY brw_files.Public",
			 Gbl.Hierarchy.Ins.InsCod,
			 (unsigned) Brw_ADMI_DOC_CRS,
			 (unsigned) Brw_ADMI_SHR_CRS,
			 (unsigned) License);
      case HieLvl_CTR:
         return (unsigned)
         DB_QuerySELECT (mysql_res,"can not get number of OERs",
			 "SELECT brw_files.Public,"	// row[0]
			        "COUNT(*)"		// row[1]
			  " FROM deg_degrees,"
			        "crs_courses,"
			        "brw_files"
			 " WHERE deg_degrees.CtrCod=%ld"
			   " AND deg_degrees.DegCod=crs_courses.DegCod"
			   " AND crs_courses.CrsCod=brw_files.Cod"
			   " AND brw_files.FileBrowser IN (%u,%u)"
			   " AND brw_files.License=%u"
			 " GROUP BY brw_files.Public",
			 Gbl.Hierarchy.Ctr.CtrCod,
			 (unsigned) Brw_ADMI_DOC_CRS,
			 (unsigned) Brw_ADMI_SHR_CRS,
			 (unsigned) License);
      case HieLvl_DEG:
         return (unsigned)
         DB_QuerySELECT (mysql_res,"can not get number of OERs",
			 "SELECT brw_files.Public,"	// row[0]
			        "COUNT(*)"		// row[1]
			  " FROM crs_courses,"
			        "brw_files"
			 " WHERE crs_courses.DegCod=%ld"
			   " AND crs_courses.CrsCod=brw_files.Cod"
			   " AND brw_files.FileBrowser IN (%u,%u)"
			   " AND brw_files.License=%u"
			 " GROUP BY brw_files.Public",
			 Gbl.Hierarchy.Deg.DegCod,
			 (unsigned) Brw_ADMI_DOC_CRS,
			 (unsigned) Brw_ADMI_SHR_CRS,
			 (unsigned) License);
      case HieLvl_CRS:
         return (unsigned)
         DB_QuerySELECT (mysql_res,"can not get number of OERs",
			 "SELECT Public,"		// row[0]
			        "COUNT(*)"		// row[1]
			  " FROM brw_files"
			 " WHERE Cod=%ld"
			   " AND FileBrowser IN (%u,%u)"
			   " AND License=%u"
			 " GROUP BY Public",
			 Gbl.Hierarchy.Crs.CrsCod,
			 (unsigned) Brw_ADMI_DOC_CRS,
			 (unsigned) Brw_ADMI_SHR_CRS,
			 (unsigned) License);
      default:
	 Err_WrongScopeExit ();
	 return 0;	// Not reached
     }
  }


/*****************************************************************************/
/******** Remove files related to an institution from the database ***********/
/*****************************************************************************/

void Brw_DB_RemoveInsFiles (long InsCod)
  {
   /***** Remove from database the entries that store the file views *****/
   DB_QueryDELETE ("can not remove file views to files of an institution",
		   "DELETE FROM brw_views"
		   " USING brw_files,"
		          "brw_views"
		   " WHERE brw_files.FileBrowser IN (%u,%u)"
		     " AND brw_files.Cod=%ld"
		     " AND brw_files.FilCod=brw_views.FilCod",
	           (unsigned) Brw_ADMI_DOC_INS,
	           (unsigned) Brw_ADMI_SHR_INS,
	           InsCod);

   /***** Remove from database expanded folders *****/
   DB_QueryDELETE ("can not remove expanded folders of an institution",
		   "DELETE LOW_PRIORITY FROM brw_expanded"
		   " WHERE FileBrowser IN (%u,%u)"
		     " AND Cod=%ld",
	           (unsigned) Brw_ADMI_DOC_INS,
	           (unsigned) Brw_ADMI_SHR_INS,
	           InsCod);

   /***** Remove from database the entries that store clipboards *****/
   DB_QueryDELETE ("can not remove clipboards"
		   " related to files of an institution",
		   "DELETE FROM brw_clipboards"
		   " WHERE FileBrowser IN (%u,%u)"
		     " AND Cod=%ld",
	           (unsigned) Brw_ADMI_DOC_INS,
	           (unsigned) Brw_ADMI_SHR_INS,
	           InsCod);

   /***** Remove from database the entries that store
          the last time users visited file zones *****/
   DB_QueryDELETE ("can not remove file last visits"
		   " to files of an institution",
		   "DELETE FROM brw_last"
		   " WHERE FileBrowser IN (%u,%u)"
		     " AND Cod=%ld",
	           (unsigned) Brw_ADMI_DOC_INS,
	           (unsigned) Brw_ADMI_SHR_INS,
	           InsCod);

   /***** Remove from database the entries that store
          the sizes of the file zones *****/
   DB_QueryDELETE ("can not remove sizes of file zones of an institution",
		   "DELETE FROM brw_sizes"
		   " WHERE FileBrowser IN (%u,%u)"
		     " AND Cod=%ld",
	           (unsigned) Brw_ADMI_DOC_INS,
	           (unsigned) Brw_ADMI_SHR_INS,
	           InsCod);

   /***** Remove from database the entries that store the data files *****/
   DB_QueryDELETE ("can not remove files of an institution",
		   "DELETE FROM brw_files"
		   " WHERE FileBrowser IN (%u,%u)"
		     " AND Cod=%ld",
	           (unsigned) Brw_ADMI_DOC_INS,
	           (unsigned) Brw_ADMI_SHR_INS,
	           InsCod);
  }

/*****************************************************************************/
/************ Remove files related to a center from the database *************/
/*****************************************************************************/

void Brw_DB_RemoveCtrFiles (long CtrCod)
  {
   /***** Remove from database the entries that store the file views *****/
   DB_QueryDELETE ("can not remove file views to files of a center",
		   "DELETE FROM brw_views"
		   " USING brw_files,"
		          "brw_views"
		   " WHERE brw_files.FileBrowser IN (%u,%u)"
		     " AND brw_files.Cod=%ld"
		     " AND brw_files.FilCod=brw_views.FilCod",
	           (unsigned) Brw_ADMI_DOC_CTR,
	           (unsigned) Brw_ADMI_SHR_CTR,
	           CtrCod);

   /***** Remove from database expanded folders *****/
   DB_QueryDELETE ("can not remove expanded folders of a center",
		   "DELETE LOW_PRIORITY FROM brw_expanded"
		   " WHERE FileBrowser IN (%u,%u)"
		     " AND Cod=%ld",
	           (unsigned) Brw_ADMI_DOC_CTR,
	           (unsigned) Brw_ADMI_SHR_CTR,
	           CtrCod);

   /***** Remove from database the entries that store clipboards *****/
   DB_QueryDELETE ("can not remove clipboards related to files of a center",
		   "DELETE FROM brw_clipboards"
		   " WHERE FileBrowser IN (%u,%u)"
		     " AND Cod=%ld",
	           (unsigned) Brw_ADMI_DOC_CTR,
	           (unsigned) Brw_ADMI_SHR_CTR,
	           CtrCod);

   /***** Remove from database the entries that store the last time users visited file zones *****/
   DB_QueryDELETE ("can not remove file last visits to files of a center",
		   "DELETE FROM brw_last"
		   " WHERE FileBrowser IN (%u,%u)"
		     " AND Cod=%ld",
	           (unsigned) Brw_ADMI_DOC_CTR,
	           (unsigned) Brw_ADMI_SHR_CTR,
	           CtrCod);

   /***** Remove from database the entries that store the sizes of the file zones *****/
   DB_QueryDELETE ("can not remove sizes of file zones of a center",
		   "DELETE FROM brw_sizes"
		   " WHERE FileBrowser IN (%u,%u)"
		     " AND Cod=%ld",
	           (unsigned) Brw_ADMI_DOC_CTR,
	           (unsigned) Brw_ADMI_SHR_CTR,
	           CtrCod);

   /***** Remove from database the entries that store the data files *****/
   DB_QueryDELETE ("can not remove files of a center",
		   "DELETE FROM brw_files"
		   " WHERE FileBrowser IN (%u,%u)"
		     " AND Cod=%ld",
	           (unsigned) Brw_ADMI_DOC_CTR,
	           (unsigned) Brw_ADMI_SHR_CTR,
	           CtrCod);
  }

/*****************************************************************************/
/************ Remove files related to a degree from the database *************/
/*****************************************************************************/

void Brw_DB_RemoveDegFiles (long DegCod)
  {
   /***** Remove from database the entries that store the file views *****/
   DB_QueryDELETE ("can not remove file views to files of a degree",
		   "DELETE FROM brw_views"
		   " USING brw_files,"
		          "brw_views"
		   " WHERE brw_files.FileBrowser IN (%u,%u)"
		     " AND brw_files.Cod=%ld"
		     " AND brw_files.FilCod=brw_views.FilCod",
	           (unsigned) Brw_ADMI_DOC_DEG,
	           (unsigned) Brw_ADMI_SHR_DEG,
	           DegCod);

   /***** Remove from database expanded folders *****/
   DB_QueryDELETE ("can not remove expanded folders of a degree",
		   "DELETE LOW_PRIORITY FROM brw_expanded"
		   " WHERE FileBrowser IN (%u,%u)"
		     " AND Cod=%ld",
	           (unsigned) Brw_ADMI_DOC_DEG,
	           (unsigned) Brw_ADMI_SHR_DEG,
	           DegCod);

   /***** Remove from database the entries that store clipboards *****/
   DB_QueryDELETE ("can not remove clipboards related to files of a degree",
		   "DELETE FROM brw_clipboards"
		   " WHERE FileBrowser IN (%u,%u)"
		     " AND Cod=%ld",
	           (unsigned) Brw_ADMI_DOC_DEG,
	           (unsigned) Brw_ADMI_SHR_DEG,
	           DegCod);

   /***** Remove from database the entries that store the last time users visited file zones *****/
   DB_QueryDELETE ("can not remove file last visits to files of a degree",
		   "DELETE FROM brw_last"
		   " WHERE FileBrowser IN (%u,%u)"
		     " AND Cod=%ld",
	           (unsigned) Brw_ADMI_DOC_DEG,
	           (unsigned) Brw_ADMI_SHR_DEG,
	           DegCod);

   /***** Remove from database the entries that store the sizes of the file zones *****/
   DB_QueryDELETE ("can not remove sizes of file zones of a degree",
		   "DELETE FROM brw_sizes"
		   " WHERE FileBrowser IN (%u,%u)"
		     " AND Cod=%ld",
	           (unsigned) Brw_ADMI_DOC_DEG,
	           (unsigned) Brw_ADMI_SHR_DEG,
	           DegCod);

   /***** Remove from database the entries that store the data files *****/
   DB_QueryDELETE ("can not remove files of a degree",
		   "DELETE FROM brw_files"
		   " WHERE FileBrowser IN (%u,%u)"
		     " AND Cod=%ld",
	           (unsigned) Brw_ADMI_DOC_DEG,
	           (unsigned) Brw_ADMI_SHR_DEG,
	           DegCod);
  }

/*****************************************************************************/
/************ Remove files related to a course from the database *************/
/*****************************************************************************/
/* Remove information related to files in course,
   including groups and projects,
   so this function must be called
   before removing groups and projects */

void Brw_DB_RemoveCrsFiles (long CrsCod)
  {
   char SubqueryGrp[256];
   char SubqueryPrj[128];

   /***** Build subquery for groups *****/
   sprintf (SubqueryGrp,"(SELECT grp_groups.GrpCod"
	                  " FROM grp_types,"
	                        "grp_groups"
		         " WHERE grp_types.CrsCod=%ld"
		           " AND grp_types.GrpTypCod=grp_groups.GrpTypCod)",
            CrsCod);

   /***** Build subquery for projects *****/
   sprintf (SubqueryPrj,"(SELECT PrjCod"
	                  " FROM prj_projects"
	                 " WHERE CrsCod=%ld)",
            CrsCod);

   /***** Remove format of files of marks *****/
   DB_QueryDELETE ("can not remove the properties of marks"
		   " associated to a course",
		   "DELETE FROM mrk_marks"
		   " USING brw_files,"
		          "mrk_marks"
		   " WHERE brw_files.FileBrowser=%u"
		     " AND brw_files.Cod=%ld"
		     " AND brw_files.FilCod=mrk_marks.FilCod",
	           (unsigned) Brw_ADMI_MRK_CRS,
	           CrsCod);

   /***** Remove from database the entries that store the file views *****/
   /* Remove from course file zones */
   DB_QueryDELETE ("can not remove file views to files of a course",
		   "DELETE FROM brw_views"
		   " USING brw_files,"
		          "brw_views"
		   " WHERE brw_files.FileBrowser IN (%u,%u,%u,%u,%u,%u)"
		     " AND brw_files.Cod=%ld"
		     " AND brw_files.FilCod=brw_views.FilCod",
	           (unsigned) Brw_ADMI_DOC_CRS,
	           (unsigned) Brw_ADMI_TCH_CRS,
	           (unsigned) Brw_ADMI_SHR_CRS,
	           (unsigned) Brw_ADMI_ASG_USR,
	           (unsigned) Brw_ADMI_WRK_USR,
	           (unsigned) Brw_ADMI_MRK_CRS,
	           CrsCod);

   /* Remove from group file zones */
   DB_QueryDELETE ("can not remove file views to files of a course",
		   "DELETE FROM brw_views"
		   " USING brw_files,"
		          "brw_views"
		   " WHERE brw_files.FileBrowser IN (%u,%u,%u,%u)"
		     " AND brw_files.Cod IN %s"
		     " AND brw_files.FilCod=brw_views.FilCod",
	           (unsigned) Brw_ADMI_DOC_GRP,
	           (unsigned) Brw_ADMI_TCH_GRP,
	           (unsigned) Brw_ADMI_SHR_GRP,
	           (unsigned) Brw_ADMI_MRK_GRP,
	           SubqueryGrp);

   /* Remove from project file zones */
   DB_QueryDELETE ("can not remove file views to files of a course",
		   "DELETE FROM brw_views"
		   " USING brw_files,"
		          "brw_views"
		   " WHERE brw_files.FileBrowser IN (%u,%u)"
		     " AND brw_files.Cod IN %s"
		     " AND brw_files.FilCod=brw_views.FilCod",
	           (unsigned) Brw_ADMI_DOC_PRJ,
	           (unsigned) Brw_ADMI_ASS_PRJ,
	           SubqueryPrj);

   /***** Remove from database expanded folders *****/
   /* Remove from course file zones */
   DB_QueryDELETE ("can not remove expanded folders of a course",
		   "DELETE LOW_PRIORITY FROM brw_expanded"
		   " WHERE FileBrowser IN (%u,%u,%u,%u,%u,%u,%u,%u)"
		     " AND Cod=%ld",
	           (unsigned) Brw_ADMI_DOC_CRS,
	           (unsigned) Brw_ADMI_TCH_CRS,
	           (unsigned) Brw_ADMI_SHR_CRS,
	           (unsigned) Brw_ADMI_ASG_USR,
	           (unsigned) Brw_ADMI_ASG_CRS,
	           (unsigned) Brw_ADMI_WRK_USR,
	           (unsigned) Brw_ADMI_WRK_CRS,
	           (unsigned) Brw_ADMI_MRK_CRS,
	           CrsCod);

   /* Remove from group file zones */
   DB_QueryDELETE ("can not remove expanded folders of a course",
		   "DELETE LOW_PRIORITY FROM brw_expanded"
		   " WHERE FileBrowser IN (%u,%u,%u,%u)"
		     " AND Cod IN %s",
	           (unsigned) Brw_ADMI_DOC_GRP,
	           (unsigned) Brw_ADMI_TCH_GRP,
	           (unsigned) Brw_ADMI_SHR_GRP,
	           (unsigned) Brw_ADMI_MRK_GRP,
	           SubqueryGrp);

   /* Remove from project file zones */
   DB_QueryDELETE ("can not remove expanded folders of a course",
		   "DELETE LOW_PRIORITY FROM brw_expanded"
		   " WHERE FileBrowser IN (%u,%u)"
		     " AND Cod IN %s",
	           (unsigned) Brw_ADMI_DOC_PRJ,
	           (unsigned) Brw_ADMI_ASS_PRJ,
	           SubqueryPrj);

   /***** Remove from database the entries that store clipboards *****/
   /* Remove from course file zones */
   DB_QueryDELETE ("can not remove clipboards related to files of a course",
		   "DELETE FROM brw_clipboards"
		   " WHERE FileBrowser IN (%u,%u,%u,%u,%u,%u,%u,%u)"
		     " AND Cod=%ld",
	           (unsigned) Brw_ADMI_DOC_CRS,
	           (unsigned) Brw_ADMI_TCH_CRS,
	           (unsigned) Brw_ADMI_SHR_CRS,
	           (unsigned) Brw_ADMI_ASG_USR,
	           (unsigned) Brw_ADMI_ASG_CRS,
	           (unsigned) Brw_ADMI_WRK_USR,
	           (unsigned) Brw_ADMI_WRK_CRS,
	           (unsigned) Brw_ADMI_MRK_CRS,
	           CrsCod);

   /* Remove from group file zones */
   DB_QueryDELETE ("can not remove clipboards related to files of a course",
		   "DELETE FROM brw_clipboards"
		   " WHERE FileBrowser IN (%u,%u,%u,%u)"
		     " AND Cod IN %s",
	           (unsigned) Brw_ADMI_DOC_GRP,
	           (unsigned) Brw_ADMI_TCH_GRP,
	           (unsigned) Brw_ADMI_SHR_GRP,
	           (unsigned) Brw_ADMI_MRK_GRP,
	           SubqueryGrp);

   /* Remove from project file zones */
   DB_QueryDELETE ("can not remove clipboards related to files of a course",
		   "DELETE FROM brw_clipboards"
		   " WHERE FileBrowser IN (%u,%u)"
		     " AND Cod IN %s",
	           (unsigned) Brw_ADMI_DOC_PRJ,
	           (unsigned) Brw_ADMI_ASS_PRJ,
	           SubqueryPrj);

   /***** Remove from database the entries that store the last time users visited file zones *****/
   // Assignments and works are stored as one in brw_last...
   // ...because a user views them at the same time
   /* Remove from course file zones */
   DB_QueryDELETE ("can not remove file last visits to files of a course",
		   "DELETE FROM brw_last"
		   " WHERE FileBrowser IN (%u,%u,%u,%u,%u)"
		     " AND Cod=%ld",
	           (unsigned) Brw_ADMI_DOC_CRS,
	           (unsigned) Brw_ADMI_TCH_CRS,
	           (unsigned) Brw_ADMI_SHR_CRS,
	           (unsigned) Brw_ADMI_ASG_USR,
	           (unsigned) Brw_ADMI_MRK_CRS,
	           CrsCod);

   /* Remove from group file zones */
    DB_QueryDELETE ("can not remove file last visits to files of a course",
		    "DELETE FROM brw_last"
		   " WHERE FileBrowser IN (%u,%u,%u,%u)"
		     " AND Cod IN %s",
	           (unsigned) Brw_ADMI_DOC_GRP,
	           (unsigned) Brw_ADMI_TCH_GRP,
	           (unsigned) Brw_ADMI_SHR_GRP,
	           (unsigned) Brw_ADMI_MRK_GRP,
	           SubqueryGrp);

   /* Remove from project file zones */
   DB_QueryDELETE ("can not remove file last visits to files of a course",
		   "DELETE FROM brw_last"
		   " WHERE FileBrowser IN (%u,%u)"
		     " AND Cod IN %s",
	           (unsigned) Brw_ADMI_DOC_PRJ,
	           (unsigned) Brw_ADMI_ASS_PRJ,
	           SubqueryPrj);

   /***** Remove from database the entries that store the sizes of the file zones *****/
   /* Remove from course file zones */
   DB_QueryDELETE ("can not remove sizes of file zones of a course",
		   "DELETE FROM brw_sizes"
		   " WHERE FileBrowser IN (%u,%u,%u,%u,%u,%u)"
		     " AND Cod=%ld",
	           (unsigned) Brw_ADMI_DOC_CRS,
	           (unsigned) Brw_ADMI_TCH_CRS,
	           (unsigned) Brw_ADMI_SHR_CRS,
	           (unsigned) Brw_ADMI_ASG_USR,
	           (unsigned) Brw_ADMI_WRK_USR,
	           (unsigned) Brw_ADMI_MRK_CRS,
	           CrsCod);

   /* Remove from group file zones */
   DB_QueryDELETE ("can not remove sizes of file zones of a course",
		   "DELETE FROM brw_sizes"
		   " WHERE FileBrowser IN (%u,%u,%u,%u)"
		     " AND Cod IN %s",
	           (unsigned) Brw_ADMI_DOC_GRP,
	           (unsigned) Brw_ADMI_TCH_GRP,
	           (unsigned) Brw_ADMI_SHR_GRP,
	           (unsigned) Brw_ADMI_MRK_GRP,
	           SubqueryGrp);

   /* Remove from project file zones */
   DB_QueryDELETE ("can not remove sizes of file zones of a course",
		   "DELETE FROM brw_sizes"
		   " WHERE FileBrowser IN (%u,%u)"
		     " AND Cod IN %s",
	           (unsigned) Brw_ADMI_DOC_PRJ,
	           (unsigned) Brw_ADMI_ASS_PRJ,
	           SubqueryPrj);

   /***** Remove from database the entries that store the data files *****/
   /* Remove from course file zones */
   DB_QueryDELETE ("can not remove files of a course",
		   "DELETE FROM brw_files"
		   " WHERE FileBrowser IN (%u,%u,%u,%u,%u,%u)"
		     " AND Cod=%ld",
	           (unsigned) Brw_ADMI_DOC_CRS,
	           (unsigned) Brw_ADMI_TCH_CRS,
	           (unsigned) Brw_ADMI_SHR_CRS,
	           (unsigned) Brw_ADMI_ASG_USR,
	           (unsigned) Brw_ADMI_WRK_USR,
	           (unsigned) Brw_ADMI_MRK_CRS,
	           CrsCod);

   /* Remove from group file zones */
   DB_QueryDELETE ("can not remove files of a course",
		   "DELETE FROM brw_files"
		   " WHERE FileBrowser IN (%u,%u,%u,%u)"
		     " AND Cod IN %s",
	           (unsigned) Brw_ADMI_DOC_GRP,
	           (unsigned) Brw_ADMI_TCH_GRP,
	           (unsigned) Brw_ADMI_SHR_GRP,
	           (unsigned) Brw_ADMI_MRK_GRP,
	           SubqueryGrp);

   /* Remove from project file zones */
   DB_QueryDELETE ("can not remove files of a course",
		   "DELETE FROM brw_files"
		   " WHERE FileBrowser IN (%u,%u)"
		     " AND Cod IN %s",
	           (unsigned) Brw_ADMI_DOC_PRJ,
	           (unsigned) Brw_ADMI_ASS_PRJ,
	           SubqueryPrj);
  }

/*****************************************************************************/
/************ Remove files related to a group from the database **************/
/*****************************************************************************/

void Brw_DB_RemoveGrpFiles (long GrpCod)
  {
   /***** Remove format of files of marks *****/
   DB_QueryDELETE ("can not remove the properties of marks"
		   " associated to a group",
		   "DELETE FROM mrk_marks"
		   " USING brw_files,"
		          "mrk_marks"
		   " WHERE brw_files.FileBrowser=%u"
		     " AND brw_files.Cod=%ld"
		     " AND brw_files.FilCod=mrk_marks.FilCod",
	           (unsigned) Brw_ADMI_MRK_GRP,
	           GrpCod);

   /***** Remove from database the entries that store the file views *****/
   DB_QueryDELETE ("can not remove file views to files of a group",
		   "DELETE FROM brw_views"
		   " USING brw_files,"
		          "brw_views"
		   " WHERE brw_files.FileBrowser IN (%u,%u,%u,%u)"
		     " AND brw_files.Cod=%ld"
		     " AND brw_files.FilCod=brw_views.FilCod",
	           (unsigned) Brw_ADMI_DOC_GRP,
	           (unsigned) Brw_ADMI_TCH_GRP,
	           (unsigned) Brw_ADMI_SHR_GRP,
	           (unsigned) Brw_ADMI_MRK_GRP,
	           GrpCod);

   /***** Remove from database expanded folders *****/
   DB_QueryDELETE ("can not remove expanded folders of a group",
		   "DELETE LOW_PRIORITY FROM brw_expanded"
		   " WHERE FileBrowser IN (%u,%u,%u,%u)"
		   " AND Cod=%ld",
	           (unsigned) Brw_ADMI_DOC_GRP,
	           (unsigned) Brw_ADMI_TCH_GRP,
	           (unsigned) Brw_ADMI_SHR_GRP,
	           (unsigned) Brw_ADMI_MRK_GRP,
	           GrpCod);

   /***** Remove from database the entries that store clipboards *****/
   DB_QueryDELETE ("can not remove clipboards related to files of a group",
		   "DELETE FROM brw_clipboards"
		   " WHERE FileBrowser IN (%u,%u,%u,%u)"
		     " AND Cod=%ld",
	           (unsigned) Brw_ADMI_DOC_GRP,
	           (unsigned) Brw_ADMI_TCH_GRP,
	           (unsigned) Brw_ADMI_SHR_GRP,
	           (unsigned) Brw_ADMI_MRK_GRP,
	           GrpCod);

   /***** Remove from database the entries that store the last time users visited file zones *****/
   DB_QueryDELETE ("can not remove file last visits to files of a group",
		   "DELETE FROM brw_last"
		   " WHERE FileBrowser IN (%u,%u,%u,%u)"
		   " AND Cod=%ld",
	           (unsigned) Brw_ADMI_DOC_GRP,
	           (unsigned) Brw_ADMI_TCH_GRP,
	           (unsigned) Brw_ADMI_SHR_GRP,
	           (unsigned) Brw_ADMI_MRK_GRP,
	           GrpCod);

   /***** Remove from database the entries that store the sizes of the file zones *****/
   DB_QueryDELETE ("can not remove sizes of file zones of a group",
		   "DELETE FROM brw_sizes"
		   " WHERE FileBrowser IN (%u,%u,%u,%u)"
		   " AND Cod=%ld",
	           (unsigned) Brw_ADMI_DOC_GRP,
	           (unsigned) Brw_ADMI_TCH_GRP,
	           (unsigned) Brw_ADMI_SHR_GRP,
	           (unsigned) Brw_ADMI_MRK_GRP,
	           GrpCod);

   /***** Remove from database the entries that store the data files *****/
   DB_QueryDELETE ("can not remove files of a group",
		   "DELETE FROM brw_files"
		   " WHERE FileBrowser IN (%u,%u,%u,%u)"
		     " AND Cod=%ld",
	           (unsigned) Brw_ADMI_DOC_GRP,
	           (unsigned) Brw_ADMI_TCH_GRP,
	           (unsigned) Brw_ADMI_SHR_GRP,
	           (unsigned) Brw_ADMI_MRK_GRP,
	           GrpCod);
  }

/*****************************************************************************/
/*********** Remove files related to a project from the database *************/
/*****************************************************************************/

void Brw_DB_RemovePrjFiles (long PrjCod)
  {
   /***** Remove from database the entries that store the file views *****/
   DB_QueryDELETE ("can not remove file views to files of a project",
		   "DELETE FROM brw_views"
		   " USING brw_files,"
		          "brw_views"
		   " WHERE brw_files.FileBrowser IN (%u,%u)"
		     " AND brw_files.Cod=%ld"
		     " AND brw_files.FilCod=brw_views.FilCod",
	           (unsigned) Brw_ADMI_DOC_PRJ,
	           (unsigned) Brw_ADMI_ASS_PRJ,
	           PrjCod);

   /***** Remove from database expanded folders *****/
   DB_QueryDELETE ("can not remove expanded folders of a project",
		   "DELETE LOW_PRIORITY FROM brw_expanded"
		   " WHERE FileBrowser IN (%u,%u)"
		     " AND Cod=%ld",
	           (unsigned) Brw_ADMI_DOC_PRJ,
	           (unsigned) Brw_ADMI_ASS_PRJ,
	           PrjCod);

   /***** Remove from database the entries that store clipboards *****/
   DB_QueryDELETE ("can not remove clipboards related to files of a project",
		   "DELETE FROM brw_clipboards"
		   " WHERE FileBrowser IN (%u,%u)"
		     " AND Cod=%ld",
	           (unsigned) Brw_ADMI_DOC_PRJ,
	           (unsigned) Brw_ADMI_ASS_PRJ,
	           PrjCod);

   /***** Remove from database the entries that store the last time users visited file zones *****/
   DB_QueryDELETE ("can not remove file last visits to files of a project",
		   "DELETE FROM brw_last"
		   " WHERE FileBrowser IN (%u,%u)"
		     " AND Cod=%ld",
	           (unsigned) Brw_ADMI_DOC_PRJ,
	           (unsigned) Brw_ADMI_ASS_PRJ,
	           PrjCod);

   /***** Remove from database the entries that store the sizes of the file zones *****/
   DB_QueryDELETE ("can not remove sizes of file zones of a project",
		   "DELETE FROM brw_sizes"
		   " WHERE FileBrowser IN (%u,%u)"
		     " AND Cod=%ld",
	           (unsigned) Brw_ADMI_DOC_PRJ,
	           (unsigned) Brw_ADMI_ASS_PRJ,
	           PrjCod);

   /***** Remove from database the entries that store the data files *****/
   DB_QueryDELETE ("can not remove files of a project",
		   "DELETE FROM brw_files"
		   " WHERE FileBrowser IN (%u,%u)"
		     " AND Cod=%ld",
	           (unsigned) Brw_ADMI_DOC_PRJ,
	           (unsigned) Brw_ADMI_ASS_PRJ,
	           PrjCod);
  }

/*****************************************************************************/
/* Remove some info about files related to a course and a user from database */
/*****************************************************************************/

void Brw_DB_RemoveSomeInfoAboutCrsUsrFiles (long UsrCod,long CrsCod)
  {
   /***** Remove from database expanded folders *****/
   DB_QueryDELETE ("can not remove expanded folders for a user in a course",
		   "DELETE LOW_PRIORITY FROM brw_expanded"
		   " WHERE UsrCod=%ld"
		     " AND ("
			    "(FileBrowser IN (%u,%u,%u,%u,%u,%u,%u,%u)"
			    " AND Cod=%ld)"
			    " OR "
			    "(FileBrowser IN (%u,%u,%u,%u)"
			    " AND Cod IN"
			    " (SELECT grp_groups.GrpCod"
			       " FROM grp_types,"
			             "grp_groups"
			      " WHERE grp_types.CrsCod=%ld"
			        " AND grp_types.GrpTypCod=grp_groups.GrpTypCod))"
		          ")",
	           UsrCod,
	           (unsigned) Brw_ADMI_DOC_CRS,
	           (unsigned) Brw_ADMI_TCH_CRS,
	           (unsigned) Brw_ADMI_SHR_CRS,
	           (unsigned) Brw_ADMI_ASG_USR,
	           (unsigned) Brw_ADMI_ASG_CRS,
	           (unsigned) Brw_ADMI_WRK_USR,
	           (unsigned) Brw_ADMI_WRK_CRS,
	           (unsigned) Brw_ADMI_MRK_CRS,
	           CrsCod,
	           (unsigned) Brw_ADMI_DOC_GRP,
	           (unsigned) Brw_ADMI_TCH_GRP,
	           (unsigned) Brw_ADMI_SHR_GRP,
	           (unsigned) Brw_ADMI_MRK_GRP,
	           CrsCod);

   /***** Remove from database the entries that store clipboards *****/
   DB_QueryDELETE ("can not remove source of copy for a user in a course",
		   "DELETE FROM brw_clipboards"
		   " WHERE UsrCod=%ld"
		     " AND ("
			    "(FileBrowser IN (%u,%u,%u,%u,%u,%u,%u,%u)"
			    " AND Cod=%ld)"
			    " OR "
			    "(FileBrowser IN (%u,%u,%u,%u)"
			    " AND Cod IN"
			    " (SELECT grp_groups.GrpCod"
			       " FROM grp_types,"
			             "grp_groups"
			      " WHERE grp_types.CrsCod=%ld"
			        " AND grp_types.GrpTypCod=grp_groups.GrpTypCod))"
		          ")",
	           UsrCod,
	           (unsigned) Brw_ADMI_DOC_CRS,
	           (unsigned) Brw_ADMI_TCH_CRS,
	           (unsigned) Brw_ADMI_SHR_CRS,
	           (unsigned) Brw_ADMI_ASG_USR,
	           (unsigned) Brw_ADMI_ASG_CRS,
	           (unsigned) Brw_ADMI_WRK_USR,
	           (unsigned) Brw_ADMI_WRK_CRS,
	           (unsigned) Brw_ADMI_MRK_CRS,
	           CrsCod,
	           (unsigned) Brw_ADMI_DOC_GRP,
	           (unsigned) Brw_ADMI_TCH_GRP,
	           (unsigned) Brw_ADMI_SHR_GRP,
	           (unsigned) Brw_ADMI_MRK_GRP,
	           CrsCod);

   /***** Remove from database the entries that store the last time user visited file zones *****/
   // Assignments and works are stored as one in brw_last...
   // ...because a user views them at the same time
   DB_QueryDELETE ("can not remove file last visits to files of a course"
		   " from a user",
		   "DELETE FROM brw_last"
		   " WHERE UsrCod=%ld"
		     " AND ("
			    "(FileBrowser IN (%u,%u,%u,%u,%u)"
			    " AND Cod=%ld)"
			    " OR "
			    "(FileBrowser IN (%u,%u,%u,%u)"
			    " AND Cod IN"
			    " (SELECT grp_groups.GrpCod"
			       " FROM grp_types,"
			             "grp_groups"
			      " WHERE grp_types.CrsCod=%ld"
			        " AND grp_types.GrpTypCod=grp_groups.GrpTypCod))"
		          ")",
	           UsrCod,
	           (unsigned) Brw_ADMI_DOC_CRS,
	           (unsigned) Brw_ADMI_TCH_CRS,
	           (unsigned) Brw_ADMI_SHR_CRS,
	           (unsigned) Brw_ADMI_ASG_USR,
	           (unsigned) Brw_ADMI_MRK_CRS,
	           CrsCod,
	           (unsigned) Brw_ADMI_DOC_GRP,
	           (unsigned) Brw_ADMI_TCH_GRP,
	           (unsigned) Brw_ADMI_SHR_GRP,
	           (unsigned) Brw_ADMI_MRK_GRP,
	           CrsCod);
  }

/*****************************************************************************/
/*************** Remove user's works in a course from database ***************/
/*****************************************************************************/

void Brw_DB_RemoveWrkFiles (long CrsCod,long UsrCod)
  {
   /***** Remove from database the entries that store the file views *****/
   DB_QueryDELETE ("can not remove file views",
		   "DELETE FROM brw_views"
		   " USING brw_files,"
		          "brw_views"
		   " WHERE brw_files.FileBrowser IN (%u,%u)"
		     " AND brw_files.Cod=%ld"
		     " AND brw_files.ZoneUsrCod=%ld"
		     " AND brw_files.FilCod=brw_views.FilCod",
	           (unsigned) Brw_ADMI_ASG_USR,
	           (unsigned) Brw_ADMI_WRK_USR,
	           CrsCod,UsrCod);

   /***** Remove from database expanded folders *****/
   DB_QueryDELETE ("can not remove expanded folders of a group",
		   "DELETE LOW_PRIORITY FROM brw_expanded"
		   " WHERE FileBrowser IN (%u,%u)"
		     " AND Cod=%ld"
		     " AND WorksUsrCod=%ld",
	           (unsigned) Brw_ADMI_ASG_CRS,
	           (unsigned) Brw_ADMI_WRK_CRS,
	           CrsCod,UsrCod);

   /***** Remove from database the entries that store clipboards *****/
   DB_QueryDELETE ("can not remove clipboards",
		   "DELETE FROM brw_clipboards"
		   " WHERE FileBrowser IN (%u,%u)"
		     " AND Cod=%ld"
		     " AND WorksUsrCod=%ld",
	           (unsigned) Brw_ADMI_ASG_CRS,
	           (unsigned) Brw_ADMI_WRK_CRS,
	           CrsCod,UsrCod);

   /***** Remove from database the entries that store the sizes of the file zones *****/
   DB_QueryDELETE ("can not remove file browser sizes",
		   "DELETE FROM brw_sizes"
		   " WHERE FileBrowser IN (%u,%u)"
		     " AND Cod=%ld"
		     " AND ZoneUsrCod=%ld",
	           (unsigned) Brw_ADMI_ASG_USR,
	           (unsigned) Brw_ADMI_WRK_USR,
	           CrsCod,UsrCod);

   /***** Remove from database the entries that store the data files *****/
   DB_QueryDELETE ("can not remove files",
		   "DELETE FROM brw_files"
		   " WHERE FileBrowser IN (%u,%u)"
		     " AND Cod=%ld"
		     " AND ZoneUsrCod=%ld",
	           (unsigned) Brw_ADMI_ASG_USR,
	           (unsigned) Brw_ADMI_WRK_USR,
	           CrsCod,UsrCod);
  }

/*****************************************************************************/
/************* Remove files related to a user from the database **************/
/*****************************************************************************/

void Brw_DB_RemoveUsrFiles (long UsrCod)
  {
   /***** Remove from database the entries that store the file views *****/
   // User is not removed from brw_views table,
   // in order to take into account his/her views
   DB_QueryDELETE ("can not remove file views to files of a user",
		   "DELETE FROM brw_views"
		   " USING brw_files,"
		          "brw_views"
		   " WHERE brw_files.ZoneUsrCod=%ld"
		     " AND brw_files.FilCod=brw_views.FilCod",
	           UsrCod);

   /***** Remove from database expanded folders *****/
   DB_QueryDELETE ("can not remove expanded folders for a user",
		   "DELETE LOW_PRIORITY FROM brw_expanded"
		   " WHERE UsrCod=%ld",
	           UsrCod);

   /***** Remove from database the entries that store clipboards *****/
   DB_QueryDELETE ("can not remove user's clipboards",
		   "DELETE FROM brw_clipboards"
		   " WHERE UsrCod=%ld",	// User's clipboard
	           UsrCod);

   /***** Remove from database the entries that store the last time users visited file zones *****/
   DB_QueryDELETE ("can not remove user's last visits to file zones",
		   "DELETE FROM brw_last"
		   " WHERE UsrCod=%ld",	// User's last visits to all zones
	           UsrCod);

   /***** Remove from database the entries that store the sizes of the file zones *****/
   DB_QueryDELETE ("can not remove sizes of user's file zones",
		   "DELETE FROM brw_sizes"
		   " WHERE ZoneUsrCod=%ld",
	           UsrCod);

   /***** Remove from database the entries that store the data files *****/
   DB_QueryDELETE ("can not remove files in user's file zones",
		   "DELETE FROM brw_files"
		   " WHERE ZoneUsrCod=%ld",
	           UsrCod);
  }
