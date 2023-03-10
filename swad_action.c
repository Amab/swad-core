// swad_action.c: actions

/*
    SWAD (Shared Workspace At a Distance),
    is a web platform developed at the University of Granada (Spain),
    and used to support university teaching.
    This file is part of SWAD core.
    Copyright (C) 1999-2023 Antonio Ca�as Vargas
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
/*********************************** Headers *********************************/
/*****************************************************************************/

#include <stddef.h>		// For NULL
#include <string.h>		// For string functions

#include "swad_action.h"
#include "swad_action_list.h"
#include "swad_global.h"
#include "swad_group_database.h"
#include "swad_photo.h"
#include "swad_role.h"
#include "swad_test_config.h"

/*****************************************************************************/
/************** External global variables from others modules ****************/
/*****************************************************************************/

extern struct Globals Gbl;
extern const struct Act_Actions ActLst_Actions[ActLst_NUM_ACTIONS];

/*****************************************************************************/
/****************** Get action from permanent action code ********************/
/*****************************************************************************/

Act_Action_t Act_GetActionFromActCod (long ActCod)
  {
   extern Act_Action_t ActLst_FromActCodToAction[1 + ActLst_MAX_ACTION_COD];

   if (ActCod >= 0 && ActCod <= ActLst_MAX_ACTION_COD)
      return ActLst_FromActCodToAction[ActCod];

   return ActUnk;
  }

/*****************************************************************************/
/****************** Get permanent action code from action ********************/
/*****************************************************************************/

long Act_GetActCod (Act_Action_t Action)
  {
   if (Action < 0 || Action >= ActLst_NUM_ACTIONS)
      return -1L;

   return ActLst_Actions[Action].ActCod;
  }

/*****************************************************************************/
/***************** Get index in menu associated to an action ******************/
/*****************************************************************************/

signed int Act_GetIndexInMenu (Act_Action_t Action)
  {
   return ActLst_Actions[Act_GetSuperAction (Action)].IndexInMenu;
  }

/*****************************************************************************/
/********************* Get tab associated to an action ***********************/
/*****************************************************************************/

Tab_Tab_t Act_GetTab (Act_Action_t Action)
  {
   return ActLst_Actions[Act_GetSuperAction (Action)].Tab;
  }

/*****************************************************************************/
/***************** Get superaction associated to an action *******************/
/*****************************************************************************/

Act_Action_t Act_GetSuperAction (Act_Action_t Action)
  {
   if (Action < 0 || Action >= ActLst_NUM_ACTIONS)
      return ActUnk;

   return ActLst_Actions[Action].SuperAction;
  }

/*****************************************************************************/
/************* Check if I have permission to execute an action ***************/
/*****************************************************************************/

bool Act_CheckIfIHavePermissionToExecuteAction (Act_Action_t Action)
  {
   unsigned Permission;

   if (Action < 0 || Action >= ActLst_NUM_ACTIONS)
      return false;

   switch (Gbl.Hierarchy.Level)
     {
      case HieLvl_SYS:	// System
         Permission = ActLst_Actions[Action].PermissionSys;
	 break;
      case HieLvl_CTY:	// Country selected
         Permission = ActLst_Actions[Action].PermissionCty;
	 break;
      case HieLvl_INS:	// Institution selected
         Permission = ActLst_Actions[Action].PermissionIns;
	 break;
      case HieLvl_CTR:	// Center selected
         Permission = ActLst_Actions[Action].PermissionCtr;
	 break;
      case HieLvl_DEG:	// Degree selected
         Permission = ActLst_Actions[Action].PermissionDeg;
	 break;
      case HieLvl_CRS:	// Course selected
	 Permission = Gbl.Usrs.Me.IBelongToCurrentCrs ? ActLst_Actions[Action].PermissionCrsIfIBelong :
							ActLst_Actions[Action].PermissionCrsIfIDontBelong;
	 break;
      default:
	 return false;
     }

   return ((Permission & (1 << Gbl.Usrs.Me.Role.Logged)) != 0);
  }

/*****************************************************************************/
/***************** Get content type associated to an action ******************/
/*****************************************************************************/

Act_Content_t Act_GetContentType (Act_Action_t Action)
  {
   if (Action < 0 || Action >= ActLst_NUM_ACTIONS)
      return Act_CONT_NORM;

   return ActLst_Actions[Action].ContentType;
  }

/*****************************************************************************/
/****************** Get browser tab associated to an action ******************/
/*****************************************************************************/

Act_BrowserTab_t Act_GetBrowserTab (Act_Action_t Action)
  {
   if (Action < 0 || Action >= ActLst_NUM_ACTIONS)
      return Act_BRW_UNK_TAB;

   return ActLst_Actions[Action].BrowserTab;
  }

/*****************************************************************************/
/********* Get pointer to function a priori associated to an action **********/
/*****************************************************************************/

void (*Act_GetFunctionPriori (Act_Action_t Action)) (void)
  {
   if (Action < 0 || Action >= ActLst_NUM_ACTIONS)
      return NULL;

   return ActLst_Actions[Action].FunctionPriori;
  }

/*****************************************************************************/
/******* Get pointer to function a posteriori associated to an action ********/
/*****************************************************************************/

void (*Act_GetFunctionPosteriori (Act_Action_t Action)) (void)
  {
   if (Action < 0 || Action >= ActLst_NUM_ACTIONS)
      return NULL;

   return ActLst_Actions[Action].FunctionPosteriori;
  }

/*****************************************************************************/
/********************** Get icon associated to an action *********************/
/*****************************************************************************/

const char *Act_GetIcon (Act_Action_t Action)
  {
   if (Action < 0 || Action >= ActLst_NUM_ACTIONS)
      return NULL;

   return Ico_GetIcon (ActLst_Actions[Action].Icon);
  }

/*****************************************************************************/
/******************* Get the title associated to an action *******************/
/*****************************************************************************/

const char *Act_GetTitleAction (Act_Action_t Action)
  {
   extern const char *Txt_MENU_TITLE[Tab_NUM_TABS][Act_MAX_OPTIONS_IN_MENU_PER_TAB];

   if (Action < 0 || Action >= ActLst_NUM_ACTIONS)
      return NULL;

   return Txt_MENU_TITLE[Act_GetTab (Action)][Act_GetIndexInMenu (Action)];
  }

/*****************************************************************************/
/********************* Get text for action from database *********************/
/*****************************************************************************/

const char *Act_GetActionText (Act_Action_t Action)
  {
   extern const char *Txt_Actions[ActLst_NUM_ACTIONS];

   if (Action >= 0 && Action < ActLst_NUM_ACTIONS)
      if (Txt_Actions[Action])
	 if (Txt_Actions[Action][0])
	    return Txt_Actions[Action];

   return "?";
  }

/*****************************************************************************/
/***************** Adjust current action when no user's logged ***************/
/*****************************************************************************/

void Act_AdjustActionWhenNoUsrLogged (void)
  {
   static const Act_Action_t Actions[HieLvl_NUM_LEVELS] =
     {
      [HieLvl_UNK] = ActUnk, 		// Unknown
      [HieLvl_SYS] = ActFrmLogIn,	// System
      [HieLvl_CTY] = ActSeeCtyInf,	// Country
      [HieLvl_INS] = ActSeeInsInf,	// Institution
      [HieLvl_CTR] = ActSeeCtrInf,	// Center
      [HieLvl_DEG] = ActSeeDegInf,	// Degree
      [HieLvl_CRS] = ActSeeCrsInf,	// Course
     };

   if (Gbl.Hierarchy.Level >= HieLvl_NUM_LEVELS)
      Gbl.Hierarchy.Level = ActUnk;

   Gbl.Action.Act = Actions[Gbl.Hierarchy.Level];
   Tab_SetCurrentTab ();
  }

/*****************************************************************************/
/****** Adjust current action if it's mandatory to fill any empty data *******/
/*****************************************************************************/

void Act_AdjustCurrentAction (void)
  {
   bool IAmATeacherInAnyCrs;
   bool JustAfterLogin = Gbl.Action.Act == ActLogIn    ||
	                 Gbl.Action.Act == ActLogInLan ||
	                 Gbl.Action.Act == ActLogInNew ||
			 Gbl.Action.Act == ActAnnSee;

   /***** Don't adjust anything when:
          - refreshing
          - web service *****/
   if (Gbl.Action.UsesAJAX ||
       Gbl.WebService.IsWebService)
      return;

   /***** Don't adjust anything when current action is not a menu option
          (except some actions just after login) *****/
   if (!JustAfterLogin &&					// Not just after login
       Gbl.Action.Act != Act_GetSuperAction (Gbl.Action.Act))	// It is not a menu option
      return;

   /***** Adjustment 1:
          -------------
          If I have no password in database,
          the only action possible
          is to show a form to change my account *****/
   if (!Gbl.Usrs.Me.UsrDat.Password[0])
     {
      Gbl.Action.Act = ActFrmMyAcc;
      Tab_SetCurrentTab ();
      return;
     }

   /***** Adjustment 2:
          -------------
          Just after login, check if my password seems good.
          If my password is not enough secure,
          the only action possible
          is to show a form to change my account *****/
   if (Gbl.Action.Act == ActLogIn)
      if (!Pwd_FastCheckIfPasswordSeemsGood (Gbl.Usrs.Me.LoginPlainPassword))
        {
         Gbl.Action.Act = ActFrmMyAcc;
         Tab_SetCurrentTab ();
         return;
        }

   /***** Adjustment 3:
          -------------
          If I have no nickname or email in database,
          the only action possible
          is to show a form to change my account *****/
   if (!Gbl.Usrs.Me.UsrDat.Nickname[0] ||
       !Gbl.Usrs.Me.UsrDat.Email[0])
     {
      Gbl.Action.Act = ActFrmMyAcc;
      Tab_SetCurrentTab ();
      return;
     }

   /***** Check if I am a teacher in any course *****/
   Rol_GetRolesInAllCrss (&Gbl.Usrs.Me.UsrDat);
   IAmATeacherInAnyCrs = (Gbl.Usrs.Me.UsrDat.Roles.InCrss & ((1 << Rol_NET) |	// I am a non-editing teacher...
	                                                     (1 << Rol_TCH)));	// ...or a teacher in any course

   /***** Adjustment 4:
          -------------
          If I haven't filled
             - my sex
             - my first name
             - my surname 1
             - my country
             - my institution
          or if I'm a teacher and I haven't filled
             - my center
             - my department,
          the only action possible
          is to show a form to change my shared record card *****/
   if ( Gbl.Usrs.Me.UsrDat.Sex == Usr_SEX_UNKNOWN ||
       !Gbl.Usrs.Me.UsrDat.FrstName[0]           ||
       !Gbl.Usrs.Me.UsrDat.Surname1 [0]           ||
        Gbl.Usrs.Me.UsrDat.CtyCod <= 0            ||
	Gbl.Usrs.Me.UsrDat.InsCod < 0             ||
       (IAmATeacherInAnyCrs && (Gbl.Usrs.Me.UsrDat.Tch.CtrCod < 0 ||
                                Gbl.Usrs.Me.UsrDat.Tch.DptCod < 0)))
     {
      Gbl.Action.Act = ActReqEdiRecSha;
      Tab_SetCurrentTab ();
      return;
     }

   /***** Adjustment 5:
          -------------
          If any of my settings about privacy is unknown
          the only action possible
          is to show a form to change my settings *****/
   if (Gbl.Usrs.Me.UsrDat.PhotoVisibility == Pri_VISIBILITY_UNKNOWN ||
       Gbl.Usrs.Me.UsrDat.BaPrfVisibility == Pri_VISIBILITY_UNKNOWN ||
       Gbl.Usrs.Me.UsrDat.ExPrfVisibility == Pri_VISIBILITY_UNKNOWN)
     {
      Gbl.Action.Act = ActReqEdiSet;
      Tab_SetCurrentTab ();
      return;
     }

   /***** If I belong to current course *****/
   if (Gbl.Usrs.Me.IBelongToCurrentCrs)
     {
      /***** Adjustment 6:
             -------------
             If I have not accepted my enrolment in the current course,
             the only action possible
             is to show a form to ask for enrolment *****/
      if (!Gbl.Usrs.Me.UsrDat.Accepted)
	{
	 switch (Gbl.Usrs.Me.UsrDat.Roles.InCurrentCrs)
	   {
	    case Rol_STD:
	       Gbl.Action.Act = ActReqAccEnrStd;
	       break;
	    case Rol_NET:
	       Gbl.Action.Act = ActReqAccEnrNET;
	       break;
	    case Rol_TCH:
	       Gbl.Action.Act = ActReqAccEnrTch;
	       break;
	    default:
	       break;
	   }
	 Tab_SetCurrentTab ();
	 return;
	}

      /***** Depending on the role I am logged in the current course... *****/
      switch (Gbl.Usrs.Me.Role.Logged)
        {
         case Rol_STD:
            /***** Adjustment 7:
		   -------------
		   If there are some group types
		   with available groups in which I must register,
		   the only action possible
		   is show a form to register in groups *****/
            if (JustAfterLogin)				// Only after login because the following query may be slow
	       if (Grp_DB_CheckIfAvailableGrpTyp (-1L))	// This query may be slow
		 {
		  Gbl.Action.Act = ActReqSelGrp;
		  Tab_SetCurrentTab ();
		  return;
		 }

	    /***** Adjustment 8:
		   -------------
		   If I have no photo
		   and current action is not available for unknown users,
		   then update number of clicks without photo.
		   If limit of clicks is reached,
		   the only action possible
		   is to show a form to send my photo *****/
	    if (!Gbl.Usrs.Me.MyPhotoExists)
	       if (!(ActLst_Actions[Gbl.Action.Act].PermissionCrsIfIBelong & (1 << Rol_UNK)))
		  if ((Gbl.Usrs.Me.NumAccWithoutPhoto =
		       Pho_UpdateMyClicksWithoutPhoto ()) > Pho_MAX_CLICKS_WITHOUT_PHOTO)
		    {
		     /* If limit of clicks has been reached,
			the only action possible is show a form to send my photo */
		     Gbl.Action.Act = ActReqMyPho;
		     Tab_SetCurrentTab ();
		     return;
		    }

            /***** Check if it is mandatory to read any information about course *****/
            if (Gbl.Action.Act == ActMnu)	// Do the following check sometimes, for example when the user changes the current tab
               Gbl.Crs.Info.ShowMsgMustBeRead = Inf_GetIfIMustReadAnyCrsInfoInThisCrs ();
            break;
         case Rol_NET:
            break;
         case Rol_TCH:
	    /***** Adjustment 9:
		   -------------
		   If current course has tests and pluggable is unknown,
		   the only action possible is configure tests *****/
	    if (TstCfg_CheckIfPluggableIsUnknownAndCrsHasTests ())
	      {
	       Gbl.Action.Act = ActCfgTst;
	       Tab_SetCurrentTab ();
	       return;
	      }
            break;
         default:
            break;
        }
     }

   /***** Adjustment 10:
          --------------
          Just after login with all checks OK ==> go to default action *****/
   if (JustAfterLogin)
     {
      Gbl.Action.Act = ActLst_DEFAULT_ACTION_AFTER_LOGIN;
      Tab_SetCurrentTab ();
     }
  }
