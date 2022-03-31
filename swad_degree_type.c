// swad_degree_type.c: degree types

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

#include <ctype.h>		// For isprint, isspace, etc.
#include <stdbool.h>		// For boolean type
#include <stddef.h>		// For NULL
#include <stdlib.h>		// For exit, system, calloc, free, etc.
#include <string.h>		// For string functions
#include <mysql/mysql.h>	// To access MySQL databases

#include "swad_box.h"
#include "swad_config.h"
#include "swad_database.h"
#include "swad_degree.h"
#include "swad_degree_database.h"
#include "swad_degree_type.h"
#include "swad_error.h"
#include "swad_figure.h"
#include "swad_form.h"
#include "swad_global.h"
#include "swad_hierarchy_level.h"
#include "swad_HTML.h"
#include "swad_parameter.h"

/*****************************************************************************/
/************** External global variables from others modules ****************/
/*****************************************************************************/

extern struct Globals Gbl;

/*****************************************************************************/
/**************************** Private variables ******************************/
/*****************************************************************************/

static struct DegreeType *DegTyp_EditingDegTyp = NULL;	// Static variable to keep the degree type being edited

/*****************************************************************************/
/*************************** Private prototypes ******************************/
/*****************************************************************************/

static void DegTyp_SeeDegreeTypes (Act_Action_t NextAction,HieLvl_Level_t Scope,
                                   DegTyp_Order_t DefaultOrder);
static DegTyp_Order_t DegTyp_GetParamDegTypOrder (DegTyp_Order_t DefaultOrder);

static void DegTyp_ListDegreeTypes (Act_Action_t NextAction,
                                    HieLvl_Level_t Scope,
                                    DegTyp_Order_t SelectedOrder);

static void DegTyp_EditDegreeTypesInternal (void);
static void DegTyp_PutIconsEditingDegreeTypes (__attribute__((unused)) void *Args);

static void DegTyp_ListDegreeTypesForSeeing (void);
static void DegTyp_PutIconsListingDegTypes (__attribute__((unused)) void *Args);
static void DegTyp_PutIconToEditDegTypes (__attribute__((unused)) void *Args);
static void DegTyp_ListDegreeTypesForEdition (void);

static void DegTyp_PutFormToCreateDegreeType (void);

static void DegTyp_PutHeadDegreeTypesForSeeing (Act_Action_t NextAction,
                                                HieLvl_Level_t Scope,
                                                DegTyp_Order_t SelectedOrder);
static void DegTyp_PutHeadDegreeTypesForEdition (void);

static void DegTyp_PutParamOtherDegTypCod (void *DegTypCod);

static void DegTyp_RemoveDegreeTypeCompletely (long DegTypCod);

static void DegTyp_EditingDegreeTypeConstructor (void);
static void DegTyp_EditingDegreeTypeDestructor (void);

/*****************************************************************************/
/************** Show selector of degree types for statistics *****************/
/*****************************************************************************/

void DegTyp_WriteSelectorDegreeTypes (long SelectedDegTypCod)
  {
   extern const char *The_Colors[The_NUM_THEMES];
   extern const char *Txt_Any_type_of_degree;
   unsigned NumDegTyp;

   /***** Form to select degree types *****/
   /* Get list of degree types */
   DegTyp_GetListDegreeTypes (HieLvl_SYS,DegTyp_ORDER_BY_DEGREE_TYPE);

   /* List degree types */
   HTM_SELECT_Begin (HTM_SUBMIT_ON_CHANGE,
		     "id=\"OthDegTypCod\" name=\"OthDegTypCod\""
		     " class=\"INPUT_%s\"",
		     The_Colors[Gbl.Prefs.Theme]);
      HTM_OPTION (HTM_Type_STRING,"-1",
		  SelectedDegTypCod == -1L,false,
		  "%s",Txt_Any_type_of_degree);
      for (NumDegTyp = 0;
	   NumDegTyp < Gbl.DegTypes.Num;
	   NumDegTyp++)
	 HTM_OPTION (HTM_Type_LONG,&Gbl.DegTypes.Lst[NumDegTyp].DegTypCod,
		     Gbl.DegTypes.Lst[NumDegTyp].DegTypCod == SelectedDegTypCod,false,
		     "%s",Gbl.DegTypes.Lst[NumDegTyp].DegTypName);
   HTM_SELECT_End ();

   /***** Free list of degree types *****/
   DegTyp_FreeListDegreeTypes ();
  }

/*****************************************************************************/
/***************************** Show degree types *****************************/
/*****************************************************************************/

void DegTyp_SeeDegreeTypesInDegTab (void)
  {
   DegTyp_SeeDegreeTypes (ActSeeDegTyp,HieLvl_SYS,
                          DegTyp_ORDER_BY_DEGREE_TYPE);	// Default order if not specified
  }

void DegTyp_SeeDegreeTypesInStaTab (void)
  {
   DegTyp_SeeDegreeTypes (ActSeeUseGbl,Gbl.Scope.Current,
                          DegTyp_ORDER_BY_NUM_DEGREES);	// Default order if not specified
  }

static void DegTyp_SeeDegreeTypes (Act_Action_t NextAction,HieLvl_Level_t Scope,
                                   DegTyp_Order_t DefaultOrder)
  {
   DegTyp_Order_t SelectedOrder;

   /***** Get parameter with the type of order in the list of degree types *****/
   SelectedOrder = DegTyp_GetParamDegTypOrder (DefaultOrder);

   /***** Get list of degree types *****/
   DegTyp_GetListDegreeTypes (Scope,SelectedOrder);

   /***** List degree types *****/
   DegTyp_ListDegreeTypes (NextAction,Scope,SelectedOrder);

   /***** Free list of degree types *****/
   DegTyp_FreeListDegreeTypes ();
  }

/*****************************************************************************/
/******* Get parameter with the type or order in list of degree types ********/
/*****************************************************************************/

static DegTyp_Order_t DegTyp_GetParamDegTypOrder (DegTyp_Order_t DefaultOrder)
  {
   return (DegTyp_Order_t) Par_GetParToUnsignedLong ("Order",
						     0,
						     DegTyp_NUM_ORDERS - 1,
						     (unsigned long) DefaultOrder);
  }

/*****************************************************************************/
/***************************** List degree types *****************************/
/*****************************************************************************/
// This function can be called from:
// - center tab		=> NextAction = ActSeeDegTyp
// - statistic tab	=> NextAction = ActSeeUseGbl

static void DegTyp_ListDegreeTypes (Act_Action_t NextAction,
                                    HieLvl_Level_t Scope,
                                    DegTyp_Order_t SelectedOrder)
  {
   extern const char *Hlp_CENTER_DegreeTypes;
   extern const char *Hlp_ANALYTICS_Figures_types_of_degree;
   extern const char *Txt_Types_of_degree;
   extern const char *Txt_No_types_of_degree;
   extern const char *Txt_Create_another_type_of_degree;
   extern const char *Txt_Create_type_of_degree;

   /***** Begin box *****/
   switch (NextAction)
     {
      case ActSeeDegTyp:
	 Box_BoxBegin (NULL,Txt_Types_of_degree,
	               DegTyp_PutIconsListingDegTypes,NULL,
		       Hlp_CENTER_DegreeTypes,Box_NOT_CLOSABLE);
	 break;
      case ActSeeUseGbl:
	 Box_BoxBegin (NULL,Txt_Types_of_degree,
	               DegTyp_PutIconToEditDegTypes,NULL,
		       Hlp_ANALYTICS_Figures_types_of_degree,Box_NOT_CLOSABLE);
	 break;
      default:	// Bad call
	 return;
     }

   if (Gbl.DegTypes.Num)
     {
      /***** Begin table *****/
      HTM_TABLE_BeginWideMarginPadding (2);

         /***** Write heading *****/
	 DegTyp_PutHeadDegreeTypesForSeeing (NextAction,Scope,SelectedOrder);

	 /***** List current degree types for seeing *****/
	 DegTyp_ListDegreeTypesForSeeing ();

      /***** End table *****/
      HTM_TABLE_End ();
     }
   else	// No degree types created
      Ale_ShowAlert (Ale_INFO,Txt_No_types_of_degree);

   /***** Button to create degree type  *****/
   if (DegTyp_CheckIfICanCreateDegreeTypes ())
     {
      Frm_BeginForm (ActEdiDegTyp);
	 Btn_PutConfirmButton (Gbl.DegTypes.Num ? Txt_Create_another_type_of_degree :
						  Txt_Create_type_of_degree);
      Frm_EndForm ();
     }

   /***** End box *****/
   Box_BoxEnd ();
  }

/*****************************************************************************/
/************************ Put forms to edit degree types *********************/
/*****************************************************************************/

void DegTyp_EditDegreeTypes (void)
  {
   /***** Degree type constructor *****/
   DegTyp_EditingDegreeTypeConstructor ();

   /***** Edit degree types *****/
   DegTyp_EditDegreeTypesInternal ();

   /***** Degree type destructor *****/
   DegTyp_EditingDegreeTypeDestructor ();
  }

static void DegTyp_EditDegreeTypesInternal (void)
  {
   extern const char *Hlp_CENTER_DegreeTypes_edit;
   extern const char *Txt_Types_of_degree;

   /***** Get list of degree types *****/
   DegTyp_GetListDegreeTypes (HieLvl_SYS,DegTyp_ORDER_BY_DEGREE_TYPE);

   /***** Begin box *****/
   Box_BoxBegin (NULL,Txt_Types_of_degree,
                 DegTyp_PutIconsEditingDegreeTypes,NULL,
                 Hlp_CENTER_DegreeTypes_edit,Box_NOT_CLOSABLE);

      /***** Put a form to create a new degree type *****/
      DegTyp_PutFormToCreateDegreeType ();

      /***** Forms to edit current degree types *****/
      if (Gbl.DegTypes.Num)
	 DegTyp_ListDegreeTypesForEdition ();

   /***** End box *****/
   Box_BoxEnd ();

   /***** Free list of degree types *****/
   DegTyp_FreeListDegreeTypes ();
  }

/*****************************************************************************/
/************ Put contextual icons when editing degree types *****************/
/*****************************************************************************/

static void DegTyp_PutIconsEditingDegreeTypes (__attribute__((unused)) void *Args)
  {
   /***** Put icon to view degree types *****/
   DegTyp_PutIconToViewDegreeTypes ();

   /***** Put icon to view degrees *****/
   Deg_PutIconToViewDegrees ();

   /***** Put icon to show a figure *****/
   Fig_PutIconToShowFigure (Fig_DEGREE_TYPES);
  }

/*****************************************************************************/
/******************* Put link (form) to view degree types ********************/
/*****************************************************************************/

void DegTyp_PutIconToViewDegreeTypes (void)
  {
   extern const char *Txt_Types_of_degree;

   Lay_PutContextualLinkOnlyIcon (ActSeeDegTyp,NULL,
                                  NULL,NULL,
				  "sitemap.svg",Ico_BLACK,
				  Txt_Types_of_degree);
  }

/*****************************************************************************/
/******************* List current degree types for seeing ********************/
/*****************************************************************************/

static void DegTyp_ListDegreeTypesForSeeing (void)
  {
   extern const char *The_Colors[The_NUM_THEMES];
   unsigned NumDegTyp;
   const char *BgColor;

   /***** List degree types with forms for edition *****/
   for (NumDegTyp = 0;
	NumDegTyp < Gbl.DegTypes.Num;
	NumDegTyp++, The_ChangeRowColor ())
     {
      BgColor = (Gbl.DegTypes.Lst[NumDegTyp].DegTypCod ==
	         Gbl.Hierarchy.Deg.DegTypCod) ? "BG_HIGHLIGHT" :
                                                The_GetColorRows ();

      /* Begin table row */
      HTM_TR_Begin (NULL);

	 /* Number of degree type in this list */
	 HTM_TD_Begin ("class=\"RM DAT_STRONG_%s %s\"",
	               The_Colors[Gbl.Prefs.Theme],BgColor);
	    HTM_Unsigned (NumDegTyp + 1);
	 HTM_TD_End ();

	 /* Name of degree type */
	 HTM_TD_Begin ("class=\"LM DAT_STRONG_%s %s\"",
	               The_Colors[Gbl.Prefs.Theme],BgColor);
	    HTM_Txt (Gbl.DegTypes.Lst[NumDegTyp].DegTypName);
	 HTM_TD_End ();

	 /* Number of degrees of this type */
	 HTM_TD_Begin ("class=\"RM DAT_STRONG_%s %s\"",
	               The_Colors[Gbl.Prefs.Theme],BgColor);
	    HTM_Unsigned (Gbl.DegTypes.Lst[NumDegTyp].NumDegs);
	 HTM_TD_End ();

      /* End table row */
      HTM_TR_End ();
     }
  }

/*****************************************************************************/
/************** Put contextual icons in list of degree types *****************/
/*****************************************************************************/

static void DegTyp_PutIconsListingDegTypes (__attribute__((unused)) void *Args)
  {
   /***** Put icon to edit degree types *****/
   DegTyp_PutIconToEditDegTypes (NULL);

   /***** Put icon to view degrees *****/
   Deg_PutIconToViewDegrees ();

   /***** Put icon to show a figure *****/
   Fig_PutIconToShowFigure (Fig_DEGREE_TYPES);
  }

/*****************************************************************************/
/******************* Put link (form) to edit degree types ********************/
/*****************************************************************************/

static void DegTyp_PutIconToEditDegTypes (__attribute__((unused)) void *Args)
  {
   if (Gbl.Hierarchy.Level == HieLvl_CTR &&	// Only editable if center tab is visible
       DegTyp_CheckIfICanCreateDegreeTypes ())
      Ico_PutContextualIconToEdit (ActEdiDegTyp,NULL,
				   NULL,NULL);
  }

/*****************************************************************************/
/******************* List current degree types for edition *******************/
/*****************************************************************************/

static void DegTyp_ListDegreeTypesForEdition (void)
  {
   extern const char *The_Colors[The_NUM_THEMES];
   unsigned NumDegTyp;

   /***** Begin table *****/
   HTM_TABLE_BeginWidePadding (2);

      /***** Write heading *****/
      DegTyp_PutHeadDegreeTypesForEdition ();

      /***** List degree types with forms for edition *****/
      for (NumDegTyp = 0;
	   NumDegTyp < Gbl.DegTypes.Num;
	   NumDegTyp++)
	{
	 /* Begin table row */
	 HTM_TR_Begin (NULL);

	    /* Put icon to remove degree type */
	    HTM_TD_Begin ("class=\"BM\"");
	       if (Gbl.DegTypes.Lst[NumDegTyp].NumDegs)	// Degree type has degrees => deletion forbidden
		  Ico_PutIconRemovalNotAllowed ();
	       else
		  Ico_PutContextualIconToRemove (ActRemDegTyp,NULL,
						 DegTyp_PutParamOtherDegTypCod,&Gbl.DegTypes.Lst[NumDegTyp].DegTypCod);
	    HTM_TD_End ();

	    /* Degree type code */
	    HTM_TD_Begin ("class=\"DAT_%s CODE\"",The_Colors[Gbl.Prefs.Theme]);
	       HTM_Long (Gbl.DegTypes.Lst[NumDegTyp].DegTypCod);
	    HTM_TD_End ();

	    /* Name of degree type */
	    HTM_TD_Begin ("class=\"LM\"");
	       Frm_BeginForm (ActRenDegTyp);
		  DegTyp_PutParamOtherDegTypCod (&Gbl.DegTypes.Lst[NumDegTyp].DegTypCod);
		  HTM_INPUT_TEXT ("DegTypName",DegTyp_MAX_CHARS_DEGREE_TYPE_NAME,
				  Gbl.DegTypes.Lst[NumDegTyp].DegTypName,
				  HTM_SUBMIT_ON_CHANGE,
				  "size=\"25\" class=\"INPUT_%s\""
				  " required=\"required\"",
				  The_Colors[Gbl.Prefs.Theme]);
	       Frm_EndForm ();
	    HTM_TD_End ();

	    /* Number of degrees of this type */
	    HTM_TD_Begin ("class=\"RM DAT_%s\"",The_Colors[Gbl.Prefs.Theme]);
	       HTM_Unsigned (Gbl.DegTypes.Lst[NumDegTyp].NumDegs);
	    HTM_TD_End ();

	 /* End table row */
	 HTM_TR_End ();
	}

   /***** End table *****/
   HTM_TABLE_End ();
  }

/*****************************************************************************/
/******************** Check if I can create degree types *********************/
/*****************************************************************************/

bool DegTyp_CheckIfICanCreateDegreeTypes (void)
  {
   return (Gbl.Usrs.Me.Role.Logged == Rol_SYS_ADM);
  }

/*****************************************************************************/
/******************** Put a form to create a new degree type *****************/
/*****************************************************************************/

static void DegTyp_PutFormToCreateDegreeType (void)
  {
   extern const char *The_Colors[The_NUM_THEMES];
   extern const char *Txt_New_type_of_degree;
   extern const char *Txt_Create_type_of_degree;

   /***** Begin form *****/
   Frm_BeginForm (ActNewDegTyp);

      /***** Begin box and table *****/
      Box_BoxTableBegin (NULL,Txt_New_type_of_degree,
			 NULL,NULL,
			 NULL,Box_NOT_CLOSABLE,2);

	 /***** Write heading *****/
	 DegTyp_PutHeadDegreeTypesForEdition ();

	 /***** Begin table row *****/
	 HTM_TR_Begin (NULL);

	    /***** Column to remove degree type, disabled here *****/
	    HTM_TD_Begin ("class=\"BM\"");
	    HTM_TD_End ();

	    /***** Degree type code *****/
	    HTM_TD_Begin ("class=\"CODE\"");
	    HTM_TD_End ();

	    /***** Degree type name *****/
	    HTM_TD_Begin ("class=\"LM\"");
	       HTM_INPUT_TEXT ("DegTypName",DegTyp_MAX_CHARS_DEGREE_TYPE_NAME,DegTyp_EditingDegTyp->DegTypName,
			       HTM_DONT_SUBMIT_ON_CHANGE,
			       "size=\"25\" class=\"INPUT_%s\""
			       " required=\"required\"",
			       The_Colors[Gbl.Prefs.Theme]);
	    HTM_TD_End ();

	    /***** Number of degrees of this degree type ****/
	    HTM_TD_Begin ("class=\"RM DAT_%s\"",The_Colors[Gbl.Prefs.Theme]);
	       HTM_Unsigned (0);
	    HTM_TD_End ();

	 /***** End table row *****/
	 HTM_TR_End ();

      /***** End table, send button and end box *****/
      Box_BoxTableWithButtonEnd (Btn_CREATE_BUTTON,Txt_Create_type_of_degree);

   /***** End form *****/
   Frm_EndForm ();
  }

/*****************************************************************************/
/***************** Write header with fields of a degree type *****************/
/*****************************************************************************/

static void DegTyp_PutHeadDegreeTypesForSeeing (Act_Action_t NextAction,
                                                HieLvl_Level_t Scope,
                                                DegTyp_Order_t SelectedOrder)
  {
   extern const char *Txt_DEGREE_TYPES_HELP_ORDER[DegTyp_NUM_ORDERS];
   extern const char *Txt_DEGREE_TYPES_ORDER[DegTyp_NUM_ORDERS];
   DegTyp_Order_t Order;
   static HTM_HeadAlign Align[DegTyp_NUM_ORDERS] =
     {
      [DegTyp_ORDER_BY_DEGREE_TYPE] = HTM_HEAD_LEFT,
      [DegTyp_ORDER_BY_NUM_DEGREES] = HTM_HEAD_RIGHT
     };
   struct Fig_Figures Figures;

   HTM_TR_Begin (NULL);

      HTM_TH_Empty (1);

      for (Order  = (DegTyp_Order_t) 0;
	   Order <= (DegTyp_Order_t) (DegTyp_NUM_ORDERS - 1);
	   Order++)
	{
         HTM_TH_Begin (Align[Order]);

	    /* Begin form to change order */
	    Frm_BeginForm (NextAction);
	       if (NextAction == ActSeeUseGbl)
		 {
		  Figures.Scope      = Scope;
		  Figures.FigureType = Fig_DEGREE_TYPES;
		  Fig_PutHiddenParamFigures (&Figures);
		 }
	       Par_PutHiddenParamUnsigned (NULL,"Order",(unsigned) Order);

	       /* Link with the head of this column */
	       HTM_BUTTON_OnSubmit_Begin (Txt_DEGREE_TYPES_HELP_ORDER[Order],NULL,
	                                  "class=\"BT_LINK\"");
		  if (Order == SelectedOrder)
		     HTM_U_Begin ();
		  HTM_Txt (Txt_DEGREE_TYPES_ORDER[Order]);
		  if (Order == SelectedOrder)
		     HTM_U_End ();
	       HTM_BUTTON_End ();

	    /* End form */
	    Frm_EndForm ();

	 HTM_TH_End ();
	}

   HTM_TR_End ();
  }

/*****************************************************************************/
/***************** Write header with fields of a degree type *****************/
/*****************************************************************************/

static void DegTyp_PutHeadDegreeTypesForEdition (void)
  {
   extern const char *Txt_Code;
   extern const char *Txt_Type_of_degree;
   extern const char *Txt_Degrees;

   HTM_TR_Begin (NULL);

      HTM_TH_Span (NULL         ,HTM_HEAD_CENTER,1,1,"BT");
      HTM_TH (Txt_Code          ,HTM_HEAD_CENTER);
      HTM_TH (Txt_Type_of_degree,HTM_HEAD_CENTER);
      HTM_TH (Txt_Degrees       ,HTM_HEAD_RIGHT );

   HTM_TR_End ();
  }

/*****************************************************************************/
/****************** Create a list with all degree types **********************/
/*****************************************************************************/

void DegTyp_GetListDegreeTypes (HieLvl_Level_t Scope,DegTyp_Order_t Order)
  {
   MYSQL_RES *mysql_res;
   MYSQL_ROW row;
   unsigned NumTyp;

   /***** Get types of degree from database *****/
   Gbl.DegTypes.Num = Deg_DB_GetDegreeTypes (&mysql_res,Scope,Order);

   /***** Get degree types *****/
   if (Gbl.DegTypes.Num)
     {
      /***** Create a list of degree types *****/
      if ((Gbl.DegTypes.Lst = calloc (Gbl.DegTypes.Num,
			              sizeof (*Gbl.DegTypes.Lst))) == NULL)
         Err_NotEnoughMemoryExit ();

      /***** Get degree types *****/
      for (NumTyp = 0;
	   NumTyp < Gbl.DegTypes.Num;
	   NumTyp++)
        {
         /* Get next degree type */
         row = mysql_fetch_row (mysql_res);

         /* Get degree type code (row[0]) */
         if ((Gbl.DegTypes.Lst[NumTyp].DegTypCod = Str_ConvertStrCodToLongCod (row[0])) <= 0)
            Err_WrongDegTypExit ();

         /* Get degree type name (row[1]) */
         Str_Copy (Gbl.DegTypes.Lst[NumTyp].DegTypName,row[1],
                   sizeof (Gbl.DegTypes.Lst[NumTyp].DegTypName) - 1);

         /* Number of degrees of this type (row[2]) */
         if (sscanf (row[2],"%u",&Gbl.DegTypes.Lst[NumTyp].NumDegs) != 1)
            Err_ShowErrorAndExit ("Error when getting number of degrees of a type");
        }
     }

   /***** Free structure that stores the query result *****/
   DB_FreeMySQLResult (&mysql_res);
  }

/*****************************************************************************/
/********* Free list of degree types and list of degrees of each type ********/
/*****************************************************************************/

void DegTyp_FreeListDegreeTypes (void)
  {
   /***** Free memory used by the list of degree types *****/
   if (Gbl.DegTypes.Lst)
     {
      free (Gbl.DegTypes.Lst);
      Gbl.DegTypes.Lst = NULL;
      Gbl.DegTypes.Num = 0;
     }
  }

/*****************************************************************************/
/***************** Receive form to create a new degree type ******************/
/*****************************************************************************/

void DegTyp_ReceiveFormNewDegreeType (void)
  {
   extern const char *Txt_The_type_of_degree_X_already_exists;
   extern const char *Txt_Created_new_type_of_degree_X;
   extern const char *Txt_You_must_specify_the_name_of_the_new_type_of_degree;

   /***** Degree type constructor *****/
   DegTyp_EditingDegreeTypeConstructor ();

   /***** Get parameters from form *****/
   /* Get the name of degree type */
   Par_GetParToText ("DegTypName",DegTyp_EditingDegTyp->DegTypName,DegTyp_MAX_BYTES_DEGREE_TYPE_NAME);

   if (DegTyp_EditingDegTyp->DegTypName[0])	// If there's a degree type name
     {
      /***** If name of degree type was in database... *****/
      if (Deg_DB_CheckIfDegreeTypeNameExists (DegTyp_EditingDegTyp->DegTypName,-1L))
         Ale_CreateAlert (Ale_WARNING,NULL,
                          Txt_The_type_of_degree_X_already_exists,
                          DegTyp_EditingDegTyp->DegTypName);
      else	// Add new degree type to database
        {
         Deg_DB_CreateDegreeType (DegTyp_EditingDegTyp->DegTypName);
      	 Ale_CreateAlert (Ale_SUCCESS,NULL,
      	                  Txt_Created_new_type_of_degree_X,
			  DegTyp_EditingDegTyp->DegTypName);
        }
     }
   else	// If there is not a degree type name
      Ale_CreateAlert (Ale_WARNING,NULL,
	               Txt_You_must_specify_the_name_of_the_new_type_of_degree);
  }

/*****************************************************************************/
/**************************** Remove a degree type ***************************/
/*****************************************************************************/

void DegTyp_RemoveDegreeType (void)
  {
   extern const char *Txt_To_remove_a_type_of_degree_you_must_first_remove_all_degrees_of_that_type;
   extern const char *Txt_Type_of_degree_X_removed;

   /***** Degree type constructor *****/
   DegTyp_EditingDegreeTypeConstructor ();

   /***** Get the code of the degree type *****/
   DegTyp_EditingDegTyp->DegTypCod = DegTyp_GetAndCheckParamOtherDegTypCod (1);

   /***** Get data of the degree type from database *****/
   if (!DegTyp_GetDataOfDegreeTypeByCod (DegTyp_EditingDegTyp))
      Err_WrongDegTypExit ();

   /***** Check if this degree type has degrees *****/
   if (DegTyp_EditingDegTyp->NumDegs)	// Degree type has degrees => don't remove
      Ale_CreateAlert (Ale_WARNING,NULL,
	               Txt_To_remove_a_type_of_degree_you_must_first_remove_all_degrees_of_that_type);
   else	// Degree type has no degrees => remove it
     {
      /***** Remove degree type *****/
      DegTyp_RemoveDegreeTypeCompletely (DegTyp_EditingDegTyp->DegTypCod);

      /***** Write message to show the change made *****/
      Ale_CreateAlert (Ale_SUCCESS,NULL,
	               Txt_Type_of_degree_X_removed,
                       DegTyp_EditingDegTyp->DegTypName);
     }
  }

/*****************************************************************************/
/***************** Write parameter with code of degree type ******************/
/*****************************************************************************/

static void DegTyp_PutParamOtherDegTypCod (void *DegTypCod)
  {
   if (DegTypCod)
      Par_PutHiddenParamLong (NULL,"OthDegTypCod",*((long *) DegTypCod));
  }

/*****************************************************************************/
/******************* Get parameter with code of degree type ******************/
/*****************************************************************************/

long DegTyp_GetAndCheckParamOtherDegTypCod (long MinCodAllowed)
  {
   long DegTypCod;

   /***** Get and check parameter with code of degree type *****/
   if ((DegTypCod = Par_GetParToLong ("OthDegTypCod")) < MinCodAllowed)
      Err_WrongDegTypExit ();

   return DegTypCod;
  }

/*****************************************************************************/
/****************** Get data of a degree type from its code ******************/
/*****************************************************************************/

bool DegTyp_GetDataOfDegreeTypeByCod (struct DegreeType *DegTyp)
  {
   /***** Trivial check: code of degree type should be >= 0 *****/
   if (DegTyp->DegTypCod <= 0)
     {
      DegTyp->DegTypName[0] = '\0';
      DegTyp->NumDegs = 0;
      return false;
     }

   /***** Get the name of a type of degree from database *****/
   Deg_DB_GetDegTypeNameByCod (DegTyp);
   if (DegTyp->DegTypName[0])
     {
      /* Count number of degrees of this type */
      DegTyp->NumDegs = Deg_DB_GetNumDegsOfType (DegTyp->DegTypCod);
      return true;
     }

   DegTyp->DegTypName[0] = '\0';
   DegTyp->NumDegs = 0;
   return false;
  }

/*****************************************************************************/
/******************** Remove a degree type and its degrees *******************/
/*****************************************************************************/

static void DegTyp_RemoveDegreeTypeCompletely (long DegTypCod)
  {
   MYSQL_RES *mysql_res;
   unsigned NumDegs;
   unsigned NumDeg;
   long DegCod;

   /***** Get degrees of a type from database *****/
   NumDegs = Deg_DB_GetDegsOfType (&mysql_res,DegTypCod);

   /***** Remove degrees ******/
   for (NumDeg = 0;
	NumDeg < NumDegs;
	NumDeg++)
     {
      /* Get next degree */
      if ((DegCod = DB_GetNextCode (mysql_res)) < 0)
         Err_WrongDegreeExit ();

      /* Remove degree */
      Deg_RemoveDegreeCompletely (DegCod);
     }

   /* Free structure that stores the query result */
   DB_FreeMySQLResult (&mysql_res);

   /***** Remove the degree type *****/
   Deg_DB_RemoveDegTyp (DegTypCod);
  }

/*****************************************************************************/
/**************************** Rename a degree type ***************************/
/*****************************************************************************/

void DegTyp_RenameDegreeType (void)
  {
   extern const char *Txt_The_type_of_degree_X_already_exists;
   extern const char *Txt_The_type_of_degree_X_has_been_renamed_as_Y;
   extern const char *Txt_The_name_of_the_type_of_degree_X_has_not_changed;
   char NewNameDegTyp[DegTyp_MAX_BYTES_DEGREE_TYPE_NAME + 1];

   /***** Degree type constructor *****/
   DegTyp_EditingDegreeTypeConstructor ();

   /***** Get parameters from form *****/
   /* Get the code of the degree type */
   DegTyp_EditingDegTyp->DegTypCod = DegTyp_GetAndCheckParamOtherDegTypCod (1);

   /* Get the new name for the degree type */
   Par_GetParToText ("DegTypName",NewNameDegTyp,DegTyp_MAX_BYTES_DEGREE_TYPE_NAME);

   /***** Get from the database the old name of the degree type *****/
   if (!DegTyp_GetDataOfDegreeTypeByCod (DegTyp_EditingDegTyp))
      Err_WrongDegTypExit ();

   /***** Check if new name is empty *****/
   if (NewNameDegTyp[0])
     {
      /***** Check if old and new names are the same
             (this happens when return is pressed without changes) *****/
      if (strcmp (DegTyp_EditingDegTyp->DegTypName,NewNameDegTyp))	// Different names
        {
         /***** If degree type was in database... *****/
         if (Deg_DB_CheckIfDegreeTypeNameExists (NewNameDegTyp,DegTyp_EditingDegTyp->DegTypCod))
            Ale_CreateAlert (Ale_WARNING,NULL,
        	             Txt_The_type_of_degree_X_already_exists,
                             NewNameDegTyp);
         else
           {
            /* Update the table changing old name by new name */
            Deg_DB_UpdateDegTypName (DegTyp_EditingDegTyp->DegTypCod,NewNameDegTyp);

            /* Write message to show the change made */
            Ale_CreateAlert (Ale_SUCCESS,NULL,
        	             Txt_The_type_of_degree_X_has_been_renamed_as_Y,
                             DegTyp_EditingDegTyp->DegTypName,NewNameDegTyp);
           }


        }
      else	// The same name
         Ale_CreateAlert (Ale_INFO,NULL,
                          Txt_The_name_of_the_type_of_degree_X_has_not_changed,
                          NewNameDegTyp);
     }
   else
      Ale_CreateAlertYouCanNotLeaveFieldEmpty ();

   /***** Set degree type name *****/
   Str_Copy (DegTyp_EditingDegTyp->DegTypName,NewNameDegTyp,
	     sizeof (DegTyp_EditingDegTyp->DegTypName) - 1);
  }

/*****************************************************************************/
/********** Show message of success after changing a degree type *************/
/*****************************************************************************/

void DegTyp_ContEditAfterChgDegTyp (void)
  {
   /***** Show possible delayed alerts *****/
   Ale_ShowAlerts (NULL);

   /***** Show the form again *****/
   DegTyp_EditDegreeTypesInternal ();

   /***** Degree type destructor *****/
   DegTyp_EditingDegreeTypeDestructor ();
  }

/*****************************************************************************/
/********************* Degree type constructor/destructor ********************/
/*****************************************************************************/

static void DegTyp_EditingDegreeTypeConstructor (void)
  {
   /***** Pointer must be NULL *****/
   if (DegTyp_EditingDegTyp != NULL)
      Err_WrongDegTypExit ();

   /***** Allocate memory for degree type *****/
   if ((DegTyp_EditingDegTyp = malloc (sizeof (*DegTyp_EditingDegTyp))) == NULL)
      Err_NotEnoughMemoryExit ();

   /***** Reset degree type *****/
   DegTyp_EditingDegTyp->DegTypCod     = -1L;
   DegTyp_EditingDegTyp->DegTypName[0] = '\0';
   DegTyp_EditingDegTyp->NumDegs       = 0;
  }

static void DegTyp_EditingDegreeTypeDestructor (void)
  {
   /***** Free memory used for degree type *****/
   if (DegTyp_EditingDegTyp != NULL)
     {
      free (DegTyp_EditingDegTyp);
      DegTyp_EditingDegTyp = NULL;
     }
  }

/*****************************************************************************/
/****************** Get and show stats about institutions ********************/
/*****************************************************************************/

void DegTyp_GetAndShowDegreeTypesStats (void)
  {
   /***** Show statistic about number of degrees in each type of degree *****/
   DegTyp_SeeDegreeTypesInStaTab ();
  }
