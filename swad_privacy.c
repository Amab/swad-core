// swad_privacy.c: users' photo and public profile visibility

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
#include <string.h>

#include "swad_action.h"
#include "swad_box.h"
#include "swad_enrolment_database.h"
#include "swad_error.h"
#include "swad_figure.h"
#include "swad_form.h"
#include "swad_global.h"
#include "swad_HTML.h"
#include "swad_parameter.h"
#include "swad_privacy.h"
#include "swad_theme.h"
#include "swad_user_database.h"

/*****************************************************************************/
/************** External global variables from others modules ****************/
/*****************************************************************************/

extern struct Globals Gbl;

/*****************************************************************************/
/****************************** Public constants *****************************/
/*****************************************************************************/

/***** Visibility (who can see user's photo or public profile) *****/
const char *Pri_VisibilityDB[Pri_NUM_OPTIONS_PRIVACY] =
  {
   [Pri_VISIBILITY_UNKNOWN] = "unknown",
   [Pri_VISIBILITY_USER   ] = "user",
   [Pri_VISIBILITY_COURSE ] = "course",
   [Pri_VISIBILITY_SYSTEM ] = "system",
   [Pri_VISIBILITY_WORLD  ] = "world",
  };

/*****************************************************************************/
/***************************** Private constants *****************************/
/*****************************************************************************/

#define Pri_PRIVACY_ID	"privacy"

/*****************************************************************************/
/***************************** Private prototypes ****************************/
/*****************************************************************************/

static void Pri_PutIconsPrivacy (__attribute__((unused)) void *Args);

static void Pri_PutFormVisibility (const char *TxtLabel,
                                   Act_Action_t Action,const char *ParamName,
                                   Pri_Visibility_t CurrentVisibilityInDB,
                                   unsigned MaskAllowedVisibility);

static void Pri_GetAndShowNumUsrsPerPrivacyForAnObject (const char *TxtObject,
                                                        const char *FieldName,
                                                        unsigned MaskAllowedVisibility);

/*****************************************************************************/
/******************************* Edit my privacy *****************************/
/*****************************************************************************/

void Pri_EditMyPrivacy (void)
  {
   extern const char *Hlp_PROFILE_Settings_privacy;
   extern const char *Txt_Please_check_your_privacy_settings;
   extern const char *Txt_Privacy;
   extern const char *Txt_Photo;
   extern const char *Txt_Basic_public_profile;
   extern const char *Txt_Extended_public_profile;
   extern const char *Txt_Timeline;

   /***** Begin section with settings on privacy *****/
   HTM_SECTION_Begin (Pri_PRIVACY_ID);

      /***** If any of my settings about privacy is unknown *****/
      if (Gbl.Usrs.Me.UsrDat.PhotoVisibility == Pri_VISIBILITY_UNKNOWN ||
	  Gbl.Usrs.Me.UsrDat.BaPrfVisibility == Pri_VISIBILITY_UNKNOWN ||
	  Gbl.Usrs.Me.UsrDat.ExPrfVisibility == Pri_VISIBILITY_UNKNOWN)
	 Ale_ShowAlert (Ale_WARNING,Txt_Please_check_your_privacy_settings);

      /***** Begin box and table *****/
      Box_BoxTableBegin (NULL,Txt_Privacy,
			 Pri_PutIconsPrivacy,NULL,
			 Hlp_PROFILE_Settings_privacy,Box_NOT_CLOSABLE,2);

	 /***** Edit photo visibility *****/
	 Pri_PutFormVisibility (Txt_Photo,
				ActChgPriPho,"VisPho",
				Gbl.Usrs.Me.UsrDat.PhotoVisibility,
				Pri_PHOTO_ALLOWED_VIS);

	 /***** Edit basic public profile visibility *****/
	 Pri_PutFormVisibility (Txt_Basic_public_profile,
				ActChgBasPriPrf,"VisBasPrf",
				Gbl.Usrs.Me.UsrDat.BaPrfVisibility,
				Pri_BASIC_PROFILE_ALLOWED_VIS);

	 /***** Edit extended public profile visibility *****/
	 Pri_PutFormVisibility (Txt_Extended_public_profile,
				ActChgExtPriPrf,"VisExtPrf",
				Gbl.Usrs.Me.UsrDat.ExPrfVisibility,
				Pri_EXTENDED_PROFILE_ALLOWED_VIS);

	 /***** Edit public activity (timeline) visibility *****/
	 Pri_PutFormVisibility (Txt_Timeline,
				ActUnk,"VisTml",
				Pri_VISIBILITY_SYSTEM,
				Pri_TIMELINE_ALLOWED_VIS);

      /***** End table and box *****/
      Box_BoxTableEnd ();

   /***** End section with settings on privacy *****/
   HTM_SECTION_End ();
  }

/*****************************************************************************/
/****************** Put contextual icons in privacy setting ******************/
/*****************************************************************************/

static void Pri_PutIconsPrivacy (__attribute__((unused)) void *Args)
  {
   /***** Put icon to show a figure *****/
   Fig_PutIconToShowFigure (Fig_PRIVACY);
  }

/*****************************************************************************/
/************************** Select photo visibility **************************/
/*****************************************************************************/

static void Pri_PutFormVisibility (const char *TxtLabel,
                                   Act_Action_t Action,const char *ParamName,
                                   Pri_Visibility_t CurrentVisibilityInDB,
                                   unsigned MaskAllowedVisibility)
  {
   extern const char *The_ClassFormInBox[The_NUM_THEMES];
   extern const char *The_ClassDat[The_NUM_THEMES];
   extern const char *The_ClassDatStrong[The_NUM_THEMES];
   extern const char *Txt_PRIVACY_OPTIONS[Pri_NUM_OPTIONS_PRIVACY];
   Pri_Visibility_t Visibility;

   HTM_TR_Begin (NULL);

      /***** Select visibility *****/
      HTM_TD_Begin ("class=\"%s RT\"",The_ClassFormInBox[Gbl.Prefs.Theme]);
	 HTM_TxtColon (TxtLabel);
      HTM_TD_End ();

      /***** Form with list of options *****/
      HTM_TD_Begin ("class=\"LT\"");
	 if (Action != ActUnk)
	    Frm_BeginFormAnchor (Action,Pri_PRIVACY_ID);
	 HTM_UL_Begin ("class=\"PRI_LIST LIST_LEFT\"");

	    for (Visibility  = Pri_VISIBILITY_USER;
		 Visibility <= Pri_VISIBILITY_WORLD;
		 Visibility++)
	       if (MaskAllowedVisibility & (1 << Visibility))
		 {
		  if (Visibility == CurrentVisibilityInDB)
		     HTM_LI_Begin ("class=\"%s BG_HIGHLIGHT\"",
		                   The_ClassDatStrong[Gbl.Prefs.Theme]);
		  else
		     HTM_LI_Begin ("class=\"%s\"",
		                   The_ClassDat[Gbl.Prefs.Theme]);
		  HTM_LABEL_Begin (NULL);
		     HTM_INPUT_RADIO (ParamName,Action != ActUnk,
				      "value=\"%u\"%s%s",
				      (unsigned) Visibility,
				      Visibility == CurrentVisibilityInDB ? " checked=\"checked\"" :
									    "",
				      Action == ActUnk ? " disabled=\"disabled\"" :
							 "");
		     HTM_Txt (Txt_PRIVACY_OPTIONS[Visibility]);
		  HTM_LABEL_End ();
		  HTM_LI_End ();
		 }

	 /***** End list and form *****/
	 HTM_UL_End ();
	 if (Action != ActUnk)
	    Frm_EndForm ();
      HTM_TD_End ();

   HTM_TR_End ();
  }

/*****************************************************************************/
/************************ Get visibility from string *************************/
/*****************************************************************************/

Pri_Visibility_t Pri_GetVisibilityFromStr (const char *Str)
  {
   Pri_Visibility_t Visibility;

   for (Visibility  = (Pri_Visibility_t) 0;
	Visibility <= (Pri_Visibility_t) (Pri_NUM_OPTIONS_PRIVACY - 1);
	Visibility++)
      if (!strcasecmp (Str,Pri_VisibilityDB[Visibility]))
	 return Visibility;

   return Pri_VISIBILITY_UNKNOWN;
  }

/*****************************************************************************/
/**************** Get parameter with visibility from form ********************/
/*****************************************************************************/

Pri_Visibility_t Pri_GetParamVisibility (const char *ParamName,
                                         unsigned MaskAllowedVisibility)
  {
   Pri_Visibility_t Visibility;

   Visibility = (Pri_Visibility_t) Par_GetParToUnsignedLong (ParamName,
							     0,
							     Pri_NUM_OPTIONS_PRIVACY - 1,
							     (unsigned long) Pri_VISIBILITY_UNKNOWN);
   return (MaskAllowedVisibility & (1 << Visibility)) ? Visibility :
	                                                Pri_VISIBILITY_UNKNOWN;
  }

/*****************************************************************************/
/*********** Check if user's photo of public profile can be shown ************/
/*****************************************************************************/
// Returns true if it can be shown and false if not.

bool Pri_ShowingIsAllowed (Pri_Visibility_t Visibility,struct UsrData *UsrDat)
  {
   /***** I always can see my things *****/
   if (Usr_ItsMe (UsrDat->UsrCod))
      return true;

   /***** System admins always can see others' profiles *****/
   if (Gbl.Usrs.Me.Role.Logged == Rol_SYS_ADM)
      return true;

   /***** Check if I can see the other's photo *****/
   switch (Visibility)
     {
      case Pri_VISIBILITY_UNKNOWN:
	 return false;			// It's not me
      case Pri_VISIBILITY_USER:		// Only visible
					// by me and my teachers if I am a student
					// or me and my students if I am a teacher
         // Do both users share the same course but whit different role?
	 return Enr_DB_CheckIfUsrSharesAnyOfMyCrsWithDifferentRole (UsrDat->UsrCod);
      case Pri_VISIBILITY_COURSE:	// Visible by users sharing courses with me
	 // Do both users share the same course?
         return Enr_CheckIfUsrSharesAnyOfMyCrs (UsrDat);
      case Pri_VISIBILITY_SYSTEM:	// Visible by any user logged in platform
         return Gbl.Usrs.Me.Logged;
      case Pri_VISIBILITY_WORLD:	// Public, visible by everyone, even unlogged visitors
         return true;
     }

   return false;	// Never reached. To avoid warning
  }

/*****************************************************************************/
/********** Get and show number of users who have chosen a privacy ***********/
/*****************************************************************************/

void Pri_GetAndShowNumUsrsPerPrivacy (void)
  {
   extern const char *Hlp_ANALYTICS_Figures_privacy;
   extern const char *Txt_Photo;
   extern const char *Txt_Basic_public_profile;
   extern const char *Txt_Extended_public_profile;
   extern const char *Txt_FIGURE_TYPES[Fig_NUM_FIGURES];

   /***** Begin box and table *****/
   Box_BoxTableBegin (NULL,Txt_FIGURE_TYPES[Fig_PRIVACY],
                      NULL,NULL,
                      Hlp_ANALYTICS_Figures_privacy,Box_NOT_CLOSABLE,2);

      /***** Privacy for photo *****/
      Pri_GetAndShowNumUsrsPerPrivacyForAnObject (Txt_Photo,
						  "PhotoVisibility",
						  Pri_PHOTO_ALLOWED_VIS);

      /***** Privacy for public profile *****/
      Pri_GetAndShowNumUsrsPerPrivacyForAnObject (Txt_Basic_public_profile,
						  "BaPrfVisibility",
						  Pri_BASIC_PROFILE_ALLOWED_VIS);
      Pri_GetAndShowNumUsrsPerPrivacyForAnObject (Txt_Extended_public_profile,
						  "ExPrfVisibility",
						  Pri_EXTENDED_PROFILE_ALLOWED_VIS);

   /***** End table and box *****/
   Box_BoxTableEnd ();
  }

/*****************************************************************************/
/********** Get and show number of users who have chosen a privacy ***********/
/*****************************************************************************/

static void Pri_GetAndShowNumUsrsPerPrivacyForAnObject (const char *TxtObject,
                                                        const char *FieldName,
                                                        unsigned MaskAllowedVisibility)
  {
   extern const char *Pri_VisibilityDB[Pri_NUM_OPTIONS_PRIVACY];
   extern const char *The_ClassDat[The_NUM_THEMES];
   extern const char *Txt_Number_of_users;
   extern const char *Txt_PERCENT_of_users;
   extern const char *Txt_PRIVACY_OPTIONS[Pri_NUM_OPTIONS_PRIVACY];
   Pri_Visibility_t Visibility;
   char *SubQuery;
   unsigned NumUsrs[Pri_NUM_OPTIONS_PRIVACY];
   unsigned NumUsrsTotal = 0;

   /***** Heading row *****/
   HTM_TR_Begin (NULL);
      HTM_TH (TxtObject           ,HTM_HEAD_LEFT);
      HTM_TH (Txt_Number_of_users ,HTM_HEAD_RIGHT);
      HTM_TH (Txt_PERCENT_of_users,HTM_HEAD_RIGHT);
   HTM_TR_End ();

   /***** For each privacy option... *****/
   for (Visibility  = (Pri_Visibility_t) 0;
	Visibility <= (Pri_Visibility_t) (Pri_NUM_OPTIONS_PRIVACY - 1);
	Visibility++)
      if (MaskAllowedVisibility & (1 << Visibility))
	{
	 /* Get the number of users who have chosen this privacy option from database */
	 if (asprintf (&SubQuery,"usr_data.%s='%s'",
		       FieldName,Pri_VisibilityDB[Visibility]) < 0)
	    Err_NotEnoughMemoryExit ();
	 NumUsrs[Visibility] = Usr_DB_GetNumUsrsWhoChoseAnOption (SubQuery);
	 free (SubQuery);

	 /* Update total number of users */
	 NumUsrsTotal += NumUsrs[Visibility];
	}

   /***** Write number of users who have chosen each privacy option *****/
   for (Visibility  = (Pri_Visibility_t) 0;
	Visibility <= (Pri_Visibility_t) (Pri_NUM_OPTIONS_PRIVACY - 1);
	Visibility++)
      if (MaskAllowedVisibility & (1 << Visibility))
	{
	 HTM_TR_Begin (NULL);

	    HTM_TD_Begin ("class=\"%s LM\"",The_ClassDat[Gbl.Prefs.Theme]);
	       HTM_Txt (Txt_PRIVACY_OPTIONS[Visibility]);
	    HTM_TD_End ();

	    HTM_TD_Begin ("class=\"%s RM\"",The_ClassDat[Gbl.Prefs.Theme]);
	       HTM_Unsigned (NumUsrs[Visibility]);
	    HTM_TD_End ();

	    HTM_TD_Begin ("class=\"%s RM\"",The_ClassDat[Gbl.Prefs.Theme]);
	       HTM_Percentage (NumUsrsTotal ? (double) NumUsrs[Visibility] * 100.0 /
					      (double) NumUsrsTotal :
					      0.0);
	    HTM_TD_End ();

         HTM_TR_End ();
	}
   }
