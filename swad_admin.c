// swad_admin.c: administrators

/*
    SWAD (Shared Workspace At a Distance),
    is a web platform developed at the University of Granada (Spain),
    and used to support university teaching.

    This file is part of SWAD core.
    Copyright (C) 1999-2021 Antonio Ca�as Vargas

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
/*********************************** Headers *********************************/
/*****************************************************************************/

// #define _GNU_SOURCE 		// For asprintf
// #include <stdio.h>		// For asprintf
// #include <stdlib.h>		// For exit, system, malloc, free, rand, etc.
// #include <string.h>		// For string functions

// #include "swad_account.h"
// #include "swad_account_database.h"
#include "swad_admin.h"
#include "swad_admin_database.h"
// #include "swad_announcement.h"
// #include "swad_attendance_database.h"
// #include "swad_box.h"
// #include "swad_database.h"
// #include "swad_duplicate.h"
// #include "swad_enrolment.h"
// #include "swad_enrolment_database.h"
// #include "swad_error.h"
// #include "swad_exam_print.h"
// #include "swad_form.h"
#include "swad_global.h"
// #include "swad_hierarchy.h"
// #include "swad_HTML.h"
// #include "swad_ID.h"
// #include "swad_match.h"
// #include "swad_message.h"
// #include "swad_notification.h"
// #include "swad_parameter.h"
// #include "swad_photo.h"
// #include "swad_role.h"
// #include "swad_setting.h"
// #include "swad_test_print.h"
// #include "swad_user.h"

/*****************************************************************************/
/****************************** Public constants *****************************/
/*****************************************************************************/

const bool Adm_ICanAdminOtherUsrs[Rol_NUM_ROLES] =
  {
   /* Users who can not admin */
   [Rol_UNK    ] = false,
   [Rol_GST    ] = false,
   [Rol_USR    ] = false,
   [Rol_STD    ] = false,
   [Rol_NET    ] = false,
   /* Users who can admin */
   [Rol_TCH    ] = true,
   [Rol_DEG_ADM] = true,
   [Rol_CTR_ADM] = true,
   [Rol_INS_ADM] = true,
   [Rol_SYS_ADM] = true,
  };

/*****************************************************************************/
/***************************** Private constants *****************************/
/*****************************************************************************/

/*****************************************************************************/
/******************************* Private types *******************************/
/*****************************************************************************/


/*****************************************************************************/
/************** External global variables from others modules ****************/
/*****************************************************************************/

extern struct Globals Gbl;

/*****************************************************************************/
/************************** Private global variables *************************/
/*****************************************************************************/

/*****************************************************************************/
/***************************** Private prototypes ****************************/
/*****************************************************************************/

static void Adm_ReqAddAdm (HieLvl_Level_t Scope,long Cod,
                           const char *InsCtrDegName);
static void Enr_AddAdm (HieLvl_Level_t Scope,long Cod,
                        const char *InsCtrDegName);
static void Adm_RegisterAdmin (struct UsrData *UsrDat,
                               HieLvl_Level_t Scope,long Cod,
                               const char *InsCtrDegName);

static void Adm_ReqRemOrRemAdm (Enr_ReqDelOrDelUsr_t ReqDelOrDelUsr,
                                HieLvl_Level_t Scope,long Cod,
                                const char *InsCtrDegName);
static void Adm_AskIfRemAdm (bool ItsMe,HieLvl_Level_t Scope,
                             const char *InsCtrDegName);
static void Adm_EffectivelyRemAdm (struct UsrData *UsrDat,
                                   HieLvl_Level_t Scope,long Cod,
                                   const char *InsCtrDegName);

/*****************************************************************************/
/**** Ask if really wanted to add an administrator to current institution ****/
/*****************************************************************************/

void Adm_ReqAddAdmOfIns (void)
  {
   Adm_ReqAddAdm (HieLvl_INS,Gbl.Hierarchy.Ins.InsCod,Gbl.Hierarchy.Ins.FullName);
  }

/*****************************************************************************/
/****** Ask if really wanted to add an administrator to current center *******/
/*****************************************************************************/

void Adm_ReqAddAdmOfCtr (void)
  {
   Adm_ReqAddAdm (HieLvl_CTR,Gbl.Hierarchy.Ctr.CtrCod,Gbl.Hierarchy.Ctr.FullName);
  }

/*****************************************************************************/
/****** Ask if really wanted to add an administrator to current degree *******/
/*****************************************************************************/

void Adm_ReqAddAdmOfDeg (void)
  {
   Adm_ReqAddAdm (HieLvl_DEG,Gbl.Hierarchy.Deg.DegCod,Gbl.Hierarchy.Deg.FullName);
  }

/*****************************************************************************/
/**** Ask if really wanted to add an administrator to current institution ****/
/*****************************************************************************/

static void Adm_ReqAddAdm (HieLvl_Level_t Scope,long Cod,
                           const char *InsCtrDegName)
  {
   extern const char *Txt_THE_USER_X_is_already_an_administrator_of_Y;
   extern const char *Txt_Do_you_really_want_to_register_the_following_user_as_an_administrator_of_X;
   extern const char *Txt_Register_user_IN_A_COURSE_OR_DEGREE;
   static const Act_Action_t Enr_ActNewAdm[HieLvl_NUM_LEVELS] =
     {
      [HieLvl_UNK] = ActUnk,
      [HieLvl_SYS] = ActUnk,
      [HieLvl_CTY] = ActUnk,
      [HieLvl_INS] = ActNewAdmIns,
      [HieLvl_CTR] = ActNewAdmCtr,
      [HieLvl_DEG] = ActNewAdmDeg,
      [HieLvl_CRS] = ActUnk,
     };
   bool ICanRegister;

   if (Cod > 0)
     {
      /***** Get user's identificator of the user to register as admin *****/
      if (Usr_GetParamOtherUsrCodEncryptedAndGetUsrData ())
        {
         /* Check if I am allowed to register user as administrator in institution/center/degree */
	 ICanRegister = ((Scope == HieLvl_DEG && Gbl.Usrs.Me.Role.Logged >= Rol_CTR_ADM) ||
                         (Scope == HieLvl_CTR && Gbl.Usrs.Me.Role.Logged >= Rol_INS_ADM) ||
                         (Scope == HieLvl_INS && Gbl.Usrs.Me.Role.Logged == Rol_SYS_ADM));
         if (ICanRegister)
           {
            if (Usr_CheckIfUsrIsAdm (Gbl.Usrs.Other.UsrDat.UsrCod,Scope,Cod))        // User is already an administrator of current institution/center/degree
              {
               Ale_ShowAlert (Ale_INFO,Txt_THE_USER_X_is_already_an_administrator_of_Y,
                              Gbl.Usrs.Other.UsrDat.FullName,InsCtrDegName);
               Rec_ShowSharedRecordUnmodifiable (&Gbl.Usrs.Other.UsrDat);
              }
            else
              {
	       /***** Show question and button to register user as administrator *****/
	       /* Begin alert */
	       Ale_ShowAlertAndButton1 (Ale_QUESTION,Txt_Do_you_really_want_to_register_the_following_user_as_an_administrator_of_X,
                                        InsCtrDegName);

	       /* Show user's record */
               Rec_ShowSharedRecordUnmodifiable (&Gbl.Usrs.Other.UsrDat);

	       /* End alert */
	       Ale_ShowAlertAndButton2 (Enr_ActNewAdm[Scope],NULL,NULL,
	                                Usr_PutParamOtherUsrCodEncrypted,Gbl.Usrs.Other.UsrDat.EnUsrCod,
	                                Btn_CREATE_BUTTON,Txt_Register_user_IN_A_COURSE_OR_DEGREE);
              }
           }
         else
            Ale_ShowAlertUserNotFoundOrYouDoNotHavePermission ();
        }
      else
         Ale_ShowAlertUserNotFoundOrYouDoNotHavePermission ();
     }
  }

/*****************************************************************************/
/*************** Add an administrator to current institution *****************/
/*****************************************************************************/

void Adm_AddAdmToIns (void)
  {
   Enr_AddAdm (HieLvl_INS,Gbl.Hierarchy.Ins.InsCod,Gbl.Hierarchy.Ins.FullName);
  }

/*****************************************************************************/
/******************* Add an administrator to current center ******************/
/*****************************************************************************/

void Adm_AddAdmToCtr (void)
  {
   Enr_AddAdm (HieLvl_CTR,Gbl.Hierarchy.Ctr.CtrCod,Gbl.Hierarchy.Ctr.FullName);
  }

/*****************************************************************************/
/******************* Add an administrator to current degree ******************/
/*****************************************************************************/

void Adm_AddAdmToDeg (void)
  {
   Enr_AddAdm (HieLvl_DEG,Gbl.Hierarchy.Deg.DegCod,Gbl.Hierarchy.Deg.FullName);
  }

/*****************************************************************************/
/******************* Add an administrator to current degree ******************/
/*****************************************************************************/

static void Enr_AddAdm (HieLvl_Level_t Scope,long Cod,
                        const char *InsCtrDegName)
  {
   bool ICanRegister;

   if (Cod > 0)
     {
      /***** Get plain user's ID of the user to add/modify *****/
      if (Usr_GetParamOtherUsrCodEncryptedAndGetUsrData ())
        {
         /* Check if I am allowed to register user as administrator in institution/center/degree */
	 ICanRegister = ((Scope == HieLvl_DEG && Gbl.Usrs.Me.Role.Logged >= Rol_CTR_ADM) ||
                         (Scope == HieLvl_CTR && Gbl.Usrs.Me.Role.Logged >= Rol_INS_ADM) ||
                         (Scope == HieLvl_INS && Gbl.Usrs.Me.Role.Logged == Rol_SYS_ADM));
         if (ICanRegister)
           {
            /***** Register administrator in current institution/center/degree in database *****/
            Adm_RegisterAdmin (&Gbl.Usrs.Other.UsrDat,Scope,
                               Cod,InsCtrDegName);

            /***** Show user's record *****/
            Rec_ShowSharedRecordUnmodifiable (&Gbl.Usrs.Other.UsrDat);
           }
         else
            Ale_ShowAlertUserNotFoundOrYouDoNotHavePermission ();
        }
      else
         Ale_ShowAlertUserNotFoundOrYouDoNotHavePermission ();
     }
  }

/*****************************************************************************/
/**************** Register administrator in current institution **************/
/*****************************************************************************/

static void Adm_RegisterAdmin (struct UsrData *UsrDat,
                               HieLvl_Level_t Scope,long Cod,
                               const char *InsCtrDegName)
  {
   extern const char *Txt_THE_USER_X_is_already_an_administrator_of_Y;
   extern const char *Txt_THE_USER_X_has_been_enroled_as_administrator_of_Y;

   /***** Check if user was and administrator of current institution/center/degree *****/
   if (Usr_CheckIfUsrIsAdm (UsrDat->UsrCod,Scope,Cod))
      Ale_ShowAlert (Ale_SUCCESS,Txt_THE_USER_X_is_already_an_administrator_of_Y,
                     UsrDat->FullName,InsCtrDegName);
   else        // User was not administrator of current institution/center/degree
     {
      /***** Insert or replace administrator in current institution/center/degree *****/
      Adm_DB_InsertAdmin (UsrDat->UsrCod,Scope,Cod);

      Ale_ShowAlert (Ale_SUCCESS,Txt_THE_USER_X_has_been_enroled_as_administrator_of_Y,
                     UsrDat->FullName,InsCtrDegName);
     }
  }

/*****************************************************************************/
/******* Ask for remove of an administrator from current institution *********/
/*****************************************************************************/

void Adm_ReqRemAdmOfIns (void)
  {
   Adm_ReqRemOrRemAdm (Enr_REQUEST_REMOVE_USR,HieLvl_INS,
                       Gbl.Hierarchy.Ins.InsCod,Gbl.Hierarchy.Ins.FullName);
  }

/*****************************************************************************/
/********* Ask for remove of an administrator from current center ************/
/*****************************************************************************/

void Adm_ReqRemAdmOfCtr (void)
  {
   Adm_ReqRemOrRemAdm (Enr_REQUEST_REMOVE_USR,HieLvl_CTR,
                       Gbl.Hierarchy.Ctr.CtrCod,Gbl.Hierarchy.Ctr.FullName);
  }

/*****************************************************************************/
/********* Ask for remove of an administrator from current degree ************/
/*****************************************************************************/

void Adm_ReqRemAdmOfDeg (void)
  {
   Adm_ReqRemOrRemAdm (Enr_REQUEST_REMOVE_USR,HieLvl_DEG,
                       Gbl.Hierarchy.Deg.DegCod,Gbl.Hierarchy.Deg.FullName);
  }

/*****************************************************************************/
/************ Remove an administrator from current institution ***************/
/*****************************************************************************/

void Adm_RemAdmIns (void)
  {
   Adm_ReqRemOrRemAdm (Enr_REMOVE_USR,HieLvl_INS,
                       Gbl.Hierarchy.Ins.InsCod,Gbl.Hierarchy.Ins.FullName);
  }

/*****************************************************************************/
/*************** Remove an administrator from current center *****************/
/*****************************************************************************/

void Adm_RemAdmCtr (void)
  {
   Adm_ReqRemOrRemAdm (Enr_REMOVE_USR,HieLvl_CTR,
                       Gbl.Hierarchy.Ctr.CtrCod,Gbl.Hierarchy.Ctr.FullName);
  }

/*****************************************************************************/
/*************** Remove an administrator from current degree *****************/
/*****************************************************************************/

void Adm_RemAdmDeg (void)
  {
   Adm_ReqRemOrRemAdm (Enr_REMOVE_USR,HieLvl_DEG,
                       Gbl.Hierarchy.Deg.DegCod,Gbl.Hierarchy.Deg.FullName);
  }

/*****************************************************************************/
/***************** Remove an admin from current institution ******************/
/*****************************************************************************/

static void Adm_ReqRemOrRemAdm (Enr_ReqDelOrDelUsr_t ReqDelOrDelUsr,
                                HieLvl_Level_t Scope,long Cod,
                                const char *InsCtrDegName)
  {
   extern const char *Txt_THE_USER_X_is_not_an_administrator_of_Y;
   bool ItsMe;
   bool ICanRemove;

   if (Cod > 0)
     {
      /***** Get user to be removed *****/
      if (Usr_GetParamOtherUsrCodEncryptedAndGetUsrData ())
        {
         /* Check if it's forbidden to remove an administrator */
         ItsMe = Usr_ItsMe (Gbl.Usrs.Other.UsrDat.UsrCod);
         ICanRemove = (ItsMe ||
                       (Scope == HieLvl_DEG && Gbl.Usrs.Me.Role.Logged >= Rol_CTR_ADM) ||
                       (Scope == HieLvl_CTR && Gbl.Usrs.Me.Role.Logged >= Rol_INS_ADM) ||
                       (Scope == HieLvl_INS && Gbl.Usrs.Me.Role.Logged == Rol_SYS_ADM));
         if (ICanRemove)
           {
            /* Check if the other user is an admin of the current institution/center/degree */
            if (Usr_CheckIfUsrIsAdm (Gbl.Usrs.Other.UsrDat.UsrCod,Scope,Cod))
              {                // The other user is an administrator of current institution/center/degree ==> ask for removing or remove her/him
               switch (ReqDelOrDelUsr)
                 {
                  case Enr_REQUEST_REMOVE_USR:     // Ask if remove administrator from current institution
                     Adm_AskIfRemAdm (ItsMe,Scope,InsCtrDegName);
                     break;
                  case Enr_REMOVE_USR:             // Remove administrator from current institution
                     Adm_EffectivelyRemAdm (&Gbl.Usrs.Other.UsrDat,Scope,
                                            Cod,InsCtrDegName);
                     break;
                 }
              }
            else        // The other user is not an administrator of current institution
               Ale_ShowAlert (Ale_WARNING,Txt_THE_USER_X_is_not_an_administrator_of_Y,
                              Gbl.Usrs.Other.UsrDat.FullName,InsCtrDegName);
           }
         else
            Ale_ShowAlertUserNotFoundOrYouDoNotHavePermission ();
        }
      else
         Ale_ShowAlertUserNotFoundOrYouDoNotHavePermission ();
     }
  }

/*****************************************************************************/
/** Ask if really wanted to remove an administrator from current institution */
/*****************************************************************************/

static void Adm_AskIfRemAdm (bool ItsMe,HieLvl_Level_t Scope,
                             const char *InsCtrDegName)
  {
   extern const char *Txt_Do_you_really_want_to_be_removed_as_an_administrator_of_X;
   extern const char *Txt_Do_you_really_want_to_remove_the_following_user_as_an_administrator_of_X;
   extern const char *Txt_Remove_me_as_an_administrator;
   extern const char *Txt_Remove_USER_as_an_administrator;
   static const Act_Action_t Enr_ActRemAdm[HieLvl_NUM_LEVELS] =
     {
      [HieLvl_UNK] = ActUnk,
      [HieLvl_SYS] = ActUnk,
      [HieLvl_CTY] = ActUnk,
      [HieLvl_INS] = ActRemAdmIns,
      [HieLvl_CTR] = ActRemAdmCtr,
      [HieLvl_DEG] = ActRemAdmDeg,
      [HieLvl_CRS] = ActUnk,
     };

   if (Usr_ChkIfUsrCodExists (Gbl.Usrs.Other.UsrDat.UsrCod))
     {
      /***** Show question and button to remove user as administrator *****/
      /* Begin alert */
      Ale_ShowAlertAndButton1 (Ale_QUESTION,ItsMe ? Txt_Do_you_really_want_to_be_removed_as_an_administrator_of_X :
                                                    Txt_Do_you_really_want_to_remove_the_following_user_as_an_administrator_of_X,
                               InsCtrDegName);

      /* Show user's record */
      Rec_ShowSharedRecordUnmodifiable (&Gbl.Usrs.Other.UsrDat);

      /* End alert */
      Ale_ShowAlertAndButton2 (Enr_ActRemAdm[Scope],NULL,NULL,
                               Usr_PutParamOtherUsrCodEncrypted,Gbl.Usrs.Other.UsrDat.EnUsrCod,
                               Btn_REMOVE_BUTTON,
                               ItsMe ? Txt_Remove_me_as_an_administrator :
                                       Txt_Remove_USER_as_an_administrator);
     }
   else
      Ale_ShowAlertUserNotFoundOrYouDoNotHavePermission ();
  }

/*****************************************************************************/
/**** Remove an administrator from current institution, center or degree *****/
/*****************************************************************************/

static void Adm_EffectivelyRemAdm (struct UsrData *UsrDat,
                                   HieLvl_Level_t Scope,long Cod,
                                   const char *InsCtrDegName)
  {
   extern const char *Txt_THE_USER_X_has_been_removed_as_administrator_of_Y;
   extern const char *Txt_THE_USER_X_is_not_an_administrator_of_Y;

   if (Usr_CheckIfUsrIsAdm (UsrDat->UsrCod,Scope,Cod))        // User is administrator of current institution/center/degree
     {
      /***** Remove user as administrator of institution, center or degree *****/
      Adm_DB_RemAdmin (UsrDat->UsrCod,Scope,Cod);

      Ale_ShowAlert (Ale_SUCCESS,Txt_THE_USER_X_has_been_removed_as_administrator_of_Y,
                     UsrDat->FullName,InsCtrDegName);
     }
   else        // User is not an administrator of the current institution/center/degree
      Ale_ShowAlert (Ale_ERROR,Txt_THE_USER_X_is_not_an_administrator_of_Y,
                     UsrDat->FullName,InsCtrDegName);
  }
