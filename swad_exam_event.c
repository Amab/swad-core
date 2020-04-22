// swad_exam_event.c: exam events (each ocurrence of an exam)

/*
    SWAD (Shared Workspace At a Distance),
    is a web platform developed at the University of Granada (Spain),
    and used to support university teaching.

    This file is part of SWAD core.
    Copyright (C) 1999-2020 Antonio Ca�as Vargas

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
#include <linux/limits.h>	// For PATH_MAX
#include <stddef.h>		// For NULL
#include <stdio.h>		// For asprintf
#include <stdlib.h>		// For calloc
#include <string.h>		// For string functions

#include "swad_database.h"
#include "swad_date.h"
#include "swad_exam.h"
#include "swad_exam_event.h"
#include "swad_exam_result.h"
#include "swad_form.h"
#include "swad_global.h"
#include "swad_HTML.h"
#include "swad_role.h"
#include "swad_setting.h"
#include "swad_test.h"

/*****************************************************************************/
/************** External global variables from others modules ****************/
/*****************************************************************************/

extern struct Globals Gbl;

/*****************************************************************************/
/***************************** Private constants *****************************/
/*****************************************************************************/

#define ExaEvt_ICON_CLOSE		"fas fa-times"
#define ExaEvt_ICON_PLAY		"fas fa-play"
#define ExaEvt_ICON_PAUSE		"fas fa-pause"
#define ExaEvt_ICON_PREVIOUS	"fas fa-step-backward"
#define ExaEvt_ICON_NEXT		"fas fa-step-forward"
#define ExaEvt_ICON_RESULTS	"fas fa-chart-bar"

#define ExaEvt_COUNTDOWN_SECONDS_LARGE  60
#define ExaEvt_COUNTDOWN_SECONDS_MEDIUM 30
#define ExaEvt_COUNTDOWN_SECONDS_SMALL  10

/*****************************************************************************/
/******************************* Private types *******************************/
/*****************************************************************************/

typedef enum
  {
   ExaEvt_CHANGE_STATUS_BY_STUDENT,
   ExaEvt_REFRESH_STATUS_BY_SERVER,
  } ExaEvt_Update_t;

/*****************************************************************************/
/***************************** Private constants *****************************/
/*****************************************************************************/

const char *ExaEvt_ShowingStringsDB[ExaEvt_NUM_SHOWING] =
  {
   [ExaEvt_START  ] = "start",
   [ExaEvt_STEM   ] = "stem",
   [ExaEvt_ANSWERS] = "answers",
   [ExaEvt_RESULTS] = "results",
   [ExaEvt_END    ] = "end",
  };

#define ExaEvt_MAX_COLS 4
#define ExaEvt_NUM_COLS_DEFAULT 1

/*****************************************************************************/
/***************************** Private variables *****************************/
/*****************************************************************************/

long ExaEvt_EvtCodBeingPlayed;	// Used to refresh exam via AJAX

/*****************************************************************************/
/***************************** Private prototypes ****************************/
/*****************************************************************************/

static void ExaEvt_SetEvtCodBeingPlayed (long EvtCod);

static void ExaEvt_PutIconsInListOfEvents (void *Exams);
static void ExaEvt_PutIconToCreateNewEvent (struct Exa_Exams *Exams);

static void ExaEvt_ListOneOrMoreEvents (struct Exa_Exams *Exams,
                                        const struct Exa_Exam *Exam,
				        unsigned NumEvents,
                                        MYSQL_RES *mysql_res);
static void ExaEvt_ListOneOrMoreEventsHeading (bool ICanEditEvents);
static bool ExaEvt_CheckIfICanEditEvents (void);
static bool ExaEvt_CheckIfICanEditThisEvent (const struct ExaEvt_Event *Event);
static void ExaEvt_ListOneOrMoreEventsIcons (struct Exa_Exams *Exams,
                                             const struct ExaEvt_Event *Event);
static void ExaEvt_ListOneOrMoreEventsAuthor (const struct ExaEvt_Event *Event);
static void ExaEvt_ListOneOrMoreEventsTimes (const struct ExaEvt_Event *Event,unsigned UniqueId);
static void ExaEvt_ListOneOrMoreEventsTitleGrps (const struct ExaEvt_Event *Event);
static void ExaEvt_GetAndWriteNamesOfGrpsAssociatedToEvent (const struct ExaEvt_Event *Event);
static void ExaEvt_ListOneOrMoreEventsNumPlayers (const struct ExaEvt_Event *Event);
static void ExaEvt_ListOneOrMoreEventsStatus (struct ExaEvt_Event *Event,unsigned NumQsts);
static void ExaEvt_ListOneOrMoreEventsResult (struct Exa_Exams *Exams,
                                              const struct ExaEvt_Event *Event);
static void ExaEvt_ListOneOrMoreEventsResultStd (struct Exa_Exams *Exams,
                                                 const struct ExaEvt_Event *Event);
static void ExaEvt_ListOneOrMoreEventsResultTch (struct Exa_Exams *Exams,
                                                 const struct ExaEvt_Event *Event);

static void ExaEvt_GetEventDataFromRow (MYSQL_RES *mysql_res,
				        struct ExaEvt_Event *Event);
static ExaEvt_Showing_t ExaEvt_GetShowingFromStr (const char *Str);

static void ExaEvt_RemoveEventFromAllTables (long EvtCod);
static void ExaEvt_RemoveEventFromTable (long EvtCod,const char *TableName);
static void ExaEvt_RemoveEventsInExamFromTable (long ExaCod,const char *TableName);
static void ExaEvt_RemoveEventInCourseFromTable (long CrsCod,const char *TableName);
static void ExaEvt_RemoveUsrEvtResultsInCrs (long UsrCod,long CrsCod,const char *TableName);

static void ExaEvt_PutParamsPlay (void *EvtCod);
static void ExaEvt_PutParamEvtCod (long EvtCod);

static void ExaEvt_PutFormNewEvent (const struct Exa_Exam *Exam);
static void ExaEvt_ShowLstGrpsToCreateEvent (void);

static long ExaEvt_CreateEvent (long ExaCod,char Title[Exa_MAX_BYTES_TITLE + 1]);
static void ExaEvt_CreateIndexes (long ExaCod,long EvtCod);
static void ExaEvt_ReorderAnswer (long EvtCod,unsigned QstInd,
			          const struct Tst_Question *Question);
static void ExaEvt_CreateGrps (long EvtCod);
static void ExaEvt_UpdateEventStatusInDB (const struct ExaEvt_Event *Event);

static void ExaEvt_UpdateElapsedTimeInQuestion (const struct ExaEvt_Event *Event);
static void ExaEvt_GetElapsedTimeInQuestion (const struct ExaEvt_Event *Event,
				          struct Time *Time);
static void ExaEvt_GetElapsedTimeInMatch (const struct ExaEvt_Event *Event,
				       struct Time *Time);
static void ExaEvt_GetElapsedTime (unsigned NumRows,MYSQL_RES *mysql_res,
				   struct Time *Time);

static void ExaEvt_SetMatchStatusToPrev (struct ExaEvt_Event *Event);
static void ExaEvt_SetMatchStatusToPrevQst (struct ExaEvt_Event *Event);
static void ExaEvt_SetMatchStatusToStart (struct ExaEvt_Event *Event);

static void ExaEvt_SetMatchStatusToNext (struct ExaEvt_Event *Event);
static void ExaEvt_SetMatchStatusToNextQst (struct ExaEvt_Event *Event);
static void ExaEvt_SetMatchStatusToEnd (struct ExaEvt_Event *Event);

static void ExaEvt_ShowMatchStatusForTch (struct ExaEvt_Event *Event);
static void ExaEvt_ShowMatchStatusForStd (struct ExaEvt_Event *Event,ExaEvt_Update_t Update);

static void ExaEvt_ShowLeftColumnTch (struct ExaEvt_Event *Event);
static void ExaEvt_ShowRefreshablePartTch (struct ExaEvt_Event *Event);
static void ExaEvt_WriteElapsedTimeInEvt (struct ExaEvt_Event *Event);
static void ExaEvt_WriteElapsedTimeInQst (struct ExaEvt_Event *Event);
static void ExaEvt_WriteNumRespondersQst (struct ExaEvt_Event *Event);
static void ExaEvt_PutFormCountdown (struct ExaEvt_Event *Event,long Seconds,const char *Color);
static void ExaEvt_PutCountdownAndHourglassIcon (struct ExaEvt_Event *Event);
static void ExaEvt_PutFormsCountdown (struct ExaEvt_Event *Event);

static void ExaEvt_ShowRightColumnTch (const struct ExaEvt_Event *Event);
static void ExaEvt_ShowLeftColumnStd (const struct ExaEvt_Event *Event,
				      const struct ExaEvt_UsrAnswer *UsrAnswer);
static void ExaEvt_ShowRightColumnStd (struct ExaEvt_Event *Event,
				       const struct ExaEvt_UsrAnswer *UsrAnswer,
				       ExaEvt_Update_t Update);

static void ExaEvt_ShowNumQstInEvt (const struct ExaEvt_Event *Event);
static void ExaEvt_PutMatchControlButtons (const struct ExaEvt_Event *Event);
static void ExaEvt_ShowFormColumns (const struct ExaEvt_Event *Event);
static void ExaEvt_PutParamNumCols (unsigned NumCols);

static void ExaEvt_ShowEventTitleTch (const struct ExaEvt_Event *Event);
static void ExaEvt_ShowEventTitleStd (const struct ExaEvt_Event *Event);

static void ExaEvt_PutCheckboxResult (const struct ExaEvt_Event *Event);
static void ExaEvt_PutIfAnswered (const struct ExaEvt_Event *Event,bool Answered);
static void ExaEvt_PutIconToRemoveMyAnswer (const struct ExaEvt_Event *Event);
static void ExaEvt_ShowQuestionAndAnswersTch (const struct ExaEvt_Event *Event);
static void ExaEvt_WriteAnswersEventResult (const struct ExaEvt_Event *Event,
                                            const struct Tst_Question *Question,
                                            const char *Class,bool ShowResult);
static bool ExaEvt_ShowQuestionAndAnswersStd (const struct ExaEvt_Event *Event,
					      const struct ExaEvt_UsrAnswer *UsrAnswer,
					      ExaEvt_Update_t Update);

static void ExaEvt_ShowEventScore (const struct ExaEvt_Event *Event);
static void ExaEvt_DrawEmptyScoreRow (unsigned NumRow,double MinScore,double MaxScore);
static void ExaEvt_DrawScoreRow (double Score,double MinScore,double MaxScore,
			      unsigned NumRow,unsigned NumUsrs,unsigned MaxUsrs);
static const char *ExaEvt_GetClassBorder (unsigned NumRow);

static void ExaEvt_PutParamNumOpt (unsigned NumOpt);
static unsigned ExaEvt_GetParamNumOpt (void);

static void ExaEvt_PutBigButton (Act_Action_t NextAction,const char *Id,
			         long EvtCod,const char *Icon,const char *Txt);
static void ExaEvt_PutBigButtonHidden (const char *Icon);
static void ExaEvt_PutBigButtonClose (void);

static void ExaEvt_ShowWaitImage (const char *Txt);

static void ExaEvt_RemoveOldPlayers (void);
static void ExaEvt_UpdateEventAsBeingPlayed (long EvtCod);
static void ExaEvt_SetEventAsNotBeingPlayed (long EvtCod);
static bool ExaEvt_GetIfEventIsBeingPlayed (long EvtCod);
static void ExaEvt_GetNumPlayers (struct ExaEvt_Event *Event);

static void ExaEvt_RemoveMyAnswerToEventQuestion (const struct ExaEvt_Event *Event);

static void ExaEvt_ComputeScore (struct TstRes_Result *Result);

static unsigned ExaEvt_GetNumUsrsWhoHaveAnswerEvt (long EvtCod);

/*****************************************************************************/
/********** Set/Get exam event code of the exam event being played ***********/
/*****************************************************************************/

static void ExaEvt_SetEvtCodBeingPlayed (long EvtCod)
  {
   ExaEvt_EvtCodBeingPlayed = EvtCod;
  }

long ExaEvt_GetEvtCodBeingPlayed (void)
  {
   return ExaEvt_EvtCodBeingPlayed;
  }

/*****************************************************************************/
/************************* List the events of an exam ************************/
/*****************************************************************************/

void ExaEvt_ListEvents (struct Exa_Exams *Exams,
                        struct Exa_Exam *Exam,
                        bool PutFormNewEvent)
  {
   extern const char *Hlp_ASSESSMENT_Games_matches;
   extern const char *Txt_Matches;
   char *SubQuery;
   MYSQL_RES *mysql_res;
   unsigned NumEvents;

   /***** Get data of events from database *****/
   /* Fill subquery for exam */
   if (Gbl.Crs.Grps.WhichGrps == Grp_MY_GROUPS)
     {
      if (asprintf (&SubQuery," AND"
			      "(EvtCod NOT IN"
			      " (SELECT EvtCod FROM exa_groups)"
			      " OR"
			      " EvtCod IN"
			      " (SELECT exa_groups.EvtCod"
			      " FROM exa_groups,crs_grp_usr"
			      " WHERE crs_grp_usr.UsrCod=%ld"
			      " AND exa_groups.GrpCod=crs_grp_usr.GrpCod))",
		     Gbl.Usrs.Me.UsrDat.UsrCod) < 0)
	  Lay_NotEnoughMemoryExit ();
      }
    else	// Gbl.Crs.Grps.WhichGrps == Grp_ALL_GROUPS
       if (asprintf (&SubQuery,"%s","") < 0)
	  Lay_NotEnoughMemoryExit ();

   /* Make query */
   NumEvents = (unsigned) DB_QuerySELECT (&mysql_res,"can not get events",
					   "SELECT EvtCod,"				// row[ 0]
						  "ExaCod,"				// row[ 1]
						  "UsrCod,"				// row[ 2]
						  "UNIX_TIMESTAMP(StartTime),"		// row[ 3]
						  "UNIX_TIMESTAMP(EndTime),"		// row[ 4]
						  "Title,"				// row[ 5]
						  "QstInd,"				// row[ 6]
						  "QstCod,"				// row[ 7]
						  "Showing,"				// row[ 8]
						  "Countdown,"				// row[ 9]
						  "NumCols,"				// row[10]
					          "ShowQstResults,"			// row[11]
					          "ShowUsrResults"			// row[12]
					   " FROM exa_events"
					   " WHERE ExaCod=%ld%s"
					   " ORDER BY EvtCod",
					   Exam->ExaCod,
					   SubQuery);

   /* Free allocated memory for subquery */
   free (SubQuery);

   /***** Begin box *****/
   Exams->ExaCod = Exam->ExaCod;
   Box_BoxBegin ("100%",Txt_Matches,
                 ExaEvt_PutIconsInListOfEvents,Exams,
                 Hlp_ASSESSMENT_Games_matches,Box_NOT_CLOSABLE);

   /***** Select whether show only my groups or all groups *****/
   if (Gbl.Crs.Grps.NumGrps)
     {
      Set_StartSettingsHead ();
      Grp_ShowFormToSelWhichGrps (ActSeeGam,
                                  Gam_PutParams,Exams);
      Set_EndSettingsHead ();
     }

   if (NumEvents)
      /***** Show the table with the events *****/
      ExaEvt_ListOneOrMoreEvents (Exams,Exam,NumEvents,mysql_res);

   /***** Free structure that stores the query result *****/
   DB_FreeMySQLResult (&mysql_res);

   /***** Put button to play a new exam event in this exam *****/
   switch (Gbl.Usrs.Me.Role.Logged)
     {
      case Rol_NET:
      case Rol_TCH:
      case Rol_SYS_ADM:
	 if (PutFormNewEvent)
	    ExaEvt_PutFormNewEvent (Exam);			// Form to fill in data and start playing a new exam event
	 else
	    Exa_PutButtonNewEvent (Exams,Exam->ExaCod);	// Button to create a new exam event
	 break;
      default:
	 break;
     }

   /***** End box *****/
   Box_BoxEnd ();
  }

/*****************************************************************************/
/******************** Get exam event data using its code *********************/
/*****************************************************************************/

void ExaEvt_GetDataOfEventByCod (struct ExaEvt_Event *Event)
  {
   MYSQL_RES *mysql_res;
   unsigned long NumRows;
   Dat_StartEndTime_t StartEndTime;

   /***** Get data of exam event from database *****/
   NumRows = (unsigned) DB_QuerySELECT (&mysql_res,"can not get events",
					"SELECT EvtCod,"			// row[ 0]
					       "ExaCod,"			// row[ 1]
					       "UsrCod,"			// row[ 2]
					       "UNIX_TIMESTAMP(StartTime),"	// row[ 3]
					       "UNIX_TIMESTAMP(EndTime),"	// row[ 4]
					       "Title,"				// row[ 5]
					       "QstInd,"			// row[ 6]
					       "QstCod,"			// row[ 7]
					       "Showing,"			// row[ 8]
					       "Countdown,"			// row[ 9]
					       "NumCols,"			// row[10]
					       "ShowQstResults,"		// row[11]
					       "ShowUsrResults"			// row[12]
					" FROM exa_events"
					" WHERE EvtCod=%ld"
					" AND ExaCod IN"		// Extra check
					" (SELECT ExaCod FROM exa_exams"
					" WHERE CrsCod='%ld')",
					Event->EvtCod,
					Gbl.Hierarchy.Crs.CrsCod);
   if (NumRows) // Event found...
      /***** Get exam event data from row *****/
      ExaEvt_GetEventDataFromRow (mysql_res,Event);
   else
     {
      /* Initialize to empty exam event */
      Event->EvtCod                  = -1L;
      Event->ExaCod                  = -1L;
      Event->UsrCod                  = -1L;
      for (StartEndTime  = (Dat_StartEndTime_t) 0;
	   StartEndTime <= (Dat_StartEndTime_t) (Dat_NUM_START_END_TIME - 1);
	   StartEndTime++)
         Event->TimeUTC[StartEndTime] = (time_t) 0;
      Event->Title[0]                = '\0';
      Event->Status.QstInd           = 0;
      Event->Status.QstCod           = -1L;
      Event->Status.QstStartTimeUTC  = (time_t) 0;
      Event->Status.Showing          = ExaEvt_START;
      Event->Status.Countdown        = -1L;
      Event->Status.Playing          = false;
      Event->Status.NumPlayers       = 0;
      Event->Status.NumCols          = ExaEvt_NUM_COLS_DEFAULT;
      Event->Status.ShowQstResults   = false;
      Event->Status.ShowUsrResults   = false;
     }

   /***** Free structure that stores the query result *****/
   DB_FreeMySQLResult (&mysql_res);
  }

/*****************************************************************************/
/****************** Put icons in list of events of an exam *******************/
/*****************************************************************************/

static void ExaEvt_PutIconsInListOfEvents (void *Exams)
  {
   bool ICanEditEvents;

   if (Exams)
     {
      /***** Put icon to create a new exam event in current exam *****/
      ICanEditEvents = ExaEvt_CheckIfICanEditEvents ();
      if (ICanEditEvents)
	 ExaEvt_PutIconToCreateNewEvent ((struct Exa_Exams *) Exams);
     }
  }

/*****************************************************************************/
/******************* Put icon to create a new exam event *********************/
/*****************************************************************************/

static void ExaEvt_PutIconToCreateNewEvent (struct Exa_Exams *Exams)
  {
   extern const char *Txt_New_match;

   /***** Put form to create a new exam event *****/
   Ico_PutContextualIconToAdd (ActReqNewExaEvt,ExaEvt_NEW_EVENT_SECTION_ID,
                               Gam_PutParams,Exams,
			       Txt_New_match);
  }

/*****************************************************************************/
/*********************** List exam events for edition ************************/
/*****************************************************************************/

static void ExaEvt_ListOneOrMoreEvents (struct Exa_Exams *Exams,
                                        const struct Exa_Exam *Exam,
				        unsigned NumEvents,
                                        MYSQL_RES *mysql_res)
  {
   unsigned NumEvent;
   unsigned UniqueId;
   struct ExaEvt_Event Event;
   bool ICanEditEvents = ExaEvt_CheckIfICanEditEvents ();

   /***** Write the heading *****/
   HTM_TABLE_BeginWidePadding (2);
   ExaEvt_ListOneOrMoreEventsHeading (ICanEditEvents);

   /***** Write rows *****/
   for (NumEvent = 0, UniqueId = 1;
	NumEvent < NumEvents;
	NumEvent++, UniqueId++)
     {
      Gbl.RowEvenOdd = NumEvent % 2;

      /***** Get exam event data from row *****/
      ExaEvt_GetEventDataFromRow (mysql_res,&Event);

      if (ExaEvt_CheckIfICanPlayThisEventBasedOnGrps (&Event))
	{
	 /***** Write row for this exam event ****/
	 HTM_TR_Begin (NULL);

	 /* Icons */
	 if (ICanEditEvents)
	    ExaEvt_ListOneOrMoreEventsIcons (Exams,&Event);

	 /* Event player */
	 ExaEvt_ListOneOrMoreEventsAuthor (&Event);

	 /* Start/end date/time */
	 ExaEvt_ListOneOrMoreEventsTimes (&Event,UniqueId);

	 /* Title and groups */
	 ExaEvt_ListOneOrMoreEventsTitleGrps (&Event);

	 /* Number of players who have answered any question in the exam event */
	 ExaEvt_ListOneOrMoreEventsNumPlayers (&Event);

	 /* Event status */
	 ExaEvt_ListOneOrMoreEventsStatus (&Event,Exam->NumQsts);

	 /* Event result visible? */
	 ExaEvt_ListOneOrMoreEventsResult (Exams,&Event);
	}
     }

   /***** End table *****/
   HTM_TABLE_End ();
  }

/*****************************************************************************/
/************** Put a column for exam event start and end times **************/
/*****************************************************************************/

static void ExaEvt_ListOneOrMoreEventsHeading (bool ICanEditEvents)
  {
   extern const char *Txt_ROLES_SINGUL_Abc[Rol_NUM_ROLES][Usr_NUM_SEXS];
   extern const char *Txt_START_END_TIME[Dat_NUM_START_END_TIME];
   extern const char *Txt_Match;
   extern const char *Txt_Players;
   extern const char *Txt_Status;
   extern const char *Txt_Results;

   /***** Start row *****/
   HTM_TR_Begin (NULL);

   /***** Column for icons *****/
   if (ICanEditEvents)
      HTM_TH_Empty (1);

   /***** The rest of columns *****/
   HTM_TH (1,1,"LT",Txt_ROLES_SINGUL_Abc[Rol_TCH][Usr_SEX_UNKNOWN]);
   HTM_TH (1,1,"LT",Txt_START_END_TIME[Gam_ORDER_BY_START_DATE]);
   HTM_TH (1,1,"LT",Txt_START_END_TIME[Gam_ORDER_BY_END_DATE  ]);
   HTM_TH (1,1,"LT",Txt_Match);
   HTM_TH (1,1,"RT",Txt_Players);
   HTM_TH (1,1,"CT",Txt_Status);
   HTM_TH (1,1,"CT",Txt_Results);

   /***** End row *****/
   HTM_TR_End ();
  }

/*****************************************************************************/
/*********************** Check if I can edit events **************************/
/*****************************************************************************/

static bool ExaEvt_CheckIfICanEditEvents (void)
  {
   switch (Gbl.Usrs.Me.Role.Logged)
     {
      case Rol_NET:
      case Rol_TCH:
      case Rol_SYS_ADM:
         return true;
      default:
         return false;
     }
  }

/*****************************************************************************/
/************** Check if I can edit (remove/resume) an exam event ************/
/*****************************************************************************/

static bool ExaEvt_CheckIfICanEditThisEvent (const struct ExaEvt_Event *Event)
  {
   switch (Gbl.Usrs.Me.Role.Logged)
     {
      case Rol_NET:
	 return (Event->UsrCod == Gbl.Usrs.Me.UsrDat.UsrCod);	// Only if I am the creator
      case Rol_TCH:
      case Rol_SYS_ADM:
	 return true;
      default:
	 return false;
     }
  }

/*****************************************************************************/
/************************* Put a column for icons ****************************/
/*****************************************************************************/

static void ExaEvt_ListOneOrMoreEventsIcons (struct Exa_Exams *Exams,
                                             const struct ExaEvt_Event *Event)
  {
   HTM_TD_Begin ("class=\"BT%u\"",Gbl.RowEvenOdd);

   /***** Put icon to remove the exam event *****/
   if (ExaEvt_CheckIfICanEditThisEvent (Event))
     {
      Exams->ExaCod = Event->ExaCod;
      Exams->EvtCod = Event->EvtCod;
      Frm_StartForm (ActReqRemExaEvt);
      ExaEvt_PutParamsEdit (Exams);
      Ico_PutIconRemove ();
      Frm_EndForm ();
     }
   else
      Ico_PutIconRemovalNotAllowed ();

   HTM_TD_End ();
  }

/*****************************************************************************/
/*********** Put a column for teacher who created the exam event *************/
/*****************************************************************************/

static void ExaEvt_ListOneOrMoreEventsAuthor (const struct ExaEvt_Event *Event)
  {
   /***** Event author (teacher) *****/
   HTM_TD_Begin ("class=\"LT COLOR%u\"",Gbl.RowEvenOdd);
   Usr_WriteAuthor1Line (Event->UsrCod,false);
   HTM_TD_End ();
  }

/*****************************************************************************/
/*************** Put a column for exam event start and end times *************/
/*****************************************************************************/

static void ExaEvt_ListOneOrMoreEventsTimes (const struct ExaEvt_Event *Event,unsigned UniqueId)
  {
   Dat_StartEndTime_t StartEndTime;
   char *Id;

   for (StartEndTime  = (Dat_StartEndTime_t) 0;
	StartEndTime <= (Dat_StartEndTime_t) (Dat_NUM_START_END_TIME - 1);
	StartEndTime++)
     {
      if (asprintf (&Id,"exa_time_%u_%u",(unsigned) StartEndTime,UniqueId) < 0)
	 Lay_NotEnoughMemoryExit ();
      HTM_TD_Begin ("id=\"%s\" class=\"%s LT COLOR%u\"",
		    Id,
		    Event->Status.Showing == ExaEvt_END ? "DATE_RED" :
						          "DATE_GREEN",
		    Gbl.RowEvenOdd);
      Dat_WriteLocalDateHMSFromUTC (Id,Event->TimeUTC[StartEndTime],
				    Gbl.Prefs.DateFormat,Dat_SEPARATOR_BREAK,
				    true,true,true,0x7);
      HTM_TD_End ();
      free (Id);
     }
  }

/*****************************************************************************/
/*************** Put a column for exam event title and grous *****************/
/*****************************************************************************/

static void ExaEvt_ListOneOrMoreEventsTitleGrps (const struct ExaEvt_Event *Event)
  {
   extern const char *Txt_Play;
   extern const char *Txt_Resume;

   HTM_TD_Begin ("class=\"LT COLOR%u\"",Gbl.RowEvenOdd);

   /***** Event title *****/
   Frm_StartForm (Gbl.Usrs.Me.Role.Logged == Rol_STD ? ActJoiExaEvt :
						       ActResExaEvt);
   ExaEvt_PutParamEvtCod (Event->EvtCod);
   HTM_BUTTON_SUBMIT_Begin (Gbl.Usrs.Me.Role.Logged == Rol_STD ? Txt_Play :
								 Txt_Resume,
			    "BT_LINK LT ASG_TITLE",NULL);
   HTM_Txt (Event->Title);
   HTM_BUTTON_End ();
   Frm_EndForm ();

   /***** Groups whose students can answer this exam event *****/
   if (Gbl.Crs.Grps.NumGrps)
      ExaEvt_GetAndWriteNamesOfGrpsAssociatedToEvent (Event);

   HTM_TD_End ();
  }

/*****************************************************************************/
/********** Get and write the names of the groups of an exam event ***********/
/*****************************************************************************/

static void ExaEvt_GetAndWriteNamesOfGrpsAssociatedToEvent (const struct ExaEvt_Event *Event)
  {
   extern const char *Txt_Group;
   extern const char *Txt_Groups;
   extern const char *Txt_and;
   extern const char *Txt_The_whole_course;
   MYSQL_RES *mysql_res;
   MYSQL_ROW row;
   unsigned long NumRow;
   unsigned long NumRows;

   /***** Get groups associated to an exam event from database *****/
   NumRows = DB_QuerySELECT (&mysql_res,"can not get groups of an exam event",
			     "SELECT crs_grp_types.GrpTypName,crs_grp.GrpName"
			     " FROM exa_groups,crs_grp,crs_grp_types"
			     " WHERE exa_groups.EvtCod=%ld"
			     " AND exa_groups.GrpCod=crs_grp.GrpCod"
			     " AND crs_grp.GrpTypCod=crs_grp_types.GrpTypCod"
			     " ORDER BY crs_grp_types.GrpTypName,crs_grp.GrpName",
			     Event->EvtCod);

   /***** Write heading *****/
   HTM_DIV_Begin ("class=\"ASG_GRP\"");
   HTM_TxtColonNBSP (NumRows == 1 ? Txt_Group  :
                                    Txt_Groups);

   /***** Write groups *****/
   if (NumRows) // Groups found...
     {
      /* Get and write the group types and names */
      for (NumRow = 0;
	   NumRow < NumRows;
	   NumRow++)
        {
         /* Get next group */
         row = mysql_fetch_row (mysql_res);

         /* Write group type name and group name */
         HTM_TxtF ("%s&nbsp;%s",row[0],row[1]);

         if (NumRows >= 2)
           {
            if (NumRow == NumRows-2)
               HTM_TxtF (" %s ",Txt_and);
            if (NumRows >= 3)
              if (NumRow < NumRows-2)
                  HTM_Txt (", ");
           }
        }
     }
   else
      HTM_TxtF ("%s&nbsp;%s",Txt_The_whole_course,Gbl.Hierarchy.Crs.ShrtName);

   HTM_DIV_End ();

   /***** Free structure that stores the query result *****/
   DB_FreeMySQLResult (&mysql_res);
  }

/*****************************************************************************/
/********** Check if an exam event is associated to a given group ************/
/*****************************************************************************/

bool ExaEvt_CheckIfMatchIsAssociatedToGrp (long EvtCod,long GrpCod)
  {
   /***** Get if an exam event is associated to a group from database *****/
   return (DB_QueryCOUNT ("can not check if an exam event is associated to a group",
			  "SELECT COUNT(*) FROM exa_groups"
			  " WHERE EvtCod=%ld AND GrpCod=%ld",
			  EvtCod,GrpCod) != 0);
  }

/*****************************************************************************/
/******************* Put a column for number of players **********************/
/*****************************************************************************/

static void ExaEvt_ListOneOrMoreEventsNumPlayers (const struct ExaEvt_Event *Event)
  {
   /***** Number of players who have answered any question in the exam event ******/
   HTM_TD_Begin ("class=\"DAT RT COLOR%u\"",Gbl.RowEvenOdd);
   HTM_Unsigned (ExaEvt_GetNumUsrsWhoHaveAnswerEvt (Event->EvtCod));
   HTM_TD_End ();
  }

/*****************************************************************************/
/******************** Put a column for exam event status *********************/
/*****************************************************************************/

static void ExaEvt_ListOneOrMoreEventsStatus (struct ExaEvt_Event *Event,unsigned NumQsts)
  {
   extern const char *Txt_Play;
   extern const char *Txt_Resume;

   HTM_TD_Begin ("class=\"DAT CT COLOR%u\"",Gbl.RowEvenOdd);

   if (Event->Status.Showing != ExaEvt_END)	// Event not over
     {
      /* Current question index / total of questions */
      HTM_DIV_Begin ("class=\"DAT\"");
      HTM_TxtF ("%u/%u",Event->Status.QstInd,NumQsts);
      HTM_DIV_End ();
     }

   /* Icon to join exam event or resume exam event */
   Lay_PutContextualLinkOnlyIcon (Gbl.Usrs.Me.Role.Logged == Rol_STD ? ActJoiExaEvt :
								       ActResExaEvt,
				  NULL,
				  ExaEvt_PutParamsPlay,&Event->EvtCod,
				  Event->Status.Showing == ExaEvt_END ? "flag-checkered.svg" :
					                                "play.svg",
				  Gbl.Usrs.Me.Role.Logged == Rol_STD ? Txt_Play :
								       Txt_Resume);

   HTM_TD_End ();
  }

/*****************************************************************************/
/************* Put a column for visibility of exam event result **************/
/*****************************************************************************/

static void ExaEvt_ListOneOrMoreEventsResult (struct Exa_Exams *Exams,
                                              const struct ExaEvt_Event *Event)
  {
   HTM_TD_Begin ("class=\"DAT CT COLOR%u\"",Gbl.RowEvenOdd);

   switch (Gbl.Usrs.Me.Role.Logged)
     {
      case Rol_STD:
	 ExaEvt_ListOneOrMoreEventsResultStd (Exams,Event);
	 break;
      case Rol_NET:
      case Rol_TCH:
      case Rol_SYS_ADM:
	 ExaEvt_ListOneOrMoreEventsResultTch (Exams,Event);
	 break;
      default:
	 Rol_WrongRoleExit ();
	 break;
     }

   HTM_TD_End ();
  }

static void ExaEvt_ListOneOrMoreEventsResultStd (struct Exa_Exams *Exams,
                                                 const struct ExaEvt_Event *Event)
  {
   extern const char *Txt_Results;

   /***** Is exam event result visible or hidden? *****/
   if (Event->Status.ShowUsrResults)
     {
      /* Result is visible by me */
      Exams->ExaCod = Event->ExaCod;
      Exams->EvtCod = Event->EvtCod;
      Lay_PutContextualLinkOnlyIcon (ActSeeMyExaEvtResEvt,ExaRes_RESULTS_BOX_ID,
				     ExaEvt_PutParamsEdit,Exams,
				     "trophy.svg",
				     Txt_Results);
     }
   else
      /* Result is forbidden to me */
      Ico_PutIconNotVisible ();
  }

static void ExaEvt_ListOneOrMoreEventsResultTch (struct Exa_Exams *Exams,
                                                 const struct ExaEvt_Event *Event)
  {
   extern const char *Txt_Visible_results;
   extern const char *Txt_Hidden_results;
   extern const char *Txt_Results;

   /***** Can I edit exam event vivibility? *****/
   if (ExaEvt_CheckIfICanEditThisEvent (Event))
     {
      Exams->ExaCod = Event->ExaCod;
      Exams->EvtCod = Event->EvtCod;

      /* Show exam event results */
      Lay_PutContextualLinkOnlyIcon (ActSeeAllExaEvtResEvt,ExaRes_RESULTS_BOX_ID,
				     ExaEvt_PutParamsEdit,Exams,
				     "trophy.svg",
				     Txt_Results);

      /* I can edit visibility */
      Lay_PutContextualLinkOnlyIcon (ActChgVisResExaEvtUsr,NULL,
				     ExaEvt_PutParamsEdit,Exams,
				     Event->Status.ShowUsrResults ? "eye-green.svg" :
								    "eye-slash-red.svg",
				     Event->Status.ShowUsrResults ? Txt_Visible_results :
								    Txt_Hidden_results);
     }
   else
      /* I can not edit visibility */
      Ico_PutIconOff (Event->Status.ShowUsrResults ? "eye-green.svg" :
						     "eye-slash-red.svg",
		      Event->Status.ShowUsrResults ? Txt_Visible_results :
						     Txt_Hidden_results);
  }

/*****************************************************************************/
/****************** Toggle visibility of exam event results ******************/
/*****************************************************************************/

void ExaEvt_ToggleVisibilResultsEvtUsr (void)
  {
   struct Exa_Exams Exams;
   struct Exa_Exam Exam;
   struct ExaEvt_Event Event;

   /***** Reset exams *****/
   Exa_ResetExams (&Exams);

   /***** Get and check parameters *****/
   ExaEvt_GetAndCheckParameters (&Exams,&Exam,&Event);

   /***** Check if I have permission to change visibility *****/
   if (!ExaEvt_CheckIfICanEditThisEvent (&Event))
      Lay_NoPermissionExit ();

   /***** Toggle visibility of exam event results *****/
   Event.Status.ShowUsrResults = !Event.Status.ShowUsrResults;
   DB_QueryUPDATE ("can not toggle visibility of exam event results",
		   "UPDATE exa_events"
		   " SET ShowUsrResults='%c'"
		   " WHERE EvtCod=%ld",
		   Event.Status.ShowUsrResults ? 'Y' :
			                         'N',
		   Event.EvtCod);

   /***** Show current exam *****/
   Exa_ShowOnlyOneExam (&Exams,&Exam,
                        false,	// Do not list exam questions
	                false);	// Do not put form to start new exam event
  }

/*****************************************************************************/
/******************** Get exam data from a database row **********************/
/*****************************************************************************/

static void ExaEvt_GetEventDataFromRow (MYSQL_RES *mysql_res,
				        struct ExaEvt_Event *Event)
  {
   MYSQL_ROW row;
   Dat_StartEndTime_t StartEndTime;
   long LongNum;

   /***** Get exam event data *****/
   row = mysql_fetch_row (mysql_res);
   /*
   row[ 0]	EvtCod
   row[ 1]	ExaCod
   row[ 2]	UsrCod
   row[ 3]	UNIX_TIMESTAMP(StartTime)
   row[ 4]	UNIX_TIMESTAMP(EndTime)
   row[ 5]	Title
   */
   /***** Get exam event data *****/
   /* Code of the exam event (row[0]) */
   if ((Event->EvtCod = Str_ConvertStrCodToLongCod (row[0])) <= 0)
      Lay_ShowErrorAndExit ("Wrong code of exam event.");

   /* Code of the exam (row[1]) */
   if ((Event->ExaCod = Str_ConvertStrCodToLongCod (row[1])) <= 0)
      Lay_ShowErrorAndExit ("Wrong code of exam.");

   /* Get exam event teacher (row[2]) */
   Event->UsrCod = Str_ConvertStrCodToLongCod (row[2]);

   /* Get start/end times (row[3], row[4] hold start/end UTC times) */
   for (StartEndTime  = (Dat_StartEndTime_t) 0;
	StartEndTime <= (Dat_StartEndTime_t) (Dat_NUM_START_END_TIME - 1);
	StartEndTime++)
      Event->TimeUTC[StartEndTime] = Dat_GetUNIXTimeFromStr (row[3 + StartEndTime]);

   /* Get the title of the exam (row[5]) */
   if (row[5])
      Str_Copy (Event->Title,row[5],
		Exa_MAX_BYTES_TITLE);
   else
      Event->Title[0] = '\0';

   /***** Get current exam event status *****/
   /*
   row[ 6]	QstInd
   row[ 7]	QstCod
   row[ 8]	Showing
   row[ 9]	Countdown
   row[10]	NumCols
   row[11]	ShowQstResults
   row[12]	ShowUsrResults
   */
   /* Current question index (row[6]) */
   Event->Status.QstInd = Str_ConvertStrToUnsigned (row[6]);

   /* Current question code (row[7]) */
   Event->Status.QstCod = Str_ConvertStrCodToLongCod (row[7]);

   /* Get what to show (stem, answers, results) (row(8)) */
   Event->Status.Showing = ExaEvt_GetShowingFromStr (row[8]);

   /* Get countdown (row[9]) */
   Event->Status.Countdown = Str_ConvertStrCodToLongCod (row[9]);

   /* Get number of columns (row[10]) */
   LongNum = Str_ConvertStrCodToLongCod (row[10]);
   Event->Status.NumCols =  (LongNum <= 1           ) ? 1 :
                           ((LongNum >= ExaEvt_MAX_COLS) ? ExaEvt_MAX_COLS :
                        	                        (unsigned) LongNum);

   /* Get whether to show question results or not (row(11)) */
   Event->Status.ShowQstResults = (row[11][0] == 'Y');

   /* Get whether to show user results or not (row(12)) */
   Event->Status.ShowUsrResults = (row[12][0] == 'Y');

   /***** Get whether the exam event is being played or not *****/
   if (Event->Status.Showing == ExaEvt_END)	// Event over
      Event->Status.Playing = false;
   else						// Event not over
      Event->Status.Playing = ExaEvt_GetIfEventIsBeingPlayed (Event->EvtCod);
  }

/*****************************************************************************/
/****************** Get parameter with what is being shown *******************/
/*****************************************************************************/

static ExaEvt_Showing_t ExaEvt_GetShowingFromStr (const char *Str)
  {
   ExaEvt_Showing_t Showing;

   for (Showing  = (ExaEvt_Showing_t) 0;
	Showing <= (ExaEvt_Showing_t) (ExaEvt_NUM_SHOWING - 1);
	Showing++)
      if (!strcmp (Str,ExaEvt_ShowingStringsDB[Showing]))
         return Showing;

   return (ExaEvt_Showing_t) ExaEvt_SHOWING_DEFAULT;
  }

/*****************************************************************************/
/*********** Request the removal of an exam event (exam instance) ************/
/*****************************************************************************/

void ExaEvt_RequestRemoveEvent (void)
  {
   extern const char *Txt_Do_you_really_want_to_remove_the_event_X;
   extern const char *Txt_Remove_event;
   struct Exa_Exams Exams;
   struct Exa_Exam Exam;
   struct ExaEvt_Event Event;

   /***** Reset exams *****/
   Exa_ResetExams (&Exams);

   /***** Get and check parameters *****/
   ExaEvt_GetAndCheckParameters (&Exams,&Exam,&Event);

   /***** Show question and button to remove question *****/
   Exams.ExaCod = Event.ExaCod;
   Exams.EvtCod = Event.EvtCod;
   Ale_ShowAlertAndButton (ActRemExaEvt,NULL,NULL,
                           ExaEvt_PutParamsEdit,&Exams,
			   Btn_REMOVE_BUTTON,Txt_Remove_event,
			   Ale_QUESTION,Txt_Do_you_really_want_to_remove_the_event_X,
	                   Event.Title);

   /***** Show current exam *****/
   Exa_ShowOnlyOneExam (&Exams,&Exam,
                        false,	// Do not list exam questions
	                false);	// Do not put form to start new exam event
  }

/*****************************************************************************/
/******************* Remove an exam event (exam instance) ********************/
/*****************************************************************************/

void ExaEvt_RemoveEvent (void)
  {
   extern const char *Txt_Match_X_removed;
   struct Exa_Exams Exams;
   struct Exa_Exam Exam;
   struct ExaEvt_Event Event;

   /***** Reset exams *****/
   Exa_ResetExams (&Exams);

   /***** Get and check parameters *****/
   ExaEvt_GetAndCheckParameters (&Exams,&Exam,&Event);

   /***** Check if I can remove this exam event *****/
   if (!ExaEvt_CheckIfICanEditThisEvent (&Event))
      Lay_NoPermissionExit ();

   /***** Remove the exam event from all database tables *****/
   ExaEvt_RemoveEventFromAllTables (Event.EvtCod);

   /***** Write message *****/
   Ale_ShowAlert (Ale_SUCCESS,Txt_Match_X_removed,
		  Event.Title);

   /***** Show current exam *****/
   Exa_ShowOnlyOneExam (&Exams,&Exam,
                        false,	// Do not list exam questions
	                false);	// Do not put form to start new exam event
  }

/*****************************************************************************/
/******************** Remove exam event from all tables **********************/
/*****************************************************************************/
/*
mysql> SELECT table_name FROM information_schema.tables WHERE table_name LIKE 'exa%';
*/
static void ExaEvt_RemoveEventFromAllTables (long EvtCod)
  {
   /***** Remove exam event from secondary tables *****/
   ExaEvt_RemoveEventFromTable (EvtCod,"exa_players");
   ExaEvt_RemoveEventFromTable (EvtCod,"exa_playing");
   ExaEvt_RemoveEventFromTable (EvtCod,"exa_results");
   ExaEvt_RemoveEventFromTable (EvtCod,"exa_answers");
   ExaEvt_RemoveEventFromTable (EvtCod,"exa_times");
   ExaEvt_RemoveEventFromTable (EvtCod,"exa_groups");
   ExaEvt_RemoveEventFromTable (EvtCod,"exa_indexes");

   /***** Remove exam event from main table *****/
   DB_QueryDELETE ("can not remove exam event",
		   "DELETE FROM exa_events WHERE EvtCod=%ld",
		   EvtCod);
  }

static void ExaEvt_RemoveEventFromTable (long EvtCod,const char *TableName)
  {
   /***** Remove exam event from secondary table *****/
   DB_QueryDELETE ("can not remove exam event from table",
		   "DELETE FROM %s WHERE EvtCod=%ld",
		   TableName,
		   EvtCod);
  }

/*****************************************************************************/
/****************** Remove exam event in exam from all tables ****************/
/*****************************************************************************/

void ExaEvt_RemoveEventsInExamFromAllTables (long ExaCod)
  {
   /***** Remove events from secondary tables *****/
   ExaEvt_RemoveEventsInExamFromTable (ExaCod,"exa_players");
   ExaEvt_RemoveEventsInExamFromTable (ExaCod,"exa_playing");
   ExaEvt_RemoveEventsInExamFromTable (ExaCod,"exa_results");
   ExaEvt_RemoveEventsInExamFromTable (ExaCod,"exa_answers");
   ExaEvt_RemoveEventsInExamFromTable (ExaCod,"exa_times");
   ExaEvt_RemoveEventsInExamFromTable (ExaCod,"exa_groups");
   ExaEvt_RemoveEventsInExamFromTable (ExaCod,"exa_indexes");

   /***** Remove events from main table *****/
   DB_QueryDELETE ("can not remove events of an exam",
		   "DELETE FROM exa_events WHERE ExaCod=%ld",
		   ExaCod);
  }

static void ExaEvt_RemoveEventsInExamFromTable (long ExaCod,const char *TableName)
  {
   /***** Remove events in exam from secondary table *****/
   DB_QueryDELETE ("can not remove events of an exam from table",
		   "DELETE FROM %s"
		   " USING exa_events,%s"
		   " WHERE exa_events.ExaCod=%ld"
		   " AND exa_events.EvtCod=%s.EvtCod",
		   TableName,
		   TableName,
		   ExaCod,
		   TableName);
  }

/*****************************************************************************/
/***************** Remove exam event in course from all tables ***************/
/*****************************************************************************/

void ExaEvt_RemoveEventInCourseFromAllTables (long CrsCod)
  {
   /***** Remove events from secondary tables *****/
   ExaEvt_RemoveEventInCourseFromTable (CrsCod,"exa_players");
   ExaEvt_RemoveEventInCourseFromTable (CrsCod,"exa_playing");
   ExaEvt_RemoveEventInCourseFromTable (CrsCod,"exa_results");
   ExaEvt_RemoveEventInCourseFromTable (CrsCod,"exa_answers");
   ExaEvt_RemoveEventInCourseFromTable (CrsCod,"exa_times");
   ExaEvt_RemoveEventInCourseFromTable (CrsCod,"exa_groups");
   ExaEvt_RemoveEventInCourseFromTable (CrsCod,"exa_indexes");

   /***** Remove events from main table *****/
   DB_QueryDELETE ("can not remove events of a course",
		   "DELETE FROM exa_events"
		   " USING exa_exams,exa_events"
		   " WHERE exa_exams.CrsCod=%ld"
		   " AND exa_exams.ExaCod=exa_events.ExaCod",
		   CrsCod);
  }

static void ExaEvt_RemoveEventInCourseFromTable (long CrsCod,const char *TableName)
  {
   /***** Remove events in course from secondary table *****/
   DB_QueryDELETE ("can not remove events of a course from table",
		   "DELETE FROM %s"
		   " USING exa_exams,exa_events,%s"
		   " WHERE exa_exams.CrsCod=%ld"
		   " AND exa_exams.ExaCod=exa_events.ExaCod"
		   " AND exa_events.EvtCod=%s.EvtCod",
		   TableName,
		   TableName,
		   CrsCod,
		   TableName);
  }

/*****************************************************************************/
/************** Remove user from secondary exam event tables *****************/
/*****************************************************************************/

void ExaEvt_RemoveUsrFromEventTablesInCrs (long UsrCod,long CrsCod)
  {
   /***** Remove student from secondary tables *****/
   ExaEvt_RemoveUsrEvtResultsInCrs (UsrCod,CrsCod,"exa_players");
   ExaEvt_RemoveUsrEvtResultsInCrs (UsrCod,CrsCod,"exa_results");
   ExaEvt_RemoveUsrEvtResultsInCrs (UsrCod,CrsCod,"exa_answers");
  }

static void ExaEvt_RemoveUsrEvtResultsInCrs (long UsrCod,long CrsCod,const char *TableName)
  {
   /***** Remove events in course from secondary table *****/
   DB_QueryDELETE ("can not remove events of a user from table",
		   "DELETE FROM %s"
		   " USING exa_exams,exa_events,%s"
		   " WHERE exa_exams.CrsCod=%ld"
		   " AND exa_exams.ExaCod=exa_events.ExaCod"
		   " AND exa_events.EvtCod=%s.EvtCod"
		   " AND %s.UsrCod=%ld",
		   TableName,
		   TableName,
		   CrsCod,
		   TableName,
		   TableName,
		   UsrCod);
  }

/*****************************************************************************/
/******************** Params used to edit an exam event **********************/
/*****************************************************************************/

void ExaEvt_PutParamsEdit (void *Exams)
  {
   if (Exams)
     {
      Gam_PutParams (Exams);
      ExaEvt_PutParamEvtCod (((struct Exa_Exams *) Exams)->EvtCod);
     }
  }

/*****************************************************************************/
/********************* Params used to edit an exam event *********************/
/*****************************************************************************/

static void ExaEvt_PutParamsPlay (void *EvtCod)
  {
   if (EvtCod)
     {
      if (*((long *) EvtCod) > 0)
	 ExaEvt_PutParamEvtCod (*((long *) EvtCod));
     }
  }

/*****************************************************************************/
/***************** Write parameter with code of exam event *******************/
/*****************************************************************************/

static void ExaEvt_PutParamEvtCod (long EvtCod)
  {
   Par_PutHiddenParamLong (NULL,"EvtCod",EvtCod);
  }

/*****************************************************************************/
/************************** Get and check parameters *************************/
/*****************************************************************************/

void ExaEvt_GetAndCheckParameters (struct Exa_Exams *Exams,
                                   struct Exa_Exam *Exam,
                                   struct ExaEvt_Event *Event)
  {
   /***** Get parameters *****/
   /* Get parameters of exam */
   if ((Exam->ExaCod = Exa_GetParams (Exams)) <= 0)
      Lay_ShowErrorAndExit ("Code of exam is missing.");
   Grp_GetParamWhichGroups ();
   Exa_GetDataOfExamByCod (Exam);

   /* Get exam event code */
   if ((Event->EvtCod = ExaEvt_GetParamEvtCod ()) <= 0)
      Lay_ShowErrorAndExit ("Code of exam event is missing.");
   ExaEvt_GetDataOfEventByCod (Event);

   /***** Ensure parameters are correct *****/
   if (Exam->ExaCod != Event->ExaCod)
      Lay_ShowErrorAndExit ("Wrong exam code.");
   if (Exam->CrsCod != Gbl.Hierarchy.Crs.CrsCod)
      Lay_ShowErrorAndExit ("Event does not belong to this course.");
  }

/*****************************************************************************/
/***************** Get parameter with code of exam event *********************/
/*****************************************************************************/

long ExaEvt_GetParamEvtCod (void)
  {
   /***** Get code of exam event *****/
   return Par_GetParToLong ("EvtCod");
  }

/*****************************************************************************/
/* Put a big button to play exam event (start a new exam event) as a teacher */
/*****************************************************************************/

static void ExaEvt_PutFormNewEvent (const struct Exa_Exam *Exam)
  {
   extern const char *Hlp_ASSESSMENT_Games_matches;
   extern const char *Txt_New_match;
   extern const char *Txt_Title;
   extern const char *Txt_Play;

   /***** Start section for a new exam event *****/
   HTM_SECTION_Begin (ExaEvt_NEW_EVENT_SECTION_ID);

   /***** Begin form *****/
   Frm_StartForm (ActNewExaEvt);
   Gam_PutParamGameCod (Exam->ExaCod);
   Gam_PutParamQstInd (0);	// Start by first question in exam

   /***** Begin box and table *****/
   Box_BoxTableBegin (NULL,Txt_New_match,
                      NULL,NULL,
		      Hlp_ASSESSMENT_Games_matches,Box_NOT_CLOSABLE,2);

   /***** Event title *****/
   HTM_TR_Begin (NULL);

   /* Label */
   Frm_LabelColumn ("RT","Title",Txt_Title);

   /* Data */
   HTM_TD_Begin ("class=\"LT\"");
   HTM_INPUT_TEXT ("Title",Gam_MAX_CHARS_TITLE,Exam->Title,false,
		   "id=\"Title\" size=\"45\" required=\"required\"");
   HTM_TD_End ();

   HTM_TR_End ();

   /***** Groups *****/
   ExaEvt_ShowLstGrpsToCreateEvent ();

   /***** End table *****/
   HTM_TABLE_End ();

   /***** Put icon to submit the form *****/
   HTM_INPUT_IMAGE (Cfg_URL_ICON_PUBLIC,"play.svg",
		    Txt_Play,"CONTEXT_OPT ICO_HIGHLIGHT ICO64x64");

   /***** End box *****/
   Box_BoxEnd ();

   /***** End form *****/
   Frm_EndForm ();

   /***** End section for a new exam event *****/
   HTM_SECTION_End ();
  }

/*****************************************************************************/
/************** Show list of groups to create a new exam event ***************/
/*****************************************************************************/

static void ExaEvt_ShowLstGrpsToCreateEvent (void)
  {
   extern const char *The_ClassFormInBox[The_NUM_THEMES];
   extern const char *Txt_Groups;
   extern const char *Txt_The_whole_course;
   unsigned NumGrpTyp;

   /***** Get list of groups types and groups in this course *****/
   Grp_GetListGrpTypesAndGrpsInThisCrs (Grp_ONLY_GROUP_TYPES_WITH_GROUPS);

   if (Gbl.Crs.Grps.GrpTypes.Num)
     {
      /***** Begin box and table *****/
      HTM_TR_Begin (NULL);

      HTM_TD_Begin ("class=\"%s RT\"",The_ClassFormInBox[Gbl.Prefs.Theme]);
      HTM_TxtF ("%s:",Txt_Groups);
      HTM_TD_End ();

      HTM_TD_Begin ("class=\"LT\"");
      Box_BoxTableBegin ("95%",NULL,
                         NULL,NULL,
                         NULL,Box_NOT_CLOSABLE,0);

      /***** First row: checkbox to select the whole course *****/
      HTM_TR_Begin (NULL);

      HTM_TD_Begin ("colspan=\"7\" class=\"DAT LM\"");
      HTM_LABEL_Begin (NULL);
      HTM_INPUT_CHECKBOX ("WholeCrs",HTM_SUBMIT_ON_CHANGE,
			  "id=\"WholeCrs\" value=\"Y\" checked=\"checked\""
			  " onclick=\"uncheckChildren(this,'GrpCods')\"");
      HTM_TxtF ("%s&nbsp;%s",Txt_The_whole_course,Gbl.Hierarchy.Crs.ShrtName);
      HTM_LABEL_End ();
      HTM_TD_End ();

      HTM_TR_End ();

      /***** List the groups for each group type *****/
      for (NumGrpTyp = 0;
	   NumGrpTyp < Gbl.Crs.Grps.GrpTypes.Num;
	   NumGrpTyp++)
         if (Gbl.Crs.Grps.GrpTypes.LstGrpTypes[NumGrpTyp].NumGrps)
            Grp_ListGrpsToEditAsgAttSvyMch (&Gbl.Crs.Grps.GrpTypes.LstGrpTypes[NumGrpTyp],
                                            -1L,	// -1 means "New exam event"
					    Grp_MATCH);

      /***** End table and box *****/
      Box_BoxTableEnd ();
      HTM_TD_End ();
      HTM_TR_End ();
     }

   /***** Free list of groups types and groups in this course *****/
   Grp_FreeListGrpTypesAndGrps ();
  }

/*****************************************************************************/
/******************* Create a new exam event (by a teacher) ******************/
/*****************************************************************************/

void ExaEvt_CreateNewEventTch (void)
  {
   long ExaCod;
   char Title[Exa_MAX_BYTES_TITLE + 1];

   /***** Get form parameters *****/
   /* Get exam event code */
   if ((ExaCod = Gam_GetParamGameCod ()) == -1L)
      Lay_ShowErrorAndExit ("Code of exam is missing.");

   /* Get exam event title */
   Par_GetParToText ("Title",Title,Exa_MAX_BYTES_TITLE);

   /* Get groups for this exams */
   Grp_GetParCodsSeveralGrps ();

   /***** Create a new exam event *****/
   ExaEvt_SetEvtCodBeingPlayed (ExaEvt_CreateEvent (ExaCod,Title));

   /***** Free memory for list of selected groups *****/
   Grp_FreeListCodSelectedGrps ();
  }

/*****************************************************************************/
/**** Show button to actually start / resume an exam event (by a teacher) ****/
/*****************************************************************************/

void ExaEvt_ResumeEvent (void)
  {
   struct ExaEvt_Event Event;

   /***** Remove old players.
          This function must be called by a teacher
          before getting exam event status. *****/
   ExaEvt_RemoveOldPlayers ();

   /***** Get data of the exam event from database *****/
   Event.EvtCod = ExaEvt_GetEvtCodBeingPlayed ();
   ExaEvt_GetDataOfEventByCod (&Event);

   /***** Check if I have permission to resume exam event *****/
   if (!ExaEvt_CheckIfICanEditThisEvent (&Event))
      Lay_NoPermissionExit ();

   /***** Update exam event status in database *****/
   ExaEvt_UpdateEventStatusInDB (&Event);

   /***** Show current exam event status *****/
   HTM_DIV_Begin ("id=\"exam event\" class=\"EXA_CONT\"");
   ExaEvt_ShowMatchStatusForTch (&Event);
   HTM_DIV_End ();
  }

/*****************************************************************************/
/******************* Create a new exam event in an exam **********************/
/*****************************************************************************/

static long ExaEvt_CreateEvent (long ExaCod,char Title[Exa_MAX_BYTES_TITLE + 1])
  {
   long EvtCod;

   /***** Insert this new exam event into database *****/
   EvtCod = DB_QueryINSERTandReturnCode ("can not create exam event",
				         "INSERT exa_events "
				         "(ExaCod,UsrCod,StartTime,EndTime,Title,"
				         "QstInd,QstCod,Showing,Countdown,"
				         "NumCols,ShowQstResults,ShowUsrResults)"
				         " VALUES "
				         "(%ld,"	// ExaCod
				         "%ld,"		// UsrCod
				         "NOW(),"	// StartTime
				         "NOW(),"	// EndTime
				         "'%s',"	// Title
				         "0,"		// QstInd: Event has not started, so not the first question yet
				         "-1,"		// QstCod: Non-existent question
				         "'%s',"	// Showing: What is being shown
					 "-1,"		// Countdown: No countdown
					 "%u,"		// NumCols: Number of columns in answers
				         "'N',"		// ShowQstResults: Don't show question results initially
				         "'N')",	// ShowUsrResults: Don't show user results initially
				         ExaCod,
				         Gbl.Usrs.Me.UsrDat.UsrCod,	// Exam creator
				         Title,
					 ExaEvt_ShowingStringsDB[ExaEvt_SHOWING_DEFAULT],
					 ExaEvt_NUM_COLS_DEFAULT);

   /***** Create indexes for answers *****/
   ExaEvt_CreateIndexes (ExaCod,EvtCod);

   /***** Create groups associated to the exam event *****/
   if (Gbl.Crs.Grps.LstGrpsSel.NumGrps)
      ExaEvt_CreateGrps (EvtCod);

   return EvtCod;
  }

/*****************************************************************************/
/******************** Create indexes for an exam event ***********************/
/*****************************************************************************/
/* Everytime a new exam event is created,
   the answers of each shufflable question are shuffled.
   The shuffling is stored in a table of indexes
   that will be read when showing an exam event */

static void ExaEvt_CreateIndexes (long ExaCod,long EvtCod)
  {
   MYSQL_RES *mysql_res;
   MYSQL_ROW row;
   unsigned NumQsts;
   unsigned NumQst;
   struct Tst_Question Question;
   long LongNum;
   unsigned QstInd;

   /***** Get questions of the exam *****/
   NumQsts = (unsigned)
	     DB_QuerySELECT (&mysql_res,"can not get questions of an exam",
			     "SELECT exa_questions.QstCod,"	// row[0]
			            "exa_questions.QstInd,"	// row[1]
			            "tst_questions.AnsType,"	// row[2]
			            "tst_questions.Shuffle"	// row[3]
			     " FROM exa_questions,tst_questions"
			     " WHERE exa_questions.ExaCod=%ld"
			     " AND exa_questions.QstCod=tst_questions.QstCod"
			     " ORDER BY exa_questions.QstInd",
			     ExaCod);

   /***** For each question in exam... *****/
   for (NumQst = 0;
	NumQst < NumQsts;
	NumQst++)
     {
      /***** Create test question *****/
      Tst_QstConstructor (&Question);

      /***** Get question data *****/
      row = mysql_fetch_row (mysql_res);
      /*
      exa_questions.QstCod	row[0]
      exa_questions.QstInd	row[1]
      tst_questions.AnsType	row[2]
      tst_questions.Shuffle	row[3]
      */

      /* Get question code (row[0]) */
      if ((Question.QstCod = Str_ConvertStrCodToLongCod (row[0])) < 0)
	 Lay_ShowErrorAndExit ("Wrong code of question.");

      /* Get question index (row[1]) */
      if ((LongNum = Str_ConvertStrCodToLongCod (row[1])) < 0)
	 Lay_ShowErrorAndExit ("Wrong question index.");
      QstInd = (unsigned) LongNum;

      /* Get answer type (row[2]) */
      Question.Answer.Type = Tst_ConvertFromStrAnsTypDBToAnsTyp (row[2]);
      if (Question.Answer.Type != Tst_ANS_UNIQUE_CHOICE)
	 Lay_ShowErrorAndExit ("Wrong answer type.");

      /* Get shuffle (row[3]) */
      Question.Answer.Shuffle = (row[3][0] == 'Y');

      /***** Reorder answer *****/
      ExaEvt_ReorderAnswer (EvtCod,QstInd,&Question);

      /***** Destroy test question *****/
      Tst_QstDestructor (&Question);
     }

   /***** Free structure that stores the query result *****/
   DB_FreeMySQLResult (&mysql_res);
  }

/*****************************************************************************/
/**************** Reorder answers of an exam event question ******************/
/*****************************************************************************/

static void ExaEvt_ReorderAnswer (long EvtCod,unsigned QstInd,
			          const struct Tst_Question *Question)
  {
   MYSQL_RES *mysql_res;
   MYSQL_ROW row;
   unsigned NumAnss;
   unsigned NumAns;
   long LongNum;
   unsigned AnsInd;
   char StrOneAnswer[Cns_MAX_DECIMAL_DIGITS_UINT + 1];
   char StrAnswersOneQst[TstRes_MAX_BYTES_ANSWERS_ONE_QST + 1];

   /***** Initialize list of answers to empty string *****/
   StrAnswersOneQst[0] = '\0';

   /***** Get questions of the exam *****/
   NumAnss = (unsigned)
	     DB_QuerySELECT (&mysql_res,"can not get questions of an exam",
			     "SELECT AnsInd"	// row[0]
			     " FROM tst_answers"
			     " WHERE QstCod=%ld"
			     " ORDER BY %s",
			     Question->QstCod,
			     Question->Answer.Shuffle ? "RAND()" :	// Use RAND() because is really random; RAND(NOW()) repeats order
				                        "AnsInd");

   /***** For each answer in question... *****/
   for (NumAns = 0;
	NumAns < NumAnss;
	NumAns++)
     {
      row = mysql_fetch_row (mysql_res);

      /* Get answer index (row[0]) */
      if ((LongNum = Str_ConvertStrCodToLongCod (row[0])) < 0)
	 Lay_ShowErrorAndExit ("Wrong answer index.");
      AnsInd = (unsigned) LongNum;
      snprintf (StrOneAnswer,sizeof (StrOneAnswer),
		"%u",AnsInd);

      /* Concatenate answer index to list of answers */
      if (NumAns)
         Str_Concat (StrAnswersOneQst,",",
		     TstRes_MAX_BYTES_ANSWERS_ONE_QST);
      Str_Concat (StrAnswersOneQst,StrOneAnswer,
		  TstRes_MAX_BYTES_ANSWERS_ONE_QST);
     }

   /***** Free structure that stores the query result *****/
   DB_FreeMySQLResult (&mysql_res);

   /***** Create entry for this question in table of exam event indexes *****/
   DB_QueryINSERT ("can not create exam event indexes",
		   "INSERT INTO exa_indexes"
		   " (EvtCod,QstInd,Indexes)"
		   " VALUES"
		   " (%ld,%u,'%s')",
		   EvtCod,QstInd,StrAnswersOneQst);
  }

/*****************************************************************************/
/***************** Get indexes for a question from database ******************/
/*****************************************************************************/

void ExaEvt_GetIndexes (long EvtCod,unsigned QstInd,
		        unsigned Indexes[Tst_MAX_OPTIONS_PER_QUESTION])
  {
   MYSQL_RES *mysql_res;
   MYSQL_ROW row;
   char StrIndexesOneQst[TstRes_MAX_BYTES_INDEXES_ONE_QST + 1];

   /***** Get indexes for a question from database *****/
   if (!DB_QuerySELECT (&mysql_res,"can not get data of a question",
			"SELECT Indexes"	// row[0]
			" FROM exa_indexes"
			" WHERE EvtCod=%ld AND QstInd=%u",
			EvtCod,QstInd))
      Lay_ShowErrorAndExit ("No indexes found for a question.");
   row = mysql_fetch_row (mysql_res);

   /* Get indexes (row[0]) */
   Str_Copy (StrIndexesOneQst,row[0],
	     TstRes_MAX_BYTES_INDEXES_ONE_QST);

   /***** Free structure that stores the query result *****/
   DB_FreeMySQLResult (&mysql_res);

   /***** Get indexes from string *****/
   Par_ReplaceCommaBySeparatorMultiple (StrIndexesOneQst);
   TstRes_GetIndexesFromStr (StrIndexesOneQst,Indexes);
  }

/*****************************************************************************/
/**************** Create groups associated to an exam event ******************/
/*****************************************************************************/

static void ExaEvt_CreateGrps (long EvtCod)
  {
   unsigned NumGrpSel;

   /***** Create groups associated to the exam event *****/
   for (NumGrpSel = 0;
	NumGrpSel < Gbl.Crs.Grps.LstGrpsSel.NumGrps;
	NumGrpSel++)
      /* Create group */
      DB_QueryINSERT ("can not associate a group to an exam event",
		      "INSERT INTO exa_groups"
		      " (EvtCod,GrpCod)"
		      " VALUES"
		      " (%ld,%ld)",
                      EvtCod,Gbl.Crs.Grps.LstGrpsSel.GrpCods[NumGrpSel]);
  }

/*****************************************************************************/
/********************* Remove one group from all events **********************/
/*****************************************************************************/

void ExaEvt_RemoveGroup (long GrpCod)
  {
   /***** Remove group from all the events *****/
   DB_QueryDELETE ("can not remove group"
	           " from the associations between events and groups",
		   "DELETE FROM exa_groups WHERE GrpCod=%ld",
		   GrpCod);
  }

/*****************************************************************************/
/***************** Remove groups of one type from all events *****************/
/*****************************************************************************/

void ExaEvt_RemoveGroupsOfType (long GrpTypCod)
  {
   /***** Remove group from all the events *****/
   DB_QueryDELETE ("can not remove groups of a type"
	           " from the associations between events and groups",
		   "DELETE FROM exa_groups"
		   " USING crs_grp,exa_groups"
		   " WHERE crs_grp.GrpTypCod=%ld"
		   " AND crs_grp.GrpCod=exa_groups.GrpCod",
                   GrpTypCod);
  }

/*****************************************************************************/
/************** Insert/update an exam exam event being played ****************/
/*****************************************************************************/

static void ExaEvt_UpdateEventStatusInDB (const struct ExaEvt_Event *Event)
  {
   char *EvtSubQuery;

   /***** Update end time only if exam event is currently being played *****/
   if (Event->Status.Playing)	// Event is being played
     {
      if (asprintf (&EvtSubQuery,"exa_events.EndTime=NOW(),") < 0)
         Lay_NotEnoughMemoryExit ();
     }
   else				// Event is paused, not being played
     {
      if (asprintf (&EvtSubQuery,"%s","") < 0)
         Lay_NotEnoughMemoryExit ();
     }

   /***** Update exam event status in database *****/
   DB_QueryUPDATE ("can not update exam event being played",
		   "UPDATE exa_events,exa_exams"
		   " SET %s"
			"exa_events.QstInd=%u,"
			"exa_events.QstCod=%ld,"
			"exa_events.Showing='%s',"
		        "exa_events.Countdown=%ld,"
		        "exa_events.NumCols=%u,"
			"exa_events.ShowQstResults='%c'"
		   " WHERE exa_events.EvtCod=%ld"
		   " AND exa_events.ExaCod=exa_exams.ExaCod"
		   " AND exa_exams.CrsCod=%ld",	// Extra check
		   EvtSubQuery,
		   Event->Status.QstInd,Event->Status.QstCod,
		   ExaEvt_ShowingStringsDB[Event->Status.Showing],
		   Event->Status.Countdown,
		   Event->Status.NumCols,
		   Event->Status.ShowQstResults ? 'Y' : 'N',
		   Event->EvtCod,Gbl.Hierarchy.Crs.CrsCod);
   free (EvtSubQuery);

   if (Event->Status.Playing)	// Event is being played
      /* Update exam event as being played */
      ExaEvt_UpdateEventAsBeingPlayed (Event->EvtCod);
   else				// Event is paused, not being played
      /* Update exam event as not being played */
      ExaEvt_SetEventAsNotBeingPlayed (Event->EvtCod);
  }

/*****************************************************************************/
/********** Update elapsed time in current question (by a teacher) ***********/
/*****************************************************************************/

static void ExaEvt_UpdateElapsedTimeInQuestion (const struct ExaEvt_Event *Event)
  {
   /***** Update elapsed time in current question in database *****/
   if (Event->Status.Playing &&		// Event is being played
       Event->Status.Showing != ExaEvt_START &&
       Event->Status.Showing != ExaEvt_END)
      DB_QueryINSERT ("can not update elapsed time in question",
		      "INSERT INTO exa_times (EvtCod,QstInd,ElapsedTime)"
		      " VALUES (%ld,%u,SEC_TO_TIME(%u))"
		      " ON DUPLICATE KEY"
		      " UPDATE ElapsedTime=ADDTIME(ElapsedTime,SEC_TO_TIME(%u))",
		      Event->EvtCod,Event->Status.QstInd,
		      Cfg_SECONDS_TO_REFRESH_MATCH_TCH,
		      Cfg_SECONDS_TO_REFRESH_MATCH_TCH);
  }

/*****************************************************************************/
/**************** Get elapsed time in an exam event question *****************/
/*****************************************************************************/

static void ExaEvt_GetElapsedTimeInQuestion (const struct ExaEvt_Event *Event,
					  struct Time *Time)
  {
   MYSQL_RES *mysql_res;
   unsigned NumRows;

   /***** Query database *****/
   NumRows = (unsigned) DB_QuerySELECT (&mysql_res,"can not get elapsed time",
				        "SELECT ElapsedTime"
				        " FROM exa_times"
				        " WHERE EvtCod=%ld AND QstInd=%u",
				        Event->EvtCod,Event->Status.QstInd);

   /***** Get elapsed time from query result *****/
   ExaEvt_GetElapsedTime (NumRows,mysql_res,Time);

   /***** Free structure that stores the query result *****/
   DB_FreeMySQLResult (&mysql_res);
  }

/*****************************************************************************/
/******************** Get elapsed time in an exam event **********************/
/*****************************************************************************/

static void ExaEvt_GetElapsedTimeInMatch (const struct ExaEvt_Event *Event,
				       struct Time *Time)
  {
   MYSQL_RES *mysql_res;
   unsigned NumRows;

   /***** Query database *****/
   NumRows = (unsigned) DB_QuerySELECT (&mysql_res,"can not get elapsed time",
				        "SELECT SEC_TO_TIME(SUM(TIME_TO_SEC(ElapsedTime)))"
				        " FROM exa_times WHERE EvtCod=%ld",
				        Event->EvtCod);

   /***** Get elapsed time from query result *****/
   ExaEvt_GetElapsedTime (NumRows,mysql_res,Time);

   /***** Free structure that stores the query result *****/
   DB_FreeMySQLResult (&mysql_res);
  }

/*****************************************************************************/
/******************** Get elapsed time in an exam event **********************/
/*****************************************************************************/

static void ExaEvt_GetElapsedTime (unsigned NumRows,MYSQL_RES *mysql_res,
				   struct Time *Time)
  {
   MYSQL_ROW row;
   bool ElapsedTimeGotFromDB = false;

   /***** Get time from H...H:MM:SS string *****/
   if (NumRows)
     {
      row = mysql_fetch_row (mysql_res);

      if (row[0])
	 /* Get the elapsed time (row[0]) */
	 if (sscanf (row[0],"%u:%02u:%02u",&Time->Hour,&Time->Minute,&Time->Second) == 3)
	    ElapsedTimeGotFromDB = true;
     }

   /***** Initialize time to default value (0) *****/
   if (!ElapsedTimeGotFromDB)
      Time->Hour   =
      Time->Minute =
      Time->Second = 0;
  }

/*****************************************************************************/
/******************* Play/pause exam event (by a teacher) ********************/
/*****************************************************************************/

void ExaEvt_PlayPauseEvent (void)
  {
   struct ExaEvt_Event Event;

   /***** Remove old players.
          This function must be called by a teacher
          before getting exam event status. *****/
   ExaEvt_RemoveOldPlayers ();

   /***** Get data of the exam event from database *****/
   Event.EvtCod = ExaEvt_GetEvtCodBeingPlayed ();
   ExaEvt_GetDataOfEventByCod (&Event);

   /***** Update status *****/
   if (Event.Status.Playing)	// Event is being played             ==> pause it
      Event.Status.Playing = false;		// Pause exam event
   else				// Event is paused, not being played ==> play it
      /* If not over, update status */
      if (Event.Status.Showing != ExaEvt_END)	// Event not over
	 Event.Status.Playing = true;		// Start/resume exam event

   /***** Update exam event status in database *****/
   ExaEvt_UpdateEventStatusInDB (&Event);

   /***** Show current exam event status *****/
   HTM_DIV_Begin ("id=\"exam event\" class=\"EXA_CONT\"");
   ExaEvt_ShowMatchStatusForTch (&Event);
   HTM_DIV_End ();
  }

/*****************************************************************************/
/**** Change number of columns in answers of an exam event (by a teacher) ****/
/*****************************************************************************/

void ExaEvt_ChangeNumColsEvt (void)
  {
   struct ExaEvt_Event Event;

   /***** Remove old players.
          This function must be called by a teacher
          before getting exam event status. *****/
   ExaEvt_RemoveOldPlayers ();

   /***** Get data of the exam event from database *****/
   Event.EvtCod = ExaEvt_GetEvtCodBeingPlayed ();
   ExaEvt_GetDataOfEventByCod (&Event);

   /***** Get number of columns *****/
   Event.Status.NumCols = (unsigned)
	                  Par_GetParToUnsignedLong ("NumCols",
						    1,
						    ExaEvt_MAX_COLS,
						    ExaEvt_NUM_COLS_DEFAULT);

   /***** Update exam event status in database *****/
   ExaEvt_UpdateEventStatusInDB (&Event);

   /***** Show current exam event status *****/
   HTM_DIV_Begin ("id=\"exam event\" class=\"EXA_CONT\"");
   ExaEvt_ShowMatchStatusForTch (&Event);
   HTM_DIV_End ();
  }

/*****************************************************************************/
/****** Toggle the display of results in an exam event (by a teacher) ********/
/*****************************************************************************/

void ExaEvt_ToggleVisibilResultsEvtQst (void)
  {
   struct ExaEvt_Event Event;

   /***** Remove old players.
          This function must be called by a teacher
          before getting exam event status. *****/
   ExaEvt_RemoveOldPlayers ();

   /***** Get data of the exam event from database *****/
   Event.EvtCod = ExaEvt_GetEvtCodBeingPlayed ();
   ExaEvt_GetDataOfEventByCod (&Event);

   /***** Update status *****/
   Event.Status.ShowQstResults = !Event.Status.ShowQstResults;	// Toggle display
   if (Event.Status.Showing == ExaEvt_RESULTS &&
       !Event.Status.ShowQstResults)
     Event.Status.Showing = ExaEvt_ANSWERS;	// Hide results

   /***** Update exam event status in database *****/
   ExaEvt_UpdateEventStatusInDB (&Event);

   /***** Show current exam event status *****/
   HTM_DIV_Begin ("id=\"exam event\" class=\"EXA_CONT\"");
   ExaEvt_ShowMatchStatusForTch (&Event);
   HTM_DIV_End ();
  }

/*****************************************************************************/
/********** Show previous question in an exam event (by a teacher) ***********/
/*****************************************************************************/

void ExaEvt_BackEvent (void)
  {
   struct ExaEvt_Event Event;

   /***** Remove old players.
          This function must be called by a teacher
          before getting exam event status. *****/
   ExaEvt_RemoveOldPlayers ();

   /***** Get data of the exam event from database *****/
   Event.EvtCod = ExaEvt_GetEvtCodBeingPlayed ();
   ExaEvt_GetDataOfEventByCod (&Event);

   /***** Update status *****/
   ExaEvt_SetMatchStatusToPrev (&Event);

   /***** Update exam event status in database *****/
   ExaEvt_UpdateEventStatusInDB (&Event);

   /***** Show current exam event status *****/
   HTM_DIV_Begin ("id=\"exam event\" class=\"EXA_CONT\"");
   ExaEvt_ShowMatchStatusForTch (&Event);
   HTM_DIV_End ();
  }

/*****************************************************************************/
/************ Show next question in an exam event (by a teacher) *************/
/*****************************************************************************/

void ExaEvt_ForwardEvent (void)
  {
   struct ExaEvt_Event Event;

   /***** Remove old players.
          This function must be called by a teacher
          before getting exam event status. *****/
   ExaEvt_RemoveOldPlayers ();

   /***** Get data of the exam event from database *****/
   Event.EvtCod = ExaEvt_GetEvtCodBeingPlayed ();
   ExaEvt_GetDataOfEventByCod (&Event);

   /***** Update status *****/
   ExaEvt_SetMatchStatusToNext (&Event);

   /***** Update exam event status in database *****/
   ExaEvt_UpdateEventStatusInDB (&Event);

   /***** Show current exam event status *****/
   HTM_DIV_Begin ("id=\"exam event\" class=\"EXA_CONT\"");
   ExaEvt_ShowMatchStatusForTch (&Event);
   HTM_DIV_End ();
  }

/*****************************************************************************/
/************ Set exam event status to previous (backward) status ************/
/*****************************************************************************/

static void ExaEvt_SetMatchStatusToPrev (struct ExaEvt_Event *Event)
  {
   /***** What to show *****/
   switch (Event->Status.Showing)
     {
      case ExaEvt_START:
	 ExaEvt_SetMatchStatusToStart (Event);
	 break;
      case ExaEvt_STEM:
      case ExaEvt_END:
	 ExaEvt_SetMatchStatusToPrevQst (Event);
	 break;
      case ExaEvt_ANSWERS:
	 Event->Status.Showing = ExaEvt_STEM;
	 break;
      case ExaEvt_RESULTS:
	 Event->Status.Showing = ExaEvt_ANSWERS;
	 break;
     }
   Event->Status.Countdown = -1L;	// No countdown
  }

/*****************************************************************************/
/**************** Set exam event status to previous question *****************/
/*****************************************************************************/

static void ExaEvt_SetMatchStatusToPrevQst (struct ExaEvt_Event *Event)
  {
   /***** Get index of the previous question *****/
   Event->Status.QstInd = Gam_GetPrevQuestionIndexInGame (Event->ExaCod,
							  Event->Status.QstInd);
   if (Event->Status.QstInd)		// Start of questions not reached
     {
      Event->Status.QstCod = Gam_GetQstCodFromQstInd (Event->ExaCod,
						      Event->Status.QstInd);
      Event->Status.Showing = Event->Status.ShowQstResults ? ExaEvt_RESULTS :
							     ExaEvt_ANSWERS;
     }
   else					// Start of questions reached
      ExaEvt_SetMatchStatusToStart (Event);
  }

/*****************************************************************************/
/********************** Set exam event status to start ***********************/
/*****************************************************************************/

static void ExaEvt_SetMatchStatusToStart (struct ExaEvt_Event *Event)
  {
   Event->Status.QstInd  = 0;				// Before first question
   Event->Status.QstCod  = -1L;
   Event->Status.Playing = false;
   Event->Status.Showing = ExaEvt_START;
  }

/*****************************************************************************/
/************** Set exam event status to next (forward) status ***************/
/*****************************************************************************/

static void ExaEvt_SetMatchStatusToNext (struct ExaEvt_Event *Event)
  {
   /***** What to show *****/
   switch (Event->Status.Showing)
     {
      case ExaEvt_START:
	 ExaEvt_SetMatchStatusToNextQst (Event);
	 break;
      case ExaEvt_STEM:
	 Event->Status.Showing = ExaEvt_ANSWERS;
	 break;
      case ExaEvt_ANSWERS:
	 if (Event->Status.ShowQstResults)
	    Event->Status.Showing = ExaEvt_RESULTS;
	 else
	    ExaEvt_SetMatchStatusToNextQst (Event);
	 break;
      case ExaEvt_RESULTS:
	 ExaEvt_SetMatchStatusToNextQst (Event);
	 break;
      case ExaEvt_END:
	 ExaEvt_SetMatchStatusToEnd (Event);
	 break;
     }
   Event->Status.Countdown = -1L;	// No countdown
  }

/*****************************************************************************/
/**************** Set exam event status to next question *********************/
/*****************************************************************************/

static void ExaEvt_SetMatchStatusToNextQst (struct ExaEvt_Event *Event)
  {
   /***** Get index of the next question *****/
   Event->Status.QstInd = Gam_GetNextQuestionIndexInGame (Event->ExaCod,
							  Event->Status.QstInd);

   /***** Get question code *****/
   if (Event->Status.QstInd < ExaEvt_AFTER_LAST_QUESTION)	// End of questions not reached
     {
      Event->Status.QstCod = Gam_GetQstCodFromQstInd (Event->ExaCod,
						      Event->Status.QstInd);
      Event->Status.Showing = ExaEvt_STEM;
     }
   else							// End of questions reached
      ExaEvt_SetMatchStatusToEnd (Event);
  }

/*****************************************************************************/
/********************** Set exam event status to end *************************/
/*****************************************************************************/

static void ExaEvt_SetMatchStatusToEnd (struct ExaEvt_Event *Event)
  {
   Event->Status.QstInd  = ExaEvt_AFTER_LAST_QUESTION;	// After last question
   Event->Status.QstCod  = -1L;
   Event->Status.Playing = false;
   Event->Status.Showing = ExaEvt_END;
  }

/*****************************************************************************/
/***** Show current exam event status (number, question, answers, button) ****/
/*****************************************************************************/

static void ExaEvt_ShowMatchStatusForTch (struct ExaEvt_Event *Event)
  {
   /***** Left column *****/
   ExaEvt_ShowLeftColumnTch (Event);

   /***** Right column *****/
   ExaEvt_ShowRightColumnTch (Event);
  }

/*****************************************************************************/
/************ Show current question being played for a student ***************/
/*****************************************************************************/

static void ExaEvt_ShowMatchStatusForStd (struct ExaEvt_Event *Event,ExaEvt_Update_t Update)
  {
   bool ICanPlayThisMatchBasedOnGrps;
   struct ExaEvt_UsrAnswer UsrAnswer;

   /***** Can I play this exam event? *****/
   ICanPlayThisMatchBasedOnGrps = ExaEvt_CheckIfICanPlayThisEventBasedOnGrps (Event);
   if (!ICanPlayThisMatchBasedOnGrps)
      Lay_NoPermissionExit ();

   /***** Get student's answer to this question
	  (<0 ==> no answer) *****/
   ExaEvt_GetQstAnsFromDB (Event->EvtCod,
			Gbl.Usrs.Me.UsrDat.UsrCod,
			Event->Status.QstInd,
			&UsrAnswer);

   /***** Left column *****/
   ExaEvt_ShowLeftColumnStd (Event,&UsrAnswer);

   /***** Right column *****/
   ExaEvt_ShowRightColumnStd (Event,&UsrAnswer,Update);
  }

/*****************************************************************************/
/********************** Get number of events in an exam **********************/
/*****************************************************************************/

unsigned ExaEvt_GetNumEventsInExam (long ExaCod)
  {
   /***** Trivial check *****/
   if (ExaCod < 0)	// A non-existing exam...
      return 0;		// ...has no events

   /***** Get number of events in an exam from database *****/
   return
   (unsigned) DB_QueryCOUNT ("can not get number of events of an exam",
			     "SELECT COUNT(*) FROM exa_events"
			     " WHERE ExaCod=%ld",
			     ExaCod);
  }

/*****************************************************************************/
/*************** Get number of unfinished events in an exam ******************/
/*****************************************************************************/

unsigned ExaEvt_GetNumUnfinishedEventsInExam (long ExaCod)
  {
   /***** Trivial check *****/
   if (ExaCod < 0)	// A non-existing exam...
      return 0;		// ...has no events

   /***** Get number of events in an exam from database *****/
   return
   (unsigned) DB_QueryCOUNT ("can not get number of unfinished events of an exam",
			     "SELECT COUNT(*) FROM exa_events"
			     " WHERE ExaCod=%ld AND Showing<>'%s'",
			     ExaCod,ExaEvt_ShowingStringsDB[ExaEvt_END]);
  }

/*****************************************************************************/
/********* Check if I belong to any of the groups of an exam event ***********/
/*****************************************************************************/

bool ExaEvt_CheckIfICanPlayThisEventBasedOnGrps (const struct ExaEvt_Event *Event)
  {
   switch (Gbl.Usrs.Me.Role.Logged)
     {
      case Rol_STD:
	 /***** Check if I belong to any of the groups
	        associated to the exam event *****/
	 return (DB_QueryCOUNT ("can not check if I can play an exam event",
				"SELECT COUNT(*) FROM exa_events"
				" WHERE EvtCod=%ld"
				" AND"
				"(EvtCod NOT IN"
				" (SELECT EvtCod FROM exa_groups)"
				" OR"
				" EvtCod IN"
				" (SELECT exa_groups.EvtCod"
				" FROM exa_groups,crs_grp_usr"
				" WHERE crs_grp_usr.UsrCod=%ld"
				" AND exa_groups.GrpCod=crs_grp_usr.GrpCod))",
				Event->EvtCod,Gbl.Usrs.Me.UsrDat.UsrCod) != 0);
	 break;
      case Rol_NET:
	 /***** Only if I am the creator *****/
	 return (Event->UsrCod == Gbl.Usrs.Me.UsrDat.UsrCod);
      case Rol_TCH:
      case Rol_SYS_ADM:
	 return true;
      default:
	 return false;
     }
  }

/*****************************************************************************/
/******** Show left column when playing an exam event (as a teacher) *********/
/*****************************************************************************/

static void ExaEvt_ShowLeftColumnTch (struct ExaEvt_Event *Event)
  {
   /***** Start left container *****/
   HTM_DIV_Begin ("class=\"EXA_LEFT_TCH\"");

   /***** Refreshable part *****/
   HTM_DIV_Begin ("id=\"match_left\" class=\"EXA_REFRESHABLE_TEACHER\"");
   ExaEvt_ShowRefreshablePartTch (Event);
   HTM_DIV_End ();

   /***** Put forms to start countdown *****/
   ExaEvt_PutFormsCountdown (Event);

   /***** Buttons *****/
   ExaEvt_PutMatchControlButtons (Event);

   /***** Put forms to choice which projects to show *****/
   Set_StartSettingsHead ();
   ExaEvt_ShowFormColumns (Event);
   Set_EndSettingsHead ();

   /***** Write button to request viewing results *****/
   ExaEvt_PutCheckboxResult (Event);

   /***** End left container *****/
   HTM_DIV_End ();
  }

/*****************************************************************************/
/***************** Show left refreshable part for teachers *******************/
/*****************************************************************************/

static void ExaEvt_ShowRefreshablePartTch (struct ExaEvt_Event *Event)
  {
   /***** Write elapsed time in exam event *****/
   ExaEvt_WriteElapsedTimeInEvt (Event);

   /***** Write number of question *****/
   ExaEvt_ShowNumQstInEvt (Event);

   /***** Write elapsed time in question *****/
   ExaEvt_WriteElapsedTimeInQst (Event);

   /***** Number of users who have responded this question *****/
   ExaEvt_WriteNumRespondersQst (Event);

   /***** Write hourglass *****/
   ExaEvt_PutCountdownAndHourglassIcon (Event);
  }

/*****************************************************************************/
/****************** Write elapsed time in current exam event *****************/
/*****************************************************************************/

static void ExaEvt_WriteElapsedTimeInEvt (struct ExaEvt_Event *Event)
  {
   struct Time Time;

   HTM_DIV_Begin ("class=\"EXA_TOP CT\"");

   /***** Get elapsed time in exam event *****/
   ExaEvt_GetElapsedTimeInMatch (Event,&Time);

   /***** Write elapsed time in hh:mm'ss" format *****/
   Dat_WriteHoursMinutesSeconds (&Time);

   HTM_DIV_End ();
  }

/*****************************************************************************/
/****************** Write elapsed time in current question *******************/
/*****************************************************************************/

static void ExaEvt_WriteElapsedTimeInQst (struct ExaEvt_Event *Event)
  {
   struct Time Time;

   HTM_DIV_Begin ("class=\"EXA_TIME_QST\"");

   switch (Event->Status.Showing)
     {
      case ExaEvt_START:
      case ExaEvt_END:
         HTM_Hyphen ();	// Do not write elapsed time
         break;
      default:
	 ExaEvt_GetElapsedTimeInQuestion (Event,&Time);
	 Dat_WriteHoursMinutesSeconds (&Time);
	 break;
     }

   HTM_DIV_End ();
  }

/*****************************************************************************/
/************ Write number of responders to an exam event question ***********/
/*****************************************************************************/

static void ExaEvt_WriteNumRespondersQst (struct ExaEvt_Event *Event)
  {
   extern const char *Txt_MATCH_respond;

   /***** Begin block *****/
   HTM_DIV_Begin ("class=\"EXA_NUM_ANSWERERS\"");
   HTM_Txt (Txt_MATCH_respond);
   HTM_BR ();
   HTM_STRONG_Begin ();

   /***** Write number of responders *****/
   switch (Event->Status.Showing)
     {
      case ExaEvt_START:
      case ExaEvt_END:
         HTM_Hyphen ();	// Do not write number of responders
         break;
      default:
	 HTM_Unsigned (ExaEvt_GetNumUsrsWhoAnsweredQst (Event->EvtCod,
					             Event->Status.QstInd));
	 break;
     }

   /***** Write number of players *****/
   if (Event->Status.Playing)	// Event is being played
     {
      /* Get current number of players */
      ExaEvt_GetNumPlayers (Event);

      /* Show current number of players */
      HTM_TxtF ("/%u",Event->Status.NumPlayers);
     }

   /***** End block *****/
   HTM_STRONG_End ();
   HTM_DIV_End ();
  }

/*****************************************************************************/
/*************** Write current countdown and hourglass icon ******************/
/*****************************************************************************/

static void ExaEvt_PutCountdownAndHourglassIcon (struct ExaEvt_Event *Event)
  {
   extern const char *Txt_Countdown;
   const char *Class;
   const char *Icon;

   /***** Set hourglass icon depending on countdown *****/
   if (Event->Status.Showing == ExaEvt_END)				// Event over
     {
      Class = "BT_LINK_OFF EXA_BUTTON_HIDDEN EXA_GREEN";
      Icon  = "fa-hourglass-start";
     }
   else if (Event->Status.Countdown < 0)				// No countdown
     {
      Class = "BT_LINK_OFF EXA_BUTTON_OFF EXA_GREEN";
      Icon  = "fa-hourglass-start";
     }
   else if (Event->Status.Countdown > ExaEvt_COUNTDOWN_SECONDS_MEDIUM)	// Countdown in progress
     {
      Class = "BT_LINK_OFF EXA_BUTTON_OFF EXA_LIMEGREEN";
      Icon  = "fa-hourglass-start";
     }
   else if (Event->Status.Countdown > ExaEvt_COUNTDOWN_SECONDS_SMALL)	// Countdown in progress
     {
      Class = "BT_LINK_OFF EXA_BUTTON_OFF EXA_YELLOW";
      Icon  = "fa-hourglass-half";
     }
   else									// Countdown about to end
     {
      Class = "BT_LINK_OFF EXA_BUTTON_OFF EXA_RED";
      Icon  = "fa-hourglass-end";
     }

   /***** Write countdown and put hourglass icon *****/
   HTM_DIV_Begin ("class=\"EXA_SHOW_HOURGLASS\"");
   HTM_DIV_Begin ("class=\"EXA_BIGBUTTON_CONT\"");
   HTM_BUTTON_BUTTON_Begin (Txt_Countdown,Class,NULL);

   /* Countdown */
   if (Event->Status.Countdown > 0)
      HTM_TxtF ("&nbsp;%02ld&Prime;",Event->Status.Countdown);
   else
      HTM_NBSP ();
   HTM_BR ();

   /* Icon */
   HTM_TxtF ("<i class=\"fas %s\"></i>",Icon);

   HTM_BUTTON_End ();
   HTM_DIV_End ();
   HTM_DIV_End ();
  }

/*****************************************************************************/
/******************** Put all forms to start countdowns **********************/
/*****************************************************************************/

static void ExaEvt_PutFormsCountdown (struct ExaEvt_Event *Event)
  {
   /***** Start container *****/
   HTM_DIV_Begin ("class=\"EXA_SHOW_HOURGLASS\"");

   /***** Put forms to start countdown *****/
   ExaEvt_PutFormCountdown (Event,-1                          ,"EXA_GREEN"    );
   ExaEvt_PutFormCountdown (Event,ExaEvt_COUNTDOWN_SECONDS_LARGE ,"EXA_LIMEGREEN");
   ExaEvt_PutFormCountdown (Event,ExaEvt_COUNTDOWN_SECONDS_MEDIUM,"EXA_YELLOW"   );
   ExaEvt_PutFormCountdown (Event,ExaEvt_COUNTDOWN_SECONDS_SMALL ,"EXA_RED"      );

   /***** End container *****/
   HTM_DIV_End ();
  }

/*****************************************************************************/
/****** Put a form to start a countdown with a given number of seconds *******/
/*****************************************************************************/

static void ExaEvt_PutFormCountdown (struct ExaEvt_Event *Event,long Seconds,const char *Color)
  {
   extern const char *Txt_Countdown;
   char *OnSubmit;
   bool PutForm = Event->Status.Showing != ExaEvt_END;

   if (PutForm)
     {
      /***** Start form *****/
      if (asprintf (&OnSubmit,"updateMatchTch('match_left',"
			      "'act=%ld&ses=%s&EvtCod=%ld&Countdown=%ld');"
			      " return false;",	// return false is necessary to not submit form
		    Act_GetActCod (ActExaEvtCntDwn),Gbl.Session.Id,
		    Event->EvtCod,Seconds) < 0)
	 Lay_NotEnoughMemoryExit ();
      Frm_StartFormOnSubmit (ActUnk,OnSubmit);

     }
   else
     {
     }

   /***** Put icon *****/
   HTM_DIV_Begin ("class=\"EXA_SMALLBUTTON_CONT\"");

   HTM_BUTTON_SUBMIT_Begin (PutForm ? Txt_Countdown :
				      NULL,
			    Str_BuildStringStr (PutForm ? "BT_LINK EXA_BUTTON_ON %s" :
				                          "BT_LINK_OFF EXA_BUTTON_HIDDEN %s",
						Color),
			    NULL);
   Str_FreeString ();

   HTM_NBSP ();
   if (Seconds >= 0)
      HTM_TxtF ("%ld&Prime;",Seconds);
   else
     {
      HTM_Txt ("&infin;");
      HTM_NBSP ();
     }

   HTM_BUTTON_End ();

   HTM_DIV_End ();

   /***** End form *****/
   if (PutForm)
     {
      Frm_EndForm ();
      free (OnSubmit);
     }
  }

/*****************************************************************************/
/******* Show right column when playing an exam event (as a teacher) *********/
/*****************************************************************************/

static void ExaEvt_ShowRightColumnTch (const struct ExaEvt_Event *Event)
  {
   /***** Start right container *****/
   HTM_DIV_Begin ("class=\"EXA_RIGHT_TCH\"");

   /***** Top row: exam event title *****/
   ExaEvt_ShowEventTitleTch (Event);

   /***** Bottom row: current question and possible answers *****/
   if (Event->Status.Showing == ExaEvt_END)	// Event over
      ExaEvt_ShowEventScore (Event);
   else						// Event not over
      ExaEvt_ShowQuestionAndAnswersTch (Event);

   /***** End right container *****/
   HTM_DIV_End ();
  }

/*****************************************************************************/
/******** Show left column when playing an exam event (as a student) *********/
/*****************************************************************************/

static void ExaEvt_ShowLeftColumnStd (const struct ExaEvt_Event *Event,
				      const struct ExaEvt_UsrAnswer *UsrAnswer)
  {
   bool Answered = UsrAnswer->NumOpt >= 0;

   /***** Start left container *****/
   HTM_DIV_Begin ("class=\"EXA_LEFT_STD\"");

   /***** Top *****/
   HTM_DIV_Begin ("class=\"EXA_TOP CT\"");
   HTM_DIV_End ();

   /***** Write number of question *****/
   ExaEvt_ShowNumQstInEvt (Event);

   switch (Event->Status.Showing)
     {
      case ExaEvt_START:
      case ExaEvt_END:
	 break;
      default:
	 /***** Write whether question is answered or not *****/
	 ExaEvt_PutIfAnswered (Event,Answered);

	 if (Event->Status.Playing &&			// Event is being played
	     Event->Status.Showing == ExaEvt_ANSWERS &&	// Teacher's screen is showing question answers
	     Answered)				// I have answered this question
	    /***** Put icon to remove my answet *****/
	    ExaEvt_PutIconToRemoveMyAnswer (Event);
	 break;
     }

   /***** End left container *****/
   HTM_DIV_End ();
  }

/*****************************************************************************/
/******* Show right column when playing an exam event (as a student) *********/
/*****************************************************************************/

static void ExaEvt_ShowRightColumnStd (struct ExaEvt_Event *Event,
				       const struct ExaEvt_UsrAnswer *UsrAnswer,
				       ExaEvt_Update_t Update)
  {
   extern const char *Txt_Please_wait_;

   /***** Start right container *****/
   HTM_DIV_Begin ("class=\"EXA_RIGHT_STD\"");

   /***** Top row *****/
   ExaEvt_ShowEventTitleStd (Event);

   /***** Bottom row *****/
   if (Event->Status.Playing)			// Event is being played
     {
      if (Event->Status.Showing == ExaEvt_END)	// Event over
	 ExaEvt_ShowWaitImage (Txt_Please_wait_);
      else					// Event not over
	{
	 HTM_DIV_Begin ("class=\"EXA_BOTTOM\"");

	 /***** Update players ******/
	 if (ExaEvt_RegisterMeAsPlayerInEvent (Event))
	   {
	    if (Event->Status.Showing == ExaEvt_ANSWERS)	// Teacher's screen is showing question answers
	       /* Show current question and possible answers */
	       if (!ExaEvt_ShowQuestionAndAnswersStd (Event,UsrAnswer,Update))
                  Ale_ShowAlert (Ale_ERROR,"Wrong question.");
	   }
	 else
	    Ale_ShowAlert (Ale_ERROR,"You can not join this exam event.");

	 HTM_DIV_End ();
	}
     }
   else						// Event is not being played
      ExaEvt_ShowWaitImage (Txt_Please_wait_);

   /***** End right container *****/
   HTM_DIV_End ();
  }

/*****************************************************************************/
/********************* Show number of question in exam ***********************/
/*****************************************************************************/

static void ExaEvt_ShowNumQstInEvt (const struct ExaEvt_Event *Event)
  {
   extern const char *Txt_MATCH_Start;
   extern const char *Txt_MATCH_End;
   unsigned NumQsts = Gam_GetNumQstsGame (Event->ExaCod);

   HTM_DIV_Begin ("class=\"EXA_NUM_QST\"");
   switch (Event->Status.Showing)
     {
      case ExaEvt_START:	// Not started
         HTM_Txt (Txt_MATCH_Start);
         break;
      case ExaEvt_END:	// Event over
         HTM_Txt (Txt_MATCH_End);
         break;
      default:
         HTM_TxtF ("%u/%u",Event->Status.QstInd,NumQsts);
         break;
     }
   HTM_DIV_End ();
  }

/*****************************************************************************/
/******************* Put buttons to control an exam event ********************/
/*****************************************************************************/

static void ExaEvt_PutMatchControlButtons (const struct ExaEvt_Event *Event)
  {
   extern const char *Txt_Go_back;
   extern const char *Txt_Go_forward;
   extern const char *Txt_Pause;
   extern const char *Txt_Start;
   extern const char *Txt_Resume;

   /***** Start buttons container *****/
   HTM_DIV_Begin ("class=\"EXA_BUTTONS_CONT\"");

   /***** Left button *****/
   HTM_DIV_Begin ("class=\"EXA_BUTTON_LEFT_CONT\"");
   switch (Event->Status.Showing)
     {
      case ExaEvt_START:
	 /* Put button to close browser tab */
	 ExaEvt_PutBigButtonClose ();
	 break;
      default:
	 /* Put button to go back */
	 ExaEvt_PutBigButton (ActBckExaEvt,"backward",Event->EvtCod,
			      ExaEvt_ICON_PREVIOUS,Txt_Go_back);
	 break;
     }
   HTM_DIV_End ();

   /***** Center button *****/
   HTM_DIV_Begin ("class=\"EXA_BUTTON_CENTER_CONT\"");
   if (Event->Status.Playing)					// Event is being played
      /* Put button to pause exam event */
      ExaEvt_PutBigButton (ActPlyPauExaEvt,"play_pause",Event->EvtCod,
			   ExaEvt_ICON_PAUSE,Txt_Pause);
   else								// Event is paused, not being played
     {
      switch (Event->Status.Showing)
        {
	 case ExaEvt_START:		// Event just started, before first question
	    /* Put button to start playing exam event */
	    ExaEvt_PutBigButton (ActPlyPauExaEvt,"play_pause",Event->EvtCod,
			         ExaEvt_ICON_PLAY,Txt_Start);
	    break;
	 case ExaEvt_END:			// Event over
	    /* Put disabled button to play exam event */
	    ExaEvt_PutBigButtonHidden (ExaEvt_ICON_PLAY);
	    break;
	 default:
	    /* Put button to resume exam event */
	    ExaEvt_PutBigButton (ActPlyPauExaEvt,"play_pause",Event->EvtCod,
			         ExaEvt_ICON_PLAY,Txt_Resume);
        }
     }
   HTM_DIV_End ();

   /***** Right button *****/
   HTM_DIV_Begin ("class=\"EXA_BUTTON_RIGHT_CONT\"");
   if (Event->Status.Showing == ExaEvt_END)	// Event over
      /* Put button to close browser tab */
      ExaEvt_PutBigButtonClose ();
   else						// Event not over
      /* Put button to show answers */
      ExaEvt_PutBigButton (ActFwdExaEvt,"forward",Event->EvtCod,
			   ExaEvt_ICON_NEXT,Txt_Go_forward);
   HTM_DIV_End ();

   /***** End buttons container *****/
   HTM_DIV_End ();
  }

/*****************************************************************************/
/** Show form to choice whether to show answers in one column or two columns */
/*****************************************************************************/

static void ExaEvt_ShowFormColumns (const struct ExaEvt_Event *Event)
  {
   extern const char *Txt_column;
   extern const char *Txt_columns;
   unsigned NumCols;
   static const char *NumColsIcon[1 + ExaEvt_MAX_COLS] =
     {
      "",		// Not used
      "1col.png",	// 1 column
      "2col.png",	// 2 columns
      "3col.png",	// 3 columns
      "4col.png",	// 4 columns
     };

   /***** Begin selector *****/
   Set_StartOneSettingSelector ();

   for (NumCols  = 1;
	NumCols <= ExaEvt_MAX_COLS;
	NumCols++)
     {
      /* Begin container for this option */
      HTM_DIV_Begin ("class=\"%s\"",
		     (Event->Status.NumCols == NumCols) ? "EXA_NUM_COL_ON" :
							  "EXA_NUM_COL_OFF");

      /* Begin form */
      Frm_StartForm (ActChgNumColExaEvt);
      ExaEvt_PutParamEvtCod (Event->EvtCod);	// Current exam event being played
      ExaEvt_PutParamNumCols (NumCols);		// Number of columns

      /* Number of columns */
      Ico_PutSettingIconLink (NumColsIcon[NumCols],
			      Str_BuildStringLongStr ((long) NumCols,
						      NumCols == 1 ? Txt_column :
								     Txt_columns));
      Str_FreeString ();

      /* End form */
      Frm_EndForm ();

      /* End container for this option */
      HTM_DIV_End ();
     }

   /***** End selector *****/
   Set_EndOneSettingSelector ();
  }

/*****************************************************************************/
/******** Write parameter with number of columns in answers of exam event *********/
/*****************************************************************************/

static void ExaEvt_PutParamNumCols (unsigned NumCols)	// Number of columns
  {
   Par_PutHiddenParamUnsigned (NULL,"NumCols",NumCols);
  }

/*****************************************************************************/
/***************** Put checkbox to select if show results ********************/
/*****************************************************************************/

static void ExaEvt_PutCheckboxResult (const struct ExaEvt_Event *Event)
  {
   extern const char *Txt_View_results;

   /***** Begin container *****/
   HTM_DIV_Begin ("class=\"EXA_SHOW_RESULTS\"");

   /***** Begin form *****/
   Frm_StartForm (ActChgVisResExaEvtQst);
   ExaEvt_PutParamEvtCod (Event->EvtCod);	// Current exam event being played

   /***** Put icon with link *****/
   HTM_BUTTON_SUBMIT_Begin (Txt_View_results,"BT_LINK DAT ICO_HIGHLIGHT",NULL);
   HTM_TxtF ("<i class=\"%s\"></i>",
	     Event->Status.ShowQstResults ? "fas fa-toggle-on" :
		                            "fas fa-toggle-off");
   HTM_TxtF ("&nbsp;%s",Txt_View_results);
   HTM_BUTTON_End ();

   /***** End form *****/
   Frm_EndForm ();

   /***** End container *****/
   HTM_DIV_End ();
  }

/*****************************************************************************/
/***************** Put checkbox to select if show results ********************/
/*****************************************************************************/

static void ExaEvt_PutIfAnswered (const struct ExaEvt_Event *Event,bool Answered)
  {
   extern const char *Txt_View_my_answer;
   extern const char *Txt_MATCH_QUESTION_Answered;
   extern const char *Txt_MATCH_QUESTION_Unanswered;

   /***** Start container *****/
   HTM_DIV_Begin ("class=\"EXA_SHOW_ANSWERED\"");

   /***** Put icon with link *****/
   if (Event->Status.Playing &&			// Event is being played
       Event->Status.Showing == ExaEvt_ANSWERS &&	// Teacher's screen is showing question answers
       Answered)				// I have answered this question
     {
      /* Start form */
      Frm_StartForm (ActSeeExaEvtAnsQstStd);
      ExaEvt_PutParamEvtCod (Event->EvtCod);	// Current exam event being played

      HTM_BUTTON_OnMouseDown_Begin (Txt_View_my_answer,"BT_LINK DAT_SMALL_GREEN");
      HTM_TxtF ("<i class=\"%s\"></i>","fas fa-check-circle");
      HTM_TxtF ("&nbsp;%s",Txt_MATCH_QUESTION_Answered);
      HTM_BUTTON_End ();

      /* End form */
      Frm_EndForm ();
     }
   else
     {
      HTM_DIV_Begin ("class=\"%s\"",Answered ? "DAT_SMALL_GREEN" :
	                                       "DAT_SMALL_RED");
      HTM_TxtF ("<i class=\"%s\" title=\"%s\"></i>",
		Answered ? "fas fa-check-circle" :
		           "fas fa-exclamation-circle",
		Answered ? Txt_MATCH_QUESTION_Answered :
		           Txt_MATCH_QUESTION_Unanswered);
      HTM_TxtF ("&nbsp;%s",Answered ? Txt_MATCH_QUESTION_Answered :
		                      Txt_MATCH_QUESTION_Unanswered);
      HTM_DIV_End ();
     }

   /***** End container *****/
   HTM_DIV_End ();
  }

/*****************************************************************************/
/***************** Put checkbox to select if show results ********************/
/*****************************************************************************/

static void ExaEvt_PutIconToRemoveMyAnswer (const struct ExaEvt_Event *Event)
  {
   extern const char *Txt_Delete_my_answer;

   /***** Start container *****/
   HTM_DIV_Begin ("class=\"EXA_REM_MY_ANS\"");

   /***** Start form *****/
   Frm_StartForm (ActRemExaEvtAnsQstStd);
   ExaEvt_PutParamEvtCod (Event->EvtCod);	// Current exam event being played
   Gam_PutParamQstInd (Event->Status.QstInd);	// Current question index shown

   /***** Put icon with link *****/
   HTM_DIV_Begin ("class=\"EXA_BIGBUTTON_CONT\"");
   HTM_BUTTON_OnMouseDown_Begin (Txt_Delete_my_answer,"BT_LINK EXA_BUTTON_ON ICO_DARKRED");
   HTM_Txt ("<i class=\"fas fa-trash\"></i>");
   HTM_BUTTON_End ();
   HTM_DIV_End ();

   /***** End form *****/
   Frm_EndForm ();

   /***** End container *****/
   HTM_DIV_End ();
  }

/*****************************************************************************/
/***************************** Show exam event title ******************************/
/*****************************************************************************/

static void ExaEvt_ShowEventTitleTch (const struct ExaEvt_Event *Event)
  {
   /***** Event title *****/
   HTM_DIV_Begin ("class=\"EXA_TOP LT\"");
   HTM_Txt (Event->Title);
   HTM_DIV_End ();
  }

static void ExaEvt_ShowEventTitleStd (const struct ExaEvt_Event *Event)
  {
   /***** Event title *****/
   HTM_DIV_Begin ("class=\"EXA_TOP CT\"");
   HTM_Txt (Event->Title);
   HTM_DIV_End ();
  }

/*****************************************************************************/
/***** Show question and its answers when playing an exam event (as a teacher) *****/
/*****************************************************************************/

static void ExaEvt_ShowQuestionAndAnswersTch (const struct ExaEvt_Event *Event)
  {
   extern const char *Txt_MATCH_Paused;
   extern const char *Txt_Question_removed;
   struct Tst_Question Question;

   /***** Create test question *****/
   Tst_QstConstructor (&Question);
   Question.QstCod = Event->Status.QstCod;

   /***** Trivial check: do not show anything on exam event start and end *****/
   switch (Event->Status.Showing)
     {
      case ExaEvt_START:
      case ExaEvt_END:
	 return;
      default:
	 break;
     }

   /***** Get data of question from database *****/
   if (Tst_GetQstDataFromDB (&Question))
     {
      /***** Show question *****/
      /* Check answer type */
      if (Question.Answer.Type != Tst_ANS_UNIQUE_CHOICE)
	 Lay_ShowErrorAndExit ("Wrong answer type.");

      /* Begin container */
      HTM_DIV_Begin ("class=\"EXA_BOTTOM\"");	// Bottom

      /* Write stem */
      Tst_WriteQstStem (Question.Stem,"EXA_TCH_STEM",
			true);	// Visible

      /* Show media */
      Med_ShowMedia (&Question.Media,
		     "TEST_MED_EDIT_LIST_STEM_CONTAINER",
		     "TEST_MED_EDIT_LIST_STEM");

      /***** Write answers? *****/
      switch (Event->Status.Showing)
	{
	 case ExaEvt_ANSWERS:
	    if (Event->Status.Playing)			// Event is being played
	       /* Write answers */
	       ExaEvt_WriteAnswersEventResult (Event,
	                                       &Question,
					       "EXA_TCH_ANS",
					       false);	// Don't show result
	    else					// Event is paused, not being played
	       ExaEvt_ShowWaitImage (Txt_MATCH_Paused);
	    break;
	 case ExaEvt_RESULTS:
	    /* Write answers with results */
	    ExaEvt_WriteAnswersEventResult (Event,
	                                    &Question,
					    "EXA_TCH_ANS",
					    true);		// Show result
	    break;
	 default:
	    /* Don't write anything */
	    break;
	}

      /* End container */
      HTM_DIV_End ();				// Bottom
     }
   else
      Ale_ShowAlert (Ale_WARNING,Txt_Question_removed);

   /***** Destroy test question *****/
   Tst_QstDestructor (&Question);
  }

/*****************************************************************************/
/************* Write answers of a question when seeing an exam event ***************/
/*****************************************************************************/

static void ExaEvt_WriteAnswersEventResult (const struct ExaEvt_Event *Event,
                                            const struct Tst_Question *Question,
                                            const char *Class,bool ShowResult)
  {
   /***** Write answer depending on type *****/
   if (Question->Answer.Type == Tst_ANS_UNIQUE_CHOICE)
      ExaEvt_WriteChoiceAnsViewEvent (Event,
				   Question,
				   Class,ShowResult);
   else
      Ale_ShowAlert (Ale_ERROR,"Type of answer not valid in an exam.");
  }

/*****************************************************************************/
/******** Write single or multiple choice answer when seeing an exam event *********/
/*****************************************************************************/

void ExaEvt_WriteChoiceAnsViewEvent (const struct ExaEvt_Event *Event,
                                     const struct Tst_Question *Question,
                                     const char *Class,bool ShowResult)
  {
   unsigned NumOpt;
   bool RowIsOpen = false;
   unsigned NumRespondersQst;
   unsigned NumRespondersAns;
   unsigned Indexes[Tst_MAX_OPTIONS_PER_QUESTION];	// Indexes of all answers of this question

   /***** Get number of users who have answered this question from database *****/
   NumRespondersQst = ExaEvt_GetNumUsrsWhoAnsweredQst (Event->EvtCod,Event->Status.QstInd);

   /***** Get indexes for this question in exam event *****/
   ExaEvt_GetIndexes (Event->EvtCod,Event->Status.QstInd,Indexes);

   /***** Begin table *****/
   HTM_TABLE_BeginWidePadding (0);

   /***** Show options distributed in columns *****/
   for (NumOpt = 0;
	NumOpt < Question->Answer.NumOptions;
	NumOpt++)
     {
      /***** Start row? *****/
      if (NumOpt % Event->Status.NumCols == 0)
	{
	 HTM_TR_Begin (NULL);
	 RowIsOpen = true;
	}

      /***** Write letter for this option *****/
      HTM_TD_Begin ("class=\"EXA_TCH_BUTTON_TD\"");
      HTM_DIV_Begin ("class=\"EXA_TCH_BUTTON BT_%c\"",'A' + (char) NumOpt);
      HTM_TxtF ("%c",'a' + (char) NumOpt);
      HTM_DIV_End ();
      HTM_TD_End ();

      /***** Write the option text and the result *****/
      HTM_TD_Begin ("class=\"LT\"");
      HTM_LABEL_Begin ("for=\"Ans%06u_%u\" class=\"%s\"",Event->Status.QstInd,NumOpt,Class);

      /* Convert text, that is in HTML, to rigorous HTML */
      Str_ChangeFormat (Str_FROM_HTML,Str_TO_RIGOROUS_HTML,
                        Question->Answer.Options[NumOpt].Text,
                        Tst_MAX_BYTES_ANSWER_OR_FEEDBACK,false);
      HTM_Txt (Question->Answer.Options[Indexes[NumOpt]].Text);

      HTM_LABEL_End ();
      Med_ShowMedia (&Question->Answer.Options[Indexes[NumOpt]].Media,
                     "TEST_MED_SHOW_CONT",
                     "TEST_MED_SHOW");

      /* Show result (number of users who answered? */
      if (ShowResult)
	{
	 /* Get number of users who selected this answer */
	 NumRespondersAns = ExaEvt_GetNumUsrsWhoHaveChosenAns (Event->EvtCod,Event->Status.QstInd,Indexes[NumOpt]);

	 /* Draw proportional bar for this answer */
	 ExaEvt_DrawBarNumUsrs (NumRespondersAns,NumRespondersQst,
	                     Question->Answer.Options[Indexes[NumOpt]].Correct);
	}
      else
         /* Draw empty bar for this answer
            in order to show the same layout that the one shown with results */
         ExaEvt_DrawBarNumUsrs (0,0,
                             false);	// Not used when length of bar is 0

      HTM_TD_End ();

      /***** End row? *****/
      if (NumOpt % Event->Status.NumCols == Event->Status.NumCols - 1)
	{
         HTM_TR_End ();
	 RowIsOpen = false;
	}
     }

   /***** End row? *****/
   if (RowIsOpen)
      HTM_TR_End ();

   /***** End table *****/
   HTM_TABLE_End ();
  }

/*****************************************************************************/
/***** Show question and its answers when playing an exam event (as a student) *****/
/*****************************************************************************/
// Return true on valid question, false on invalid question

static bool ExaEvt_ShowQuestionAndAnswersStd (const struct ExaEvt_Event *Event,
					      const struct ExaEvt_UsrAnswer *UsrAnswer,
					      ExaEvt_Update_t Update)
  {
   unsigned NumOptions;
   unsigned NumOpt;
   char *Class;

   /***** Trivial check: this question must be valid for exams *****/
   if (!Tst_CheckIfQuestionIsValidForGame (Event->Status.QstCod))
      return false;

   /***** Get number of options in this question *****/
   NumOptions = Tst_GetNumAnswersQst (Event->Status.QstCod);

   /***** Begin table *****/
   HTM_TABLE_BeginWidePadding (8);

   for (NumOpt = 0;
	NumOpt < NumOptions;
	NumOpt++)
     {
      /***** Start row *****/
      HTM_TR_Begin (NULL);

      /***** Write letter for this option *****/
      /* Begin table cell */
      HTM_TD_Begin ("class=\"EXA_STD_CELL\"");

      /* Form with button.
	 Sumitting onmousedown instead of default onclick
	 is necessary in order to be fast
	 and not lose clicks due to refresh */
      Frm_StartForm (ActAnsExaEvtQstStd);
      ExaEvt_PutParamEvtCod (Event->EvtCod);		// Current exam event being played
      Gam_PutParamQstInd (Event->Status.QstInd);	// Current question index shown
      ExaEvt_PutParamNumOpt (NumOpt);		// Number of button

      if (asprintf (&Class,"EXA_STD_BUTTON%s BT_%c",
		    UsrAnswer->NumOpt == (int) NumOpt &&	// Student's answer
		    Update == ExaEvt_CHANGE_STATUS_BY_STUDENT ? " EXA_STD_ANSWER_SELECTED" :
							     "",
		    'A' + (char) NumOpt) < 0)
	 Lay_NotEnoughMemoryExit ();
      HTM_BUTTON_OnMouseDown_Begin (NULL,Class);
      HTM_TxtF ("%c",'a' + (char) NumOpt);
      HTM_BUTTON_End ();
      free (Class);

      Frm_EndForm ();

      /* End table cell */
      HTM_TD_End ();

      /***** End row *****/
      HTM_TR_End ();
     }

   /***** End table *****/
   HTM_TABLE_End ();

   return true;
  }

/*****************************************************************************/
/***************************** Show exam event scores *****************************/
/*****************************************************************************/

#define ExaEvt_NUM_ROWS_SCORE 50

static void ExaEvt_ShowEventScore (const struct ExaEvt_Event *Event)
  {
   MYSQL_RES *mysql_res;
   MYSQL_ROW row;
   unsigned NumScores;
   unsigned NumScore;
   double MinScore;
   double MaxScore;
   double Range;
   double NumRowsPerScorePoint;
   double Score;
   unsigned MaxUsrs = 0;
   unsigned NumUsrs;
   unsigned NumRowForThisScore;
   unsigned NumRow;

   /***** Get minimum and maximum scores *****/
   Gam_GetScoreRange (Event->ExaCod,&MinScore,&MaxScore);
   Range = MaxScore - MinScore;
   if (Range == 0.0)
      return;
   NumRowsPerScorePoint = (double) ExaEvt_NUM_ROWS_SCORE / Range;

   /***** Get maximum number of users *****/
   if (DB_QuerySELECT (&mysql_res,"can not get max users",
		       "SELECT MAX(NumUsrs)"
		       " FROM "
		       "(SELECT COUNT(*) AS NumUsrs"	// row[1]
		       " FROM exa_results"
		       " WHERE EvtCod=%ld"
		       " GROUP BY Score"
		       " ORDER BY Score) AS Scores",
		       Event->EvtCod))
     {
      row = mysql_fetch_row (mysql_res);

      /* Get maximum number of users (row[0]) *****/
      if (row)
         if (row[0])
	    if (sscanf (row[0],"%u",&MaxUsrs) != 1)
	       MaxUsrs = 0;
     }

   /* Free structure that stores the query result */
   DB_FreeMySQLResult (&mysql_res);

   /***** Get scores from database *****/
   NumScores = (unsigned)
	       DB_QuerySELECT (&mysql_res,"can not get scores",
			       "SELECT Score,"			// row[0]
				      "COUNT(*) AS NumUsrs"	// row[1]
			       " FROM exa_results"
			       " WHERE EvtCod=%ld"
			       " GROUP BY Score"
			       " ORDER BY Score DESC",
			       Event->EvtCod);

   /***** Begin table ****/
   HTM_TABLE_BeginWide ();

   /***** Get and draw scores *****/
   for (NumScore = 0, NumRow = 0;
	NumScore < NumScores;
	NumScore++)
     {
      /***** Get score and number of users from database *****/
      row = mysql_fetch_row (mysql_res);

      /* Get score (row[0]) */
      Str_SetDecimalPointToUS ();	// To get the decimal point as a dot
      if (sscanf (row[0],"%lf",&Score) != 1)
	 Score = 0.0;
      Str_SetDecimalPointToLocal ();	// Return to local system

      /* Get number of users (row[1]) *****/
      if (sscanf (row[1],"%u",&NumUsrs) != 1)
	 NumUsrs = 0;

      /***** Draw empty rows until reaching the adequate row *****/
      NumRowForThisScore = (unsigned) ((MaxScore - Score) * NumRowsPerScorePoint);
      if (NumRowForThisScore == ExaEvt_NUM_ROWS_SCORE)
	 NumRowForThisScore = ExaEvt_NUM_ROWS_SCORE - 1;
      for (;
	   NumRow < NumRowForThisScore;
	   NumRow++)
         ExaEvt_DrawEmptyScoreRow (NumRow,MinScore,MaxScore);

      /***** Draw row for this score *****/
      ExaEvt_DrawScoreRow (Score,MinScore,MaxScore,NumRow,NumUsrs,MaxUsrs);
      NumRow++;
     }

   /***** Draw final empty rows *****/
   for (;
	NumRow < ExaEvt_NUM_ROWS_SCORE;
	NumRow++)
      ExaEvt_DrawEmptyScoreRow (NumRow,MinScore,MaxScore);

   /***** End table *****/
   HTM_TABLE_End ();

   /***** Free structure that stores the query result *****/
   DB_FreeMySQLResult (&mysql_res);
  }

/*****************************************************************************/
/*************************** Draw empty score row ****************************/
/*****************************************************************************/

static void ExaEvt_DrawEmptyScoreRow (unsigned NumRow,double MinScore,double MaxScore)
  {
   /***** Draw row *****/
   HTM_TR_Begin (NULL);

   /* Write score */
   HTM_TD_Begin ("class=\"EXA_SCO_SCO\"");
   if (NumRow == 0)
     {
      HTM_DoubleFewDigits (MaxScore);
      HTM_NBSP ();
     }
   else if (NumRow == ExaEvt_NUM_ROWS_SCORE - 1)
     {
      HTM_DoubleFewDigits (MinScore);
      HTM_NBSP ();
     }
   HTM_TD_End ();

   /* Empty column with bar and number of users */
   HTM_TD_Begin ("class=\"EXA_SCO_NUM%s\"",ExaEvt_GetClassBorder (NumRow));
   HTM_TD_End ();

   HTM_TR_End ();
  }

/*****************************************************************************/
/******************************* Draw score row ******************************/
/*****************************************************************************/

static void ExaEvt_DrawScoreRow (double Score,double MinScore,double MaxScore,
			      unsigned NumRow,unsigned NumUsrs,unsigned MaxUsrs)
  {
   extern const char *Txt_ROLES_SINGUL_abc[Rol_NUM_ROLES][Usr_NUM_SEXS];
   extern const char *Txt_ROLES_PLURAL_abc[Rol_NUM_ROLES][Usr_NUM_SEXS];
   unsigned Color;
   unsigned BarWidth;
   char *Icon;

   /***** Compute color *****/
   /*
   +----------------- MaxScore
   | score9_1x1.png
   +-----------------
   | score8_1x1.png
   +-----------------
   | score7_1x1.png
   +-----------------
   | score6_1x1.png
   +-----------------
   | score5_1x1.png
   +-----------------
   | score4_1x1.png
   +-----------------
   | score3_1x1.png
   +-----------------
   | score2_1x1.png
   +-----------------
   | score1_1x1.png
   +-----------------
   | score0_1x1.png
   +----------------- MinScore
   */
   Color = (unsigned) (((Score - MinScore) / (MaxScore - MinScore)) * 10.0);
   if (Color == 10)
      Color = 9;

   /***** Compute bar width *****/
   if (MaxUsrs > 0)
     {
      BarWidth = (unsigned) (((NumUsrs * 95.0) / MaxUsrs) + 0.5);
      if (BarWidth == 0)
	 BarWidth = 1;
     }
   else
      BarWidth = 0;

   /***** Draw row *****/
   HTM_TR_Begin (NULL);

   /* Write score */
   HTM_TD_Begin ("class=\"EXA_SCO_SCO\"");
   HTM_DoubleFewDigits (Score);
   HTM_NBSP ();
   HTM_TD_End ();

   /* Draw bar and write number of users for this score */
   HTM_TD_Begin ("class=\"EXA_SCO_NUM%s\"",ExaEvt_GetClassBorder (NumRow));
   if (asprintf (&Icon,"score%u_1x1.png",Color) < 0)	// Background
      Lay_NotEnoughMemoryExit ();
   HTM_IMG (Cfg_URL_ICON_PUBLIC,Icon,
	    Str_BuildStringLongStr ((long) NumUsrs,
				    NumUsrs == 1 ? Txt_ROLES_SINGUL_abc[Rol_STD][Usr_SEX_UNKNOWN] :
						   Txt_ROLES_PLURAL_abc[Rol_STD][Usr_SEX_UNKNOWN]),
	    "class=\"EXA_SCO_BAR\" style=\"width:%u%%;\"",BarWidth);
   Str_FreeString ();
   free (Icon);
   HTM_TxtF ("&nbsp;%u",NumUsrs);
   HTM_TD_End ();

   HTM_TR_End ();
  }

/*****************************************************************************/
/****** Write parameter with number of option (button) pressed by user *******/
/*****************************************************************************/

static const char *ExaEvt_GetClassBorder (unsigned NumRow)
  {
   return NumRow == 0                         ? " EXA_SCO_TOP" :
	 (NumRow == ExaEvt_NUM_ROWS_SCORE - 1 ? " EXA_SCO_BOT" :
		                                " EXA_SCO_MID");
  }

/*****************************************************************************/
/****** Write parameter with number of option (button) pressed by user *******/
/*****************************************************************************/

static void ExaEvt_PutParamNumOpt (unsigned NumOpt)
  {
   Par_PutHiddenParamUnsigned (NULL,"NumOpt",NumOpt);
  }

/*****************************************************************************/
/******* Get parameter with number of option (button) pressed by user ********/
/*****************************************************************************/

static unsigned ExaEvt_GetParamNumOpt (void)
  {
   long NumOpt;

   NumOpt = Par_GetParToLong ("NumOpt");
   if (NumOpt < 0)
      Lay_ShowErrorAndExit ("Wrong number of option.");

   return (unsigned) NumOpt;
  }

/*****************************************************************************/
/*********************** Put a big button to do action ***********************/
/*****************************************************************************/

static void ExaEvt_PutBigButton (Act_Action_t NextAction,const char *Id,
			         long EvtCod,const char *Icon,const char *Txt)
  {
   /***** Begin form *****/
   Frm_StartFormId (NextAction,Id);
   ExaEvt_PutParamEvtCod (EvtCod);

   /***** Put icon with link *****/
   HTM_DIV_Begin ("class=\"EXA_BIGBUTTON_CONT\"");
   HTM_BUTTON_SUBMIT_Begin (Txt,"BT_LINK EXA_BUTTON_ON ICO_BLACK",NULL);
   HTM_TxtF ("<i class=\"%s\"></i>",Icon);
   HTM_BUTTON_End ();
   HTM_DIV_End ();

   /***** End form *****/
   Frm_EndForm ();
  }

/*****************************************************************************/
/************************** Put a big button hidden **************************/
/*****************************************************************************/

static void ExaEvt_PutBigButtonHidden (const char *Icon)
  {
   /***** Put inactive icon *****/
   HTM_DIV_Begin ("class=\"EXA_BIGBUTTON_CONT\"");
   HTM_BUTTON_BUTTON_Begin (NULL,"BT_LINK_OFF EXA_BUTTON_HIDDEN ICO_BLACK",NULL);
   HTM_TxtF ("<i class=\"%s\"></i>",Icon);
   HTM_BUTTON_End ();
   HTM_DIV_End ();
  }
/*****************************************************************************/
/********************** Put a big button to close window *********************/
/*****************************************************************************/

static void ExaEvt_PutBigButtonClose (void)
  {
   extern const char *Txt_Close;

   /***** Put icon with link *****/
   HTM_DIV_Begin ("class=\"EXA_BIGBUTTON_CONT\"");
   HTM_BUTTON_BUTTON_Begin (Txt_Close,"BT_LINK EXA_BUTTON_ON ICO_DARKRED","window.close();");
   HTM_TxtF ("<i class=\"%s\"></i>",ExaEvt_ICON_CLOSE);
   HTM_BUTTON_End ();
   HTM_DIV_End ();
  }

/*****************************************************************************/
/****************************** Show wait image ******************************/
/*****************************************************************************/

static void ExaEvt_ShowWaitImage (const char *Txt)
  {
   HTM_DIV_Begin ("class=\"EXA_WAIT_CONT\"");
   Ico_PutIcon ("Spin-1s-200px.gif",Txt,"EXA_WAIT_IMG");
   HTM_DIV_End ();
  }

/*****************************************************************************/
/**************************** Remove old players *****************************/
/*****************************************************************************/

static void ExaEvt_RemoveOldPlayers (void)
  {
   /***** Delete events not being played by teacher *****/
   DB_QueryDELETE ("can not update events as not being played",
		   "DELETE FROM exa_playing"
		   " WHERE TS<FROM_UNIXTIME(UNIX_TIMESTAMP()-%lu)",
		   Cfg_SECONDS_TO_REFRESH_MATCH_TCH*3);

   /***** Delete players (students) who have left events *****/
   DB_QueryDELETE ("can not update exam event players",
		   "DELETE FROM exa_players"
		   " WHERE TS<FROM_UNIXTIME(UNIX_TIMESTAMP()-%lu)",
		   Cfg_SECONDS_TO_REFRESH_MATCH_STD*3);
  }

/*****************************************************************************/
/********************** Update exam event as being played *************************/
/*****************************************************************************/

static void ExaEvt_UpdateEventAsBeingPlayed (long EvtCod)
  {
   /***** Insert exam event as being played *****/
   DB_QueryREPLACE ("can not set exam event as being played",
		    "REPLACE exa_playing (EvtCod) VALUE (%ld)",
		    EvtCod);
  }

/*****************************************************************************/
/**************** Update exam event as paused, not being played *******************/
/*****************************************************************************/

static void ExaEvt_SetEventAsNotBeingPlayed (long EvtCod)
  {
   /***** Delete all exam event players ******/
   DB_QueryDELETE ("can not update exam event players",
		    "DELETE FROM exa_players"
		    " WHERE EvtCod=%ld",
		    EvtCod);

   /***** Delete exam event as being played ******/
   DB_QueryDELETE ("can not set exam event as not being played",
		    "DELETE FROM exa_playing"
		    " WHERE EvtCod=%ld",
		    EvtCod);
  }

/*****************************************************************************/
/*********************** Get if exam event is being played ************************/
/*****************************************************************************/

static bool ExaEvt_GetIfEventIsBeingPlayed (long EvtCod)
  {
   /***** Get if an exam event is being played or not *****/
   return
   (bool) (DB_QueryCOUNT ("can not get if exam event is being played",
			  "SELECT COUNT(*) FROM exa_playing"
			  " WHERE EvtCod=%ld",
			  EvtCod) != 0);
  }

/*****************************************************************************/
/*************************** Get number of players ***************************/
/*****************************************************************************/

static void ExaEvt_GetNumPlayers (struct ExaEvt_Event *Event)
  {
   /***** Get number of players who are playing an exam event *****/
   Event->Status.NumPlayers =
   (unsigned) DB_QueryCOUNT ("can not get number of players",
			     "SELECT COUNT(*) FROM exa_players"
			     " WHERE EvtCod=%ld",
			     Event->EvtCod);
  }

/*****************************************************************************/
/******************* Register me as a player in an exam event **********************/
/*****************************************************************************/
// Return true on success

bool ExaEvt_RegisterMeAsPlayerInEvent (struct ExaEvt_Event *Event)
  {
   /***** Trivial check: exam event code must be > 0 *****/
   if (Event->EvtCod <= 0)
      return false;

   /***** Trivial check: exam event must be being played *****/
   if (!Event->Status.Playing)				// Event is paused, not being played
      return false;

   /***** Trivial check: exam event must not be over *****/
   if (Event->Status.Showing == ExaEvt_END)		// Event over
      return false;

   /***** Trivial check: only a student can join an exam event *****/
   if (Gbl.Usrs.Me.Role.Logged != Rol_STD)		// I am not logged as student
      return false;

   /***** Insert me as exam event player *****/
   DB_QueryREPLACE ("can not insert exam event player",
		    "REPLACE exa_players (EvtCod,UsrCod) VALUES (%ld,%ld)",
		    Event->EvtCod,Gbl.Usrs.Me.UsrDat.UsrCod);
   return true;
  }

/*****************************************************************************/
/********************** Get code of exam event being played ***********************/
/*****************************************************************************/

void ExaEvt_GetEventBeingPlayed (void)
  {
   long EvtCodBeingPlayed;

   /***** Get exam event code ****/
   if ((EvtCodBeingPlayed = ExaEvt_GetParamEvtCod ()) <= 0)
      Lay_ShowErrorAndExit ("Code of exam event is missing.");

   ExaEvt_SetEvtCodBeingPlayed (EvtCodBeingPlayed);
  }

/*****************************************************************************/
/********************* Show exam event being played as student ********************/
/*****************************************************************************/

void ExaEvt_JoinEventAsStd (void)
  {
   struct ExaEvt_Event Event;

   /***** Get data of the exam event from database *****/
   Event.EvtCod = ExaEvt_GetEvtCodBeingPlayed ();
   ExaEvt_GetDataOfEventByCod (&Event);

   /***** Show current exam event status *****/
   HTM_DIV_Begin ("id=\"exam event\" class=\"EXA_CONT\"");
   ExaEvt_ShowMatchStatusForStd (&Event,ExaEvt_CHANGE_STATUS_BY_STUDENT);
   HTM_DIV_End ();
  }

/*****************************************************************************/
/****** Remove student's answer to a question and show exam event as student ******/
/*****************************************************************************/

void ExaEvt_RemoveMyQuestionAnswer (void)
  {
   struct ExaEvt_Event Event;
   unsigned QstInd;

   /***** Get data of the exam event from database *****/
   Event.EvtCod = ExaEvt_GetEvtCodBeingPlayed ();
   ExaEvt_GetDataOfEventByCod (&Event);

   /***** Get question index from form *****/
   QstInd = Exa_GetParamQstInd ();

   /***** Check that teacher's screen is showing answers
          and question index is the current one being played *****/
   if (Event.Status.Playing &&			// Event is being played
       Event.Status.Showing == ExaEvt_ANSWERS &&	// Teacher's screen is showing answers
       QstInd == Event.Status.QstInd)		// Removing answer to the current question being played
      /***** Remove answer to this question *****/
      ExaEvt_RemoveMyAnswerToEventQuestion (&Event);

   /***** Show current exam event status *****/
   HTM_DIV_Begin ("id=\"exam event\" class=\"EXA_CONT\"");
   ExaEvt_ShowMatchStatusForStd (&Event,ExaEvt_CHANGE_STATUS_BY_STUDENT);
   HTM_DIV_End ();
  }

/*****************************************************************************/
/******************** Start exam event countdown (by a teacher) *******************/
/*****************************************************************************/

void ExaEvt_StartCountdown (void)
  {
   struct ExaEvt_Event Event;
   long NewCountdown;

   /***** Get countdown parameter ****/
   NewCountdown = Par_GetParToLong ("Countdown");

   /***** Remove old players.
          This function must be called by a teacher
          before getting exam event status. *****/
   ExaEvt_RemoveOldPlayers ();

   /***** Get data of the exam event from database *****/
   Event.EvtCod = ExaEvt_GetEvtCodBeingPlayed ();
   ExaEvt_GetDataOfEventByCod (&Event);

   /***** Start countdown *****/
   Event.Status.Countdown = NewCountdown;

   /***** Update exam event status in database *****/
   ExaEvt_UpdateEventStatusInDB (&Event);

   /***** Show current exam event status *****/
   ExaEvt_ShowRefreshablePartTch (&Event);
  }

/*****************************************************************************/
/****************** Refresh exam event for a teacher via AJAX *********************/
/*****************************************************************************/

void ExaEvt_RefreshEventTch (void)
  {
   struct ExaEvt_Event Event;
   enum {REFRESH_LEFT,REFRESH_ALL} WhatToRefresh;

   if (!Gbl.Session.IsOpen)	// If session has been closed, do not write anything
      return;

   /***** Remove old players.
          This function must be called by a teacher
          before getting exam event status. *****/
   ExaEvt_RemoveOldPlayers ();

   /***** Get data of the exam event from database *****/
   Event.EvtCod = ExaEvt_GetEvtCodBeingPlayed ();
   ExaEvt_GetDataOfEventByCod (&Event);

   /***** Update countdown *****/
   // If current countdown is < 0 ==> no countdown in progress
   WhatToRefresh = REFRESH_LEFT;
   if (Event.Status.Playing &&		// Event is being played
       Event.Status.Countdown >= 0)	// Countdown in progress
     {
      /* Decrease countdown */
      Event.Status.Countdown -= Cfg_SECONDS_TO_REFRESH_MATCH_TCH;

      /* On countdown reached, set exam event status to next (forward) status */
      if (Event.Status.Countdown <= 0)	// End of countdown reached
	{
	 ExaEvt_SetMatchStatusToNext (&Event);
	 WhatToRefresh = REFRESH_ALL;	// Refresh the whole page
	}
     }

   /***** Update exam event status in database *****/
   ExaEvt_UpdateEventStatusInDB (&Event);

   /***** Update elapsed time in this question *****/
   ExaEvt_UpdateElapsedTimeInQuestion (&Event);

   /***** Show current exam event status *****/
   switch (WhatToRefresh)
     {
      case REFRESH_LEFT:	// Refresh only left part
         HTM_Txt ("match_left|0|");	// 0 ==> do not evaluate MatJax scripts after updating HTML
         ExaEvt_ShowRefreshablePartTch (&Event);
         break;
      case REFRESH_ALL:		// Refresh the whole page
         HTM_Txt ("exam event|1|");		// 1 ==> evaluate MatJax scripts after updating HTML
         ExaEvt_ShowMatchStatusForTch (&Event);
         break;
     }
  }

/*****************************************************************************/
/*************** Refresh current exam for a student via AJAX *****************/
/*****************************************************************************/

void ExaEvt_RefreshEventStd (void)
  {
   struct ExaEvt_Event Event;

   if (!Gbl.Session.IsOpen)	// If session has been closed, do not write anything
      return;

   /***** Get data of the exam event from database *****/
   Event.EvtCod = ExaEvt_GetEvtCodBeingPlayed ();
   ExaEvt_GetDataOfEventByCod (&Event);

   /***** Show current exam event status *****/
   ExaEvt_ShowMatchStatusForStd (&Event,ExaEvt_REFRESH_STATUS_BY_SERVER);
  }

/*****************************************************************************/
/**** Receive previous question answer in an exam event question from database *****/
/*****************************************************************************/

void ExaEvt_GetQstAnsFromDB (long EvtCod,long UsrCod,unsigned QstInd,
		             struct ExaEvt_UsrAnswer *UsrAnswer)
  {
   MYSQL_RES *mysql_res;
   MYSQL_ROW row;
   unsigned NumRows;

   /***** Set default values for number of option and answer index *****/
   UsrAnswer->NumOpt = -1;	// < 0 ==> no answer selected
   UsrAnswer->AnsInd = -1;	// < 0 ==> no answer selected

   /***** Get student's answer *****/
   NumRows = (unsigned) DB_QuerySELECT (&mysql_res,"can not get user's answer to an exam event question",
					"SELECT NumOpt,"	// row[0]
					       "AnsInd"		// row[1]
					" FROM exa_answers"
					" WHERE EvtCod=%ld"
					" AND UsrCod=%ld"
					" AND QstInd=%u",
					EvtCod,UsrCod,QstInd);
   if (NumRows) // Answer found...
     {
      row = mysql_fetch_row (mysql_res);

      /***** Get number of option index (row[0]) *****/
      if (sscanf (row[0],"%d",&(UsrAnswer->NumOpt)) != 1)
	 Lay_ShowErrorAndExit ("Error when getting student's answer to an exam event question.");

      /***** Get answer index (row[1]) *****/
      if (sscanf (row[1],"%d",&(UsrAnswer->AnsInd)) != 1)
	 Lay_ShowErrorAndExit ("Error when getting student's answer to an exam event question.");
     }

   /***** Free structure that stores the query result *****/
   DB_FreeMySQLResult (&mysql_res);
  }

/*****************************************************************************/
/********* Receive question answer from student when playing an exam event *********/
/*****************************************************************************/

void ExaEvt_ReceiveQuestionAnswer (void)
  {
   struct ExaEvt_Event Event;
   unsigned QstInd;
   unsigned Indexes[Tst_MAX_OPTIONS_PER_QUESTION];
   struct ExaEvt_UsrAnswer PreviousUsrAnswer;
   struct ExaEvt_UsrAnswer UsrAnswer;
   struct TstRes_Result Result;

   /***** Get data of the exam event from database *****/
   Event.EvtCod = ExaEvt_GetEvtCodBeingPlayed ();
   ExaEvt_GetDataOfEventByCod (&Event);

   /***** Get question index from form *****/
   QstInd = Exa_GetParamQstInd ();

   /***** Check that teacher's screen is showing answers
          and question index is the current one being played *****/
   if (Event.Status.Showing == ExaEvt_ANSWERS &&	// Teacher's screen is showing answers
       QstInd == Event.Status.QstInd)		// Receiving an answer to the current question being played
     {
      /***** Get indexes for this question from database *****/
      ExaEvt_GetIndexes (Event.EvtCod,Event.Status.QstInd,Indexes);

      /***** Get answer index *****/
      /*
      Indexes[4] = {0,3,1,2}
      +-------+--------+----------+---------+
      | Button | Option | Answer   | Correct |
      | letter | number | index    |         |
      | screen | screen | database |         |
      +--------+--------+----------+---------+
      |   a    |    0   |    0     |         |
      |   b    |    1   |    3     |         |
      |   c    |    2   |    1     |    Y    | <---- User press button #2 (index = 1, correct)
      |   d    |    3   |    2     |         |
      +--------+--------+----------+---------+
      UsrAnswer.NumOpt = 2
      UsrAnswer.AnsInd = 1
      */
      UsrAnswer.NumOpt = ExaEvt_GetParamNumOpt ();
      UsrAnswer.AnsInd = Indexes[UsrAnswer.NumOpt];

      /***** Get previous student's answer to this question
	     (<0 ==> no answer) *****/
      ExaEvt_GetQstAnsFromDB (Event.EvtCod,Gbl.Usrs.Me.UsrDat.UsrCod,Event.Status.QstInd,
			   &PreviousUsrAnswer);

      /***** Store student's answer *****/
      if (UsrAnswer.NumOpt >= 0 &&
	  UsrAnswer.AnsInd >= 0 &&
	  UsrAnswer.AnsInd != PreviousUsrAnswer.AnsInd)
	 DB_QueryREPLACE ("can not register your answer to the exam event question",
			  "REPLACE exa_answers"
			  " (EvtCod,UsrCod,QstInd,NumOpt,AnsInd)"
			  " VALUES"
			  " (%ld,%ld,%u,%d,%d)",
			  Event.EvtCod,Gbl.Usrs.Me.UsrDat.UsrCod,Event.Status.QstInd,
			  UsrAnswer.NumOpt,
			  UsrAnswer.AnsInd);

      /***** Update student's exam event result *****/
      ExaRes_GetExamResultQuestionsFromDB (Event.EvtCod,Gbl.Usrs.Me.UsrDat.UsrCod,
					 &Result);
      ExaEvt_ComputeScore (&Result);

      Str_SetDecimalPointToUS ();	// To print the floating point as a dot
      if (DB_QueryCOUNT ("can not get if exam event result exists",
			 "SELECT COUNT(*) FROM exa_results"
			 " WHERE EvtCod=%ld AND UsrCod=%ld",
			 Event.EvtCod,Gbl.Usrs.Me.UsrDat.UsrCod))	// Result exists
	 /* Update result */
	 DB_QueryUPDATE ("can not update exam event result",
			  "UPDATE exa_results"
			  " SET EndTime=NOW(),"
			       "NumQsts=%u,"
			       "NumQstsNotBlank=%u,"
			       "Score='%.15lg'"
			  " WHERE EvtCod=%ld AND UsrCod=%ld",
			  Result.NumQsts,
			  Result.NumQstsNotBlank,
			  Result.Score,
			  Event.EvtCod,Gbl.Usrs.Me.UsrDat.UsrCod);
      else								// Result doesn't exist
	 /* Create result */
	 DB_QueryINSERT ("can not create exam event result",
			  "INSERT exa_results "
			  "(EvtCod,UsrCod,StartTime,EndTime,NumQsts,NumQstsNotBlank,Score)"
			  " VALUES "
			  "(%ld,"	// EvtCod
			  "%ld,"	// UsrCod
			  "NOW(),"	// StartTime
			  "NOW(),"	// EndTime
			  "%u,"		// NumQsts
			  "%u,"		// NumQstsNotBlank
			  "'%.15lg')",	// Score
			  Event.EvtCod,Gbl.Usrs.Me.UsrDat.UsrCod,
			  Result.NumQsts,
			  Result.NumQstsNotBlank,
			  Result.Score);
      Str_SetDecimalPointToLocal ();	// Return to local system
     }

   /***** Show current exam event status *****/
   HTM_DIV_Begin ("id=\"exam event\" class=\"EXA_CONT\"");
   ExaEvt_ShowMatchStatusForStd (&Event,ExaEvt_CHANGE_STATUS_BY_STUDENT);
   HTM_DIV_End ();
  }

/*****************************************************************************/
/********************* Remove answer to exam event question ***********************/
/*****************************************************************************/

static void ExaEvt_RemoveMyAnswerToEventQuestion (const struct ExaEvt_Event *Event)
  {
   DB_QueryDELETE ("can not remove your answer to the exam event question",
		    "DELETE FROM exa_answers"
		    " WHERE EvtCod=%ld AND UsrCod=%ld AND QstInd=%u",
		    Event->EvtCod,Gbl.Usrs.Me.UsrDat.UsrCod,Event->Status.QstInd);
  }

/*****************************************************************************/
/******************** Compute exam event score for a student **********************/
/*****************************************************************************/

static void ExaEvt_ComputeScore (struct TstRes_Result *Result)
  {
   unsigned NumQst;
   struct Tst_Question Question;

   for (NumQst = 0, Result->Score = 0.0;
	NumQst < Result->NumQsts;
	NumQst++)
     {
      /***** Create test question *****/
      Tst_QstConstructor (&Question);
      Question.QstCod = Result->Questions[NumQst].QstCod;
      Question.Answer.Type = Tst_ANS_UNIQUE_CHOICE;

      /***** Compute score for this answer ******/
      TstRes_ComputeChoiceAnsScore (Result,NumQst,&Question);

      /***** Update total score *****/
      Result->Score += Result->Questions[NumQst].Score;

      /***** Destroy test question *****/
      Tst_QstDestructor (&Question);
     }
  }

/*****************************************************************************/
/********** Get number of users who answered a question in an exam event ***********/
/*****************************************************************************/

unsigned ExaEvt_GetNumUsrsWhoAnsweredQst (long EvtCod,unsigned QstInd)
  {
   /***** Get number of users who answered
          a question in an exam event from database *****/
   return
   (unsigned) DB_QueryCOUNT ("can not get number of users who answered a question",
			     "SELECT COUNT(*) FROM exa_answers"
			     " WHERE EvtCod=%ld AND QstInd=%u",
			     EvtCod,QstInd);
  }

/*****************************************************************************/
/** Get number of users who have chosen a given answer of an exam question ***/
/*****************************************************************************/

unsigned ExaEvt_GetNumUsrsWhoHaveChosenAns (long EvtCod,unsigned QstInd,unsigned AnsInd)
  {
   /***** Get number of users who have chosen
          an answer of a question from database *****/
   return
   (unsigned) DB_QueryCOUNT ("can not get number of users who have chosen an answer",
			     "SELECT COUNT(*) FROM exa_answers"
			     " WHERE EvtCod=%ld AND QstInd=%u AND AnsInd=%u",
			     EvtCod,QstInd,AnsInd);
  }

/*****************************************************************************/
/**** Get number of users who have answered any question in an exam event ****/
/*****************************************************************************/

static unsigned ExaEvt_GetNumUsrsWhoHaveAnswerEvt (long EvtCod)
  {
   /***** Get number of users who have answered
          any question in exam event from database *****/
   return
   (unsigned) DB_QueryCOUNT ("can not get number of users who have answered an exam event",
			     "SELECT COUNT(DISTINCT UsrCod) FROM exa_answers"
			     " WHERE EvtCod=%ld",
			     EvtCod);
  }

/*****************************************************************************/
/***************** Draw a bar with the percentage of answers *****************/
/*****************************************************************************/

#define ExaEvt_MAX_BAR_WIDTH 100

void ExaEvt_DrawBarNumUsrs (unsigned NumRespondersAns,unsigned NumRespondersQst,bool Correct)
  {
   extern const char *Txt_of_PART_OF_A_TOTAL;
   unsigned i;
   unsigned BarWidth = 0;

   /***** Start container *****/
   HTM_DIV_Begin ("class=\"EXA_RESULT\"");

   /***** Draw bar with a with proportional to the number of clicks *****/
   if (NumRespondersAns && NumRespondersQst)
      BarWidth = (unsigned) ((((double) NumRespondersAns * (double) ExaEvt_MAX_BAR_WIDTH) /
	                       (double) NumRespondersQst) + 0.5);

   /***** Bar proportional to number of users *****/
   HTM_TABLE_BeginWide ();
   HTM_TR_Begin ("class=\"EXA_RES_TR\"");
   for (i = 0;
	i < 100;
	i++)
     {
      HTM_TD_Begin ("class=\"%s\"",
		    (i < BarWidth) ? (Correct ? "EXA_RES_CORRECT" :
					        "EXA_RES_WRONG") :
				     "EXA_RES_VOID");
      HTM_TD_End ();
     }
   HTM_TR_End ();
   HTM_TABLE_End ();

   /***** Write the number of users *****/
   if (NumRespondersAns && NumRespondersQst)
      HTM_TxtF ("%u&nbsp;(%u%%&nbsp;%s&nbsp;%u)",
		NumRespondersAns,
		(unsigned) ((((double) NumRespondersAns * 100.0) / (double) NumRespondersQst) + 0.5),
		Txt_of_PART_OF_A_TOTAL,NumRespondersQst);
   else
      HTM_NBSP ();

   /***** End container *****/
   HTM_DIV_End ();
  }