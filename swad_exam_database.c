// swad_exam_database.c: exams operations with database

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

#include "swad_database.h"
#include "swad_error.h"
#include "swad_exam_database.h"
#include "swad_exam_log.h"
#include "swad_exam_print.h"
#include "swad_exam_set.h"
#include "swad_global.h"

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

static void Exa_DB_RemoveUsrSesResultsInCrs (long UsrCod,long CrsCod,const char *TableName);


/*****************************************************************************/
/**************************** Create a new exam ******************************/
/*****************************************************************************/

long Exa_DB_CreateExam (const struct Exa_Exam *Exam,const char *Txt)
  {
   long ExaCod;

   Str_SetDecimalPointToUS ();		// To write the decimal point as a dot
   ExaCod =
   DB_QueryINSERTandReturnCode ("can not create new exam",
				"INSERT INTO exa_exams"
				" (CrsCod,Hidden,UsrCod,MaxGrade,Visibility,"
				  "Title,Txt)"
				" VALUES"
				" (%ld,'N',%ld,%.15lg,%u,"
				  "'%s','%s')",
				Gbl.Hierarchy.Crs.CrsCod,
				Gbl.Usrs.Me.UsrDat.UsrCod,
				Exam->MaxGrade,
				Exam->Visibility,
				Exam->Title,
				Txt);
   Str_SetDecimalPointToLocal ();	// Return to local system

   return ExaCod;
  }

/*****************************************************************************/
/************************* Update an existing exam *************************/
/*****************************************************************************/

void Exa_DB_UpdateExam (const struct Exa_Exam *Exam,const char *Txt)
  {
   Str_SetDecimalPointToUS ();		// To write the decimal point as a dot
   DB_QueryUPDATE ("can not update exam",
		   "UPDATE exa_exams"
		     " SET CrsCod=%ld,"
		          "MaxGrade=%.15lg,"
		          "Visibility=%u,"
		          "Title='%s',"
		          "Txt='%s'"
		   " WHERE ExaCod=%ld",
		   Gbl.Hierarchy.Crs.CrsCod,
		   Exam->MaxGrade,
		   Exam->Visibility,
	           Exam->Title,
	           Txt,
	           Exam->ExaCod);
   Str_SetDecimalPointToLocal ();	// Return to local system
  }

/*****************************************************************************/
/***************************** Hide/unhide exam ******************************/
/*****************************************************************************/

void Exa_DB_HideUnhideExam (long ExaCod,bool Hide)
  {
   DB_QueryUPDATE ("can not hide exam",
		   "UPDATE exa_exams"
		     " SET Hidden='%c'"
		   " WHERE ExaCod=%ld",
		   Hide ? 'Y' :
			  'N',
		   ExaCod);
  }

/*****************************************************************************/
/*********************** Get list of all the exams *************************/
/*****************************************************************************/

unsigned Exa_DB_GetListExams (MYSQL_RES **mysql_res,Exa_Order_t SelectedOrder)
  {
   static const char *OrderBySubQuery[Exa_NUM_ORDERS] =
     {
      [Exa_ORDER_BY_START_DATE] = "StartTime DESC,EndTime DESC,exa_exams.Title DESC",
      [Exa_ORDER_BY_END_DATE  ] = "EndTime DESC,StartTime DESC,exa_exams.Title DESC",
      [Exa_ORDER_BY_TITLE     ] = "exa_exams.Title",
     };
   char *HiddenSubQuery;
   unsigned NumExams;

   /***** Subquery: get hidden exams depending on user's role *****/
   switch (Gbl.Usrs.Me.Role.Logged)
     {
      case Rol_STD:
         if (asprintf (&HiddenSubQuery," AND exa_exams.Hidden='N'") < 0)
	    Err_NotEnoughMemoryExit ();
	 break;
      case Rol_NET:
      case Rol_TCH:
      case Rol_DEG_ADM:
      case Rol_CTR_ADM:
      case Rol_INS_ADM:
      case Rol_SYS_ADM:
	 if (asprintf (&HiddenSubQuery,"%s","") < 0)
	    Err_NotEnoughMemoryExit ();
	 break;
      default:
	 Err_WrongRoleExit ();
	 break;
     }

   /***** Get list of exams from database *****/
   NumExams = (unsigned)
   DB_QuerySELECT (mysql_res,"can not get exams",
		   "SELECT exa_exams.ExaCod,"				// row[0]
			  "MIN(exa_sessions.StartTime) AS StartTime,"	// row[1]
			  "MAX(exa_sessions.EndTime) AS EndTime"	// row[2]
		    " FROM exa_exams"
		    " LEFT JOIN exa_sessions"
		      " ON exa_exams.ExaCod=exa_sessions.ExaCod"
		   " WHERE exa_exams.CrsCod=%ld"
		       "%s"
		   " GROUP BY exa_exams.ExaCod"
		   " ORDER BY %s",
		   Gbl.Hierarchy.Crs.CrsCod,
		   HiddenSubQuery,
		   OrderBySubQuery[SelectedOrder]);

   /***** Free allocated memory for subquery *****/
   free (HiddenSubQuery);

   return NumExams;
  }

/*****************************************************************************/
/********************** Get exam data using its code *************************/
/*****************************************************************************/

unsigned Exa_DB_GetDataOfExamByCod (MYSQL_RES **mysql_res,long ExaCod)
  {
   return (unsigned)
   DB_QuerySELECT (mysql_res,"can not get exam data",
		   "SELECT ExaCod,"	// row[0]
			  "CrsCod,"	// row[1]
			  "Hidden,"	// row[2]
			  "UsrCod,"	// row[3]
			  "MaxGrade,"	// row[4]
			  "Visibility,"	// row[5]
			  "Title"	// row[6]
		    " FROM exa_exams"
		   " WHERE ExaCod=%ld",
		   ExaCod);
  }

/*****************************************************************************/
/*********************** Get exam start and end times ************************/
/*****************************************************************************/

unsigned Exa_DB_GetExamStartEnd (MYSQL_RES **mysql_res,long ExaCod)
  {
   return (unsigned)
   DB_QuerySELECT (mysql_res,"can not get exam data",
		   "SELECT UNIX_TIMESTAMP(MIN(StartTime)),"	// row[0]
			  "UNIX_TIMESTAMP(MAX(EndTime))"		// row[1]
		    " FROM exa_sessions"
		   " WHERE ExaCod=%ld",
		   ExaCod);
  }

/*****************************************************************************/
/********************** Get exam text from database **************************/
/*****************************************************************************/

void Exa_DB_GetExamTxt (long ExaCod,char Txt[Cns_MAX_BYTES_TEXT + 1])
  {
   /***** Get text of exam from database *****/
   DB_QuerySELECTString (Txt,Cns_MAX_BYTES_TEXT,"can not get exam text",
		         "SELECT Txt"
			  " FROM exa_exams"
		         " WHERE ExaCod=%ld",
		         ExaCod);
  }

/*****************************************************************************/
/******************* Check if the title of an exam exists ********************/
/*****************************************************************************/

bool Exa_DB_CheckIfSimilarExamExists (long CrsCod,long ExaCod,const char *Title)
  {
   return (DB_QueryCOUNT ("can not get similar exams",
			  "SELECT COUNT(*)"
			   " FROM exa_exams"
			  " WHERE CrsCod=%ld"
			    " AND Title='%s'"
			    " AND ExaCod<>%ld",
			  CrsCod,
			  Title,
			  ExaCod) != 0);
  }

/*****************************************************************************/
/********************* Get number of courses with exams **********************/
/*****************************************************************************/
// Returns the number of courses with exams in this location

unsigned Exa_DB_GetNumCoursesWithExams (HieLvl_Level_t Scope)
  {
   /***** Get number of courses with exams from database *****/
   switch (Scope)
     {
      case HieLvl_SYS:
         return DB_QueryCOUNT ("can not get number of courses with exams",
			       "SELECT COUNT(DISTINCT CrsCod)"
				" FROM exa_exams");
      case HieLvl_CTY:
         return DB_QueryCOUNT ("can not get number of courses with exams",
			       "SELECT COUNT(DISTINCT exa_exams.CrsCod)"
				" FROM ins_instits,"
				      "ctr_centers,"
				      "deg_degrees,"
				      "crs_courses,"
				      "exa_exams"
			       " WHERE ins_instits.CtyCod=%ld"
				 " AND ins_instits.InsCod=ctr_centers.InsCod"
				 " AND ctr_centers.CtrCod=deg_degrees.CtrCod"
				 " AND deg_degrees.DegCod=crs_courses.DegCod"
				 " AND crs_courses.CrsCod=exa_exams.CrsCod",
			       Gbl.Hierarchy.Ins.InsCod);
      case HieLvl_INS:
         return DB_QueryCOUNT ("can not get number of courses with exams",
			       "SELECT COUNT(DISTINCT exa_exams.CrsCod)"
				" FROM ctr_centers,"
				      "deg_degrees,"
				      "crs_courses,"
				      "exa_exams"
			       " WHERE ctr_centers.InsCod=%ld"
				 " AND ctr_centers.CtrCod=deg_degrees.CtrCod"
				 " AND deg_degrees.DegCod=crs_courses.DegCod"
				 " AND crs_courses.CrsCod=exa_exams.CrsCod",
			       Gbl.Hierarchy.Ins.InsCod);
      case HieLvl_CTR:
         return DB_QueryCOUNT ("can not get number of courses with exams",
			       "SELECT COUNT(DISTINCT exa_exams.CrsCod)"
				" FROM deg_degrees,"
				      "crs_courses,"
				      "exa_exams"
			       " WHERE deg_degrees.CtrCod=%ld"
				 " AND deg_degrees.DegCod=crs_courses.DegCod"
				 " AND crs_courses.CrsCod=exa_exams.CrsCod",
			       Gbl.Hierarchy.Ctr.CtrCod);
      case HieLvl_DEG:
         return DB_QueryCOUNT ("can not get number of courses with exams",
			       "SELECT COUNT(DISTINCT exa_exams.CrsCod)"
				" FROM crs_courses,"
				      "exa_exams"
			       " WHERE crs_courses.DegCod=%ld"
				 " AND crs_courses.CrsCod=exa_exams.CrsCod",
			       Gbl.Hierarchy.Deg.DegCod);
      case HieLvl_CRS:
         return DB_QueryCOUNT ("can not get number of courses with exams",
			       "SELECT COUNT(DISTINCT CrsCod)"
				" FROM exa_exams"
			       " WHERE CrsCod=%ld",
			       Gbl.Hierarchy.Crs.CrsCod);
      default:
	 Err_WrongScopeExit ();
	 return 0;	// Not reached
     }
  }

/*****************************************************************************/
/**************************** Get number of exams ****************************/
/*****************************************************************************/
// Returns the number of exams in this location

unsigned Exa_DB_GetNumExams (HieLvl_Level_t Scope)
  {
   /***** Get number of exams from database *****/
   switch (Scope)
     {
      case HieLvl_SYS:
         return DB_QueryCOUNT ("can not get number of exams",
			       "SELECT COUNT(*)"
				" FROM exa_exams");
      case HieLvl_CTY:
         return DB_QueryCOUNT ("can not get number of exams",
			       "SELECT COUNT(*)"
				" FROM ins_instits,"
				      "ctr_centers,"
				      "deg_degrees,"
				      "crs_courses,"
				      "exa_exams"
			       " WHERE ins_instits.CtyCod=%ld"
				 " AND ins_instits.InsCod=ctr_centers.InsCod"
				 " AND ctr_centers.CtrCod=deg_degrees.CtrCod"
				 " AND deg_degrees.DegCod=crs_courses.DegCod"
				 " AND crs_courses.CrsCod=exa_exams.CrsCod",
			       Gbl.Hierarchy.Cty.CtyCod);
      case HieLvl_INS:
         return DB_QueryCOUNT ("can not get number of exams",
			       "SELECT COUNT(*)"
				" FROM ctr_centers,"
				      "deg_degrees,"
				      "crs_courses,"
				      "exa_exams"
			       " WHERE ctr_centers.InsCod=%ld"
				 " AND ctr_centers.CtrCod=deg_degrees.CtrCod"
				 " AND deg_degrees.DegCod=crs_courses.DegCod"
				 " AND crs_courses.CrsCod=exa_exams.CrsCod",
			       Gbl.Hierarchy.Ins.InsCod);
      case HieLvl_CTR:
         return DB_QueryCOUNT ("can not get number of exams",
			       "SELECT COUNT(*)"
				" FROM deg_degrees,"
				      "crs_courses,"
				      "exa_exams"
			       " WHERE deg_degrees.CtrCod=%ld"
				 " AND deg_degrees.DegCod=crs_courses.DegCod"
				 " AND crs_courses.CrsCod=exa_exams.CrsCod",
			       Gbl.Hierarchy.Ctr.CtrCod);
      case HieLvl_DEG:
         return DB_QueryCOUNT ("can not get number of exams",
			       "SELECT COUNT(*)"
				" FROM crs_courses,"
				      "exa_exams"
			       " WHERE crs_courses.DegCod=%ld"
				 " AND crs_courses.CrsCod=exa_exams.CrsCod",
			       Gbl.Hierarchy.Deg.DegCod);
      case HieLvl_CRS:
         return DB_QueryCOUNT ("can not get number of exams",
			       "SELECT COUNT(*)"
				" FROM exa_exams"
			       " WHERE CrsCod=%ld",
			       Gbl.Hierarchy.Crs.CrsCod);
      default:
	 Err_WrongScopeExit ();
	 return 0;	// Not reached
     }
  }

/*****************************************************************************/
/************* Get average number of questions per course exam ***************/
/*****************************************************************************/

double Exa_DB_GetNumQstsPerCrsExam (HieLvl_Level_t Scope)
  {
   /***** Get number of questions per exam from database *****/
   switch (Scope)
     {
      case HieLvl_SYS:
         return DB_QuerySELECTDouble ("can not get number of questions per exam",
				      "SELECT AVG(NumQsts)"
				       " FROM (SELECT COUNT(exa_set_questions.QstCod) AS NumQsts"
					       " FROM exa_exams,"
						     "exa_set_questions"
					      " WHERE exa_exams.ExaCod=exa_set_questions.ExaCod"
					      " GROUP BY exa_set_questions.ExaCod) AS NumQstsTable");
      case HieLvl_CTY:
         return DB_QuerySELECTDouble ("can not get number of questions per exam",
				      "SELECT AVG(NumQsts)"
				       " FROM (SELECT COUNT(exa_set_questions.QstCod) AS NumQsts"
					      "  FROM ins_instits,"
						     "ctr_centers,"
						     "deg_degrees,"
						     "crs_courses,"
						     "exa_exams,"
						     "exa_set_questions"
					       " WHERE ins_instits.CtyCod=%ld"
						 " AND ins_instits.InsCod=ctr_centers.InsCod"
						 " AND ctr_centers.CtrCod=deg_degrees.CtrCod"
						 " AND deg_degrees.DegCod=crs_courses.DegCod"
						 " AND crs_courses.CrsCod=exa_exams.CrsCod"
						 " AND exa_exams.ExaCod=exa_set_questions.ExaCod"
					       " GROUP BY exa_set_questions.ExaCod) AS NumQstsTable",
				      Gbl.Hierarchy.Cty.CtyCod);
      case HieLvl_INS:
         return DB_QuerySELECTDouble ("can not get number of questions per exam",
				      "SELECT AVG(NumQsts)"
				       " FROM (SELECT COUNT(exa_set_questions.QstCod) AS NumQsts"
					       " FROM ctr_centers,"
						     "deg_degrees,"
						     "crs_courses,"
						     "exa_exams,"
						     "exa_set_questions"
					      " WHERE ctr_centers.InsCod=%ld"
					        " AND ctr_centers.CtrCod=deg_degrees.CtrCod"
					        " AND deg_degrees.DegCod=crs_courses.DegCod"
					        " AND crs_courses.CrsCod=exa_exams.CrsCod"
					        " AND exa_exams.ExaCod=exa_set_questions.ExaCod"
					      " GROUP BY exa_set_questions.ExaCod) AS NumQstsTable",
				      Gbl.Hierarchy.Ins.InsCod);
      case HieLvl_CTR:
         return DB_QuerySELECTDouble ("can not get number of questions per exam",
				      "SELECT AVG(NumQsts)"
				       " FROM (SELECT COUNT(exa_set_questions.QstCod) AS NumQsts"
					       " FROM deg_degrees,"
						     "crs_courses,"
						     "exa_exams,"
						     "exa_set_questions"
					      " WHERE deg_degrees.CtrCod=%ld"
					        " AND deg_degrees.DegCod=crs_courses.DegCod"
					        " AND crs_courses.CrsCod=exa_exams.CrsCod"
					        " AND exa_exams.ExaCod=exa_set_questions.ExaCod"
					      " GROUP BY exa_set_questions.ExaCod) AS NumQstsTable",
				      Gbl.Hierarchy.Ctr.CtrCod);
      case HieLvl_DEG:
         return DB_QuerySELECTDouble ("can not get number of questions per exam",
				      "SELECT AVG(NumQsts)"
				       " FROM (SELECT COUNT(exa_set_questions.QstCod) AS NumQsts"
					       " FROM crs_courses,"
						     "exa_exams,"
						     "exa_set_questions"
					      " WHERE crs_courses.DegCod=%ld"
					        " AND crs_courses.CrsCod=exa_exams.CrsCod"
					        " AND exa_exams.ExaCod=exa_set_questions.ExaCod"
					      " GROUP BY exa_set_questions.ExaCod) AS NumQstsTable",
				      Gbl.Hierarchy.Deg.DegCod);
      case HieLvl_CRS:
         return DB_QuerySELECTDouble ("can not get number of questions per exam",
				      "SELECT AVG(NumQsts)"
				       " FROM (SELECT COUNT(exa_set_questions.QstCod) AS NumQsts"
					       " FROM exa_exams,"
						     "exa_set_questions"
					      " WHERE exa_exams.Cod=%ld"
					        " AND exa_exams.ExaCod=exa_set_questions.ExaCod"
					      " GROUP BY exa_set_questions.ExaCod) AS NumQstsTable",
				      Gbl.Hierarchy.Crs.CrsCod);
      default:
	 Err_WrongScopeExit ();
	 return 0.0;	// Not reached
     }
  }

/*****************************************************************************/
/********************************* Remove exam *******************************/
/*****************************************************************************/

void Exa_DB_RemoveExam (long ExaCod)
  {
   DB_QueryDELETE ("can not remove exam",
		   "DELETE FROM exa_exams"
		   " WHERE ExaCod=%ld",
		   ExaCod);
  }

/*****************************************************************************/
/********************** Remove all exams from a course ***********************/
/*****************************************************************************/

void Exa_DB_RemoveAllExamsFromCrs (long CrsCod)
  {
   DB_QueryDELETE ("can not remove course exams",
		   "DELETE FROM exa_exams"
		   " WHERE CrsCod=%ld",
                   CrsCod);
  }

/*****************************************************************************/
/********************** Create a new set of questions ************************/
/*****************************************************************************/

long Exa_DB_CreateSet (const struct ExaSet_Set *Set,unsigned SetInd)
  {
   return
   DB_QueryINSERTandReturnCode ("can not create new set of questions",
				"INSERT INTO exa_sets"
				" (ExaCod,SetInd,NumQstsToPrint,Title)"
				" VALUES"
				" (%ld,%u,%u,'%s')",
				Set->ExaCod,
				SetInd,
				Set->NumQstsToPrint,
				Set->Title);
  }

/*****************************************************************************/
/******************** Update an existing set of questions ********************/
/*****************************************************************************/

void Exa_DB_UpdateSet (const struct ExaSet_Set *Set)
  {
   DB_QueryUPDATE ("can not update set of questions",
		   "UPDATE exa_sets"
		     " SET ExaCod=%ld,"
		          "SetInd=%u,"
		          "NumQstsToPrint=%u,"
		          "Title='%s'"
		   " WHERE SetCod=%ld",
		   Set->ExaCod,
		   Set->SetInd,
		   Set->NumQstsToPrint,
	           Set->Title,
	           Set->SetCod);
  }

/*****************************************************************************/
/************************ Update set title in database ***********************/
/*****************************************************************************/

void Exa_DB_UpdateSetTitle (long SetCod,long ExaCod,
                            const char NewTitle[ExaSet_MAX_BYTES_TITLE + 1])
  {
   /***** Update set of questions changing old title by new title *****/
   DB_QueryUPDATE ("can not update the title of a set of questions",
		   "UPDATE exa_sets"
		     " SET Title='%s'"
		   " WHERE SetCod=%ld"
		     " AND ExaCod=%ld",	// Extra check
	           NewTitle,
	           SetCod,
	           ExaCod);
  }

/*****************************************************************************/
/****** Update number of questions to appear in exam print in database *******/
/*****************************************************************************/

void Exa_DB_UpdateNumQstsToExam (long SetCod,long ExaCod,unsigned NumQstsToPrint)
  {
   /***** Update set of questions changing old number by new number *****/
   DB_QueryUPDATE ("can not update the number of questions to appear in exam print",
		   "UPDATE exa_sets"
		     " SET NumQstsToPrint=%u"
		   " WHERE SetCod=%ld"
		     " AND ExaCod=%ld",	// Extra check
	           NumQstsToPrint,
	           SetCod,
	           ExaCod);
  }

/*****************************************************************************/
/************ Change indexes of sets greater than a given index **************/
/*****************************************************************************/

void Exa_DB_UpdateSetIndexesInExamGreaterThan (long ExaCod,long SetInd)
  {
   DB_QueryUPDATE ("can not update indexes of sets",
		   "UPDATE exa_sets"
		     " SET SetInd=SetInd-1"
		   " WHERE ExaCod=%ld"
		     " AND SetInd>%u",
		   ExaCod,
		   SetInd);
  }

/*****************************************************************************/
/********************* Change index of a set in an exam **********************/
/*****************************************************************************/

void Exa_DB_UpdateSetIndex (long SetInd,long SetCod,long ExaCod)
  {
   DB_QueryUPDATE ("can not exchange indexes of sets",
		   "UPDATE exa_sets"
		     " SET SetInd=%ld"
		   " WHERE SetCod=%ld"
		     " AND ExaCod=%ld",	// Extra check
		   SetInd,
		   SetCod,
		   ExaCod);
  }

/*****************************************************************************/
/************ Lock tables to make the exchange of sets atomic ****************/
/*****************************************************************************/

void Exa_DB_LockTables (void)
  {
   DB_Query ("can not lock tables to exchange sets of questions",
	     "LOCK TABLES exa_sets WRITE");
   Gbl.DB.LockedTables = true;
  }

/*****************************************************************************/
/********** Unlock tables to make the exchange of sets atomic ****************/
/*****************************************************************************/

void Exa_DB_UnlockTables (void)
  {
   Gbl.DB.LockedTables = false;	// Set to false before the following unlock...
				// ...to not retry the unlock if error in unlocking
   DB_Query ("can not unlock tables after exchanging sets of questions",
	     "UNLOCK TABLES");
  }

/*****************************************************************************/
/*********************** Get number of sets in an exam ***********************/
/*****************************************************************************/

unsigned Exa_DB_GetNumSetsExam (long ExaCod)
  {
   return (unsigned)
   DB_QueryCOUNT ("can not get number of sets in an exam",
		  "SELECT COUNT(*)"
		   " FROM exa_sets"
		  " WHERE ExaCod=%ld",
		  ExaCod);
  }

/*****************************************************************************/
/************* Get total number of questions to print in an exam *************/
/*****************************************************************************/

unsigned Exa_DB_GetNumQstsExam (long ExaCod)
  {
   return DB_QuerySELECTUnsigned ("can not get number of questions to print in an exam",
				  "SELECT SUM(NumQstsToPrint)"
				   " FROM exa_sets"
				  " WHERE ExaCod=%ld",
				  ExaCod);
  }

/*****************************************************************************/
/***************** Get sets of questions in a given exam *********************/
/*****************************************************************************/

unsigned Exa_DB_GetExamSets (MYSQL_RES **mysql_res,long ExaCod)
  {
   return (unsigned)
   DB_QuerySELECT (mysql_res,"can not get sets of questions",
		   "SELECT SetCod,"		// row[0]
			  "SetInd,"		// row[1]
			  "NumQstsToPrint,"	// row[2]
			  "Title"		// row[3]
		    " FROM exa_sets"
		   " WHERE ExaCod=%ld"
		   " ORDER BY SetInd",
		   ExaCod);
  }

/*****************************************************************************/
/*********************** Get set data using its code *************************/
/*****************************************************************************/

unsigned Exa_DB_GetDataOfSetByCod (MYSQL_RES **mysql_res,long SetCod)
  {
   return (unsigned)
   DB_QuerySELECT (mysql_res,"can not get set data",
		   "SELECT SetCod,"		// row[0]
			  "ExaCod,"		// row[1]
			  "SetInd,"		// row[2]
			  "NumQstsToPrint,"	// row[3]
			  "Title"		// row[4]
		    " FROM exa_sets"
		   " WHERE SetCod=%ld",
		   SetCod);
  }

/*****************************************************************************/
/************** Check if the title of a set of questions exists **************/
/*****************************************************************************/

bool Exa_DB_CheckIfSimilarSetExists (const struct ExaSet_Set *Set,
                                     const char Title[ExaSet_MAX_BYTES_TITLE + 1])
  {
   /***** Get number of set of questions with a field value from database *****/
   return (DB_QueryCOUNT ("can not get similar sets of questions",
			  "SELECT COUNT(*)"
			   " FROM exa_sets,"
			         "exa_exams"
			  " WHERE exa_sets.ExaCod=%ld"
			    " AND exa_sets.Title='%s'"
			    " AND exa_sets.SetCod<>%ld"
			    " AND exa_sets.ExaCod=exa_exams.ExaCod"
			    " AND exa_exams.CrsCod=%ld",	// Extra check
			  Set->ExaCod,Title,
			  Set->SetCod,
			  Gbl.Hierarchy.Crs.CrsCod) != 0);
  }

/*****************************************************************************/
/****************** Get set index given exam and set code ********************/
/*****************************************************************************/

unsigned Exa_DB_GetSetIndFromSetCod (long ExaCod,long SetCod)
  {
   return DB_QuerySELECTUnsigned ("can not get set index",
				  "SELECT SetInd"	// row[0]
				   " FROM exa_sets"
				  " WHERE SetCod=%u"
				    " AND ExaCod=%ld",	// Extra check
				  SetCod,ExaCod);
  }

/*****************************************************************************/
/****************** Get set code given exam and set index ********************/
/*****************************************************************************/

long Exa_DB_GetSetCodFromSetInd (long ExaCod,unsigned SetInd)
  {
   long SetCod;

   /***** Get set code from set index *****/
   SetCod = DB_QuerySELECTCode ("can not get set code",
				"SELECT SetCod"
				 " FROM exa_sets"
				" WHERE ExaCod=%ld"
				  " AND SetInd=%u",
				ExaCod,
				SetInd);
   if (SetCod <= 0)
      Err_WrongSetExit ();

   return SetCod;
  }

/*****************************************************************************/
/********************* Get maximum set index in an exam **********************/
/*****************************************************************************/
// Question index can be 1, 2, 3...
// Return 0 if no questions

unsigned Exa_DB_GetMaxSetIndexInExam (long ExaCod)
  {
   /***** Get maximum set index in an exam from database *****/
   return DB_QuerySELECTUnsigned ("can not get max set index",
				  "SELECT MAX(SetInd)"
				   " FROM exa_sets"
				  " WHERE ExaCod=%ld",
				  ExaCod);
  }

/*****************************************************************************/
/*********** Get previous set index to a given set index in an exam **********/
/*****************************************************************************/
// Input set index can be 1, 2, 3... n-1
// Return set index will be 1, 2, 3... n if previous set exists, or 0 if no previous set

unsigned Exa_DB_GetPrevSetIndexInExam (long ExaCod,unsigned SetInd)
  {
   /***** Get previous set index in an exam from database *****/
   // Although indexes are always continuous...
   // ...this implementation works even with non continuous indexes
   return DB_QuerySELECTUnsigned ("can not get previous set index",
				  "SELECT COALESCE(MAX(SetInd),0)"
				   " FROM exa_sets"
				  " WHERE ExaCod=%ld"
				    " AND SetInd<%u",
				  ExaCod,
				  SetInd);
  }

/*****************************************************************************/
/*************** Get next set index to a given index in an exam **************/
/*****************************************************************************/
// Input set index can be 0, 1, 2, 3... n-1
// Return set index will be 1, 2, 3... n if next set exists, or 0 if no next set

unsigned Exa_DB_GetNextSetIndexInExam (long ExaCod,unsigned SetInd)
  {
   /***** Get next set index in an exam from database *****/
   // Although indexes are always continuous...
   // ...this implementation works even with non continuous indexes
   return DB_QuerySELECTUnsigned ("can not get next set index",
				  "SELECT COALESCE(MIN(SetInd),0)"
				   " FROM exa_sets"
				  " WHERE ExaCod=%ld"
				    " AND SetInd>%u",
				  ExaCod,
				  SetInd);
  }

/*****************************************************************************/
/******************* Remove a set of questions from an exam ******************/
/*****************************************************************************/

void Exa_DB_RemoveSetFromExam (long SetCod,long ExaCod)
  {
   DB_QueryDELETE ("can not remove set",
		   "DELETE FROM exa_sets"
		   " WHERE SetCod=%ld"
                     " AND ExaCod=%ld",		// Extra check
		   SetCod,
		   ExaCod);
  }

/*****************************************************************************/
/***************** Remove the sets of questions from an course ***************/
/*****************************************************************************/

void Exa_DB_RemoveAllSetsFromExam (long ExaCod)
  {
   DB_QueryDELETE ("can not remove exam sets",
		   "DELETE FROM exa_sets"
		   " WHERE ExaCod=%ld",
		   ExaCod);
  }

/*****************************************************************************/
/************ Remove the sets of questions from exams in a course ************/
/*****************************************************************************/

void Exa_DB_RemoveAllSetsFromCrs (long CrsCod)
  {
   DB_QueryDELETE ("can not remove sets in course exams",
		   "DELETE FROM exa_sets"
		   " USING exa_exams,"
		          "exa_sets"
		   " WHERE exa_exams.CrsCod=%ld"
		     " AND exa_exams.ExaCod=exa_sets.ExaCod",
                   CrsCod);
  }

/*****************************************************************************/
/******************* Insert question in table of questions *******************/
/*****************************************************************************/

long Exa_DB_AddQuestionToSet (long SetCod,const struct Tst_Question *Question,long MedCod)
  {
   extern const char *Tst_StrAnswerTypesDB[Tst_NUM_ANS_TYPES];
   static char CharInvalid[Tst_NUM_VALIDITIES] =
     {
      [Tst_INVALID_QUESTION] = 'Y',
      [Tst_VALID_QUESTION  ] = 'N'
     };

   return
   DB_QueryINSERTandReturnCode ("can not add question to set",
				"INSERT INTO exa_set_questions"
				" (SetCod,Invalid,AnsType,Shuffle,"
				  "Stem,Feedback,MedCod)"
				" VALUES"
				" (%ld,'%c','%s','%c',"
				 "'%s','%s',%ld)",
				SetCod,
				CharInvalid[Question->Validity],
				Tst_StrAnswerTypesDB[Question->Answer.Type],
				Question->Answer.Shuffle ? 'Y' :
							   'N',
				Question->Stem,
				Question->Feedback,
				MedCod);
  }

/*****************************************************************************/
/*********************** Validate/invalidate a question **********************/
/*****************************************************************************/

void Exa_DB_ChangeValidityQst (long QstCod,long SetCod,long ExaCod,long CrsCod,
                               Tst_Validity_t Validity)
  {
   static char CharInvalid[Tst_NUM_VALIDITIES] =
     {
      [Tst_INVALID_QUESTION] = 'Y',
      [Tst_VALID_QUESTION  ] = 'N'
     };

   DB_QueryUPDATE ("can not validate question",
		   "UPDATE exa_set_questions,"
		          "exa_sets,"
		          "exa_exams"
		     " SET exa_set_questions.Invalid='%c'"
		   " WHERE exa_set_questions.QstCod=%ld"
		     " AND exa_set_questions.SetCod=%ld"	// Extra check
		     " AND exa_set_questions.SetCod=exa_sets.SetCod"
		     " AND exa_sets.ExaCod=%ld"			// Extra check
		     " AND exa_sets.ExaCod=exa_exams.ExaCod"
		     " AND exa_exams.CrsCod=%ld",		// Extra check
		   CharInvalid[Validity],
		   QstCod,
		   SetCod,
		   ExaCod,
		   CrsCod);
  }

/*****************************************************************************/
/********************* Get number of questions in a set **********************/
/*****************************************************************************/

unsigned Exa_DB_GetNumQstsInSet (long SetCod)
  {
   /***** Get number of questions in set from database *****/
   return (unsigned)
   DB_QueryCOUNT ("can not get number of questions in a set",
		  "SELECT COUNT(*)"
		   " FROM exa_set_questions"
		  " WHERE SetCod=%ld",
		  SetCod);
  }

/*****************************************************************************/
/*********************** Get all questions from a set ************************/
/*****************************************************************************/

unsigned Exa_DB_GetQstsFromSet (MYSQL_RES **mysql_res,long SetCod)
  {
   return (unsigned)
   DB_QuerySELECT (mysql_res,"can not get questions from set",
		   "SELECT QstCod"	// row[0]
		    " FROM exa_set_questions"
		   " WHERE SetCod=%ld"
		   " ORDER BY Stem",
		   SetCod);
  }

/*****************************************************************************/
/******************* Get some random questions from a set ********************/
/*****************************************************************************/

unsigned Exa_DB_GetSomeQstsFromSetToPrint (MYSQL_RES **mysql_res,
                                           long SetCod,unsigned NumQstsToPrint)
  {
   return (unsigned)
   DB_QuerySELECT (mysql_res,"can not get questions from set",
		   "SELECT QstCod,"	// row[0]
			  "AnsType,"	// row[1]
			  "Shuffle"	// row[2]
		    " FROM exa_set_questions"
		   " WHERE SetCod=%ld"
		   " ORDER BY RAND()"	// Don't use RAND(NOW()) because the same ordering will be repeated across sets
		   " LIMIT %u",
		   SetCod,
		   NumQstsToPrint);
  }

/*****************************************************************************/
/********************* Get data from a question in a set *********************/
/*****************************************************************************/

unsigned Exa_DB_GetQstDataByCod (MYSQL_RES **mysql_res,long QstCod)
  {
   return (unsigned)
   DB_QuerySELECT (mysql_res,"can not get a question",
		   "SELECT Invalid,"	// row[0]
			  "AnsType,"	// row[1]
			  "Shuffle,"	// row[2]
			  "Stem,"	// row[3]
			  "Feedback,"	// row[4]
			  "MedCod"	// row[5]
		    " FROM exa_set_questions"
		   " WHERE QstCod=%ld",
		   QstCod);
  }

/*****************************************************************************/
/********** Get validity and answer type from a question in a set ************/
/*****************************************************************************/

unsigned Exa_DB_GetValidityAndAnswerType (MYSQL_RES **mysql_res,long QstCod)
  {
   return (unsigned)
   DB_QuerySELECT (mysql_res,"can not get a question",
		   "SELECT Invalid,"	// row[0]
			  "AnsType"	// row[1]
		    " FROM exa_set_questions"
		   " WHERE QstCod=%ld",
		   QstCod);
  }

/*****************************************************************************/
/***************** Get answer type from a question in a set ******************/
/*****************************************************************************/

unsigned Exa_DB_GetAnswerType (MYSQL_RES **mysql_res,long QstCod)
  {
   return (unsigned)
   DB_QuerySELECT (mysql_res,"can not get a question",
		   "SELECT AnsType"	// row[0]
		    " FROM exa_set_questions"
		   " WHERE QstCod=%ld",
		   QstCod);
  }

/*****************************************************************************/
/************* Get media code associated to stem of set question *************/
/*****************************************************************************/

unsigned Exa_DB_GetMediaFromStemOfQst (MYSQL_RES **mysql_res,long QstCod,long SetCod)
  {
   return (unsigned)
   DB_QuerySELECT (mysql_res,"can not get media",
		   "SELECT MedCod"	// row[0]
		    " FROM exa_set_questions"
		   " WHERE QstCod=%ld"
		     " AND SetCod=%ld",	// Extra check
		   QstCod,
		   SetCod);
  }

/*****************************************************************************/
/********* Get media codes associated to stems of an exam questions **********/
/*****************************************************************************/

unsigned Exa_DB_GetAllMediaFomStemOfAllQstsFromExam (MYSQL_RES **mysql_res,long ExaCod)
  {
   return (unsigned)
   DB_QuerySELECT (mysql_res,"can not get media",
		   "SELECT exa_set_questions.MedCod"
		    " FROM exa_sets,"
			  "exa_set_questions"
		   " WHERE exa_sets.ExaCod=%ld"
		     " AND exa_sets.SetCod=exa_set_questions.SetCod",
		   ExaCod);
  }

/*****************************************************************************/
/*** Get media codes associated to stems of all exam questions in a course ***/
/*****************************************************************************/

unsigned Exa_DB_GetAllMediaFomStemOfAllQstsFromCrs (MYSQL_RES **mysql_res,long CrsCod)
  {
   return (unsigned)
   DB_QuerySELECT (mysql_res,"can not get media",
		   "SELECT exa_set_questions.MedCod"
		    " FROM exa_exams,"
			  "exa_sets,"
			  "exa_set_questions"
		   " WHERE exa_exams.CrsCod=%ld"
		     " AND exa_exams.ExaCod=exa_sets.ExaCod"
		     " AND exa_sets.SetCod=exa_set_questions.SetCod",
		   CrsCod);
  }

/*****************************************************************************/
/**************** Remove a question from a set of questions ******************/
/*****************************************************************************/

void Exa_DB_RemoveSetQuestion (long QstCod,long SetCod)
  {
   DB_QueryDELETE ("can not remove a question from a set",
		   "DELETE FROM exa_set_questions"
		   " WHERE QstCod=%ld"
		     " AND SetCod=%ld",	// Extra check
		   QstCod,
		   SetCod);
  }

/*****************************************************************************/
/*************** Remove the questions in a set of questions ******************/
/*****************************************************************************/

void Exa_DB_RemoveAllSetQuestionsFromSet (long SetCod,long ExaCod)
  {
   DB_QueryDELETE ("can not remove questions associated to set",
		   "DELETE FROM exa_set_questions"
		   " USING exa_set_questions,"
		          "exa_sets"
		   " WHERE exa_set_questions.SetCod=%ld"
                     " AND exa_set_questions.SetCod=exa_sets.SetCod"
		     " AND exa_sets.ExaCod=%ld",	// Extra check
		   SetCod,
		   ExaCod);
  }

/*****************************************************************************/
/********* Remove the questions in sets of questions from an exam ************/
/*****************************************************************************/

void Exa_DB_RemoveAllSetQuestionsFromExam (long ExaCod)
  {
   DB_QueryDELETE ("can not remove exam questions",
		   "DELETE FROM exa_set_questions"
		   " USING exa_sets,"
		          "exa_set_questions"
		   " WHERE exa_sets.ExaCod=%ld"
		     " AND exa_sets.SetCod=exa_set_questions.SetCod",
		   ExaCod);
  }

/*****************************************************************************/
/********* Remove the questions in sets of questions from a course ***********/
/*****************************************************************************/

void Exa_DB_RemoveAllSetQuestionsFromCrs (long CrsCod)
  {
   DB_QueryDELETE ("can not remove questions in course exams",
		   "DELETE FROM exa_set_questions"
		   " USING exa_exams,"
		          "exa_sets,"
		          "exa_set_questions"
		   " WHERE exa_exams.CrsCod=%ld"
		     " AND exa_exams.ExaCod=exa_sets.ExaCod",
		     " AND exa_sets.SetCod=exa_set_questions.SetCod",
                   CrsCod);
   }

/*****************************************************************************/
/********************* Add one answer to question in set *********************/
/*****************************************************************************/

void Exa_DB_AddAnsToQstInSet (long QstCod,unsigned AnsInd,
                              const char *Answer,const char *Feedback,
                              long MedCod,bool Correct)
  {
   DB_QueryINSERT ("can not add answer to set",
		   "INSERT INTO exa_set_answers"
		   " (QstCod,AnsInd,Answer,Feedback,MedCod,Correct)"
		   " VALUES"
		   " (%ld,%u,'%s','%s',%ld,'%c')",
		   QstCod,	// Question code in set
		   AnsInd,	// Answer index (number of option)
		   Answer,	// Copy of text
		   Feedback,	// Copy of feedback
		   MedCod,	// Media code of the new cloned media
		   Correct ? 'Y' :
			     'N');	// Copy of correct
  }

/*****************************************************************************/
/*************** Get answers of a test question from database ****************/
/*****************************************************************************/

unsigned Exa_DB_GetQstAnswersFromSet (MYSQL_RES **mysql_res,long QstCod,bool Shuffle)
  {
   unsigned NumOptions;

   /***** Get answers of a question from database *****/
   NumOptions = (unsigned)
   DB_QuerySELECT (mysql_res,"can not get answers of a question",
		   "SELECT AnsInd,"	// row[0]
			  "Answer,"	// row[1]
			  "Feedback,"	// row[2]
			  "MedCod,"	// row[3]
			  "Correct"	// row[4]
		    " FROM exa_set_answers"
		   " WHERE QstCod=%ld"
		   " ORDER BY %s",
		   QstCod,
		   Shuffle ? "RAND()" :
		             "AnsInd");

   if (!NumOptions)
      Ale_ShowAlert (Ale_ERROR,"Error when getting answers of a question.");

   return NumOptions;
  }

/*****************************************************************************/
/************** Get answers text for a question in an exam set ***************/
/*****************************************************************************/

unsigned Exa_DB_GetQstAnswersTextFromSet (MYSQL_RES **mysql_res,long QstCod)
  {
   return (unsigned)
   DB_QuerySELECT (mysql_res,"can not get text of answers of a question",
		   "SELECT Answer"		// row[0]
		    " FROM exa_set_answers"
		   " WHERE QstCod=%ld",
		   QstCod);
  }

/*****************************************************************************/
/********** Get answers correctness for a question in an exam set ************/
/*****************************************************************************/

unsigned Exa_DB_GetQstAnswersCorrFromSet (MYSQL_RES **mysql_res,long QstCod)
  {
   return (unsigned)
   DB_QuerySELECT (mysql_res,"can not get correctness of answers of a question",
		   "SELECT Correct"		// row[0]
		    " FROM exa_set_answers"
		   " WHERE QstCod=%ld"
		   " ORDER BY AnsInd",
		   QstCod);
  }

/*****************************************************************************/
/*********** Get media codes associated to answers of set question ***********/
/*****************************************************************************/

unsigned Exa_DB_GetMediaFromAllAnsOfQst (MYSQL_RES **mysql_res,long QstCod,long SetCod)
  {
   return (unsigned)
   DB_QuerySELECT (mysql_res,"can not get media",
		   "SELECT exa_set_answers.MedCod"	// row[0]
		    " FROM exa_set_answers,"
			  "exa_set_questions"
		   " WHERE exa_set_answers.QstCod=%ld"
		     " AND exa_set_answers.QstCod=exa_set_questions.QstCod"
		     " AND exa_set_questions.SetCod=%ld",	// Extra check
		   QstCod,
		   SetCod);
  }

/*****************************************************************************/
/********** Get media codes associated to answers of exam questions **********/
/*****************************************************************************/

unsigned Exa_DB_GetAllMediaFromAnsOfAllQstsFromExam (MYSQL_RES **mysql_res,long ExaCod)
  {
   return (unsigned)
   DB_QuerySELECT (mysql_res,"can not get media",
		   "SELECT exa_set_answers.MedCod"
		    " FROM exa_sets,"
			  "exa_set_questions,"
			  "exa_set_answers"
		   " WHERE exa_sets.ExaCod=%ld"
		     " AND exa_sets.SetCod=exa_set_questions.SetCod"
		     " AND exa_set_questions.QstCod=exa_set_answers.QstCod",
		   ExaCod);
  }

/*****************************************************************************/
/***** Get media codes associated to answers of exam questions ********/
/*****************************************************************************/

unsigned Exa_DB_GetAllMediaFromAnsOfAllQstsFromCrs (MYSQL_RES **mysql_res,long CrsCod)
  {
   return (unsigned)
   DB_QuerySELECT (mysql_res,"can not get media",
		   "SELECT exa_set_answers.MedCod"
		    " FROM exa_exams,"
			  "exa_sets,"
			  "exa_set_questions,"
			  "exa_set_answers"
		   " WHERE exa_exams.CrsCod=%ld"
		     " AND exa_exams.ExaCod=exa_sets.ExaCod"
		     " AND exa_sets.SetCod=exa_set_questions.SetCod"
		     " AND exa_set_questions.QstCod=exa_set_answers.QstCod",
		   CrsCod);
  }

/*****************************************************************************/
/********** Remove the answers in sets of questions from an exam *************/
/*****************************************************************************/

void Exa_DB_RemoveAllSetAnswersFromExam (long ExaCod)
  {
   DB_QueryDELETE ("can not remove exam answers",
		   "DELETE FROM exa_set_answers"
		   " USING exa_sets,"
		          "exa_set_questions,"
		          "exa_set_answers"
		   " WHERE exa_sets.ExaCod=%ld"
		     " AND exa_sets.SetCod=exa_set_questions.SetCod",
		     " AND exa_set_questions.QstCod=exa_set_answers.QstCod",
		   ExaCod);
  }

/*****************************************************************************/
/********** Remove the answers in sets of questions from a course ************/
/*****************************************************************************/

void Exa_DB_RemoveAllSetAnswersFromCrs (long CrsCod)
  {
   DB_QueryDELETE ("can not remove answers in course exams",
		   "DELETE FROM exa_set_answers"
		   " USING exa_exams,"
		          "exa_sets,"
		          "exa_set_questions,"
		          "exa_set_answers"
		   " WHERE exa_exams.CrsCod=%ld"
		     " AND exa_exams.ExaCod=exa_sets.ExaCod",
		     " AND exa_sets.SetCod=exa_set_questions.SetCod"
		     " AND exa_set_questions.QstCod=exa_set_answers.QstCod",
                   CrsCod);
  }

/*****************************************************************************/
/************************* Create a new exam session *************************/
/*****************************************************************************/

long Exa_DB_CreateSession (const struct ExaSes_Session *Session)
  {
   return
   DB_QueryINSERTandReturnCode ("can not create exam session",
				"INSERT exa_sessions"
				" (ExaCod,"
				  "Hidden,"
				  "UsrCod,"
				  "StartTime,"
				  "EndTime,"
				  "Title,"
				  "ShowUsrResults)"
				" VALUES"
				" (%ld,"		// ExaCod
                                 "'%c',"		// Hidden
				 "%ld,"			// UsrCod
                                 "FROM_UNIXTIME(%ld),"	// Start time
                                 "FROM_UNIXTIME(%ld),"	// End time
				 "'%s',"		// Title
				 "'N')",		// ShowUsrResults: Don't show user results initially
				Session->ExaCod,
				Session->Hidden ? 'Y' :
					          'N',
				Gbl.Usrs.Me.UsrDat.UsrCod,		// Session creator
				Session->TimeUTC[Dat_START_TIME],	// Start time
				Session->TimeUTC[Dat_END_TIME  ],	// End time
				Session->Title);
  }

/*****************************************************************************/
/********************** Update an existing exam session **********************/
/*****************************************************************************/

void Exa_DB_UpdateSession (const struct ExaSes_Session *Session)
  {
   /***** Insert this new exam session into database *****/
   DB_QueryUPDATE ("can not update exam session",
		   "UPDATE exa_sessions,"
		          "exa_exams"
		     " SET exa_sessions.Hidden='%c',"
		          "exa_sessions.StartTime=FROM_UNIXTIME(%ld),"
                          "exa_sessions.EndTime=FROM_UNIXTIME(%ld),"
                          "exa_sessions.Title='%s',"
                          "exa_sessions.ShowUsrResults='%c'"
		   " WHERE exa_sessions.SesCod=%ld"
		     " AND exa_sessions.ExaCod=%ld"	// Extra check
		     " AND exa_sessions.ExaCod=exa_exams.ExaCod"
		     " AND exa_exams.CrsCod=%ld",	// Extra check
		   Session->Hidden ? 'Y' :
			             'N',
	           Session->TimeUTC[Dat_START_TIME],	// Start time
		   Session->TimeUTC[Dat_END_TIME  ],	// End time
		   Session->Title,
		   Session->ShowUsrResults ? 'Y' :
			                     'N',
		   Session->SesCod,
		   Session->ExaCod,
		   Gbl.Hierarchy.Crs.CrsCod);
  }

/*****************************************************************************/
/********************* Get number of sessions in an exam *********************/
/*****************************************************************************/

unsigned Exa_DB_GetNumSessionsInExam (long ExaCod)
  {
   /***** Trivial check *****/
   if (ExaCod < 0)	// A non-existing exam...
      return 0;		// ...has no sessions

   /***** Get number of sessions in an exam from database *****/
   return (unsigned)
   DB_QueryCOUNT ("can not get number of sessions of an exam",
		  "SELECT COUNT(*)"
		   " FROM exa_sessions"
		  " WHERE ExaCod=%ld",
		  ExaCod);
  }

/*****************************************************************************/
/***************** Get number of open sessions in an exam ********************/
/*****************************************************************************/

unsigned Exa_DB_GetNumOpenSessionsInExam (long ExaCod)
  {
   /***** Trivial check *****/
   if (ExaCod < 0)	// A non-existing exam...
      return 0;		// ...has no sessions

   /***** Get number of open sessions in an exam from database *****/
   return (unsigned)
   DB_QueryCOUNT ("can not get number of open sessions of an exam",
		  "SELECT COUNT(*)"
		   " FROM exa_sessions"
		  " WHERE ExaCod=%ld"
		    " AND NOW() BETWEEN StartTime AND EndTime",
		  ExaCod);
  }

/*****************************************************************************/
/************************ List the sessions of an exam ***********************/
/*****************************************************************************/

unsigned Exa_DB_GetSessions (MYSQL_RES **mysql_res,long ExaCod)
  {
   char *HiddenSubQuery;
   char *GroupsSubQuery;
   unsigned NumSessions;

   /***** Subquery: get hidden sessions depending on user's role *****/
   switch (Gbl.Usrs.Me.Role.Logged)
     {
      case Rol_STD:
         if (asprintf (&HiddenSubQuery," AND Hidden='N'") < 0)
	    Err_NotEnoughMemoryExit ();
	 break;
      case Rol_NET:
      case Rol_TCH:
      case Rol_DEG_ADM:
      case Rol_CTR_ADM:
      case Rol_INS_ADM:
      case Rol_SYS_ADM:
	 if (asprintf (&HiddenSubQuery,"%s","") < 0)
	    Err_NotEnoughMemoryExit ();
	 break;
      default:
	 Err_WrongRoleExit ();
	 break;
     }

   /***** Subquery: get sessions depending on groups *****/
   if (Gbl.Crs.Grps.WhichGrps == Grp_MY_GROUPS)
     {
      if (asprintf (&GroupsSubQuery," AND"
				    "(SesCod NOT IN"
				    " (SELECT SesCod FROM exa_groups)"
				    " OR"
				    " SesCod IN"
				    " (SELECT exa_groups.SesCod"
				       " FROM exa_groups,"
				             "grp_users"
				      " WHERE grp_users.UsrCod=%ld"
				        " AND exa_groups.GrpCod=grp_users.GrpCod))",
		     Gbl.Usrs.Me.UsrDat.UsrCod) < 0)
	  Err_NotEnoughMemoryExit ();
      }
    else	// Gbl.Crs.Grps.WhichGrps == Grp_ALL_GROUPS
       if (asprintf (&GroupsSubQuery,"%s","") < 0)
	  Err_NotEnoughMemoryExit ();

   /***** Get data of sessions from database *****/
   NumSessions = (unsigned)
   DB_QuerySELECT (mysql_res,"can not get sessions",
		   "SELECT SesCod,"					// row[0]
			  "ExaCod,"					// row[1]
			  "Hidden,"					// row[2]
			  "UsrCod,"					// row[3]
			  "UNIX_TIMESTAMP(StartTime),"			// row[4]
			  "UNIX_TIMESTAMP(EndTime),"			// row[5]
			  "NOW() BETWEEN StartTime AND EndTime,"	// row[6]
			  "Title,"					// row[7]
			  "ShowUsrResults"				// row[8]
		    " FROM exa_sessions"
		   " WHERE ExaCod=%ld%s%s"
		   " ORDER BY SesCod",
		   ExaCod,
		   HiddenSubQuery,
		   GroupsSubQuery);

   /***** Free allocated memory for subqueries *****/
   free (GroupsSubQuery);
   free (HiddenSubQuery);

   return NumSessions;
  }

/*****************************************************************************/
/******************* Get exam session data using its code ********************/
/*****************************************************************************/

unsigned Exa_DB_GetDataOfSessionByCod (MYSQL_RES **mysql_res,long SesCod)
  {
   return (unsigned)
   DB_QuerySELECT (mysql_res,"can not get sessions",
		   "SELECT SesCod,"					// row[0]
			  "ExaCod,"					// row[1]
			  "Hidden,"					// row[2]
			  "UsrCod,"					// row[3]
			  "UNIX_TIMESTAMP(StartTime),"			// row[4]
			  "UNIX_TIMESTAMP(EndTime),"			// row[5]
			  "NOW() BETWEEN StartTime AND EndTime,"	// row[6]
			  "Title,"					// row[7]
			  "ShowUsrResults"				// row[8]
		    " FROM exa_sessions"
		   " WHERE SesCod=%ld"
		     " AND ExaCod IN"		// Extra check
		         " (SELECT ExaCod"
			    " FROM exa_exams"
			   " WHERE CrsCod='%ld')",
		   SesCod,
		   Gbl.Hierarchy.Crs.CrsCod);
  }

/*****************************************************************************/
/***************** Toggle visibility of exam session results *****************/
/*****************************************************************************/

void Exa_DB_ToggleVisResultsSesUsr (const struct ExaSes_Session *Session)
  {
   /***** Toggle visibility of exam session results *****/
   DB_QueryUPDATE ("can not toggle visibility of session results",
		   "UPDATE exa_sessions,"
		          "exa_exams"
		     " SET exa_sessions.ShowUsrResults='%c'"
		   " WHERE exa_sessions.SesCod=%ld"
		     " AND exa_sessions.ExaCod=%ld"	// Extra check
		     " AND exa_sessions.ExaCod=exa_exams.ExaCod"
		     " AND exa_exams.CrsCod=%ld",	// Extra check
		   Session->ShowUsrResults ? 'Y' :
			                     'N',
		   Session->SesCod,
		   Session->ExaCod,
		   Gbl.Hierarchy.Crs.CrsCod);
  }

/*****************************************************************************/
/******************************** Hide a session *****************************/
/*****************************************************************************/

void Exa_DB_HideUnhideSession (const struct ExaSes_Session *Session,bool Hide)
  {
   DB_QueryUPDATE ("can not hide exam sessions",
		   "UPDATE exa_sessions,"
		          "exa_exams"
		     " SET exa_sessions.Hidden='Y'"
		   " WHERE exa_sessions.SesCod=%ld"
		     " AND exa_sessions.ExaCod=%ld"	// Extra check
		     " AND exa_sessions.ExaCod=exa_exams.ExaCod"
		     " AND exa_exams.CrsCod=%ld",	// Extra check
		   Hide ? 'Y' :
			  'N',
		   Session->SesCod,
		   Session->ExaCod,
		   Gbl.Hierarchy.Crs.CrsCod);
  }

/*****************************************************************************/
/******************* Remove exam session from all tables *********************/
/*****************************************************************************/

void Exa_DB_RemoveSessionFromAllTables (long SesCod)
  {
   /***** Remove groups associated to this exam session *****/
   DB_QueryDELETE ("can not remove groups associated to exam session",
		   "DELETE FROM exa_groups"
		   " WHERE SesCod=%ld",
		   SesCod);

   /***** Remove exam session from main table *****/
   DB_QueryDELETE ("can not remove exam session",
		   "DELETE FROM exa_sessions"
		   " WHERE SesCod=%ld",
		   SesCod);
  }

/*****************************************************************************/
/*********************** Remove exam sessions from exam **********************/
/*****************************************************************************/

void Exa_DB_RemoveAllSessionsFromExam (long ExaCod)
  {
   DB_QueryDELETE ("can not remove sessions of an exam",
		   "DELETE FROM exa_sessions"
		   " WHERE ExaCod=%ld",
		   ExaCod);
  }

/*****************************************************************************/
/********************** Remove exam sessions from course *********************/
/*****************************************************************************/

void Exa_DB_RemoveAllSessionsFromCrs (long CrsCod)
  {
   DB_QueryDELETE ("can not remove sessions of a course",
		   "DELETE FROM exa_sessions"
		   " USING exa_exams,"
		          "exa_sessions"
		   " WHERE exa_exams.CrsCod=%ld"
		     " AND exa_exams.ExaCod=exa_sessions.ExaCod",
		   CrsCod);
  }

/*****************************************************************************/
/************* Remove user from secondary exam session tables ****************/
/*****************************************************************************/

void Exa_DB_RemoveUsrFromSessionTablesInCrs (long UsrCod,long CrsCod)
  {
   /***** Remove student from secondary tables *****/
   Exa_DB_RemoveUsrSesResultsInCrs (UsrCod,CrsCod,"exa_prints");
  }

static void Exa_DB_RemoveUsrSesResultsInCrs (long UsrCod,long CrsCod,const char *TableName)
  {
   /***** Remove sessions in course from secondary table *****/
   DB_QueryDELETE ("can not remove sessions of a user from table",
		   "DELETE FROM %s"
		   " USING exa_exams,"
		          "exa_sessions,"
		          "%s"
		   " WHERE exa_exams.CrsCod=%ld"
		     " AND exa_exams.ExaCod=exa_sessions.ExaCod"
		     " AND exa_sessions.SesCod=%s.SesCod"
		     " AND %s.UsrCod=%ld",
		   TableName,
		   TableName,
		   CrsCod,
		   TableName,
		   TableName,
		   UsrCod);
  }

/*****************************************************************************/
/**************** Create group associated to an exam session *****************/
/*****************************************************************************/

void Exa_DB_CreateGrpAssociatedToSes (long SesCod,long GrpCod)
  {
   DB_QueryINSERT ("can not associate a group to an exam session",
		   "INSERT INTO exa_groups"
		   " (SesCod,GrpCod)"
		   " VALUES"
		   " (%ld,%ld)",
		   SesCod,
		   GrpCod);
  }

/*****************************************************************************/
/*********** Get groups associated to an exam session from database **********/
/*****************************************************************************/

unsigned Exa_DB_GetGrpsAssociatedToSes (MYSQL_RES **mysql_res,long SesCod)
  {
   return (unsigned)
   DB_QuerySELECT (mysql_res,"can not get groups of an exam session",
		   "SELECT grp_types.GrpTypName,"	// row[0]
			  "grp_groups.GrpName"		// row[1]
		    " FROM exa_groups,"
			  "grp_groups,"
			  "grp_types"
		   " WHERE exa_groups.SesCod=%ld"
		     " AND exa_groups.GrpCod=grp_groups.GrpCod"
		     " AND grp_groups.GrpTypCod=grp_types.GrpTypCod"
		   " ORDER BY grp_types.GrpTypName,"
			     "grp_groups.GrpName",
		   SesCod);
  }

/*****************************************************************************/
/*** Check if I belong to any of the groups associated to the exam session ***/
/*****************************************************************************/

bool Exa_DB_CheckIfICanListThisSessionBasedOnGrps (long SesCod)
  {
   return (DB_QueryCOUNT ("can not check if I can play an exam session",
			  "SELECT COUNT(*)"
			   " FROM exa_sessions"
			  " WHERE SesCod=%ld"
			  " AND (SesCod NOT IN"
			       " (SELECT SesCod FROM exa_groups)"
			       " OR"
			       " SesCod IN"
			       " (SELECT exa_groups.SesCod"
				  " FROM exa_groups,"
					"grp_users"
				 " WHERE grp_users.UsrCod=%ld"
				   " AND grp_users.GrpCod=exa_groups.GrpCod))",
			  SesCod,
			  Gbl.Usrs.Me.UsrDat.UsrCod) != 0);
  }

/*****************************************************************************/
/******************** Remove all groups from one session *********************/
/*****************************************************************************/

void Exa_DB_RemoveAllGrpsFromSes (long SesCod)
  {
   DB_QueryDELETE ("can not remove groups associated to a session",
		   "DELETE FROM exa_groups"
		   " WHERE SesCod=%ld",
		   SesCod);
  }

/*****************************************************************************/
/********* Remove groups associated to exam sessions of a given exam *********/
/*****************************************************************************/

void Exa_DB_RemoveAllGrpsFromExa (long ExaCod)
  {
   DB_QueryDELETE ("can not remove groups associated to sessions of an exam",
		   "DELETE FROM exa_groups"
		   " USING exa_sessions,"
		          "exa_groups"
		   " WHERE exa_sessions.ExaCod=%ld"
		     " AND exa_sessions.SesCod=exa_groups.SesCod",
		   ExaCod);
  }

/*****************************************************************************/
/******** Remove groups associated to exam sessions of a given course ********/
/*****************************************************************************/

void Exa_DB_RemoveAllGrpsFromCrs (long CrsCod)
  {
   DB_QueryDELETE ("can not remove sessions of a course",
		   "DELETE FROM exa_groups"
		   " USING exa_exams,"
		          "exa_sessions,"
		          "exa_groups"
		   " WHERE exa_exams.CrsCod=%ld"
		     " AND exa_exams.ExaCod=exa_sessions.ExaCod"
		     " AND exa_sessions.SesCod=exa_groups.SesCod",
		   CrsCod);
  }

/*****************************************************************************/
/**************** Remove groups of one type from all sessions ****************/
/*****************************************************************************/

void Exa_DB_RemoveAllGrpsOfType (long GrpTypCod)
  {
   DB_QueryDELETE ("can not remove groups of a type"
	           " from the associations between sessions and groups",
		   "DELETE FROM exa_groups"
		   " USING grp_groups,"
		          "exa_groups"
		   " WHERE grp_groups.GrpTypCod=%ld"
		     " AND grp_groups.GrpCod=exa_groups.GrpCod",
                   GrpTypCod);
  }

/*****************************************************************************/
/******************** Remove one group from all sessions *********************/
/*****************************************************************************/

void Exa_DB_RemoveGrpAssociatedToExamSess (long GrpCod)
  {
   /***** Remove group from all the sessions *****/
   DB_QueryDELETE ("can not remove group"
	           " from the associations between sessions and groups",
		   "DELETE FROM exa_groups"
		   " WHERE GrpCod=%ld",
		   GrpCod);
  }

/*****************************************************************************/
/***************** Create new blank exam print in database *******************/
/*****************************************************************************/

long Exa_DB_CreatePrint (const struct ExaPrn_Print *Print)
  {
   return
   DB_QueryINSERTandReturnCode ("can not create new exam print",
				"INSERT INTO exa_prints"
				" (SesCod,UsrCod,StartTime,EndTime,"
				  "NumQsts,NumQstsNotBlank,Sent,Score)"
				" VALUES"
				" (%ld,%ld,NOW(),NOW(),"
				  "%u,0,'N',0)",
				Print->SesCod,
				Print->UsrCod,
				Print->NumQsts.All);
  }

/*****************************************************************************/
/********************** Update exam print in database ************************/
/*****************************************************************************/

void Exa_DB_UpdatePrint (const struct ExaPrn_Print *Print)
  {
   /***** Update exam print in database *****/
   Str_SetDecimalPointToUS ();		// To print the floating point as a dot
   DB_QueryUPDATE ("can not update exam print",
		   "UPDATE exa_prints"
	           " SET EndTime=NOW(),"
	                "NumQstsNotBlank=%u,"
		        "Sent='%c',"
	                "Score='%.15lg'"
	           " WHERE PrnCod=%ld"
	             " AND SesCod=%ld"
	             " AND UsrCod=%ld",	// Extra checks
		   Print->NumQsts.NotBlank,
		   Print->Sent ? 'Y' :
			         'N',
		   Print->Score,
		   Print->PrnCod,
		   Print->SesCod,
		   Gbl.Usrs.Me.UsrDat.UsrCod);
   Str_SetDecimalPointToLocal ();	// Return to local system
  }

/*****************************************************************************/
/**************** Get data of an exam print using print code *****************/
/*****************************************************************************/

unsigned Exa_DB_GetDataOfPrintByPrnCod (MYSQL_RES **mysql_res,long PrnCod)
  {
   return (unsigned)
   DB_QuerySELECT (mysql_res,"can not get data of an exam print",
		   "SELECT PrnCod,"			// row[0]
			  "SesCod,"			// row[1]
			  "UsrCod,"			// row[2]
			  "UNIX_TIMESTAMP(StartTime),"	// row[3]
			  "UNIX_TIMESTAMP(EndTime),"	// row[4]
			  "NumQsts,"			// row[5]
			  "NumQstsNotBlank,"		// row[6]
			  "Sent,"			// row[7]
			  "Score"			// row[8]
		    " FROM exa_prints"
		   " WHERE PrnCod=%ld",
		   PrnCod);
  }

/*****************************************************************************/
/******** Get data of an exam print using session code and user code *********/
/*****************************************************************************/

unsigned Exa_DB_GetDataOfPrintBySesCodAndUsrCod (MYSQL_RES **mysql_res,
                                                 long SesCod,long UsrCod)
  {
   return (unsigned)
   DB_QuerySELECT (mysql_res,"can not get data of an exam print",
		   "SELECT PrnCod,"			// row[0]
			  "SesCod,"			// row[1]
			  "UsrCod,"			// row[2]
			  "UNIX_TIMESTAMP(StartTime),"	// row[3]
			  "UNIX_TIMESTAMP(EndTime),"	// row[4]
			  "NumQsts,"			// row[5]
			  "NumQstsNotBlank,"		// row[6]
			  "Sent,"			// row[7]
			  "Score"			// row[8]
		    " FROM exa_prints"
		   " WHERE SesCod=%ld"
		     " AND UsrCod=%ld",
		   SesCod,
		   UsrCod);
  }

/*****************************************************************************/
/******************* Remove exam prints for a given user *********************/
/*****************************************************************************/

void Exa_DB_RemoveAllPrintsMadeByUsrInAllCrss (long UsrCod)
  {
   DB_QueryDELETE ("can not remove exam prints made by a user",
		   "DELETE FROM exa_prints"
	           " WHERE UsrCod=%ld",
		   UsrCod);
  }

/*****************************************************************************/
/*************** Remove exam prints made by a user in a course ***************/
/*****************************************************************************/

void Exa_DB_RemoveAllPrintsMadeByUsrInCrs (long UsrCod,long CrsCod)
  {
   DB_QueryDELETE ("can not remove exams prints made by a user in a course",
		   "DELETE FROM exa_prints"
	           " USING exa_exams,"
	                  "exa_sessions,"
	                  "exa_prints"
                   " WHERE exa_exams.CrsCod=%ld"
                     " AND exa_exams.ExaCod=exa_sessions.ExaCod"
                     " AND exa_sessions.SesCod=exa_prints.SesCod"
                     " AND exa_prints.UsrCod=%ld",
		   CrsCod,
		   UsrCod);
  }

/*****************************************************************************/
/************ Remove exams prints made in the given exam session *************/
/*****************************************************************************/

void Exa_DB_RemoveAllPrintsFromSes (long SesCod)
  {
   DB_QueryDELETE ("can not remove exam prints in exam session",
		   "DELETE FROM exa_prints"
                   " WHERE exa_prints.SesCod=%ld",
		   SesCod);
  }

/*****************************************************************************/
/**************** Remove exams prints made in the given exam *****************/
/*****************************************************************************/

void Exa_DB_RemoveAllPrintsFromExa (long ExaCod)
  {
   DB_QueryDELETE ("can not remove exams prints in a course",
		   "DELETE FROM exa_prints"
	           " USING exa_sessions,"
	                  "exa_prints"
                   " WHERE exa_sessions.ExaCod=%ld"
                     " AND exa_sessions.SesCod=exa_prints.SesCod",
		   ExaCod);
  }

/*****************************************************************************/
/*************** Remove exams prints made in the given course ****************/
/*****************************************************************************/

void Exa_DB_RemoveAllPrintsFromCrs (long CrsCod)
  {
   DB_QueryDELETE ("can not remove exams prints in a course",
		   "DELETE FROM exa_prints"
	           " USING exa_exams,"
	                  "exa_sessions,"
	                  "exa_prints"
                   " WHERE exa_exams.CrsCod=%ld"
                     " AND exa_exams.ExaCod=exa_sessions.ExaCod"
                     " AND exa_sessions.SesCod=exa_prints.SesCod",
		   CrsCod);
  }

/*****************************************************************************/
/************* Store user's answers of an test exam into database ************/
/*****************************************************************************/

void Exa_DB_StoreOneQstOfPrint (const struct ExaPrn_Print *Print,
                                unsigned QstInd)
  {
   Str_SetDecimalPointToUS ();	// To print the floating point as a dot
   DB_QueryREPLACE ("can not update a question in an exam print",
		    "REPLACE INTO exa_print_questions"
		    " (PrnCod,QstCod,QstInd,SetCod,Score,Indexes,Answers)"
		    " VALUES"
		    " (%ld,%ld,%u,%ld,'%.15lg','%s','%s')",
		    Print->PrnCod,
		    Print->PrintedQuestions[QstInd].QstCod,
		    QstInd,	// 0, 1, 2, 3...
		    Print->PrintedQuestions[QstInd].SetCod,
		    Print->PrintedQuestions[QstInd].Score,
		    Print->PrintedQuestions[QstInd].StrIndexes,
		    Print->PrintedQuestions[QstInd].StrAnswers);
   Str_SetDecimalPointToLocal ();	// Return to local system
  }

/*****************************************************************************/
/************* Get the questions of an exam print from database **************/
/*****************************************************************************/

unsigned Exa_DB_GetPrintQuestions (MYSQL_RES **mysql_res,long PrnCod)
  {
   return (unsigned)
   DB_QuerySELECT (mysql_res,"can not get questions of an exam print",
		   "SELECT QstCod,"	// row[0]
			  "SetCod,"	// row[1]
			  "Score,"	// row[2]
			  "Indexes,"	// row[3]
			  "Answers"	// row[4]
		    " FROM exa_print_questions"
		   " WHERE PrnCod=%ld"
		   " ORDER BY QstInd",
		   PrnCod);
  }

/*****************************************************************************/
/************** Get the answers of an exam print from database ***************/
/*****************************************************************************/

void Exa_DB_GetAnswersFromQstInPrint (long PrnCod,long QstCod,
                                      char StrAnswers[Tst_MAX_BYTES_ANSWERS_ONE_QST + 1])
  {
   DB_QuerySELECTString (StrAnswers,Tst_MAX_BYTES_ANSWERS_ONE_QST,
                         "can not get answer in an exam print",
			 "SELECT Answers"
			  " FROM exa_print_questions"
			 " WHERE PrnCod=%ld"
			   " AND QstCod=%ld",
			 PrnCod,QstCod);
  }

/*****************************************************************************/
/************ Get number of questions not blank in an exam print *************/
/*****************************************************************************/

unsigned Exa_DB_GetNumQstsNotBlankInPrint (long PrnCod)
  {
   return (unsigned)
   DB_QueryCOUNT ("can not get number of questions not blank",
		  "SELECT COUNT(*)"
		   " FROM exa_print_questions"
		  " WHERE PrnCod=%ld"
		    " AND Answers<>''",
		  PrnCod);
  }

/*****************************************************************************/
/************* Compute total score of questions of an exam print *************/
/*****************************************************************************/

double Exa_DB_ComputeTotalScoreOfPrint (long PrnCod)
  {
   return DB_QuerySELECTDouble ("can not get score of exam print",
				"SELECT SUM(Score)"
				 " FROM exa_print_questions"
				" WHERE PrnCod=%ld",
				PrnCod);
  }

/*****************************************************************************/
/*************** Remove exam prints questions for a given user ***************/
/*****************************************************************************/

void Exa_DB_RemovePrintQstsMadeByUsrInAllCrss (long UsrCod)
  {
   DB_QueryDELETE ("can not remove exam prints made by a user",
		   "DELETE FROM exa_print_questions"
	           " USING exa_prints,"
	                  "exa_print_questions"
                   " WHERE exa_prints.UsrCod=%ld"
                     " AND exa_prints.PrnCod=exa_print_questions.PrnCod",
		   UsrCod);
  }

/*****************************************************************************/
/* Remove questions of exams prints made by the given user in a given course */
/*****************************************************************************/

void Exa_DB_RemovePrintQstsMadeByUsrInCrs (long UsrCod,long CrsCod)
  {
   DB_QueryDELETE ("can not remove exams prints made by a user in a course",
		   "DELETE FROM exa_print_questions"
	           " USING exa_exams,"
	                  "exa_sessions,"
	                  "exa_prints,"
	                  "exa_print_questions"
                   " WHERE exa_exams.CrsCod=%ld"
                     " AND exa_exams.ExaCod=exa_sessions.ExaCod"
                     " AND exa_sessions.SesCod=exa_prints.SesCod"
                     " AND exa_prints.UsrCod=%ld"
                     " AND exa_prints.PrnCod=exa_print_questions.PrnCod",
		   CrsCod,
		   UsrCod);
  }

/*****************************************************************************/
/************ Remove questions of exams prints in a given session ************/
/*****************************************************************************/

void Exa_DB_RemovePrintQstsFromSes (long SesCod)
  {
   DB_QueryDELETE ("can not remove exam print questions in exam session",
		   "DELETE FROM exa_print_questions"
		   " USING exa_prints,"
		          "exa_print_questions"
                   " WHERE exa_prints.SesCod=%ld"
                   " AND exa_prints.PrnCod=exa_print_questions.PrnCod",
		   SesCod);
  }

/*****************************************************************************/
/************ Remove questions of exams prints in a given course *************/
/*****************************************************************************/

void Exa_DB_RemovePrintQstsFromExa (long ExaCod)
  {
   DB_QueryDELETE ("can not remove exams prints in an exam",
		   "DELETE FROM exa_print_questions"
	           " USING exa_sessions,"
	                  "exa_prints,"
	                  "exa_print_questions"
                   " WHERE exa_sessions.ExaCod=%ld"
                     " AND exa_sessions.SesCod=exa_prints.SesCod"
                     " AND exa_prints.PrnCod=exa_print_questions.PrnCod",
		   ExaCod);
  }

/*****************************************************************************/
/************ Remove questions of exams prints in a given course *************/
/*****************************************************************************/

void Exa_DB_RemovePrintQstsFromCrs (long CrsCod)
  {
   DB_QueryDELETE ("can not remove exams prints in a course",
		   "DELETE FROM exa_print_questions"
	           " USING exa_exams,"
	                  "exa_sessions,"
	                  "exa_prints,"
	                  "exa_print_questions"
                   " WHERE exa_exams.CrsCod=%ld"
                     " AND exa_exams.ExaCod=exa_sessions.ExaCod"
                     " AND exa_sessions.SesCod=exa_prints.SesCod"
                     " AND exa_prints.PrnCod=exa_print_questions.PrnCod",
		   CrsCod);
  }

/*****************************************************************************/
/******** Check if the current session id is the same as the last one ********/
/*****************************************************************************/

bool Exa_DB_CheckIfSessionIsTheSameAsTheLast (long PrnCod)
  {
   /***** Check if the current session id
          is the same as the last one stored in database *****/
   return (DB_QueryCOUNT ("can not check session",
			  "SELECT COUNT(*)"
			   " FROM exa_log_sessions"
			  " WHERE LogCod="
				 "(SELECT MAX(LogCod)"
				   " FROM exa_log_sessions"
				  " WHERE PrnCod=%ld)"
			    " AND SessionId='%s'",
			  PrnCod,
			  Gbl.Session.Id) != 0);
  }

/*****************************************************************************/
/******** Check if the current user agent is the same as the last one ********/
/*****************************************************************************/

bool Exa_DB_CheckIfUserAgentIsTheSameAsTheLast (long PrnCod,const char *UserAgentDB)
  {
   /***** Get if the current user agent
          is the same as the last stored in database *****/
   return (DB_QueryCOUNT ("can not check user agent",
			  "SELECT COUNT(*)"
			   " FROM exa_log_user_agents"
			  " WHERE LogCod="
				 "(SELECT MAX(LogCod)"
				   " FROM exa_log_user_agents"
				  " WHERE PrnCod=%ld)"
			    " AND UserAgent='%s'",
			  PrnCod,
			  UserAgentDB) != 0);
  }

/*****************************************************************************/
/******************************** Log access *********************************/
/*****************************************************************************/

void Exa_DB_LogAccess (long LogCod,long PrnCod,ExaLog_Action_t Action)
  {
   /* Log access in exam log.
      Redundant data (also present in log table) are stored for speed */
   DB_QueryINSERT ("can not log exam access",
		   "INSERT INTO exa_log "
		   "(LogCod,PrnCod,ActCod,QstInd,CanAnswer,ClickTime,IP)"
		   " VALUES "
		   "(%ld,%ld,%ld,%d,'%c',NOW(),'%s')",
		   LogCod,
		   PrnCod,
		   (unsigned) Action,
		   ExaLog_GetQstInd (),
		   ExaLog_GetIfCanAnswer () ? 'Y' :
					      'N',
		   // NOW()   	   Redundant, for speed
		   Gbl.IP);	// Redundant, for speed
  }

/*****************************************************************************/
/*************************** Log session in database *************************/
/*****************************************************************************/

void Exa_DB_LogSession (long LogCod,long PrnCod)
  {
   DB_QueryINSERT ("can not log session",
		   "INSERT INTO exa_log_sessions "
		   "(LogCod,PrnCod,SessionId)"
		   " VALUES "
		   "(%ld,%ld,'%s')",
		   LogCod,
		   PrnCod,
		   Gbl.Session.Id);
  }

/*****************************************************************************/
/************************* Log user agent in database ************************/
/*****************************************************************************/

void Exa_DB_LogUserAgent (long LogCod,long PrnCod,const char *UserAgentDB)
  {
   DB_QueryINSERT ("can not log user agent",
		   "INSERT INTO exa_log_user_agents "
		   "(LogCod,PrnCod,UserAgent)"
		   " VALUES "
		   "(%ld,%ld,'%s')",
		   LogCod,
		   PrnCod,
		   UserAgentDB);
  }

/*****************************************************************************/
/********************* Get exam print log from database **********************/
/*****************************************************************************/

unsigned Exa_DB_GetExamLog (MYSQL_RES **mysql_res,long PrnCod)
  {
   return (unsigned)
   DB_QuerySELECT (mysql_res,"can not get exam print log",
		   "SELECT exa_log.ActCod,"			// row[0]
			  "exa_log.QstInd,"			// row[1]
			  "exa_log.CanAnswer,"			// row[2]
			  "UNIX_TIMESTAMP(exa_log.ClickTime),"	// row[3]
			  "exa_log.IP,"				// row[4]
			  "exa_log_sessions.SessionId,"		// row[5]
			  "exa_log_user_agents.UserAgent"	// row[6]
		    " FROM exa_log"
		    " LEFT JOIN exa_log_sessions"
		      " ON exa_log.LogCod=exa_log_sessions.LogCod"
		    " LEFT JOIN exa_log_user_agents"
		      " ON exa_log.LogCod=exa_log_user_agents.LogCod"
		   " WHERE exa_log.PrnCod=%ld"
		   " ORDER BY exa_log.LogCod",
		   PrnCod);
  }

/*****************************************************************************/
/*** Get all users who have answered any session question in a given exam ****/
/*****************************************************************************/

unsigned Exa_DB_GetAllUsrsWhoHaveMadeExam (MYSQL_RES **mysql_res,long ExaCod)
  {
   return (unsigned)
   DB_QuerySELECT (mysql_res,"can not get users in exam",
		   "SELECT users.UsrCod"	// row[0]
		    " FROM (SELECT DISTINCT exa_prints.UsrCod AS UsrCod"
			    " FROM exa_prints,"
			          "exa_sessions,"
			          "exa_exams"
			   " WHERE exa_sessions.ExaCod=%ld"
			     " AND exa_sessions.SesCod=exa_prints.SesCod"
			     " AND exa_sessions.ExaCod=exa_exams.ExaCod"
			     " AND exa_exams.CrsCod=%ld) AS users,"		// Extra check
			  "usr_data"
		   " WHERE users.UsrCod=usr_data.UsrCod"
		   " ORDER BY usr_data.Surname1,"
			     "usr_data.Surname2,"
			     "usr_data.FirstName",
		   ExaCod,
		   Gbl.Hierarchy.Crs.CrsCod);
  }

/*****************************************************************************/
/*** Get all users who have answered any question in a given exam session ****/
/*****************************************************************************/

unsigned Exa_DB_GetAllUsrsWhoHaveMadeSession (MYSQL_RES **mysql_res,long SesCod)
  {
   return (unsigned)
   DB_QuerySELECT (mysql_res,"can not get users in session",
		   "SELECT users.UsrCod"	// row[0]
		    " FROM (SELECT exa_prints.UsrCod AS UsrCod"
			    " FROM exa_prints,exa_sessions,exa_exams"
			   " WHERE exa_prints.SesCod=%ld"
			     " AND exa_prints.SesCod=exa_sessions.SesCod"
			     " AND exa_sessions.ExaCod=exa_exams.ExaCod"
			     " AND exa_exams.CrsCod=%ld) AS users,"	// Extra check
			  "usr_data"
		   " WHERE users.UsrCod=usr_data.UsrCod"
		   " ORDER BY usr_data.Surname1,"
			     "usr_data.Surname2,"
			     "usr_data.FirstName",
		   SesCod,
		   Gbl.Hierarchy.Crs.CrsCod);
  }

/*****************************************************************************/
/********* Show the sessions results of a user in the current course *********/
/*****************************************************************************/

unsigned Exa_DB_GetResults (MYSQL_RES **mysql_res,
			    Usr_MeOrOther_t MeOrOther,
			    long SesCod,	// <= 0 ==> any
			    long ExaCod,	// <= 0 ==> any
			    const char *ExamsSelectedCommas)
  {
   char *SesSubQuery;
   char *HidSesSubQuery;
   char *HidExaSubQuery;
   char *ExaSubQuery;
   long UsrCod;
   unsigned NumResults;

   /***** Set user *****/
   UsrCod = (MeOrOther == Usr_ME) ? Gbl.Usrs.Me.UsrDat.UsrCod :
				    Gbl.Usrs.Other.UsrDat.UsrCod;

   /***** Build sessions subquery *****/
   if (SesCod > 0)	// One unique session
     {
      if (asprintf (&SesSubQuery," AND exa_prints.SesCod=%ld",SesCod) < 0)
	 Err_NotEnoughMemoryExit ();
     }
   else			// All sessions of selected exams
     {
      if (asprintf (&SesSubQuery,"%s","") < 0)
	 Err_NotEnoughMemoryExit ();
     }

   /***** Subquery: get hidden sessions?
	  � A student will not be able to see their results in hidden sessions
	  � A teacher will be able to see results from other users even in hidden sessions
   *****/
   switch (MeOrOther)
     {
      case Usr_ME:	// A student watching her/his results
         if (asprintf (&HidSesSubQuery," AND exa_sessions.Hidden='N'") < 0)
	    Err_NotEnoughMemoryExit ();
	 break;
      case Usr_OTHER:	// A teacher/admin watching the results of other users
      default:
	 if (asprintf (&HidSesSubQuery,"%s","") < 0)
	    Err_NotEnoughMemoryExit ();
	 break;
     }

   /***** Build exams subquery *****/
   if (ExaCod > 0)			// One unique exams
     {
      if (asprintf (&ExaSubQuery," AND exa_sessions.ExaCod=%ld",ExaCod) < 0)
	 Err_NotEnoughMemoryExit ();
     }
   else if (ExamsSelectedCommas)
     {
      if (ExamsSelectedCommas[0])	// Selected exams
	{
	 if (asprintf (&ExaSubQuery," AND exa_sessions.ExaCod IN (%s)",
		       ExamsSelectedCommas) < 0)
	    Err_NotEnoughMemoryExit ();
	}
      else
	{
	 if (asprintf (&ExaSubQuery,"%s","") < 0)
	    Err_NotEnoughMemoryExit ();
	}
     }
   else					// All exams
     {
      if (asprintf (&ExaSubQuery,"%s","") < 0)
	 Err_NotEnoughMemoryExit ();
     }

   /***** Subquery: get hidden exams?
	  � A student will not be able to see their results in hidden exams
	  � A teacher will be able to see results from other users even in hidden exams
   *****/
   switch (MeOrOther)
     {
      case Usr_ME:	// A student watching her/his results
         if (asprintf (&HidExaSubQuery," AND exa_exams.Hidden='N'") < 0)
	    Err_NotEnoughMemoryExit ();
	 break;
      case Usr_OTHER:	// A teacher/admin watching the results of other users
      default:
	 if (asprintf (&HidExaSubQuery,"%s","") < 0)
	    Err_NotEnoughMemoryExit ();
	 break;
     }

   /***** Make database query *****/
   // Do not filter by groups, because a student who has changed groups
   // must be able to access exams taken in other groups
   NumResults = (unsigned)
   DB_QuerySELECT (mysql_res,"can not get sessions results",
		   "SELECT exa_prints.PrnCod"			// row[0]
		    " FROM exa_prints,exa_sessions,exa_exams"
		   " WHERE exa_prints.UsrCod=%ld"
		      "%s"	// Session subquery
		     " AND exa_prints.SesCod=exa_sessions.SesCod"
		      "%s"	// Hidden sessions subquery
		      "%s"	// Exams subquery
		     " AND exa_sessions.ExaCod=exa_exams.ExaCod"
		      "%s"	// Hidden exams subquery
		     " AND exa_exams.CrsCod=%ld"		// Extra check
		   " ORDER BY exa_sessions.Title",
		   UsrCod,
		   SesSubQuery,
		   HidSesSubQuery,
		   ExaSubQuery,
		   HidExaSubQuery,
		   Gbl.Hierarchy.Crs.CrsCod);
   free (HidExaSubQuery);
   free (ExaSubQuery);
   free (HidSesSubQuery);
   free (SesSubQuery);

   return NumResults;
  }
