// swad_figure.c: figures (global stats)

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

#include "swad_action.h"
#include "swad_agenda.h"
#include "swad_box.h"
#include "swad_calendar.h"
#include "swad_cookie.h"
#include "swad_exam.h"
#include "swad_figure.h"
#include "swad_follow.h"
#include "swad_form.h"
#include "swad_global.h"
#include "swad_hierarchy.h"
#include "swad_HTML.h"
#include "swad_message.h"
#include "swad_network.h"
#include "swad_notice.h"
#include "swad_privacy.h"
#include "swad_program.h"
#include "swad_project.h"
#include "swad_survey.h"
#include "swad_test.h"
#include "swad_theme.h"
#include "swad_timeline.h"

/*****************************************************************************/
/************** External global variables from others modules ****************/
/*****************************************************************************/

extern struct Globals Gbl;

/*****************************************************************************/
/****************************** Private prototypes ***************************/
/*****************************************************************************/

static void Fig_ReqShowFigure (Fig_FigureType_t SelectedFigureType);

static void Fig_PutHiddenParamFigureType (Fig_FigureType_t FigureType);
static void Fig_PutHiddenParamScopeFig (HieLvl_Level_t ScopeFig);

/*****************************************************************************/
/************************** Show use of the platform *************************/
/*****************************************************************************/

void Fig_ReqShowFigures (void)
  {
   Fig_ReqShowFigure (Fig_FIGURE_TYPE_DEF);
  }

static void Fig_ReqShowFigure (Fig_FigureType_t SelectedFigureType)
  {
   extern const char *Hlp_ANALYTICS_Figures;
   extern const char *The_ClassFormInBox[The_NUM_THEMES];
   extern const char *The_ClassInput[The_NUM_THEMES];
   extern const char *Txt_Figures;
   extern const char *Txt_Scope;
   extern const char *Txt_Statistic;
   extern const char *Txt_FIGURE_TYPES[Fig_NUM_FIGURES];
   extern const char *Txt_Show_statistic;
   Fig_FigureType_t FigType;
   unsigned FigureTypeUnsigned;

   /***** Form to show statistic *****/
   Frm_BeginForm (ActSeeUseGbl);

      /***** Begin box *****/
      Box_BoxBegin (NULL,Txt_Figures,
		    NULL,NULL,
		    Hlp_ANALYTICS_Figures,Box_NOT_CLOSABLE);

	 /***** Compute stats for anywhere, degree or course? *****/
	 HTM_LABEL_Begin ("class=\"%s\"",The_ClassFormInBox[Gbl.Prefs.Theme]);
	    HTM_TxtColonNBSP (Txt_Scope);
	    Gbl.Scope.Allowed = 1 << HieLvl_SYS |
				1 << HieLvl_CTY |
				1 << HieLvl_INS |
				1 << HieLvl_CTR |
				1 << HieLvl_DEG |
				1 << HieLvl_CRS;
	    Gbl.Scope.Default = HieLvl_SYS;
	    Sco_GetScope ("ScopeFig");
	    Sco_PutSelectorScope ("ScopeFig",HTM_DONT_SUBMIT_ON_CHANGE);
	 HTM_LABEL_End ();

	 HTM_BR ();

	 /***** Type of statistic *****/
	 HTM_LABEL_Begin ("class=\"%s\"",The_ClassFormInBox[Gbl.Prefs.Theme]);
	    HTM_TxtColonNBSP (Txt_Statistic);
	    HTM_SELECT_Begin (HTM_DONT_SUBMIT_ON_CHANGE,
			      "name=\"FigureType\" class=\"%s\"",
			      The_ClassInput[Gbl.Prefs.Theme]);
	       for (FigType  = (Fig_FigureType_t) 0;
		    FigType <= (Fig_FigureType_t) (Fig_NUM_FIGURES - 1);
		    FigType++)
		 {
		  FigureTypeUnsigned = (unsigned) FigType;
		  HTM_OPTION (HTM_Type_UNSIGNED,&FigureTypeUnsigned,
			      FigType == SelectedFigureType,false,
			      "%s",Txt_FIGURE_TYPES[FigType]);
		 }
	    HTM_SELECT_End ();
	 HTM_LABEL_End ();

      /***** Send button and end box *****/
      Box_BoxWithButtonEnd (Btn_CONFIRM_BUTTON,Txt_Show_statistic);

   /***** End form *****/
   Frm_EndForm ();
  }

/*****************************************************************************/
/************************* Put icon to show a figure *************************/
/*****************************************************************************/

void Fig_PutIconToShowFigure (Fig_FigureType_t FigureType)
  {
   extern const char *Txt_Show_statistic;
   struct Fig_Figures Figures;

   /***** Set default scope (used only if Gbl.Scope.Current is unknown) *****/
   Gbl.Scope.Default = HieLvl_CRS;
   Sco_AdjustScope ();

   /***** Put icon to show figure *****/
   Figures.Scope      = Gbl.Scope.Current;
   Figures.FigureType = FigureType;
   Lay_PutContextualLinkOnlyIcon (ActSeeUseGbl,NULL,
                                  Fig_PutHiddenParamFigures,&Figures,
				  "chart-pie.svg",Ico_BLACK,
				  Txt_Show_statistic);
  }

/*****************************************************************************/
/************* Put hidden parameters for figures (statistics) ****************/
/*****************************************************************************/

void Fig_PutHiddenParamFigures (void *Figures)
  {
   if (Figures)
     {
      Fig_PutHiddenParamScopeFig (((struct Fig_Figures *) Figures)->Scope);
      Fig_PutHiddenParamFigureType (((struct Fig_Figures *) Figures)->FigureType);
     }
  }

/*****************************************************************************/
/********* Put hidden parameter for the type of figure (statistic) ***********/
/*****************************************************************************/

static void Fig_PutHiddenParamFigureType (Fig_FigureType_t FigureType)
  {
   Par_PutHiddenParamUnsigned (NULL,"FigureType",(unsigned) FigureType);
  }

/*****************************************************************************/
/********* Put hidden parameter for the type of figure (statistic) ***********/
/*****************************************************************************/

static void Fig_PutHiddenParamScopeFig (HieLvl_Level_t ScopeFig)
  {
   Sco_PutParamScope ("ScopeFig",ScopeFig);
  }

/*****************************************************************************/
/************************** Show use of the platform *************************/
/*****************************************************************************/

void Fig_ShowFigures (void)
  {
   static void (*Fig_Function[Fig_NUM_FIGURES])(void) =	// Array of pointers to functions
     {
      [Fig_USERS            ] = Usr_GetAndShowUsersStats,
      [Fig_USERS_RANKING    ] = Usr_GetAndShowUsersRanking,
      [Fig_HIERARCHY        ] = Hie_GetAndShowHierarchyStats,
      [Fig_INSTITS          ] = Ins_GetAndShowInstitutionsStats,
      [Fig_DEGREE_TYPES     ] = DegTyp_GetAndShowDegreeTypesStats,
      [Fig_FOLDERS_AND_FILES] = Brw_GetAndShowFileBrowsersStats,
      [Fig_OER              ] = Brw_GetAndShowOERsStats,
      [Fig_COURSE_PROGRAMS  ] = Prg_GetAndShowCourseProgramStats,
      [Fig_ASSIGNMENTS      ] = Asg_GetAndShowAssignmentsStats,
      [Fig_PROJECTS         ] = Prj_GetAndShowProjectsStats,
      [Fig_TESTS            ] = Tst_GetAndShowTestsStats,
      [Fig_EXAMS            ] = Exa_GetAndShowExamsStats,
      [Fig_GAMES            ] = Gam_GetAndShowGamesStats,
      [Fig_SURVEYS          ] = Svy_GetAndShowSurveysStats,
      [Fig_TIMELINE         ] = Tml_GetAndShowTimelineActivityStats,
      [Fig_FOLLOW           ] = Fol_GetAndShowFollowStats,
      [Fig_FORUMS           ] = For_GetAndShowForumStats,
      [Fig_NOTIFY_EVENTS    ] = Ntf_GetAndShowNumUsrsPerNotifyEvent,
      [Fig_NOTICES          ] = Not_GetAndShowNoticesStats,
      [Fig_MESSAGES         ] = Msg_GetAndShowMsgsStats,
      [Fig_AGENDAS          ] = Agd_GetAndShowAgendasStats,
      [Fig_SOCIAL_NETWORKS  ] = Net_ShowWebAndSocialNetworksStats,
      [Fig_LANGUAGES        ] = Lan_GetAndShowNumUsrsPerLanguage,
      [Fig_FIRST_DAY_OF_WEEK] = Cal_GetAndShowNumUsrsPerFirstDayOfWeek,
      [Fig_DATE_FORMAT      ] = Dat_GetAndShowNumUsrsPerDateFormat,
      [Fig_ICON_SETS        ] = Ico_GetAndShowNumUsrsPerIconSet,
      [Fig_MENUS            ] = Mnu_GetAndShowNumUsrsPerMenu,
      [Fig_THEMES           ] = The_GetAndShowNumUsrsPerTheme,
      [Fig_SIDE_COLUMNS     ] = Lay_GetAndShowNumUsrsPerSideColumns,
      [Fig_PHOTO_SHAPES     ] = PhoSha_GetAndShowNumUsrsPerPhotoShape,
      [Fig_PRIVACY          ] = Pri_GetAndShowNumUsrsPerPrivacy,
      [Fig_COOKIES          ] = Coo_GetAndShowNumUsrsPerCookies,
     };
   Fig_FigureType_t SelectedFigureType;

   /***** Get the type of figure ******/
   SelectedFigureType = (Fig_FigureType_t)
		        Par_GetParToUnsignedLong ("FigureType",
						  0,
						  Fig_NUM_FIGURES - 1,
						  (unsigned long) Fig_FIGURE_TYPE_DEF);

   /***** Show again the form to see use of the platform *****/
   Fig_ReqShowFigure (SelectedFigureType);

   /***** Show the stat of use selected by user *****/
   Fig_Function[SelectedFigureType] ();
  }
