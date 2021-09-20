// swad_game_database.c: games using remote control, operations with database

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
// #include <float.h>		// For DBL_MAX
// #include <linux/limits.h>	// For PATH_MAX
// #include <stddef.h>		// For NULL
#include <stdio.h>		// For asprintf
// #include <stdlib.h>		// For free
// #include <string.h>		// For string functions

#include "swad_database.h"
#include "swad_error.h"
// #include "swad_figure.h"
// #include "swad_form.h"
#include "swad_game.h"
#include "swad_game_database.h"
#include "swad_global.h"
// #include "swad_hierarchy_level.h"
// #include "swad_HTML.h"
// #include "swad_match.h"
// #include "swad_match_result.h"
// #include "swad_pagination.h"
// #include "swad_role.h"
// #include "swad_test.h"
// #include "swad_test_visibility.h"

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
/**************************** Create a new game ******************************/
/*****************************************************************************/

long Gam_DB_CreateGame (const struct Gam_Game *Game,const char *Txt)
  {
   Str_SetDecimalPointToUS ();		// To write the decimal point as a dot

   return
   DB_QueryINSERTandReturnCode ("can not create new game",
				"INSERT INTO gam_games"
				" (CrsCod,Hidden,UsrCod,MaxGrade,Visibility,"
				  "Title,Txt)"
				" VALUES"
				" (%ld,'N',%ld,%.15lg,%u,"
				  "'%s','%s')",
				Gbl.Hierarchy.Crs.CrsCod,
				Gbl.Usrs.Me.UsrDat.UsrCod,
				Game->MaxGrade,
				Game->Visibility,
				Game->Title,
				Txt);

   Str_SetDecimalPointToLocal ();	// Return to local system
  }
/*****************************************************************************/
/*************************** Update an existing game *************************/
/*****************************************************************************/

void Gam_DB_UpdateGame (const struct Gam_Game *Game,const char *Txt)
  {
   Str_SetDecimalPointToUS ();		// To write the decimal point as a dot

   DB_QueryUPDATE ("can not update game",
		   "UPDATE gam_games"
		     " SET CrsCod=%ld,"
		          "MaxGrade=%.15lg,"
		          "Visibility=%u,"
		          "Title='%s',"
		          "Txt='%s'"
		   " WHERE GamCod=%ld",
		   Gbl.Hierarchy.Crs.CrsCod,
		   Game->MaxGrade,
		   Game->Visibility,
	           Game->Title,
	           Txt,
	           Game->GamCod);

   Str_SetDecimalPointToLocal ();	// Return to local system
  }

/*****************************************************************************/
/********************************** Hide a game ******************************/
/*****************************************************************************/

void Gam_DB_HideGame (long GamCod)
  {
   DB_QueryUPDATE ("can not hide game",
		   "UPDATE gam_games"
		     " SET Hidden='Y'"
		   " WHERE GamCod=%ld",
		   GamCod);
  }

/*****************************************************************************/
/******************************** Unhide a game ******************************/
/*****************************************************************************/

void Gam_DB_UnhideGame (long GamCod)
  {
   DB_QueryUPDATE ("can not show game",
		   "UPDATE gam_games"
		     " SET Hidden='N'"
		   " WHERE GamCod=%ld",
		   GamCod);
  }

/*****************************************************************************/
/************* Get list of all the games in the current course ***************/
/*****************************************************************************/

unsigned Gam_DB_GetListGames (MYSQL_RES **mysql_res,Gam_Order_t SelectedOrder)
  {
   static const char *OrderBySubQuery[Gam_NUM_ORDERS] =
     {
      [Gam_ORDER_BY_START_DATE] = "StartTime DESC,"
	                          "EndTime DESC,"
	                          "gam_games.Title DESC",
      [Gam_ORDER_BY_END_DATE  ] = "EndTime DESC,"
	                          "StartTime DESC,"
	                          "gam_games.Title DESC",
      [Gam_ORDER_BY_TITLE     ] = "gam_games.Title",
     };
   ;
   char *HiddenSubQuery;
   unsigned NumGames;

   /***** Subquery: get hidden games depending on user's role *****/
   switch (Gbl.Usrs.Me.Role.Logged)
     {
      case Rol_STD:
         if (asprintf (&HiddenSubQuery," AND gam_games.Hidden='N'") < 0)
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

   /***** Get list of games from database *****/
   NumGames = (unsigned)
   DB_QuerySELECT (mysql_res,"can not get games",
		   "SELECT gam_games.GamCod,"				// row[0]
			  "MIN(mch_matches.StartTime) AS StartTime,"	// row[1]
			  "MAX(mch_matches.EndTime) AS EndTime"		// row[2]
		    " FROM gam_games"
		    " LEFT JOIN mch_matches"
		      " ON gam_games.GamCod=mch_matches.GamCod"
		   " WHERE gam_games.CrsCod=%ld"
		      "%s"
		   " GROUP BY gam_games.GamCod"
		   " ORDER BY %s",
		   Gbl.Hierarchy.Crs.CrsCod,
		   HiddenSubQuery,
		   OrderBySubQuery[SelectedOrder]);

   /***** Free allocated memory for subquery *****/
   free (HiddenSubQuery);

   return NumGames;
  }

/*****************************************************************************/
/********************** Get game data using its code *************************/
/*****************************************************************************/

unsigned Gam_DB_GetDataOfGameByCod (MYSQL_RES **mysql_res,long GamCod)
  {
   return (unsigned)
   DB_QuerySELECT (mysql_res,"can not get game data",
		   "SELECT gam_games.GamCod,"		// row[0]
			  "gam_games.CrsCod,"		// row[1]
			  "gam_games.Hidden,"		// row[2]
			  "gam_games.UsrCod,"		// row[3]
			  "gam_games.MaxGrade,"		// row[4]
			  "gam_games.Visibility,"	// row[5]
			  "gam_games.Title"		// row[6]
		    " FROM gam_games"
		    " LEFT JOIN mch_matches"
		      " ON gam_games.GamCod=mch_matches.GamCod"
		   " WHERE gam_games.GamCod=%ld"
		     " AND gam_games.CrsCod='%ld'",	// Extra check
		   GamCod,
		   Gbl.Hierarchy.Crs.CrsCod);
  }

/*****************************************************************************/
/********************** Get game text from database ************************/
/*****************************************************************************/

void Gam_DB_GetGameTxt (long GamCod,char Txt[Cns_MAX_BYTES_TEXT + 1])
  {
   /***** Get text of game from database *****/
   DB_QuerySELECTString (Txt,Cns_MAX_BYTES_TEXT,"can not get game text",
		         "SELECT Txt"	// row[0]
			  " FROM gam_games"
		         " WHERE GamCod=%ld",
		         GamCod);
  }

/*****************************************************************************/
/******************* Check if the title of a game exists *******************/
/*****************************************************************************/

bool Gam_DB_CheckIfSimilarGameExists (const struct Gam_Game *Game)
  {
   return (DB_QueryCOUNT ("can not get similar games",
			  "SELECT COUNT(*)"
			   " FROM gam_games"
			  " WHERE CrsCod=%ld"
			    " AND Title='%s'"
			    " AND GamCod<>%ld",
			  Gbl.Hierarchy.Crs.CrsCod,
			  Game->Title,
			  Game->GamCod) != 0);
  }

/*****************************************************************************/
/********************* Get number of courses with games **********************/
/*****************************************************************************/
// Returns the number of courses with games in this location

unsigned Gam_DB_GetNumCoursesWithGames (HieLvl_Level_t Scope)
  {
   /***** Get number of courses with games from database *****/
   switch (Scope)
     {
      case HieLvl_SYS:
         return (unsigned)
         DB_QueryCOUNT ("can not get number of courses with games",
			 "SELECT COUNT(DISTINCT CrsCod)"
			  " FROM gam_games");
      case HieLvl_CTY:
         return (unsigned)
         DB_QueryCOUNT ("can not get number of courses with games",
			 "SELECT COUNT(DISTINCT gam_games.CrsCod)"
			  " FROM ins_instits,"
			        "ctr_centers,"
			        "deg_degrees,"
			        "crs_courses,"
			        "gam_games"
			 " WHERE ins_instits.CtyCod=%ld"
			   " AND ins_instits.InsCod=ctr_centers.InsCod"
			   " AND ctr_centers.CtrCod=deg_degrees.CtrCod"
			   " AND deg_degrees.DegCod=crs_courses.DegCod"
			   " AND crs_courses.CrsCod=gam_games.CrsCod",
                         Gbl.Hierarchy.Ins.InsCod);
      case HieLvl_INS:
         return (unsigned)
         DB_QueryCOUNT ("can not get number of courses with games",
			 "SELECT COUNT(DISTINCT gam_games.CrsCod)"
			  " FROM ctr_centers,"
			        "deg_degrees,"
			        "crs_courses,"
			        "gam_games"
			 " WHERE ctr_centers.InsCod=%ld"
			   " AND ctr_centers.CtrCod=deg_degrees.CtrCod"
			   " AND deg_degrees.DegCod=crs_courses.DegCod"
			   " AND crs_courses.CrsCod=gam_games.CrsCod",
		         Gbl.Hierarchy.Ins.InsCod);
      case HieLvl_CTR:
         return (unsigned)
         DB_QueryCOUNT ("can not get number of courses with games",
			 "SELECT COUNT(DISTINCT gam_games.CrsCod)"
			  " FROM deg_degrees,"
			        "crs_courses,"
			        "gam_games"
			 " WHERE deg_degrees.CtrCod=%ld"
			   " AND deg_degrees.DegCod=crs_courses.DegCod"
			   " AND crs_courses.CrsCod=gam_games.CrsCod",
                         Gbl.Hierarchy.Ctr.CtrCod);
      case HieLvl_DEG:
         return (unsigned)
         DB_QueryCOUNT ("can not get number of courses with games",
			 "SELECT COUNT(DISTINCT gam_games.CrsCod)"
			  " FROM crs_courses,"
			        "gam_games"
			 " WHERE crs_courses.DegCod=%ld"
			   " AND crs_courses.CrsCod=gam_games.CrsCod",
		         Gbl.Hierarchy.Deg.DegCod);
      case HieLvl_CRS:
         return (unsigned)
         DB_QueryCOUNT ("can not get number of courses with games",
			 "SELECT COUNT(DISTINCT CrsCod)"
			  " FROM gam_games"
			 " WHERE CrsCod=%ld",
                         Gbl.Hierarchy.Crs.CrsCod);
      default:
	 return 0;
     }
  }

/*****************************************************************************/
/**************************** Get number of games ****************************/
/*****************************************************************************/
// Returns the number of games in this location

unsigned Gam_DB_GetNumGames (HieLvl_Level_t Scope)
  {
   /***** Get number of games from database *****/
   switch (Scope)
     {
      case HieLvl_SYS:
         return (unsigned)
         DB_QueryCOUNT ("can not get number of games",
                         "SELECT COUNT(*)"
			  " FROM gam_games");
      case HieLvl_CTY:
         return (unsigned)
         DB_QueryCOUNT ("can not get number of games",
                         "SELECT COUNT(*)"
			  " FROM ins_instits,"
			        "ctr_centers,"
			        "deg_degrees,"
			        "crs_courses,"
			        "gam_games"
			 " WHERE ins_instits.CtyCod=%ld"
			   " AND ins_instits.InsCod=ctr_centers.InsCod"
			   " AND ctr_centers.CtrCod=deg_degrees.CtrCod"
			   " AND deg_degrees.DegCod=crs_courses.DegCod"
			   " AND crs_courses.CrsCod=gam_games.CrsCod",
		         Gbl.Hierarchy.Cty.CtyCod);
      case HieLvl_INS:
         return (unsigned)
         DB_QueryCOUNT ("can not get number of games",
                         "SELECT COUNT(*)"
			  " FROM ctr_centers,"
			        "deg_degrees,"
			        "crs_courses,"
			        "gam_games"
			 " WHERE ctr_centers.InsCod=%ld"
			   " AND ctr_centers.CtrCod=deg_degrees.CtrCod"
			   " AND deg_degrees.DegCod=crs_courses.DegCod"
			   " AND crs_courses.CrsCod=gam_games.CrsCod",
		         Gbl.Hierarchy.Ins.InsCod);
      case HieLvl_CTR:
         return (unsigned)
         DB_QueryCOUNT ("can not get number of games",
                         "SELECT COUNT(*)"
			  " FROM deg_degrees,"
			        "crs_courses,"
			        "gam_games"
			 " WHERE deg_degrees.CtrCod=%ld"
			   " AND deg_degrees.DegCod=crs_courses.DegCod"
			   " AND crs_courses.CrsCod=gam_games.CrsCod",
		         Gbl.Hierarchy.Ctr.CtrCod);
      case HieLvl_DEG:
         return (unsigned)
         DB_QueryCOUNT ("can not get number of games",
                         "SELECT COUNT(*)"
			  " FROM crs_courses,"
			        "gam_games"
			 " WHERE crs_courses.DegCod=%ld"
			   " AND crs_courses.CrsCod=gam_games.CrsCod",
		         Gbl.Hierarchy.Deg.DegCod);
      case HieLvl_CRS:
         return (unsigned)
         DB_QueryCOUNT ("can not get number of games",
                         "SELECT COUNT(*)"
			  " FROM gam_games"
			 " WHERE CrsCod=%ld",
                         Gbl.Hierarchy.Crs.CrsCod);
      default:
	 return 0;
     }
  }

/*****************************************************************************/
/********************************* Remove game *******************************/
/*****************************************************************************/

void Gam_DB_RemoveGame (long GamCod)
  {
   DB_QueryDELETE ("can not remove game",
		   "DELETE FROM gam_games"
		   " WHERE GamCod=%ld",
		   GamCod);
  }

/*****************************************************************************/
/*********************** Remove the games of a course ************************/
/*****************************************************************************/

void Gam_DB_RemoveCrsGames (long CrsCod)
  {
   DB_QueryDELETE ("can not remove course games",
		   "DELETE FROM gam_games"
		   " WHERE CrsCod=%ld",
                   CrsCod);
  }

/*****************************************************************************/
/**************** Insert question in the table of questions ******************/
/*****************************************************************************/

void Gam_DB_InsertQstInGame (long GamCod,unsigned QstInd,long QstCod)
  {
   DB_QueryINSERT ("can not add question to game",
		   "INSERT INTO gam_questions"
		   " (GamCod,QstInd,QstCod)"
		   " VALUES"
		   " (%ld,%u,%ld)",
		   GamCod,
		   QstInd,
		   QstCod);
  }

/*****************************************************************************/
/*********** Update indexes of questions greater than a given one ************/
/*****************************************************************************/

void Gam_DB_UpdateIndexesOfQstsGreaterThan (long GamCod,unsigned QstInd)
  {
   DB_QueryUPDATE ("can not update indexes of questions",
		   "UPDATE gam_questions"
		     " SET QstInd=QstInd-1"
		   " WHERE GamCod=%ld"
		     " AND QstInd>%u",
		   GamCod,
		   QstInd);
  }

/*****************************************************************************/
/********************* Change index of a set in an exam **********************/
/*****************************************************************************/

void Gam_DB_UpdateQstIndex (long QstInd,long GamCod,long QstCod)
  {
   DB_QueryUPDATE ("can not exchange indexes of questions",
		   "UPDATE gam_questions"
		     " SET QstInd=%ld"
		   " WHERE GamCod=%ld"
		     " AND QstCod=%ld",
		   QstInd,
		   GamCod,
		   QstCod);
  }

/*****************************************************************************/
/************ Lock table to make the exchange of questions atomic ************/
/*****************************************************************************/

void Gam_DB_LockTable (void)
  {
   DB_Query ("can not lock tables to move game question",
	     "LOCK TABLES gam_questions WRITE");
   Gbl.DB.LockedTables = true;
  }

/*****************************************************************************/
/********** Unlock table to make the exchange of questions atomic ************/
/*****************************************************************************/

void Gam_DB_UnlockTable (void)
  {
   Gbl.DB.LockedTables = false;	// Set to false before the following unlock...
				// ...to not retry the unlock if error in unlocking
   DB_Query ("can not unlock tables after moving game questions",
	     "UNLOCK TABLES");
  }

/*****************************************************************************/
/******************* Get number of questions of a game *********************/
/*****************************************************************************/

unsigned Gam_DB_GetNumQstsGame (long GamCod)
  {
   /***** Get nuumber of questions in a game from database *****/
   return (unsigned)
   DB_QueryCOUNT ("can not get number of questions of a game",
		  "SELECT COUNT(*)"
		   " FROM gam_questions"
		  " WHERE GamCod=%ld",
		  GamCod);
  }
/*****************************************************************************/
/************************ Get the questions of a game ************************/
/*****************************************************************************/

unsigned Gam_DB_GetGameQuestions (MYSQL_RES **mysql_res,long GamCod)
  {
   return (unsigned)
   DB_QuerySELECT (mysql_res,"can not get game questions",
		   "SELECT QstInd,"	// row[0]
			  "QstCod"	// row[1]
		    " FROM gam_questions"
		   " WHERE GamCod=%ld"
		   " ORDER BY QstInd",
		   GamCod);
  }

/*****************************************************************************/
/************ Get question code given game and index of question *************/
/*****************************************************************************/

long Gam_DB_GetQstCodFromQstInd (long GamCod,unsigned QstInd)
  {
   long QstCod;

   /***** Get question code of the question to be moved up *****/
   QstCod = DB_QuerySELECTCode ("can not get question code",
				"SELECT QstCod"
				 " FROM gam_questions"
				" WHERE GamCod=%ld"
				  " AND QstInd=%u",
				GamCod,
				QstInd);
   if (QstCod <= 0)
      Err_WrongQuestionExit ();

   return QstCod;
  }

/*****************************************************************************/
/************ Get question index given game and code of question *************/
/*****************************************************************************/
// Return 0 is question is not present in game

unsigned Gam_DB_GetQstIndFromQstCod (long GamCod,long QstCod)
  {
   /***** Get question index in a game given the question code *****/
   return DB_QuerySELECTUnsigned ("can not get question index",
				  "SELECT QstInd"
				   " FROM gam_questions"
				  " WHERE GamCod=%ld"
				    " AND QstCod=%ld",
				  GamCod,
				  QstCod);
  }

/*****************************************************************************/
/****************** Get maximum question index in a game *********************/
/*****************************************************************************/
// Question index can be 1, 2, 3...
// Return 0 if no questions

unsigned Gam_DB_GetMaxQuestionIndexInGame (long GamCod)
  {
   /***** Get maximum question index in a game from database *****/
   return DB_QuerySELECTUnsigned ("can not get last question index",
				  "SELECT MAX(QstInd)"
				   " FROM gam_questions"
				  " WHERE GamCod=%ld",
				  GamCod);
  }

/*****************************************************************************/
/*********** Get previous question index to a given index in a game **********/
/*****************************************************************************/
// Input question index can be 1, 2, 3... n-1
// Return question index will be 1, 2, 3... n if previous question exists, or 0 if no previous question

unsigned Gam_DB_GetPrevQuestionIndexInGame (long GamCod,unsigned QstInd)
  {
   /***** Get previous question index in a game from database *****/
   // Although indexes are always continuous...
   // ...this implementation works even with non continuous indexes
   return DB_QuerySELECTUnsigned ("can not get previous question index",
				  "SELECT COALESCE(MAX(QstInd),0)"
				   " FROM gam_questions"
				  " WHERE GamCod=%ld"
				    " AND QstInd<%u",
				  GamCod,
				  QstInd);
  }

/*****************************************************************************/
/************* Get next question index to a given index in a game ************/
/*****************************************************************************/
// Input question index can be 0, 1, 2, 3... n-1
// Return question index will be 1, 2, 3... n if next question exists, or big number if no next question

unsigned Gam_DB_GetNextQuestionIndexInGame (long GamCod,unsigned QstInd)
  {
   /***** Get next question index in a game from database *****/
   // Although indexes are always continuous...
   // ...this implementation works even with non continuous indexes
   return DB_QuerySELECTUnsigned ("can not get next question index",
				  "SELECT COALESCE(MIN(QstInd),%u)"
				   " FROM gam_questions"
				  " WHERE GamCod=%ld"
				    " AND QstInd>%u",
				  Gam_AFTER_LAST_QUESTION,	// End of questions has been reached
				  GamCod,
				  QstInd);
  }

/*****************************************************************************/
/***************** Get average number of questions per game ******************/
/*****************************************************************************/

double Gam_DB_GetNumQstsPerGame (HieLvl_Level_t Scope)
  {
   /***** Get number of questions per game from database *****/
   switch (Scope)
     {
      case HieLvl_SYS:
         return
         DB_QuerySELECTDouble ("can not get number of questions per game",
			       "SELECT AVG(NumQsts)"
			        " FROM (SELECT COUNT(gam_questions.QstCod) AS NumQsts"
				        " FROM gam_games,"
					      "gam_questions"
				       " WHERE gam_games.GamCod=gam_questions.GamCod"
				       " GROUP BY gam_questions.GamCod) AS NumQstsTable");
      case HieLvl_CTY:
         return
         DB_QuerySELECTDouble ("can not get number of questions per game",
			       "SELECT AVG(NumQsts)"
			       " FROM (SELECT COUNT(gam_questions.QstCod) AS NumQsts"
				       " FROM ins_instits,"
					     "ctr_centers,"
					     "deg_degrees,"
					     "crs_courses,"
					     "gam_games,"
					     "gam_questions"
				      " WHERE ins_instits.CtyCod=%ld"
				        " AND ins_instits.InsCod=ctr_centers.InsCod"
				        " AND ctr_centers.CtrCod=deg_degrees.CtrCod"
				        " AND deg_degrees.DegCod=crs_courses.DegCod"
				        " AND crs_courses.CrsCod=gam_games.CrsCod"
				        " AND gam_games.GamCod=gam_questions.GamCod"
				      " GROUP BY gam_questions.GamCod) AS NumQstsTable",
			       Gbl.Hierarchy.Cty.CtyCod);
      case HieLvl_INS:
         return
         DB_QuerySELECTDouble ("can not get number of questions per game",
			       "SELECT AVG(NumQsts)"
			       " FROM (SELECT COUNT(gam_questions.QstCod) AS NumQsts"
				       " FROM ctr_centers,"
					     "deg_degrees,"
					     "crs_courses,"
					     "gam_games,"
					     "gam_questions"
				      " WHERE ctr_centers.InsCod=%ld"
				        " AND ctr_centers.CtrCod=deg_degrees.CtrCod"
				        " AND deg_degrees.DegCod=crs_courses.DegCod"
				        " AND crs_courses.CrsCod=gam_games.CrsCod"
				        " AND gam_games.GamCod=gam_questions.GamCod"
				     " GROUP BY gam_questions.GamCod) AS NumQstsTable",
			       Gbl.Hierarchy.Ins.InsCod);
      case HieLvl_CTR:
         return
         DB_QuerySELECTDouble ("can not get number of questions per game",
			       "SELECT AVG(NumQsts)"
			        " FROM (SELECT COUNT(gam_questions.QstCod) AS NumQsts"
				        " FROM deg_degrees,"
					      "crs_courses,"
					      "gam_games,"
					      "gam_questions"
				       " WHERE deg_degrees.CtrCod=%ld"
				         " AND deg_degrees.DegCod=crs_courses.DegCod"
				         " AND crs_courses.CrsCod=gam_games.CrsCod"
				         " AND gam_games.GamCod=gam_questions.GamCod"
				       " GROUP BY gam_questions.GamCod) AS NumQstsTable",
			       Gbl.Hierarchy.Ctr.CtrCod);
      case HieLvl_DEG:
         return
         DB_QuerySELECTDouble ("can not get number of questions per game",
			       "SELECT AVG(NumQsts)"
			        " FROM (SELECT COUNT(gam_questions.QstCod) AS NumQsts"
				        " FROM crs_courses,"
					      "gam_games,"
					      "gam_questions"
				       " WHERE crs_courses.DegCod=%ld"
				         " AND crs_courses.CrsCod=gam_games.CrsCod"
				         " AND gam_games.GamCod=gam_questions.GamCod"
				       " GROUP BY gam_questions.GamCod) AS NumQstsTable",
			       Gbl.Hierarchy.Deg.DegCod);
      case HieLvl_CRS:
         return
         DB_QuerySELECTDouble ("can not get number of questions per game",
			       "SELECT AVG(NumQsts)"
			        " FROM (SELECT COUNT(gam_questions.QstCod) AS NumQsts"
				        " FROM gam_games,"
					      "gam_questions"
				       " WHERE gam_games.Cod=%ld"
				         " AND gam_games.GamCod=gam_questions.GamCod"
				       " GROUP BY gam_questions.GamCod) AS NumQstsTable",
			       Gbl.Hierarchy.Crs.CrsCod);
      default:
	 Err_WrongScopeExit ();
	 return 0.0;	// Not reached
     }
  }

/*****************************************************************************/
/********************* Get all tags of questions in a game *******************/
/*****************************************************************************/

unsigned Gam_DB_GetTstTagsPresentInAGame (MYSQL_RES **mysql_res,long GamCod)
  {
   return (unsigned)
   DB_QuerySELECT (mysql_res,"can not get tags present in a match result",
		   "SELECT tst_tags.TagTxt"	// row[0]
		    " FROM (SELECT DISTINCT(tst_question_tags.TagCod)"
			    " FROM tst_question_tags,gam_questions"
			   " WHERE gam_questions.GamCod=%ld"
			     " AND gam_questions.QstCod=tst_question_tags.QstCod) AS TagsCods,"
			  "tst_tags"
		   " WHERE TagsCods.TagCod=tst_tags.TagCod"
		   " ORDER BY tst_tags.TagTxt",
		   GamCod);
  }

/*****************************************************************************/
/************* Get number of answers of each question in a game **************/
/*****************************************************************************/

unsigned Gam_DB_GetNumAnswersOfQstsInGame (MYSQL_RES **mysql_res,long GamCod)
  {
   return (unsigned)
   DB_QuerySELECT (mysql_res,"can not get data of a question",
		   "SELECT COUNT(tst_answers.AnsInd) AS N"	// row[0]
		    " FROM tst_answers,"
		          "gam_questions"
		   " WHERE gam_questions.GamCod=%ld"
		     " AND gam_questions.QstCod=tst_answers.QstCod"
		   " GROUP BY tst_answers.QstCod",
		   GamCod);
  }

/*****************************************************************************/
/************************* Remove question from game *************************/
/*****************************************************************************/

void Gam_DB_RemoveQstFromGame (long GamCod,unsigned QstInd)
  {
   DB_QueryDELETE ("can not remove a question",
		   "DELETE FROM gam_questions"
		   " WHERE GamCod=%ld"
		     " AND QstInd=%u",
		   GamCod,
		   QstInd);
   }

/*****************************************************************************/
/**************************** Remove game questions **************************/
/*****************************************************************************/

void Gam_DB_RemoveGameQsts (long GamCod)
  {
   DB_QueryDELETE ("can not remove game questions",
		   "DELETE FROM gam_questions"
		   " WHERE GamCod=%ld",
		   GamCod);
  }

/*****************************************************************************/
/***************** Remove the questions in games of a course *****************/
/*****************************************************************************/

void Gam_DB_RemoveCrsGameQsts (long CrsCod)
  {
   DB_QueryDELETE ("can not remove questions in course games",
		   "DELETE FROM gam_questions"
		   " USING gam_games,"
		          "gam_questions"
		   " WHERE gam_games.CrsCod=%ld"
		     " AND gam_games.GamCod=gam_questions.GamCod",
                   CrsCod);
  }
