// swad_department.c: departments

/*
    SWAD (Shared Workspace At a Distance),
    is a web platform developed at the University of Granada (Spain),
    and used to support university teaching.

    This file is part of SWAD core.
    Copyright (C) 1999-2022 Antonio Ca�as Vargas

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
#include <stdbool.h>		// For boolean type
#include <stddef.h>		// For NULL
#include <stdio.h>		// For asprintf
#include <stdlib.h>		// For calloc
#include <string.h>		// For string functions

#include "swad_action_list.h"
#include "swad_alert.h"
#include "swad_box.h"
#include "swad_constant.h"
#include "swad_database.h"
#include "swad_department.h"
#include "swad_department_database.h"
#include "swad_error.h"
#include "swad_form.h"
#include "swad_global.h"
#include "swad_HTML.h"
#include "swad_institution.h"
#include "swad_language.h"
#include "swad_parameter.h"
#include "swad_string.h"

/*****************************************************************************/
/************** External global variables from others modules ****************/
/*****************************************************************************/

extern struct Globals Gbl;

/*****************************************************************************/
/***************************** Private variables *****************************/
/*****************************************************************************/

static struct Dpt_Department *Dpt_EditingDpt = NULL;	// Static variable to keep the department being edited

/*****************************************************************************/
/***************************** Private prototypes ****************************/
/*****************************************************************************/

static void Dpt_ResetDepartments (struct Dpt_Departments *Departments);

static Dpt_Order_t Dpt_GetParamDptOrder (void);
static void Dpt_PutIconToEditDpts (__attribute__((unused)) void *Args);
static void Dpt_EditDepartmentsInternal (void);

static void Dpt_GetListDepartments (struct Dpt_Departments *Departments,long InsCod);

static void Dpt_ListDepartmentsForEdition (const struct Dpt_Departments *Departments);
static void Dpt_PutParamDptCod (void *DptCod);

static void Dpt_RenameDepartment (Cns_ShrtOrFullName_t ShrtOrFullName);

static void Dpt_PutFormToCreateDepartment (void);
static void Dpt_PutHeadDepartments (void);

static void Dpt_EditingDepartmentConstructor (void);
static void Dpt_EditingDepartmentDestructor (void);

/*****************************************************************************/
/************************* Reset departments context *************************/
/*****************************************************************************/

static void Dpt_ResetDepartments (struct Dpt_Departments *Departments)
  {
   Departments->Num           = 0;
   Departments->Lst           = NULL;
   Departments->SelectedOrder = Dpt_ORDER_DEFAULT;
  }

/*****************************************************************************/
/*************************** List all departments ****************************/
/*****************************************************************************/

void Dpt_SeeDepts (void)
  {
   extern const char *Hlp_INSTITUTION_Departments;
   extern const char *Txt_Departments_of_INSTITUTION_X;
   extern const char *Txt_DEPARTMENTS_HELP_ORDER[2];
   extern const char *Txt_DEPARTMENTS_ORDER[2];
   extern const char *Txt_Other_departments;
   extern const char *Txt_Department_unspecified;
   struct Dpt_Departments Departments;
   char *Title;
   Dpt_Order_t Order;
   static HTM_HeadAlign Align[Dpt_NUM_ORDERS] =
     {
      [Dpt_ORDER_BY_DEPARTMENT] = HTM_HEAD_LEFT,
      [Dpt_ORDER_BY_NUM_TCHS  ] = HTM_HEAD_RIGHT
     };
   unsigned NumDpt;
   unsigned NumTchsInsInOtherDpts;
   unsigned NumTchsInsWithNoDpt;

   /***** Trivial check *****/
   if (Gbl.Hierarchy.Ins.InsCod <= 0)	// No institution selected
      return;

   /***** Reset departments context *****/
   Dpt_ResetDepartments (&Departments);

   /***** Get parameter with the type of order in the list of departments *****/
   Departments.SelectedOrder = Dpt_GetParamDptOrder ();

   /***** Get list of departments *****/
   Dpt_GetListDepartments (&Departments,Gbl.Hierarchy.Ins.InsCod);

   /***** Begin box and table *****/
   if (asprintf (&Title,Txt_Departments_of_INSTITUTION_X,Gbl.Hierarchy.Ins.FullName) < 0)
      Err_NotEnoughMemoryExit ();
   Box_BoxTableBegin (NULL,Title,
		      Gbl.Usrs.Me.Role.Logged == Rol_SYS_ADM ? Dpt_PutIconToEditDpts :
							       NULL,
		      NULL,
		      Hlp_INSTITUTION_Departments,Box_NOT_CLOSABLE,2);
   free (Title);

      /***** Write heading *****/
      HTM_TR_Begin (NULL);
	 for (Order  = (Dpt_Order_t) 0;
	      Order <= (Dpt_Order_t) (Dpt_NUM_ORDERS - 1);
	      Order++)
	   {
            HTM_TH_Begin (Align[Order]);

	       Frm_BeginForm (ActSeeDpt);
		  Par_PutParUnsigned (NULL,"Order",(unsigned) Order);
		  HTM_BUTTON_Submit_Begin (Txt_DEPARTMENTS_HELP_ORDER[Order],
		                           "class=\"BT_LINK\"");
		     if (Order == Departments.SelectedOrder)
			HTM_U_Begin ();
		     HTM_Txt (Txt_DEPARTMENTS_ORDER[Order]);
		     if (Order == Departments.SelectedOrder)
			HTM_U_End ();
		  HTM_BUTTON_End ();
	       Frm_EndForm ();

	    HTM_TH_End ();
	   }
      HTM_TR_End ();

      /***** Write all departments and their number of teachers *****/
      for (NumDpt = 0;
	   NumDpt < Departments.Num;
	   NumDpt++)
	{
	 /* Write data of this department */
	 HTM_TR_Begin (NULL);

	    HTM_TD_Begin ("class=\"LM\"");
	       HTM_A_Begin ("href=\"%s\" target=\"_blank\" class=\"DAT_%s\"",
			    Departments.Lst[NumDpt].WWW,
			    The_GetSuffix ());
		  HTM_Txt (Departments.Lst[NumDpt].FullName);
	       HTM_A_End ();
	    HTM_TD_End ();

	    HTM_TD_Begin ("class=\"RM DAT_%s\"",The_GetSuffix ());
	       HTM_Unsigned (Departments.Lst[NumDpt].NumTchs);
	    HTM_TD_End ();

	 HTM_TR_End ();
	}

      /***** Separation row *****/
      HTM_TR_Begin (NULL);
	 HTM_TD_Begin ("colspan=\"3\" class=\"DAT_%s\"",
	               The_GetSuffix ());
	    HTM_NBSP ();
	 HTM_TD_End ();
      HTM_TR_End ();

      /***** Write teachers of this institution with other department *****/
      NumTchsInsInOtherDpts = Dpt_DB_GetNumTchsCurrentInsInDepartment (0);

      HTM_TR_Begin (NULL);

	 HTM_TD_Begin ("class=\"LM DAT_%s\"",The_GetSuffix ());
	    HTM_Txt (Txt_Other_departments);
	 HTM_TD_End ();

	 HTM_TD_Begin ("class=\"RM DAT_%s\"",The_GetSuffix ());
	    HTM_Unsigned (NumTchsInsInOtherDpts);
	 HTM_TD_End ();

      HTM_TR_End ();

      /***** Write teachers with no department *****/
      NumTchsInsWithNoDpt = Dpt_DB_GetNumTchsCurrentInsInDepartment (-1L);

      HTM_TR_Begin (NULL);

	 HTM_TD_Begin ("class=\"LM DAT_%s\"",The_GetSuffix ());
	    HTM_Txt (Txt_Department_unspecified);
	 HTM_TD_End ();

	 HTM_TD_Begin ("class=\"RM DAT_%s\"",The_GetSuffix ());
	    HTM_Unsigned (NumTchsInsWithNoDpt);
	 HTM_TD_End ();

      HTM_TR_End ();

   /***** End table and box *****/
   Box_BoxTableEnd ();

   /***** Free list of departments *****/
   Dpt_FreeListDepartments (&Departments);
  }

/*****************************************************************************/
/******** Get parameter with the type or order in list of departments ********/
/*****************************************************************************/

static Dpt_Order_t Dpt_GetParamDptOrder (void)
  {
   return (Dpt_Order_t) Par_GetParUnsignedLong ("Order",
						  0,
						  Dpt_NUM_ORDERS - 1,
						  (unsigned long) Dpt_ORDER_DEFAULT);
  }

/*****************************************************************************/
/************************ Put icon to edit departments ***********************/
/*****************************************************************************/

static void Dpt_PutIconToEditDpts (__attribute__((unused)) void *Args)
  {
   Ico_PutContextualIconToEdit (ActEdiDpt,NULL,
				NULL,NULL);
  }

/*****************************************************************************/
/******* Put forms to edit the departments of the current institution ********/
/*****************************************************************************/

void Dpt_EditDepartments (void)
  {
   /***** Department constructor *****/
   Dpt_EditingDepartmentConstructor ();

   /***** Edit departments *****/
   Dpt_EditDepartmentsInternal ();

   /***** Department destructor *****/
   Dpt_EditingDepartmentDestructor ();
  }

static void Dpt_EditDepartmentsInternal (void)
  {
   extern const char *Hlp_INSTITUTION_Departments_edit;
   extern const char *Txt_Departments_of_INSTITUTION_X;
   struct Dpt_Departments Departments;
   char *Title;

   /***** Trivial check *****/
   if (Gbl.Hierarchy.Ins.InsCod <= 0)	// An institution must be selected
      return;

   /***** Reset departments context *****/
   Dpt_ResetDepartments (&Departments);

   /***** Get list of institutions *****/
   Ins_GetBasicListOfInstitutions (Gbl.Hierarchy.Cty.CtyCod);

   /***** Get list of departments *****/
   Dpt_GetListDepartments (&Departments,Gbl.Hierarchy.Ins.InsCod);

   /***** Begin box *****/
   if (asprintf (&Title,Txt_Departments_of_INSTITUTION_X,Gbl.Hierarchy.Ins.FullName) < 0)
      Err_NotEnoughMemoryExit ();
   Box_BoxBegin (NULL,Title,NULL,NULL,
                 Hlp_INSTITUTION_Departments_edit,Box_NOT_CLOSABLE);
   free (Title);

      /***** Put a form to create a new department *****/
      Dpt_PutFormToCreateDepartment ();

      /***** Forms to edit current departments *****/
      if (Departments.Num)
	 Dpt_ListDepartmentsForEdition (&Departments);

   /***** End box *****/
   Box_BoxEnd ();

   /***** Free list of departments *****/
   Dpt_FreeListDepartments (&Departments);

   /***** Free list of institutions *****/
   Ins_FreeListInstitutions ();
  }

/*****************************************************************************/
/************************** Get list of departments **************************/
/*****************************************************************************/
// If InsCod  > 0 ==> get departments of an institution
// If InsCod <= 0 ==> an empty list is returned

static void Dpt_GetListDepartments (struct Dpt_Departments *Departments,long InsCod)
  {
   MYSQL_RES *mysql_res;
   MYSQL_ROW row;
   unsigned NumDpt;
   struct Dpt_Department *Dpt;

   /***** Free list of departments *****/
   Dpt_FreeListDepartments (Departments);	// List is initialized to empty

   if (InsCod > 0)	// Institution specified
     {
      /***** Get departments from database *****/
      Departments->Num = Dpt_DB_GetListDepartments (&mysql_res,InsCod,
                                                    Departments->SelectedOrder);
      if (Departments->Num) // Departments found...
	{
	 /***** Create list with courses in degree *****/
	 if ((Departments->Lst = calloc (Departments->Num,
	                                 sizeof (*Departments->Lst))) == NULL)
	    Err_NotEnoughMemoryExit ();

	 /***** Get the departments *****/
	 for (NumDpt = 0;
	      NumDpt < Departments->Num;
	      NumDpt++)
	   {
	    Dpt = &(Departments->Lst[NumDpt]);

	    /* Get next department */
	    row = mysql_fetch_row (mysql_res);

	    /* Get department code (row[0]) */
	    if ((Dpt->DptCod = Str_ConvertStrCodToLongCod (row[0])) <= 0)
	       Err_WrongDepartmentExit ();

	    /* Get institution code (row[1]) */
	    if ((Dpt->InsCod = Str_ConvertStrCodToLongCod (row[1])) <= 0)
	       Err_WrongInstitExit ();

	    /* Get short name (row[2]), full name (row[3])
	       and URL (row[4]) of the department  */
	    Str_Copy (Dpt->ShrtName,row[2],sizeof (Dpt->ShrtName) - 1);
	    Str_Copy (Dpt->FullName,row[3],sizeof (Dpt->FullName) - 1);
	    Str_Copy (Dpt->WWW     ,row[4],sizeof (Dpt->WWW     ) - 1);

	    /* Get number of non-editing teachers and teachers in this department (row[5]) */
	    if (sscanf (row[5],"%u",&Dpt->NumTchs) != 1)
	       Dpt->NumTchs = 0;
	   }
	}

      /***** Free structure that stores the query result *****/
      DB_FreeMySQLResult (&mysql_res);
     }
  }

/*****************************************************************************/
/****************** Get data of department using its code ********************/
/*****************************************************************************/

void Dpt_GetDataOfDepartmentByCod (struct Dpt_Department *Dpt)
  {
   extern const char *Txt_Another_department;
   MYSQL_RES *mysql_res;
   MYSQL_ROW row;

   /***** Clear data *****/
   Dpt->InsCod = -1L;
   Dpt->ShrtName[0] = Dpt->FullName[0] = Dpt->WWW[0] = '\0';
   Dpt->NumTchs = 0;

   /***** Check if department code is correct *****/
   if (Dpt->DptCod == 0)
     {
      Str_Copy (Dpt->ShrtName,Txt_Another_department,sizeof (Dpt->ShrtName) - 1);
      Str_Copy (Dpt->FullName,Txt_Another_department,sizeof (Dpt->FullName) - 1);
     }
   else if (Dpt->DptCod > 0)
     {
      /***** Get data of a department from database *****/
      if (Dpt_DB_GetDataOfDepartmentByCod (&mysql_res,Dpt->DptCod)) // Department found...
        {
         /* Get row */
         row = mysql_fetch_row (mysql_res);

         /* Get the code of the institution (row[0]) */
         Dpt->InsCod = Str_ConvertStrCodToLongCod (row[0]);

	 /* Get short name (row[1]), full name (row[2])
	    and URL (row[3]) of the department  */
         Str_Copy (Dpt->ShrtName,row[1],sizeof (Dpt->ShrtName) - 1);
         Str_Copy (Dpt->FullName,row[2],sizeof (Dpt->FullName) - 1);
         Str_Copy (Dpt->WWW     ,row[3],sizeof (Dpt->WWW     ) - 1);

         /* Get number of teachers in this department (row[4]) */
         if (sscanf (row[4],"%u",&Dpt->NumTchs) != 1)
            Dpt->NumTchs = 0;
        }

      /***** Free structure that stores the query result *****/
      DB_FreeMySQLResult (&mysql_res);
     }
  }

/*****************************************************************************/
/************************** Free list of departments *************************/
/*****************************************************************************/

void Dpt_FreeListDepartments (struct Dpt_Departments *Departments)
  {
   if (Departments->Lst)
      /***** Free memory used by the list of departments *****/
      free (Departments->Lst);

   Departments->Lst = NULL;
   Departments->Num = 0;
  }

/*****************************************************************************/
/***************************** List all departments **************************/
/*****************************************************************************/

static void Dpt_ListDepartmentsForEdition (const struct Dpt_Departments *Departments)
  {
   extern const char *Txt_Another_institution;
   unsigned NumDpt;
   struct Dpt_Department *Dpt;
   struct Ins_Instit Ins;
   unsigned NumIns;

   /***** Begin table *****/
   HTM_TABLE_BeginPadding (2);

      /***** Write heading *****/
      Dpt_PutHeadDepartments ();

      /***** Write all departments *****/
      for (NumDpt = 0;
	   NumDpt < Departments->Num;
	   NumDpt++)
	{
	 Dpt = &Departments->Lst[NumDpt];

	 /* Get data of institution of this department */
	 Ins.InsCod = Dpt->InsCod;
	 Ins_GetDataOfInstitByCod (&Ins);

	 HTM_TR_Begin (NULL);

	    /* Put icon to remove department */
	    HTM_TD_Begin ("class=\"BM\"");
	       if (Dpt->NumTchs)	// Department has teachers ==> deletion forbidden
		  Ico_PutIconRemovalNotAllowed ();
	       else
		  Ico_PutContextualIconToRemove (ActRemDpt,NULL,
						 Dpt_PutParamDptCod,&Dpt->DptCod);
	    HTM_TD_End ();

	    /* Department code */
	    HTM_TD_Begin ("class=\"RM DAT_%s\"",The_GetSuffix ());
	       HTM_TxtF ("%ld&nbsp;",Dpt->DptCod);
	    HTM_TD_End ();

	    /* Institution */
	    HTM_TD_Begin ("class=\"CM\"");
	       Frm_BeginForm (ActChgDptIns);
		  Dpt_PutParamDptCod (&Dpt->DptCod);
		  HTM_SELECT_Begin (HTM_SUBMIT_ON_CHANGE,
				    "name=\"OthInsCod\""
				    " class=\"HIE_SEL_NARROW INPUT_%s\"",
				    The_GetSuffix ());
		     HTM_OPTION (HTM_Type_STRING,"0",Dpt->InsCod == 0,false,
				 "%s",Txt_Another_institution);
		     for (NumIns = 0;
			  NumIns < Gbl.Hierarchy.Inss.Num;
			  NumIns++)
			HTM_OPTION (HTM_Type_LONG,&Gbl.Hierarchy.Inss.Lst[NumIns].InsCod,
				    Gbl.Hierarchy.Inss.Lst[NumIns].InsCod == Dpt->InsCod,false,
				    "%s",Gbl.Hierarchy.Inss.Lst[NumIns].ShrtName);
		  HTM_SELECT_End ();
	       Frm_EndForm ();
	    HTM_TD_End ();

	    /* Department short name */
	    HTM_TD_Begin ("class=\"CM\"");
	       Frm_BeginForm (ActRenDptSho);
		  Dpt_PutParamDptCod (&Dpt->DptCod);
		  HTM_INPUT_TEXT ("ShortName",Cns_HIERARCHY_MAX_CHARS_SHRT_NAME,Dpt->ShrtName,
				  HTM_SUBMIT_ON_CHANGE,
				  "class=\"INPUT_SHORT_NAME INPUT_%s\"",
				  The_GetSuffix ());
	       Frm_EndForm ();
	    HTM_TD_End ();

	    /* Department full name */
	    HTM_TD_Begin ("class=\"CM\"");
	       Frm_BeginForm (ActRenDptFul);
		  Dpt_PutParamDptCod (&Dpt->DptCod);
		  HTM_INPUT_TEXT ("FullName",Cns_HIERARCHY_MAX_CHARS_FULL_NAME,Dpt->FullName,
				  HTM_SUBMIT_ON_CHANGE,
				  "class=\"INPUT_FULL_NAME INPUT_%s\"",
				  The_GetSuffix ());
	       Frm_EndForm ();
	    HTM_TD_End ();

	    /* Department WWW */
	    HTM_TD_Begin ("class=\"CM\"");
	       Frm_BeginForm (ActChgDptWWW);
		  Dpt_PutParamDptCod (&Dpt->DptCod);
		  HTM_INPUT_URL ("WWW",Dpt->WWW,HTM_SUBMIT_ON_CHANGE,
				 "class=\"INPUT_WWW_NARROW INPUT_%s\""
				 " required=\"required\"",
				 The_GetSuffix ());
	       Frm_EndForm ();
	    HTM_TD_End ();

	    /* Number of teachers */
	    HTM_TD_Begin ("class=\"RM DAT_%s\"",The_GetSuffix ());
	       HTM_Unsigned (Dpt->NumTchs);
	    HTM_TD_End ();

	 HTM_TR_End ();
	}

   /***** End table *****/
   HTM_TABLE_End ();
  }

/*****************************************************************************/
/****************** Write parameter with code of department ******************/
/*****************************************************************************/

static void Dpt_PutParamDptCod (void *DptCod)
  {
   extern const char *Par_CodeStr[];

   if (DptCod)
      Par_PutParLong (NULL,Par_CodeStr[Par_DptCod],*((long *) DptCod));
  }

/*****************************************************************************/
/***************************** Remove a department ***************************/
/*****************************************************************************/

void Dpt_RemoveDepartment (void)
  {
   extern const char *Txt_To_remove_a_department_you_must_first_remove_all_teachers_in_the_department;
   extern const char *Txt_Department_X_removed;

   /***** Department constructor *****/
   Dpt_EditingDepartmentConstructor ();

   /***** Get department code *****/
   Dpt_EditingDpt->DptCod = Par_GetAndCheckParCode (Par_DptCod);

   /***** Get data of the department from database *****/
   Dpt_GetDataOfDepartmentByCod (Dpt_EditingDpt);

   /***** Check if this department has teachers *****/
   if (Dpt_EditingDpt->NumTchs)	// Department has teachers ==> don't remove
      Ale_CreateAlert (Ale_WARNING,NULL,
	               Txt_To_remove_a_department_you_must_first_remove_all_teachers_in_the_department);
   else	// Department has no teachers ==> remove it
     {
      /***** Remove department *****/
      Dpt_DB_RemoveDepartment (Dpt_EditingDpt->DptCod);

      /***** Write message to show the change made *****/
      Ale_CreateAlert (Ale_SUCCESS,NULL,
	               Txt_Department_X_removed,
		       Dpt_EditingDpt->FullName);
     }
  }

/*****************************************************************************/
/****************** Change the institution of a department *******************/
/*****************************************************************************/

void Dpt_ChangeDepartIns (void)
  {
   extern const char *Txt_The_institution_of_the_department_has_changed;
   long NewInsCod;

   /***** Department constructor *****/
   Dpt_EditingDepartmentConstructor ();

   /***** Get parameters from form *****/
   /* Get the code of the department */
   Dpt_EditingDpt->DptCod = Par_GetAndCheckParCode (Par_DptCod);

   /* Get parameter with institution code */
   NewInsCod = Par_GetAndCheckParCode (Par_OthInsCod);

   /***** Get data of the department from database *****/
   Dpt_GetDataOfDepartmentByCod (Dpt_EditingDpt);

   /***** Update institution in table of departments *****/
   Dpt_DB_UpdateDptIns (Dpt_EditingDpt->DptCod,NewInsCod);

   /***** Write message to show the change made *****/
   Ale_CreateAlert (Ale_SUCCESS,NULL,
	            Txt_The_institution_of_the_department_has_changed);
  }

/*****************************************************************************/
/******************* Change the short name of a department *******************/
/*****************************************************************************/

void Dpt_RenameDepartShort (void)
  {
   /***** Department constructor *****/
   Dpt_EditingDepartmentConstructor ();

   /***** Rename department *****/
   Dpt_RenameDepartment (Cns_SHRT_NAME);
  }

/*****************************************************************************/
/******************* Change the full name of a department ********************/
/*****************************************************************************/

void Dpt_RenameDepartFull (void)
  {
   /***** Department constructor *****/
   Dpt_EditingDepartmentConstructor ();

   /***** Rename department *****/
   Dpt_RenameDepartment (Cns_FULL_NAME);
  }

/*****************************************************************************/
/************************ Change the name of a degree ************************/
/*****************************************************************************/

static void Dpt_RenameDepartment (Cns_ShrtOrFullName_t ShrtOrFullName)
  {
   extern const char *Txt_The_department_X_already_exists;
   extern const char *Txt_The_department_X_has_been_renamed_as_Y;
   extern const char *Txt_The_name_X_has_not_changed;
   const char *ParamName = NULL;	// Initialized to avoid warning
   const char *FieldName = NULL;	// Initialized to avoid warning
   size_t MaxBytes = 0;			// Initialized to avoid warning
   char *CurrentDptName = NULL;		// Initialized to avoid warning
   char NewDptName[Cns_HIERARCHY_MAX_BYTES_FULL_NAME + 1];

   switch (ShrtOrFullName)
     {
      case Cns_SHRT_NAME:
         ParamName = "ShortName";
         FieldName = "ShortName";
         MaxBytes = Cns_HIERARCHY_MAX_BYTES_SHRT_NAME;
         CurrentDptName = Dpt_EditingDpt->ShrtName;
         break;
      case Cns_FULL_NAME:
         ParamName = "FullName";
         FieldName = "FullName";
         MaxBytes = Cns_HIERARCHY_MAX_BYTES_FULL_NAME;
         CurrentDptName = Dpt_EditingDpt->FullName;
         break;
     }

   /***** Get parameters from form *****/
   /* Get the code of the department */
   Dpt_EditingDpt->DptCod = Par_GetAndCheckParCode (Par_DptCod);

   /* Get the new name for the department */
   Par_GetParText (ParamName,NewDptName,MaxBytes);

   /***** Get from the database the old names of the department *****/
   Dpt_GetDataOfDepartmentByCod (Dpt_EditingDpt);

   /***** Check if new name is empty *****/
   if (NewDptName[0])
     {
      /***** Check if old and new names are the same
             (this happens when return is pressed without changes) *****/
      if (strcmp (CurrentDptName,NewDptName))	// Different names
        {
         /***** If degree was in database... *****/
         if (Dpt_DB_CheckIfDepartmentNameExists (ParamName,NewDptName,Dpt_EditingDpt->DptCod))
            Ale_CreateAlert (Ale_WARNING,NULL,
        	             Txt_The_department_X_already_exists,
                             NewDptName);
         else
           {
            /* Update the table changing old name by new name */
            Dpt_DB_UpdateDptName (Dpt_EditingDpt->DptCod,FieldName,NewDptName);

            /* Write message to show the change made */
            Ale_CreateAlert (Ale_SUCCESS,NULL,
        	             Txt_The_department_X_has_been_renamed_as_Y,
                             CurrentDptName,NewDptName);
           }
        }
      else	// The same name
         Ale_CreateAlert (Ale_INFO,NULL,
                          Txt_The_name_X_has_not_changed,CurrentDptName);
     }
   else
      Ale_CreateAlertYouCanNotLeaveFieldEmpty ();

   /***** Update name *****/
   Str_Copy (CurrentDptName,NewDptName,MaxBytes);
  }

/******************************************************************************/
/*********************** Change the URL of a department *********************/
/*****************************************************************************/

void Dpt_ChangeDptWWW (void)
  {
   extern const char *Txt_The_new_web_address_is_X;
   char NewWWW[Cns_MAX_BYTES_WWW + 1];

   /***** Department constructor *****/
   Dpt_EditingDepartmentConstructor ();

   /***** Get parameters from form *****/
   /* Get the code of the department */
   Dpt_EditingDpt->DptCod = Par_GetAndCheckParCode (Par_DptCod);

   /* Get the new WWW for the department */
   Par_GetParText ("WWW",NewWWW,Cns_MAX_BYTES_WWW);

   /***** Get data of the department from database *****/
   Dpt_GetDataOfDepartmentByCod (Dpt_EditingDpt);

   /***** Check if new WWW is empty *****/
   if (NewWWW[0])
     {
      /* Update the table changing old WWW by new WWW */
      Dpt_DB_UpdateDptWWW (Dpt_EditingDpt->DptCod,NewWWW);

      /***** Write message to show the change made *****/
      Ale_CreateAlert (Ale_SUCCESS,NULL,
	               Txt_The_new_web_address_is_X,
                       NewWWW);
     }
   else
      Ale_CreateAlertYouCanNotLeaveFieldEmpty ();

   /***** Update web *****/
   Str_Copy (Dpt_EditingDpt->WWW,NewWWW,sizeof (Dpt_EditingDpt->WWW) - 1);
  }

/*****************************************************************************/
/******* Show alerts after changing a department and continue editing ********/
/*****************************************************************************/

void Dpt_ContEditAfterChgDpt (void)
  {
   /***** Write message to show the change made *****/
   Ale_ShowAlerts (NULL);

   /***** Show the form again *****/
   Dpt_EditDepartmentsInternal ();

   /***** Department destructor *****/
   Dpt_EditingDepartmentDestructor ();
  }

/*****************************************************************************/
/******************* Put a form to create a new department *******************/
/*****************************************************************************/

static void Dpt_PutFormToCreateDepartment (void)
  {
   extern const char *Txt_New_department;
   extern const char *Txt_Institution;
   extern const char *Txt_Short_name;
   extern const char *Txt_Full_name;
   extern const char *Txt_WWW;
   extern const char *Txt_Another_institution;
   extern const char *Txt_Create_department;
   unsigned NumIns;

   /***** Begin form *****/
   Frm_BeginForm (ActNewDpt);

      /***** Begin box and table *****/
      Box_BoxTableBegin (NULL,Txt_New_department,
			 NULL,NULL,
			 NULL,Box_NOT_CLOSABLE,2);

	 /***** Write heading *****/
	 HTM_TR_Begin (NULL);
            HTM_TH (Txt_Institution,HTM_HEAD_LEFT );
            HTM_TH (Txt_Short_name ,HTM_HEAD_LEFT );
            HTM_TH (Txt_Full_name  ,HTM_HEAD_LEFT );
            HTM_TH (Txt_WWW        ,HTM_HEAD_LEFT );
	 HTM_TR_End ();

	 HTM_TR_Begin (NULL);

	    /***** Institution *****/
	    HTM_TD_Begin ("class=\"CM\"");
	       HTM_SELECT_Begin (HTM_DONT_SUBMIT_ON_CHANGE,
				 "name=\"OthInsCod\""
				 " class=\"HIE_SEL_NARROW INPUT_%s\"",
				 The_GetSuffix ());
		  HTM_OPTION (HTM_Type_STRING,"0",Dpt_EditingDpt->InsCod == 0,false,
			      "%s",Txt_Another_institution);
		  for (NumIns = 0;
		       NumIns < Gbl.Hierarchy.Inss.Num;
		       NumIns++)
		     HTM_OPTION (HTM_Type_LONG,&Gbl.Hierarchy.Inss.Lst[NumIns].InsCod,
				 Gbl.Hierarchy.Inss.Lst[NumIns].InsCod == Dpt_EditingDpt->InsCod,false,
				 "%s",Gbl.Hierarchy.Inss.Lst[NumIns].ShrtName);
	       HTM_SELECT_End ();
	    HTM_TD_End ();

	    /***** Department short name *****/
	    HTM_TD_Begin ("class=\"CM\"");
	       HTM_INPUT_TEXT ("ShortName",Cns_HIERARCHY_MAX_CHARS_SHRT_NAME,Dpt_EditingDpt->ShrtName,
			       HTM_DONT_SUBMIT_ON_CHANGE,
			       "class=\"INPUT_SHORT_NAME INPUT_%s\""
			       " required=\"required\"",
			       The_GetSuffix ());
	    HTM_TD_End ();

	    /***** Department full name *****/
	    HTM_TD_Begin ("class=\"CM\"");
	       HTM_INPUT_TEXT ("FullName",Cns_HIERARCHY_MAX_CHARS_FULL_NAME,Dpt_EditingDpt->FullName,
			       HTM_DONT_SUBMIT_ON_CHANGE,
			       "class=\"INPUT_FULL_NAME INPUT_%s\""
			       " required=\"required\"",
			       The_GetSuffix ());
	    HTM_TD_End ();

	    /***** Department WWW *****/
	    HTM_TD_Begin ("class=\"CM\"");
	       HTM_INPUT_URL ("WWW",Dpt_EditingDpt->WWW,HTM_DONT_SUBMIT_ON_CHANGE,
			      "class=\"INPUT_WWW_NARROW INPUT_%s\""
			      " required=\"required\"",
			      The_GetSuffix ());
	    HTM_TD_End ();

	 HTM_TR_End ();

      /***** End table, send button and end box *****/
      Box_BoxTableWithButtonEnd (Btn_CREATE_BUTTON,Txt_Create_department);

   /***** End form *****/
   Frm_EndForm ();
  }

/*****************************************************************************/
/******************** Write header with fields of a degree *******************/
/*****************************************************************************/

static void Dpt_PutHeadDepartments (void)
  {
   extern const char *Txt_Code;
   extern const char *Txt_Institution;
   extern const char *Txt_Short_name;
   extern const char *Txt_Full_name;
   extern const char *Txt_WWW;
   extern const char *Txt_ROLES_PLURAL_BRIEF_Abc[Rol_NUM_ROLES];

   HTM_TR_Begin (NULL);
      HTM_TH_Empty (1);
      HTM_TH (Txt_Code                           ,HTM_HEAD_RIGHT);
      HTM_TH (Txt_Institution                    ,HTM_HEAD_LEFT );
      HTM_TH (Txt_Short_name                     ,HTM_HEAD_LEFT );
      HTM_TH (Txt_Full_name                      ,HTM_HEAD_LEFT );
      HTM_TH (Txt_WWW                            ,HTM_HEAD_LEFT );
      HTM_TH (Txt_ROLES_PLURAL_BRIEF_Abc[Rol_TCH],HTM_HEAD_RIGHT);
   HTM_TR_End ();
  }

/*****************************************************************************/
/***************** Receive form to create a new department *******************/
/*****************************************************************************/

void Dpt_ReceiveFormNewDpt (void)
  {
   extern const char *Txt_The_department_X_already_exists;
   extern const char *Txt_Created_new_department_X;

   /***** Department constructor *****/
   Dpt_EditingDepartmentConstructor ();

   /***** Get parameters from form *****/
   /* Get institution */
   Dpt_EditingDpt->InsCod = Par_GetAndCheckParCode (Par_OthInsCod);

   /* Get department short name */
   Par_GetParText ("ShortName",Dpt_EditingDpt->ShrtName,Cns_HIERARCHY_MAX_BYTES_SHRT_NAME);

   /* Get department full name */
   Par_GetParText ("FullName",Dpt_EditingDpt->FullName,Cns_HIERARCHY_MAX_BYTES_FULL_NAME);

   /* Get department WWW */
   Par_GetParText ("WWW",Dpt_EditingDpt->WWW,Cns_MAX_BYTES_WWW);

   if (Dpt_EditingDpt->ShrtName[0] &&
       Dpt_EditingDpt->FullName[0])	// If there's a department name
     {
      if (Dpt_EditingDpt->WWW[0])
        {
         /***** If name of department was in database... *****/
         if (Dpt_DB_CheckIfDepartmentNameExists ("ShortName",Dpt_EditingDpt->ShrtName,-1L))
            Ale_CreateAlert (Ale_WARNING,NULL,
        	             Txt_The_department_X_already_exists,
                             Dpt_EditingDpt->ShrtName);
         else if (Dpt_DB_CheckIfDepartmentNameExists ("FullName",Dpt_EditingDpt->FullName,-1L))
            Ale_CreateAlert (Ale_WARNING,NULL,
        	             Txt_The_department_X_already_exists,
                             Dpt_EditingDpt->FullName);
         else	// Add new department to database
           {
            Dpt_DB_CreateDepartment (Dpt_EditingDpt);
	    Ale_CreateAlert (Ale_SUCCESS,NULL,
		             Txt_Created_new_department_X,
			     Dpt_EditingDpt->FullName);
           }
        }
      else	// If there is not a web
         Ale_ShowAlertYouMustSpecifyTheWebAddress ();
     }
   else	// If there is not a department name
      Ale_ShowAlertYouMustSpecifyTheShortNameAndTheFullName ();
  }

/*****************************************************************************/
/************************** Get number of departments ************************/
/*****************************************************************************/

unsigned Dpt_GetTotalNumberOfDepartments (void)
  {
   /***** Get number of departments from database *****/
   return (unsigned) DB_GetNumRowsTable ("dpt_departments");
  }

/*****************************************************************************/
/**************** Get number of departments in a institution *****************/
/*****************************************************************************/

void Dpt_FlushCacheNumDptsInIns (void)
  {
   Gbl.Cache.NumDptsInIns.InsCod  = -1L;
   Gbl.Cache.NumDptsInIns.NumDpts = 0;
  }

unsigned Dpt_GetNumDptsInIns (long InsCod)
  {
   /***** 1. Fast check: Trivial case *****/
   if (InsCod <= 0)
      return 0;

   /***** 2. Fast check: If cached... *****/
   if (InsCod == Gbl.Cache.NumDptsInIns.InsCod)
      return Gbl.Cache.NumDptsInIns.NumDpts;

   /***** 3. Slow: number of departments of an institution from database *****/
   Gbl.Cache.NumDptsInIns.InsCod  = InsCod;
   Gbl.Cache.NumDptsInIns.NumDpts = Dpt_DB_GetNumDepartmentsInInstitution (InsCod);
   return Gbl.Cache.NumDptsInIns.NumDpts;
  }

/*****************************************************************************/
/*********************** Put selector for department *************************/
/*****************************************************************************/

void Dpt_WriteSelectorDepartment (long InsCod,long DptCod,
                                  const char *ParamName,
		                  const char *SelectClass,
                                  long FirstOption,
                                  const char *TextWhenNoDptSelected,
                                  bool SubmitFormOnChange)
  {
   extern const char *Txt_Another_department;
   struct Dpt_Departments Departments;
   unsigned NumDpt;
   bool NoDptSelectable;

   /***** Reset departments context *****/
   Dpt_ResetDepartments (&Departments);

   /***** Get list of departments *****/
   Dpt_GetListDepartments (&Departments,InsCod);

   /***** Selector to select department *****/
   /* Begin selector */
   HTM_SELECT_Begin (SubmitFormOnChange,
		     "id=\"%s\" name=\"%s\" class=\"%s\"",
		     ParamName,ParamName,SelectClass);

      if (FirstOption <= 0)
	{
	 /* Option when no department selected */
	 if (FirstOption < 0)
	   {
	    NoDptSelectable = false;
	    if (TextWhenNoDptSelected)
	       if (TextWhenNoDptSelected[0])
		  NoDptSelectable = true;

	    HTM_OPTION (HTM_Type_STRING,"-1",DptCod < 0,!NoDptSelectable,
			"%s",TextWhenNoDptSelected);
	   }

	 /* Another department selected (different to all departments listed) */
	 HTM_OPTION (HTM_Type_STRING,"0",DptCod == 0,false,
		     "%s",Txt_Another_department);
	}

      /* List all departments */
      for (NumDpt = 0;
	   NumDpt < Departments.Num;
	   NumDpt++)
	 HTM_OPTION (HTM_Type_LONG,&Departments.Lst[NumDpt].DptCod,
		     Departments.Lst[NumDpt].DptCod == DptCod,false,
		     "%s",Departments.Lst[NumDpt].FullName);

   /* End selector */
   HTM_SELECT_End ();

   /***** Free list of departments *****/
   Dpt_FreeListDepartments (&Departments);
  }

/*****************************************************************************/
/********************** Department constructor/destructor ********************/
/*****************************************************************************/

static void Dpt_EditingDepartmentConstructor (void)
  {
   /***** Pointer must be NULL *****/
   if (Dpt_EditingDpt != NULL)
      Err_WrongDepartmentExit ();

   /***** Allocate memory for department *****/
   if ((Dpt_EditingDpt = malloc (sizeof (*Dpt_EditingDpt))) == NULL)
      Err_NotEnoughMemoryExit ();

   /***** Reset department *****/
   Dpt_EditingDpt->DptCod      = -1L;
   Dpt_EditingDpt->InsCod      = -1L;
   Dpt_EditingDpt->ShrtName[0] = '\0';
   Dpt_EditingDpt->FullName[0] = '\0';
   Dpt_EditingDpt->WWW[0]      = '\0';
   Dpt_EditingDpt->NumTchs     = 0;
  }

static void Dpt_EditingDepartmentDestructor (void)
  {
   /***** Free memory used for department *****/
   if (Dpt_EditingDpt != NULL)
     {
      free (Dpt_EditingDpt);
      Dpt_EditingDpt = NULL;
     }
  }
