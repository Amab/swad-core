// swad_hierarchy_config.c: hierarchy (country, institution, center, degree, course) configuration

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

#include "swad_figure_cache.h"
#include "swad_form.h"
#include "swad_global.h"
#include "swad_HTML.h"
#include "swad_logo.h"
#include "swad_QR.h"

/*****************************************************************************/
/************** External global variables from others modules ****************/
/*****************************************************************************/

extern struct Globals Gbl;

/*****************************************************************************/
/************************ Show title in configuration ************************/
/*****************************************************************************/

void HieCfg_Title (bool PutLink,
		   HieLvl_Level_t LogoScope,
		   long LogoCod,
                   char LogoShrtName[Cns_HIERARCHY_MAX_BYTES_SHRT_NAME + 1],
		   char LogoFullName[Cns_HIERARCHY_MAX_BYTES_FULL_NAME + 1],
		   char LogoWWW[Cns_MAX_BYTES_WWW + 1],
		   char TextFullName[Cns_HIERARCHY_MAX_BYTES_FULL_NAME + 1])
  {
   extern const char *The_ClassFrameTitleColor[The_NUM_THEMES];

   /***** Begin container *****/
   HTM_DIV_Begin ("class=\"FRAME_TITLE FRAME_TITLE_BIG %s\"",
                  The_ClassFrameTitleColor[Gbl.Prefs.Theme]);

      /* Begin link */
      if (PutLink)
	 HTM_A_Begin ("href=\"%s\" target=\"_blank\" title=\"%s\""
		      " class=\"FRAME_TITLE_BIG %s\"",
		      LogoWWW,
		      LogoFullName,
		      The_ClassFrameTitleColor[Gbl.Prefs.Theme]);

      /* Logo and name */
      Lgo_DrawLogo (LogoScope,LogoCod,LogoShrtName,64,NULL,true);
      HTM_BR ();
      HTM_Txt (TextFullName);

      /* End link */
      if (PutLink)
	 HTM_A_End ();

   /***** End container *****/
   HTM_DIV_End ();
  }

/*****************************************************************************/
/********************** Show full name in configuration **********************/
/*****************************************************************************/

void HieCfg_FullName (bool PutForm,const char *Label,Act_Action_t NextAction,
		      const char FullName[Cns_HIERARCHY_MAX_BYTES_FULL_NAME + 1])
  {
   extern const char *The_ClassDatStrong[The_NUM_THEMES];
   extern const char *The_ClassInput[The_NUM_THEMES];

   /***** Full name *****/
   HTM_TR_Begin (NULL);

      /* Label */
      Frm_LabelColumn ("RT",PutForm ? "FullName" :
				      NULL,
		       Label);

      /* Data */
      HTM_TD_Begin ("class=\"LB %s\"",The_ClassDatStrong[Gbl.Prefs.Theme]);
	 if (PutForm)
	   {
	    /* Form to change full name */
	    Frm_BeginForm (NextAction);
	       HTM_INPUT_TEXT ("FullName",Cns_HIERARCHY_MAX_CHARS_FULL_NAME,FullName,
			       HTM_SUBMIT_ON_CHANGE,
			       "id=\"FullName\" class=\"INPUT_FULL_NAME %s\""
			       " required=\"required\"",
			       The_ClassInput[Gbl.Prefs.Theme]);
	    Frm_EndForm ();
	   }
	 else	// I can not edit full name
	    HTM_Txt (FullName);
      HTM_TD_End ();

   HTM_TR_End ();
  }

/*****************************************************************************/
/********* Show institution short name in institution configuration **********/
/*****************************************************************************/

void HieCfg_ShrtName (bool PutForm,Act_Action_t NextAction,
		      const char ShrtName[Cns_HIERARCHY_MAX_BYTES_SHRT_NAME + 1])
  {
   extern const char *The_ClassDatStrong[The_NUM_THEMES];
   extern const char *The_ClassInput[The_NUM_THEMES];
   extern const char *Txt_Short_name;

   /***** Short name *****/
   HTM_TR_Begin (NULL);

      /* Label */
      Frm_LabelColumn ("RT",PutForm ? "ShortName" :
				      NULL,
		       Txt_Short_name);

      /* Data */
      HTM_TD_Begin ("class=\"LB %s\"",The_ClassDatStrong[Gbl.Prefs.Theme]);
	 if (PutForm)
	   {
	    /* Form to change short name */
	    Frm_BeginForm (NextAction);
	       HTM_INPUT_TEXT ("ShortName",Cns_HIERARCHY_MAX_CHARS_SHRT_NAME,ShrtName,
			       HTM_SUBMIT_ON_CHANGE,
			       "id=\"ShortName\" class=\"INPUT_SHORT_NAME %s\""
			       " required=\"required\"",
			       The_ClassInput[Gbl.Prefs.Theme]);
	    Frm_EndForm ();
	   }
	 else	// I can not edit short name
	    HTM_Txt (ShrtName);
      HTM_TD_End ();

   HTM_TR_End ();
  }

/*****************************************************************************/
/************************* Show web in configuration *************************/
/*****************************************************************************/

void HieCfg_WWW (bool PrintView,bool PutForm,Act_Action_t NextAction,
		 const char WWW[Cns_MAX_BYTES_WWW + 1])
  {
   extern const char *The_ClassDat[The_NUM_THEMES];
   extern const char *The_ClassInput[The_NUM_THEMES];
   extern const char *Txt_Web;

   /***** Web *****/
   HTM_TR_Begin (NULL);

      /* Label */
      Frm_LabelColumn ("RT",PutForm ? "WWW" :
				      NULL,
		       Txt_Web);

      /* Data */
      HTM_TD_Begin ("class=\"%s LB\"",The_ClassDat[Gbl.Prefs.Theme]);
	 if (PutForm)
	   {
	    /* Form to change web */
	    Frm_BeginForm (NextAction);
	       HTM_INPUT_URL ("WWW",WWW,HTM_SUBMIT_ON_CHANGE,
			      "id=\"WWW\" class=\"INPUT_WWW_WIDE %s\""
			      " required=\"required\"",
			      The_ClassInput[Gbl.Prefs.Theme]);
	    Frm_EndForm ();
	   }
	 else	// I can not change web
	   {
	    HTM_DIV_Begin ("class=\"EXTERNAL_WWW_LONG\"");
	       if (!PrintView)
		  HTM_A_Begin ("href=\"%s\" target=\"_blank\" class=\"%s\"",
		               WWW,The_ClassDat[Gbl.Prefs.Theme]);
	       HTM_Txt (WWW);
	       if (!PrintView)
		  HTM_A_End ();
	    HTM_DIV_End ();
	   }
      HTM_TD_End ();

   HTM_TR_End ();
  }

/*****************************************************************************/
/********************** Show shortcut in configuration ***********************/
/*****************************************************************************/

void HieCfg_Shortcut (bool PrintView,const char *ParamName,long HieCod)
  {
   extern const char *The_ClassDat[The_NUM_THEMES];
   extern const char *Txt_Shortcut;

   /***** Short cut *****/
   HTM_TR_Begin (NULL);

      /* Label */
      Frm_LabelColumn ("RT",NULL,Txt_Shortcut);

      /* Data */
      HTM_TD_Begin ("class=\"%s LB\"",The_ClassDat[Gbl.Prefs.Theme]);
	 if (!PrintView)
	   {
	    if (ParamName)
	       HTM_A_Begin ("href=\"%s/?%s=%ld\" class=\"%s\" target=\"_blank\"",
			    Cfg_URL_SWAD_CGI,
			    ParamName,HieCod,
			    The_ClassDat[Gbl.Prefs.Theme]);
	    else
	       HTM_A_Begin ("href=\"%s/\" class=\"%s\" target=\"_blank\"",
			    Cfg_URL_SWAD_CGI,
			    The_ClassDat[Gbl.Prefs.Theme]);
	   }
	 if (ParamName)
	    HTM_TxtF ("%s/?%s=%ld",
		      Cfg_URL_SWAD_CGI,
		      ParamName,HieCod);
	 else
	    HTM_TxtF ("%s/",
		      Cfg_URL_SWAD_CGI);
	 if (!PrintView)
	    HTM_A_End ();
      HTM_TD_End ();

   HTM_TR_End ();
  }

/*****************************************************************************/
/************************** Show number of centers ***************************/
/*****************************************************************************/

void HieCfg_NumCtrs (unsigned NumCtrs,bool PutForm)
  {
   extern const char *The_ClassDat[The_NUM_THEMES];
   extern const char *Txt_Centers;
   extern const char *Txt_Centers_of_INSTITUTION_X;

   /***** Number of centers *****/
   HTM_TR_Begin (NULL);

      /* Label */
      Frm_LabelColumn ("RT",NULL,Txt_Centers);

      /* Data */
      HTM_TD_Begin ("class=\"LB\"");
	 if (PutForm)
	   {
	    Frm_BeginFormGoTo (ActSeeCtr);
	       Ins_PutParamInsCod (Gbl.Hierarchy.Ins.InsCod);
	       HTM_BUTTON_SUBMIT_Begin (Str_BuildString (Txt_Centers_of_INSTITUTION_X,
							 Gbl.Hierarchy.Ins.ShrtName),
	                                Str_BuildString ("BT_LINK %s",
	                                                 The_ClassDat[Gbl.Prefs.Theme]),
					NULL);
	       Str_FreeStrings ();
	   }
	 HTM_Unsigned (NumCtrs);
	 if (PutForm)
	   {
	       HTM_BUTTON_End ();
	    Frm_EndForm ();
	   }
      HTM_TD_End ();

   HTM_TR_End ();
  }

/*****************************************************************************/
/********************* Show number of centers with map ***********************/
/*****************************************************************************/

void HieCfg_NumCtrsWithMap (unsigned NumCtrs,unsigned NumCtrsWithMap)
  {
   extern const char *The_ClassDat[The_NUM_THEMES];
   extern const char *Txt_Centers_with_map;

   /***** Number of centers with map *****/
   HTM_TR_Begin (NULL);

      /* Label */
      Frm_LabelColumn ("RT",NULL,Txt_Centers_with_map);

      /* Data */
      HTM_TD_Begin ("class=\"%s LB\"",The_ClassDat[Gbl.Prefs.Theme]);
	 HTM_TxtF ("%u (%.1lf%%)",
		   NumCtrsWithMap,
		   NumCtrs ? (double) NumCtrsWithMap * 100.0 /
			     (double) NumCtrs :
			     0.0);
      HTM_TD_End ();

   HTM_TR_End ();
  }

/*****************************************************************************/
/************************* Show QR in configuration **************************/
/*****************************************************************************/

void HieCfg_QR (const char *ParamName,long HieCod)
  {
   extern const char *The_ClassDat[The_NUM_THEMES];
   extern const char *Txt_QR_code;

   /***** QR *****/
   HTM_TR_Begin (NULL);

      /* Label */
      Frm_LabelColumn ("RT",NULL,Txt_QR_code);

      /* Data */
      HTM_TD_Begin ("class=\"%s LB\"",The_ClassDat[Gbl.Prefs.Theme]);
	 QR_LinkTo (250,ParamName,HieCod);
      HTM_TD_End ();

   HTM_TR_End ();
  }

/*****************************************************************************/
/************************ Number of users in courses *************************/
/*****************************************************************************/

void HieCfg_NumUsrsInCrss (HieLvl_Level_t Scope,long Cod,Rol_Role_t Role)
  {
   extern const char *The_ClassDat[The_NUM_THEMES];
   extern const char *Txt_Users_in_courses;
   extern const char *Txt_ROLES_PLURAL_Abc[Rol_NUM_ROLES][Usr_NUM_SEXS];

   /***** Number of users in courses *****/
   HTM_TR_Begin (NULL);

      /* Label */
      Frm_LabelColumn ("RT",NULL,
		       Role == Rol_UNK ? Txt_Users_in_courses :
					 Txt_ROLES_PLURAL_Abc[Role][Usr_SEX_UNKNOWN]);

      /* Data */
      HTM_TD_Begin ("class=\"%s LB\"",The_ClassDat[Gbl.Prefs.Theme]);
	 HTM_Unsigned (Enr_GetCachedNumUsrsInCrss (Scope,Cod,
						   Role == Rol_UNK ? (1 << Rol_STD) |
								     (1 << Rol_NET) |
								     (1 << Rol_TCH) :	// Any user
								     (1 << Role)));
      HTM_TD_End ();

   HTM_TR_End ();
  }
