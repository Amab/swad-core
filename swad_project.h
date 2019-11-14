// swad_project.h: projects (final degree projects, thesis)

#ifndef _SWAD_PRJ
#define _SWAD_PRJ
/*
    SWAD (Shared Workspace At a Distance),
    is a web platform developed at the University of Granada (Spain),
    and used to support university teaching.

    This file is part of SWAD core.
    Copyright (C) 1999-2019 Antonio Ca�as Vargas

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

#include "swad_date.h"
#include "swad_file_browser.h"
#include "swad_notification.h"
#include "swad_user.h"

/*****************************************************************************/
/************************** Public types and constants ***********************/
/*****************************************************************************/

/***** Configuration *****/
#define Prj_EDITABLE_DEFAULT true

/***** Filters to list projects *****/
/* Whose projects */
#define Prj_FILTER_WHO_DEFAULT	Usr_WHO_ME

/* Assigned projects / non-assigned projects */
#define Prj_NUM_ASSIGNED_NONASSIG 2
typedef enum
  {
   Prj_ASSIGNED = 0,
   Prj_NONASSIG = 1,
  } Prj_AssignedNonassig_t;
#define Prj_NEW_PRJ_ASSIGNED_NONASSIG_DEFAULT Prj_NONASSIG
#define Prj_FILTER_ASSIGNED_DEFAULT	(1 << Prj_ASSIGNED)	// on
#define Prj_FILTER_NONASSIG_DEFAULT	(1 << Prj_NONASSIG)	// on

/* Locked/unlocked project */
#define Prj_NUM_LOCKED_UNLOCKED 2
typedef enum
  {
   Prj_LOCKED   = 0,
   Prj_UNLOCKED = 1,
  } Prj_Locked_t;

/* Hidden/visible project */
#define Prj_NUM_HIDDEN_VISIBL 2
typedef enum
  {
   Prj_HIDDEN = 0,
   Prj_VISIBL = 1,
  } Prj_HiddenVisibl_t;
#define Prj_NEW_PRJ_HIDDEN_VISIBL_DEFAULT Prj_VISIBL
#define Prj_FILTER_HIDDEN_DEFAULT	(1 << Prj_HIDDEN)	// on
#define Prj_FILTER_VISIBL_DEFAULT	(1 << Prj_VISIBL)	// on

/* Faulty/faultless project */
#define Prj_NUM_FAULTINESS 2
typedef enum
  {
   Prj_FAULTY    = 0,
   Prj_FAULTLESS = 1,
  } Prj_Faultiness_t;
#define Prj_FILTER_FAULTY_DEFAULT	(1 << Prj_FAULTY)	// on
#define Prj_FILTER_FAULTLESS_DEFAULT	(1 << Prj_FAULTLESS)	// on

/* Project department */
#define Prj_FILTER_DPT_DEFAULT -1L	// Any department

/* Struct with all filters */
struct Prj_Filter
  {
   Usr_Who_t Who;		// Show my / selected users' / all projects
   unsigned Assign;		// Show assigned / non assigned projects
   unsigned Hidden;		// Show hidden / visible projects
   unsigned Faulti;		// Show faulty / faultless projects
   long DptCod;			// Show projects of this department
  };

/***** Order listing of projects by... *****/
#define Prj_NUM_ORDERS 4
typedef enum
  {
   Prj_ORDER_START_TIME = 0,
   Prj_ORDER_END_TIME   = 1,
   Prj_ORDER_TITLE      = 2,
   Prj_ORDER_DEPARTMENT = 3,
  } Prj_Order_t;
#define Prj_ORDER_DEFAULT Prj_ORDER_START_TIME

/***** Project title *****/
#define Prj_MAX_CHARS_PROJECT_TITLE	(128 - 1)	// 127
#define Prj_MAX_BYTES_PROJECT_TITLE	((Prj_MAX_CHARS_PROJECT_TITLE       + 1) * Str_MAX_BYTES_PER_CHAR - 1)	// 2047

/***** Type of proposal ******/
#define Prj_NUM_PROPOSAL_TYPES 3
typedef enum
  {
   Prj_PROPOSAL_NEW,
   Prj_PROPOSAL_MODIFIED,
   Prj_PROPOSAL_UNMODIFIED,
  } Prj_Proposal_t;
#define Prj_PROPOSAL_DEFAULT Prj_PROPOSAL_NEW

/***** User roles in a project *****/
// Don't change these numbers! They are used in database
#define Prj_NUM_ROLES_IN_PROJECT 4
typedef enum
  {
   Prj_ROLE_UNK	= 0,	// Unknown
   Prj_ROLE_STD	= 1,	// Student
   Prj_ROLE_TUT	= 2,	// Tutor
   Prj_ROLE_EVL	= 3,	// Evaluator
  } Prj_RoleInProject_t;

/***** Struct to store a project *****/
struct Project
  {
   long PrjCod;
   long CrsCod;
   Prj_Locked_t Locked;
   Prj_HiddenVisibl_t Hidden;
   Prj_AssignedNonassig_t Assigned;
   unsigned NumStds;
   Prj_Proposal_t Proposal;
   time_t CreatTime;
   time_t ModifTime;
   char Title[Prj_MAX_BYTES_PROJECT_TITLE + 1];
   long DptCod;
   char *Description;
   char *Knowledge;
   char *Materials;
   char URL[Cns_MAX_BYTES_WWW + 1];
  };

/*****************************************************************************/
/***************************** Public prototypes *****************************/
/*****************************************************************************/

void Prj_ListUsrsToSelect (void);
void Prj_SeeProjects (void);
void Prj_ShowTableSelectedPrjs (void);

void Prj_PutParams (struct Prj_Filter *Filter,
                    Prj_Order_t Order,
                    unsigned NumPage,
                    long PrjCod);

void Prj_ShowOneUniqueProject (struct Project *Prj);

void Prj_PrintOneProject (void);

void Prj_FlushCacheMyRolesInProject (void);
Prj_RoleInProject_t Prj_GetMyRolesInProject (long PrjCod);

void Prj_ReqAddStds (void);
void Prj_ReqAddTuts (void);
void Prj_ReqAddEvls (void);

void Prj_GetSelectedUsrsAndAddStds (void);
void Prj_GetSelectedUsrsAndAddTuts (void);
void Prj_GetSelectedUsrsAndAddEvls (void);

void Prj_ReqRemStd (void);
void Prj_ReqRemTut (void);
void Prj_ReqRemEvl (void);

void Prj_RemStd (void);
void Prj_RemTut (void);
void Prj_RemEvl (void);

void Prj_PutHiddenParamPrjOrder (void);

bool Prj_CheckIfICanViewProjectFiles (unsigned MyRolesInProject);

void Prj_RequestCreatePrj (void);
void Prj_RequestEditPrj (void);

void Prj_AllocMemProject (struct Project *Prj);
void Prj_FreeMemProject (struct Project *Prj);

void Prj_GetDataOfProjectByCod (struct Project *Prj);
long Prj_GetCourseOfProject (long PrjCod);
void Prj_FreeListProjects (void);

void Prj_PutParamPrjCod (long PrjCod);
long Prj_GetParamPrjCod (void);
void Prj_ReqRemProject (void);
void Prj_RemoveProject (void);
void Prj_HideProject (void);
void Prj_ShowProject (void);

void Prj_RecFormProject (void);

void Prj_ShowFormConfig (void);
void Prj_ReceiveConfigPrj (void);
void Prj_ReqLockSelectedPrjsEdition (void);
void Prj_ReqUnloSelectedPrjsEdition (void);
void Prj_LockSelectedPrjsEdition (void);
void Prj_UnloSelectedPrjsEdition (void);
void Prj_LockProjectEdition (void);
void Prj_UnloProjectEdition (void);

void Prj_RemoveCrsProjects (long CrsCod);
void Prj_RemoveUsrFromProjects (long UsrCod);

unsigned Prj_GetNumCoursesWithProjects (Hie_Level_t Scope);
unsigned Prj_GetNumProjects (Hie_Level_t Scope);

#endif
