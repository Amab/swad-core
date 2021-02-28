// swad_timeline_database.c: social timeline operations with database

/*
    SWAD (Shared Workspace At a Distance),
    is a web platform developed at the University of Granada (Spain),
    and used to support university teaching.

    This file is part of SWAD core.
    Copyright (C) 1999-2021 Antonio Ca�as Vargas

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU Affero General 3 License as
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
/*********************************** Headers *********************************/
/*****************************************************************************/

#include "swad_database.h"
#include "swad_follow.h"
#include "swad_global.h"
#include "swad_timeline.h"
#include "swad_timeline_database.h"
#include "swad_timeline_publication.h"

/*****************************************************************************/
/****************************** Public constants *****************************/
/*****************************************************************************/

/*****************************************************************************/
/************************* Private constants and types ***********************/
/*****************************************************************************/

/*****************************************************************************/
/************** External global variables from others modules ****************/
/*****************************************************************************/

extern struct Globals Gbl;

/*****************************************************************************/
/************************* Private global variables **************************/
/*****************************************************************************/

/*****************************************************************************/
/***************************** Private prototypes ****************************/
/*****************************************************************************/

static long TL_DB_GetMedCodFromPub (long PubCod,const char *DBTable);

/*****************************************************************************/
/********************* Get data of note using its code ***********************/
/*****************************************************************************/
// Returns the number of rows got

unsigned TL_DB_GetDataOfNoteByCod (long NotCod,MYSQL_RES **mysql_res)
  {
   /***** Trivial check: note code should be > 0 *****/
   if (NotCod <= 0)
      return 0;

   /***** Get data of note from database *****/
   return (unsigned)
   DB_QuerySELECT (mysql_res,"can not get data of note",
		   "SELECT NotCod,"			// row[0]
			  "NoteType,"			// row[1]
			  "Cod,"			// row[2]
			  "UsrCod,"			// row[3]
			  "HieCod,"			// row[4]
			  "Unavailable,"		// row[5]
			  "UNIX_TIMESTAMP(TimeNote)"	// row[6]
		   " FROM tl_notes"
		   " WHERE NotCod=%ld",
		   NotCod);
  }

/*****************************************************************************/
/*************** Get code of publication of the original note ****************/
/*****************************************************************************/

long TL_DB_GetPubCodOfOriginalNote (long NotCod)
  {
   MYSQL_RES *mysql_res;
   MYSQL_ROW row;
   long OriginalPubCod = -1L;	// Default value

   /***** Get code of publication of the original note *****/
   if (DB_QuerySELECT (&mysql_res,"can not get code of publication",
		       "SELECT PubCod FROM tl_pubs"
		       " WHERE NotCod=%ld AND PubType=%u",
		       NotCod,(unsigned) TL_Pub_ORIGINAL_NOTE) == 1)   // Result should have a unique row
     {
      /* Get code of publication (row[0]) */
      row = mysql_fetch_row (mysql_res);
      OriginalPubCod = Str_ConvertStrCodToLongCod (row[0]);
     }

   /***** Free structure that stores the query result *****/
   DB_FreeMySQLResult (&mysql_res);

   return OriginalPubCod;
  }

/*****************************************************************************/
/***************************** Create a new note *****************************/
/*****************************************************************************/
// Returns code of note just created

long TL_DB_CreateNewNote (TL_Not_NoteType_t NoteType,long Cod,
                          long PublisherCod,long HieCod)
  {
   return
   DB_QueryINSERTandReturnCode ("can not create new note",
				"INSERT INTO tl_notes"
				" (NoteType,Cod,UsrCod,HieCod,Unavailable,TimeNote)"
				" VALUES"
				" (%u,%ld,%ld,%ld,'N',NOW())",
				(unsigned) NoteType,	// Post, file, exam, notice, forum
				Cod,			// Post, file, exam, notice, forum code
				PublisherCod,		// Publisher code
				HieCod);		// Where in hierarchy
  }

/*****************************************************************************/
/************************* Mark a note as unavailable ************************/
/*****************************************************************************/

void TL_DB_MarkNoteAsUnavailable (TL_Not_NoteType_t NoteType,long Cod)
  {
   /***** Mark note as unavailable *****/
   DB_QueryUPDATE ("can not mark note as unavailable",
		   "UPDATE tl_notes SET Unavailable='Y'"
		   " WHERE NoteType=%u AND Cod=%ld",
		   (unsigned) NoteType,Cod);
  }

/*****************************************************************************/
/***** Mark possible notes involving children of a folder as unavailable *****/
/*****************************************************************************/

void TL_DB_MarkNotesChildrenOfFolderAsUnavailable (TL_Not_NoteType_t NoteType,
                                                   Brw_FileBrowser_t FileBrowser,
                                                   long Cod,
                                                   const char *Path)
  {
   /***** Mark notes as unavailable *****/
   DB_QueryUPDATE ("can not mark notes as unavailable",
		   "UPDATE tl_notes SET Unavailable='Y'"
		   " WHERE NoteType=%u AND Cod IN"
		   " (SELECT FilCod FROM files"
		   " WHERE FileBrowser=%u AND Cod=%ld"
		   " AND Path LIKE '%s/%%' AND Public='Y')",	// Only public files
		   (unsigned) NoteType,
		   (unsigned) FileBrowser,Cod,
		   Path);
  }

/*****************************************************************************/
/******* Create temporary tables used to not get notes already shown *********/
/*****************************************************************************/

void TL_DB_CreateTmpTableJustRetrievedNotes (void)
  {
   /***** Create temporary table with notes just retrieved *****/
   DB_Query ("can not create temporary table",
	     "CREATE TEMPORARY TABLE tl_tmp_just_retrieved_notes "
	     "(NotCod BIGINT NOT NULL,UNIQUE INDEX(NotCod))"
	     " ENGINE=MEMORY");
  }

void TL_DB_CreateTmpTableVisibleTimeline (void)
  {
   /***** Create temporary table with all notes visible in timeline *****/
   DB_Query ("can not create temporary table",
	     "CREATE TEMPORARY TABLE tl_tmp_visible_timeline "
	     "(NotCod BIGINT NOT NULL,UNIQUE INDEX(NotCod))"
	     " ENGINE=MEMORY"
	     " SELECT NotCod FROM tl_timelines WHERE SessionId='%s'",
	     Gbl.Session.Id);
  }

/*****************************************************************************/
/**** Insert note in temporary tables used to not get notes already shown ****/
/*****************************************************************************/

void TL_DB_InsertNoteInJustRetrievedNotes (long NotCod)
  {
   /* Insert note in temporary table with just retrieved notes.
      This table will be used to not get notes already shown */
   DB_QueryINSERT ("can not store note code",
		   "INSERT IGNORE INTO tl_tmp_just_retrieved_notes"
		   " SET NotCod=%ld",
		   NotCod);
  }

void TL_DB_InsertNoteInVisibleTimeline (long NotCod)
  {
   /* Insert note in temporary table with visible timeline.
      This table will be used to not get notes already shown */
   DB_QueryINSERT ("can not store note code",
		   "INSERT IGNORE INTO tl_tmp_visible_timeline"
		   " SET NotCod=%ld",
		   NotCod);
  }

/*****************************************************************************/
/****** Add just retrieved notes to current timeline for this session ********/
/*****************************************************************************/

void TL_DB_AddNotesJustRetrievedToVisibleTimelineOfSession (void)
  {
   /* tl_timelines contains the distinct notes in timeline of each open session:
mysql> SELECT SessionId,COUNT(*) FROM tl_timelines GROUP BY SessionId;
+---------------------------------------------+----------+
| SessionId                                   | COUNT(*) |
+---------------------------------------------+----------+
| u-X-R3gKki7eKMXrNCP8bGhwOAZuVngRy7FNGZFMKzI |       52 | --> 52 distinct notes
| u1CoqL1YWl3_hR4wk4bI7vhnc-uRcCmIDyKYAgBB6kk |       10 |
| u8xqamzkorHfY4BvYRMXjNhzHvQyigZUZemO0YiMn48 |       10 |
| u_n2V_L3KrFjnd4SqZk0gxMFwZHRuWZ8_EIVTU9sdpI |       10 |
| V6pGe1kGGS_uO5i__waqXKnuDkPYaDZHNAYr-Zv-GJQ |        2 |
| vqDRz-iiM8v10Dl8ThwqIqmDRIklz8szJaqflwXZucs |       10 |
| w11juqKPx6lg-f_pL2ZBYqlagU1mEepSvvk9L3gDGac |       10 | --> 10 distinct notes
| wLg4e8KQljCcVuFWIkJjNeti89kAiwOZ3iyXdzm_eDk |       10 |
| wnU85YrwJHhZGWIZhd7LQfQTPrclIWHfMF3DcB-Rcgw |        4 |
| wRzRJFnHfzW61fZYnvMIaMRlkuWUeEyqXVQ6JeWA32k |       11 |
+---------------------------------------------+----------+
10 rows in set (0,01 sec)
   */
   DB_QueryINSERT ("can not insert notes in timeline",
		   "INSERT IGNORE INTO tl_timelines"
	           " (SessionId,NotCod)"
	           " SELECT '%s',NotCod FROM tl_tmp_just_retrieved_notes",
		   Gbl.Session.Id);
  }

/*****************************************************************************/
/******** Drop temporary tables used to not get notes already shown **********/
/*****************************************************************************/

void TL_DB_DropTmpTableJustRetrievedNotes (void)
  {
   /***** Drop temporary table with notes just retrieved *****/
   DB_Query ("can not remove temporary table",
	     "DROP TEMPORARY TABLE IF EXISTS tl_tmp_just_retrieved_notes");
  }

void TL_DB_DropTmpTableVisibleTimeline (void)
  {
   /***** Drop temporary table with all notes visible in timeline *****/
   DB_Query ("can not remove temporary table",
             "DROP TEMPORARY TABLE IF EXISTS tl_tmp_visible_timeline");
  }

/*****************************************************************************/
/******************* Clear unused old timelines in database ******************/
/*****************************************************************************/

void TL_DB_ClearOldTimelinesNotesFromDB (void)
  {
   /***** Remove timelines for expired sessions *****/
   DB_QueryDELETE ("can not remove old timelines",
		   "DELETE LOW_PRIORITY FROM tl_timelines"
                   " WHERE SessionId NOT IN (SELECT SessionId FROM sessions)");
  }

/*****************************************************************************/
/***************** Clear timeline for a session in database ******************/
/*****************************************************************************/

void TL_DB_ClearTimelineNotesOfSessionFromDB (void)
  {
   /***** Remove timeline for a session *****/
   DB_QueryDELETE ("can not remove timeline",
		   "DELETE FROM tl_timelines"
		   " WHERE SessionId='%s'",
		   Gbl.Session.Id);
  }

/*****************************************************************************/
/*************************** Remove favs for a note **************************/
/*****************************************************************************/

void TL_DB_RemoveNoteFavs (long NotCod)
  {
   /***** Remove favs for note *****/
   DB_QueryDELETE ("can not remove favs for note",
		   "DELETE FROM tl_notes_fav"
		   " WHERE NotCod=%ld",
		   NotCod);
  }

/*****************************************************************************/
/******************** Remove all publications of this note *******************/
/*****************************************************************************/

void TL_DB_RemoveNotePubs (long NotCod)
  {
   /***** Remove all publications of this note *****/
   DB_QueryDELETE ("can not remove a publication",
		   "DELETE FROM tl_pubs"
		   " WHERE NotCod=%ld",
		   NotCod);
  }

/*****************************************************************************/
/******************* Remove note publication from database *******************/
/*****************************************************************************/

void TL_DB_RemoveNote (long NotCod)
  {
   /***** Remove note *****/
   DB_QueryDELETE ("can not remove a note",
		   "DELETE FROM tl_notes"
	           " WHERE NotCod=%ld"
	           " AND UsrCod=%ld",		// Extra check: author
		   NotCod,
		   Gbl.Usrs.Me.UsrDat.UsrCod);
  }

/*****************************************************************************/
/************** Get publication codes of comments of a note from database *****************/
/*****************************************************************************/
// Returns the number of rows got

unsigned TL_DB_GetPostByCod (long PstCod,MYSQL_RES **mysql_res)
  {
   return (unsigned)
   DB_QuerySELECT (mysql_res,"can not get the content"
			     " of a post",
		   "SELECT Txt,"		// row[0]
			  "MedCod"		// row[1]
		   " FROM tl_posts"
		   " WHERE PstCod=%ld",
		   PstCod);
  }

/*****************************************************************************/
/***************** Get code of media associated to post **********************/
/*****************************************************************************/

long TL_DB_GetMedCodFromPost (long PubCod)
  {
   return TL_DB_GetMedCodFromPub (PubCod,"tl_posts");
  }

/*****************************************************************************/
/********************* Insert post content in database ***********************/
/*****************************************************************************/
// Returns code of just created post

long TL_DB_CreateNewPost (const struct TL_Pst_PostContent *Content)
  {
   /***** Insert post content in the database *****/
   return
   DB_QueryINSERTandReturnCode ("can not create post",
				"INSERT INTO tl_posts"
				" (Txt,MedCod)"
				" VALUES"
				" ('%s',%ld)",
				Content->Txt,
				Content->Media.MedCod);
  }

/*****************************************************************************/
/************************* Remove post from database *************************/
/*****************************************************************************/

void TL_DB_RemovePost (long PstCod)
  {
   /***** Remove post *****/
   DB_QueryDELETE ("can not remove a post",
		   "DELETE FROM tl_posts"
		   " WHERE PstCod=%ld",
		   PstCod);
  }

/*****************************************************************************/
/********************* Get number of comments in a note **********************/
/*****************************************************************************/

unsigned TL_DB_GetNumCommentsInNote (long NotCod)
  {
   return (unsigned)
   DB_QueryCOUNT ("can not get number of comments in a note",
		  "SELECT COUNT(*) FROM tl_pubs"
		  " WHERE NotCod=%ld AND PubType=%u",
		  NotCod,(unsigned) TL_Pub_COMMENT_TO_NOTE);
  }

/*****************************************************************************/
/************** Get publication codes of comments of a note from database *****************/
/*****************************************************************************/
// Returns the number of rows got

unsigned TL_DB_GetComments (long NotCod,MYSQL_RES **mysql_res)
  {
   return (unsigned)
   DB_QuerySELECT (mysql_res,"can not get comments",
		   "SELECT PubCod"	// row[0]
		   " FROM tl_pubs"
		   " WHERE NotCod=%ld AND PubType=%u",
		   NotCod,(unsigned) TL_Pub_COMMENT_TO_NOTE);
  }

/*****************************************************************************/
/************** Get initial comments of a note from database *****************/
/*****************************************************************************/
// Returns the number of rows got

unsigned TL_DB_GetInitialComments (long NotCod,
				   unsigned NumInitialCommentsToGet,
				   MYSQL_RES **mysql_res)
  {
   return (unsigned)
   DB_QuerySELECT (mysql_res,"can not get comments",
		   "SELECT tl_pubs.PubCod,"			// row[0]
			  "tl_pubs.PublisherCod,"		// row[1]
			  "tl_pubs.NotCod,"			// row[2]
			  "UNIX_TIMESTAMP(tl_pubs.TimePublish),"// row[3]
			  "tl_comments.Txt,"			// row[4]
			  "tl_comments.MedCod"			// row[5]
		   " FROM tl_pubs,tl_comments"
		   " WHERE tl_pubs.NotCod=%ld"
		   " AND tl_pubs.PubType=%u"
		   " AND tl_pubs.PubCod=tl_comments.PubCod"
		   " ORDER BY tl_pubs.PubCod"
		   " LIMIT %lu",
		   NotCod,(unsigned) TL_Pub_COMMENT_TO_NOTE,
		   NumInitialCommentsToGet);
  }

/*****************************************************************************/
/*************** Get final comments of a note from database ******************/
/*****************************************************************************/
// Returns the number of rows got

unsigned TL_DB_GetFinalComments (long NotCod,
				 unsigned NumFinalCommentsToGet,
				 MYSQL_RES **mysql_res)
  {
   /***** Get final comments of a note from database *****/
   return (unsigned)
   DB_QuerySELECT (mysql_res,"can not get comments",
		   "SELECT * FROM "
		   "("
		   "SELECT tl_pubs.PubCod,"			// row[0]
			  "tl_pubs.PublisherCod,"		// row[1]
			  "tl_pubs.NotCod,"			// row[2]
			  "UNIX_TIMESTAMP(tl_pubs.TimePublish),"// row[3]
			  "tl_comments.Txt,"			// row[4]
			  "tl_comments.MedCod"			// row[5]
	          " FROM tl_pubs,tl_comments"
		  " WHERE tl_pubs.NotCod=%ld"
		  " AND tl_pubs.PubType=%u"
		  " AND tl_pubs.PubCod=tl_comments.PubCod"
		  " ORDER BY tl_pubs.PubCod DESC LIMIT %u"
		  ") AS comments"
		  " ORDER BY PubCod",
		  NotCod,(unsigned) TL_Pub_COMMENT_TO_NOTE,
		  NumFinalCommentsToGet);
  }

/*****************************************************************************/
/******************* Get data of comment using its code **********************/
/*****************************************************************************/
// Returns the number of rows got

unsigned TL_DB_GetDataOfCommByCod (long PubCod,MYSQL_RES **mysql_res)
  {
   /***** Trivial check: publication code should be > 0 *****/
   if (PubCod <= 0)
      return 0;

   /***** Get data of comment from database *****/
   return (unsigned)
   DB_QuerySELECT (mysql_res,"can not get data of comment",
		   "SELECT tl_pubs.PubCod,"			// row[0]
			  "tl_pubs.PublisherCod,"		// row[1]
			  "tl_pubs.NotCod,"			// row[2]
			  "UNIX_TIMESTAMP(tl_pubs.TimePublish),"// row[3]
			  "tl_comments.Txt,"			// row[4]
			  "tl_comments.MedCod"			// row[5]
		   " FROM tl_pubs,tl_comments"
		   " WHERE tl_pubs.PubCod=%ld"
		   " AND tl_pubs.PubType=%u"
		   " AND tl_pubs.PubCod=tl_comments.PubCod",
		   PubCod,(unsigned) TL_Pub_COMMENT_TO_NOTE);
  }

/*****************************************************************************/
/******************* Insert comment content in database **********************/
/*****************************************************************************/

void TL_DB_InsertCommentContent (long PubCod,
				 const struct TL_Pst_PostContent *Content)
  {
   /***** Insert comment content in database *****/
   DB_QueryINSERT ("can not store comment content",
		   "INSERT INTO tl_comments"
		   " (PubCod,Txt,MedCod)"
		   " VALUES"
		   " (%ld,'%s',%ld)",
		   PubCod,
		   Content->Txt,
		   Content->Media.MedCod);
  }

/*****************************************************************************/
/**************** Get code of media associated to comment ********************/
/*****************************************************************************/

long TL_DB_GetMedCodFromComment (long PubCod)
  {
   return TL_DB_GetMedCodFromPub (PubCod,"tl_comments");
  }

/*****************************************************************************/
/****************** Remove favs for comment from database ********************/
/*****************************************************************************/

void TL_DB_RemoveCommentFavs (long PubCod)
  {
   /***** Remove favs for comment *****/
   DB_QueryDELETE ("can not remove favs for comment",
		   "DELETE FROM tl_comments_fav"
		   " WHERE PubCod=%ld",
		   PubCod);
  }

/*****************************************************************************/
/***************** Remove content of comment from database *******************/
/*****************************************************************************/

void TL_DB_RemoveCommentContent (long PubCod)
  {
   /***** Remove content of comment *****/
   DB_QueryDELETE ("can not remove comment content",
		   "DELETE FROM tl_comments"
		   " WHERE PubCod=%ld",
		   PubCod);
  }

/*****************************************************************************/
/***************** Remove comment publication from database ******************/
/*****************************************************************************/

void TL_DB_RemoveCommentPub (long PubCod)
  {
   /***** Remove comment publication *****/
   DB_QueryDELETE ("can not remove comment",
		   "DELETE FROM tl_pubs"
	           " WHERE PubCod=%ld"
	           " AND PublisherCod=%ld"	// Extra check: author
	           " AND PubType=%u",		// Extra check: it's a comment
		   PubCod,
		   Gbl.Usrs.Me.UsrDat.UsrCod,
		   (unsigned) TL_Pub_COMMENT_TO_NOTE);
  }

/*****************************************************************************/
/*************** Get code of media associated to post/comment ****************/
/*****************************************************************************/

static long TL_DB_GetMedCodFromPub (long PubCod,const char *DBTable)
  {
   MYSQL_RES *mysql_res;
   MYSQL_ROW row;
   long MedCod = -1L;	// Default value

   /***** Get code of media associated to comment *****/
   if (DB_QuerySELECT (&mysql_res,"can not get media code",
		       "SELECT MedCod"	// row[0]
		       " FROM %s"
		       " WHERE PubCod=%ld",
		       DBTable,PubCod) == 1)   // Result should have a unique row
     {
      /* Get media code */
      row = mysql_fetch_row (mysql_res);
      MedCod = Str_ConvertStrCodToLongCod (row[0]);
     }

   /* Free structure that stores the query result */
   DB_FreeMySQLResult (&mysql_res);

   return MedCod;
  }

/*****************************************************************************/
/******* Create temporary table and subquery with potential publishers *******/
/*****************************************************************************/

void TL_DB_CreateSubQueryPublishers (const struct TL_Timeline *Timeline,
                                     struct TL_Pub_SubQueries *SubQueries)
  {
   /***** Create temporary table and subquery with potential publishers *****/
   switch (Timeline->UsrOrGbl)
     {
      case TL_Usr_TIMELINE_USR:		// Show the timeline of a user
	 SubQueries->TablePublishers = "";
	 sprintf (SubQueries->Publishers,"tl_pubs.PublisherCod=%ld AND ",
	          Gbl.Usrs.Other.UsrDat.UsrCod);
	 break;
      case TL_Usr_TIMELINE_GBL:		// Show the global timeline
	 switch (Timeline->Who)
	   {
	    case Usr_WHO_ME:		// Show my timeline
	       SubQueries->TablePublishers = "";
	       snprintf (SubQueries->Publishers,sizeof (SubQueries->Publishers),
	                 "tl_pubs.PublisherCod=%ld AND ",
	                 Gbl.Usrs.Me.UsrDat.UsrCod);
               break;
	    case Usr_WHO_FOLLOWED:	// Show the timeline of the users I follow
	       Fol_CreateTmpTableMeAndUsrsIFollow ();
	       SubQueries->TablePublishers = ",fol_tmp_me_and_followed";
	       Str_Copy (SubQueries->Publishers,
			 "tl_pubs.PublisherCod=fol_tmp_me_and_followed.UsrCod AND ",
			 sizeof (SubQueries->Publishers) - 1);
	       break;
	    case Usr_WHO_ALL:		// Show the timeline of all users
	       SubQueries->TablePublishers = "";
	       SubQueries->Publishers[0] = '\0';
	       break;
	    default:
	       Lay_WrongWhoExit ();
	       break;
	   }
	 break;
     }
  }

/*****************************************************************************/
/********* Create subquery to get only notes not present in timeline *********/
/*****************************************************************************/

void TL_DB_CreateSubQueryAlreadyExists (const struct TL_Timeline *Timeline,
                                        struct TL_Pub_SubQueries *SubQueries)
  {
   switch (Timeline->WhatToGet)
     {
      case TL_GET_RECENT_TIMELINE:
      case TL_GET_ONLY_NEW_PUBS:
	 Str_Copy (SubQueries->AlreadyExists,
		   " tl_pubs.NotCod NOT IN"
		   " (SELECT NotCod FROM tl_tmp_just_retrieved_notes)",	// Avoid notes just retrieved
		   sizeof (SubQueries->AlreadyExists) - 1);
         break;
      case TL_GET_ONLY_OLD_PUBS:	// Get only old publications
	 Str_Copy (SubQueries->AlreadyExists,
		   " tl_pubs.NotCod NOT IN"
		   " (SELECT NotCod FROM tl_tmp_visible_timeline)",	// Avoid notes already shown
		   sizeof (SubQueries->AlreadyExists) - 1);
	 break;
     }
  }

/*****************************************************************************/
/***** Create subqueries with range of publications to get from tl_pubs ******/
/*****************************************************************************/

void TL_DB_CreateSubQueryRangeBottom (const struct TL_Pub_RangePubsToGet *RangePubsToGet,
                                      struct TL_Pub_SubQueries *SubQueries)
  {
   if (RangePubsToGet->Bottom > 0)
      sprintf (SubQueries->RangeBottom,"tl_pubs.PubCod>%ld AND ",
	       RangePubsToGet->Bottom);
   else
      SubQueries->RangeBottom[0] = '\0';
  }

void TL_DB_CreateSubQueryRangeTop (const struct TL_Pub_RangePubsToGet *RangePubsToGet,
                                   struct TL_Pub_SubQueries *SubQueries)
  {
   if (RangePubsToGet->Top > 0)
      sprintf (SubQueries->RangeTop,"tl_pubs.PubCod<%ld AND ",
	       RangePubsToGet->Top);
   else
      SubQueries->RangeTop[0] = '\0';
  }

/*****************************************************************************/
/******************** Select the most recent publication *********************/
/*****************************************************************************/
// Returns the number of rows got

unsigned TL_DB_SelectTheMostRecentPub (const struct TL_Pub_SubQueries *SubQueries,
                                       MYSQL_RES **mysql_res)
  {
   return (unsigned)
   DB_QuerySELECT (mysql_res,"can not get publication",
		   "SELECT tl_pubs.PubCod,"		// row[0]
			  "tl_pubs.NotCod,"		// row[1]
			  "tl_pubs.PublisherCod,"	// row[2]
			  "tl_pubs.PubType"		// row[3]
		   " FROM tl_pubs%s"
		   " WHERE %s%s%s%s"
		   " ORDER BY tl_pubs.PubCod DESC LIMIT 1",
		   SubQueries->TablePublishers,
		   SubQueries->RangeBottom,
		   SubQueries->RangeTop,
		   SubQueries->Publishers,
		   SubQueries->AlreadyExists);
  }

/*****************************************************************************/
/*********************** Get code of note of a publication *******************/
/*****************************************************************************/

long TL_DB_GetNotCodFromPubCod (long PubCod)
  {
   MYSQL_RES *mysql_res;
   MYSQL_ROW row;
   long NotCod = -1L;	// Default value

   /***** Get code of note from database *****/
   if (DB_QuerySELECT (&mysql_res,"can not get code of note",
		       "SELECT NotCod FROM tl_pubs"
		       " WHERE PubCod=%ld",
		       PubCod) == 1)   // Result should have a unique row
     {
      /* Get code of note */
      row = mysql_fetch_row (mysql_res);
      NotCod = Str_ConvertStrCodToLongCod (row[0]);
     }

   /***** Free structure that stores the query result *****/
   DB_FreeMySQLResult (&mysql_res);

   return NotCod;
  }

/*****************************************************************************/
/************* Get last/first publication code stored in session *************/
/*****************************************************************************/
// FieldName can be:
// "LastPubCod"
// "FirstPubCod"

long TL_DB_GetPubCodFromSession (const char *FieldName)
  {
   MYSQL_RES *mysql_res;
   MYSQL_ROW row;
   long PubCod;

   /***** Get last publication code from database *****/
   if (DB_QuerySELECT (&mysql_res,"can not get publication code from session",
		       "SELECT %s FROM sessions"
		       " WHERE SessionId='%s'",
		       FieldName,Gbl.Session.Id) == 1)
     {
      /***** Get last publication code *****/
      row = mysql_fetch_row (mysql_res);
      if (sscanf (row[0],"%ld",&PubCod) != 1)
	 PubCod = 0;
     }
   else
      PubCod = 0;

   /***** Free structure that stores the query result *****/
   DB_FreeMySQLResult (&mysql_res);

   return PubCod;
  }

/*****************************************************************************/
/****************** Get number of publications from a user *******************/
/*****************************************************************************/

unsigned long TL_DB_GetNumPubsUsr (long UsrCod)
  {
   /***** Get number of posts from a user from database *****/
   return DB_QueryCOUNT ("can not get number of publications from a user",
			 "SELECT COUNT(*) FROM tl_pubs"
			 " WHERE PublisherCod=%ld",
			 UsrCod);
  }

/*****************************************************************************/
/********************* Insert new publication in database ********************/
/*****************************************************************************/
// Return just created publication code

long TL_DB_CreateNewPub (const struct TL_Pub_Publication *Pub)
  {
   /***** Insert new publication in database *****/
   return
   DB_QueryINSERTandReturnCode ("can not publish note/comment",
				"INSERT INTO tl_pubs"
				" (NotCod,PublisherCod,PubType,TimePublish)"
				" VALUES"
				" (%ld,%ld,%u,NOW())",
				Pub->NotCod,
				Pub->PublisherCod,
				(unsigned) Pub->PubType);
  }

/*****************************************************************************/
/************** Update first publication code stored in session **************/
/*****************************************************************************/

void TL_DB_UpdateFirstPubCodInSession (long FirstPubCod)
  {
   DB_QueryUPDATE ("can not update first publication code into session",
		   "UPDATE sessions"
		   " SET FirstPubCod=%ld"
		   " WHERE SessionId='%s'",
		   FirstPubCod,
		   Gbl.Session.Id);
  }

/*****************************************************************************/
/*************** Update last publication code stored in session **************/
/*****************************************************************************/

void TL_DB_UpdateLastPubCodInSession (void)
  {
   DB_QueryUPDATE ("can not update last publication code into session",
		   "UPDATE sessions"
		   " SET LastPubCod="
			"(SELECT IFNULL(MAX(PubCod),0)"
			" FROM tl_pubs)"	// The most recent publication
		   " WHERE SessionId='%s'",
		   Gbl.Session.Id);
  }

/*****************************************************************************/
/********* Update first and last publication codes stored in session *********/
/*****************************************************************************/

void TL_DB_UpdateFirstLastPubCodsInSession (long FirstPubCod)
  {
   DB_QueryUPDATE ("can not update first/last publication codes into session",
		   "UPDATE sessions"
		   " SET FirstPubCod=%ld,"
			"LastPubCod="
			"(SELECT IFNULL(MAX(PubCod),0)"
			" FROM tl_pubs)"	// The most recent publication
		   " WHERE SessionId='%s'",
		   FirstPubCod,
		   Gbl.Session.Id);
  }


/*****************************************************************************/
/****************** Check if a user has favourited a note ********************/
/*****************************************************************************/

bool TL_DB_CheckIfNoteIsFavedByUsr (long NotCod,long UsrCod)
  {
   return (DB_QueryCOUNT ("can not check if a user"
			  " has favourited a note",
			  "SELECT COUNT(*) FROM tl_notes_fav"
			  " WHERE NotCod=%ld AND UsrCod=%ld",
			  NotCod,UsrCod) != 0);
  }

/*****************************************************************************/
/**************** Check if a user has favourited a comment *******************/
/*****************************************************************************/

bool TL_DB_CheckIfCommIsFavedByUsr (long PubCod,long UsrCod)
  {
   return (DB_QueryCOUNT ("can not check if a user"
			  " has favourited a comment",
			  "SELECT COUNT(*) FROM tl_comments_fav"
			  " WHERE PubCod=%ld AND UsrCod=%ld",
			  PubCod,UsrCod) != 0);
  }

/*****************************************************************************/
/********************** Mark note as favourite in database *******************/
/*****************************************************************************/

void TL_DB_MarkNoteAsFav (long NotCod)
  {
   /***** Insert note in favourited in database *****/
   DB_QueryINSERT ("can not favourite note",
		   "INSERT IGNORE INTO tl_notes_fav"
		   " (NotCod,UsrCod,TimeFav)"
		   " VALUES"
		   " (%ld,%ld,NOW())",
		   NotCod,
		   Gbl.Usrs.Me.UsrDat.UsrCod);
  }

/*****************************************************************************/
/********************** Mark note as favourite in database *******************/
/*****************************************************************************/

void TL_DB_MarkCommAsFav (long PubCod)
  {
   /***** Insert comment in favourited in database *****/
   DB_QueryINSERT ("can not favourite comment",
		   "INSERT IGNORE INTO tl_comments_fav"
		   " (PubCod,UsrCod,TimeFav)"
		   " VALUES"
		   " (%ld,%ld,NOW())",
		   PubCod,
		   Gbl.Usrs.Me.UsrDat.UsrCod);
  }

/*****************************************************************************/
/********************* Unmark note as favourite in database ******************/
/*****************************************************************************/

void TL_DB_UnmarkNoteAsFav (long NotCod)
  {
   /***** Delete the mark as favourite from database *****/
   DB_QueryDELETE ("can not unfavourite note",
		   "DELETE FROM tl_notes_fav"
		   " WHERE NotCod=%ld AND UsrCod=%ld",
		   NotCod,
		   Gbl.Usrs.Me.UsrDat.UsrCod);
  }

/*****************************************************************************/
/******************* Unmark comment as favourite in database *****************/
/*****************************************************************************/

void TL_DB_UnmarkCommAsFav (long PubCod)
  {
   DB_QueryDELETE ("can not unfavourite comment",
		   "DELETE FROM tl_comments_fav"
		   " WHERE PubCod=%ld AND UsrCod=%ld",
		   PubCod,
		   Gbl.Usrs.Me.UsrDat.UsrCod);
  }
