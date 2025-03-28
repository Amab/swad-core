// swad_rubric_resource.c: links to rubrics as resources

/*
    SWAD (Shared Workspace At a Distance),
    is a web platform developed at the University of Granada (Spain),
    and used to support university teaching.

    This file is part of SWAD core.
    Copyright (C) 1999-2025 Antonio Ca�as Vargas

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
/********************************* Headers ***********************************/
/*****************************************************************************/

#include "swad_action_list.h"
#include "swad_alert.h"
#include "swad_box.h"
#include "swad_error.h"
#include "swad_parameter_code.h"
#include "swad_resource.h"
#include "swad_resource_database.h"
#include "swad_rubric.h"
#include "swad_rubric_database.h"
#include "swad_rubric_resource.h"

/*****************************************************************************/
/***************************** Private prototypes ****************************/
/*****************************************************************************/

static void RubRsc_ShowClipboard (void);

static void RubRsc_PutIconsClipboard (__attribute__((unused)) void *Args);

/*****************************************************************************/
/**************************** Get link to rubric *****************************/
/*****************************************************************************/

void RubRsc_GetLinkToRubric (void)
  {
   extern const char *Txt_Link_to_resource_X_copied_into_clipboard;
   extern const char *Txt_Rubrics;
   struct Rub_Rubrics Rubrics;

   /***** Reset rubrics context *****/
   Rub_ResetRubrics (&Rubrics);
   Rub_RubricConstructor (&Rubrics.Rubric);

   /***** Get parameters *****/
   Rub_GetPars (&Rubrics,Rub_DONT_CHECK_RUB_COD);

   /***** Get rubric data *****/
   if (Rubrics.Rubric.RubCod > 0)
      Rub_GetRubricDataByCod (&Rubrics.Rubric);

   /***** Copy link to rubric into resource clipboard *****/
   Rsc_DB_CopyToClipboard (Rsc_RUBRIC,Rubrics.Rubric.RubCod);

   /***** Write success message *****/
   Ale_ShowAlert (Ale_SUCCESS,Txt_Link_to_resource_X_copied_into_clipboard,
   		  Rubrics.Rubric.RubCod > 0 ? Rubrics.Rubric.Title :
   					      Txt_Rubrics);

   /***** Show rubrics again *****/
   Rub_ListAllRubrics (&Rubrics);

   /***** Free memory used for rubric *****/
   Rub_RubricDestructor (&Rubrics.Rubric);
  }

/*****************************************************************************/
/******************** Show clipboard on top of rubrics ***********************/
/*****************************************************************************/

void RubRsc_ViewResourceClipboard (void)
  {
   /***** View resource clipboard *****/
   RubRsc_ShowClipboard ();

   /***** Show all rubrics *****/
   Rub_SeeAllRubrics ();
  }

/*****************************************************************************/
/************************* Show resources clipboard **************************/
/*****************************************************************************/

static void RubRsc_ShowClipboard (void)
  {
   extern const char *Hlp_ASSESSMENT_Rubrics_resource_clipboard;
   extern const char *Txt_Clipboard;

   Box_BoxBegin (Txt_Clipboard,RubRsc_PutIconsClipboard,NULL,
		 Hlp_ASSESSMENT_Rubrics_resource_clipboard,Box_CLOSABLE);
      Rsc_ShowClipboard ();
   Box_BoxEnd ();
  }

/*****************************************************************************/
/****** Put contextual icons when showing resource clipboard in rubrics ******/
/*****************************************************************************/

static void RubRsc_PutIconsClipboard (__attribute__((unused)) void *Args)
  {
   /***** Put icon to remove resource clipboard in rubrics *****/
   if (Rub_CheckIfICanEditRubrics () == Usr_CAN)
      if (Rsc_DB_GetNumResourcesInClipboard ())	// Only if there are resources
	 Ico_PutContextualIconToRemove (ActRemRscCliRub,NULL,
					NULL,NULL);
  }

/*****************************************************************************/
/******************* Remove clipboard and show program ***********************/
/*****************************************************************************/

void RubRsc_RemoveResourceClipboard (void)
  {
   extern const char *Txt_Clipboard_removed;

   /***** Remove resource clipboard *****/
   Rsc_DB_RemoveClipboard ();
   Ale_ShowAlert (Ale_SUCCESS,Txt_Clipboard_removed);

   /***** View resource clipboard again *****/
   RubRsc_ViewResourceClipboard ();
  }
