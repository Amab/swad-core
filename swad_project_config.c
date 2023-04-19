// swad_project_config.c: projects configuration
/*
    SWAD (Shared Workspace At a Distance),
    is a web platform developed at the University of Granada (Spain),
    and used to support university teaching.

    This file is part of SWAD core.
    Copyright (C) 1999-2023 Antonio Ca�as Vargas

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

#include <stdbool.h>		// For boolean type

#include "swad_action_list.h"
#include "swad_alert.h"
#include "swad_box.h"
#include "swad_database.h"
#include "swad_form.h"
#include "swad_global.h"
#include "swad_parameter.h"
#include "swad_project.h"
#include "swad_project_config.h"
#include "swad_project_database.h"
#include "swad_rubric.h"
#include "swad_rubric_database.h"

/*****************************************************************************/
/**************************** Private constants ******************************/
/*****************************************************************************/

static const char *PrjCfg_RubricIDs[PrjCfg_NUM_RUBRICS] =
  {
   [PrjCfg_RUBRIC_TUT] = "rub_tut",
   [PrjCfg_RUBRIC_EVL] = "rub_evl",
   [PrjCfg_RUBRIC_GBL] = "rub_gbl",
  };

/*****************************************************************************/
/************** External global variables from others modules ****************/
/*****************************************************************************/

extern struct Globals Gbl;

/*****************************************************************************/
/************************* Private global variables **************************/
/*****************************************************************************/

// static struct Rub_Rubric PrjCfg_Rubrics[PrjCfg_NUM_RUBRICS];

/*****************************************************************************/
/***************************** Private prototypes ****************************/
/*****************************************************************************/

static void PrjCfg_ShowFormNETCanCreate (const struct PrjCfg_Config *Config);
static void PrjCfg_ShowFormRubrics (const struct PrjCfg_Config *Config);
static void PrjCfg_ShowFormRubric (const struct PrjCfg_Config *Config,
                                   const struct Rub_Rubrics *Rubrics,
                                   PrjCfg_Rubric_t WhichRubric);

static void PrjCfg_GetConfigDataFromRow (MYSQL_RES *mysql_res,
				         struct PrjCfg_Config *Config);
static bool PrjCfg_GetIfNETCanCreateFromForm (void);

/*****************************************************************************/
/************** Get configuration of projects for current course *************/
/*****************************************************************************/

void PrjCfg_GetConfig (struct PrjCfg_Config *Config)
  {
   MYSQL_RES *mysql_res;
   PrjCfg_Rubric_t WhichRubric;

   /***** Get configuration of projects for current course from database *****/
   if (Prj_DB_GetConfig (&mysql_res))
      PrjCfg_GetConfigDataFromRow (mysql_res,Config);
   else
     {
      for (WhichRubric  = (PrjCfg_Rubric_t) 0;
	   WhichRubric <= (PrjCfg_Rubric_t) (PrjCfg_NUM_RUBRICS - 1);
	   WhichRubric++)
	 Config->RubCod[WhichRubric] = -1L;
      Config->NETCanCreate = PrjCfg_NET_CAN_CREATE_DEFAULT;
     }

   /***** Free structure that stores the query result *****/
   DB_FreeMySQLResult (&mysql_res);
  }

/*****************************************************************************/
/************************ Can I configure all projects? **********************/
/*****************************************************************************/

bool PrjCfg_CheckIfICanConfig (void)
  {
   static const bool ICanConfigAllProjects[Rol_NUM_ROLES] =
     {
      [Rol_TCH	  ] = true,
      [Rol_SYS_ADM] = true,
     };

   return ICanConfigAllProjects[Gbl.Usrs.Me.Role.Logged];
  }

/*****************************************************************************/
/********************** Configuration of all projects ************************/
/*****************************************************************************/

void PrjCfg_ShowFormConfig (void)
  {
   extern const char *Hlp_ASSESSMENT_Projects;
   extern const char *Txt_Configure_projects;
   extern const char *Txt_Save_changes;
   struct Prj_Projects Projects;

   /***** Reset projects *****/
   Prj_ResetPrjsAndReadConfig (&Projects);

   /***** Begin box *****/
   Box_BoxBegin (NULL,Txt_Configure_projects,
                 Prj_PutIconsListProjects,&Projects,
                 Hlp_ASSESSMENT_Projects,Box_NOT_CLOSABLE);

      /***** Begin form *****/
      Frm_BeginForm (ActRcvCfgPrj);

	 HTM_TABLE_BeginCenterPadding (2);

	    /***** Projects are editable by non-editing teachers? *****/
	    PrjCfg_ShowFormNETCanCreate (&Projects.Config);

	    /***** Rubrics *****/
	    PrjCfg_ShowFormRubrics (&Projects.Config);

	 HTM_TABLE_End ();

	 /***** Send button *****/
	 Btn_PutConfirmButton (Txt_Save_changes);

      /***** End form *****/
      Frm_EndForm ();

   /***** End box *****/
   Box_BoxEnd ();
  }

/*****************************************************************************/
/************ Show form to edit rubrics associated to projects ***************/
/*****************************************************************************/

static void PrjCfg_ShowFormRubrics (const struct PrjCfg_Config *Config)
  {
   struct Rub_Rubrics Rubrics;
   PrjCfg_Rubric_t WhichRubric;

   /***** Reset rubrics context *****/
   Rub_ResetRubrics (&Rubrics);

   /***** Get list of rubrics *****/
   Rub_GetListRubrics (&Rubrics);

   /***** Rubric selectors *****/
   for (WhichRubric  = (PrjCfg_Rubric_t) 0;
	WhichRubric <= (PrjCfg_Rubric_t) (PrjCfg_NUM_RUBRICS - 1);
	WhichRubric++)
      PrjCfg_ShowFormRubric (Config,&Rubrics,WhichRubric);

   /***** Free list of rubrics *****/
   Rub_FreeListRubrics (&Rubrics);
  }

/*****************************************************************************/
/******** Show form to edit one of the rubrics associated to projects ********/
/*****************************************************************************/

static void PrjCfg_ShowFormRubric (const struct PrjCfg_Config *Config,
                                   const struct Rub_Rubrics *Rubrics,
                                   PrjCfg_Rubric_t WhichRubric)
  {
   extern const char *Txt_PROJECT_RUBRIC[PrjCfg_NUM_RUBRICS];
   extern const char *Txt_no_rubric;
   unsigned NumRub;
   char Title[Rub_MAX_BYTES_TITLE + 1];
   long RubCodInConfig = Config->RubCod[WhichRubric];

   /***** Row with form for rubric *****/
   HTM_TR_Begin (NULL);

      /* Label */
      HTM_TD_Begin ("class=\"RM FORM_IN_%s\"",The_GetSuffix ());
	 HTM_TxtColon (Txt_PROJECT_RUBRIC[WhichRubric]);
      HTM_TD_End ();

      /* Selector of rubrics */
      HTM_TD_Begin ("class=\"LM\"");
	 HTM_SELECT_Begin (HTM_DONT_SUBMIT_ON_CHANGE,NULL,
			   "id=\"%s\" name=\"%s\" class=\"PrjCfg_RUBRIC_SEL\"",
			   PrjCfg_RubricIDs[WhichRubric],
			   PrjCfg_RubricIDs[WhichRubric]);

	    /* First option to indicate that no rubric is selected */
	    HTM_OPTION (HTM_Type_STRING,"-1",
			RubCodInConfig <= 0,			// Selected?
			HTM_OPTION_ENABLED,
			"[%s]",Txt_no_rubric);

	    /* One option for each rubric in this course */
	    for (NumRub = 0;
		 NumRub < Rubrics->Num;
		 NumRub++)
	      {
	       Rub_DB_GetRubricTitle (Rubrics->Lst[NumRub],Title,Rub_MAX_BYTES_TITLE);
	       HTM_OPTION (HTM_Type_LONG,&Rubrics->Lst[NumRub],
			   Rubrics->Lst[NumRub] == RubCodInConfig,	// Selected?
			   HTM_OPTION_ENABLED,
			   "%s",Title);
	      }

	 HTM_SELECT_End ();
      HTM_TD_End ();

   HTM_TR_End ();
  }

/*****************************************************************************/
/************ Get configuration values from a database table row *************/
/*****************************************************************************/

static void PrjCfg_GetConfigDataFromRow (MYSQL_RES *mysql_res,
				         struct PrjCfg_Config *Config)
  {
   MYSQL_ROW row;
   PrjCfg_Rubric_t WhichRubric;

   /***** Get row *****/
   row = mysql_fetch_row (mysql_res);
   /*
   row[0]	RubTutCod
   row[1]	RubEvlCod
   row[2]	RubGblCod
   row[3]	NETCanCreate
   */
   /***** Get rubric codes (row[0], row[1], row[2] *****/
   for (WhichRubric  = (PrjCfg_Rubric_t) 0;
	WhichRubric <= (PrjCfg_Rubric_t) (PrjCfg_NUM_RUBRICS - 1);
	WhichRubric++)
      Config->RubCod[WhichRubric] = Str_ConvertStrCodToLongCod (row[WhichRubric]);

   /***** Get whether non-editing teachers can create new projects or not (row[3]) *****/
   Config->NETCanCreate = (row[3][0] == 'Y');
  }

/*****************************************************************************/
/****** Show form to edit if non-editing teachers create new projects ********/
/*****************************************************************************/

static void PrjCfg_ShowFormNETCanCreate (const struct PrjCfg_Config *Config)
  {
   extern const char *Txt_Create_project;
   extern const char *Txt_Non_editing_teachers_can_create_new_projects;

   HTM_TR_Begin (NULL);

      /***** Label *****/
      HTM_TD_Begin ("class=\"RM FORM_IN_%s\"",The_GetSuffix ());
	 HTM_TxtColon (Txt_Create_project);
      HTM_TD_End ();

      /***** Data *****/
      HTM_TD_Begin ("class=\"LM\"");
	 HTM_LABEL_Begin ("class=\"DAT_%s\"",The_GetSuffix ());
	    HTM_INPUT_CHECKBOX ("NETCanCreate",HTM_DONT_SUBMIT_ON_CHANGE,
				"id=\"NETCanCreate\" value=\"Y\"%s",
				Config->NETCanCreate ? " checked=\"checked\"" :
						       "");
	    HTM_Txt (Txt_Non_editing_teachers_can_create_new_projects);
	 HTM_LABEL_End ();
      HTM_TD_End ();

   HTM_TR_End ();
  }

/*****************************************************************************/
/************ Receive configuration of projects for current course ***********/
/*****************************************************************************/

void PrjCfg_ReceiveConfig (void)
  {
   extern const char *Txt_The_configuration_of_the_projects_has_been_updated;
   struct Prj_Projects Projects;
   PrjCfg_Rubric_t WhichRubric;

   /***** Reset projects *****/
   Prj_ResetPrjsAndReadConfig (&Projects);

   /***** Get rubric codes *****/
   for (WhichRubric  = (PrjCfg_Rubric_t) 0;
	WhichRubric <= (PrjCfg_Rubric_t) (PrjCfg_NUM_RUBRICS - 1);
	WhichRubric++)
      Projects.Config.RubCod[WhichRubric] = Par_GetParLong (PrjCfg_RubricIDs[WhichRubric]);

   /***** Get non-editing teachers can create new projects or not *****/
   Projects.Config.NETCanCreate = PrjCfg_GetIfNETCanCreateFromForm ();

   /***** Update database *****/
   Prj_DB_UpdateConfig (&Projects);

   /***** Show confirmation message *****/
   Ale_ShowAlert (Ale_SUCCESS,Txt_The_configuration_of_the_projects_has_been_updated);

   /***** Show again the form to configure projects *****/
   PrjCfg_ShowFormConfig ();
  }

/*****************************************************************************/
/****** Get if projects are creatable by non-editing teachers from form *******/
/*****************************************************************************/

static bool PrjCfg_GetIfNETCanCreateFromForm (void)
  {
   return Par_GetParBool ("NETCanCreate");
  }