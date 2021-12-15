// swad_cookies.c: user's preferences about cookies

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

#include "swad_box.h"
#include "swad_cookie.h"
#include "swad_cookie_database.h"
#include "swad_database.h"
#include "swad_figure.h"
#include "swad_form.h"
#include "swad_global.h"
#include "swad_HTML.h"
#include "swad_layout.h"
#include "swad_setting.h"

/*****************************************************************************/
/************** External global variables from others modules ****************/
/*****************************************************************************/

extern struct Globals Gbl;

/*****************************************************************************/
/***************************** Private prototypes ****************************/
/*****************************************************************************/

static void Coo_PutIconsCookies (__attribute__((unused)) void *Args);

/*****************************************************************************/
/********************* Edit my preferences on cookies ************************/
/*****************************************************************************/

void Coo_EditMyPrefsOnCookies (void)
  {
   extern const char *Hlp_PROFILE_Settings_cookies;
   extern const char *The_ClassBgHighlight[The_NUM_THEMES];
   extern const char *The_ClassDat[The_NUM_THEMES];
   extern const char *The_ClassDatStrong[The_NUM_THEMES];
   extern const char *Txt_Cookies;
   extern const char *Txt_Accept_third_party_cookies_to_view_multimedia_content_from_other_websites;

   /***** Begin section with preferences about cookies *****/
   HTM_SECTION_Begin (Coo_COOKIES_ID);

      /***** Begin box and table *****/
      Box_BoxTableBegin (NULL,Txt_Cookies,
			 Coo_PutIconsCookies,NULL,
			 Hlp_PROFILE_Settings_cookies,Box_NOT_CLOSABLE,2);

	 /***** Edit my preference about cookies *****/
	 /* Begin form */
	 Frm_BeginFormAnchor (ActChgCooPrf,Coo_COOKIES_ID);

	    /* Begin container */
	    if (Gbl.Usrs.Me.UsrDat.Prefs.AcceptCookies)
	       HTM_DIV_Begin ("class=\"%s %s\"",
	                      The_ClassDatStrong[Gbl.Prefs.Theme],
	                      The_ClassBgHighlight[Gbl.Prefs.Theme]);
	    else
	       HTM_DIV_Begin ("class=\"%s\"",The_ClassDat[Gbl.Prefs.Theme]);

	    /* Check box */
	    HTM_LABEL_Begin (NULL);
	       HTM_INPUT_CHECKBOX ("cookies",HTM_SUBMIT_ON_CHANGE,
				   "value=\"Y\"%s",
				   Gbl.Usrs.Me.UsrDat.Prefs.AcceptCookies ? " checked=\"checked\"" :
									    "");
	       HTM_Txt (Txt_Accept_third_party_cookies_to_view_multimedia_content_from_other_websites);
	    HTM_LABEL_End ();

	    /* End container */
	    HTM_DIV_End ();

	 /* End form */
         Frm_EndForm ();

      /***** End table and box *****/
      Box_BoxTableEnd ();

   /***** End section with preferences about cookies *****/
   HTM_SECTION_End ();
  }

/*****************************************************************************/
/***************** Put contextual icons in cookies preference ****************/
/*****************************************************************************/

static void Coo_PutIconsCookies (__attribute__((unused)) void *Args)
  {
   /***** Put icon to show a figure *****/
   Fig_PutIconToShowFigure (Fig_COOKIES);
  }

/*****************************************************************************/
/************** Change my preference about third party cookies ***************/
/*****************************************************************************/

void Coo_ChangeMyPrefsCookies (void)
  {
   /***** Get param with preference about third party cookies *****/
   Gbl.Usrs.Me.UsrDat.Prefs.AcceptCookies = Par_GetParToBool ("cookies");

   /***** Store preference in database *****/
   if (Gbl.Usrs.Me.Logged)
      Coo_DB_UpdateMyPrefsCookies ();

   /***** Show forms again *****/
   Set_EditSettings ();
  }
