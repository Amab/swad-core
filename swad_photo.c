// swad_photo.c: Users' photos management

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
#include <linux/limits.h>	// For PATH_MAX
#include <math.h>		// For log10, floor, ceil, modf, sqrt...
#include <stddef.h>		// For NULL
#include <stdio.h>		// For asprintf
#include <stdlib.h>		// For system, getenv, etc.
#include <string.h>		// For string functions
#include <sys/wait.h>		// For the macro WEXITSTATUS
#include <unistd.h>		// For unlink

#include "swad_action.h"
#include "swad_box.h"
#include "swad_browser.h"
#include "swad_config.h"
#include "swad_database.h"
#include "swad_enrolment.h"
#include "swad_error.h"
#include "swad_file.h"
#include "swad_follow.h"
#include "swad_form.h"
#include "swad_global.h"
#include "swad_hierarchy.h"
#include "swad_hierarchy_level.h"
#include "swad_HTML.h"
#include "swad_logo.h"
#include "swad_parameter.h"
#include "swad_photo.h"
#include "swad_photo_database.h"
#include "swad_privacy.h"
#include "swad_setting.h"
#include "swad_setting_database.h"
#include "swad_statistic.h"
#include "swad_theme.h"
#include "swad_user.h"

/*****************************************************************************/
/************** External global variables from others modules ****************/
/*****************************************************************************/

extern struct Globals Gbl;

/*****************************************************************************/
/***************************** Private constants *****************************/
/*****************************************************************************/

static const char *Pho_StrAvgPhotoDirs[Pho_NUM_AVERAGE_PHOTO_TYPES] =
  {
   Cfg_FOLDER_DEGREE_PHOTO_MEDIAN,
   Cfg_FOLDER_DEGREE_PHOTO_AVERAGE,
  };
static const char *Pho_StrAvgPhotoPrograms[Pho_NUM_AVERAGE_PHOTO_TYPES] =
  {
   Cfg_COMMAND_DEGREE_PHOTO_MEDIAN,
   Cfg_COMMAND_DEGREE_PHOTO_AVERAGE,
  };

/*****************************************************************************/
/***************************** Private prototypes ****************************/
/*****************************************************************************/

static void Pho_PutIconToRequestRemoveMyPhoto (__attribute__((unused)) void *Args);
static void Pho_PutIconToRequestRemoveOtherUsrPhoto (__attribute__((unused)) void *Args);
static void Pho_ReqOtherUsrPhoto (void);

static void Pho_ReqPhoto (const struct UsrData *UsrDat);

static bool Pho_ReceivePhotoAndDetectFaces (bool ItsMe,const struct UsrData *UsrDat);

static void Pho_UpdatePhoto1 (struct UsrData *UsrDat);
static void Pho_UpdatePhoto2 (void);

static long Pho_GetDegWithAvgPhotoLeastRecentlyUpdated (void);
static long Pho_GetTimeAvgPhotoWasComputed (long DegCod);
static long Pho_GetTimeToComputeAvgPhoto (long DegCod);
static void Pho_ComputeAveragePhoto (long DegCod,Usr_Sex_t Sex,Rol_Role_t Role,
                                     Pho_AvgPhotoTypeOfAverage_t TypeOfAverage,const char *DirAvgPhotosRelPath,
                                     unsigned *NumStds,unsigned *NumStdsWithPhoto,long *TimeToComputeAvgPhotoInMicroseconds);
static void Pho_ShowOrPrintPhotoDegree (Pho_AvgPhotoSeeOrPrint_t SeeOrPrint);
static void Pho_PutParamsDegPhoto (void *DegPhotos);
static void Pho_PutSelectorForTypeOfAvg (const struct Pho_DegPhotos *DegPhotos);
static Pho_AvgPhotoTypeOfAverage_t Pho_GetPhotoAvgTypeFromForm (void);
static void Pho_PutSelectorForHowComputePhotoSize (const struct Pho_DegPhotos *DegPhotos);
static Pho_HowComputePhotoSize_t Pho_GetHowComputePhotoSizeFromForm (void);
static void Pho_PutSelectorForHowOrderDegrees (const struct Pho_DegPhotos *DegPhotos);
static Pho_HowOrderDegrees_t Pho_GetHowOrderDegreesFromForm (void);

static void Pho_PutIconToPrintDegreeStats (void *DegPhotos);
static void Pho_PutLinkToPrintViewOfDegreeStatsParams (void *DegPhotos);

static void Pho_PutLinkToCalculateDegreeStats (const struct Pho_DegPhotos *DegPhotos);
static void Pho_GetMaxStdsPerDegree (struct Pho_DegPhotos *DegPhotos);
static void Pho_ShowOrPrintClassPhotoDegrees (struct Pho_DegPhotos *DegPhotos,
                                              Pho_AvgPhotoSeeOrPrint_t SeeOrPrint);
static void Pho_GetNumStdsInDegree (long DegCod,Usr_Sex_t Sex,
                                    int *NumStds,int *NumStdsWithPhoto);
static void Pho_ShowOrPrintListDegrees (struct Pho_DegPhotos *DegPhotos,
                                        Pho_AvgPhotoSeeOrPrint_t SeeOrPrint);

static void Pho_ShowDegreeStat (int NumStds,int NumStdsWithPhoto);
static void Pho_ShowDegreeAvgPhotoAndStat (const struct Deg_Degree *Deg,
                                           const struct Pho_DegPhotos *DegPhotos,
                                           Pho_AvgPhotoSeeOrPrint_t SeeOrPrint,
                                           Usr_Sex_t Sex,
                                           int NumStds,int NumStdsWithPhoto);
static void Pho_ComputePhotoSize (const struct Pho_DegPhotos *DegPhotos,
                                  int NumStds,int NumStdsWithPhoto,
                                  unsigned *PhotoWidth,unsigned *PhotoHeight);

/*****************************************************************************/
/************** Check if I can change the photo of another user **************/
/*****************************************************************************/

bool Pho_ICanChangeOtherUsrPhoto (struct UsrData *UsrDat)
  {
   /***** I can change my photo *****/
   if (Usr_ItsMe (UsrDat->UsrCod))
      return true;

   /***** Check if I have permission to change user's photo *****/
   switch (Gbl.Usrs.Me.Role.Logged)
     {
      case Rol_TCH:
	 /* A teacher can change the photo of confirmed students */
	 if (UsrDat->Roles.InCurrentCrs != Rol_STD)	// Not a student
            return false;

	 /* It's a student in this course,
	    check if he/she has accepted registration */
         return (UsrDat->Accepted = Enr_CheckIfUsrHasAcceptedInCurrentCrs (UsrDat));
      case Rol_DEG_ADM:
      case Rol_CTR_ADM:
      case Rol_INS_ADM:
      case Rol_SYS_ADM:
         return Usr_ICanEditOtherUsr (UsrDat);
      default:
	 return false;
     }
  }

/*****************************************************************************/
/********** Put a link to the action used to request user's photo ************/
/*****************************************************************************/

void Pho_PutIconToChangeUsrPhoto (void)
  {
   extern const char *Txt_Change_photo;
   extern const char *Txt_Upload_photo;
   bool PhotoExists;
   char PhotoURL[PATH_MAX + 1];
   const char *TitleText;
   static const Act_Action_t NextAction[Rol_NUM_ROLES] =
     {
      [Rol_UNK	  ] = ActReqOthPho,
      [Rol_GST	  ] = ActReqOthPho,
      [Rol_USR	  ] = ActReqOthPho,
      [Rol_STD	  ] = ActReqStdPho,
      [Rol_NET	  ] = ActReqTchPho,
      [Rol_TCH	  ] = ActReqTchPho,
      [Rol_DEG_ADM] = ActReqOthPho,
      [Rol_CTR_ADM] = ActReqOthPho,
      [Rol_INS_ADM] = ActReqOthPho,
      [Rol_SYS_ADM] = ActReqOthPho,
     };

   /***** Link for changing / uploading the photo *****/
   if (Usr_ItsMe (Gbl.Record.UsrDat->UsrCod))
     {
      TitleText = Gbl.Usrs.Me.MyPhotoExists ? Txt_Change_photo :
			                      Txt_Upload_photo;
      Lay_PutContextualLinkOnlyIcon (ActReqMyPho,NULL,
                                     NULL,NULL,
				     "camera.svg",Ico_BLACK,
				     TitleText);
     }
   else	// Not me
      if (Pho_ICanChangeOtherUsrPhoto (Gbl.Record.UsrDat))
	{
	 PhotoExists = Pho_BuildLinkToPhoto (Gbl.Record.UsrDat,PhotoURL);
	 TitleText = PhotoExists ? Txt_Change_photo :
				   Txt_Upload_photo;
	 Lay_PutContextualLinkOnlyIcon (NextAction[Gbl.Record.UsrDat->Roles.InCurrentCrs],NULL,
				        Rec_PutParamUsrCodEncrypted,NULL,
	                                "camera.svg",Ico_BLACK,
				        TitleText);
	}
  }

/*****************************************************************************/
/************** Put a link to request the removal of my photo ****************/
/*****************************************************************************/

static void Pho_PutIconToRequestRemoveMyPhoto (__attribute__((unused)) void *Args)
  {
   extern const char *Txt_Remove_photo;

   /***** Link to request the removal of my photo *****/
   if (Gbl.Usrs.Me.MyPhotoExists)
      Lay_PutContextualLinkOnlyIcon (ActReqRemMyPho,NULL,
				     NULL,NULL,
				     "trash.svg",Ico_RED,
				     Txt_Remove_photo);
  }

/*****************************************************************************/
/********** Put a link to request the removal of a user's photo **************/
/*****************************************************************************/

static void Pho_PutIconToRequestRemoveOtherUsrPhoto (__attribute__((unused)) void *Args)
  {
   extern const char *Txt_Remove_photo;
   char PhotoURL[PATH_MAX + 1];
   bool PhotoExists;
   static const Act_Action_t NextAction[Rol_NUM_ROLES] =
     {
      [Rol_UNK	  ] = ActReqRemOthPho,
      [Rol_GST	  ] = ActReqRemOthPho,
      [Rol_USR	  ] = ActReqRemOthPho,
      [Rol_STD	  ] = ActReqRemStdPho,
      [Rol_NET	  ] = ActReqRemTchPho,
      [Rol_TCH	  ] = ActReqRemTchPho,
      [Rol_DEG_ADM] = ActReqRemOthPho,
      [Rol_CTR_ADM] = ActReqRemOthPho,
      [Rol_INS_ADM] = ActReqRemOthPho,
      [Rol_SYS_ADM] = ActReqRemOthPho,
     };

   /***** Link to request the removal of another user's photo *****/
   PhotoExists = Pho_BuildLinkToPhoto (&Gbl.Usrs.Other.UsrDat,PhotoURL);
   if (PhotoExists)
      Lay_PutContextualLinkOnlyIcon (NextAction[Gbl.Usrs.Other.UsrDat.Roles.InCurrentCrs],NULL,
				     Usr_PutParamOtherUsrCodEncrypted,Gbl.Usrs.Other.UsrDat.EnUsrCod,
				     "trash.svg",Ico_RED,
				     Txt_Remove_photo);
  }

/*****************************************************************************/
/************************ Form for sending my photo **************************/
/*****************************************************************************/

void Pho_ReqMyPhoto (void)
  {
   /***** Show the form for sending the photo *****/
   Pho_ReqPhoto (&Gbl.Usrs.Me.UsrDat);

   /***** Show my record and other data *****/
   Rec_ShowMySharedRecordAndMore ();
  }

/*****************************************************************************/
/******************* Form for sending other user's photo *********************/
/*****************************************************************************/

static void Pho_ReqOtherUsrPhoto (void)
  {
   /***** Show the form to send another user's photo *****/
   Pho_ReqPhoto (&Gbl.Usrs.Other.UsrDat);

   /***** Show another user's record card *****/
   Rec_ShowPublicSharedRecordOtherUsr ();
  }

/*****************************************************************************/
/****************** Show a form for sending an user's photo ******************/
/*****************************************************************************/

static void Pho_ReqPhoto (const struct UsrData *UsrDat)
  {
   extern const char *Hlp_PROFILE_Photo;
   extern const char *Txt_Photo;
   extern const char *Txt_You_can_send_a_file_with_an_image_in_JPEG_format_;
   extern const char *Txt_File_with_the_photo;
   extern const char *Txt_Upload_photo;
   bool ItsMe = Usr_ItsMe (UsrDat->UsrCod);
   static const Act_Action_t NextAction[Rol_NUM_ROLES] =
     {
      [Rol_UNK	  ] = ActDetOthPho,
      [Rol_GST	  ] = ActDetOthPho,
      [Rol_USR	  ] = ActDetOthPho,
      [Rol_STD	  ] = ActDetStdPho,
      [Rol_NET	  ] = ActDetTchPho,
      [Rol_TCH	  ] = ActDetTchPho,
      [Rol_DEG_ADM] = ActDetOthPho,
      [Rol_CTR_ADM] = ActDetOthPho,
      [Rol_INS_ADM] = ActDetOthPho,
      [Rol_SYS_ADM] = ActDetOthPho,
     };

   /***** Begin box *****/
   Box_BoxBegin (NULL,Txt_Photo,
                 ItsMe ? Pho_PutIconToRequestRemoveMyPhoto :
	                 Pho_PutIconToRequestRemoveOtherUsrPhoto,NULL,
		 Hlp_PROFILE_Photo,Box_NOT_CLOSABLE);

      /***** Begin form *****/
      if (ItsMe)
	 Frm_BeginForm (ActDetMyPho);
      else
	{
	 Frm_BeginForm (NextAction[Gbl.Usrs.Other.UsrDat.Roles.InCurrentCrs]);
	    Usr_PutParamUsrCodEncrypted (UsrDat->EnUsrCod);
	}

	 /***** Show help message *****/
	 Ale_ShowAlert (Ale_INFO,Txt_You_can_send_a_file_with_an_image_in_JPEG_format_);

	 /***** Form to upload photo *****/
	 HTM_LABEL_Begin ("class=\"FORM_IN_%s\"",The_GetSuffix ());
	    HTM_TxtColonNBSP (Txt_File_with_the_photo);
	    HTM_INPUT_FILE (Fil_NAME_OF_PARAM_FILENAME_ORG,"image/*",
			    HTM_SUBMIT_ON_CHANGE,NULL);
	 HTM_LABEL_End ();

      /***** End form *****/
      Frm_EndForm ();

   /***** End box *****/
   Box_BoxEnd ();
  }

/*****************************************************************************/
/*********************** Send another user's photograph **********************/
/*****************************************************************************/

void Pho_SendPhotoUsr (void)
  {
   /***** Get user whose photo must be sent or removed *****/
   if (!Usr_GetParamOtherUsrCodEncryptedAndGetUsrData ())
     {	// User not found
      Ale_ShowAlertUserNotFoundOrYouDoNotHavePermission ();
      return;
     }

   /***** Check if I have permission to change user's photo *****/
   if (!Pho_ICanChangeOtherUsrPhoto (&Gbl.Usrs.Other.UsrDat))
     {
      Ale_ShowAlertUserNotFoundOrYouDoNotHavePermission ();
      return;
     }

   if (Usr_ItsMe (Gbl.Usrs.Other.UsrDat.UsrCod))
      /***** Form to send my photo *****/
      Pho_ReqMyPhoto ();
   else	// Not me
      /***** Form to send another user's photo *****/
      Pho_ReqOtherUsrPhoto ();
  }

/*****************************************************************************/
/**************** Receive my photo and detect faces on it ********************/
/*****************************************************************************/

void Pho_RecMyPhotoDetFaces (void)
  {
   /***** Receive my photo and detect faces on it *****/
   if (!Pho_ReceivePhotoAndDetectFaces (true,&Gbl.Usrs.Me.UsrDat))
      Pho_ReqPhoto (&Gbl.Usrs.Me.UsrDat);	// Request my photograph again

   /***** Show my record and other data *****/
   Rec_ShowMySharedRecordAndMore ();
  }

/*****************************************************************************/
/********** Receive another user's photo and detect faces on it **************/
/*****************************************************************************/

void Pho_RecOtherUsrPhotoDetFaces (void)
  {
   /***** Get user's code from form *****/
   Usr_GetParamOtherUsrCodEncryptedAndGetListIDs ();

   /***** Get password, user type and user's data from database *****/
   if (Usr_ChkUsrCodAndGetAllUsrDataFromUsrCod (&Gbl.Usrs.Other.UsrDat,
                                                Usr_DONT_GET_PREFS,
                                                Usr_DONT_GET_ROLE_IN_CURRENT_CRS))
     {
      /***** Receive photo *****/
      if (!Pho_ReceivePhotoAndDetectFaces (false,&Gbl.Usrs.Other.UsrDat))
         Pho_ReqPhoto (&Gbl.Usrs.Other.UsrDat);	// Request user's photograph again

      /***** Show another user's record card *****/
      Rec_ShowPublicSharedRecordOtherUsr ();
     }
   else
      Ale_ShowAlertUserNotFoundOrYouDoNotHavePermission ();
  }

/*****************************************************************************/
/********************** Request the removal of my photo **********************/
/*****************************************************************************/

void Pho_ReqRemoveMyPhoto (void)
  {
   extern const char *Txt_Do_you_really_want_to_remove_your_photo;
   extern const char *Txt_Remove_photo;
   extern const char *Txt_The_photo_no_longer_exists;
   static const char *ClassPhoto[PhoSha_NUM_SHAPES] =
     {
      [PhoSha_SHAPE_CIRCLE   ] = "PHOTOR186x248",
      [PhoSha_SHAPE_ELLIPSE  ] = "PHOTOR186x248",
      [PhoSha_SHAPE_OVAL     ] = "PHOTOR186x248",
      [PhoSha_SHAPE_RECTANGLE] = "PHOTOR186x248",
     };

   /***** Show current photo and help message *****/
   if (Pho_BuildLinkToPhoto (&Gbl.Usrs.Me.UsrDat,Gbl.Usrs.Me.PhotoURL))
     {
      /***** Show question and button to remove my photo *****/
      /* Begin alert */
      Ale_ShowAlertAndButton1 (Ale_QUESTION,Txt_Do_you_really_want_to_remove_your_photo);

      /* Show current photo */
      Pho_ShowUsrPhoto (&Gbl.Usrs.Me.UsrDat,Gbl.Usrs.Me.PhotoURL,
			ClassPhoto[Gbl.Prefs.PhotoShape],Pho_NO_ZOOM,
			false);

      /* End alert */
      Ale_ShowAlertAndButton2 (ActRemMyPho,NULL,NULL,
                               NULL,NULL,
                               Btn_REMOVE_BUTTON,Txt_Remove_photo);
     }
   else
      Ale_ShowAlert (Ale_INFO,Txt_The_photo_no_longer_exists);

   /***** Show my record and other data *****/
   Rec_ShowMySharedRecordAndMore ();
  }

/*****************************************************************************/
/****************************** Remove my photo ******************************/
/*****************************************************************************/

void Pho_RemoveMyPhoto1 (void)
  {
   /***** Remove photo *****/
   Pho_RemovePhoto (&Gbl.Usrs.Me.UsrDat);

   /***** The link to my photo is not valid now, so build it again before writing the web page *****/
   Gbl.Usrs.Me.MyPhotoExists = Pho_BuildLinkToPhoto (&Gbl.Usrs.Me.UsrDat,Gbl.Usrs.Me.PhotoURL);
  }

void Pho_RemoveMyPhoto2 (void)
  {
   /***** Write success / warning message *****/
   Ale_ShowAlerts (NULL);

   /***** Show my record and other data *****/
   Rec_ShowMySharedRecordAndMore ();
  }

/*****************************************************************************/
/**************** Request the removal of another user's photo ****************/
/*****************************************************************************/

void Pho_ReqRemoveUsrPhoto (void)
  {
   extern const char *Txt_Photo;
   extern const char *Txt_Do_you_really_want_to_remove_the_photo_of_X;
   extern const char *Txt_Remove_photo;
   extern const char *Txt_The_photo_no_longer_exists;
   static const Act_Action_t NextAction[Rol_NUM_ROLES] =
     {
      [Rol_UNK	  ] = ActRemOthPho,
      [Rol_GST	  ] = ActRemOthPho,
      [Rol_USR	  ] = ActRemOthPho,
      [Rol_STD	  ] = ActRemStdPho,
      [Rol_NET	  ] = ActRemTchPho,
      [Rol_TCH	  ] = ActRemTchPho,
      [Rol_DEG_ADM] = ActRemOthPho,
      [Rol_CTR_ADM] = ActRemOthPho,
      [Rol_INS_ADM] = ActRemOthPho,
      [Rol_SYS_ADM] = ActRemOthPho,
     };
   static const char *ClassPhoto[PhoSha_NUM_SHAPES] =
     {
      [PhoSha_SHAPE_CIRCLE   ] = "PHOTOC186x248",
      [PhoSha_SHAPE_ELLIPSE  ] = "PHOTOE186x248",
      [PhoSha_SHAPE_OVAL     ] = "PHOTOO186x248",
      [PhoSha_SHAPE_RECTANGLE] = "PHOTOR186x248",
     };
   char PhotoURL[PATH_MAX + 1];

   /***** Get user's code from form *****/
   Usr_GetParamOtherUsrCodEncryptedAndGetListIDs ();

   /***** Get password, user type and user's data from database *****/
   if (Usr_ChkUsrCodAndGetAllUsrDataFromUsrCod (&Gbl.Usrs.Other.UsrDat,
                                                Usr_DONT_GET_PREFS,
                                                Usr_DONT_GET_ROLE_IN_CURRENT_CRS))
     {
      if (Pho_ICanChangeOtherUsrPhoto (&Gbl.Usrs.Other.UsrDat))
	{
	 /***** Show current photo and help message *****/
	 if (Pho_BuildLinkToPhoto (&Gbl.Usrs.Other.UsrDat,PhotoURL))
	   {
	    /***** Show question and button to remove user's photo *****/
	    /* Begin alert */
	    Ale_ShowAlertAndButton1 (Ale_QUESTION,Txt_Do_you_really_want_to_remove_the_photo_of_X,
	                             Gbl.Usrs.Other.UsrDat.FullName);

	    /* Show current photo */
	    Pho_ShowUsrPhoto (&Gbl.Usrs.Other.UsrDat,PhotoURL,
			      ClassPhoto[Gbl.Prefs.PhotoShape],Pho_NO_ZOOM,
			      false);

	    /* End alert */
	    Ale_ShowAlertAndButton2 (NextAction[Gbl.Usrs.Other.UsrDat.Roles.InCurrentCrs],NULL,NULL,
	                             Usr_PutParamOtherUsrCodEncrypted,Gbl.Usrs.Other.UsrDat.EnUsrCod,
				     Btn_REMOVE_BUTTON,Txt_Remove_photo);
	   }
	 else
	    Ale_ShowAlert (Ale_INFO,Txt_The_photo_no_longer_exists);
	}
      else
	 Ale_ShowAlertUserNotFoundOrYouDoNotHavePermission ();
     }
   else
      Ale_ShowAlertUserNotFoundOrYouDoNotHavePermission ();

   /***** Show another user's record card *****/
   Rec_ShowPublicSharedRecordOtherUsr ();
  }

/*****************************************************************************/
/************************ Remove another user's photo ************************/
/*****************************************************************************/

void Pho_RemoveUsrPhoto (void)
  {
   /***** Get user's code from form *****/
   Usr_GetParamOtherUsrCodEncryptedAndGetListIDs ();

   /***** Get password, user type and user's data from database *****/
   if (Usr_ChkUsrCodAndGetAllUsrDataFromUsrCod (&Gbl.Usrs.Other.UsrDat,
                                                Usr_DONT_GET_PREFS,
                                                Usr_DONT_GET_ROLE_IN_CURRENT_CRS))
     {
      /***** Remove photo *****/
      if (Pho_RemovePhoto (&Gbl.Usrs.Other.UsrDat))
         Ale_ShowAlerts (NULL);
     }
   else
      Ale_ShowAlertUserNotFoundOrYouDoNotHavePermission ();

   /***** Show another user's record card *****/
   Rec_ShowPublicSharedRecordOtherUsr ();
  }

/*****************************************************************************/
/***************** Receive a photo and detect faces on it ********************/
/*****************************************************************************/
// Return false if no "green" faces detected

static bool Pho_ReceivePhotoAndDetectFaces (bool ItsMe,const struct UsrData *UsrDat)
  {
   extern const char *Txt_The_file_is_not_X;
   extern const char *Txt_Could_not_detect_any_face_in_front_position_;
   extern const char *Txt_A_face_marked_in_green_has_been_detected_;
   extern const char *Txt_A_face_marked_in_red_has_been_detected_;
   extern const char *Txt_X_faces_marked_in_green_have_been_detected_;
   extern const char *Txt_X_faces_marked_in_red_have_been_detected_;
   extern const char *Txt_X_faces_have_been_detected_in_front_position_1_Z_;
   extern const char *Txt_X_faces_have_been_detected_in_front_position_Y_Z_;
   extern const char *Txt_Faces_detected;
   char PathPhotosPriv[PATH_MAX + 1];
   struct Param *Param;
   char FileNamePhotoSrc[PATH_MAX + 1];
   char FileNamePhotoTmp[PATH_MAX + 1];	// Full name (including path and .jpg) of the destination temporary file
   char FileNamePhotoMap[PATH_MAX + 1];	// Full name (including path) of the temporary file with the original image with faces
   char FileNameTxtMap[PATH_MAX + 1];	// Full name (including path) of the temporary file with the text neccesary to make the image map
   char PathRelPhoto[PATH_MAX + 1];
   FILE *FileTxtMap = NULL;		// Temporary file with the text neccesary to make the image map. Initialized to avoid warning
   char MIMEType[Brw_MAX_BYTES_MIME_TYPE + 1];
   bool WrongType = false;
   char Command[256 + PATH_MAX];	// Command to call the program of preprocessing of photos
   int ReturnCode;
   int NumLastForm = 0;	// Initialized to avoid warning
   char FormId[32];
   unsigned NumFacesTotal = 0;
   unsigned NumFacesGreen = 0;
   unsigned NumFacesRed = 0;
   unsigned NumFace;
   unsigned X;
   unsigned Y;
   unsigned Radius;
   unsigned BackgroundCode;
   char StrFileName[NAME_MAX + 1];
   static const Act_Action_t NextAction[Rol_NUM_ROLES] =
     {
      [Rol_UNK	  ] = ActUpdOthPho,
      [Rol_GST	  ] = ActUpdOthPho,
      [Rol_USR	  ] = ActUpdOthPho,
      [Rol_STD	  ] = ActUpdStdPho,
      [Rol_NET	  ] = ActUpdTchPho,
      [Rol_TCH	  ] = ActUpdTchPho,
      [Rol_DEG_ADM] = ActUpdOthPho,
      [Rol_CTR_ADM] = ActUpdOthPho,
      [Rol_INS_ADM] = ActUpdOthPho,
      [Rol_SYS_ADM] = ActUpdOthPho,
     };
   char ErrorTxt[256];
   char *Icon;

   /***** Creates directories if not exist *****/
   Fil_CreateDirIfNotExists (Cfg_PATH_PHOTO_PRIVATE);
   snprintf (PathPhotosPriv,sizeof (PathPhotosPriv),"%s/%02u",
	     Cfg_PATH_PHOTO_PRIVATE,(unsigned) (UsrDat->UsrCod % 100));
   Fil_CreateDirIfNotExists (PathPhotosPriv);

   /***** Create directories if not exists
          and remove old temporary files *****/
   /* Create public directory for photos */
   Fil_CreateDirIfNotExists (Cfg_PATH_PHOTO_PUBLIC);

   /* Create temporary directory for photos */
   Fil_CreateDirIfNotExists (Cfg_PATH_PHOTO_TMP_PUBLIC);

   /***** First of all, copy in disk the file received from stdin (really from Gbl.F.Tmp) *****/
   Param = Fil_StartReceptionOfFile (Fil_NAME_OF_PARAM_FILENAME_ORG,
                                     FileNamePhotoSrc,MIMEType);

   /* Check if the file type is image/jpeg or image/pjpeg or application/octet-stream */
   if (strcmp (MIMEType,"image/jpeg"))
      if (strcmp (MIMEType,"image/pjpeg"))
         if (strcmp (MIMEType,"application/octet-stream"))
            if (strcmp (MIMEType,"application/octetstream"))
               if (strcmp (MIMEType,"application/octet"))
                  WrongType = true;
   if (WrongType)
     {
      Ale_ShowAlert (Ale_WARNING,Txt_The_file_is_not_X,
		     "jpg");
      return false;
     }

   /* End the reception of photo in a temporary file */
   snprintf (FileNamePhotoTmp,sizeof (FileNamePhotoTmp),"%s/%s.jpg",
             Cfg_PATH_PHOTO_TMP_PUBLIC,Gbl.UniqueNameEncrypted);
   if (!Fil_EndReceptionOfFile (FileNamePhotoTmp,Param))
     {
      Ale_ShowAlert (Ale_ERROR,"Error copying file.");
      return false;
     }

   /* Copy the original photo received to private directory.
      The purpose of this copy is only to have a backup used for researching better methods to detect faces in images */
   snprintf (PathRelPhoto,sizeof (PathRelPhoto),"%s/%02u/%ld_original.jpg",
             Cfg_PATH_PHOTO_PRIVATE,
             (unsigned) (UsrDat->UsrCod % 100),UsrDat->UsrCod);
   Fil_FastCopyOfFiles (FileNamePhotoTmp,PathRelPhoto);

   /***** Call to program that makes photo processing / face detection *****/
   snprintf (Command,sizeof (Command),Cfg_COMMAND_FACE_DETECTION,
	     FileNamePhotoTmp);
   ReturnCode = system (Command);
   if (ReturnCode == -1)
      Err_ShowErrorAndExit ("Error when running command to process photo and detect faces.");

   /***** Write message depending on return code *****/
   ReturnCode = WEXITSTATUS(ReturnCode);
   switch (ReturnCode)
     {
      case 0:        // Faces detected
         /***** Open text file with text for image map *****/
         snprintf (FileNameTxtMap,sizeof (FileNameTxtMap),"%s/%s_map.txt",
                   Cfg_PATH_PHOTO_TMP_PUBLIC,Gbl.UniqueNameEncrypted);
         if ((FileTxtMap = fopen (FileNameTxtMap,"rb")) == NULL)
            Err_ShowErrorAndExit ("Can not read text file with coordinates of detected faces.");

         /***** Read file with coordinates for image map and compute the number of faces *****/
         NumLastForm = Gbl.Form.Num;
         while (!feof (FileTxtMap))
           {
            if (fscanf (FileTxtMap,"%u %u %u %u %s\n",&X,&Y,&Radius,&BackgroundCode,StrFileName) != 5)        // Example of StrFileName = "4924a838630e_016"

               break;
            if (BackgroundCode == 1)
              {
               NumFacesGreen++;
               if (ItsMe)
        	  Frm_BeginForm (ActUpdMyPho);
               else
        	 {
		  Frm_BeginForm (NextAction[Gbl.Usrs.Other.UsrDat.Roles.InCurrentCrs]);
		     Usr_PutParamUsrCodEncrypted (UsrDat->EnUsrCod);
        	 }
		  Par_PutHiddenParamString (NULL,"FileName",StrFileName);
		  Frm_EndForm ();
              }
            else
               NumFacesRed++;
           }
         NumFacesTotal = NumFacesGreen + NumFacesRed;
         break;
      case 1:        // No faces detected
         NumFacesTotal = NumFacesGreen = NumFacesRed = 0;
         break;
      default:        // Error
         snprintf (ErrorTxt,sizeof (ErrorTxt),
	           "Photo could not be processed successfully.<br />"
                   "Error code returned by the program of processing: %d",
                   ReturnCode);
         Err_ShowErrorAndExit (ErrorTxt);
         break;
     }

   /***** Begin alert to the user about the number of faces detected in the image*****/
   if (NumFacesTotal == 0)
      Ale_ShowAlertAndButton1 (Ale_WARNING,Txt_Could_not_detect_any_face_in_front_position_);
   else if (NumFacesTotal == 1)
     {
      if (NumFacesGreen == 1)
         Ale_ShowAlertAndButton1 (Ale_SUCCESS,Txt_A_face_marked_in_green_has_been_detected_);
      else
         Ale_ShowAlertAndButton1 (Ale_WARNING,Txt_A_face_marked_in_red_has_been_detected_);
     }
   else        // NumFacesTotal > 1
     {
      if (NumFacesRed == 0)
         Ale_ShowAlertAndButton1 (Ale_SUCCESS,Txt_X_faces_marked_in_green_have_been_detected_,
                                  NumFacesGreen);
      else if (NumFacesGreen == 0)
         Ale_ShowAlertAndButton1 (Ale_WARNING,Txt_X_faces_marked_in_red_have_been_detected_,
                                  NumFacesRed);
      else        // NumFacesGreen > 0
        {
         if (NumFacesGreen == 1)
            Ale_ShowAlertAndButton1 (Ale_SUCCESS,Txt_X_faces_have_been_detected_in_front_position_1_Z_,
                                     NumFacesTotal,NumFacesRed);
         else
            Ale_ShowAlertAndButton1 (Ale_SUCCESS,Txt_X_faces_have_been_detected_in_front_position_Y_Z_,
                                     NumFacesTotal,NumFacesGreen,NumFacesRed);
        }
     }

   /***** Create map *****/
   HTM_Txt ("<map name=\"faces_map\">\n");
   if (NumFacesTotal)
     {
      /***** Read again the file with coordinates and create area shapes *****/
      rewind (FileTxtMap);
      for (NumFace = 0; !feof (FileTxtMap);)
        {
         if (fscanf (FileTxtMap,"%u %u %u %u %s\n",&X,&Y,&Radius,&BackgroundCode,StrFileName) != 5)
            break;
         if (BackgroundCode == 1)
           {
            NumFace++;
            snprintf (FormId,sizeof (FormId),"form_%d",NumLastForm + NumFace);
            HTM_TxtF ("<area shape=\"circle\""
                      " href=\"\""
                      " onclick=\"javascript:document.getElementById('%s').submit();return false;\""
                      " coords=\"%u,%u,%u\">\n",
                      FormId,X,Y,Radius);
           }
        }
      /***** Close text file with text for image map *****/
      fclose (FileTxtMap);
     }
   HTM_Txt ("</map>\n");

   /***** Show map photo *****/
   snprintf (FileNamePhotoMap,sizeof (FileNamePhotoMap),"%s/%s_map.jpg",
             Cfg_PATH_PHOTO_TMP_PUBLIC,Gbl.UniqueNameEncrypted);
   HTM_DIV_Begin ("class=\"TIT CM\"");
      if (asprintf (&Icon,"%s_map.jpg",Gbl.UniqueNameEncrypted) < 0)
	 Err_NotEnoughMemoryExit ();
      HTM_IMG (Cfg_URL_PHOTO_TMP_PUBLIC,Icon,Txt_Faces_detected,"usemap=\"#faces_map\"");
      free (Icon);
   HTM_DIV_End ();

   /***** End alert *****/
   Ale_ShowAlertAndButton2 (ActUnk,NULL,NULL,
                            NULL,NULL,
                            Btn_NO_BUTTON,NULL);

   /***** Button to send another photo *****/
   return (NumFacesGreen != 0);
  }

/*****************************************************************************/
/***************************** Update my photo *******************************/
/*****************************************************************************/

void Pho_UpdateMyPhoto1 (void)
  {
   Pho_UpdatePhoto1 (&Gbl.Usrs.Me.UsrDat);

   /***** The link to my photo is not valid now, so build it again before writing the web page *****/
   Gbl.Usrs.Me.MyPhotoExists = Pho_BuildLinkToPhoto (&Gbl.Usrs.Me.UsrDat,Gbl.Usrs.Me.PhotoURL);
  }

void Pho_UpdateMyPhoto2 (void)
  {
   Pho_UpdatePhoto2 ();

   /***** Show my record and other data *****/
   Rec_ShowMySharedRecordAndMore ();
  }

/*****************************************************************************/
/*********************** Update another user's photo *************************/
/*****************************************************************************/

void Pho_UpdateUsrPhoto1 (void)
  {
   /***** Get user's code from form and user's data *****/
   if (Usr_GetParamOtherUsrCodEncryptedAndGetUsrData ())
      Pho_UpdatePhoto1 (&Gbl.Usrs.Other.UsrDat);
   else
      Ale_ShowAlertUserNotFoundOrYouDoNotHavePermission ();
  }

void Pho_UpdateUsrPhoto2 (void)
  {
   Pho_UpdatePhoto2 ();

   /***** Show another user's record card *****/
   Rec_ShowPublicSharedRecordOtherUsr ();
  }

/*****************************************************************************/
/*************** Update a user's photo with a selected face ******************/
/*****************************************************************************/

static void Pho_UpdatePhoto1 (struct UsrData *UsrDat)
  {
   extern const char *Txt_Photo_has_been_updated;
   char PathPhotoTmp[PATH_MAX + 1];	// Full name (including path and .jpg) of the temporary file with the selected face
   char PathRelPhoto[PATH_MAX + 1];

   /***** Get the name of the file with the selected face *****/
   Par_GetParToText ("FileName",Gbl.Usrs.FileNamePhoto,NAME_MAX);        // Example of FileNamePhoto: "4924a838630e_016"

   /***** Convert the temporary photo resulting of the processing to the current photo of the user *****/
   snprintf (PathPhotoTmp,sizeof (PathPhotoTmp),"%s/%s_paso3.jpg",
             Cfg_PATH_PHOTO_TMP_PUBLIC,Gbl.Usrs.FileNamePhoto);
   if (Fil_CheckIfPathExists (PathPhotoTmp))        // The file with the selected photo exists
     {
      /* Copy the temporary file of the third (last) step resulting of the processing to the directory of private photos */
      snprintf (PathRelPhoto,sizeof (PathRelPhoto),"%s/%02u/%ld.jpg",
                Cfg_PATH_PHOTO_PRIVATE,
                (unsigned) (UsrDat->UsrCod % 100),UsrDat->UsrCod);
      Fil_FastCopyOfFiles (PathPhotoTmp,PathRelPhoto);

      /* Update public photo name in database */
      Pho_UpdatePhotoName (UsrDat);

      /* Remove the user from the list of users without photo */
      Pho_DB_RemoveUsrFromTableClicksWithoutPhoto (UsrDat->UsrCod);

      Ale_CreateAlert (Ale_SUCCESS,NULL,
		       Txt_Photo_has_been_updated);
     }
   else
      Ale_CreateAlert (Ale_ERROR,NULL,
	               "Error updating photo.");
  }

static void Pho_UpdatePhoto2 (void)
  {
   extern const char *Txt_PHOTO_PROCESSING_CAPTIONS[3];
   unsigned NumPhoto;
   char *Img;

   /***** Begin alert *****/
   Ale_ShowLastAlertAndButton1 ();

   /***** Show the three images resulting of the processing *****/
   HTM_TABLE_BeginWide ();
      HTM_TR_Begin (NULL);
	 for (NumPhoto = 0;
	      NumPhoto < 3;
	      NumPhoto++)
	   {
	    HTM_TD_Begin ("class=\"CT DAT_%s\" style=\"width:33%%;\"",
	                  The_GetSuffix ());
	       if (asprintf (&Img,"%s_paso%u.jpg",Gbl.Usrs.FileNamePhoto,NumPhoto + 1) < 0)
		  Err_NotEnoughMemoryExit ();
	       HTM_IMG (Cfg_URL_PHOTO_TMP_PUBLIC,Img,Txt_PHOTO_PROCESSING_CAPTIONS[NumPhoto],
			"style=\"width:%upx;height:%upx;\"",
			Pho_PHOTO_REAL_WIDTH,Pho_PHOTO_REAL_HEIGHT);
	       free (Img);
	       HTM_BR ();
	       HTM_Txt (Txt_PHOTO_PROCESSING_CAPTIONS[NumPhoto]);
	    HTM_TD_End ();
	   }
      HTM_TR_End ();
   HTM_TABLE_End ();

   /***** End alert *****/
   Ale_ShowAlertAndButton2 (ActUnk,NULL,NULL,
                            NULL,NULL,
                            Btn_NO_BUTTON,NULL);
  }

/*****************************************************************************/
/******************* Update number of clicks without photo *******************/
/*****************************************************************************/

unsigned Pho_UpdateMyClicksWithoutPhoto (void)
  {
   MYSQL_RES *mysql_res;
   MYSQL_ROW row;
   unsigned NumClicks;

   /***** Get number of clicks without photo from database *****/
   if (Pho_DB_GetMyClicksWithoutPhoto (&mysql_res))        // The user exists ==> update number of clicks without photo
     {
      /* Get current number of clicks */
      row = mysql_fetch_row (mysql_res);
      sscanf (row[0],"%u",&NumClicks);

      /* Update number of clicks */
      if (NumClicks <= Pho_MAX_CLICKS_WITHOUT_PHOTO)
        {
         Pho_DB_IncrMyClicksWithoutPhoto ();
         NumClicks++;
        }
     }
   else                                      		  // The user does not exist ==> add him/her
     {
      /* Add the user, with one access */
      Pho_DB_InitMyClicksWithoutPhoto ();
      NumClicks = 1;
     }

   /***** Free structure that stores the query result *****/
   DB_FreeMySQLResult (&mysql_res);

   /***** Return the number of rows of the result *****/
   return NumClicks;
  }

/*****************************************************************************/
/********************* Show a user's photo if allowed ************************/
/*****************************************************************************/

void Pho_ShowUsrPhotoIfAllowed (struct UsrData *UsrDat,
                                const char *ClassPhoto,Pho_Zoom_t Zoom,
                                bool FormUnique)
  {
   char PhotoURL[PATH_MAX + 1];
   bool ShowPhoto = Pho_ShowingUsrPhotoIsAllowed (UsrDat,PhotoURL);

   Pho_ShowUsrPhoto (UsrDat,ShowPhoto ? PhotoURL :
					NULL,
		     ClassPhoto,Zoom,FormUnique);
  }

/*****************************************************************************/
/********************* Check if user's photo can be shown ********************/
/*****************************************************************************/
// Returns true if the photo can be shown and false if not.
// Public photo means two different things depending on the user's type

bool Pho_ShowingUsrPhotoIsAllowed (struct UsrData *UsrDat,
                                   char PhotoURL[PATH_MAX + 1])
  {
   bool ICanSeePhoto;

   /***** Check if I can see the other's photo *****/
   ICanSeePhoto = Pri_ShowingIsAllowed (UsrDat->PhotoVisibility,UsrDat);

   /***** Photo is shown if I can see it, and it exists *****/
   return ICanSeePhoto ? Pho_BuildLinkToPhoto (UsrDat,PhotoURL) :
	                 false;
  }

/*****************************************************************************/
/***************** Create a public link to a private photo *******************/
/*****************************************************************************/
// Returns false if photo does not exist
// Returns true if link is created successfully

bool Pho_BuildLinkToPhoto (const struct UsrData *UsrDat,char PhotoURL[PATH_MAX + 1])
  {
   char PathPublPhoto[PATH_MAX + 1];
   char PathPrivPhoto[PATH_MAX + 1];

   if (UsrDat->Photo[0])
     {
      /***** Make path to public photo *****/
      snprintf (PathPublPhoto,sizeof (PathPublPhoto),"%s/%s.jpg",
                Cfg_PATH_PHOTO_PUBLIC,UsrDat->Photo);

      /***** Make path to private photo from public directory *****/
      snprintf (PathPrivPhoto,sizeof (PathPrivPhoto),"%s/%02u/%ld.jpg",
                Cfg_PATH_PHOTO_PRIVATE,
                (unsigned) (UsrDat->UsrCod % 100),UsrDat->UsrCod);

      /***** Create a symbolic link to the private photo, if not exists *****/
      if (!Fil_CheckIfPathExists (PathPublPhoto))
         if (symlink (PathPrivPhoto,PathPublPhoto) != 0)
            Err_ShowErrorAndExit ("Can not create public link"
                                 " to access to user's private photo");

      /***** Create the public URL of the photo *****/
      snprintf (PhotoURL,PATH_MAX + 1,"%s/%s.jpg",
                Cfg_URL_PHOTO_PUBLIC,UsrDat->Photo);

      return true;
     }
   else
     {
      PhotoURL[0] = '\0';
      return false;
     }
  }

/*****************************************************************************/
/*************** Check if photo exists and return path to it *****************/
/*****************************************************************************/
// Returns false if photo does not exist
// Returns true if photo exists

bool Pho_CheckIfPrivPhotoExists (long UsrCod,char PathPrivRelPhoto[PATH_MAX + 1])
  {
   /***** Make path to private photo *****/
   snprintf (PathPrivRelPhoto,PATH_MAX + 1,"%s/%02u/%ld.jpg",
             Cfg_PATH_PHOTO_PRIVATE,(unsigned) (UsrCod % 100),UsrCod);

   return Fil_CheckIfPathExists (PathPrivRelPhoto);
  }

/*****************************************************************************/
/********************* Build HTML code for user's photo **********************/
/*****************************************************************************/

void Pho_BuildHTMLUsrPhoto (const struct UsrData *UsrDat,const char *PhotoURL,
			    const char *ClassPhoto,Pho_Zoom_t Zoom,
			    char **ImgStr,
			    char **CaptionStr)
  {
   extern const char *Rol_Icons[Rol_NUM_ROLES];
   extern const char *Txt_Following;
   extern const char *Txt_Followers;
   unsigned NumFollowing;
   unsigned NumFollowers;
   bool PhotoExists;
   Act_BrowserTab_t BrowserTab = Act_GetBrowserTab (Gbl.Action.Act);
   bool BrowserTabIs1stTab = (BrowserTab == Act_BRW_1ST_TAB ||
	                      BrowserTab == Act_AJAX_NORMAL ||
			      BrowserTab == Act_AJAX_RFRESH);
   bool PutZoomCode = (Zoom == Pho_ZOOM) &&		// Make zoom
                      BrowserTabIs1stTab;		// Only in main browser tab (or AJAX)
   char IdCaption[Frm_MAX_BYTES_ID + 1];
   char CtyName[Cns_HIERARCHY_MAX_BYTES_FULL_NAME + 1];
   struct Ins_Instit Ins;
   char MainDegreeShrtName[Cns_HIERARCHY_MAX_BYTES_SHRT_NAME + 1];
   Rol_Role_t MaxRole;	// Maximum user's role in his/her main degree
   struct
     {
      char *Name;
      char *Nick;
      char *InsCty;
      char *MainDeg;
      char *Follow;
     } Caption;

   /***** Hidden div to pass user's name to Javascript *****/
   if (PutZoomCode)
     {
      /* First name and surnames */
      if (asprintf (&Caption.Name,"<div class=\"ZOOM_TXT DAT_STRONG_%s BOLD\">"	// Limited width
				     "%s<br />"
				     "%s%s%s"
				  "</div>",
		    The_GetSuffix (),
		    UsrDat->FrstName,
		    UsrDat->Surname1,
		    UsrDat->Surname2[0] ? "&nbsp;" :
					  "",
		    UsrDat->Surname2[0] ? UsrDat->Surname2 :
					  "") < 0)
	 Err_NotEnoughMemoryExit ();

      /* Nickname */
      if (UsrDat->Nickname[0])
	{
	 if (asprintf (&Caption.Nick,"<div class=\"ZOOM_TXT DAT_SMALL_STRONG_%s\">"
					"@%s"
				     "</div>",
		       The_GetSuffix (),
		       UsrDat->Nickname) < 0)
	    Err_NotEnoughMemoryExit ();
	}
      else if (asprintf (&Caption.Nick,"%s","") < 0)
	 Err_NotEnoughMemoryExit ();

      /* Institution full name and institution country */
      if (UsrDat->InsCod > 0)
	{
	 /* Get institution short name and country name */
	 Ins.InsCod = UsrDat->InsCod;
	 Ins_GetShrtNameAndCtyOfInstitution (&Ins,CtyName);

	 /* Write institution short name and country name */
	 if (asprintf (&Caption.InsCty,"<div class=\"ZOOM_TXT DAT_SMALL_%s\">"
					  "%s&nbsp;(%s)"
				       "</div>",
		       The_GetSuffix (),
		       Ins.ShrtName,CtyName) < 0)
	    Err_NotEnoughMemoryExit ();
	}
      else if (UsrDat->CtyCod > 0)
	{
	 /* Get country name */
	 Cty_GetCountryName (UsrDat->CtyCod,Gbl.Prefs.Language,CtyName);

	 /* Write country name */
	 if (asprintf (&Caption.InsCty,"<div class=\"ZOOM_TXT DAT_SMALL_%s\">"
					  "%s"
				       "</div>",
		       The_GetSuffix (),
		       CtyName) < 0)
	    Err_NotEnoughMemoryExit ();
	}
      else if (asprintf (&Caption.InsCty,"%s","") < 0)
	 Err_NotEnoughMemoryExit ();

      /* Main degree (in which the user has more courses) short name */
      Deg_GetUsrMainDeg (UsrDat->UsrCod,MainDegreeShrtName,&MaxRole);
      if (MainDegreeShrtName[0])
	{
	 if (asprintf (&Caption.MainDeg,"<div class=\"ZOOM_TXT DAT_SMALL_%s\">"
					    "<img src=\"%s/%s\" alt=\"\""
					    " class=\"ICO16x16 ICO_BLACK_%s\" />"
					    " %s"
					 "</div>",
		       The_GetSuffix (),
		       Cfg_URL_ICON_PUBLIC,Rol_Icons[MaxRole],
		       The_GetSuffix (),
		       MainDegreeShrtName) < 0)
	    Err_NotEnoughMemoryExit ();
	}
      else if (asprintf (&Caption.MainDeg,"%s","") < 0)
	 Err_NotEnoughMemoryExit ();

      /* Following and followers */
      if (UsrDat->Nickname[0])	// Get social data only if nickname is retrieved (in some actions)
	{
	 Fol_GetNumFollow (UsrDat->UsrCod,&NumFollowing,&NumFollowers);
	 if (asprintf (&Caption.Follow,"<div class=\"ZOOM_TXT\">"
					   "<span class=\"DAT_STRONG_%s BOLD\">"
					      "%u"
					   "</span>"
					   "<span class=\"DAT_SMALL_%s\">"
					      "&nbsp;%s&nbsp;"
					   "</span>"
					   "<span class=\"DAT_STRONG_%s BOLD\">"
					      "%u"
					   "</span>"
					   "<span class=\"DAT_SMALL_%s\">"
					      "&nbsp;%s"
					   "</span>"
					"</div>",
		       The_GetSuffix (),
		       NumFollowing,
		       The_GetSuffix (),
		       Txt_Following,
		       The_GetSuffix (),
		       NumFollowers,
		       The_GetSuffix (),
		       Txt_Followers) < 0)
	    Err_NotEnoughMemoryExit ();
	}
      else if (asprintf (&Caption.Follow,"%s","") < 0)
	 Err_NotEnoughMemoryExit ();

      /* Hidden div */
      Frm_SetUniqueId (IdCaption);
      if (asprintf (CaptionStr,"<div id=\"%s\" class=\"NOT_SHOWN\">"
				  "%s%s%s%s%s"
			       "</div>",
		    IdCaption,
		    Caption.Name,
		    Caption.Nick,
		    Caption.InsCty,
		    Caption.MainDeg,
		    Caption.Follow) < 0)
	  Err_NotEnoughMemoryExit ();
      free (Caption.Follow);
      free (Caption.MainDeg);
      free (Caption.InsCty);
      free (Caption.Nick);
      free (Caption.Name);
     }
   else
     {
      if (asprintf (CaptionStr,"%s","") < 0)
	  Err_NotEnoughMemoryExit ();
     }

   /***** Image zoom *****/
   PhotoExists = false;
   if (PhotoURL)
      if (PhotoURL[0])
	 PhotoExists = true;
   if (PutZoomCode)
     {
      if (PhotoExists)
	{
	 if (asprintf (ImgStr,"<img src=\"%s\" alt=\"\" title=\"%s\" class=\"%s\""
			      " onmouseover=\"zoom(this,'%s','%s');\""
			      " onmouseout=\"noZoom();\""
			      " />",
		       PhotoURL,UsrDat->FullName,ClassPhoto,
		       PhotoURL,IdCaption) < 0)
	    Err_NotEnoughMemoryExit ();
	}
      else
	{
	 if (asprintf (ImgStr,"<img src=\"%s/usr_bl.jpg\" alt=\"\" title=\"%s\" class=\"%s\""
			      " onmouseover=\"zoom(this,'%s/usr_bl.jpg','%s');\""
			      " onmouseout=\"noZoom();\""
			      " />",
		       Cfg_URL_ICON_PUBLIC,UsrDat->FullName,ClassPhoto,
		       Cfg_URL_ICON_PUBLIC,IdCaption) < 0)
	    Err_NotEnoughMemoryExit ();
	}
     }
   else
     {
      if (PhotoExists)
	{
	 if (asprintf (ImgStr,"<img src=\"%s\" alt=\"\" title=\"%s\" class=\"%s\" />",
		       PhotoURL,UsrDat->FullName,ClassPhoto) < 0)
	    Err_NotEnoughMemoryExit ();
	}
      else
	{
	 if (asprintf (ImgStr,"<img src=\"%s/usr_bl.jpg\" alt=\"\" title=\"%s\" class=\"%s\" />",
		       Cfg_URL_ICON_PUBLIC,UsrDat->FullName,ClassPhoto) < 0)
	    Err_NotEnoughMemoryExit ();
	}
     }
  }

/*****************************************************************************/
/*************************** Show a user's photo *****************************/
/*****************************************************************************/

void Pho_ShowUsrPhoto (const struct UsrData *UsrDat,const char *PhotoURL,
                       const char *ClassPhoto,Pho_Zoom_t Zoom,
                       bool FormUnique)
  {
   Act_BrowserTab_t BrowserTab = Act_GetBrowserTab (Gbl.Action.Act);
   bool BrowserTabIs1stTab = (BrowserTab == Act_BRW_1ST_TAB ||
	                      BrowserTab == Act_AJAX_NORMAL ||
			      BrowserTab == Act_AJAX_RFRESH);
   bool PutLinkToPublicProfile = !Gbl.Form.Inside &&	// Only if not inside another form
                                 BrowserTabIs1stTab;	// Only in main browser tab (or AJAX)
   char *CaptionStr;
   char *ImgStr;

   /***** Begin form to go to public profile *****/
   if (PutLinkToPublicProfile)
     {
      if (FormUnique)
	 Frm_BeginFormUnique (ActSeeOthPubPrf);
      else
	 Frm_BeginForm (ActSeeOthPubPrf);
      Usr_PutParamUsrCodEncrypted (UsrDat->EnUsrCod);
	 HTM_BUTTON_Submit_Begin (NULL,"class=\"BT_LINK\"");
     }

   /***** Hidden div to pass user's name to Javascript *****/
   Pho_BuildHTMLUsrPhoto (UsrDat,PhotoURL,
			  ClassPhoto,Zoom,
			  &CaptionStr,
			  &ImgStr);
   HTM_Txt (CaptionStr);
   HTM_Txt (ImgStr);
   free (ImgStr);
   free (CaptionStr);

   /***** End form to go to public profile *****/
   if (PutLinkToPublicProfile)
     {
	 HTM_BUTTON_End ();
      Frm_EndForm ();
     }
  }

/*****************************************************************************/
/************************** Remove a user's photo ****************************/
/*****************************************************************************/
// Returns true on success, false on error

bool Pho_RemovePhoto (struct UsrData *UsrDat)
  {
   extern const char *Txt_Photo_removed;
   char PathPrivRelPhoto[PATH_MAX + 1];
   char PathPublPhoto[PATH_MAX + 1];
   unsigned NumErrors = 0;

   if (UsrDat->Photo[0])
     {
      /***** Clear photo name in database *****/
      Pho_DB_ClearPhotoName (UsrDat->UsrCod);

      /***** Remove public link *****/
      snprintf (PathPublPhoto,sizeof (PathPublPhoto),"%s/%s.jpg",
                Cfg_PATH_PHOTO_PUBLIC,UsrDat->Photo);
      if (Fil_CheckIfPathExists (PathPublPhoto))	// Public link exists
         if (unlink (PathPublPhoto))			// Remove public link
            NumErrors++;

      /***** Remove photo *****/
      snprintf (PathPrivRelPhoto,sizeof (PathPrivRelPhoto),"%s/%02u/%ld.jpg",
                Cfg_PATH_PHOTO_PRIVATE,
                (unsigned) (UsrDat->UsrCod % 100),UsrDat->UsrCod);
      if (Fil_CheckIfPathExists (PathPrivRelPhoto))        // Photo exists
        {
         if (unlink (PathPrivRelPhoto))                        // Remove photo
            NumErrors++;
        }

      /***** Remove original photo *****/
      snprintf (PathPrivRelPhoto,sizeof (PathPrivRelPhoto),
                "%s/%02u/%ld_original.jpg",
                Cfg_PATH_PHOTO_PRIVATE,
                (unsigned) (UsrDat->UsrCod % 100),UsrDat->UsrCod);
      if (Fil_CheckIfPathExists (PathPrivRelPhoto))		// Original photo exists
         if (unlink (PathPrivRelPhoto))				// Remove original photo
            NumErrors++;

      /***** Clear photo name in user's data *****/
      UsrDat->Photo[0] = '\0';
     }

   if (NumErrors)
     {
      Ale_CreateAlert (Ale_ERROR,NULL,
		       "Error removing photo.");
      return false;
     }
   else
     {
      Ale_CreateAlert (Ale_SUCCESS,NULL,
	               Txt_Photo_removed);
      return true;
     }
  }

/*****************************************************************************/
/***************** Update photo name of an user in database ******************/
/*****************************************************************************/

void Pho_UpdatePhotoName (struct UsrData *UsrDat)
  {
   char PathPublPhoto[PATH_MAX + 1];

   /***** Update photo name in database *****/
   Pho_DB_UpdatePhotoName (UsrDat->UsrCod,Gbl.UniqueNameEncrypted);

   /***** Remove the old symbolic link to photo *****/
   snprintf (PathPublPhoto,sizeof (PathPublPhoto),"%s/%s.jpg",
             Cfg_PATH_PHOTO_PUBLIC,UsrDat->Photo);
   unlink (PathPublPhoto);                // Remove public link

   /***** Update photo name in user's data *****/
   Str_Copy (UsrDat->Photo,Gbl.UniqueNameEncrypted,sizeof (UsrDat->Photo) - 1);
  }

/*****************************************************************************/
/************************** Change photo visibility **************************/
/*****************************************************************************/

void Pho_ChangePhotoVisibility (void)
  {
   /***** Get param with public/private photo *****/
   Gbl.Usrs.Me.UsrDat.PhotoVisibility = Pri_GetParamVisibility ("VisPho",
	                                                        Pri_PHOTO_ALLOWED_VIS);

   /***** Store public/private photo in database *****/
   Set_DB_UpdateMySettingsAboutPhotoVisibility ();

   /***** Show form again *****/
   Set_EditSettings ();
  }

/*****************************************************************************/
/******** Calculate average photos of all students from each degree **********/
/*****************************************************************************/

void Pho_CalcPhotoDegree (void)
  {
   Pho_AvgPhotoTypeOfAverage_t TypeOfAverage;
   long DegCod = -1L;
   char DirAvgPhotosRelPath[Pho_NUM_AVERAGE_PHOTO_TYPES][PATH_MAX + 1];
   unsigned NumStds,NumStdsWithPhoto;
   Usr_Sex_t Sex;
   long TotalTimeToComputeAvgPhotoInMicroseconds,PartialTimeToComputeAvgPhotoInMicroseconds;

   /***** Create public directories for average photos if not exist *****/
   Fil_CreateDirIfNotExists (Cfg_PATH_PHOTO_PUBLIC);
   for (TypeOfAverage = (Pho_AvgPhotoTypeOfAverage_t) 0;
	TypeOfAverage < Pho_NUM_AVERAGE_PHOTO_TYPES;
	TypeOfAverage++)
     {
      snprintf (DirAvgPhotosRelPath[TypeOfAverage],
	        sizeof (DirAvgPhotosRelPath[TypeOfAverage]),"%s/%s",
                Cfg_PATH_PHOTO_PUBLIC,Pho_StrAvgPhotoDirs[TypeOfAverage]);
      Fil_CreateDirIfNotExists (DirAvgPhotosRelPath[TypeOfAverage]);
     }

   /***** Creates private directory for lists of users' photos if not exists *****/
   Fil_CreateDirIfNotExists (Cfg_PATH_PHOTO_TMP_PRIVATE);

   /***** Get the degree which photo will be computed *****/
   DegCod = Deg_GetAndCheckParamOtherDegCod (-1L);	// Parameter may be omitted
							// (when selecting classphoto/list)

   if (DegCod > 0)
     {
      /***** Prevent the computing of an average photo too recently updated *****/
      if (Pho_GetTimeAvgPhotoWasComputed (DegCod) >=
	  Gbl.StartExecutionTimeUTC - Cfg_MIN_TIME_TO_RECOMPUTE_AVG_PHOTO)
	 Err_ShowErrorAndExit ("Average photo has been computed recently.");

      /***** Get list of students in this degree *****/
      Usr_GetUnorderedStdsCodesInDeg (DegCod);

      for (Sex  = (Usr_Sex_t) 0;
	   Sex <= (Usr_Sex_t) (Usr_NUM_SEXS - 1);
	   Sex++)
	{
	 TotalTimeToComputeAvgPhotoInMicroseconds = 0;
	 for (TypeOfAverage  = (Pho_AvgPhotoTypeOfAverage_t) 0;
	      TypeOfAverage <= (Pho_AvgPhotoTypeOfAverage_t) (Pho_NUM_AVERAGE_PHOTO_TYPES - 1);
	      TypeOfAverage++)
	   {
	    /***** Compute average photos of students belonging this degree *****/
	    Pho_ComputeAveragePhoto (DegCod,Sex,Rol_STD,
				     TypeOfAverage,DirAvgPhotosRelPath[TypeOfAverage],
				     &NumStds,&NumStdsWithPhoto,&PartialTimeToComputeAvgPhotoInMicroseconds);
	    TotalTimeToComputeAvgPhotoInMicroseconds += PartialTimeToComputeAvgPhotoInMicroseconds;
	   }

	 /***** Store stats in database *****/
	 Pho_DB_UpdateDegStats (DegCod,Sex,NumStds,NumStdsWithPhoto,TotalTimeToComputeAvgPhotoInMicroseconds);
	}

      /***** Free memory for students list *****/
      Usr_FreeUsrsList (Rol_STD);
     }

   /***** Show photos *****/
   Pho_ShowOrPrintPhotoDegree (Pho_DEGREES_SEE);
  }

/*****************************************************************************/
/******** Get degree code with average photo least recently updated **********/
/*****************************************************************************/
// Returns -1 when error or when all degrees have been updated too recently

static long Pho_GetDegWithAvgPhotoLeastRecentlyUpdated (void)
  {
   long DegCod;

   /***** Delete all degrees in sta_degrees table not present in degrees table *****/
   Pho_DB_RemoveObsoleteStatDegrees ();

   /***** 1. If a degree is not in table of computed degrees,
             choose it as least recently updated *****/
   if ((DegCod = Pho_DB_GetADegWithStdsNotInTableOfComputedDegs ()) > 0)
      return DegCod;

   /***** 2. If all degrees are in table,
	     choose the least recently updated that has students *****/
   return Pho_DB_GetDegWithAvgPhotoLeastRecentlyUpdated ();
  }

/*****************************************************************************/
/***************** Get time when average photo was computed ******************/
/*****************************************************************************/

static long Pho_GetTimeAvgPhotoWasComputed (long DegCod)
  {
   MYSQL_RES *mysql_res;
   MYSQL_ROW row;
   long TimeAvgPhotoWasComputed = 0L;

   /***** Get last time an average photo was computed from database *****/
   if (Pho_DB_GetTimeAvgPhotoWasComputed (&mysql_res,DegCod) == 1)
     {
      /***** Get row *****/
      row = mysql_fetch_row (mysql_res);

      /***** Get time when average photo was computed (row[0]) *****/
      if (row[0])
         sscanf (row[0],"%ld",&TimeAvgPhotoWasComputed);
     }

   /***** Free structure that stores the query result *****/
   DB_FreeMySQLResult (&mysql_res);

   return TimeAvgPhotoWasComputed;
  }

/*****************************************************************************/
/********************* Get time to compute average photo *********************/
/*****************************************************************************/

static long Pho_GetTimeToComputeAvgPhoto (long DegCod)
  {
   MYSQL_RES *mysql_res;
   MYSQL_ROW row;
   Usr_Sex_t Sex;
   long PartialTimeToComputeAvgPhoto;
   long TotalTimeToComputeAvgPhoto = -1L;

   /***** Get time to compute average photo from database *****/
   if (Pho_DB_GetTimeToComputeAvgPhoto (&mysql_res,DegCod) == Usr_NUM_SEXS)
     {
      TotalTimeToComputeAvgPhoto = 0;
      for (Sex  = (Usr_Sex_t) 0;
	   Sex <= (Usr_Sex_t) (Usr_NUM_SEXS - 1);
	   Sex++)
        {
         /***** Get row *****/
         row = mysql_fetch_row (mysql_res);

         /***** Get time to compute average photo (row[0]) *****/
         PartialTimeToComputeAvgPhoto = -1L;
         if (row[0][0])
            if (sscanf (row[0],"%ld",&PartialTimeToComputeAvgPhoto) != 1)
               PartialTimeToComputeAvgPhoto = -1L;
         if (PartialTimeToComputeAvgPhoto < 0)
           {
            TotalTimeToComputeAvgPhoto = -1L;
            break;
           }
         TotalTimeToComputeAvgPhoto += PartialTimeToComputeAvgPhoto;
        }
     }

   /***** Free structure that stores the query result *****/
   DB_FreeMySQLResult (&mysql_res);

   return TotalTimeToComputeAvgPhoto;
  }

/*****************************************************************************/
/********************** Compute the average photo of a list ******************/
/*****************************************************************************/
// Returns number of users in list with photo

static void Pho_ComputeAveragePhoto (long DegCod,Usr_Sex_t Sex,Rol_Role_t Role,
                                     Pho_AvgPhotoTypeOfAverage_t TypeOfAverage,const char *DirAvgPhotosRelPath,
                                     unsigned *NumStds,unsigned *NumStdsWithPhoto,long *TimeToComputeAvgPhotoInMicroseconds)
  {
   extern const char *Usr_StringsSexDB[Usr_NUM_SEXS];
   unsigned NumUsr;
   char PathPrivRelPhoto[PATH_MAX + 1];	// Relative path to private photo, to calculate average face
   char *PathRelAvgPhoto;
   char *FileNamePhotoNames;
   FILE *FilePhotoNames = NULL;	// Initialized to avoid warning
   char StrCallToProgram[3 * (PATH_MAX + 1)];	// Call to photo processing program
   int ReturnCode;
   /* To compute execution time of this function */
   struct timeval tvStartComputingStat;
   struct timeval tvEndComputingStat;
   struct timezone tz;

   /***** Set start time to compute the stats of this degree *****/
   gettimeofday (&tvStartComputingStat,&tz);

   /***** Reset number of students in this degree *****/
   *NumStds = *NumStdsWithPhoto = 0;

   /***** Build name for file with average photo *****/
   if (asprintf (&PathRelAvgPhoto,"%s/%ld_%s.jpg",
                 DirAvgPhotosRelPath,DegCod,Usr_StringsSexDB[Sex]) < 0)
      Err_NotEnoughMemoryExit ();

   /***** Remove old file if exists *****/
   if (Fil_CheckIfPathExists (PathRelAvgPhoto))  // If file exists
      unlink (PathRelAvgPhoto);

   /***** Build names for text file with photo paths *****/
   if (asprintf (&FileNamePhotoNames,"%s/%ld.txt",
	         Cfg_PATH_PHOTO_TMP_PRIVATE,DegCod) < 0)
      Err_NotEnoughMemoryExit ();
   if ((FilePhotoNames = fopen (FileNamePhotoNames,"wb")) == NULL)
      Err_ShowErrorAndExit ("Can not open file to compute average photo.");

   /***** Loop writing file names in text file *****/
   for (NumUsr = 0;
	NumUsr < Gbl.Usrs.LstUsrs[Role].NumUsrs;
	NumUsr++)
     {
      Gbl.Usrs.Other.UsrDat.Sex = Gbl.Usrs.LstUsrs[Role].Lst[NumUsr].Sex;
      if (Sex == Usr_SEX_ALL || Sex == Gbl.Usrs.Other.UsrDat.Sex)
	{
	 (*NumStds)++;

	 /***** Add photo to file for average face calculation *****/
	 Gbl.Usrs.Other.UsrDat.UsrCod = Gbl.Usrs.LstUsrs[Role].Lst[NumUsr].UsrCod;
	 if (Pho_CheckIfPrivPhotoExists (Gbl.Usrs.Other.UsrDat.UsrCod,PathPrivRelPhoto))
	   {
	    (*NumStdsWithPhoto)++;
	    fprintf (FilePhotoNames,"%s\n",PathPrivRelPhoto);
	   }
	}
     }
   fclose (FilePhotoNames);

   /***** Call to program to calculate average photo *****/
   if (*NumStdsWithPhoto)
     {
      snprintf (StrCallToProgram,sizeof (StrCallToProgram),"%s %s %s",
	        Pho_StrAvgPhotoPrograms[TypeOfAverage],
	        FileNamePhotoNames,PathRelAvgPhoto);
      ReturnCode = system (StrCallToProgram);
      if (ReturnCode == -1)
	 Err_ShowErrorAndExit ("Error when running program that computes the average photo.");

      /* Write message depending on the return code */
      if (WEXITSTATUS(ReturnCode))
	 Err_ShowErrorAndExit ("The average photo has not been computed successfully.");
     }

   free (PathRelAvgPhoto);
   free (FileNamePhotoNames);

   /***** Time used to compute the stats of this degree *****/
   if (gettimeofday (&tvEndComputingStat, &tz))
      // Error in gettimeofday
      *TimeToComputeAvgPhotoInMicroseconds = -1L;
   else
     {
      if (tvEndComputingStat.tv_usec < tvStartComputingStat.tv_usec)
	{
	 tvEndComputingStat.tv_sec--;
	 tvEndComputingStat.tv_usec += 1000000L;
	}
      *TimeToComputeAvgPhotoInMicroseconds = (tvEndComputingStat.tv_sec -
	                                      tvStartComputingStat.tv_sec) * 1000000L +
	                                      tvEndComputingStat.tv_usec -
	                                      tvStartComputingStat.tv_usec;
     }
  }

/*****************************************************************************/
/*** Show class photo with average photos of all students from each degree ***/
/*****************************************************************************/

void Pho_ShowPhotoDegree (void)
  {
   Pho_ShowOrPrintPhotoDegree (Pho_DEGREES_SEE);
  }

/*****************************************************************************/
/** Print class photo with average photos of all students from each degree ***/
/*****************************************************************************/

void Pho_PrintPhotoDegree (void)
  {
   Pho_ShowOrPrintPhotoDegree (Pho_DEGREES_PRINT);
  }

/*****************************************************************************/
/*** Show class photo with average photos of all students from each degree ***/
/*****************************************************************************/

static void Pho_ShowOrPrintPhotoDegree (Pho_AvgPhotoSeeOrPrint_t SeeOrPrint)
  {
   extern const char *Hlp_ANALYTICS_Degrees;
   extern const char *Txt_Degrees;
   static void (*ShowOrPrintDegrees[Set_NUM_USR_LIST_TYPES]) (struct Pho_DegPhotos *DegPhotos,
                                                              Pho_AvgPhotoSeeOrPrint_t SeeOrPrint) =
     {
      [Set_USR_LIST_AS_CLASS_PHOTO] = Pho_ShowOrPrintClassPhotoDegrees,
      [Set_USR_LIST_AS_LISTING	  ] = Pho_ShowOrPrintListDegrees,
     };
   struct Pho_DegPhotos DegPhotos;

   /***** Get parameters from form *****/
   DegPhotos.TypeOfAverage       = Pho_GetPhotoAvgTypeFromForm ();
   DegPhotos.HowComputePhotoSize = Pho_GetHowComputePhotoSizeFromForm ();
   DegPhotos.HowOrderDegrees     = Pho_GetHowOrderDegreesFromForm ();

   /***** Get and update type of list,
          number of columns in class photo
          and preference about view photos *****/
   Set_GetAndUpdatePrefsAboutUsrList ();

   switch (SeeOrPrint)
     {
      case Pho_DEGREES_SEE:
	 /***** Begin box *****/
	 Box_BoxBegin (NULL,Txt_Degrees,
	               Pho_PutIconToPrintDegreeStats,&DegPhotos,
		       Hlp_ANALYTICS_Degrees,Box_NOT_CLOSABLE);

	    HTM_TABLE_BeginCenterPadding (2);

	       /***** Put a selector for the type of average *****/
	       Pho_PutSelectorForTypeOfAvg (&DegPhotos);

	       /***** Put a selector for the size of photos *****/
	       Pho_PutSelectorForHowComputePhotoSize (&DegPhotos);

	       /***** Put a selector for the order of degrees *****/
	       Pho_PutSelectorForHowOrderDegrees (&DegPhotos);

	    HTM_TABLE_End ();

	    /***** Link to compute average photos *****/
	    Pho_PutLinkToCalculateDegreeStats (&DegPhotos);

	 break;
      case Pho_DEGREES_PRINT:
	 /***** Begin box *****/
	 Box_BoxBegin (NULL,Txt_Degrees,
	               NULL,NULL,
	               NULL,Box_NOT_CLOSABLE);
	 break;
     }

   /***** Get maximum number of students
          and maximum number of students with photo
          in all degrees *****/
   Pho_GetMaxStdsPerDegree (&DegPhotos);

   /***** Draw the classphoto/list *****/
   if (ShowOrPrintDegrees[Gbl.Usrs.Me.ListType])
      ShowOrPrintDegrees[Gbl.Usrs.Me.ListType] (&DegPhotos,SeeOrPrint);

   /***** End box *****/
   Box_BoxEnd ();
  }

/*****************************************************************************/
/**************** Put parameter for degree average photos ********************/
/*****************************************************************************/

static void Pho_PutParamsDegPhoto (void *DegPhotos)
  {
   if (DegPhotos)
     {
      Pho_PutHiddenParamTypeOfAvg (((struct Pho_DegPhotos *) DegPhotos)->TypeOfAverage);
      Pho_PutHiddenParamPhotoSize (((struct Pho_DegPhotos *) DegPhotos)->HowComputePhotoSize);
      Pho_PutHiddenParamOrderDegrees (((struct Pho_DegPhotos *) DegPhotos)->HowOrderDegrees);
     }
  }

/*****************************************************************************/
/******************* Put a selector for the type of average ******************/
/*****************************************************************************/

static void Pho_PutSelectorForTypeOfAvg (const struct Pho_DegPhotos *DegPhotos)
  {
   extern const char *Txt_Average_type;
   extern const char *Txt_AVERAGE_PHOTO_TYPES[Pho_NUM_AVERAGE_PHOTO_TYPES];
   Pho_AvgPhotoTypeOfAverage_t TypeOfAvg;
   unsigned TypeOfAvgUnsigned;

   /***** Type of average *****/
   HTM_TR_Begin (NULL);

      /* Label */
      Frm_LabelColumn ("RT","AvgType",Txt_Average_type);

      /* Data */
      HTM_TD_Begin ("class=\"LT\"");
	 Frm_BeginForm (ActSeePhoDeg);
	    Pho_PutHiddenParamPhotoSize (DegPhotos->HowComputePhotoSize);
	    Pho_PutHiddenParamOrderDegrees (DegPhotos->HowOrderDegrees);
	    Set_PutParamsPrefsAboutUsrList ();
	    HTM_SELECT_Begin (HTM_SUBMIT_ON_CHANGE,
			      "id=\"AvgType\" name=\"AvgType\""
			      " class=\"INPUT_%s\"",
			      The_GetSuffix ());
	       for (TypeOfAvg  = (Pho_AvgPhotoTypeOfAverage_t) 0;
		    TypeOfAvg <= (Pho_AvgPhotoTypeOfAverage_t) (Pho_NUM_AVERAGE_PHOTO_TYPES - 1);
		    TypeOfAvg++)
		 {
		  TypeOfAvgUnsigned = (unsigned) TypeOfAvg;
		  HTM_OPTION (HTM_Type_UNSIGNED,&TypeOfAvgUnsigned,
			      TypeOfAvg == DegPhotos->TypeOfAverage,false,
			      "%s",Txt_AVERAGE_PHOTO_TYPES[TypeOfAvg]);
		 }
	    HTM_SELECT_End ();
	 Frm_EndForm ();
      HTM_TD_End ();

   HTM_TR_End ();
  }

/*****************************************************************************/
/**************** Put hidden parameter for the type of average ***************/
/*****************************************************************************/

void Pho_PutHiddenParamTypeOfAvg (Pho_AvgPhotoTypeOfAverage_t TypeOfAverage)
  {
   Par_PutHiddenParamUnsigned (NULL,"AvgType",(unsigned) TypeOfAverage);
  }

/*****************************************************************************/
/*********************** Get type of average for photos **********************/
/*****************************************************************************/

static Pho_AvgPhotoTypeOfAverage_t Pho_GetPhotoAvgTypeFromForm (void)
  {
   return (Pho_AvgPhotoTypeOfAverage_t)
	  Par_GetParToUnsignedLong ("AvgType",
                                    0,
                                    Pho_NUM_AVERAGE_PHOTO_TYPES - 1,
                                    (unsigned long) Pho_PHOTO_AVG_DEFAULT);
  }

/*****************************************************************************/
/****************** Put a selector for the size of photos ********************/
/*****************************************************************************/

static void Pho_PutSelectorForHowComputePhotoSize (const struct Pho_DegPhotos *DegPhotos)
  {
   extern const char *Txt_Size_of_photos;
   extern const char *Txt_STAT_DEGREE_PHOTO_SIZE[Pho_NUM_HOW_COMPUTE_PHOTO_SIZES];
   Pho_HowComputePhotoSize_t PhoSi;
   unsigned PhoSiUnsigned;

   /***** Photo size *****/
   HTM_TR_Begin (NULL);

      /* Label */
      Frm_LabelColumn ("RT","PhotoSize",Txt_Size_of_photos);

      /* Data */
      HTM_TD_Begin ("class=\"LT\"");
	 Frm_BeginForm (ActSeePhoDeg);
	    Pho_PutHiddenParamTypeOfAvg (DegPhotos->TypeOfAverage);
	    Pho_PutHiddenParamOrderDegrees (DegPhotos->HowOrderDegrees);
	    Set_PutParamsPrefsAboutUsrList ();
	    HTM_SELECT_Begin (HTM_SUBMIT_ON_CHANGE,
			      "id=\"PhotoSize\" name=\"PhotoSize\""
			      " class=\"INPUT_%s\"",
			      The_GetSuffix ());
	       for (PhoSi  = (Pho_HowComputePhotoSize_t) 0;
		    PhoSi <= (Pho_HowComputePhotoSize_t) (Pho_NUM_HOW_COMPUTE_PHOTO_SIZES - 1);
		    PhoSi++)
		 {
		  PhoSiUnsigned = (unsigned) PhoSi;
		  HTM_OPTION (HTM_Type_UNSIGNED,&PhoSiUnsigned,
			      PhoSi == DegPhotos->HowComputePhotoSize,false,
			      "%s",Txt_STAT_DEGREE_PHOTO_SIZE[PhoSi]);
		 }
	    HTM_SELECT_End ();
	 Frm_EndForm ();
      HTM_TD_End ();

   HTM_TR_End ();
  }

/*****************************************************************************/
/**************** Put hidden parameter for the size of photos ****************/
/*****************************************************************************/

void Pho_PutHiddenParamPhotoSize (Pho_HowComputePhotoSize_t HowComputePhotoSize)
  {
   Par_PutHiddenParamUnsigned (NULL,"PhotoSize",(unsigned) HowComputePhotoSize);
  }

/*****************************************************************************/
/********************* Get how to compute size of photos *********************/
/*****************************************************************************/

static Pho_HowComputePhotoSize_t Pho_GetHowComputePhotoSizeFromForm (void)
  {
   return (Pho_HowComputePhotoSize_t)
	  Par_GetParToUnsignedLong ("PhotoSize",
                                    0,
                                    Pho_NUM_HOW_COMPUTE_PHOTO_SIZES - 1,
                                    (unsigned long) Pho_PHOTO_SIZE_DEFAULT);
  }

/*****************************************************************************/
/****************** Put a selector for the order of degrees ******************/
/*****************************************************************************/

static void Pho_PutSelectorForHowOrderDegrees (const struct Pho_DegPhotos *DegPhotos)
  {
   extern const char *Txt_Sort_degrees_by;
   extern const char *Txt_STAT_DEGREE_PHOTO_ORDER[Pho_NUM_HOW_ORDER_DEGREES];
   Pho_HowOrderDegrees_t Order;
   unsigned OrderUnsigned;

   /***** Order *****/
   HTM_TR_Begin (NULL);

      /* Label */
      Frm_LabelColumn ("RT","Order",Txt_Sort_degrees_by);

      /* Data */
      HTM_TD_Begin ("class=\"LT\"");
	 Frm_BeginForm (ActSeePhoDeg);
	    Pho_PutHiddenParamTypeOfAvg (DegPhotos->TypeOfAverage);
	    Pho_PutHiddenParamPhotoSize (DegPhotos->HowComputePhotoSize);
	    Set_PutParamsPrefsAboutUsrList ();
	    HTM_SELECT_Begin (HTM_SUBMIT_ON_CHANGE,
			      "id=\"Order\" name=\"Order\" class=\"INPUT_%s\"",
			      The_GetSuffix ());
	       for (Order  = (Pho_HowOrderDegrees_t) 0;
		    Order <= (Pho_HowOrderDegrees_t) (Pho_NUM_HOW_ORDER_DEGREES - 1);
		    Order++)
		 {
		  OrderUnsigned = (unsigned) Order;
		  HTM_OPTION (HTM_Type_UNSIGNED,&OrderUnsigned,
			      Order == DegPhotos->HowOrderDegrees,false,
			      "%s",Txt_STAT_DEGREE_PHOTO_ORDER[Order]);
		 }
	    HTM_SELECT_End ();
	 Frm_EndForm ();
      HTM_TD_End ();

   HTM_TR_End ();
  }

/*****************************************************************************/
/**************** Put hidden parameter for the order of degrees **************/
/*****************************************************************************/

void Pho_PutHiddenParamOrderDegrees (Pho_HowOrderDegrees_t HowOrderDegrees)
  {
   Par_PutHiddenParamUnsigned (NULL,"Order",(unsigned) HowOrderDegrees);
  }

/*****************************************************************************/
/*************************** Get how to order degrees ************************/
/*****************************************************************************/

static Pho_HowOrderDegrees_t Pho_GetHowOrderDegreesFromForm (void)
  {
   return (Pho_HowOrderDegrees_t)
	  Par_GetParToUnsignedLong ("Order",
                                    0,
                                    Pho_NUM_HOW_ORDER_DEGREES - 1,
                                    (unsigned long) Pho_ORDER_DEFAULT);
  }

/*****************************************************************************/
/*************** Put icon to print view the stats of degrees ***************/
/*****************************************************************************/

static void Pho_PutIconToPrintDegreeStats (void *DegPhotos)
  {
   Ico_PutContextualIconToPrint (ActPrnPhoDeg,
				 Pho_PutLinkToPrintViewOfDegreeStatsParams,DegPhotos);
  }

static void Pho_PutLinkToPrintViewOfDegreeStatsParams (void *DegPhotos)
  {
   Pho_PutHiddenParamTypeOfAvg (((struct Pho_DegPhotos *) DegPhotos)->TypeOfAverage);
   Pho_PutHiddenParamPhotoSize (((struct Pho_DegPhotos *) DegPhotos)->HowComputePhotoSize);
   Pho_PutHiddenParamOrderDegrees (((struct Pho_DegPhotos *) DegPhotos)->HowOrderDegrees);
   Set_PutParamsPrefsAboutUsrList ();
  }

/*****************************************************************************/
/*************** Put a link to calculate the stats of degrees ****************/
/*****************************************************************************/

static void Pho_PutLinkToCalculateDegreeStats (const struct Pho_DegPhotos *DegPhotos)
  {
   extern const char *Txt_Calculate_average_photo_of_THE_DEGREE_X;
   extern const char *Txt_unknown_TIME;
   extern const char *Txt_time;
   struct ListDegrees Degs;
   unsigned NumDeg;
   struct Deg_Degree Deg;
   long EstimatedTimeToComputeAvgPhotoInMicroseconds;
   char StrEstimatedTimeToComputeAvgPhoto[Dat_MAX_BYTES_TIME + 1];
   bool Selected;
   bool Disabled;

   if ((Deg.DegCod = Pho_GetDegWithAvgPhotoLeastRecentlyUpdated ()) > 0)
     {
      /***** Get list of all degrees *****/
      Deg_GetListAllDegsWithStds (&Degs);

      /***** Get data of the degree from database *****/
      Deg_GetDataOfDegreeByCod (&Deg);

      /***** Contextual menu *****/
      HTM_DIV_Begin ("class=\"UPD\"");

	 /* Begin form */
	 Frm_BeginForm (ActCalPhoDeg);
	    Pho_PutHiddenParamTypeOfAvg (DegPhotos->TypeOfAverage);
	    Pho_PutHiddenParamPhotoSize (DegPhotos->HowComputePhotoSize);
	    Pho_PutHiddenParamOrderDegrees (DegPhotos->HowOrderDegrees);
	    Set_PutParamsPrefsAboutUsrList ();

	    /***** Put button to refresh *****/
	    HTM_BUTTON_Submit_Begin (Txt_Calculate_average_photo_of_THE_DEGREE_X,
				     "class=\"BT_LINK FORM_IN_%s BOLD\"",
				     The_GetSuffix ());
	       Ico_PutIconTextLink ("recycle.svg",Ico_BLACK,Txt_Calculate_average_photo_of_THE_DEGREE_X);
	    HTM_BUTTON_End ();

	    /* Selector with all degrees with students */
	    HTM_SELECT_Begin (HTM_DONT_SUBMIT_ON_CHANGE,
			      "name=\"OthDegCod\" class=\"INPUT_%s\"",
			      The_GetSuffix ());
	       for (NumDeg = 0;
		    NumDeg < Degs.Num;
		    NumDeg++)
		 {
		  /* Get time to compute average photo of this degree */
		  EstimatedTimeToComputeAvgPhotoInMicroseconds = Pho_GetTimeToComputeAvgPhoto (Degs.Lst[NumDeg].DegCod);
		  if (EstimatedTimeToComputeAvgPhotoInMicroseconds == -1L)
		     Str_Copy (StrEstimatedTimeToComputeAvgPhoto,Txt_unknown_TIME,
			       sizeof (StrEstimatedTimeToComputeAvgPhoto) - 1);
		  else
		     Sta_WriteTime (StrEstimatedTimeToComputeAvgPhoto,
				    EstimatedTimeToComputeAvgPhotoInMicroseconds);

		  Selected = (Degs.Lst[NumDeg].DegCod == Deg.DegCod);
		  if (Selected)
		     Disabled = false;
		  else
		     // Too recently computed ?
		     Disabled = Pho_GetTimeAvgPhotoWasComputed (Degs.Lst[NumDeg].DegCod) >=
				Gbl.StartExecutionTimeUTC - Cfg_MIN_TIME_TO_RECOMPUTE_AVG_PHOTO;
		  HTM_OPTION (HTM_Type_LONG,&Degs.Lst[NumDeg].DegCod,Selected,Disabled,
			      "%s (%s: %s)",
			      Degs.Lst[NumDeg].ShrtName,
			      Txt_time,StrEstimatedTimeToComputeAvgPhoto);
		 }
	    HTM_SELECT_End ();

	 /* End form and contextual menu */
	 Frm_EndForm ();

      HTM_DIV_End ();

      /***** Free list of all degrees with students *****/
      Deg_FreeListDegs (&Degs);
     }
  }

/*****************************************************************************/
/*** Get number of students and number of students with photo in a degree ****/
/*****************************************************************************/

static void Pho_GetMaxStdsPerDegree (struct Pho_DegPhotos *DegPhotos)
  {
   MYSQL_RES *mysql_res;
   MYSQL_ROW row;

   /***** Get maximum number of students in a degree from database *****/
   if (Pho_DB_GetMaxStdsPerDegree (&mysql_res) == 1)
     {
      row = mysql_fetch_row (mysql_res);

      if (row[0] == NULL)
	 DegPhotos->MaxStds = -1;
      else if (sscanf (row[0],"%d",&DegPhotos->MaxStds) != 1)
	 DegPhotos->MaxStds = -1;

      if (row[1] == NULL)
	 DegPhotos->MaxStdsWithPhoto = -1;
      else if (sscanf (row[1],"%d",&DegPhotos->MaxStdsWithPhoto) != 1)
	 DegPhotos->MaxStdsWithPhoto = -1;

      if (row[2] == NULL)
	 DegPhotos->MaxPercent = -1.0;
      else if (sscanf (row[2],"%lf",&DegPhotos->MaxPercent) != 1)
	 DegPhotos->MaxPercent = -1.0;

      /***** Free structure that stores the query result *****/
      DB_FreeMySQLResult (&mysql_res);
     }
   else
     {
      DegPhotos->MaxStds = DegPhotos->MaxStdsWithPhoto = -1;
      DegPhotos->MaxPercent = -1.0;
     }
  }

/*****************************************************************************/
/************ Show or print the stats of degrees as class photo **************/
/*****************************************************************************/

static void Pho_ShowOrPrintClassPhotoDegrees (struct Pho_DegPhotos *DegPhotos,
                                              Pho_AvgPhotoSeeOrPrint_t SeeOrPrint)
  {
   MYSQL_RES *mysql_res;
   unsigned long NumDeg;
   unsigned long NumDegs;
   struct Deg_Degree Deg;
   unsigned NumDegsNotEmpty;
   int NumStds;
   int NumStdsWithPhoto;
   bool TRIsOpen = false;

   /***** Get degrees from database *****/
   if ((NumDegs = Pho_DB_QueryDegrees (&mysql_res,DegPhotos->HowOrderDegrees)))	// Degrees with students found
     {
      /***** Form to select type of list used to display degree photos *****/
      if (SeeOrPrint == Pho_DEGREES_SEE)
	 Usr_ShowFormsToSelectUsrListType (Pho_PutParamsDegPhoto,DegPhotos);

      HTM_TABLE_BeginCenter ();

	 /***** Get and print degrees *****/
	 for (NumDeg = 0, NumDegsNotEmpty = 0;
	      NumDeg < NumDegs;
	      NumDeg++)
	   {
	    /***** Get next degree *****/
	    if ((Deg.DegCod = DB_GetNextCode (mysql_res)) < 0)
	       Err_WrongDegreeExit ();

	    /* Get data of degree */
	    Deg_GetDataOfDegreeByCod (&Deg);

	    /* Get number of students and number of students with photo in this degree */
	    Pho_GetNumStdsInDegree (Deg.DegCod,Usr_SEX_ALL,&NumStds,&NumStdsWithPhoto);

	    if (NumStds > 0)
	      {
	       if ((NumDegsNotEmpty % Gbl.Usrs.ClassPhoto.Cols) == 0)
		 {
		  HTM_TR_Begin (NULL);
		  TRIsOpen = true;
		 }

	       /***** Show average photo of students belonging to this degree *****/
	       HTM_TD_Begin ("class=\"CLASSPHOTO CLASSPHOTO_%s CM\"",
	                     The_GetSuffix ());
		  Pho_ShowDegreeAvgPhotoAndStat (&Deg,DegPhotos,
						 SeeOrPrint,
						 Usr_SEX_ALL,
						 NumStds,NumStdsWithPhoto);
	       HTM_TD_End ();

	       if ((++NumDegsNotEmpty % Gbl.Usrs.ClassPhoto.Cols) == 0)
		 {
		  HTM_TR_End ();
		  TRIsOpen = false;
		 }
	      }
	   }
	 if (TRIsOpen)
	    HTM_TR_End ();

      HTM_TABLE_End ();
     }
   else	// No degrees with students found
      /***** Show warning indicating no students found *****/
      Usr_ShowWarningNoUsersFound (Rol_STD);

   /***** Free structure that stores the query result *****/
   DB_FreeMySQLResult (&mysql_res);
  }

/*****************************************************************************/
/**************** Show or print the stats of degrees as list *****************/
/*****************************************************************************/

static void Pho_ShowOrPrintListDegrees (struct Pho_DegPhotos *DegPhotos,
                                        Pho_AvgPhotoSeeOrPrint_t SeeOrPrint)
  {
   extern const char *Txt_No_INDEX;
   extern const char *Txt_Degree;
   extern const char *Txt_SEX_PLURAL_Abc[Usr_NUM_SEXS];
   MYSQL_RES *mysql_res;
   MYSQL_ROW row;
   unsigned long NumDeg;
   unsigned long NumDegs;
   unsigned NumDegsNotEmpty;
   int NumStds;
   int NumStdsWithPhoto;
   struct Deg_Degree Deg;
   Usr_Sex_t Sex;

   /***** Get degrees from database *****/
   if ((NumDegs = Pho_DB_QueryDegrees (&mysql_res,DegPhotos->HowOrderDegrees)))	// Degrees with students found
     {
      /***** Class photo start *****/
      if (SeeOrPrint == Pho_DEGREES_SEE)
	 /***** Form to select type of list used to display degree photos *****/
	 Usr_ShowFormsToSelectUsrListType (Pho_PutParamsDegPhoto,DegPhotos);

      /***** Write heading *****/
      HTM_TABLE_BeginCenterPadding (2);
	 HTM_TR_Begin (NULL);
	    HTM_TH (Txt_No_INDEX,HTM_HEAD_RIGHT);
	    HTM_TH (Txt_Degree  ,HTM_HEAD_CENTER);
	    for (Sex  = (Usr_Sex_t) 0;
		 Sex <= (Usr_Sex_t) (Usr_NUM_SEXS - 1);
		 Sex++)
	       HTM_TH (Txt_SEX_PLURAL_Abc[Sex],HTM_HEAD_CENTER);
	 HTM_TR_End ();

	 /***** Get degrees *****/
	 for (NumDeg = 0, NumDegsNotEmpty = 0;
	      NumDeg < NumDegs;
	      NumDeg++, The_ChangeRowColor ())
	   {
	    /***** Get next degree *****/
	    row = mysql_fetch_row (mysql_res);

	    /* Get degree code (row[0]) */
	    if ((Deg.DegCod = Str_ConvertStrCodToLongCod (row[0])) <= 0)
	       Err_WrongDegreeExit ();

	    /* Get data of degree */
	    Deg_GetDataOfDegreeByCod (&Deg);

	    HTM_TR_Begin (NULL);

	       /***** Show logo and name of this degree *****/
	       HTM_TD_Begin ("class=\"RM DAT_%s %s\"",
	                     The_GetSuffix (),
	                     The_GetColorRows ());
		  HTM_Unsigned (++NumDegsNotEmpty);
	       HTM_TD_End ();

	       /***** Show logo and name of this degree *****/
	       HTM_TD_Begin ("class=\"LM DAT_%s %s\"",
	                     The_GetSuffix (),
	                     The_GetColorRows ());
		  if (SeeOrPrint == Pho_DEGREES_SEE)
		     Deg_DrawDegreeLogoAndNameWithLink (&Deg,ActSeeDegInf,"CT");
		  else	// Pho_DEGREES_PRINT
		    {
		     Lgo_DrawLogo (HieLvl_DEG,Deg.DegCod,Deg.ShrtName,20,"CT",true);
		     HTM_TxtF ("&nbsp;%s",Deg.FullName);
		    }
	       HTM_TD_End ();

	       for (Sex  = (Usr_Sex_t) 0;
		    Sex <= (Usr_Sex_t) (Usr_NUM_SEXS - 1);
		    Sex++)
		 {
		  /***** Show average photo of students belonging to this degree *****/
		  Pho_GetNumStdsInDegree (Deg.DegCod,Sex,&NumStds,&NumStdsWithPhoto);
		  HTM_TD_Begin ("class=\"CLASSPHOTO CLASSPHOTO_%s RM %s\"",
		                The_GetSuffix (),
		                The_GetColorRows ());
		     if (Gbl.Usrs.Listing.WithPhotos)
			Pho_ShowDegreeAvgPhotoAndStat (&Deg,DegPhotos,
						       SeeOrPrint,Sex,
						       NumStds,NumStdsWithPhoto);
		     else
			Pho_ShowDegreeStat (NumStds,NumStdsWithPhoto);
		  HTM_TD_End ();
		 }

	    HTM_TR_End ();
	   }

      /***** Photos end *****/
      HTM_TABLE_End ();
     }
   else	// No degrees with students found!
      /***** Show warning indicating no students found *****/
      Usr_ShowWarningNoUsersFound (Rol_STD);

   /***** Free structure that stores the query result *****/
   DB_FreeMySQLResult (&mysql_res);
  }

/*****************************************************************************/
/*** Get number of students and number of students with photo in a degree ****/
/*****************************************************************************/

static void Pho_GetNumStdsInDegree (long DegCod,Usr_Sex_t Sex,
                                    int *NumStds,int *NumStdsWithPhoto)
  {
   MYSQL_RES *mysql_res;
   MYSQL_ROW row;

   /***** Get the number of students in a degree from database *****/
   if (Pho_DB_GetNumStdsInDegree (&mysql_res,DegCod,Sex))
     {
      row = mysql_fetch_row (mysql_res);

      /* Get number of students (row[0])
         and number of students with photo (row[1]) */
      if (sscanf (row[0],"%d",NumStds) != 1)
	 *NumStds = -1;
      if (sscanf (row[1],"%d",NumStdsWithPhoto) != 1)
	 *NumStdsWithPhoto = -1;

      /***** Free structure that stores the query result *****/
      DB_FreeMySQLResult (&mysql_res);
     }
   else
      *NumStds = *NumStdsWithPhoto = -1;
  }

/*****************************************************************************/
/******************* Show the average photo of a degree **********************/
/*****************************************************************************/

static void Pho_ShowDegreeStat (int NumStds,int NumStdsWithPhoto)
  {
   extern const char *Txt_photos;

   HTM_SPAN_Begin ("class=\"DAT_%s\"",The_GetSuffix ());
      HTM_TxtF ("%d&nbsp;",NumStds);
   HTM_SPAN_End ();

   HTM_SPAN_Begin ("class=\"DAT_SMALL_%s\"",The_GetSuffix ());
      HTM_TxtF ("(%d&nbsp;%s,&nbsp;%d%%)",
		NumStdsWithPhoto,Txt_photos,
		NumStds > 0 ? (int) (((NumStdsWithPhoto * 100.0) / NumStds) + 0.5) :
			      0);
   HTM_SPAN_End ();
  }

/*****************************************************************************/
/******************* Show the average photo of a degree **********************/
/*****************************************************************************/

static void Pho_ShowDegreeAvgPhotoAndStat (const struct Deg_Degree *Deg,
                                           const struct Pho_DegPhotos *DegPhotos,
                                           Pho_AvgPhotoSeeOrPrint_t SeeOrPrint,
                                           Usr_Sex_t Sex,
                                           int NumStds,int NumStdsWithPhoto)
  {
   extern const char *Usr_StringsSexDB[Usr_NUM_SEXS];
   extern const char *Txt_students_ABBREVIATION;
   extern const char *Txt_SEX_PLURAL_abc[Usr_NUM_SEXS];
   extern const char *Txt_photos;
   unsigned PhotoWidth;
   unsigned PhotoHeight;
   char PathRelAvgPhoto[PATH_MAX + 1];
   char PhotoURL[PATH_MAX + 1];
   char PhotoCaption[1024 + Cns_HIERARCHY_MAX_BYTES_SHRT_NAME];
   bool ShowDegPhoto;
   char IdCaption[Frm_MAX_BYTES_ID + 1];

   /***** Initializations *****/
   PhotoURL[0] = '\0';
   PhotoCaption[0] = '\0';

   /***** Compute photo width and height
          to be proportional to number of students *****/
   Pho_ComputePhotoSize (DegPhotos,
                         NumStds,NumStdsWithPhoto,
                         &PhotoWidth,&PhotoHeight);

   /***** Put link to degree *****/
   if (SeeOrPrint == Pho_DEGREES_SEE)
     {
      Frm_BeginFormGoTo (ActSeeDegInf);
	 Deg_PutParamDegCod (Deg->DegCod);
	 HTM_BUTTON_Submit_Begin (Str_BuildGoToTitle (Deg->FullName),
	                          "class=\"BT_LINK\"");
	 Str_FreeGoToTitle ();
     }

   /***** Check if photo of degree can be shown *****/
   if (Gbl.Usrs.Me.Role.Logged == Rol_SYS_ADM)
      ShowDegPhoto = (NumStds > 0);
   else
      ShowDegPhoto = (NumStds > 0 &&
		      NumStdsWithPhoto >= Cfg_MIN_PHOTOS_TO_SHOW_AVERAGE);

   if (ShowDegPhoto)
     {
      snprintf (PathRelAvgPhoto,sizeof (PathRelAvgPhoto),"%s/%s/%ld_%s.jpg",
	        Cfg_PATH_PHOTO_PUBLIC,
	        Pho_StrAvgPhotoDirs[DegPhotos->TypeOfAverage],
	        Deg->DegCod,Usr_StringsSexDB[Sex]);
      if (Fil_CheckIfPathExists (PathRelAvgPhoto))
	{
	 snprintf (PhotoURL,sizeof (PhotoURL),"%s/%s/%ld_%s.jpg",
		   Cfg_URL_PHOTO_PUBLIC,
		   Pho_StrAvgPhotoDirs[DegPhotos->TypeOfAverage],
		   Deg->DegCod,Usr_StringsSexDB[Sex]);
         if (SeeOrPrint == Pho_DEGREES_SEE)
           {
            /***** Hidden div to pass user's name to Javascript *****/
	    snprintf (PhotoCaption,sizeof (PhotoCaption),
		      "%s<br />"
		      "%d&nbsp;%s&nbsp;(%s)<br />"
		      "%d&nbsp;%s&nbsp;(%d%%)",
		      Deg->ShrtName,
		      NumStds,Txt_students_ABBREVIATION,Txt_SEX_PLURAL_abc[Sex],
		      NumStdsWithPhoto,Txt_photos,
		      NumStds > 0 ? (int) (((NumStdsWithPhoto * 100.0) / NumStds) + 0.5) :
				    0);
	    Frm_SetUniqueId (IdCaption);
	    HTM_DIV_Begin ("id=\"%s\" class=\"NOT_SHOWN\"",IdCaption);
	       HTM_DIV_Begin ("class=\"ZOOM_TXT DAT_STRONG_%s\"",
	                      The_GetSuffix ());
		  HTM_Txt (PhotoCaption);
	       HTM_DIV_End ();
	    HTM_DIV_End ();
	   }
	}
     }

   /***** Show photo *****/
   if (PhotoURL[0])
     {
      if (PhotoCaption[0])
         HTM_IMG (PhotoURL,NULL,Deg->ShrtName,
	          " style=\"width:%upx;height:%upx;\""
		  " onmouseover=\"zoom(this,'%s','%s');\""
	          " onmouseout=\"noZoom();\"",
	          PhotoWidth,PhotoHeight,
		  PhotoURL,IdCaption);
      else
         HTM_IMG (PhotoURL,NULL,Deg->ShrtName,
                  " style=\"width:%upx;height:%upx;\" />",
	          PhotoWidth,PhotoHeight);
     }
   else
      HTM_IMG (Cfg_URL_ICON_PUBLIC,"usr_bl.jpg",Deg->ShrtName,
	       "style=\"width:%upx;height:%upx;\"",
	       PhotoWidth,PhotoHeight);

   /***** Caption *****/
   HTM_DIV_Begin ("class=\"CLASSPHOTO_CAPTION CLASSPHOTO_%s\"",
                  The_GetSuffix ());
      HTM_Txt (Deg->ShrtName);
      HTM_BR ();
      HTM_TxtF ("%d&nbsp;%s",NumStds,Txt_students_ABBREVIATION);
      HTM_BR ();
      HTM_TxtF ("%d&nbsp;%s",NumStdsWithPhoto,Txt_photos);
      HTM_BR ();
      HTM_TxtF ("(%d%%)",
		NumStds > 0 ? (int) (((NumStdsWithPhoto * 100.0) / NumStds) + 0.5) :
			      0);
   HTM_DIV_End ();

   if (SeeOrPrint == Pho_DEGREES_SEE)
     {
	 HTM_BUTTON_End ();
      Frm_EndForm ();
     }
  }

/*****************************************************************************/
/******** Compute photo width and height for class photo of degrees **********/
/*****************************************************************************/
#define MIN_WIDTH_PHOTO		10
#define MIN_HEIGHT_PHOTO	15
#define MIN_PIXELS_PHOTO	(MIN_WIDTH_PHOTO * MIN_HEIGHT_PHOTO)
#define DEF_WIDTH_PHOTO		45
#define DEF_HEIGHT_PHOTO	60
#define DEF_PIXELS_PHOTO	(DEF_WIDTH_PHOTO * DEF_HEIGHT_PHOTO)
#define MAX_WIDTH_PHOTO		90
#define MAX_HEIGHT_PHOTO	120
#define MAX_PIXELS_PHOTO	(MAX_WIDTH_PHOTO * MAX_HEIGHT_PHOTO)

static void Pho_ComputePhotoSize (const struct Pho_DegPhotos *DegPhotos,
                                  int NumStds,int NumStdsWithPhoto,
                                  unsigned *PhotoWidth,unsigned *PhotoHeight)
  {
   unsigned PhotoPixels = DEF_PIXELS_PHOTO;

   switch (DegPhotos->HowComputePhotoSize)
     {
      case Pho_PROPORTIONAL_TO_NUMBER_OF_STUDENTS:
         if (DegPhotos->MaxStds > 0)
            PhotoPixels = (unsigned) (((double) (MAX_PIXELS_PHOTO - MIN_PIXELS_PHOTO) /
        	                       DegPhotos->MaxStds) * NumStds +
        	                      MIN_PIXELS_PHOTO + 0.5);
         break;
      case Pho_PROPORTIONAL_TO_NUMBER_OF_PHOTOS:
         if (DegPhotos->MaxStdsWithPhoto > 0)
            PhotoPixels = (unsigned) (((double) (MAX_PIXELS_PHOTO - MIN_PIXELS_PHOTO) /
        	                       DegPhotos->MaxStdsWithPhoto) * NumStdsWithPhoto +
        	                      MIN_PIXELS_PHOTO + 0.5);
         break;
      case Pho_PROPORTIONAL_TO_PERCENT:
         if (DegPhotos->MaxPercent > 0.0)
           {
            if (NumStds)
               PhotoPixels = (unsigned) (((double) (MAX_PIXELS_PHOTO - MIN_PIXELS_PHOTO) /
        	                          DegPhotos->MaxPercent) *
        	                         ((double) NumStdsWithPhoto / NumStds) +
        	                         MIN_PIXELS_PHOTO + 0.5);
            else
              {
               *PhotoWidth  = MIN_WIDTH_PHOTO;
               *PhotoHeight = MIN_HEIGHT_PHOTO;
               return;
              }
           }
         break;
      case Pho_UNIQUE_SIZE:
         *PhotoWidth  = DEF_WIDTH_PHOTO;
         *PhotoHeight = DEF_HEIGHT_PHOTO;
         return;
     }
   *PhotoWidth  = (unsigned) (sqrt (((double) Pho_PHOTO_REAL_WIDTH /
	                             (double) Pho_PHOTO_REAL_HEIGHT) *
	                            (double) PhotoPixels) + 0.5);
   *PhotoHeight = (unsigned) (((double) Pho_PHOTO_REAL_HEIGHT /
	                       (double) Pho_PHOTO_REAL_WIDTH) *
	                      (double) *PhotoWidth + 0.5);
  }
