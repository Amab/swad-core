// swad_info.c: info about course

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

#include <limits.h>		// For maximum values
#include <linux/limits.h>	// For PATH_MAX, NAME_MAX
#include <stddef.h>		// For NULL
#include <stdlib.h>		// For getenv, etc
#include <stdsoap2.h>		// For SOAP_OK and soap functions
#include <string.h>		// For string functions
#include <unistd.h>		// For unlink

#include "swad_action.h"
#include "swad_autolink.h"
#include "swad_box.h"
#include "swad_database.h"
#include "swad_error.h"
#include "swad_form.h"
#include "swad_global.h"
#include "swad_HTML.h"
#include "swad_info.h"
#include "swad_info_database.h"
#include "swad_parameter.h"
#include "swad_string.h"

/*****************************************************************************/
/************** External global variables from others modules ****************/
/*****************************************************************************/

extern struct Globals Gbl;

/*****************************************************************************/
/****************************** Public constants *****************************/
/*****************************************************************************/

const Act_Action_t Inf_ActionsSeeInfo[Inf_NUM_TYPES] =
  {
   [Inf_INTRODUCTION  ] = ActSeeCrsInf,
   [Inf_TEACHING_GUIDE] = ActSeeTchGui,
   [Inf_LECTURES      ] = ActSeeSylLec,
   [Inf_PRACTICALS    ] = ActSeeSylPra,
   [Inf_BIBLIOGRAPHY  ] = ActSeeBib,
   [Inf_FAQ           ] = ActSeeFAQ,
   [Inf_LINKS         ] = ActSeeCrsLnk,
   [Inf_ASSESSMENT    ] = ActSeeAss,
  };

/*****************************************************************************/
/***************************** Private constants *****************************/
/*****************************************************************************/

static const char *Inf_FileNamesForInfoType[Inf_NUM_TYPES] =
  {
   [Inf_INTRODUCTION  ] = Cfg_CRS_INFO_INTRODUCTION,
   [Inf_TEACHING_GUIDE] = Cfg_CRS_INFO_TEACHING_GUIDE,
   [Inf_LECTURES      ] = Cfg_CRS_INFO_LECTURES,
   [Inf_PRACTICALS    ] = Cfg_CRS_INFO_PRACTICALS,
   [Inf_BIBLIOGRAPHY  ] = Cfg_CRS_INFO_BIBLIOGRAPHY,
   [Inf_FAQ           ] = Cfg_CRS_INFO_FAQ,
   [Inf_LINKS         ] = Cfg_CRS_INFO_LINKS,
   [Inf_ASSESSMENT    ] = Cfg_CRS_INFO_ASSESSMENT,
  };

/* Functions to write forms in course edition (FAQ, links, etc.) */
static void (*Inf_FormsForEditionTypes[Inf_NUM_SOURCES])(Inf_Src_t InfoSrc) =
  {
   [Inf_NONE      ] = NULL,
   [Inf_EDITOR    ] = Inf_FormToEnterIntegratedEditor,
   [Inf_PLAIN_TEXT] = Inf_FormToEnterPlainTextEditor,
   [Inf_RICH_TEXT ] = Inf_FormToEnterRichTextEditor,
   [Inf_PAGE      ] = Inf_FormToSendPage,
   [Inf_URL       ] = Inf_FormToSendURL,
  };

static const Act_Action_t Inf_ActionsEditInfo[Inf_NUM_TYPES] =
  {
   [Inf_INTRODUCTION  ] = ActEdiCrsInf,
   [Inf_TEACHING_GUIDE] = ActEdiTchGui,
   [Inf_LECTURES      ] = ActEdiSylLec,
   [Inf_PRACTICALS    ] = ActEdiSylPra,
   [Inf_BIBLIOGRAPHY  ] = ActEdiBib,
   [Inf_FAQ           ] = ActEdiFAQ,
   [Inf_LINKS         ] = ActEdiCrsLnk,
   [Inf_ASSESSMENT    ] = ActEdiAss,
  };

static const Act_Action_t Inf_ActionsChangeForceReadInfo[Inf_NUM_TYPES] =
  {
   [Inf_INTRODUCTION  ] = ActChgFrcReaCrsInf,
   [Inf_TEACHING_GUIDE] = ActChgFrcReaTchGui,
   [Inf_LECTURES      ] = ActChgFrcReaSylLec,
   [Inf_PRACTICALS    ] = ActChgFrcReaSylPra,
   [Inf_BIBLIOGRAPHY  ] = ActChgFrcReaBib,
   [Inf_FAQ           ] = ActChgFrcReaFAQ,
   [Inf_LINKS         ] = ActChgFrcReaCrsLnk,
   [Inf_ASSESSMENT    ] = ActChgFrcReaAss,
  };

static const Act_Action_t Inf_ActionsIHaveReadInfo[Inf_NUM_TYPES] =
  {
   [Inf_INTRODUCTION  ] = ActChgHavReaCrsInf,
   [Inf_TEACHING_GUIDE] = ActChgHavReaTchGui,
   [Inf_LECTURES      ] = ActChgHavReaSylLec,
   [Inf_PRACTICALS    ] = ActChgHavReaSylPra,
   [Inf_BIBLIOGRAPHY  ] = ActChgHavReaBib,
   [Inf_FAQ           ] = ActChgHavReaFAQ,
   [Inf_LINKS         ] = ActChgHavReaCrsLnk,
   [Inf_ASSESSMENT    ] = ActChgHavReaAss,
  };

static const Act_Action_t Inf_ActionsSelecInfoSrc[Inf_NUM_TYPES] =
  {
   [Inf_INTRODUCTION  ] = ActSelInfSrcCrsInf,
   [Inf_TEACHING_GUIDE] = ActSelInfSrcTchGui,
   [Inf_LECTURES      ] = ActSelInfSrcSylLec,
   [Inf_PRACTICALS    ] = ActSelInfSrcSylPra,
   [Inf_BIBLIOGRAPHY  ] = ActSelInfSrcBib,
   [Inf_FAQ           ] = ActSelInfSrcFAQ,
   [Inf_LINKS         ] = ActSelInfSrcCrsLnk,
   [Inf_ASSESSMENT    ] = ActSelInfSrcAss,
  };

static const Act_Action_t Inf_ActionsInfo[Inf_NUM_SOURCES][Inf_NUM_TYPES] =
  {
   [Inf_NONE      ][Inf_INTRODUCTION  ] = ActUnk,
   [Inf_NONE      ][Inf_TEACHING_GUIDE] = ActUnk,
   [Inf_NONE      ][Inf_LECTURES      ] = ActUnk,
   [Inf_NONE      ][Inf_PRACTICALS    ] = ActUnk,
   [Inf_NONE      ][Inf_BIBLIOGRAPHY  ] = ActUnk,
   [Inf_NONE      ][Inf_FAQ           ] = ActUnk,
   [Inf_NONE      ][Inf_LINKS         ] = ActUnk,
   [Inf_NONE      ][Inf_ASSESSMENT    ] = ActUnk,

   [Inf_EDITOR    ][Inf_INTRODUCTION  ] = ActEditorCrsInf,
   [Inf_EDITOR    ][Inf_TEACHING_GUIDE] = ActEditorTchGui,
   [Inf_EDITOR    ][Inf_LECTURES      ] = ActEditorSylLec,
   [Inf_EDITOR    ][Inf_PRACTICALS    ] = ActEditorSylPra,
   [Inf_EDITOR    ][Inf_BIBLIOGRAPHY  ] = ActEditorBib,
   [Inf_EDITOR    ][Inf_FAQ           ] = ActEditorFAQ,
   [Inf_EDITOR    ][Inf_LINKS         ] = ActEditorCrsLnk,
   [Inf_EDITOR    ][Inf_ASSESSMENT    ] = ActEditorAss,

   [Inf_PLAIN_TEXT][Inf_INTRODUCTION  ] = ActPlaTxtEdiCrsInf,
   [Inf_PLAIN_TEXT][Inf_TEACHING_GUIDE] = ActPlaTxtEdiTchGui,
   [Inf_PLAIN_TEXT][Inf_LECTURES      ] = ActPlaTxtEdiSylLec,
   [Inf_PLAIN_TEXT][Inf_PRACTICALS    ] = ActPlaTxtEdiSylPra,
   [Inf_PLAIN_TEXT][Inf_BIBLIOGRAPHY  ] = ActPlaTxtEdiBib,
   [Inf_PLAIN_TEXT][Inf_FAQ           ] = ActPlaTxtEdiFAQ,
   [Inf_PLAIN_TEXT][Inf_LINKS         ] = ActPlaTxtEdiCrsLnk,
   [Inf_PLAIN_TEXT][Inf_ASSESSMENT    ] = ActPlaTxtEdiAss,

   [Inf_RICH_TEXT ][Inf_INTRODUCTION  ] = ActRchTxtEdiCrsInf,
   [Inf_RICH_TEXT ][Inf_TEACHING_GUIDE] = ActRchTxtEdiTchGui,
   [Inf_RICH_TEXT ][Inf_LECTURES      ] = ActRchTxtEdiSylLec,
   [Inf_RICH_TEXT ][Inf_PRACTICALS    ] = ActRchTxtEdiSylPra,
   [Inf_RICH_TEXT ][Inf_BIBLIOGRAPHY  ] = ActRchTxtEdiBib,
   [Inf_RICH_TEXT ][Inf_FAQ           ] = ActRchTxtEdiFAQ,
   [Inf_RICH_TEXT ][Inf_LINKS         ] = ActRchTxtEdiCrsLnk,
   [Inf_RICH_TEXT ][Inf_ASSESSMENT    ] = ActRchTxtEdiAss,

   [Inf_PAGE      ][Inf_INTRODUCTION  ] = ActRcvPagCrsInf,
   [Inf_PAGE      ][Inf_TEACHING_GUIDE] = ActRcvPagTchGui,
   [Inf_PAGE      ][Inf_LECTURES      ] = ActRcvPagSylLec,
   [Inf_PAGE      ][Inf_PRACTICALS    ] = ActRcvPagSylPra,
   [Inf_PAGE      ][Inf_BIBLIOGRAPHY  ] = ActRcvPagBib,
   [Inf_PAGE      ][Inf_FAQ           ] = ActRcvPagFAQ,
   [Inf_PAGE      ][Inf_LINKS         ] = ActRcvPagCrsLnk,
   [Inf_PAGE      ][Inf_ASSESSMENT    ] = ActRcvPagAss,

   [Inf_URL       ][Inf_INTRODUCTION  ] = ActRcvURLCrsInf,
   [Inf_URL       ][Inf_TEACHING_GUIDE] = ActRcvURLTchGui,
   [Inf_URL       ][Inf_LECTURES      ] = ActRcvURLSylLec,
   [Inf_URL       ][Inf_PRACTICALS    ] = ActRcvURLSylPra,
   [Inf_URL       ][Inf_BIBLIOGRAPHY  ] = ActRcvURLBib,
   [Inf_URL       ][Inf_FAQ           ] = ActRcvURLFAQ,
   [Inf_URL       ][Inf_LINKS         ] = ActRcvURLCrsLnk,
   [Inf_URL       ][Inf_ASSESSMENT    ] = ActRcvURLAss,
  };

static const Act_Action_t Inf_ActionsRcvPlaTxtInfo[Inf_NUM_TYPES] =
  {
   [Inf_INTRODUCTION  ] = ActRcvPlaTxtCrsInf,
   [Inf_TEACHING_GUIDE] = ActRcvPlaTxtTchGui,
   [Inf_LECTURES      ] = ActRcvPlaTxtSylLec,
   [Inf_PRACTICALS    ] = ActRcvPlaTxtSylPra,
   [Inf_BIBLIOGRAPHY  ] = ActRcvPlaTxtBib,
   [Inf_FAQ           ] = ActRcvPlaTxtFAQ,
   [Inf_LINKS         ] = ActRcvPlaTxtCrsLnk,
   [Inf_ASSESSMENT    ] = ActRcvPlaTxtAss,
  };

static const Act_Action_t Inf_ActionsRcvRchTxtInfo[Inf_NUM_TYPES] =
  {
   [Inf_INTRODUCTION  ] = ActRcvRchTxtCrsInf,
   [Inf_TEACHING_GUIDE] = ActRcvRchTxtTchGui,
   [Inf_LECTURES      ] = ActRcvRchTxtSylLec,
   [Inf_PRACTICALS    ] = ActRcvRchTxtSylPra,
   [Inf_BIBLIOGRAPHY  ] = ActRcvRchTxtBib,
   [Inf_FAQ           ] = ActRcvRchTxtFAQ,
   [Inf_LINKS         ] = ActRcvRchTxtCrsLnk,
   [Inf_ASSESSMENT    ] = ActRcvRchTxtAss,
  };

/***** Help *****/
extern const char *Hlp_COURSE_Information_textual_information;
extern const char *Hlp_COURSE_Guide;
extern const char *Hlp_COURSE_Syllabus;
extern const char *Hlp_COURSE_Bibliography;
extern const char *Hlp_COURSE_FAQ;
extern const char *Hlp_COURSE_Links;
extern const char *Hlp_ASSESSMENT_System;

extern const char *Hlp_COURSE_Information_edit;
extern const char *Hlp_COURSE_Guide_edit;
extern const char *Hlp_COURSE_Syllabus_edit;
extern const char *Hlp_COURSE_Bibliography_edit;
extern const char *Hlp_COURSE_FAQ_edit;
extern const char *Hlp_COURSE_Links_edit;
extern const char *Hlp_ASSESSMENT_System_edit;

/*****************************************************************************/
/**************************** Private prototypes *****************************/
/*****************************************************************************/

static void Inf_PutButtonToEditInfo (void);
static void Inf_PutIconToViewInfo (void *Type);
static void Inf_PutCheckboxForceStdsToReadInfo (bool MustBeRead,bool Disabled);
static void Inf_PutCheckboxConfirmIHaveReadInfo (void);
static bool Inf_GetMustBeReadFromForm (void);
static bool Inf_GetIfIHaveReadFromForm (void);

static bool Inf_CheckPage (long CrsCod,Inf_Type_t InfoType);
static bool Inf_CheckAndShowPage (void);

static bool Inf_CheckURL (long CrsCod,Inf_Type_t InfoType);
static bool Inf_CheckAndShowURL (void);
static void Inf_BuildPathURL (long CrsCod,Inf_Type_t InfoType,
                              char PathFile[PATH_MAX + 1]);

static void Inf_ShowPage (const char *URL);

static bool Inf_CheckIfInfoAvailable (struct Syl_Syllabus *Syllabus,
                                      Inf_Src_t InfoSrc);
static void Inf_AsignInfoType (struct Inf_Info *Info,
                               struct Syl_Syllabus *Syllabus);

static bool Inf_CheckPlainTxt (long CrsCod,Inf_Type_t InfoType);
static bool Inf_CheckAndShowPlainTxt (void);

static bool Inf_CheckRichTxt (long CrsCod,Inf_Type_t InfoType);
static bool Inf_CheckAndShowRichTxt (void);

/*****************************************************************************/
/******** Show course info (theory, practices, bibliography, etc.) ***********/
/*****************************************************************************/

void Inf_ShowInfo (void)
  {
   extern const char *Txt_INFO_TITLE[Inf_NUM_TYPES];
   extern const char *Txt_No_information;
   struct Syl_Syllabus Syllabus;
   struct Inf_FromDB FromDB;
   bool Disabled;
   bool ICanEdit = (Gbl.Usrs.Me.Role.Logged == Rol_TCH ||
                    Gbl.Usrs.Me.Role.Logged == Rol_SYS_ADM);
   bool ShowWarningNoInfo = false;
   const char *Help[Inf_NUM_TYPES] =
     {
      [Inf_INTRODUCTION  ] = Hlp_COURSE_Information_textual_information,
      [Inf_TEACHING_GUIDE] = Hlp_COURSE_Guide,
      [Inf_LECTURES      ] = Hlp_COURSE_Syllabus,
      [Inf_PRACTICALS    ] = Hlp_COURSE_Syllabus,
      [Inf_BIBLIOGRAPHY  ] = Hlp_COURSE_Bibliography,
      [Inf_FAQ           ] = Hlp_COURSE_FAQ,
      [Inf_LINKS         ] = Hlp_COURSE_Links,
      [Inf_ASSESSMENT    ] = Hlp_ASSESSMENT_System,
     };

   /***** Reset syllabus context *****/
   Syl_ResetSyllabus (&Syllabus);

   /***** Set info type *****/
   Inf_AsignInfoType (&Gbl.Crs.Info,&Syllabus);

   /***** Get info source from database *****/
   Inf_GetAndCheckInfoSrcFromDB (&Syllabus,
                                 Gbl.Hierarchy.Crs.CrsCod,
                                 Gbl.Crs.Info.Type,
                                 &FromDB);

   switch (Gbl.Crs.Info.Type)
     {
      case Inf_LECTURES:
      case Inf_PRACTICALS:
         Syl_PutFormWhichSyllabus (Syllabus.WhichSyllabus);
         break;
      default:
	 break;
     }

   switch (Gbl.Usrs.Me.Role.Logged)
     {
      case Rol_STD:
         if (FromDB.MustBeRead)
           {
            /***** Contextual menu *****/
            Mnu_ContextMenuBegin ();
	       Inf_PutCheckboxConfirmIHaveReadInfo ();	// Checkbox to confirm that...
							   // ...I have read this couse info
            Mnu_ContextMenuEnd ();
           }
         break;
      case Rol_NET:
      case Rol_TCH:
      case Rol_SYS_ADM:
         /* Put  */
         if (FromDB.Src != Inf_NONE)
           {
            /***** Contextual menu *****/
            Mnu_ContextMenuBegin ();
	       Disabled = (Gbl.Usrs.Me.Role.Logged == Rol_NET);	// Non-editing teachers can not change the status of checkbox
	       Inf_PutCheckboxForceStdsToReadInfo (FromDB.MustBeRead,Disabled);	// Checkbox to force students...
										// ...to read this couse info
            Mnu_ContextMenuEnd ();
           }
         break;
      default:
         break;
     }

   switch (FromDB.Src)
     {
      case Inf_NONE:
	 ShowWarningNoInfo = true;
         break;
      case Inf_EDITOR:
         switch (Gbl.Crs.Info.Type)
           {
            case Inf_LECTURES:
            case Inf_PRACTICALS:
               ShowWarningNoInfo = !Syl_CheckAndEditSyllabus (&Syllabus);
               break;
            case Inf_INTRODUCTION:
            case Inf_TEACHING_GUIDE:
            case Inf_BIBLIOGRAPHY:
            case Inf_FAQ:
            case Inf_LINKS:
            case Inf_ASSESSMENT:
               ShowWarningNoInfo = true;
	       break;
           }
         break;
      case Inf_PLAIN_TEXT:
         ShowWarningNoInfo = !Inf_CheckAndShowPlainTxt ();
         break;
      case Inf_RICH_TEXT:
         ShowWarningNoInfo = !Inf_CheckAndShowRichTxt ();
         break;
      case Inf_PAGE:
         /***** Open file with web page *****/
	 ShowWarningNoInfo = !Inf_CheckAndShowPage ();
         break;
      case Inf_URL:
         /***** Check if file with URL exists *****/
	 ShowWarningNoInfo = !Inf_CheckAndShowURL ();
         break;
     }

   if (ShowWarningNoInfo)
     {
      if (ICanEdit)
	 Box_BoxBegin ("100%",Txt_INFO_TITLE[Gbl.Crs.Info.Type],
		       Inf_PutIconToEditInfo,&Gbl.Crs.Info.Type,
		       Help[Gbl.Crs.Info.Type],Box_NOT_CLOSABLE);
      else
	 Box_BoxBegin ("100%",Txt_INFO_TITLE[Gbl.Crs.Info.Type],
		       NULL,NULL,
		       Help[Gbl.Crs.Info.Type],Box_NOT_CLOSABLE);
      Ale_ShowAlert (Ale_INFO,Txt_No_information);
      if (ICanEdit)
	 Inf_PutButtonToEditInfo ();
      Box_BoxEnd ();
     }
  }

/*****************************************************************************/
/*********************** Put button to edit course info **********************/
/*****************************************************************************/

static void Inf_PutButtonToEditInfo (void)
  {
   extern const char *Txt_Edit;

   Frm_BeginForm (Inf_ActionsEditInfo[Gbl.Crs.Info.Type]);
      Btn_PutConfirmButton (Txt_Edit);
   Frm_EndForm ();
  }

/*****************************************************************************/
/************************ Put icon to edit course info ***********************/
/*****************************************************************************/

static void Inf_PutIconToViewInfo (void *Type)
  {
   if (Type)
      Ico_PutContextualIconToView (Inf_ActionsSeeInfo[*((Inf_Type_t *) Type)],
				   NULL,NULL);
  }

void Inf_PutIconToEditInfo (void *Type)
  {
   if (Type)
      Ico_PutContextualIconToEdit (Inf_ActionsEditInfo[*((Inf_Type_t *) Type)],NULL,
				   NULL,NULL);
  }

/*****************************************************************************/
/********** Put a form (checkbox) to force students to read info *************/
/*****************************************************************************/

static void Inf_PutCheckboxForceStdsToReadInfo (bool MustBeRead,bool Disabled)
  {
   extern const char *Txt_Force_students_to_read_this_information;

   Lay_PutContextualCheckbox (Inf_ActionsChangeForceReadInfo[Gbl.Crs.Info.Type],
                              NULL,
                              "MustBeRead",
                              MustBeRead,Disabled,
                              Txt_Force_students_to_read_this_information,
                              Txt_Force_students_to_read_this_information);
  }

/*****************************************************************************/
/********** Put a form (checkbox) to force students to read info *************/
/*****************************************************************************/

static void Inf_PutCheckboxConfirmIHaveReadInfo (void)
  {
   extern const char *Txt_I_have_read_this_information;
   bool IHaveRead = Inf_DB_CheckIfIHaveReadInfo ();

   Lay_PutContextualCheckbox (Inf_ActionsIHaveReadInfo[Gbl.Crs.Info.Type],
                              NULL,
                              "IHaveRead",
                              IHaveRead,false,
                              Txt_I_have_read_this_information,
                              Txt_I_have_read_this_information);
  }

/*****************************************************************************/
/********* Get if students must read any info about current course ***********/
/*****************************************************************************/

bool Inf_GetIfIMustReadAnyCrsInfoInThisCrs (void)
  {
   MYSQL_RES *mysql_res;
   MYSQL_ROW row;
   unsigned NumInfos;
   unsigned NumInfo;
   Inf_Type_t InfoType;

   /***** Reset must-be-read to false for all info types *****/
   for (InfoType  = (Inf_Type_t) 0;
	InfoType <= (Inf_Type_t) (Inf_NUM_TYPES - 1);
	InfoType++)
      Gbl.Crs.Info.MustBeRead[InfoType] = false;

   /***** Get info types where students must read info *****/
   NumInfos = Inf_DB_GetInfoTypesfIMustReadInfo (&mysql_res);

   /***** Set must-be-read to true for each rown in result *****/
   for (NumInfo = 0;
	NumInfo < NumInfos;
	NumInfo++)
     {
      row = mysql_fetch_row (mysql_res);

      /* Get info type (row[0]) */
      InfoType = Inf_DB_ConvertFromStrDBToInfoType (row[0]);

      Gbl.Crs.Info.MustBeRead[InfoType] = true;
     }

   /***** Free structure that stores the query result *****/
   DB_FreeMySQLResult (&mysql_res);

   return (NumInfos != 0);
  }

/*****************************************************************************/
/***** Write message if students must read any info about current course *****/
/*****************************************************************************/

void Inf_WriteMsgYouMustReadInfo (void)
  {
   extern const char *The_ClassFormLinkInBox[The_NUM_THEMES];
   extern const char *Txt_Required_reading;
   extern const char *Txt_You_should_read_the_following_information;
   Inf_Type_t InfoType;

   /***** Begin box *****/
   Box_BoxBegin (NULL,Txt_Required_reading,
                 NULL,NULL,
                 NULL,Box_CLOSABLE);

      /***** Write message *****/
      Ale_ShowAlert (Ale_WARNING,Txt_You_should_read_the_following_information);

      /***** Write every information I must read *****/
      HTM_DIV_Begin ("class=\"CM\"");
	 HTM_UL_Begin ("class=\"LIST_I_MUST_READ\"");
	    for (InfoType  = (Inf_Type_t) 0;
		 InfoType <= (Inf_Type_t) (Inf_NUM_TYPES - 1);
		 InfoType++)
	       if (Gbl.Crs.Info.MustBeRead[InfoType])
		 {
		  HTM_LI_Begin (NULL);
		     Frm_BeginForm (Inf_ActionsSeeInfo[InfoType]);
			HTM_BUTTON_SUBMIT_Begin (Act_GetTitleAction (Inf_ActionsSeeInfo[InfoType]),
						 The_ClassFormLinkInBox[Gbl.Prefs.Theme],NULL);
			   HTM_Txt (Act_GetTitleAction (Inf_ActionsSeeInfo[InfoType]));
			HTM_BUTTON_End ();
		     Frm_EndForm ();
		  HTM_LI_End ();
		 }
	 HTM_UL_End ();
      HTM_DIV_End ();

   /***** End box *****/
   Box_BoxEnd ();
  }

/*****************************************************************************/
/****** Change teacher's preference about force students to read info ********/
/*****************************************************************************/

void Inf_ChangeForceReadInfo (void)
  {
   extern const char *Txt_Students_now_are_required_to_read_this_information;
   extern const char *Txt_Students_are_no_longer_obliged_to_read_this_information;
   struct Syl_Syllabus Syllabus;
   bool MustBeRead = Inf_GetMustBeReadFromForm ();

   /***** Reset syllabus context *****/
   Syl_ResetSyllabus (&Syllabus);

   /***** Set info type *****/
   Inf_AsignInfoType (&Gbl.Crs.Info,&Syllabus);

   /***** Set status (if info must be read or not) into database *****/
   Inf_DB_SetForceRead (MustBeRead);

   /***** Write message of success *****/
   Ale_ShowAlert (Ale_SUCCESS,
                  MustBeRead ? Txt_Students_now_are_required_to_read_this_information :
                               Txt_Students_are_no_longer_obliged_to_read_this_information);

   /***** Show the selected info *****/
   Inf_ShowInfo ();
  }

/*****************************************************************************/
/************** Change confirmation of I have read course info ***************/
/*****************************************************************************/

void Inf_ChangeIHaveReadInfo (void)
  {
   extern const char *Txt_You_have_confirmed_that_you_have_read_this_information;
   extern const char *Txt_You_have_eliminated_the_confirmation_that_you_have_read_this_information;
   struct Syl_Syllabus Syllabus;
   bool IHaveRead = Inf_GetIfIHaveReadFromForm ();

   /***** Reset syllabus context *****/
   Syl_ResetSyllabus (&Syllabus);

   /***** Set info type *****/
   Inf_AsignInfoType (&Gbl.Crs.Info,&Syllabus);

   /***** Set status (if I have read or not a information) into database *****/
   Inf_DB_SetIHaveRead (IHaveRead);

   /***** Write message of success *****/
   Ale_ShowAlert (Ale_SUCCESS,
                  IHaveRead ? Txt_You_have_confirmed_that_you_have_read_this_information :
                              Txt_You_have_eliminated_the_confirmation_that_you_have_read_this_information);

   /***** Show the selected info *****/
   Inf_ShowInfo ();
  }

/*****************************************************************************/
/************* Get if info must be read by students from form ****************/
/*****************************************************************************/

static bool Inf_GetMustBeReadFromForm (void)
  {
   return Par_GetParToBool ("MustBeRead");
  }

/*****************************************************************************/
/************* Get if info must be read by students from form ****************/
/*****************************************************************************/

static bool Inf_GetIfIHaveReadFromForm (void)
  {
   return Par_GetParToBool ("IHaveRead");
  }

/*****************************************************************************/
/************************** Check if exists a page ***************************/
/*****************************************************************************/
// Return true if info available

static bool Inf_CheckPage (long CrsCod,Inf_Type_t InfoType)
  {
   char PathRelDirHTML[PATH_MAX + 1];
   char PathRelFileHTML[PATH_MAX + 1 + 10 + 1];

   // TODO !!!!!!!!!!!! If the page is hosted in server ==> it should be created a temporary public directory
   //                                                       and host the page in a private directory !!!!!!!!!!!!!!!!!

   /***** Build path of directory containing web page *****/
   Inf_BuildPathPage (CrsCod,InfoType,PathRelDirHTML);

   /***** Open file with web page *****/
   /* 1. Check if index.html exists */
   snprintf (PathRelFileHTML,sizeof (PathRelFileHTML),"%s/index.html",
	     PathRelDirHTML);
   if (Fil_CheckIfPathExists (PathRelFileHTML))	// TODO: Check if not empty?
      return true;

   /* 2. If index.html does not exist, try index.htm */
   snprintf (PathRelFileHTML,sizeof (PathRelFileHTML),"%s/index.htm",
	     PathRelDirHTML);
   if (Fil_CheckIfPathExists (PathRelFileHTML))	// TODO: Check if not empty?
      return true;

   return false;
  }

/*****************************************************************************/
/**************** Check if exists and show link to a page ********************/
/*****************************************************************************/
// Return true if info available

static bool Inf_CheckAndShowPage (void)
  {
   char PathRelDirHTML[PATH_MAX + 1];
   char PathRelFileHTML[PATH_MAX + 1 + 10 + 1];
   char URL[PATH_MAX + 1];

   // TODO !!!!!!!!!!!! If the page is hosted in server ==> it should be created a temporary public directory
   //                                                       and host the page in a private directory !!!!!!!!!!!!!!!!!

   /***** Build path of directory containing web page *****/
   Inf_BuildPathPage (Gbl.Hierarchy.Crs.CrsCod,Gbl.Crs.Info.Type,PathRelDirHTML);

   /***** Open file with web page *****/
   /* 1. Check if index.html exists */
   snprintf (PathRelFileHTML,sizeof (PathRelFileHTML),"%s/index.html",
	     PathRelDirHTML);
   if (Fil_CheckIfPathExists (PathRelFileHTML))	// TODO: Check if not empty?
     {
      snprintf (URL,sizeof (URL),"%s/%ld/%s/index.html",
	        Cfg_URL_CRS_PUBLIC,Gbl.Hierarchy.Crs.CrsCod,
	        Inf_FileNamesForInfoType[Gbl.Crs.Info.Type]);
      Inf_ShowPage (URL);

      return true;
     }

   /* 2. If index.html does not exist, try index.htm */
   snprintf (PathRelFileHTML,sizeof (PathRelFileHTML),"%s/index.htm",
	     PathRelDirHTML);
   if (Fil_CheckIfPathExists (PathRelFileHTML))	// TODO: Check if not empty?
     {
      snprintf (URL,sizeof (URL),"%s/%ld/%s/index.htm",
	        Cfg_URL_CRS_PUBLIC,Gbl.Hierarchy.Crs.CrsCod,
	        Inf_FileNamesForInfoType[Gbl.Crs.Info.Type]);
      Inf_ShowPage (URL);

      return true;
     }

   return false;
  }

/*****************************************************************************/
/* Build path inside a course for a given a info type to store web page file */
/*****************************************************************************/

void Inf_BuildPathPage (long CrsCod,Inf_Type_t InfoType,char PathDir[PATH_MAX + 1])
  {
   snprintf (PathDir,PATH_MAX + 1,"%s/%ld/%s",
             Cfg_PATH_CRS_PUBLIC,CrsCod,Inf_FileNamesForInfoType[InfoType]);
  }

/*****************************************************************************/
/********************* Check if exists link to a page ************************/
/*****************************************************************************/
// Return true if info available

static bool Inf_CheckURL (long CrsCod,Inf_Type_t InfoType)
  {
   char PathFile[PATH_MAX + 1];
   FILE *FileURL;

   /***** Build path to file containing URL *****/
   Inf_BuildPathURL (CrsCod,InfoType,PathFile);

   /***** Check if file with URL exists *****/
   if ((FileURL = fopen (PathFile,"rb")))
     {
      if (fgets (Gbl.Crs.Info.URL,Cns_MAX_BYTES_WWW,FileURL) == NULL)
	 Gbl.Crs.Info.URL[0] = '\0';
      /* File is not longer needed  ==> close it */
      fclose (FileURL);

      if (Gbl.Crs.Info.URL[0])
         return true;
     }

   return false;
  }

/*****************************************************************************/
/**************** Check if exists and show link to a page ********************/
/*****************************************************************************/
// Return true if info available

static bool Inf_CheckAndShowURL (void)
  {
   char PathFile[PATH_MAX + 1];
   FILE *FileURL;

   /***** Build path to file containing URL *****/
   Inf_BuildPathURL (Gbl.Hierarchy.Crs.CrsCod,Gbl.Crs.Info.Type,PathFile);

   /***** Check if file with URL exists *****/
   if ((FileURL = fopen (PathFile,"rb")))
     {
      if (fgets (Gbl.Crs.Info.URL,Cns_MAX_BYTES_WWW,FileURL) == NULL)
	 Gbl.Crs.Info.URL[0] = '\0';
      /* File is not longer needed  ==> close it */
      fclose (FileURL);

      if (Gbl.Crs.Info.URL[0])
	{
	 Inf_ShowPage (Gbl.Crs.Info.URL);
         return true;
	}
     }

   return false;
  }

/*****************************************************************************/
/*** Build path inside a course for a given a info type to store URL file ****/
/*****************************************************************************/

static void Inf_BuildPathURL (long CrsCod,Inf_Type_t InfoType,
                              char PathFile[PATH_MAX + 1])
  {
   snprintf (PathFile,PATH_MAX + 1,"%s/%ld/%s.url",
	     Cfg_PATH_CRS_PRIVATE,CrsCod,Inf_FileNamesForInfoType[InfoType]);
  }

/*****************************************************************************/
/************* Check if exists and write URL into text buffer ****************/
/*****************************************************************************/
// This function is called only from web service

void Inf_WriteURLIntoTxtBuffer (char TxtBuffer[Cns_MAX_BYTES_WWW + 1])
  {
   char PathFile[PATH_MAX + 1];
   FILE *FileURL;

   /***** Initialize buffer *****/
   TxtBuffer[0] = '\0';

   /***** Build path to file containing URL *****/
   Inf_BuildPathURL (Gbl.Hierarchy.Crs.CrsCod,Gbl.Crs.Info.Type,PathFile);

   /***** Check if file with URL exists *****/
   if ((FileURL = fopen (PathFile,"rb")))
     {
      if (fgets (TxtBuffer,Cns_MAX_BYTES_WWW,FileURL) == NULL)
	 TxtBuffer[0] = '\0';
      /* File is not longer needed  ==> close it */
      fclose (FileURL);
     }
  }

/*****************************************************************************/
/*************** Show link to a internal or external a page ******************/
/*****************************************************************************/

static void Inf_ShowPage (const char *URL)
  {
   extern const char *The_ClassFormOutBoxBold[The_NUM_THEMES];
   extern const char *Txt_View_in_a_new_window;
   extern const char *Txt_INFO_TITLE[Inf_NUM_TYPES];
   bool ICanEdit = (Gbl.Usrs.Me.Role.Logged == Rol_TCH ||
                    Gbl.Usrs.Me.Role.Logged == Rol_SYS_ADM);
   const char *Help[Inf_NUM_TYPES] =
     {
      [Inf_INTRODUCTION  ] = Hlp_COURSE_Information_textual_information,
      [Inf_TEACHING_GUIDE] = Hlp_COURSE_Guide,
      [Inf_LECTURES      ] = Hlp_COURSE_Syllabus,
      [Inf_PRACTICALS    ] = Hlp_COURSE_Syllabus,
      [Inf_BIBLIOGRAPHY  ] = Hlp_COURSE_Bibliography,
      [Inf_FAQ           ] = Hlp_COURSE_FAQ,
      [Inf_LINKS         ] = Hlp_COURSE_Links,
      [Inf_ASSESSMENT    ] = Hlp_ASSESSMENT_System,
     };

   /***** Begin box *****/
   if (ICanEdit)
      Box_BoxBegin (NULL,Txt_INFO_TITLE[Gbl.Crs.Info.Type],
		    Inf_PutIconToEditInfo,&Gbl.Crs.Info.Type,
		    Help[Gbl.Crs.Info.Type],Box_NOT_CLOSABLE);
   else
      Box_BoxBegin (NULL,Txt_INFO_TITLE[Gbl.Crs.Info.Type],
		    NULL,NULL,
		    Help[Gbl.Crs.Info.Type],Box_NOT_CLOSABLE);

   /***** Link to view in a new window *****/
   HTM_A_Begin ("href=\"%s\" target=\"_blank\" class=\"%s\"",
	        URL,The_ClassFormOutBoxBold[Gbl.Prefs.Theme]);
      Ico_PutIconTextLink ("expand-arrows-alt.svg",
			   Txt_View_in_a_new_window);
   HTM_A_End ();

   /***** End box *****/
   Box_BoxEnd ();
  }

/*****************************************************************************/
/************************** Set course info source ***************************/
/*****************************************************************************/

void Inf_SetInfoSrc (void)
  {
   struct Syl_Syllabus Syllabus;
   Inf_Src_t InfoSrcSelected = Inf_GetInfoSrcFromForm ();

   /***** Reset syllabus context *****/
   Syl_ResetSyllabus (&Syllabus);

   /***** Set info type *****/
   Inf_AsignInfoType (&Gbl.Crs.Info,&Syllabus);

   /***** Set info source into database *****/
   Inf_DB_SetInfoSrc (InfoSrcSelected);

   /***** Show the selected info *****/
   Inf_ShowInfo ();
  }

/*****************************************************************************/
/************** Select course info source and send course info ***************/
/*****************************************************************************/

void Inf_FormsToSelSendInfo (void)
  {
   extern const char *The_ClassFormInBox[The_NUM_THEMES];
   extern const char *The_ClassDat[The_NUM_THEMES];
   extern const char *Txt_Source_of_information;
   extern const char *Txt_INFO_SRC_FULL_TEXT[Inf_NUM_SOURCES];
   extern const char *Txt_INFO_SRC_HELP[Inf_NUM_SOURCES];
   struct Syl_Syllabus Syllabus;
   struct Inf_FromDB FromDB;
   Inf_Src_t InfoSrc;
   bool InfoAvailable[Inf_NUM_SOURCES];
   const char *HelpEdit[Inf_NUM_TYPES] =
     {
      [Inf_INTRODUCTION  ] = Hlp_COURSE_Information_edit,
      [Inf_TEACHING_GUIDE] = Hlp_COURSE_Guide_edit,
      [Inf_LECTURES      ] = Hlp_COURSE_Syllabus_edit,
      [Inf_PRACTICALS    ] = Hlp_COURSE_Syllabus_edit,
      [Inf_BIBLIOGRAPHY  ] = Hlp_COURSE_Bibliography_edit,
      [Inf_FAQ           ] = Hlp_COURSE_FAQ_edit,
      [Inf_LINKS         ] = Hlp_COURSE_Links_edit,
      [Inf_ASSESSMENT    ] = Hlp_ASSESSMENT_System_edit,
     };

   /***** Reset syllabus context *****/
   Syl_ResetSyllabus (&Syllabus);

   /***** Set info type *****/
   Inf_AsignInfoType (&Gbl.Crs.Info,&Syllabus);

   /***** Get current info source from database *****/
   Inf_GetAndCheckInfoSrcFromDB (&Syllabus,
                                 Gbl.Hierarchy.Crs.CrsCod,
                                 Gbl.Crs.Info.Type,
                                 &FromDB);

   /***** Check if info available *****/
   for (InfoSrc  = (Inf_Src_t) 0;
	InfoSrc <= (Inf_Src_t) (Inf_NUM_SOURCES - 1);
	InfoSrc++)
      InfoAvailable[InfoSrc] = Inf_CheckIfInfoAvailable (&Syllabus,InfoSrc);

   /***** Set info source to none
          when no info available for the current source *****/
   if (FromDB.Src != Inf_NONE &&
       !InfoAvailable[FromDB.Src])
     {
      FromDB.Src = Inf_NONE;
      Inf_DB_SetInfoSrc (Inf_NONE);
     }

   /***** Form to choice between alternatives *****/
   /* Begin box and table */
   Box_BoxTableBegin (NULL,Txt_Source_of_information,
                      Inf_PutIconToViewInfo,&Gbl.Crs.Info.Type,
                      HelpEdit[Gbl.Crs.Info.Type],Box_NOT_CLOSABLE,4);

      /* Options */
      for (InfoSrc  = (Inf_Src_t) 0;
	   InfoSrc <= (Inf_Src_t) (Inf_NUM_SOURCES - 1);
	   InfoSrc++)
	{
	 HTM_TR_Begin (NULL);

	    /* Select info source */
	    HTM_TD_Begin ("class=\"%s LT%s\"",
			  The_ClassDat[Gbl.Prefs.Theme],
			  InfoSrc == FromDB.Src ? " LIGHT_BLUE" :
				                  "");
	       Frm_BeginForm (Inf_ActionsSelecInfoSrc[Gbl.Crs.Info.Type]);
		  HTM_INPUT_RADIO ("InfoSrc",InfoSrc != FromDB.Src &&
					     (InfoSrc == Inf_NONE ||
					      InfoAvailable[InfoSrc]),	// Info available for this source
				   "id=\"InfoSrc%u\" value=\"%u\"%s",
				   (unsigned) InfoSrc,(unsigned) InfoSrc,
				   InfoSrc == FromDB.Src ? " checked=\"checked\"" :
							   (InfoSrc == Inf_NONE ||
							   InfoAvailable[InfoSrc]) ? "" :	// Info available for this source
										     " disabled=\"disabled\"");
	       Frm_EndForm ();
	    HTM_TD_End ();

	    /* Form for this info source */
	    if (InfoSrc == FromDB.Src)
	       HTM_TD_Begin ("class=\"LT LIGHT_BLUE\"");
	    else
	       HTM_TD_Begin ("class=\"LT\"");
	    HTM_LABEL_Begin ("for=\"InfoSrc%u\" class=\"%s\"",
			     (unsigned) InfoSrc,The_ClassFormInBox[Gbl.Prefs.Theme]);
	       HTM_Txt (Txt_INFO_SRC_FULL_TEXT[InfoSrc]);
	    HTM_LABEL_End ();
	    if (Txt_INFO_SRC_HELP[InfoSrc])
	      {
	       HTM_SPAN_Begin ("class=\"%s\"",The_ClassDat[Gbl.Prefs.Theme]);
		  HTM_BR ();
		  HTM_TxtF ("(%s)",Txt_INFO_SRC_HELP[InfoSrc]);
	       HTM_SPAN_End ();
	      }
	    if (Inf_FormsForEditionTypes[InfoSrc])
	       Inf_FormsForEditionTypes[InfoSrc] (InfoSrc);
	    HTM_TD_End ();

	 HTM_TR_End ();
	}

   /***** End table and box *****/
   Box_BoxTableEnd ();
  }

/*****************************************************************************/
/* Check if there is info available for current info type and a given source */
/*****************************************************************************/

static bool Inf_CheckIfInfoAvailable (struct Syl_Syllabus *Syllabus,
                                      Inf_Src_t InfoSrc)
  {
   switch (InfoSrc)
     {
      case Inf_NONE:
	 return false;
      case Inf_EDITOR:
         switch (Gbl.Crs.Info.Type)
           {
            case Inf_LECTURES:
	       Syllabus->WhichSyllabus = Syl_LECTURES;
               return Syl_CheckSyllabus (Syllabus,Gbl.Hierarchy.Crs.CrsCod);
            case Inf_PRACTICALS:
	       Syllabus->WhichSyllabus = Syl_PRACTICALS;
               return Syl_CheckSyllabus (Syllabus,Gbl.Hierarchy.Crs.CrsCod);
            default:
               return false;
           }
         return false;	// Not reached
      case Inf_PLAIN_TEXT:
         return Inf_CheckPlainTxt (Gbl.Hierarchy.Crs.CrsCod,
                                   Gbl.Crs.Info.Type);
      case Inf_RICH_TEXT:
         return Inf_CheckRichTxt (Gbl.Hierarchy.Crs.CrsCod,
                                  Gbl.Crs.Info.Type);
      case Inf_PAGE:
	 return Inf_CheckPage (Gbl.Hierarchy.Crs.CrsCod,
                               Gbl.Crs.Info.Type);
      case Inf_URL:
	 return Inf_CheckURL (Gbl.Hierarchy.Crs.CrsCod,
                              Gbl.Crs.Info.Type);
     }

   return false;	// Not reached
  }

/*****************************************************************************/
/****************** Form to enter in integrated editor ***********************/
/*****************************************************************************/

void Inf_FormToEnterIntegratedEditor (Inf_Src_t InfoSrc)
  {
   extern const char *Txt_Edit;

   Frm_BeginForm (Inf_ActionsInfo[InfoSrc][Gbl.Crs.Info.Type]);
      Btn_PutConfirmButton (Txt_Edit);
   Frm_EndForm ();
  }

/*****************************************************************************/
/****************** Form to enter in plain text editor ***********************/
/*****************************************************************************/

void Inf_FormToEnterPlainTextEditor (Inf_Src_t InfoSrc)
  {
   extern const char *Txt_Edit_plain_text;

   Frm_BeginForm (Inf_ActionsInfo[InfoSrc][Gbl.Crs.Info.Type]);
      Btn_PutConfirmButton (Txt_Edit_plain_text);
   Frm_EndForm ();
  }

/*****************************************************************************/
/******************* Form to enter in rich text editor ***********************/
/*****************************************************************************/

void Inf_FormToEnterRichTextEditor (Inf_Src_t InfoSrc)
  {
   extern const char *Txt_Edit_rich_text;

   Frm_BeginForm (Inf_ActionsInfo[InfoSrc][Gbl.Crs.Info.Type]);
      Btn_PutConfirmButton (Txt_Edit_rich_text);
   Frm_EndForm ();
  }

/*****************************************************************************/
/******************* Form to upload a file with a page ***********************/
/*****************************************************************************/

void Inf_FormToSendPage (Inf_Src_t InfoSrc)
  {
   extern const char *The_ClassFormInBox[The_NUM_THEMES];
   extern const char *Txt_File;
   extern const char *Txt_Upload_file;

   /***** Begin form *****/
   Frm_BeginForm (Inf_ActionsInfo[InfoSrc][Gbl.Crs.Info.Type]);

      /***** File *****/
      HTM_DIV_Begin ("class=\"CM\"");
	 HTM_LABEL_Begin ("class=\"%s\"",The_ClassFormInBox[Gbl.Prefs.Theme]);
	    HTM_TxtColonNBSP (Txt_File);
	    HTM_INPUT_FILE (Fil_NAME_OF_PARAM_FILENAME_ORG,".htm,.html,.pdf,.zip",
			    HTM_DONT_SUBMIT_ON_CHANGE,
			    NULL);
	 HTM_LABEL_End ();
      HTM_DIV_End ();

      /***** Send button *****/
      Btn_PutCreateButton (Txt_Upload_file);

   /***** End form *****/
   Frm_EndForm ();
  }

/*****************************************************************************/
/********************* Form to send a link to a web page *********************/
/*****************************************************************************/

void Inf_FormToSendURL (Inf_Src_t InfoSrc)
  {
   extern const char *The_ClassFormInBox[The_NUM_THEMES];
   extern const char *Txt_URL;
   extern const char *Txt_Send_URL;
   char PathFile[PATH_MAX + 1];
   FILE *FileURL;

   /***** Build path to file containing URL *****/
   Inf_BuildPathURL (Gbl.Hierarchy.Crs.CrsCod,Gbl.Crs.Info.Type,PathFile);

   /***** Begin form *****/
   Frm_BeginForm (Inf_ActionsInfo[InfoSrc][Gbl.Crs.Info.Type]);

      /***** Link *****/
      if ((FileURL = fopen (PathFile,"rb")) != NULL)
	{
	 if (fgets (Gbl.Crs.Info.URL,Cns_MAX_BYTES_WWW,FileURL) == NULL)
	    Gbl.Crs.Info.URL[0] = '\0';
	 /* File is not longer needed. Close it */
	 fclose (FileURL);
	}
      else
	 Gbl.Crs.Info.URL[0] = '\0';

      HTM_DIV_Begin ("class=\"CM\"");
	 HTM_LABEL_Begin ("class=\"%s\"",The_ClassFormInBox[Gbl.Prefs.Theme]);
	    HTM_TxtColonNBSP (Txt_URL);
	    HTM_INPUT_URL ("InfoSrcURL",Gbl.Crs.Info.URL,HTM_DONT_SUBMIT_ON_CHANGE,
			   "size=\"50\"");
	 HTM_LABEL_End ();
      HTM_DIV_End ();

      /***** Send button *****/
      Btn_PutCreateButton (Txt_Send_URL);

   /***** End form *****/
   Frm_EndForm ();
  }

/*****************************************************************************/
/******** Returns bibliography, assessment, etc. from Gbl.Action.Act *********/
/*****************************************************************************/

static void Inf_AsignInfoType (struct Inf_Info *Info,
                               struct Syl_Syllabus *Syllabus)
  {
   switch (Gbl.Action.Act)
     {
      case ActSeeCrsInf:
      case ActEdiCrsInf:
      case ActChgFrcReaCrsInf:
      case ActChgHavReaCrsInf:
      case ActSelInfSrcCrsInf:
      case ActEditorCrsInf:
      case ActRcvURLCrsInf:
      case ActRcvPagCrsInf:
      case ActPlaTxtEdiCrsInf:
      case ActRchTxtEdiCrsInf:
      case ActRcvPlaTxtCrsInf:
      case ActRcvRchTxtCrsInf:
         Info->Type = Inf_INTRODUCTION;
         break;
      case ActSeeTchGui:
      case ActEdiTchGui:
      case ActChgFrcReaTchGui:
      case ActChgHavReaTchGui:
      case ActSelInfSrcTchGui:
      case ActEditorTchGui:
      case ActRcvURLTchGui:
      case ActRcvPagTchGui:
      case ActPlaTxtEdiTchGui:
      case ActRchTxtEdiTchGui:
      case ActRcvPlaTxtTchGui:
      case ActRcvRchTxtTchGui:
         Info->Type = Inf_TEACHING_GUIDE;
         break;
      case ActSeeSyl:
	 Syllabus->WhichSyllabus = Syl_GetParamWhichSyllabus ();
	 Info->Type = (Syllabus->WhichSyllabus == Syl_LECTURES ? Inf_LECTURES :
	                                                         Inf_PRACTICALS);
	 break;
      case ActSeeSylLec:
      case ActEdiSylLec:
      case ActChgFrcReaSylLec:
      case ActChgHavReaSylLec:
      case ActSelInfSrcSylLec:
      case ActEditorSylLec:
      case ActRcvURLSylLec:
      case ActRcvPagSylLec:
      case ActPlaTxtEdiSylLec:
      case ActRchTxtEdiSylLec:
      case ActRcvPlaTxtSylLec:
      case ActRcvRchTxtSylLec:
	 Syllabus->WhichSyllabus = Syl_LECTURES;
         Info->Type = Inf_LECTURES;
         break;
      case ActSeeSylPra:
      case ActEdiSylPra:
      case ActChgFrcReaSylPra:
      case ActChgHavReaSylPra:
      case ActSelInfSrcSylPra:
      case ActEditorSylPra:
      case ActRcvURLSylPra:
      case ActRcvPagSylPra:
      case ActPlaTxtEdiSylPra:
      case ActRchTxtEdiSylPra:
      case ActRcvPlaTxtSylPra:
      case ActRcvRchTxtSylPra:
	 Syllabus->WhichSyllabus = Syl_PRACTICALS;
         Info->Type = Inf_PRACTICALS;
         break;
      case ActSeeBib:
      case ActEdiBib:
      case ActChgFrcReaBib:
      case ActChgHavReaBib:
      case ActSelInfSrcBib:
      case ActEditorBib:
      case ActRcvURLBib:
      case ActRcvPagBib:
      case ActPlaTxtEdiBib:
      case ActRchTxtEdiBib:
      case ActRcvPlaTxtBib:
      case ActRcvRchTxtBib:
         Info->Type = Inf_BIBLIOGRAPHY;
         break;
      case ActSeeFAQ:
      case ActEdiFAQ:
      case ActChgFrcReaFAQ:
      case ActChgHavReaFAQ:
      case ActSelInfSrcFAQ:
      case ActEditorFAQ:
      case ActRcvURLFAQ:
      case ActRcvPagFAQ:
      case ActPlaTxtEdiFAQ:
      case ActRchTxtEdiFAQ:
      case ActRcvPlaTxtFAQ:
      case ActRcvRchTxtFAQ:
         Info->Type = Inf_FAQ;
         break;
      case ActSeeCrsLnk:
      case ActEdiCrsLnk:
      case ActChgFrcReaCrsLnk:
      case ActChgHavReaCrsLnk:
      case ActSelInfSrcCrsLnk:
      case ActEditorCrsLnk:
      case ActRcvURLCrsLnk:
      case ActRcvPagCrsLnk:
      case ActPlaTxtEdiCrsLnk:
      case ActRchTxtEdiCrsLnk:
      case ActRcvPlaTxtCrsLnk:
      case ActRcvRchTxtCrsLnk:
         Info->Type = Inf_LINKS;
         break;
      case ActSeeAss:
      case ActEdiAss:
      case ActChgFrcReaAss:
      case ActChgHavReaAss:
      case ActSelInfSrcAss:
      case ActEditorAss:
      case ActRcvURLAss:
      case ActRcvPagAss:
      case ActPlaTxtEdiAss:
      case ActRchTxtEdiAss:
      case ActRcvPlaTxtAss:
      case ActRcvRchTxtAss:
         Info->Type = Inf_ASSESSMENT;
         break;
     }
  }

/*****************************************************************************/
/********** Get info source for bibliography, FAQ, etc. from form ************/
/*****************************************************************************/

Inf_Src_t Inf_GetInfoSrcFromForm (void)
  {
   /***** Get info source for a specific type of course information
          (introduction, teaching guide, bibliography, FAQ, links or evaluation) *****/
   return (Inf_Src_t)
	  Par_GetParToUnsignedLong ("InfoSrc",
                                    0,
                                    Inf_NUM_SOURCES - 1,
                                    (unsigned long) Inf_NONE);
  }

/*****************************************************************************/
/***** Get and check info source for a type of course info from database *****/
/*****************************************************************************/

Inf_Src_t Inf_GetInfoSrcFromDB (long CrsCod,Inf_Type_t InfoType)
  {
   MYSQL_RES *mysql_res;
   MYSQL_ROW row;
   Inf_Src_t InfoSrc;

   /***** Get info source for a specific type of info from database *****/
   if (Inf_DB_GetInfoSrc (&mysql_res,CrsCod,InfoType))
     {
      /* Get row */
      row = mysql_fetch_row (mysql_res);

      /* Get info source (row[0]) */
      InfoSrc = Inf_DB_ConvertFromStrDBToInfoSrc (row[0]);
     }
   else
      InfoSrc = Inf_NONE;

   /***** Free structure that stores the query result *****/
   DB_FreeMySQLResult (&mysql_res);

   return InfoSrc;
  }

/*****************************************************************************/
/***** Get and check info source for a type of course info from database *****/
/*****************************************************************************/

void Inf_GetAndCheckInfoSrcFromDB (struct Syl_Syllabus *Syllabus,
                                   long CrsCod,
                                   Inf_Type_t InfoType,
                                   struct Inf_FromDB *FromDB)
  {
   MYSQL_RES *mysql_res;
   MYSQL_ROW row;

   /***** Set default values *****/
   FromDB->Src        = Inf_NONE;
   FromDB->MustBeRead = false;

   /***** Get info source for a specific type of info from database *****/
   if (Inf_DB_GetInfoSrcAndMustBeRead (&mysql_res,CrsCod,InfoType) == 1)
     {
      /* Get row */
      row = mysql_fetch_row (mysql_res);

      /* Get info source (row[0]) and if students must read info (row[1]) */
      FromDB->Src        = Inf_DB_ConvertFromStrDBToInfoSrc (row[0]);
      FromDB->MustBeRead = (row[1][0] == 'Y');
     }

   /***** Free structure that stores the query result *****/
   DB_FreeMySQLResult (&mysql_res);

   /***** If info is empty, return Inf_INFO_SRC_NONE *****/
   switch (FromDB->Src)
     {
      case Inf_NONE:
         break;
      case Inf_EDITOR:
         switch (InfoType)
           {
            case Inf_LECTURES:
	       Syllabus->WhichSyllabus = Syl_LECTURES;
               if (!Syl_CheckSyllabus (Syllabus,CrsCod))
                  FromDB->Src = Inf_NONE;
               break;
            case Inf_PRACTICALS:
	       Syllabus->WhichSyllabus = Syl_PRACTICALS;
               if (!Syl_CheckSyllabus (Syllabus,CrsCod))
                  FromDB->Src = Inf_NONE;
               break;
            case Inf_INTRODUCTION:
            case Inf_TEACHING_GUIDE:
            case Inf_BIBLIOGRAPHY:
            case Inf_FAQ:
            case Inf_LINKS:
            case Inf_ASSESSMENT:
               FromDB->Src = Inf_NONE;
	       break;	// Internal editor is not yet available
           }
         break;
      case Inf_PLAIN_TEXT:
	 if (!Inf_CheckPlainTxt (CrsCod,InfoType))
            FromDB->Src = Inf_NONE;
         break;
      case Inf_RICH_TEXT:
	 if (!Inf_CheckRichTxt (CrsCod,InfoType))
            FromDB->Src = Inf_NONE;
         break;
      case Inf_PAGE:
	 if (!Inf_CheckPage (CrsCod,InfoType))
	    FromDB->Src = Inf_NONE;
         break;
      case Inf_URL:
	 if (!Inf_CheckURL (CrsCod,InfoType))
	    FromDB->Src = Inf_NONE;
         break;
     }

   if (FromDB->Src == Inf_NONE)
      FromDB->MustBeRead = false;
  }

/*****************************************************************************/
/********** Get info text for a type of course info from database ************/
/*****************************************************************************/

void Inf_GetInfoTxtFromDB (long CrsCod,Inf_Type_t InfoType,
                           char InfoTxtHTML[Cns_MAX_BYTES_LONG_TEXT + 1],
                           char InfoTxtMD  [Cns_MAX_BYTES_LONG_TEXT + 1])
  {
   MYSQL_RES *mysql_res;
   MYSQL_ROW row;

   /***** Get info source for a specific type of course information
          (bibliography, FAQ, links or evaluation) from database *****/
   if (Inf_DB_GetInfoTxt (&mysql_res,CrsCod,InfoType) == 1)
     {
      /* Get info text */
      row = mysql_fetch_row (mysql_res);

      /* Get text in HTML format (not rigorous) */
      if (InfoTxtHTML)
	 Str_Copy (InfoTxtHTML,row[0],Cns_MAX_BYTES_LONG_TEXT);

      /* Get text in Markdown format */
      if (InfoTxtMD)
	 Str_Copy (InfoTxtMD  ,row[1],Cns_MAX_BYTES_LONG_TEXT);
     }
   else
     {
      if (InfoTxtHTML)
         InfoTxtHTML[0] = '\0';
      if (InfoTxtMD)
         InfoTxtMD  [0] = '\0';
     }

   /***** Free structure that stores the query result *****/
   DB_FreeMySQLResult (&mysql_res);
  }

/*****************************************************************************/
/********************* Check information about the course ********************/
/*****************************************************************************/
// Return true if info available

static bool Inf_CheckPlainTxt (long CrsCod,Inf_Type_t InfoType)
  {
   char TxtHTML[Cns_MAX_BYTES_LONG_TEXT + 1];

   /***** Get info text from database *****/
   Inf_GetInfoTxtFromDB (CrsCod,InfoType,TxtHTML,NULL);

   return (TxtHTML[0] != '\0');
  }

/*****************************************************************************/
/********************* Show information about the course *********************/
/*****************************************************************************/
// Return true if info available

static bool Inf_CheckAndShowPlainTxt (void)
  {
   extern const char *The_ClassDat[The_NUM_THEMES];
   extern const char *Txt_INFO_TITLE[Inf_NUM_TYPES];
   char TxtHTML[Cns_MAX_BYTES_LONG_TEXT + 1];
   bool ICanEdit = (Gbl.Usrs.Me.Role.Logged == Rol_TCH ||
                    Gbl.Usrs.Me.Role.Logged == Rol_SYS_ADM);
   const char *Help[Inf_NUM_TYPES] =
     {
      [Inf_INTRODUCTION  ] = Hlp_COURSE_Information_textual_information,
      [Inf_TEACHING_GUIDE] = Hlp_COURSE_Guide,
      [Inf_LECTURES      ] = Hlp_COURSE_Syllabus,
      [Inf_PRACTICALS    ] = Hlp_COURSE_Syllabus,
      [Inf_BIBLIOGRAPHY  ] = Hlp_COURSE_Bibliography,
      [Inf_FAQ           ] = Hlp_COURSE_FAQ,
      [Inf_LINKS         ] = Hlp_COURSE_Links,
      [Inf_ASSESSMENT    ] = Hlp_ASSESSMENT_System,
     };

   /***** Get info text from database *****/
   Inf_GetInfoTxtFromDB (Gbl.Hierarchy.Crs.CrsCod,Gbl.Crs.Info.Type,
                         TxtHTML,NULL);

   if (TxtHTML[0])
     {
      /***** Begin box *****/
      if (ICanEdit)
	 Box_BoxBegin (NULL,Txt_INFO_TITLE[Gbl.Crs.Info.Type],
		       Inf_PutIconToEditInfo,&Gbl.Crs.Info.Type,
		       Help[Gbl.Crs.Info.Type],Box_NOT_CLOSABLE);
      else
	 Box_BoxBegin (NULL,Txt_INFO_TITLE[Gbl.Crs.Info.Type],
		       NULL,NULL,
		       Help[Gbl.Crs.Info.Type],Box_NOT_CLOSABLE);

      if (Gbl.Crs.Info.Type == Inf_INTRODUCTION ||
          Gbl.Crs.Info.Type == Inf_TEACHING_GUIDE)
         Lay_WriteHeaderClassPhoto (false,false,Gbl.Hierarchy.Ins.InsCod,Gbl.Hierarchy.Deg.DegCod,Gbl.Hierarchy.Crs.CrsCod);

      HTM_DIV_Begin ("class=\"%s LM\"",The_ClassDat[Gbl.Prefs.Theme]);

	 /***** Convert to respectful HTML and insert links *****/
	 Str_ChangeFormat (Str_FROM_HTML,Str_TO_RIGOROUS_HTML,
			   TxtHTML,Cns_MAX_BYTES_LONG_TEXT,false);	// Convert from HTML to recpectful HTML
	 ALn_InsertLinks (TxtHTML,Cns_MAX_BYTES_LONG_TEXT,60);	// Insert links

	 /***** Write text *****/
	 HTM_Txt (TxtHTML);

      /***** End box *****/
      HTM_DIV_End ();
      Box_BoxEnd ();

      return true;
     }

   return false;
  }

/*****************************************************************************/
/********************* Show information about the course *********************/
/*****************************************************************************/
// Return true if info available

static bool Inf_CheckRichTxt (long CrsCod,Inf_Type_t InfoType)
  {
   char TxtHTML[Cns_MAX_BYTES_LONG_TEXT + 1];
   char TxtMD[Cns_MAX_BYTES_LONG_TEXT + 1];

   /***** Get info text from database *****/
   Inf_GetInfoTxtFromDB (CrsCod,InfoType,
                         TxtHTML,TxtMD);

   return (TxtMD[0] != '\0');
  }

/*****************************************************************************/
/********************* Show information about the course *********************/
/*****************************************************************************/
// Return true if info available

static bool Inf_CheckAndShowRichTxt (void)
  {
   extern const char *Txt_INFO_TITLE[Inf_NUM_TYPES];
   char TxtHTML[Cns_MAX_BYTES_LONG_TEXT + 1];
   char TxtMD[Cns_MAX_BYTES_LONG_TEXT + 1];
   char PathFileMD[PATH_MAX + 1];
   char PathFileHTML[PATH_MAX + 1];
   FILE *FileMD;		// Temporary Markdown file
   FILE *FileHTML;		// Temporary HTML file
   char MathJaxURL[PATH_MAX + 1];
   char Command[512 + PATH_MAX * 3]; // Command to call the program of preprocessing of photos
   int ReturnCode;
   bool ICanEdit = (Gbl.Usrs.Me.Role.Logged == Rol_TCH ||
                    Gbl.Usrs.Me.Role.Logged == Rol_SYS_ADM);
   const char *Help[Inf_NUM_TYPES] =
     {
      [Inf_INTRODUCTION  ] = Hlp_COURSE_Information_textual_information,
      [Inf_TEACHING_GUIDE] = Hlp_COURSE_Guide,
      [Inf_LECTURES      ] = Hlp_COURSE_Syllabus,
      [Inf_PRACTICALS    ] = Hlp_COURSE_Syllabus,
      [Inf_BIBLIOGRAPHY  ] = Hlp_COURSE_Bibliography,
      [Inf_FAQ           ] = Hlp_COURSE_FAQ,
      [Inf_LINKS         ] = Hlp_COURSE_Links,
      [Inf_ASSESSMENT    ] = Hlp_ASSESSMENT_System,
     };

   /***** Get info text from database *****/
   Inf_GetInfoTxtFromDB (Gbl.Hierarchy.Crs.CrsCod,Gbl.Crs.Info.Type,
                         TxtHTML,TxtMD);

   if (TxtMD[0])
     {
      /***** Begin box *****/
      if (ICanEdit)
	 Box_BoxBegin (NULL,Txt_INFO_TITLE[Gbl.Crs.Info.Type],
		       Inf_PutIconToEditInfo,&Gbl.Crs.Info.Type,
		       Help[Gbl.Crs.Info.Type],Box_NOT_CLOSABLE);
      else
	 Box_BoxBegin (NULL,Txt_INFO_TITLE[Gbl.Crs.Info.Type],
		       NULL,NULL,
		       Help[Gbl.Crs.Info.Type],Box_NOT_CLOSABLE);

      if (Gbl.Crs.Info.Type == Inf_INTRODUCTION ||
          Gbl.Crs.Info.Type == Inf_TEACHING_GUIDE)
         Lay_WriteHeaderClassPhoto (false,false,Gbl.Hierarchy.Ins.InsCod,Gbl.Hierarchy.Deg.DegCod,Gbl.Hierarchy.Crs.CrsCod);

      HTM_DIV_Begin ("id=\"crs_info\" class=\"LM\"");

	 /***** Store text into a temporary .md file in HTML output directory *****/
	 // TODO: change to another directory?
	 /* Create a unique name for the .md file */
	 snprintf (PathFileMD,sizeof (PathFileMD),"%s/%s.md",
		   Cfg_PATH_OUT_PRIVATE,Gbl.UniqueNameEncrypted);
	 snprintf (PathFileHTML,sizeof (PathFileHTML),"%s/%s.md.html",	// Do not use only .html because that is the output temporary file
		   Cfg_PATH_OUT_PRIVATE,Gbl.UniqueNameEncrypted);

	 /* Open Markdown file for writing */
	 if ((FileMD = fopen (PathFileMD,"wb")) == NULL)
	    Err_ShowErrorAndExit ("Can not create temporary Markdown file.");

	 /* Write text into Markdown file */
	 fprintf (FileMD,"%s",TxtMD);

	 /* Close Markdown file */
	 fclose (FileMD);

	 /***** Convert from Markdown to HTML *****/
	 /* MathJax 2.5.1
#ifdef Cfg_MATHJAX_LOCAL
	 // Use the local copy of MathJax
	 snprintf (MathJaxURL,sizeof (MathJaxURL),
		   "=%s/MathJax/MathJax.js?config=TeX-AMS-MML_HTMLorMML",
		   Cfg_URL_SWAD_PUBLIC);
#else
	 // Use the MathJax Content Delivery Network (CDN)
	 MathJaxURL[0] = '\0';
#endif
	 */
	 /* MathJax 3.0.1 */
#ifdef Cfg_MATHJAX_LOCAL
	 // Use the local copy of MathJax
	 snprintf (MathJaxURL,sizeof (MathJaxURL),"=%s/mathjax/tex-chtml.js",
		   Cfg_URL_SWAD_PUBLIC);
#else
	 // Use the MathJax Content Delivery Network (CDN)
	 MathJaxURL[0] = '\0';
#endif
	 // --ascii uses only ascii characters in output
	 //         (uses numerical entities instead of UTF-8)
	 //         is mandatory in order to convert (with iconv) the UTF-8 output of pandoc to WINDOWS-1252
	 snprintf (Command,sizeof (Command),
		   "iconv -f WINDOWS-1252 -t UTF-8 %s"
		   " | "
		   "pandoc --ascii --mathjax%s -f markdown_github+tex_math_dollars -t html5"
		   " | "
		   "iconv -f UTF-8 -t WINDOWS-1252 -o %s",
		   PathFileMD,
		   MathJaxURL,
		   PathFileHTML);
	 ReturnCode = system (Command);
	 if (ReturnCode == -1)
	    Err_ShowErrorAndExit ("Error when running command to convert from Markdown to HTML.");

	 /***** Remove Markdown file *****/
	 unlink (PathFileMD);

	 /***** Copy HTML file just created to HTML output *****/
	 /* Open temporary HTML file for reading */
	 if ((FileHTML = fopen (PathFileHTML,"rb")) == NULL)
	    Err_ShowErrorAndExit ("Can not open temporary HTML file.");

	 /* Copy from temporary HTML file to output file */
	 Fil_FastCopyOfOpenFiles (FileHTML,Gbl.F.Out);

	 /* Close and remove temporary HTML file */
	 fclose (FileHTML);
	 unlink (PathFileHTML);

      /***** End box *****/
      HTM_DIV_End ();
      Box_BoxEnd ();

      return true;
     }

   return false;
  }

/*****************************************************************************/
/************** Edit plain text information about the course *****************/
/*****************************************************************************/

void Inf_EditPlainTxtInfo (void)
  {
   extern const char *Txt_INFO_TITLE[Inf_NUM_TYPES];
   extern const char *Txt_Save_changes;
   struct Syl_Syllabus Syllabus;
   char TxtHTML[Cns_MAX_BYTES_LONG_TEXT + 1];
   const char *HelpEdit[Inf_NUM_TYPES] =
     {
      [Inf_INTRODUCTION  ] = Hlp_COURSE_Information_edit,
      [Inf_TEACHING_GUIDE] = Hlp_COURSE_Guide_edit,
      [Inf_LECTURES      ] = Hlp_COURSE_Syllabus_edit,
      [Inf_PRACTICALS    ] = Hlp_COURSE_Syllabus_edit,
      [Inf_BIBLIOGRAPHY  ] = Hlp_COURSE_Bibliography_edit,
      [Inf_FAQ           ] = Hlp_COURSE_FAQ_edit,
      [Inf_LINKS         ] = Hlp_COURSE_Links_edit,
      [Inf_ASSESSMENT    ] = Hlp_ASSESSMENT_System_edit,
     };

   /***** Reset syllabus context *****/
   Syl_ResetSyllabus (&Syllabus);

   /***** Set info type *****/
   Inf_AsignInfoType (&Gbl.Crs.Info,&Syllabus);

   /***** Begin form and box *****/
   Frm_BeginForm (Inf_ActionsRcvPlaTxtInfo[Gbl.Crs.Info.Type]);
      Box_BoxBegin (NULL,Txt_INFO_TITLE[Gbl.Crs.Info.Type],
		    NULL,NULL,
		    HelpEdit[Gbl.Crs.Info.Type],Box_NOT_CLOSABLE);

	 if (Gbl.Crs.Info.Type == Inf_INTRODUCTION ||
	     Gbl.Crs.Info.Type == Inf_TEACHING_GUIDE)
	    Lay_WriteHeaderClassPhoto (false,false,Gbl.Hierarchy.Ins.InsCod,Gbl.Hierarchy.Deg.DegCod,Gbl.Hierarchy.Crs.CrsCod);

	 /***** Get info text from database *****/
	 Inf_GetInfoTxtFromDB (Gbl.Hierarchy.Crs.CrsCod,Gbl.Crs.Info.Type,
			       TxtHTML,NULL);

	 /***** Edition area *****/
	 HTM_DIV_Begin ("class=\"CM\"");
	    Lay_HelpPlainEditor ();
	    HTM_TEXTAREA_Begin ("name=\"Txt\" cols=\"80\" rows=\"20\"");
	       HTM_Txt (TxtHTML);
	    HTM_TEXTAREA_End ();
	 HTM_DIV_End ();

      /***** Send button and end box *****/
      Box_BoxWithButtonEnd (Btn_CONFIRM_BUTTON,Txt_Save_changes);
   Frm_EndForm ();
  }

/*****************************************************************************/
/*************** Edit rich text information about the course *****************/
/*****************************************************************************/

void Inf_EditRichTxtInfo (void)
  {
   extern const char *Txt_INFO_TITLE[Inf_NUM_TYPES];
   extern const char *Txt_Save_changes;
   struct Syl_Syllabus Syllabus;
   char TxtHTML[Cns_MAX_BYTES_LONG_TEXT + 1];
   const char *HelpEdit[Inf_NUM_TYPES] =
     {
      [Inf_INTRODUCTION  ] = Hlp_COURSE_Information_edit,
      [Inf_TEACHING_GUIDE] = Hlp_COURSE_Guide_edit,
      [Inf_LECTURES      ] = Hlp_COURSE_Syllabus_edit,
      [Inf_PRACTICALS    ] = Hlp_COURSE_Syllabus_edit,
      [Inf_BIBLIOGRAPHY  ] = Hlp_COURSE_Bibliography_edit,
      [Inf_FAQ           ] = Hlp_COURSE_FAQ_edit,
      [Inf_LINKS         ] = Hlp_COURSE_Links_edit,
      [Inf_ASSESSMENT    ] = Hlp_ASSESSMENT_System_edit,
     };

   /***** Reset syllabus context *****/
   Syl_ResetSyllabus (&Syllabus);

   /***** Set info type *****/
   Inf_AsignInfoType (&Gbl.Crs.Info,&Syllabus);

   /***** Begin form and box *****/
   Frm_BeginForm (Inf_ActionsRcvRchTxtInfo[Gbl.Crs.Info.Type]);
      Box_BoxBegin (NULL,Txt_INFO_TITLE[Gbl.Crs.Info.Type],
		    NULL,NULL,
		    HelpEdit[Gbl.Crs.Info.Type],Box_NOT_CLOSABLE);

      if (Gbl.Crs.Info.Type == Inf_INTRODUCTION ||
	  Gbl.Crs.Info.Type == Inf_TEACHING_GUIDE)
	 Lay_WriteHeaderClassPhoto (false,false,Gbl.Hierarchy.Ins.InsCod,Gbl.Hierarchy.Deg.DegCod,Gbl.Hierarchy.Crs.CrsCod);

      /***** Get info text from database *****/
      Inf_GetInfoTxtFromDB (Gbl.Hierarchy.Crs.CrsCod,Gbl.Crs.Info.Type,
			    TxtHTML,NULL);

      /***** Edition area *****/
      HTM_DIV_Begin ("class=\"CM\"");
	 Lay_HelpRichEditor ();
	 HTM_TEXTAREA_Begin ("name=\"Txt\" cols=\"80\" rows=\"20\"");
	    HTM_Txt (TxtHTML);
	 HTM_TEXTAREA_End ();
      HTM_DIV_End ();

      /***** Send button and end box *****/
      Box_BoxWithButtonEnd (Btn_CONFIRM_BUTTON,Txt_Save_changes);
   Frm_EndForm ();
  }

/*****************************************************************************/
/*************** Receive and change plain text of course info ****************/
/*****************************************************************************/

void Inf_RecAndChangePlainTxtInfo (void)
  {
   struct Syl_Syllabus Syllabus;
   char Txt_HTMLFormat    [Cns_MAX_BYTES_LONG_TEXT + 1];
   char Txt_MarkdownFormat[Cns_MAX_BYTES_LONG_TEXT + 1];

   /***** Reset syllabus context *****/
   Syl_ResetSyllabus (&Syllabus);

   /***** Set info type *****/
   Inf_AsignInfoType (&Gbl.Crs.Info,&Syllabus);

   /***** Get text with course information from form *****/
   Par_GetParameter (Par_PARAM_SINGLE,"Txt",Txt_HTMLFormat,
                     Cns_MAX_BYTES_LONG_TEXT,NULL);
   Str_Copy (Txt_MarkdownFormat,Txt_HTMLFormat,sizeof (Txt_MarkdownFormat) - 1);
   Str_ChangeFormat (Str_FROM_FORM,Str_TO_HTML,
                     Txt_HTMLFormat,Cns_MAX_BYTES_LONG_TEXT,true);	// Store in HTML format (not rigorous)
   Str_ChangeFormat (Str_FROM_FORM,Str_TO_MARKDOWN,
                     Txt_MarkdownFormat,Cns_MAX_BYTES_LONG_TEXT,true);	// Store a copy in Markdown format

   /***** Update text of course info in database *****/
   Inf_DB_SetInfoTxt (Txt_HTMLFormat,Txt_MarkdownFormat);

   /***** Change info source to "plain text" in database *****/
   Inf_DB_SetInfoSrc (Txt_HTMLFormat[0] ? Inf_PLAIN_TEXT :
	                                  Inf_NONE);
   if (Txt_HTMLFormat[0])
      /***** Show the updated info *****/
      Inf_ShowInfo ();
   else
      /***** Show again the form to select and send course info *****/
      Inf_FormsToSelSendInfo ();
  }

/*****************************************************************************/
/*************** Receive and change rich text of course info *****************/
/*****************************************************************************/

void Inf_RecAndChangeRichTxtInfo (void)
  {
   struct Syl_Syllabus Syllabus;
   char Txt_HTMLFormat    [Cns_MAX_BYTES_LONG_TEXT + 1];
   char Txt_MarkdownFormat[Cns_MAX_BYTES_LONG_TEXT + 1];

   /***** Reset syllabus context *****/
   Syl_ResetSyllabus (&Syllabus);

   /***** Set info type *****/
   Inf_AsignInfoType (&Gbl.Crs.Info,&Syllabus);

   /***** Get text with course information from form *****/
   Par_GetParameter (Par_PARAM_SINGLE,"Txt",Txt_HTMLFormat,
                     Cns_MAX_BYTES_LONG_TEXT,NULL);
   Str_Copy (Txt_MarkdownFormat,Txt_HTMLFormat,sizeof (Txt_MarkdownFormat) - 1);
   Str_ChangeFormat (Str_FROM_FORM,Str_TO_HTML,
                     Txt_HTMLFormat,Cns_MAX_BYTES_LONG_TEXT,true);	// Store in HTML format (not rigorous)
   Str_ChangeFormat (Str_FROM_FORM,Str_TO_MARKDOWN,
                     Txt_MarkdownFormat,Cns_MAX_BYTES_LONG_TEXT,true);	// Store a copy in Markdown format

   /***** Update text of course info in database *****/
   Inf_DB_SetInfoTxt (Txt_HTMLFormat,Txt_MarkdownFormat);

   /***** Change info source to "rich text" in database *****/
   Inf_DB_SetInfoSrc (Txt_HTMLFormat[0] ? Inf_RICH_TEXT :
	                                  Inf_NONE);
   if (Txt_HTMLFormat[0])
      /***** Show the updated info *****/
      Inf_ShowInfo ();
   else
      /***** Show again the form to select and send course info *****/
      Inf_FormsToSelSendInfo ();
  }

/*****************************************************************************/
/********* Receive a link to a page of syllabus, bibliography, etc. **********/
/*****************************************************************************/

void Inf_ReceiveURLInfo (void)
  {
   extern const char *Txt_The_URL_X_has_been_updated;
   struct Syl_Syllabus Syllabus;
   char PathFile[PATH_MAX + 1];
   FILE *FileURL;
   bool URLIsOK = false;

   /***** Reset syllabus context *****/
   Syl_ResetSyllabus (&Syllabus);

   /***** Set info type *****/
   Inf_AsignInfoType (&Gbl.Crs.Info,&Syllabus);

   /***** Get parameter with URL *****/
   Par_GetParToText ("InfoSrcURL",Gbl.Crs.Info.URL,Cns_MAX_BYTES_WWW);

   /***** Build path to file containing URL *****/
   Inf_BuildPathURL (Gbl.Hierarchy.Crs.CrsCod,Gbl.Crs.Info.Type,PathFile);

   /***** Open file with URL *****/
   if ((FileURL = fopen (PathFile,"wb")) != NULL)
     {
      /***** Write URL *****/
      fprintf (FileURL,"%s",Gbl.Crs.Info.URL);

      /***** Close file *****/
      fclose (FileURL);

      /***** Write message *****/
      Ale_ShowAlert (Ale_SUCCESS,Txt_The_URL_X_has_been_updated,
                     Gbl.Crs.Info.URL);
      URLIsOK = true;
     }
   else
      Ale_ShowAlert (Ale_ERROR,"Error when saving URL to file.");

   if (URLIsOK)
     {
      /***** Change info source to URL in database *****/
      Inf_DB_SetInfoSrc (Inf_URL);

      /***** Show the updated info *****/
      Inf_ShowInfo ();
     }
   else
     {
      /***** Change info source to none in database *****/
      Inf_DB_SetInfoSrc (Inf_NONE);

      /***** Show again the form to select and send course info *****/
      Inf_FormsToSelSendInfo ();
     }
  }

/*****************************************************************************/
/************** Receive a page of syllabus, bibliography, etc. ***************/
/*****************************************************************************/

void Inf_ReceivePagInfo (void)
  {
   extern const char *Txt_The_file_type_is_X_and_should_be_HTML_or_ZIP;
   extern const char *Txt_The_HTML_file_has_been_received_successfully;
   extern const char *Txt_The_ZIP_file_has_been_received_successfully;
   extern const char *Txt_The_ZIP_file_has_been_unzipped_successfully;
   extern const char *Txt_Found_an_index_html_file;
   extern const char *Txt_Found_an_index_htm_file;
   extern const char *Txt_No_file_index_html_index_htm_found_within_the_ZIP_file;
   extern const char *Txt_The_file_type_should_be_HTML_or_ZIP;
   struct Syl_Syllabus Syllabus;
   struct Param *Param;
   char SourceFileName[PATH_MAX + 1];
   char PathRelDirHTML[PATH_MAX + 1];
   char PathRelFileHTML[PATH_MAX + 1 + 10 + 1];
   char PathRelFileZIP[PATH_MAX + 1 + NAME_MAX + 1];
   char MIMEType[Brw_MAX_BYTES_MIME_TYPE + 1];
   char StrUnzip[128 + PATH_MAX + 1 + NAME_MAX + 1 + PATH_MAX + 1];
   bool WrongType = false;
   bool FileIsOK = false;

   /***** Reset syllabus context *****/
   Syl_ResetSyllabus (&Syllabus);

   /***** Set info type *****/
   Inf_AsignInfoType (&Gbl.Crs.Info,&Syllabus);

   /***** First of all, store in disk the file from stdin (really from Gbl.F.Tmp) *****/
   Param = Fil_StartReceptionOfFile (Fil_NAME_OF_PARAM_FILENAME_ORG,
                                     SourceFileName,MIMEType);

   /***** Check that MIME type is HTML or ZIP *****/
   if (strcmp (MIMEType,"text/html"))
      if (strcmp (MIMEType,"text/plain"))
         if (strcmp (MIMEType,"application/x-zip-compressed"))
            if (strcmp (MIMEType,"application/zip"))
               if (strcmp (MIMEType,"application/x-download"))
		  if (strcmp (MIMEType,"application/octet-stream"))
	             if (strcmp (MIMEType,"application/octetstream"))
	                if (strcmp (MIMEType,"application/octet"))
                           WrongType = true;
   if (WrongType)
      Ale_ShowAlert (Ale_INFO,Txt_The_file_type_is_X_and_should_be_HTML_or_ZIP,
                     MIMEType);
   else
     {
      /***** Build path of directory containing web page *****/
      Inf_BuildPathPage (Gbl.Hierarchy.Crs.CrsCod,Gbl.Crs.Info.Type,PathRelDirHTML);

      /***** End the reception of the data *****/
      if (Str_FileIs (SourceFileName,"html") ||
          Str_FileIs (SourceFileName,"htm" )) // .html or .htm file
        {
         Fil_RemoveTree (PathRelDirHTML);
         Fil_CreateDirIfNotExists (PathRelDirHTML);
         snprintf (PathRelFileHTML,sizeof (PathRelFileHTML),"%s/index.html",
		   PathRelDirHTML);
         if (Fil_EndReceptionOfFile (PathRelFileHTML,Param))
           {
            Ale_ShowAlert (Ale_SUCCESS,Txt_The_HTML_file_has_been_received_successfully);
            FileIsOK = true;
           }
         else
            Ale_ShowAlert (Ale_ERROR,"Error uploading file.");
        }
      else if (Str_FileIs (SourceFileName,"zip")) // .zip file
        {
         Fil_RemoveTree (PathRelDirHTML);
         Fil_CreateDirIfNotExists (PathRelDirHTML);
         snprintf (PathRelFileZIP,sizeof (PathRelFileZIP),"%s/%s.zip",
                   Gbl.Crs.PathPriv,
                   Inf_FileNamesForInfoType[Gbl.Crs.Info.Type]);

         if (Fil_EndReceptionOfFile (PathRelFileZIP,Param))
           {
            Ale_ShowAlert (Ale_SUCCESS,Txt_The_ZIP_file_has_been_received_successfully);

            /* Uncompress ZIP */
            snprintf (StrUnzip,sizeof (StrUnzip),"unzip -qq -o %s -d %s",
                      PathRelFileZIP,PathRelDirHTML);
            if (system (StrUnzip) == 0)
              {
               /* Check if uploaded file is index.html or index.htm */
               snprintf (PathRelFileHTML,sizeof (PathRelFileHTML),
        	         "%s/index.html",PathRelDirHTML);
               if (Fil_CheckIfPathExists (PathRelFileHTML))
                 {
                  Ale_ShowAlert (Ale_SUCCESS,Txt_The_ZIP_file_has_been_unzipped_successfully);
                  Ale_ShowAlert (Ale_SUCCESS,Txt_Found_an_index_html_file);
                  FileIsOK = true;
                 }
	       else
	         {
	          snprintf (PathRelFileHTML,sizeof (PathRelFileHTML),
	        	    "%s/index.htm",PathRelDirHTML);
	          if (Fil_CheckIfPathExists (PathRelFileHTML))
                    {
                     Ale_ShowAlert (Ale_SUCCESS,Txt_The_ZIP_file_has_been_unzipped_successfully);
                     Ale_ShowAlert (Ale_SUCCESS,Txt_Found_an_index_htm_file);
                     FileIsOK = true;
                    }
	          else
                     Ale_ShowAlert (Ale_WARNING,Txt_No_file_index_html_index_htm_found_within_the_ZIP_file);
	         }
	      }
            else
               Err_ShowErrorAndExit ("Can not unzip file.");
           }
         else
            Ale_ShowAlert (Ale_ERROR,"Error uploading file.");
        }
      else
         Ale_ShowAlert (Ale_WARNING,Txt_The_file_type_should_be_HTML_or_ZIP);
     }

   if (FileIsOK)
     {
      /***** Change info source to page in database *****/
      Inf_DB_SetInfoSrc (Inf_PAGE);

      /***** Show the updated info *****/
      Inf_ShowInfo ();
     }
   else
     {
      /***** Change info source to none in database *****/
      Inf_DB_SetInfoSrc (Inf_NONE);

      /***** Show again the form to select and send course info *****/
      Inf_FormsToSelSendInfo ();
     }
  }

/*****************************************************************************/
/******************* Integrated editor for introduction **********************/
/*****************************************************************************/

void Inf_EditorCourseInfo (void)
  {
   extern const char *Txt_The_integrated_editor_is_not_yet_available;

   Ale_ShowAlert (Ale_WARNING,Txt_The_integrated_editor_is_not_yet_available);

   /***** Show again the form to select and send course info *****/
   Inf_FormsToSelSendInfo ();
  }

/*****************************************************************************/
/****************** Integrated editor for teaching guide *********************/
/*****************************************************************************/

void Inf_EditorTeachingGuide (void)
  {
   extern const char *Txt_The_integrated_editor_is_not_yet_available;

   Ale_ShowAlert (Ale_WARNING,Txt_The_integrated_editor_is_not_yet_available);

   /***** Show again the form to select and send course info *****/
   Inf_FormsToSelSendInfo ();
  }

/*****************************************************************************/
/******************* Integrated editor for bibliography **********************/
/*****************************************************************************/

void Inf_EditorBibliography (void)
  {
   extern const char *Txt_The_integrated_editor_is_not_yet_available;

   Ale_ShowAlert (Ale_WARNING,Txt_The_integrated_editor_is_not_yet_available);

   /***** Show again the form to select and send course info *****/
   Inf_FormsToSelSendInfo ();
  }

/*****************************************************************************/
/*********************** Integrated editor for FAQ ***************************/
/*****************************************************************************/

void Inf_EditorFAQ (void)
  {
   extern const char *Txt_The_integrated_editor_is_not_yet_available;

   Ale_ShowAlert (Ale_WARNING,Txt_The_integrated_editor_is_not_yet_available);

   /***** Show again the form to select and send course info *****/
   Inf_FormsToSelSendInfo ();
  }

/*****************************************************************************/
/********************* Integrated editor for links ***************************/
/*****************************************************************************/

void Inf_EditorLinks (void)
  {
   extern const char *Txt_The_integrated_editor_is_not_yet_available;

   Ale_ShowAlert (Ale_WARNING,Txt_The_integrated_editor_is_not_yet_available);

   /***** Show again the form to select and send course info *****/
   Inf_FormsToSelSendInfo ();
  }

/*****************************************************************************/
/****************** Integrated editor for assessment system ******************/
/*****************************************************************************/

void Inf_EditorAssessment (void)
  {
   extern const char *Txt_The_integrated_editor_is_not_yet_available;

   Ale_ShowAlert (Ale_WARNING,Txt_The_integrated_editor_is_not_yet_available);

   /***** Show again the form to select and send course info *****/
   Inf_FormsToSelSendInfo ();
  }
