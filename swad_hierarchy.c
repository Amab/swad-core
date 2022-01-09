// swad_hierarchy.c: hierarchy (system, institution, center, degree, course)

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
#include <stdlib.h>		// For free

#include "swad_database.h"
#include "swad_degree_database.h"
#include "swad_error.h"
#include "swad_figure.h"
#include "swad_form.h"
#include "swad_global.h"
#include "swad_group_database.h"
#include "swad_hierarchy.h"
#include "swad_hierarchy_database.h"
#include "swad_hierarchy_level.h"
#include "swad_HTML.h"
#include "swad_logo.h"

/*****************************************************************************/
/************** External global variables from others modules ****************/
/*****************************************************************************/

extern struct Globals Gbl;

/*****************************************************************************/
/***************************** Private prototypes ****************************/
/*****************************************************************************/

static Hie_StatusTxt_t Hie_GetStatusTxtFromStatusBits (Hie_Status_t Status);
static Hie_Status_t Hie_GetStatusBitsFromStatusTxt (Hie_StatusTxt_t StatusTxt);

static void Hie_WriteHeadHierarchy (void);
static void Hie_GetAndShowHierarchyWithInss (void);
static void Hie_GetAndShowHierarchyWithCtrs (void);
static void Hie_GetAndShowHierarchyWithDegs (void);
static void Hie_GetAndShowHierarchyWithCrss (void);
static void Hie_GetAndShowHierarchyWithUsrs (Rol_Role_t Role);
static void Hie_GetAndShowHierarchyTotal (void);
static void Hie_ShowHierarchyRow (const char *Text1,const char *Text2,
				  const char *ClassTxt,
				  int NumCtys,	// < 0 ==> do not show number
				  int NumInss,	// < 0 ==> do not show number
				  int NumCtrs,	// < 0 ==> do not show number
				  int NumDegs,	// < 0 ==> do not show number
				  int NumCrss);	// < 0 ==> do not show number
static void Hie_ShowHierarchyCell (const char *ClassTxt,int Num);

/*****************************************************************************/
/********** List pending institutions, centers, degrees and courses **********/
/*****************************************************************************/

void Hie_SeePending (void)
  {
   /***** List countries with pending institutions *****/
   Cty_SeeCtyWithPendingInss ();

   /***** List institutions with pending centers *****/
   Ins_SeeInsWithPendingCtrs ();

   /***** List centers with pending degrees *****/
   Ctr_SeeCtrWithPendingDegs ();

   /***** List degrees with pending courses *****/
   Deg_SeeDegWithPendingCrss ();
  }

/*****************************************************************************/
/*** Write menu to select country, institution, center, degree and course ****/
/*****************************************************************************/

void Hie_WriteMenuHierarchy (void)
  {
   extern const char *Txt_Country;
   extern const char *Txt_Institution;
   extern const char *Txt_Center;
   extern const char *Txt_Degree;
   extern const char *Txt_Course;

   /***** Begin table *****/
   HTM_TABLE_BeginCenterPadding (2);

      /***** Write a 1st selector with all countries *****/
      HTM_TR_Begin (NULL);

	 /* Label */
	 Frm_LabelColumn ("RT","cty",Txt_Country);

	 /* Data */
	 HTM_TD_Begin ("class=\"LT\"");
	    Cty_WriteSelectorOfCountry ();
	 HTM_TD_End ();

      HTM_TR_End ();

      if (Gbl.Hierarchy.Cty.CtyCod > 0)
	{
	 /***** Write a 2nd selector
		with the institutions of selected country *****/
	 HTM_TR_Begin (NULL);

	    /* Label */
	    Frm_LabelColumn ("RT","ins",Txt_Institution);

	    /* Data */
	    HTM_TD_Begin ("class=\"LT\"");
	       Ins_WriteSelectorOfInstitution ();
	    HTM_TD_End ();

	 HTM_TR_End ();

	 if (Gbl.Hierarchy.Ins.InsCod > 0)
	   {
	    /***** Write a 3rd selector
		   with all the centers of selected institution *****/
	    HTM_TR_Begin (NULL);

	       /* Label */
	       Frm_LabelColumn ("RT","ctr",Txt_Center);

	       /* Data */
	       HTM_TD_Begin ("class=\"LT\"");
		  Ctr_WriteSelectorOfCenter ();
	       HTM_TD_End ();

	    HTM_TR_End ();

	    if (Gbl.Hierarchy.Ctr.CtrCod > 0)
	      {
	       /***** Write a 4th selector
		      with all degrees of selected center *****/
	       HTM_TR_Begin (NULL);

		  /* Label */
		  Frm_LabelColumn ("RT","deg",Txt_Degree);

		  /* Data */
		  HTM_TD_Begin ("class=\"LT\"");
		     Deg_WriteSelectorOfDegree ();
		  HTM_TD_End ();

	       HTM_TR_End ();

	       if (Gbl.Hierarchy.Deg.DegCod > 0)
		 {
		  /***** Write a 5th selector
			 with all courses of selected degree *****/
		  HTM_TR_Begin (NULL);

		     /* Label */
		     Frm_LabelColumn ("RT","crs",Txt_Course);

		     /* Data */
		     HTM_TD_Begin ("class=\"LT\"");
			Crs_WriteSelectorOfCourse ();
		     HTM_TD_End ();

		  HTM_TR_End ();
		 }
	      }
	   }
	}

   /***** End table *****/
   HTM_TABLE_End ();
  }

/*****************************************************************************/
/************* Write hierarchy breadcrumb in the top of the page *************/
/*****************************************************************************/

void Hie_WriteHierarchyInBreadcrumb (void)
  {
   extern const char *The_Colors[The_NUM_THEMES];
   extern const char *Txt_System;
   extern const char *Txt_Country;
   extern const char *Txt_Institution;
   extern const char *Txt_Center;
   extern const char *Txt_Degree;

   /***** Form to go to the system *****/
   HTM_DIV_Begin ("class=\"BC BC_%s\"",The_Colors[Gbl.Prefs.Theme]);

      HTM_NBSP ();

      Frm_BeginFormGoTo (ActMnu);
	 Par_PutHiddenParamUnsigned (NULL,"NxtTab",(unsigned) TabSys);
	 HTM_BUTTON_OnSubmit_Begin (Txt_System,"BT_LINK",NULL);
	    HTM_Txt (Txt_System);
	 HTM_BUTTON_End ();
      Frm_EndForm ();

   HTM_DIV_End ();

   if (Gbl.Hierarchy.Cty.CtyCod > 0)		// Country selected...
     {
      HTM_DIV_Begin ("class=\"BC BC_%s\"",The_Colors[Gbl.Prefs.Theme]);

	 /***** Separator *****/
	 HTM_Txt ("&nbsp;&gt;&nbsp;");

	 /***** Form to go to see institutions of this country *****/
	 Frm_BeginFormGoTo (ActSeeIns);
	    Cty_PutParamCtyCod (Gbl.Hierarchy.Cty.CtyCod);
	    HTM_BUTTON_OnSubmit_Begin (Gbl.Hierarchy.Cty.Name[Gbl.Prefs.Language],
	                               "BT_LINK",NULL);
	       HTM_Txt (Gbl.Hierarchy.Cty.Name[Gbl.Prefs.Language]);
	    HTM_BUTTON_End ();
	 Frm_EndForm ();

      HTM_DIV_End ();
     }
   else
     {
      HTM_DIV_Begin ("class=\"BC BC_SEMIOFF BC_%s\"",The_Colors[Gbl.Prefs.Theme]);

	 /***** Separator *****/
	 HTM_Txt ("&nbsp;&gt;&nbsp;");

	 /***** Form to go to select countries *****/
	 Frm_BeginFormGoTo (ActSeeCty);
	    HTM_BUTTON_OnSubmit_Begin (Txt_Country,"BT_LINK",NULL);
	       HTM_Txt (Txt_Country);
	    HTM_BUTTON_End ();
	 Frm_EndForm ();

      HTM_DIV_End ();
     }

   if (Gbl.Hierarchy.Ins.InsCod > 0)		// Institution selected...
     {
      HTM_DIV_Begin ("class=\"BC BC_%s\"",The_Colors[Gbl.Prefs.Theme]);

	 /***** Separator *****/
	 HTM_Txt ("&nbsp;&gt;&nbsp;");

	 /***** Form to see centers of this institution *****/
	 Frm_BeginFormGoTo (ActSeeCtr);
	    Ins_PutParamInsCod (Gbl.Hierarchy.Ins.InsCod);
	    HTM_BUTTON_OnSubmit_Begin (Gbl.Hierarchy.Ins.FullName,
	                               "BT_LINK",NULL);
	       HTM_Txt (Gbl.Hierarchy.Ins.ShrtName);
	    HTM_BUTTON_End ();
	 Frm_EndForm ();

      HTM_DIV_End ();
     }
   else if (Gbl.Hierarchy.Cty.CtyCod > 0)
     {
      HTM_DIV_Begin ("class=\"BC BC_SEMIOFF BC_%s\"",The_Colors[Gbl.Prefs.Theme]);

	 /***** Separator *****/
	 HTM_Txt ("&nbsp;&gt;&nbsp;");

	 /***** Form to go to select institutions *****/
	 Frm_BeginFormGoTo (ActSeeIns);
	    HTM_BUTTON_OnSubmit_Begin (Txt_Institution,"BT_LINK",NULL);
	       HTM_Txt (Txt_Institution);
	    HTM_BUTTON_End ();
	 Frm_EndForm ();

      HTM_DIV_End ();
     }
   else
     {
      HTM_DIV_Begin ("class=\"BC BC_OFF BC_%s\"",The_Colors[Gbl.Prefs.Theme]);

	 /***** Separator *****/
	 HTM_Txt ("&nbsp;&gt;&nbsp;");

	 /***** Hidden institution *****/
	 HTM_Txt (Txt_Institution);

      HTM_DIV_End ();
     }

   if (Gbl.Hierarchy.Ctr.CtrCod > 0)	// Center selected...
     {
      HTM_DIV_Begin ("class=\"BC BC_%s\"",The_Colors[Gbl.Prefs.Theme]);

	 /***** Separator *****/
	 HTM_Txt ("&nbsp;&gt;&nbsp;");

	 /***** Form to see degrees of this center *****/
	 Frm_BeginFormGoTo (ActSeeDeg);
	    Ctr_PutParamCtrCod (Gbl.Hierarchy.Ctr.CtrCod);
	    HTM_BUTTON_OnSubmit_Begin (Gbl.Hierarchy.Ctr.FullName,
	                               "BT_LINK",NULL);
	       HTM_Txt (Gbl.Hierarchy.Ctr.ShrtName);
	    HTM_BUTTON_End ();
	 Frm_EndForm ();

      HTM_DIV_End ();
     }
   else if (Gbl.Hierarchy.Ins.InsCod > 0)
     {
      HTM_DIV_Begin ("class=\"BC BC_SEMIOFF BC_%s\"",The_Colors[Gbl.Prefs.Theme]);

	 /***** Separator *****/
	 HTM_Txt ("&nbsp;&gt;&nbsp;");

	 /***** Form to go to select centers *****/
	 Frm_BeginFormGoTo (ActSeeCtr);
	    HTM_BUTTON_OnSubmit_Begin (Txt_Center,"BT_LINK",NULL);
	       HTM_Txt (Txt_Center);
	    HTM_BUTTON_End ();
	 Frm_EndForm ();

      HTM_DIV_End ();
     }
   else
     {
      HTM_DIV_Begin ("class=\"BC BC_OFF BC_%s\"",The_Colors[Gbl.Prefs.Theme]);

	 /***** Separator *****/
	 HTM_Txt ("&nbsp;&gt;&nbsp;");

	 /***** Hidden center *****/
	 HTM_Txt (Txt_Center);

      HTM_DIV_End ();
     }

   if (Gbl.Hierarchy.Deg.DegCod > 0)	// Degree selected...
     {
      HTM_DIV_Begin ("class=\"BC BC_%s\"",The_Colors[Gbl.Prefs.Theme]);

	 /***** Separator *****/
	 HTM_Txt ("&nbsp;&gt;&nbsp;");

	 /***** Form to go to see courses of this degree *****/
	 Frm_BeginFormGoTo (ActSeeCrs);
	    Deg_PutParamDegCod (Gbl.Hierarchy.Deg.DegCod);
	    HTM_BUTTON_OnSubmit_Begin (Gbl.Hierarchy.Deg.FullName,
	                               "BT_LINK",NULL);
	       HTM_Txt (Gbl.Hierarchy.Deg.ShrtName);
	    HTM_BUTTON_End ();
	 Frm_EndForm ();

      HTM_DIV_End ();
     }
   else if (Gbl.Hierarchy.Ctr.CtrCod > 0)
     {
      HTM_DIV_Begin ("class=\"BC BC_SEMIOFF BC_%s\"",The_Colors[Gbl.Prefs.Theme]);

	 /***** Separator *****/
	 HTM_Txt ("&nbsp;&gt;&nbsp;");

	 /***** Form to go to select degrees *****/
	 Frm_BeginFormGoTo (ActSeeDeg);
	    HTM_BUTTON_OnSubmit_Begin (Txt_Degree,"BT_LINK",NULL);
	       HTM_Txt (Txt_Degree);
	    HTM_BUTTON_End ();
	 Frm_EndForm ();

      HTM_DIV_End ();
     }
   else
     {
      HTM_DIV_Begin ("class=\"BC BC_OFF BC_%s\"",The_Colors[Gbl.Prefs.Theme]);

	 /***** Separator *****/
	 HTM_Txt ("&nbsp;&gt;&nbsp;");

	 /***** Hidden degree *****/
	 HTM_Txt (Txt_Degree);

      HTM_DIV_End ();
     }

   HTM_DIV_Begin ("class=\"BC%s BC_%s\"",
		   (Gbl.Hierarchy.Level == HieLvl_CRS) ? "" :
		  ((Gbl.Hierarchy.Deg.DegCod > 0) ? " BC_SEMIOFF" :
						    " BC_OFF"),
		  The_Colors[Gbl.Prefs.Theme]);

      /***** Separator *****/
      HTM_Txt ("&nbsp;&gt;&nbsp;");

   HTM_DIV_End ();
  }

/*****************************************************************************/
/*************** Write course full name in the top of the page ***************/
/*****************************************************************************/

void Hie_WriteBigNameCtyInsCtrDegCrs (void)
  {
   extern const char *The_Colors[The_NUM_THEMES];
   extern const char *Txt_TAGLINE;

   HTM_TxtF ("<h1 id=\"main_title\" class=\"MAIN_TITLE_%s\">",
	     The_Colors[Gbl.Prefs.Theme]);

   /***** Logo *****/
   switch (Gbl.Hierarchy.Level)
     {
      case HieLvl_SYS:	// System
	 Ico_PutIcon ("swad64x64.png",Ico_UNCHANGED,
	              Cfg_PLATFORM_FULL_NAME,"ICO40x40 TOP_LOGO");
         break;
      case HieLvl_CTY:	// Country
         Cty_DrawCountryMap (&Gbl.Hierarchy.Cty,"COUNTRY_MAP_TITLE");
         break;
      case HieLvl_INS:	// Institution
	 Lgo_DrawLogo (HieLvl_INS,Gbl.Hierarchy.Ins.InsCod,
		       Gbl.Hierarchy.Ins.ShrtName,40,"TOP_LOGO",false);
         break;
      case HieLvl_CTR:	// Center
	 Lgo_DrawLogo (HieLvl_CTR,Gbl.Hierarchy.Ctr.CtrCod,
		       Gbl.Hierarchy.Ctr.ShrtName,40,"TOP_LOGO",false);
         break;
      case HieLvl_DEG:	// Degree
      case HieLvl_CRS:	// Course
	 Lgo_DrawLogo (HieLvl_DEG,Gbl.Hierarchy.Deg.DegCod,
		       Gbl.Hierarchy.Deg.ShrtName,40,"TOP_LOGO",false);
         break;
      default:
	 break;
     }

   /***** Text *****/
   HTM_DIV_Begin ("id=\"big_name_container\"");
      if (Gbl.Hierarchy.Cty.CtyCod > 0)
	{
	 HTM_DIV_Begin ("id=\"big_full_name\"");
	    HTM_Txt (	(Gbl.Hierarchy.Level == HieLvl_CRS) ? Gbl.Hierarchy.Crs.FullName :// Full name
		     ((Gbl.Hierarchy.Level == HieLvl_DEG) ? Gbl.Hierarchy.Deg.FullName :
		     ((Gbl.Hierarchy.Level == HieLvl_CTR) ? Gbl.Hierarchy.Ctr.FullName :
		     ((Gbl.Hierarchy.Level == HieLvl_INS) ? Gbl.Hierarchy.Ins.FullName :
							 Gbl.Hierarchy.Cty.Name[Gbl.Prefs.Language]))));
	 HTM_DIV_End ();

	 HTM_DIV_Begin ("class=\"NOT_SHOWN\"");
	    HTM_Txt (" / ");	// To separate
	 HTM_DIV_End ();

	 HTM_DIV_Begin ("id=\"big_short_name\"");
	    HTM_Txt ( (Gbl.Hierarchy.Level == HieLvl_CRS) ? Gbl.Hierarchy.Crs.ShrtName :// Short name
		     ((Gbl.Hierarchy.Level == HieLvl_DEG) ? Gbl.Hierarchy.Deg.ShrtName :
		     ((Gbl.Hierarchy.Level == HieLvl_CTR) ? Gbl.Hierarchy.Ctr.ShrtName :
		     ((Gbl.Hierarchy.Level == HieLvl_INS) ? Gbl.Hierarchy.Ins.ShrtName :
							    Gbl.Hierarchy.Cty.Name[Gbl.Prefs.Language]))));
	 HTM_DIV_End ();
	}
      else	// No country specified ==> home page
	{
	 HTM_DIV_Begin ("id=\"big_full_name\"");	// Full name
	    HTM_TxtF ("%s:&nbsp;%s",Cfg_PLATFORM_SHORT_NAME,Txt_TAGLINE);
	 HTM_DIV_End ();

	 HTM_DIV_Begin ("class=\"NOT_SHOWN\"");
	    HTM_Txt (" / ");	// To separate
	 HTM_DIV_End ();

	 HTM_DIV_Begin ("id=\"big_short_name\"");	// Short name
	    HTM_Txt (Cfg_PLATFORM_SHORT_NAME);
	 HTM_DIV_End ();
	}
   HTM_DIV_End ();
   HTM_TxtF ("</h1>");
  }

/*****************************************************************************/
/**************** Copy last hierarchy to current hierarchy *******************/
/*****************************************************************************/

void Hie_SetHierarchyFromUsrLastHierarchy (void)
  {
   /***** Initialize all codes to -1 *****/
   Hie_ResetHierarchy ();

   /***** Copy last hierarchy scope and code to current hierarchy *****/
   switch (Gbl.Usrs.Me.UsrLast.LastHie.Scope)
     {
      case HieLvl_CTY:	// Country
         Gbl.Hierarchy.Cty.CtyCod = Gbl.Usrs.Me.UsrLast.LastHie.Cod;
	 break;
      case HieLvl_INS:	// Institution
         Gbl.Hierarchy.Ins.InsCod = Gbl.Usrs.Me.UsrLast.LastHie.Cod;
	 break;
      case HieLvl_CTR:	// Center
         Gbl.Hierarchy.Ctr.CtrCod = Gbl.Usrs.Me.UsrLast.LastHie.Cod;
	 break;
      case HieLvl_DEG:	// Degree
         Gbl.Hierarchy.Deg.DegCod = Gbl.Usrs.Me.UsrLast.LastHie.Cod;
	 break;
      case HieLvl_CRS:	// Course
         Gbl.Hierarchy.Crs.CrsCod = Gbl.Usrs.Me.UsrLast.LastHie.Cod;
	 break;
      default:
	 break;
     }

   /****** Initialize again current course, degree, center... ******/
   Hie_InitHierarchy ();
  }

/*****************************************************************************/
/**** Initialize current country, institution, center, degree and course *****/
/*****************************************************************************/

void Hie_InitHierarchy (void)
  {
   /***** If course code is available, get course data *****/
   if (Gbl.Hierarchy.Crs.CrsCod > 0)
     {
      if (Crs_GetDataOfCourseByCod (&Gbl.Hierarchy.Crs))	// Course found
         Gbl.Hierarchy.Deg.DegCod = Gbl.Hierarchy.Crs.DegCod;
      else
         Hie_ResetHierarchy ();
     }

   /***** If degree code is available, get degree data *****/
   if (Gbl.Hierarchy.Deg.DegCod > 0)
     {
      if (Deg_GetDataOfDegreeByCod (&Gbl.Hierarchy.Deg))	// Degree found
	{
	 Gbl.Hierarchy.Ctr.CtrCod = Gbl.Hierarchy.Deg.CtrCod;
         Gbl.Hierarchy.Ins.InsCod = Deg_DB_GetInsCodOfDegreeByCod (Gbl.Hierarchy.Deg.DegCod);
	}
      else
         Hie_ResetHierarchy ();
     }

   /***** If center code is available, get center data *****/
   if (Gbl.Hierarchy.Ctr.CtrCod > 0)
     {
      if (Ctr_GetDataOfCenterByCod (&Gbl.Hierarchy.Ctr))	// Center found
         Gbl.Hierarchy.Ins.InsCod = Gbl.Hierarchy.Ctr.InsCod;
      else
         Hie_ResetHierarchy ();
     }

   /***** If institution code is available, get institution data *****/
   if (Gbl.Hierarchy.Ins.InsCod > 0)
     {
      if (Ins_GetDataOfInstitByCod (&Gbl.Hierarchy.Ins))	// Institution found
	 Gbl.Hierarchy.Cty.CtyCod = Gbl.Hierarchy.Ins.CtyCod;
      else
         Hie_ResetHierarchy ();
     }

   /***** If country code is available, get country data *****/
   if (Gbl.Hierarchy.Cty.CtyCod > 0)
      if (!Cty_GetDataOfCountryByCod (&Gbl.Hierarchy.Cty))		// Country not found
         Hie_ResetHierarchy ();

   /***** Set current hierarchy level and code
          depending on course code, degree code, etc. *****/
   if      (Gbl.Hierarchy.Crs.CrsCod > 0)	// Course selected
     {
      Gbl.Hierarchy.Level = HieLvl_CRS;
      Gbl.Hierarchy.Cod = Gbl.Hierarchy.Crs.CrsCod;
     }
   else if (Gbl.Hierarchy.Deg.DegCod > 0)	// Degree selected
     {
      Gbl.Hierarchy.Level = HieLvl_DEG;
      Gbl.Hierarchy.Cod = Gbl.Hierarchy.Deg.DegCod;
     }
   else if (Gbl.Hierarchy.Ctr.CtrCod > 0)	// Center selected
     {
      Gbl.Hierarchy.Level = HieLvl_CTR;
      Gbl.Hierarchy.Cod = Gbl.Hierarchy.Ctr.CtrCod;
     }
   else if (Gbl.Hierarchy.Ins.InsCod > 0)	// Institution selected
     {
      Gbl.Hierarchy.Level = HieLvl_INS;
      Gbl.Hierarchy.Cod = Gbl.Hierarchy.Ins.InsCod;
     }
   else if (Gbl.Hierarchy.Cty.CtyCod > 0)	// Country selected
     {
      Gbl.Hierarchy.Level = HieLvl_CTY;
      Gbl.Hierarchy.Cod = Gbl.Hierarchy.Cty.CtyCod;
     }
   else
     {
      Gbl.Hierarchy.Level = HieLvl_SYS;
      Gbl.Hierarchy.Cod = -1L;
     }

   /***** Initialize paths *****/
   if (Gbl.Hierarchy.Level == HieLvl_CRS)	// Course selected
     {
      /***** Paths of course directories *****/
      snprintf (Gbl.Crs.PathPriv,sizeof (Gbl.Crs.PathPriv),"%s/%ld",
	        Cfg_PATH_CRS_PRIVATE,Gbl.Hierarchy.Crs.CrsCod);
      snprintf (Gbl.Crs.PathRelPubl,sizeof (Gbl.Crs.PathRelPubl),"%s/%ld",
	        Cfg_PATH_CRS_PUBLIC,Gbl.Hierarchy.Crs.CrsCod);
      snprintf (Gbl.Crs.PathURLPubl,sizeof (Gbl.Crs.PathURLPubl),"%s/%ld",
	        Cfg_URL_CRS_PUBLIC,Gbl.Hierarchy.Crs.CrsCod);

      /***** If any of the course directories does not exist, create it *****/
      if (!Fil_CheckIfPathExists (Gbl.Crs.PathPriv))
	 Fil_CreateDirIfNotExists (Gbl.Crs.PathPriv);
      if (!Fil_CheckIfPathExists (Gbl.Crs.PathRelPubl))
	 Fil_CreateDirIfNotExists (Gbl.Crs.PathRelPubl);

      /***** Count number of groups in current course
             (used in some actions) *****/
      Gbl.Crs.Grps.NumGrps = Grp_DB_CountNumGrpsInCurrentCrs ();
     }
  }

/*****************************************************************************/
/******* Reset current country, institution, center, degree and course *******/
/*****************************************************************************/

void Hie_ResetHierarchy (void)
  {
   /***** Country *****/
   Gbl.Hierarchy.Cty.CtyCod = -1L;

   /***** Institution *****/
   Gbl.Hierarchy.Ins.InsCod = -1L;

   /***** Center *****/
   Gbl.Hierarchy.Ctr.CtrCod = -1L;
   Gbl.Hierarchy.Ctr.InsCod = -1L;
   Gbl.Hierarchy.Ctr.PlcCod = -1L;

   /***** Degree *****/
   Gbl.Hierarchy.Deg.DegCod = -1L;

   /***** Course *****/
   Gbl.Hierarchy.Crs.CrsCod = -1L;

   /***** Hierarchy level and code *****/
   Gbl.Hierarchy.Level = HieLvl_UNK;
   Gbl.Hierarchy.Cod   = -1L;
  }

/*****************************************************************************/
/***** Write institutions, centers and degrees administrated by an admin *****/
/*****************************************************************************/

void Hie_GetAndWriteInsCtrDegAdminBy (long UsrCod,unsigned ColSpan)
  {
   extern const char *The_ClassDatSmallNoBR[The_NUM_THEMES];
   extern const char *Txt_all_degrees;
   MYSQL_RES *mysql_res;
   MYSQL_ROW row;
   unsigned NumRow;
   unsigned NumRows;
   struct Hie_Hierarchy Hie;

   /***** Get institutions, centers, degrees admin by user from database *****/
   NumRows = Hie_DB_GetInsCtrDegAdminBy (&mysql_res,UsrCod);

   /***** Get the list of degrees *****/
   for (NumRow  = 1;
	NumRow <= NumRows;
	NumRow++)
     {
      HTM_TR_Begin (NULL);

	 /***** Indent *****/
	 HTM_TD_Begin ("class=\"RT %s\"",Gbl.ColorRows[Gbl.RowEvenOdd]);
	    Ico_PutIcon (NumRow == NumRows ? "subend20x20.gif" :
					     "submid20x20.gif",Ico_BLACK,
			 "","ICO25x25");
	 HTM_TD_End ();

	 /***** Write institution, center, degree *****/
	 HTM_TD_Begin ("colspan=\"%u\" class=\"%s %s LT\"",
		       ColSpan - 1,
		       The_ClassDatSmallNoBR[Gbl.Prefs.Theme],
		       Gbl.ColorRows[Gbl.RowEvenOdd]);

	    /* Get next institution, center, degree */
	    row = mysql_fetch_row (mysql_res);

	    /* Get scope */
	    switch (Sco_GetScopeFromUnsignedStr (row[0]))
	      {
	       case HieLvl_SYS:	// System
		  Ico_PutIcon ("swad64x64.png",Ico_UNCHANGED,
		               Txt_all_degrees,"ICO16x16");
		  HTM_TxtF ("&nbsp;%s",Txt_all_degrees);
		  break;
	       case HieLvl_INS:	// Institution
		  if ((Hie.Ins.InsCod = Str_ConvertStrCodToLongCod (row[1])) > 0)
		    {
		     /* Get data of institution */
		     Ins_GetDataOfInstitByCod (&Hie.Ins);

		     /* Write institution logo and name */
		     Ins_DrawInstitLogoAndNameWithLink (&Hie.Ins,ActSeeInsInf,"LT");
		    }
		  break;
	       case HieLvl_CTR:	// Center
		  if ((Hie.Ctr.CtrCod = Str_ConvertStrCodToLongCod (row[1])) > 0)
		    {
		     /* Get data of center */
		     Ctr_GetDataOfCenterByCod (&Hie.Ctr);

		     /* Write center logo and name */
		     Ctr_DrawCenterLogoAndNameWithLink (&Hie.Ctr,ActSeeCtrInf,"LT");
		    }
		  break;
	       case HieLvl_DEG:	// Degree
		  if ((Hie.Deg.DegCod = Str_ConvertStrCodToLongCod (row[1])) > 0)
		    {
		     /* Get data of degree */
		     Deg_GetDataOfDegreeByCod (&Hie.Deg);

		     /* Write degree logo and name */
		     Deg_DrawDegreeLogoAndNameWithLink (&Hie.Deg,ActSeeDegInf,"LT");
		    }
		  break;
	       default:	// There are no administrators in other scopes
		  Err_WrongScopeExit ();
		  break;
	      }
	 HTM_TD_End ();

      HTM_TR_End ();
     }

   /***** Free structure that stores the query result *****/
   DB_FreeMySQLResult (&mysql_res);
  }

/*****************************************************************************/
/*********************** Write status cell in table **************************/
/*****************************************************************************/

void Hie_WriteStatusCell (Hie_Status_t Status,
			  const char *Class,const char *BgColor,
			  const char *Txt[Hie_NUM_STATUS_TXT])
  {
   Hie_StatusTxt_t StatusTxt = Hie_GetStatusTxtFromStatusBits (Status);

   HTM_TD_Begin ("class=\"%s LM %s\"",Class,BgColor);
      if (StatusTxt != Hie_STATUS_ACTIVE) // If active ==> do not show anything
	 HTM_Txt (Txt[StatusTxt]);
   HTM_TD_End ();
  }

void Hie_WriteStatusCellEditable (bool ICanEdit,Hie_Status_t Status,
                                  Act_Action_t NextAction,long HieCod,
                                  const char *Txt[Hie_NUM_STATUS_TXT])
  {
   extern const char *The_ClassDat[The_NUM_THEMES];
   Hie_StatusTxt_t StatusTxt = Hie_GetStatusTxtFromStatusBits (Status);
   unsigned StatusUnsigned;

   /***** Begin cell *****/
   HTM_TD_Begin ("class=\"%s LM\"",The_ClassDat[Gbl.Prefs.Theme]);
      if (ICanEdit && StatusTxt == Hie_STATUS_PENDING)
	{
	 /* Begin form */
	 Frm_BeginForm (NextAction);
	    Hie_PutParamOtherHieCod (&HieCod);

	    /* Selector */
	    HTM_SELECT_Begin (HTM_SUBMIT_ON_CHANGE,
			      "name=\"Status\" class=\"INPUT_STATUS\"");

	       StatusUnsigned = (unsigned) Hie_GetStatusBitsFromStatusTxt (Hie_STATUS_PENDING);
	       HTM_OPTION (HTM_Type_UNSIGNED,&StatusUnsigned,true,false,
			   "%s",Txt[Hie_STATUS_PENDING]);

	       StatusUnsigned = (unsigned) Hie_GetStatusBitsFromStatusTxt (Hie_STATUS_ACTIVE);
	       HTM_OPTION (HTM_Type_UNSIGNED,&StatusUnsigned,false,false,
			   "%s",Txt[Hie_STATUS_ACTIVE]);

	    HTM_SELECT_End ();

	 /* End form */
	 Frm_EndForm ();
	}
      else if (StatusTxt != Hie_STATUS_ACTIVE)	// If active ==> do not show anything
	 HTM_Txt (Txt[StatusTxt]);

   /***** End cell *****/
   HTM_TD_End ();
  }

/*****************************************************************************/
/**************************** Get parameter status ***************************/
/*****************************************************************************/

Hie_Status_t Hie_GetParamStatus (void)
  {
   Hie_Status_t Status;
   Hie_StatusTxt_t StatusTxt;

   /***** Get parameter with status *****/
   Status = (Hie_Status_t)
	    Par_GetParToUnsignedLong ("Status",
	                              0,
	                              (unsigned long) Hie_MAX_STATUS,
                                      (unsigned long) Hie_WRONG_STATUS);
   if (Status == Hie_WRONG_STATUS)
      Err_WrongStatusExit ();

   StatusTxt = Hie_GetStatusTxtFromStatusBits (Status);
   Status = Hie_GetStatusBitsFromStatusTxt (StatusTxt);	// New status

   return Status;
  }

/*****************************************************************************/
/******************* Set StatusTxt depending on status bits ******************/
/*****************************************************************************/
// Hie_STATUS_UNKNOWN = 0	// Other
// Hie_STATUS_ACTIVE  = 1	// 00 (Status == 0)
// Hie_STATUS_PENDING = 2	// 01 (Status == Hie_STATUS_BIT_PENDING)
// Hie_STATUS_REMOVED = 3	// 1- (Status & Hie_STATUS_BIT_REMOVED)

static Hie_StatusTxt_t Hie_GetStatusTxtFromStatusBits (Hie_Status_t Status)
  {
   if (Status == 0)
      return Hie_STATUS_ACTIVE;
   if (Status == Hie_STATUS_BIT_PENDING)
      return Hie_STATUS_PENDING;
   if (Status & Hie_STATUS_BIT_REMOVED)
      return Hie_STATUS_REMOVED;
   return Hie_STATUS_UNKNOWN;
  }

/*****************************************************************************/
/******************* Set status bits depending on StatusTxt ******************/
/*****************************************************************************/
// Hie_STATUS_UNKNOWN = 0	// Other
// Hie_STATUS_ACTIVE  = 1	// 00 (Status == 0)
// Hie_STATUS_PENDING = 2	// 01 (Status == Hie_STATUS_BIT_PENDING)
// Hie_STATUS_REMOVED = 3	// 1- (Status & Hie_STATUS_BIT_REMOVED)

static Hie_Status_t Hie_GetStatusBitsFromStatusTxt (Hie_StatusTxt_t StatusTxt)
  {
   static const Hie_Status_t StatusBits[Hie_NUM_STATUS_TXT] =
     {
      [Hie_STATUS_UNKNOWN] = (Hie_Status_t) 0,
      [Hie_STATUS_ACTIVE ] = (Hie_Status_t) 0,
      [Hie_STATUS_PENDING] = Hie_STATUS_BIT_PENDING,
      [Hie_STATUS_REMOVED] = Hie_STATUS_BIT_REMOVED,
     };

   return StatusBits[StatusTxt];
  }

/*****************************************************************************/
/**** Write parameter with code of other institution/center/degree/course ****/
/*****************************************************************************/

void Hie_PutParamOtherHieCod (void *HieCod)
  {
   if (HieCod)
      Par_PutHiddenParamLong (NULL,"OthHieCod",*((long *) HieCod));
  }

/*****************************************************************************/
/***** Get parameter with code of other institution/center/degree/course *****/
/*****************************************************************************/

long Hie_GetAndCheckParamOtherHieCod (long MinCodAllowed)
  {
   long HieCod;

   /***** Get and check parameter with code *****/
   if ((HieCod = Par_GetParToLong ("OthHieCod")) < MinCodAllowed)
      Err_WrongHierarchyExit ();

   return HieCod;
  }

/*****************************************************************************/
/*********            Get and show stats about hierarchy           ***********/
/********* (countries, institutions, centers, degrees and courses) ***********/
/*****************************************************************************/

void Hie_GetAndShowHierarchyStats (void)
  {
   extern const char *Hlp_ANALYTICS_Figures_hierarchy;
   extern const char *Txt_FIGURE_TYPES[Fig_NUM_FIGURES];
   Rol_Role_t Role;

   /***** Begin box and table *****/
   Box_BoxTableBegin (NULL,Txt_FIGURE_TYPES[Fig_HIERARCHY],
                      NULL,NULL,
                      Hlp_ANALYTICS_Figures_hierarchy,Box_NOT_CLOSABLE,2);

      Hie_WriteHeadHierarchy ();
      Hie_GetAndShowHierarchyWithInss ();
      Hie_GetAndShowHierarchyWithCtrs ();
      Hie_GetAndShowHierarchyWithDegs ();
      Hie_GetAndShowHierarchyWithCrss ();
      for (Role  = Rol_TCH;
	   Role >= Rol_STD;
	   Role--)
	 Hie_GetAndShowHierarchyWithUsrs (Role);
      Hie_GetAndShowHierarchyTotal ();

   /***** End table and box *****/
   Box_BoxTableEnd ();
  }

/*****************************************************************************/
/************************ Write head of hierarchy table **********************/
/*****************************************************************************/

static void Hie_WriteHeadHierarchy (void)
  {
   extern const char *Txt_Countries;
   extern const char *Txt_Institutions;
   extern const char *Txt_Centers;
   extern const char *Txt_Degrees;
   extern const char *Txt_Courses;

   HTM_TR_Begin (NULL);

      HTM_TH_Empty (1);

      HTM_TH_Begin (HTM_HEAD_RIGHT);
	 Ico_PutIcon ("globe-americas.svg",Ico_BLACK,
	              Txt_Countries,"ICOx16");
	 HTM_BR ();
	 HTM_Txt (Txt_Countries);
      HTM_TH_End ();

      HTM_TH_Begin (HTM_HEAD_RIGHT);
	 Ico_PutIcon ("university.svg",Ico_BLACK,
	              Txt_Institutions,"ICOx16");
	 HTM_BR ();
	 HTM_Txt (Txt_Institutions);
      HTM_TH_End ();

      HTM_TH_Begin (HTM_HEAD_RIGHT);
	 Ico_PutIcon ("building.svg",Ico_BLACK,
	              Txt_Centers,"ICOx16");
	 HTM_BR ();
	 HTM_Txt (Txt_Centers);
      HTM_TH_End ();

      HTM_TH_Begin (HTM_HEAD_RIGHT);
	 Ico_PutIcon ("graduation-cap.svg",Ico_BLACK,
	              Txt_Degrees,"ICOx16");
	 HTM_BR ();
	 HTM_Txt (Txt_Degrees);
      HTM_TH_End ();

      HTM_TH_Begin (HTM_HEAD_RIGHT);
	 Ico_PutIcon ("chalkboard-teacher.svg",Ico_BLACK,
	              Txt_Courses,"ICOx16");
	 HTM_BR ();
	 HTM_Txt (Txt_Courses);
      HTM_TH_End ();

   HTM_TR_End ();
  }

/*****************************************************************************/
/****** Get and show number of elements in hierarchy with institutions *******/
/*****************************************************************************/

static void Hie_GetAndShowHierarchyWithInss (void)
  {
   extern const char *The_ClassDat[The_NUM_THEMES];
   extern const char *Txt_With_;
   extern const char *Txt_institutions;
   unsigned NumCtysWithInss = 1;

   /***** Get number of elements with institutions *****/
   switch (Gbl.Scope.Current)
     {
      case HieLvl_SYS:
	 NumCtysWithInss = Cty_GetCachedNumCtysWithInss ();
         break;
      case HieLvl_CTY:
      case HieLvl_INS:
      case HieLvl_CTR:
      case HieLvl_DEG:
      case HieLvl_CRS:
	 break;
      default:
	 Err_WrongScopeExit ();
	 break;
     }

   /***** Write number of elements with institutions *****/
   Hie_ShowHierarchyRow (Txt_With_,Txt_institutions,
			 The_ClassDat[Gbl.Prefs.Theme],
                         (int) NumCtysWithInss,
                         -1,		// < 0 ==> do not show number
                         -1,		// < 0 ==> do not show number
                         -1,		// < 0 ==> do not show number
			 -1);		// < 0 ==> do not show number
  }

/*****************************************************************************/
/******** Get and show number of elements in hierarchy with centers **********/
/*****************************************************************************/

static void Hie_GetAndShowHierarchyWithCtrs (void)
  {
   extern const char *The_ClassDat[The_NUM_THEMES];
   extern const char *Txt_With_;
   extern const char *Txt_centers;
   unsigned NumCtysWithCtrs = 1;
   unsigned NumInssWithCtrs = 1;

   /***** Get number of elements with centers *****/
   switch (Gbl.Scope.Current)
     {
      case HieLvl_SYS:
	 NumCtysWithCtrs = Cty_GetCachedNumCtysWithCtrs ();
	 /* falls through */
	 /* no break */
      case HieLvl_CTY:
	 NumInssWithCtrs = Ins_GetCachedNumInssWithCtrs ();
         break;
      case HieLvl_INS:
      case HieLvl_CTR:
      case HieLvl_DEG:
      case HieLvl_CRS:
	 break;
      default:
	 Err_WrongScopeExit ();
	 break;
     }

   /***** Write number of elements with centers *****/
   Hie_ShowHierarchyRow (Txt_With_,Txt_centers,
			 The_ClassDat[Gbl.Prefs.Theme],
                         (int) NumCtysWithCtrs,
                         (int) NumInssWithCtrs,
                         -1,		// < 0 ==> do not show number
                         -1,		// < 0 ==> do not show number
			 -1);		// < 0 ==> do not show number
  }

/*****************************************************************************/
/******** Get and show number of elements in hierarchy with degrees **********/
/*****************************************************************************/

static void Hie_GetAndShowHierarchyWithDegs (void)
  {
   extern const char *The_ClassDat[The_NUM_THEMES];
   extern const char *Txt_With_;
   extern const char *Txt_degrees;
   unsigned NumCtysWithDegs = 1;
   unsigned NumInssWithDegs = 1;
   unsigned NumCtrsWithDegs = 1;

   /***** Get number of elements with degrees *****/
   switch (Gbl.Scope.Current)
     {
      case HieLvl_SYS:
	 NumCtysWithDegs = Cty_GetCachedNumCtysWithDegs ();
	 /* falls through */
	 /* no break */
         break;
      case HieLvl_CTY:
	 NumInssWithDegs = Ins_GetCachedNumInssWithDegs ();
	 /* falls through */
	 /* no break */
      case HieLvl_INS:
         NumCtrsWithDegs = Ctr_GetCachedNumCtrsWithDegs ();
         break;
      case HieLvl_CTR:
      case HieLvl_DEG:
      case HieLvl_CRS:
	 break;
      default:
	 Err_WrongScopeExit ();
	 break;
     }

   /***** Write number of elements with degrees *****/
   Hie_ShowHierarchyRow (Txt_With_,Txt_degrees,
			 The_ClassDat[Gbl.Prefs.Theme],
                         (int) NumCtysWithDegs,
                         (int) NumInssWithDegs,
                         (int) NumCtrsWithDegs,
                         -1,		// < 0 ==> do not show number
			 -1);		// < 0 ==> do not show number
  }

/*****************************************************************************/
/******** Get and show number of elements in hierarchy with courses **********/
/*****************************************************************************/

static void Hie_GetAndShowHierarchyWithCrss (void)
  {
   extern const char *The_ClassDat[The_NUM_THEMES];
   extern const char *Txt_With_;
   extern const char *Txt_courses;
   unsigned NumCtysWithCrss = 1;
   unsigned NumInssWithCrss = 1;
   unsigned NumCtrsWithCrss = 1;
   unsigned NumDegsWithCrss = 1;

   /***** Get number of elements with courses *****/
   switch (Gbl.Scope.Current)
     {
      case HieLvl_SYS:
	 NumCtysWithCrss = Cty_GetCachedNumCtysWithCrss ();
	 /* falls through */
	 /* no break */
      case HieLvl_CTY:
	 NumInssWithCrss = Ins_GetCachedNumInssWithCrss ();
	 /* falls through */
	 /* no break */
      case HieLvl_INS:
         NumCtrsWithCrss = Ctr_GetCachedNumCtrsWithCrss ();
	 /* falls through */
	 /* no break */
      case HieLvl_CTR:
	 NumDegsWithCrss = Deg_GetCachedNumDegsWithCrss ();
	 break;
      case HieLvl_DEG:
      case HieLvl_CRS:
	 break;
      default:
	 Err_WrongScopeExit ();
	 break;
     }

   /***** Write number of elements with courses *****/
   Hie_ShowHierarchyRow (Txt_With_,Txt_courses,
			 The_ClassDat[Gbl.Prefs.Theme],
                         (int) NumCtysWithCrss,
                         (int) NumInssWithCrss,
                         (int) NumCtrsWithCrss,
                         (int) NumDegsWithCrss,
			 -1);		// < 0 ==> do not show number
  }

/*****************************************************************************/
/********** Get and show number of elements in hierarchy with users **********/
/*****************************************************************************/

static void Hie_GetAndShowHierarchyWithUsrs (Rol_Role_t Role)
  {
   extern const char *The_ClassDat[The_NUM_THEMES];
   extern const char *Txt_With_;
   extern const char *Txt_ROLES_PLURAL_abc[Rol_NUM_ROLES][Usr_NUM_SEXS];
   unsigned NumCtysWithUsrs;
   unsigned NumInssWithUsrs;
   unsigned NumCtrsWithUsrs;
   unsigned NumDegsWithUsrs;
   unsigned NumCrssWithUsrs;

   /***** Get number of elements with students *****/
   NumCtysWithUsrs = Cty_GetCachedNumCtysWithUsrs (Role);
   NumInssWithUsrs = Ins_GetCachedNumInssWithUsrs (Role);
   NumCtrsWithUsrs = Ctr_GetCachedNumCtrsWithUsrs (Role);
   NumDegsWithUsrs = Deg_GetCachedNumDegsWithUsrs (Role);
   NumCrssWithUsrs = Crs_GetCachedNumCrssWithUsrs (Role);

   /***** Write number of elements with students *****/
   Hie_ShowHierarchyRow (Txt_With_,Txt_ROLES_PLURAL_abc[Role][Usr_SEX_UNKNOWN],
			 The_ClassDat[Gbl.Prefs.Theme],
                         (int) NumCtysWithUsrs,
                         (int) NumInssWithUsrs,
                         (int) NumCtrsWithUsrs,
                         (int) NumDegsWithUsrs,
			 (int) NumCrssWithUsrs);
  }

/*****************************************************************************/
/************ Get and show total number of elements in hierarchy *************/
/*****************************************************************************/

static void Hie_GetAndShowHierarchyTotal (void)
  {
   extern const char *The_ClassDatStrong[The_NUM_THEMES];
   extern const char *Txt_Total;
   char *ClassTxt;
   unsigned NumCtysTotal = 1;
   unsigned NumInssTotal = 1;
   unsigned NumCtrsTotal = 1;
   unsigned NumDegsTotal = 1;
   unsigned NumCrssTotal = 1;

   /***** Get total number of elements *****/
   switch (Gbl.Scope.Current)
     {
      case HieLvl_SYS:
	 NumCtysTotal = Cty_GetCachedNumCtysInSys ();
	 NumInssTotal = Ins_GetCachedNumInssInSys ();
	 NumCtrsTotal = Ctr_GetCachedNumCtrsInSys ();
	 NumDegsTotal = Deg_GetCachedNumDegsInSys ();
	 NumCrssTotal = Crs_GetCachedNumCrssInSys ();
         break;
      case HieLvl_CTY:
	 NumInssTotal = Ins_GetCachedNumInssInCty (Gbl.Hierarchy.Cty.CtyCod);
	 NumCtrsTotal = Ctr_GetCachedNumCtrsInCty (Gbl.Hierarchy.Cty.CtyCod);
	 NumDegsTotal = Deg_GetCachedNumDegsInCty (Gbl.Hierarchy.Cty.CtyCod);
	 NumCrssTotal = Crs_GetCachedNumCrssInCty (Gbl.Hierarchy.Cty.CtyCod);
         break;
      case HieLvl_INS:
	 NumCtrsTotal = Ctr_GetCachedNumCtrsInIns (Gbl.Hierarchy.Ins.InsCod);
	 NumDegsTotal = Deg_GetCachedNumDegsInIns (Gbl.Hierarchy.Ins.InsCod);
	 NumCrssTotal = Crs_GetCachedNumCrssInIns (Gbl.Hierarchy.Ins.InsCod);
         break;
      case HieLvl_CTR:
	 NumDegsTotal = Deg_GetCachedNumDegsInCtr (Gbl.Hierarchy.Ctr.CtrCod);
	 NumCrssTotal = Crs_GetCachedNumCrssInCtr (Gbl.Hierarchy.Ctr.CtrCod);
	 break;
      case HieLvl_DEG:
	 NumCrssTotal = Crs_GetCachedNumCrssInDeg (Gbl.Hierarchy.Deg.DegCod);
	 break;
     case HieLvl_CRS:
	 break;
      default:
	 Err_WrongScopeExit ();
	 break;
     }

   /***** Write total number of elements *****/
   if (asprintf (&ClassTxt,"%s LINE_TOP",The_ClassDatStrong[Gbl.Prefs.Theme]) < 0)
      Err_NotEnoughMemoryExit ();
   Hie_ShowHierarchyRow ("",Txt_Total,ClassTxt,
                         (int) NumCtysTotal,
                         (int) NumInssTotal,
                         (int) NumCtrsTotal,
                         (int) NumDegsTotal,
			 (int) NumCrssTotal);
   free (ClassTxt);
  }

/*****************************************************************************/
/************** Show row with number of elements in hierarchy ****************/
/*****************************************************************************/

static void Hie_ShowHierarchyRow (const char *Text1,const char *Text2,
				  const char *ClassTxt,
				  int NumCtys,	// < 0 ==> do not show number
				  int NumInss,	// < 0 ==> do not show number
				  int NumCtrs,	// < 0 ==> do not show number
				  int NumDegs,	// < 0 ==> do not show number
				  int NumCrss)	// < 0 ==> do not show number
  {
   /***** Begin row *****/
   HTM_TR_Begin (NULL);

      /***** Write text *****/
      HTM_TD_Begin ("class=\"%s RM\"",ClassTxt);
	 HTM_Txt (Text1);
	 HTM_Txt (Text2);
      HTM_TD_End ();

      /***** Write number of countries *****/
      Hie_ShowHierarchyCell (ClassTxt,NumCtys);
      Hie_ShowHierarchyCell (ClassTxt,NumInss);
      Hie_ShowHierarchyCell (ClassTxt,NumCtrs);
      Hie_ShowHierarchyCell (ClassTxt,NumDegs);
      Hie_ShowHierarchyCell (ClassTxt,NumCrss);

   /***** End row *****/
   HTM_TR_End ();
  }

static void Hie_ShowHierarchyCell (const char *ClassTxt,int Num)
  {
   /***** Write number *****/
   HTM_TD_Begin ("class=\"%s RM\"",ClassTxt);
      if (Num >= 0)
	 HTM_Unsigned ((unsigned) Num);
      else		// < 0 ==> do not show number
	 HTM_Hyphen ();
   HTM_TD_End ();
  }
