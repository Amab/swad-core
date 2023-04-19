// swad_admin.c: administrators

/*
    SWAD (Shared Workspace At a Distance),
    is a web platform developed at the University of Granada (Spain),
    and used to support university teaching.

    This file is part of SWAD core.
    Copyright (C) 1999-2023 Antonio Ca�as Vargas

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

#include <stdlib.h>		// For free

#include "swad_action_list.h"
#include "swad_admin.h"
#include "swad_admin_database.h"
#include "swad_alert.h"
#include "swad_global.h"
#include "swad_user_database.h"

/*****************************************************************************/
/****************************** Public constants *****************************/
/*****************************************************************************/

const bool Adm_ICanAdminOtherUsrs[Rol_NUM_ROLES] =
  {
   /* Users who can admin */
   [Rol_TCH    ] = true,
   [Rol_DEG_ADM] = true,
   [Rol_CTR_ADM] = true,
   [Rol_INS_ADM] = true,
   [Rol_SYS_ADM] = true,
  };

/*****************************************************************************/
/************** External global variables from others modules ****************/
/*****************************************************************************/

extern struct Globals Gbl;

/*****************************************************************************/
/***************************** Private prototypes ****************************/
/*****************************************************************************/

static void Adm_ReqAddAdm (HieLvl_Level_t Scope,long Cod,
                           const char *InsCtrDegName);
static void Enr_AddAdm (HieLvl_Level_t Scope,long Cod,
                        const char *InsCtrDegName);
static void Adm_RegisterAdmin (struct Usr_Data *UsrDat,
                               HieLvl_Level_t Scope,long Cod,
                               const char *InsCtrDegName);

static void Adm_ReqRemOrRemAdm (Enr_ReqDelOrDelUsr_t ReqDelOrDelUsr,
                                HieLvl_Level_t Scope,long Cod,
                                const char *InsCtrDegName);
static void Adm_AskIfRemAdm (Usr_MeOrOther_t MeOrOther,HieLvl_Level_t Scope,
                             const char *InsCtrDegName);
static void Adm_EffectivelyRemAdm (struct Usr_Data *UsrDat,
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
      if (Usr_GetParOtherUsrCodEncryptedAndGetUsrData ())
        {
         /* Check if I am allowed to register user as administrator in institution/center/degree */
	 ICanRegister = ((Scope == HieLvl_DEG && Gbl.Usrs.Me.Role.Logged >= Rol_CTR_ADM) ||
                         (Scope == HieLvl_CTR && Gbl.Usrs.Me.Role.Logged >= Rol_INS_ADM) ||
                         (Scope == HieLvl_INS && Gbl.Usrs.Me.Role.Logged == Rol_SYS_ADM));
         if (ICanRegister)
           {
            if (Adm_DB_CheckIfUsrIsAdm (Gbl.Usrs.Other.UsrDat.UsrCod,Scope,Cod))        // User is already an administrator of current institution/center/degree
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
	                                Usr_PutParOtherUsrCodEncrypted,Gbl.Usrs.Other.UsrDat.EnUsrCod,
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
/******************** Get list with data of administrators *******************/
/*****************************************************************************/

void Adm_GetAdmsLst (HieLvl_Level_t Scope)
  {
   char *Query = NULL;

   /***** Build query *****/
   Usr_DB_BuildQueryToGetAdmsLst (Scope,&Query);

   /***** Get list of administrators from database *****/
   Usr_GetListUsrsFromQuery (Query,Rol_DEG_ADM,Scope);

   /***** Free query string *****/
   free (Query);
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
      if (Usr_GetParOtherUsrCodEncryptedAndGetUsrData ())
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

static void Adm_RegisterAdmin (struct Usr_Data *UsrDat,
                               HieLvl_Level_t Scope,long Cod,
                               const char *InsCtrDegName)
  {
   extern const char *Txt_THE_USER_X_is_already_an_administrator_of_Y;
   extern const char *Txt_THE_USER_X_has_been_enroled_as_administrator_of_Y;

   /***** Check if user was and administrator of current institution/center/degree *****/
   if (Adm_DB_CheckIfUsrIsAdm (UsrDat->UsrCod,Scope,Cod))
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
   Usr_MeOrOther_t MeOrOther;
   bool ICanRemove;

   if (Cod > 0)
     {
      /***** Get user to be removed *****/
      if (Usr_GetParOtherUsrCodEncryptedAndGetUsrData ())
        {
         /* Check if it's forbidden to remove an administrator */
         MeOrOther = Usr_ItsMe (Gbl.Usrs.Other.UsrDat.UsrCod);
         ICanRemove = (MeOrOther == Usr_ME ||
                       (Scope == HieLvl_DEG && Gbl.Usrs.Me.Role.Logged >= Rol_CTR_ADM) ||
                       (Scope == HieLvl_CTR && Gbl.Usrs.Me.Role.Logged >= Rol_INS_ADM) ||
                       (Scope == HieLvl_INS && Gbl.Usrs.Me.Role.Logged == Rol_SYS_ADM));
         if (ICanRemove)
           {
            /* Check if the other user is an admin of the current institution/center/degree */
            if (Adm_DB_CheckIfUsrIsAdm (Gbl.Usrs.Other.UsrDat.UsrCod,Scope,Cod))
              {                // The other user is an administrator of current institution/center/degree ==> ask for removing or remove her/him
               switch (ReqDelOrDelUsr)
                 {
                  case Enr_REQUEST_REMOVE_USR:     // Ask if remove administrator from current institution
                     Adm_AskIfRemAdm (MeOrOther,Scope,InsCtrDegName);
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

static void Adm_AskIfRemAdm (Usr_MeOrOther_t MeOrOther,HieLvl_Level_t Scope,
                             const char *InsCtrDegName)
  {
   extern const char *Txt_Do_you_really_want_to_be_removed_as_an_administrator_of_X;
   extern const char *Txt_Do_you_really_want_to_remove_the_following_user_as_an_administrator_of_X;
   extern const char *Txt_Remove_me_as_an_administrator;
   extern const char *Txt_Remove_USER_as_an_administrator;
   static const Act_Action_t ActRemAdm[HieLvl_NUM_LEVELS] =
     {
      [HieLvl_UNK] = ActUnk,
      [HieLvl_SYS] = ActUnk,
      [HieLvl_CTY] = ActUnk,
      [HieLvl_INS] = ActRemAdmIns,
      [HieLvl_CTR] = ActRemAdmCtr,
      [HieLvl_DEG] = ActRemAdmDeg,
      [HieLvl_CRS] = ActUnk,
     };
   const char *Question[Usr_NUM_ME_OR_OTHER] =
     {
      [Usr_ME   ] = Txt_Do_you_really_want_to_be_removed_as_an_administrator_of_X,
      [Usr_OTHER] = Txt_Do_you_really_want_to_remove_the_following_user_as_an_administrator_of_X,
     };
   const char *TxtButton[Usr_NUM_ME_OR_OTHER] =
     {
      [Usr_ME   ] = Txt_Remove_me_as_an_administrator,
      [Usr_OTHER] = Txt_Remove_USER_as_an_administrator,
     };

   if (Usr_DB_ChkIfUsrCodExists (Gbl.Usrs.Other.UsrDat.UsrCod))
     {
      /***** Show question and button to remove user as administrator *****/
      /* Begin alert */
      Ale_ShowAlertAndButton1 (Ale_QUESTION,Question[MeOrOther],InsCtrDegName);

      /* Show user's record */
      Rec_ShowSharedRecordUnmodifiable (&Gbl.Usrs.Other.UsrDat);

      /* End alert */
      Ale_ShowAlertAndButton2 (ActRemAdm[Scope],NULL,NULL,
                               Usr_PutParOtherUsrCodEncrypted,Gbl.Usrs.Other.UsrDat.EnUsrCod,
                               Btn_REMOVE_BUTTON,TxtButton[MeOrOther]);
     }
   else
      Ale_ShowAlertUserNotFoundOrYouDoNotHavePermission ();
  }

/*****************************************************************************/
/**** Remove an administrator from current institution, center or degree *****/
/*****************************************************************************/

static void Adm_EffectivelyRemAdm (struct Usr_Data *UsrDat,
                                   HieLvl_Level_t Scope,long Cod,
                                   const char *InsCtrDegName)
  {
   extern const char *Txt_THE_USER_X_has_been_removed_as_administrator_of_Y;
   extern const char *Txt_THE_USER_X_is_not_an_administrator_of_Y;

   if (Adm_DB_CheckIfUsrIsAdm (UsrDat->UsrCod,Scope,Cod))        // User is administrator of current institution/center/degree
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
