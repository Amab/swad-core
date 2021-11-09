// swad_project_database.c: projects (final degree projects, thesis), operations with database

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

#define _GNU_SOURCE 		// For asprintf
#include <stdio.h>		// For asprintf
#include <stdlib.h>		// For free

#include "swad_database.h"
#include "swad_error.h"
#include "swad_global.h"
#include "swad_project.h"

/*****************************************************************************/
/************** External global variables from others modules ****************/
/*****************************************************************************/

extern struct Globals Gbl;

/*****************************************************************************/
/************************** Public constants and types ***********************/
/*****************************************************************************/

/***** Enum field in database for types of proposal *****/
const char *Prj_Proposal_DB[Prj_NUM_PROPOSAL_TYPES] =
  {
   [Prj_PROPOSAL_NEW       ] = "new",
   [Prj_PROPOSAL_MODIFIED  ] = "modified",
   [Prj_PROPOSAL_UNMODIFIED] = "unmodified",
  };

/*****************************************************************************/
/************ Update configuration of projects for current course ************/
/*****************************************************************************/

void Prj_DB_UpdateCrsPrjsConfig (bool Editable)
  {
   DB_QueryREPLACE ("can not save configuration of projects",
		    "REPLACE INTO prj_config"
	            " (CrsCod,Editable)"
                    " VALUES"
                    " (%ld,'%c')",
		    Gbl.Hierarchy.Crs.CrsCod,
		    Editable ? 'Y' :
			       'N');
  }

/*****************************************************************************/
/**************************** Lock project edition ***************************/
/*****************************************************************************/

void Prj_DB_LockProjectEdition (long PrjCod)
  {
   DB_QueryUPDATE ("can not lock project edition",
		   "UPDATE prj_projects"
		     " SET Locked='Y'"
		   " WHERE PrjCod=%ld"
		     " AND CrsCod=%ld",
		   PrjCod,
		   Gbl.Hierarchy.Crs.CrsCod);
  }

/*****************************************************************************/
/************************** Unlock project edition ***************************/
/*****************************************************************************/

void Prj_DB_UnlockProjectEdition (long PrjCod)
  {
   DB_QueryUPDATE ("can not lock project edition",
		   "UPDATE prj_projects"
		     " SET Locked='N'"
		   " WHERE PrjCod=%ld"
		     " AND CrsCod=%ld",	// Extra check
		   PrjCod,
		   Gbl.Hierarchy.Crs.CrsCod);
  }

/*****************************************************************************/
/************************** Create a new project *****************************/
/*****************************************************************************/

long Prj_DB_CreateProject (const struct Prj_Project *Prj)
  {
   return
   DB_QueryINSERTandReturnCode ("can not create new project",
				"INSERT INTO prj_projects"
				" (CrsCod,DptCod,Hidden,Assigned,NumStds,Proposal,"
				  "CreatTime,ModifTime,"
				  "Title,Description,Knowledge,Materials,URL)"
				" VALUES"
				" (%ld,%ld,'%c','%c',%u,'%s',"
				  "FROM_UNIXTIME(%ld),FROM_UNIXTIME(%ld),"
				  "'%s','%s','%s','%s','%s')",
				Gbl.Hierarchy.Crs.CrsCod,
				Prj->DptCod,
				Prj->Hidden == Prj_HIDDEN ? 'Y' :
							    'N',
				Prj->Assigned == Prj_ASSIGNED ? 'Y' :
								'N',
				Prj->NumStds,
				Prj_Proposal_DB[Prj->Proposal],
				Prj->CreatTime,
				Prj->ModifTime,
				Prj->Title,
				Prj->Description,
				Prj->Knowledge,
				Prj->Materials,
				Prj->URL);
  }

/*****************************************************************************/
/*********************** Update an existing project **************************/
/*****************************************************************************/

void Prj_DB_UpdateProject (const struct Prj_Project *Prj)
  {
   DB_QueryUPDATE ("can not update project",
		   "UPDATE prj_projects"
		     " SET DptCod=%ld,"
		          "Hidden='%c',"
		          "Assigned='%c',"
		          "NumStds=%u,"
		          "Proposal='%s',"
		          "ModifTime=FROM_UNIXTIME(%ld),"
		          "Title='%s',"
		          "Description='%s',"
		          "Knowledge='%s',"
		          "Materials='%s',"
		          "URL='%s'"
		   " WHERE PrjCod=%ld"
		     " AND CrsCod=%ld",	// Extra check
	           Prj->DptCod,
	           Prj->Hidden == Prj_HIDDEN ? 'Y' :
					       'N',
	           Prj->Assigned == Prj_ASSIGNED ? 'Y' :
						   'N',
	           Prj->NumStds,
	           Prj_Proposal_DB[Prj->Proposal],
	           Prj->ModifTime,
	           Prj->Title,
	           Prj->Description,
	           Prj->Knowledge,
	           Prj->Materials,
	           Prj->URL,
	           Prj->PrjCod,
	           Gbl.Hierarchy.Crs.CrsCod);
  }

/*****************************************************************************/
/****************************** Add user to project **************************/
/*****************************************************************************/

void Prj_DB_AddUsrToPrj (long PrjCod,Prj_RoleInProject_t RoleInProject,long UsrCod)
  {
   DB_QueryREPLACE ("can not add user to project",
		    "REPLACE INTO prj_users"
		    " (PrjCod,RoleInProject,UsrCod)"
		    " VALUES"
		    " (%ld,%u,%ld)",
		    PrjCod,
		    (unsigned) RoleInProject,
		    UsrCod);
  }

/*****************************************************************************/
/****************************** Hide a project *******************************/
/*****************************************************************************/

void Prj_DB_HideOrUnhideProject (long PrjCod,bool Hide)
  {
   DB_QueryUPDATE ("can not hide/unhide project",
		   "UPDATE prj_projects"
		     " SET Hidden='%c'"
		   " WHERE PrjCod=%ld"
		     " AND CrsCod=%ld",	// Extra check
		   Hide ? 'Y' :
			  'N',
		   PrjCod,
		   Gbl.Hierarchy.Crs.CrsCod);
  }

/*****************************************************************************/
/******************************** Get projects *******************************/
/*****************************************************************************/

unsigned Prj_DB_GetListProjects (MYSQL_RES **mysql_res,
                                 const struct Prj_Projects *Projects,
                                 const char *UsrsSubQuery)	// NULL if no users
  {
   char *AssignSubQuery;
   char *HidVisSubQuery;
   char *DptCodSubQuery;
   static const char *OrderBySubQuery[Prj_NUM_ORDERS] =
     {
      [Prj_ORDER_START_TIME] = "prj_projects.CreatTime DESC,"
			       "prj_projects.ModifTime DESC,"
			       "prj_projects.Title",
      [Prj_ORDER_END_TIME  ] = "prj_projects.ModifTime DESC,"
			       "prj_projects.CreatTime DESC,"
			       "prj_projects.Title",
      [Prj_ORDER_TITLE     ] = "prj_projects.Title,"
			       "prj_projects.CreatTime DESC,"
			       "prj_projects.ModifTime DESC",
      [Prj_ORDER_DEPARTMENT] = "dpt_departments.FullName,"
			       "prj_projects.CreatTime DESC,"
			       "prj_projects.ModifTime DESC,"
			       "prj_projects.Title",
     };
   unsigned NumPrjsFromDB = 0;

   /* Assigned subquery */
   switch (Projects->Filter.Assign)
     {
      case (1 << Prj_ASSIGNED):	// Assigned projects
	 if (asprintf (&AssignSubQuery," AND prj_projects.Assigned='Y'") < 0)
	    Err_NotEnoughMemoryExit ();
	 break;
      case (1 << Prj_NONASSIG):	// Non-assigned projects
	 if (asprintf (&AssignSubQuery," AND prj_projects.Assigned='N'") < 0)
	    Err_NotEnoughMemoryExit ();
	 break;
      default:			// All projects
	 if (asprintf (&AssignSubQuery,"%s","") < 0)
	    Err_NotEnoughMemoryExit ();
	 break;
     }

   /* Hidden subquery */
   switch (Gbl.Usrs.Me.Role.Logged)
     {
      case Rol_STD:	// Students can view only visible projects
	 if (asprintf (&HidVisSubQuery," AND prj_projects.Hidden='N'") < 0)
	    Err_NotEnoughMemoryExit ();
	 break;
      case Rol_NET:
      case Rol_TCH:
      case Rol_SYS_ADM:
	 switch (Projects->Filter.Hidden)
	   {
	    case (1 << Prj_HIDDEN):	// Hidden projects
	       if (asprintf (&HidVisSubQuery," AND prj_projects.Hidden='Y'") < 0)
		  Err_NotEnoughMemoryExit ();
	       break;
	    case (1 << Prj_VISIBL):	// Visible projects
	       if (asprintf (&HidVisSubQuery," AND prj_projects.Hidden='N'") < 0)
		  Err_NotEnoughMemoryExit ();
	       break;
	    default:			// All projects
	       if (asprintf (&HidVisSubQuery,"%s","") < 0)
		  Err_NotEnoughMemoryExit ();
	       break;
	   }
	 break;
      default:
	 Err_WrongRoleExit ();
	 break;
     }

   /* Department subquery */
   if (Projects->Filter.DptCod >= 0)
     {
      if (asprintf (&DptCodSubQuery," AND prj_projects.DptCod=%ld",
		    Projects->Filter.DptCod) < 0)
	 Err_NotEnoughMemoryExit ();
     }
   else	// Any department
     {
      if (asprintf (&DptCodSubQuery,"%s","") < 0)
	 Err_NotEnoughMemoryExit ();
     }

   /* Query */
   switch (Projects->Filter.Who)
     {
      case Usr_WHO_ME:
	 /* Get list of projects */
	 switch (Projects->SelectedOrder)
	   {
	    case Prj_ORDER_START_TIME:
	    case Prj_ORDER_END_TIME:
	    case Prj_ORDER_TITLE:
	       NumPrjsFromDB = (unsigned)
	       DB_QuerySELECT (mysql_res,"can not get projects",
			       "SELECT prj_projects.PrjCod"
				" FROM prj_projects,"
				      "prj_users"
			       " WHERE prj_projects.CrsCod=%ld"
				 "%s"
				 "%s"
				 "%s"
				 " AND prj_projects.PrjCod=prj_users.PrjCod"
				 " AND prj_users.UsrCod=%ld"
			       " GROUP BY prj_projects.PrjCod"	// To not repeat projects (DISTINCT can not be used)
			       " ORDER BY %s",
			       Gbl.Hierarchy.Crs.CrsCod,
			       AssignSubQuery,
			       HidVisSubQuery,
			       DptCodSubQuery,
			       Gbl.Usrs.Me.UsrDat.UsrCod,
			       OrderBySubQuery[Projects->SelectedOrder]);
	       break;
	    case Prj_ORDER_DEPARTMENT:
	       NumPrjsFromDB = (unsigned)
	       DB_QuerySELECT (mysql_res,"can not get projects",
			       "SELECT prj_projects.PrjCod"
				" FROM prj_projects LEFT JOIN dpt_departments,"
				      "prj_users"
				  " ON prj_projects.DptCod=dpt_departments.DptCod"
			       " WHERE prj_projects.CrsCod=%ld"
				 "%s"
				 "%s"
				 "%s"
				 " AND prj_projects.PrjCod=prj_users.PrjCod"
				 " AND prj_users.UsrCod=%ld"
			       " GROUP BY prj_projects.PrjCod"	// To not repeat projects (DISTINCT can not be used)
			       " ORDER BY %s",
			       Gbl.Hierarchy.Crs.CrsCod,
			       AssignSubQuery,
			       HidVisSubQuery,
			       DptCodSubQuery,
			       Gbl.Usrs.Me.UsrDat.UsrCod,
			       OrderBySubQuery[Projects->SelectedOrder]);
	       break;
	   }
	 break;
      case Usr_WHO_SELECTED:
	 if (UsrsSubQuery)
	   {
	    /* Get list of projects */
	    switch (Projects->SelectedOrder)
	      {
	       case Prj_ORDER_START_TIME:
	       case Prj_ORDER_END_TIME:
	       case Prj_ORDER_TITLE:
		  NumPrjsFromDB = (unsigned)
		  DB_QuerySELECT (mysql_res,"can not get projects",
				  "SELECT prj_projects.PrjCod"
				   " FROM prj_projects,"
					 "prj_users"
				  " WHERE prj_projects.CrsCod=%ld"
				    "%s"
				    "%s"
				    "%s"
				    " AND prj_projects.PrjCod=prj_users.PrjCod"
				    " AND prj_users.UsrCod IN (%s)"
				  " GROUP BY prj_projects.PrjCod"	// To not repeat projects (DISTINCT can not be used)
				  " ORDER BY %s",
				  Gbl.Hierarchy.Crs.CrsCod,
				  AssignSubQuery,
				  HidVisSubQuery,
				  DptCodSubQuery,
				  UsrsSubQuery,
				  OrderBySubQuery[Projects->SelectedOrder]);
		  break;
	       case Prj_ORDER_DEPARTMENT:
		  NumPrjsFromDB = (unsigned)
		  DB_QuerySELECT (mysql_res,"can not get projects",
				  "SELECT prj_projects.PrjCod"
				   " FROM prj_projects LEFT JOIN dpt_departments,"
					 "prj_users"
				     " ON prj_projects.DptCod=dpt_departments.DptCod"
				  " WHERE prj_projects.CrsCod=%ld"
				    "%s"
				    "%s"
				    "%s"
				    " AND prj_projects.PrjCod=prj_users.PrjCod"
				    " AND prj_users.UsrCod IN (%s)"
				  " GROUP BY prj_projects.PrjCod"	// To not repeat projects (DISTINCT can not be used)
				  " ORDER BY %s",
				  Gbl.Hierarchy.Crs.CrsCod,
				  AssignSubQuery,
				  HidVisSubQuery,
				  DptCodSubQuery,
				  UsrsSubQuery,
				  OrderBySubQuery[Projects->SelectedOrder]);
		  break;
	      }
	   }
	 break;
      case Usr_WHO_ALL:
	 /* Get list of projects */
	 switch (Projects->SelectedOrder)
	   {
	    case Prj_ORDER_START_TIME:
	    case Prj_ORDER_END_TIME:
	    case Prj_ORDER_TITLE:
	       NumPrjsFromDB = (unsigned)
	       DB_QuerySELECT (mysql_res,"can not get projects",
			       "SELECT prj_projects.PrjCod"
				" FROM prj_projects"
			       " WHERE prj_projects.CrsCod=%ld"
				 "%s"
				 "%s"
				 "%s"
			       " ORDER BY %s",
			       Gbl.Hierarchy.Crs.CrsCod,
			       AssignSubQuery,
			       HidVisSubQuery,
			       DptCodSubQuery,
			       OrderBySubQuery[Projects->SelectedOrder]);
	       break;
	    case Prj_ORDER_DEPARTMENT:
	       NumPrjsFromDB = (unsigned)
	       DB_QuerySELECT (mysql_res,"can not get projects",
			       "SELECT prj_projects.PrjCod"
				" FROM prj_projects LEFT JOIN dpt_departments"
				  " ON prj_projects.DptCod=dpt_departments.DptCod"
			       " WHERE prj_projects.CrsCod=%ld"
				 "%s"
				 "%s"
				 "%s"
			       " ORDER BY %s",
			       Gbl.Hierarchy.Crs.CrsCod,
			       AssignSubQuery,
			       HidVisSubQuery,
			       DptCodSubQuery,
			       OrderBySubQuery[Projects->SelectedOrder]);
	       break;
	   }
	 break;
      default:
	 Err_WrongWhoExit ();
	 break;
     }

   /* Free allocated memory for subqueries */
   free (AssignSubQuery);
   free (HidVisSubQuery);
   free (DptCodSubQuery);

   return NumPrjsFromDB;
  }

/*****************************************************************************/
/************** Get configuration of projects for current course *************/
/*****************************************************************************/

unsigned Prj_DB_GetCrsPrjsConfig (MYSQL_RES **mysql_res)
  {
   return (unsigned)
   DB_QuerySELECT (mysql_res,"can not get configuration of test",
		   "SELECT Editable"		// row[0]
		    " FROM prj_config"
		   " WHERE CrsCod=%ld",
		   Gbl.Hierarchy.Crs.CrsCod);
  }

/*****************************************************************************/
/********************* Get project data using its code ***********************/
/*****************************************************************************/

unsigned Prj_DB_GetDataOfProjectByCod (MYSQL_RES **mysql_res,long PrjCod)
  {
   return (unsigned)
   DB_QuerySELECT (mysql_res,"can not get project data",
		   "SELECT PrjCod,"			// row[ 0]
			  "CrsCod,"			// row[ 1]
			  "DptCod,"			// row[ 2]
			  "Locked,"			// row[ 3]
			  "Hidden,"			// row[ 4]
			  "Assigned,"			// row[ 5]
			  "NumStds,"			// row[ 6]
			  "Proposal,"			// row[ 7]
			  "UNIX_TIMESTAMP(CreatTime),"	// row[ 8]
			  "UNIX_TIMESTAMP(ModifTime),"	// row[ 9]
			  "Title,"			// row[10]
			  "Description,"		// row[11]
			  "Knowledge,"			// row[12]
			  "Materials,"			// row[13]
			  "URL"				// row[14]
		    " FROM prj_projects"
		   " WHERE PrjCod=%ld"
		     " AND CrsCod=%ld",	// Extra check
		   PrjCod,
		   Gbl.Hierarchy.Crs.CrsCod);
  }

/*****************************************************************************/
/******************* Get some project data to check faults *******************/
/*****************************************************************************/

unsigned Prj_DB_GetPrjDataToCheckFaults (MYSQL_RES **mysql_res,long PrjCod)
  {
   return (unsigned)
   DB_QuerySELECT (mysql_res,"can not get project data",
		   "SELECT Assigned='Y',"		// row[0] = 0 / 1
			  "NumStds,"			// row[1] =
			  "Title<>'',"			// row[2] = 0 / 1
			  "Description<>''"		// row[3] = 0 / 1
		    " FROM prj_projects"
		   " WHERE PrjCod=%ld",
		   PrjCod);
  }

/*****************************************************************************/
/*************** Get number of users with a role in a project ****************/
/*****************************************************************************/

unsigned Prj_DB_GetNumUsrsInPrj (long PrjCod,Prj_RoleInProject_t RoleInProject)
  {
   return (unsigned)
   DB_QueryCOUNT ("can not get number of users in project",
		  "SELECT COUNT(UsrCod)"
		   " FROM prj_users"
		  " WHERE PrjCod=%ld"
		    " AND RoleInProject=%u",
		  PrjCod,
		  (unsigned) RoleInProject);
  }

/*****************************************************************************/
/******************** Get users with a role in a project *********************/
/*****************************************************************************/

unsigned Prj_DB_GetUsrsInPrj (MYSQL_RES **mysql_res,
                              long PrjCod,Prj_RoleInProject_t RoleInProject)
  {
   return (unsigned)
   DB_QuerySELECT (mysql_res,"can not get users in project",
		   "SELECT prj_users.UsrCod,"		// row[0]
			  "usr_data.Surname1 AS S1,"	// row[1]
			  "usr_data.Surname2 AS S2,"	// row[2]
			  "usr_data.FirstName AS FN"	// row[3]
		    " FROM prj_users,"
			  "usr_data"
		   " WHERE prj_users.PrjCod=%ld"
		     " AND prj_users.RoleInProject=%u"
		     " AND prj_users.UsrCod=usr_data.UsrCod"
		   " ORDER BY S1,"
			     "S2,"
			     "FN",
		   PrjCod,
		   (unsigned) RoleInProject);
  }

/*****************************************************************************/
/************************ Get my roles in a project **************************/
/*****************************************************************************/

unsigned Prj_DB_GetMyRolesInPrj (MYSQL_RES **mysql_res,long PrjCod)
  {
   return (unsigned)
   DB_QuerySELECT (mysql_res,"can not get my roles in project",
		   "SELECT RoleInProject"	// row[0]
		    " FROM prj_users"
		   " WHERE PrjCod=%ld"
		     " AND UsrCod=%ld",
		   PrjCod,
		   Gbl.Usrs.Me.UsrDat.UsrCod);
  }

/*****************************************************************************/
/************************** Get course of a project **************************/
/*****************************************************************************/

long Prj_DB_GetCrsOfPrj (long PrjCod)
  {
   /***** Trivial check: project code should be > 0 *****/
   if (PrjCod <= 0)
      return -1L;

   /***** Get course code from database *****/
   return DB_QuerySELECTCode ("can not get project course",
			      "SELECT CrsCod"		// row[0]
			       " FROM prj_projects"
			      " WHERE PrjCod=%ld",
			      PrjCod); // Project found...
  }

/*****************************************************************************/
/******************** Get number of courses with projects ********************/
/*****************************************************************************/
// Returns the number of courses with projects
// in this location (all the platform, current degree or current course)

unsigned Prj_DB_GetNumCoursesWithProjects (HieLvl_Level_t Scope)
  {
   /***** Get number of courses with projects from database *****/
   switch (Scope)
     {
      case HieLvl_SYS:
	 return (unsigned)
         DB_QueryCOUNT ("can not get number of courses with projects",
			"SELECT COUNT(DISTINCT CrsCod)"
			 " FROM prj_projects"
			" WHERE CrsCod>0");
      case HieLvl_CTY:
         return (unsigned)
         DB_QueryCOUNT ("can not get number of courses with projects",
			"SELECT COUNT(DISTINCT prj_projects.CrsCod)"
			 " FROM ins_instits,"
			       "ctr_centers,"
			       "deg_degrees,"
			       "crs_courses,"
			       "prj_projects"
			" WHERE ins_instits.CtyCod=%ld"
			  " AND ins_instits.InsCod=ctr_centers.InsCod"
			  " AND ctr_centers.CtrCod=deg_degrees.CtrCod"
			  " AND deg_degrees.DegCod=crs_courses.DegCod"
			  " AND crs_courses.CrsCod=prj_projects.CrsCod",
                        Gbl.Hierarchy.Cty.CtyCod);
      case HieLvl_INS:
         return (unsigned)
         DB_QueryCOUNT ("can not get number of courses with projects",
			"SELECT COUNT(DISTINCT prj_projects.CrsCod)"
			 " FROM ctr_centers,"
			       "deg_degrees,"
			       "crs_courses,"
			       "prj_projects"
			" WHERE ctr_centers.InsCod=%ld"
			  " AND ctr_centers.CtrCod=deg_degrees.CtrCod"
			  " AND deg_degrees.DegCod=crs_courses.DegCod"
			  " AND crs_courses.CrsCod=prj_projects.CrsCod",
                        Gbl.Hierarchy.Ins.InsCod);
      case HieLvl_CTR:
         return (unsigned)
         DB_QueryCOUNT ("can not get number of courses with projects",
			"SELECT COUNT(DISTINCT prj_projects.CrsCod)"
			 " FROM deg_degrees,"
			       "crs_courses,"
			      "prj_projects"
			" WHERE deg_degrees.CtrCod=%ld"
			  " AND deg_degrees.DegCod=crs_courses.DegCod"
			  " AND crs_courses.CrsCod=prj_projects.CrsCod",
                        Gbl.Hierarchy.Ctr.CtrCod);
      case HieLvl_DEG:
         return (unsigned)
         DB_QueryCOUNT ("can not get number of courses with projects",
			"SELECT COUNT(DISTINCT prj_projects.CrsCod)"
			 " FROM crs_courses,"
			       "prj_projects"
			" WHERE crs_courses.DegCod=%ld"
			  " AND crs_courses.CrsCod=prj_projects.CrsCod",
                        Gbl.Hierarchy.Deg.DegCod);
      case HieLvl_CRS:
         return (unsigned)
         DB_QueryCOUNT ("can not get number of courses with projects",
			"SELECT COUNT(DISTINCT CrsCod)"
			 " FROM prj_projects"
			" WHERE CrsCod=%ld",
			Gbl.Hierarchy.Crs.CrsCod);
      default:
	 Err_WrongScopeExit ();
	 return 0;	// Not reached
     }
  }

/*****************************************************************************/
/************************** Get number of projects ***************************/
/*****************************************************************************/
// Returns the number of projects in this location

unsigned Prj_DB_GetNumProjects (HieLvl_Level_t Scope)
  {
   /***** Get number of projects from database *****/
   switch (Scope)
     {
      case HieLvl_SYS:
         return (unsigned)
         DB_QueryCOUNT ("can not get number of projects",
			"SELECT COUNT(*)"
                         " FROM prj_projects"
                        " WHERE CrsCod>0");
      case HieLvl_CTY:
         return (unsigned)
         DB_QueryCOUNT ("can not get number of projects",
			"SELECT COUNT(*)"
			 " FROM ins_instits,"
			       "ctr_centers,"
			       "deg_degrees,"
			       "crs_courses,"
			       "prj_projects"
			" WHERE ins_instits.CtyCod=%ld"
			  " AND ins_instits.InsCod=ctr_centers.InsCod"
			  " AND ctr_centers.CtrCod=deg_degrees.CtrCod"
			  " AND deg_degrees.DegCod=crs_courses.DegCod"
			  " AND crs_courses.CrsCod=prj_projects.CrsCod",
                        Gbl.Hierarchy.Cty.CtyCod);
      case HieLvl_INS:
         return (unsigned)
         DB_QueryCOUNT ("can not get number of projects",
			"SELECT COUNT(*)"
			 " FROM ctr_centers,"
			       "deg_degrees,"
			       "crs_courses,"
			       "prj_projects"
			" WHERE ctr_centers.InsCod=%ld"
			  " AND ctr_centers.CtrCod=deg_degrees.CtrCod"
			  " AND deg_degrees.DegCod=crs_courses.DegCod"
			  " AND crs_courses.CrsCod=prj_projects.CrsCod",
                        Gbl.Hierarchy.Ins.InsCod);
      case HieLvl_CTR:
         return (unsigned)
         DB_QueryCOUNT ("can not get number of projects",
			"SELECT COUNT(*)"
			 " FROM deg_degrees,"
			       "crs_courses,"
			       "prj_projects"
			" WHERE deg_degrees.CtrCod=%ld"
			  " AND deg_degrees.DegCod=crs_courses.DegCod"
			  " AND crs_courses.CrsCod=prj_projects.CrsCod",
                        Gbl.Hierarchy.Ctr.CtrCod);
      case HieLvl_DEG:
         return (unsigned)
         DB_QueryCOUNT ("can not get number of projects",
			"SELECT COUNT(*)"
			 " FROM crs_courses,"
			       "prj_projects"
			" WHERE crs_courses.DegCod=%ld"
			  " AND crs_courses.CrsCod=prj_projects.CrsCod",
                        Gbl.Hierarchy.Deg.DegCod);
      case HieLvl_CRS:
         return (unsigned)
         DB_QueryCOUNT ("can not get number of projects",
			"SELECT COUNT(*)"
			 " FROM prj_projects"
			" WHERE CrsCod=%ld",
                        Gbl.Hierarchy.Crs.CrsCod);
      default:
	 Err_WrongScopeExit ();
	 return 0;	// Not reached
     }
  }

/*****************************************************************************/
/************************ Remove user from a project *************************/
/*****************************************************************************/

void Prj_DB_RemoveUsrFromPrj (long PrjCod,Prj_RoleInProject_t RoleInPrj,long UsrCod)
  {
   DB_QueryDELETE ("can not remove a user from a project",
		   "DELETE FROM prj_users"
		   " WHERE PrjCod=%ld"
		     " AND RoleInProject=%u"
		     " AND UsrCod=%ld",
		   PrjCod,
		   (unsigned) RoleInPrj,
		   UsrCod);
  }

/*****************************************************************************/
/******************* Remove user from all his/her projects *******************/
/*****************************************************************************/

void Prj_DB_RemoveUsrFromProjects (long UsrCod)
  {
   DB_QueryDELETE ("can not remove user from projects",
		   "DELETE FROM prj_users"
		   " WHERE UsrCod=%ld",
		   UsrCod);
  }

/*****************************************************************************/
/********************* Remove all users from a project ***********************/
/*****************************************************************************/

void Prj_DB_RemoveUsrsFromPrj (long PrjCod)
  {
   DB_QueryDELETE ("can not remove project",
		   "DELETE FROM prj_users"
		   " USING prj_projects,"
			  "prj_users"
		   " WHERE prj_projects.PrjCod=%ld"
		     " AND prj_projects.CrsCod=%ld"	// Extra check
		     " AND prj_projects.PrjCod=prj_users.PrjCod",
		   PrjCod,
		   Gbl.Hierarchy.Crs.CrsCod);
  }

/*****************************************************************************/
/************** Remove all users from all projects in a course ***************/
/*****************************************************************************/

void Prj_DB_RemoveUsrsFromCrsPrjs (long CrsCod)
  {
   DB_QueryDELETE ("can not remove all projects in a course",
		   "DELETE FROM prj_users"
		   " USING prj_projects,"
		          "prj_users"
		   " WHERE prj_projects.CrsCod=%ld"
		     " AND prj_projects.PrjCod=prj_users.PrjCod",
                   CrsCod);
  }

/*****************************************************************************/
/*************** Remove configuration of projects in the course **************/
/*****************************************************************************/

void Prj_DB_RemoveConfigOfCrsPrjs (long CrsCod)
  {
   DB_QueryDELETE ("can not remove configuration of projects in a course",
		   "DELETE FROM prj_config"
		   " WHERE CrsCod=%ld",
		   CrsCod);
  }

/*****************************************************************************/
/******************************* Remove project ******************************/
/*****************************************************************************/

void Prj_DB_RemovePrj (long PrjCod)
  {
   DB_QueryDELETE ("can not remove project",
		   "DELETE FROM prj_projects"
		   " WHERE PrjCod=%ld"
		     " AND CrsCod=%ld",	// Extra check
		   PrjCod,
		   Gbl.Hierarchy.Crs.CrsCod);
  }

/*****************************************************************************/
/********************* Remove all projects in a course ***********************/
/*****************************************************************************/

void Prj_DB_RemoveCrsPrjs (long CrsCod)
  {
   DB_QueryDELETE ("can not remove all projects of a course",
		   "DELETE FROM prj_projects"
		   " WHERE CrsCod=%ld",
		   CrsCod);
  }
