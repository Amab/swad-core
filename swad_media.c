// swad_media.c: processing of image/video uploaded in a form

/*
    SWAD (Shared Workspace At a Distance),
    is a web platform developed at the University of Granada (Spain),
    and used to support university teaching.

    This file is part of SWAD core.
    Copyright (C) 1999-2019 Antonio Ca�as Vargas

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

#define _GNU_SOURCE         	// For strcasestr, asprintf
#include <linux/limits.h>	// For PATH_MAX
#include <stdbool.h>		// For boolean type
#include <stdio.h>		// For asprintf
#include <stdlib.h>		// For exit, system, malloc, free, etc
#include <string.h>		// For string functions
#include <sys/stat.h>		// For lstat
#include <sys/types.h>		// For lstat
#include <sys/wait.h>		// For the macro WEXITSTATUS
#include <unistd.h>		// For unlink, lstat

#include "swad_box.h"
#include "swad_config.h"
#include "swad_cookie.h"
#include "swad_database.h"
#include "swad_file.h"
#include "swad_file_browser.h"
#include "swad_form.h"
#include "swad_global.h"
#include "swad_HTML.h"
#include "swad_media.h"

/*****************************************************************************/
/****************************** Public constants *****************************/
/*****************************************************************************/

/*****************************************************************************/
/***************************** Internal constants ****************************/
/*****************************************************************************/

const char *Med_StringsTypeDB[Med_NUM_TYPES] =
  {
   "none",	// Med_NONE
   "jpg",	// Med_JPG
   "gif",	// Med_GIF
   "mp4",	// Med_MP4
   "webm",	// Med_WEBM
   "ogg",	// Med_OGG
   "youtube",	// Med_YOUTUBE
   "embed",	// Med_EMBED
   };

const char *Med_Extensions[Med_NUM_TYPES] =
  {
   "",		// Med_NONE
   "jpg",	// Med_JPG
   "gif",	// Med_GIF
   "mp4",	// Med_MP4
   "webm",	// Med_WEBM
   "ogg",	// Med_OGG
   "",		// Med_YOUTUBE
   "",		// Med_EMBED
   };

#define Med_MAX_SIZE_GIF (5UL * 1024UL * 1024UL)	// 5 MiB
#define Med_MAX_SIZE_MP4 (5UL * 1024UL * 1024UL)	// 5 MiB

/*****************************************************************************/
/****************************** Internal types *******************************/
/*****************************************************************************/

#define Med_NUM_FORM_TYPES 4

typedef enum
  {
   Med_FORM_NONE    = 0,
   Med_FORM_FILE    = 1,
   Med_FORM_YOUTUBE = 2,
   Med_FORM_EMBED   = 3,
  } Med_FormType_t;

struct MediaUploader
  {
   Med_FormType_t FormType;
   const char *IconSuffix;
   const char *ParamSuffix;
   const char *FunctionName;
   const char *Icon;
   const char *Title;
  };

/*****************************************************************************/
/************** External global variables from others modules ****************/
/*****************************************************************************/

extern struct Globals Gbl;

/*****************************************************************************/
/************************* Internal global variables *************************/
/*****************************************************************************/

/*****************************************************************************/
/***************************** Internal prototypes ***************************/
/*****************************************************************************/

static void Med_ResetMediaExceptURLAndTitle (struct Media *Media);
static void Med_FreeMediaURL (struct Media *Media);
static void Med_FreeMediaTitle (struct Media *Media);

static void Med_PutIconMediaUploader (const char UniqueId[Frm_MAX_BYTES_ID + 1],
				      struct MediaUploader *MediaUploader);
static void Med_PutHiddenFormTypeMediaUploader (const char UniqueId[Frm_MAX_BYTES_ID + 1],
						struct MediaUploader *MediaUploader,
					        struct ParamUploadMedia *ParamUploadMedia);

static Med_Action_t Med_GetMediaActionFromForm (const char *ParamAction);
static Med_FormType_t Usr_GetFormTypeFromForm (struct ParamUploadMedia *ParamUploadMedia);
static void Usr_GetURLFromForm (const char *ParamName,struct Media *Media);
static void Usr_GetTitleFromForm (const char *ParamName,struct Media *Media);
static void Med_GetAndProcessFileFromForm (const char *ParamFile,
                                           struct Media *Media);
static bool Med_DetectIfAnimated (struct Media *Media,
			          const char PathFileOrg[PATH_MAX + 1]);

static void Med_ProcessJPG (struct Media *Media,
			    const char PathFileOrg[PATH_MAX + 1]);
static void Med_ProcessGIF (struct Media *Media,
			    const char PathFileOrg[PATH_MAX + 1]);
static void Med_ProcessVideo (struct Media *Media,
			      const char PathFileOrg[PATH_MAX + 1]);

static int Med_ResizeImage (struct Media *Media,
                            const char PathFileOriginal[PATH_MAX + 1],
                            const char PathFileProcessed[PATH_MAX + 1]);
static int Med_GetFirstFrame (const char PathFileOriginal[PATH_MAX + 1],
                              const char PathFileProcessed[PATH_MAX + 1]);

static void Med_GetAndProcessYouTubeFromForm (const char *ParamURL,
                                              struct Media *Media);
static void Med_GetAndProcessEmbedFromForm (const char *ParamURL,
                                            struct Media *Media);

static bool Med_MoveTmpFileToDefDir (struct Media *Media,
				     const char PathMedPriv[PATH_MAX + 1],
				     const char *Extension);

static void Med_ShowJPG (struct Media *Media,
			 const char PathMedPriv[PATH_MAX + 1],
			 const char *ClassMedia);
static void Med_ShowGIF (struct Media *Media,
			 const char PathMedPriv[PATH_MAX + 1],
			 const char *ClassMedia);
static void Med_ShowVideo (struct Media *Media,
			   const char PathMedPriv[PATH_MAX + 1],
			   const char *ClassMedia);
static void Med_ShowYoutube (struct Media *Media,const char *ClassMedia);
static void Med_ShowEmbed (struct Media *Media,const char *ClassMedia);
static void Med_AlertThirdPartyCookies (void);

static Med_Type_t Med_GetTypeFromStrInDB (const char *Str);
static Med_Type_t Med_GetTypeFromExtAndMIME (const char *Extension,
                                             const char *MIMEType);
static const char *Med_GetStringTypeForDB (Med_Type_t Type);

/*****************************************************************************/
/********************** Media (image/video) constructor **********************/
/*****************************************************************************/
// Every struct Media must be initialized with this constructor function after it is declared
// Every call to constructor must have a call to destructor

void Med_MediaConstructor (struct Media *Media)
  {
   Med_ResetMediaExceptURLAndTitle (Media);
   Media->URL   = NULL;
   Media->Title = NULL;
  }

/*****************************************************************************/
/********************** Media (image/video) destructor ***********************/
/*****************************************************************************/
// Every call to constructor must have a call to destructor

void Med_MediaDestructor (struct Media *Media)
  {
   Med_ResetMedia (Media);
  }

/*****************************************************************************/
/********************* Reset media (image/video) fields **********************/
/*****************************************************************************/
// Media must be constructed before calling this function

void Med_ResetMedia (struct Media *Media)
  {
   Med_ResetMediaExceptURLAndTitle (Media);
   Med_FreeMediaURL (Media);
   Med_FreeMediaTitle (Media);
  }

/*****************************************************************************/
/***************** Reset media fields except URL and title *******************/
/*****************************************************************************/

static void Med_ResetMediaExceptURLAndTitle (struct Media *Media)
  {
   Media->MedCod  = -1L;
   Media->Action  = Med_ACTION_NO_MEDIA;
   Media->Status  = Med_STATUS_NONE;
   Media->Name[0] = '\0';
   Media->Type    = Med_TYPE_NONE;
  }

/*****************************************************************************/
/******************************* Free image URL ******************************/
/*****************************************************************************/

static void Med_FreeMediaURL (struct Media *Media)
  {
   // Media->URL is initialized to NULL in constructor
   if (Media->URL)
     {
      free ((void *) Media->URL);
      Media->URL = NULL;
     }
  }

/*****************************************************************************/
/****************************** Free image title *****************************/
/*****************************************************************************/

static void Med_FreeMediaTitle (struct Media *Media)
  {
   // Media->Title is initialized to NULL in constructor
   if (Media->Title)
     {
      free ((void *) Media->Title);
      Media->Title = NULL;
     }
  }

/*****************************************************************************/
/**** Get media name, title and URL from a query result and copy to struct ***/
/*****************************************************************************/

void Med_GetMediaDataByCod (struct Media *Media)
  {
   MYSQL_RES *mysql_res;
   MYSQL_ROW row;
   unsigned NumRows;
   size_t Length;

   /***** Get data of a media from database *****/
   NumRows = DB_QuerySELECT (&mysql_res,"can not get data of a post",
			     "SELECT Type,"	// row[0]
			            "Name,"	// row[1]
			            "URL,"	// row[2]
			            "Title"	// row[3]
			     " FROM media WHERE MedCod=%ld",
			     Media->MedCod);

   /***** Result should have a unique row *****/
   if (NumRows == 0)	// Media not found
      /***** Reset media data *****/
      Med_ResetMedia (Media);
   else if (NumRows == 1)
     {
      /***** Get row *****/
      row = mysql_fetch_row (mysql_res);

      /***** Convert type string (row[0]) to type *****/
      Media->Type = Med_GetTypeFromStrInDB (row[0]);

      /***** Set status of media file *****/
      Media->Status = (Media->Type != Med_TYPE_NONE) ? Med_STORED_IN_DB :
						       Med_STATUS_NONE;

      /***** Copy media name (row[1]) to struct *****/
      Str_Copy (Media->Name,row[1],
		Med_BYTES_NAME);

      /***** Copy media URL (row[2]) to struct *****/
      // Media->URL can be empty or filled with previous value
      // If filled  ==> free it
      Med_FreeMediaURL (Media);
      if (row[2][0])
	{
	 /* Get and limit length of the URL */
	 Length = strlen (row[2]);
	 if (Length > Cns_MAX_BYTES_WWW)
	     Length = Cns_MAX_BYTES_WWW;

	 if ((Media->URL = (char *) malloc (Length + 1)) == NULL)
	    Lay_ShowErrorAndExit ("Error allocating memory for image URL.");
	 Str_Copy (Media->URL,row[2],
		   Length);
	}

      /***** Copy media title (row[3]) to struct *****/
      // Media->Title can be empty or filled with previous value
      // If filled  ==> free it
      Med_FreeMediaTitle (Media);
      if (row[3][0])
	{
	 /* Get and limit length of the title */
	 Length = strlen (row[3]);
	 if (Length > Med_MAX_BYTES_TITLE)
	     Length = Med_MAX_BYTES_TITLE;

	 if ((Media->Title = (char *) malloc (Length + 1)) == NULL)
	    Lay_ShowErrorAndExit ("Error allocating memory for image title.");
	 Str_Copy (Media->Title,row[3],
		   Length);
	}
     }
   else
      Lay_ShowErrorAndExit ("Internal error in database when getting media data.");

   /***** Free structure that stores the query result *****/
   DB_FreeMySQLResult (&mysql_res);
  }

/*****************************************************************************/
/********* Draw input fields to upload an image/video inside a form **********/
/*****************************************************************************/

void Med_PutMediaUploader (int NumMediaInForm,const char *ClassInput)
  {
   extern const char *Hlp_Multimedia;
   extern const char *Txt_Multimedia;
   extern const char *Txt_Image_video;
   extern const char *Txt_Title_attribution;
   extern const char *Txt_Link;
   struct ParamUploadMedia ParamUploadMedia;
   char Id[Frm_MAX_BYTES_ID + 1];
   size_t NumUploader;

#define Med_NUM_MEDIA_UPLOADERS 3
   struct MediaUploader MediaUploader[Med_NUM_MEDIA_UPLOADERS] =
     {
	{/* Upload */
	 Med_FORM_FILE,
	 "ico_upl",			// <id>_ico_upl
	 "par_upl",			// <id>_par_upl
	 "mediaClickOnActivateUpload",
	 "photo-video.svg",
	 Txt_Image_video
	},
	{/* YouTube */
	 Med_FORM_YOUTUBE,
	 "ico_you",			// <id>_ico_you
	 "par_you",			// <id>_par_you
	 "mediaClickOnActivateYoutube",
	 "youtube-brands.svg",
	 "YouTube"
	},
	{/* Embed */
	 Med_FORM_EMBED,
	 "ico_emb",			// <id>_ico_emb
	 "par_emb",			// <id>_par_emb
	 "mediaClickOnActivateEmbed",
	 "code.svg",
	 "Embed"
	}
     };

   /***** Set names of parameters depending on number of media in form *****/
   Med_SetParamNames (&ParamUploadMedia,NumMediaInForm);

   /***** Create unique id for this media uploader *****/
   Frm_SetUniqueId (Id);

   /***** Start media uploader container *****/
   HTM_DIV_Begin ("class=\"MED_UPLOADER\"");		// container

   /***** Icon 'clip' *****/
   HTM_DIV_Begin ("id=\"%s_med_ico\"",Id);		// <id>_med_ico
   fprintf (Gbl.F.Out,"<a href=\"\""
                      " onclick=\"mediaActivateMediaUploader('%s');return false;\">",
            Id);
   fprintf (Gbl.F.Out,"<img src=\"%s/paperclip.svg\""
	              " alt=\"%s\" title=\"%s\""
	              " class=\"ICO_HIGHLIGHT ICOx16\" />",
	    Cfg_URL_ICON_PUBLIC,
            Txt_Multimedia,Txt_Multimedia);
   HTM_A_End ();
   HTM_DIV_End ();						// <id>_med_ico

   /***** Start media uploader *****/
   HTM_DIV_Begin ("id=\"%s_med_upl\" style=\"display:none;\"",	// container <id>_med_upl
		  Id);

   /***** Begin box *****/
   Box_BoxBegin (NULL,Txt_Multimedia,NULL,
                 Hlp_Multimedia,Box_NOT_CLOSABLE);

   /***** Action to perform on media *****/
   Par_PutHiddenParamUnsigned (ParamUploadMedia.Action,(unsigned) Med_ACTION_NEW_MEDIA);

   /***** Icons *****/
   /* Start icons */
   HTM_DIV_Begin ("class=\"PREF_CONTAINERS\"");			// icons containers
   HTM_DIV_Begin ("class=\"PREF_CONTAINER\"");			// icons container

   /* Draw icons */
   for (NumUploader = 0;
	NumUploader < Med_NUM_MEDIA_UPLOADERS;
	NumUploader++)
      Med_PutIconMediaUploader (Id,&MediaUploader[NumUploader]);

   /* End icons */
   HTM_DIV_End ();						// icons container
   HTM_DIV_End ();						// icons containers

   /***** Form types *****/
   for (NumUploader = 0;
	NumUploader < Med_NUM_MEDIA_UPLOADERS;
	NumUploader++)
      Med_PutHiddenFormTypeMediaUploader (Id,&MediaUploader[NumUploader],
					  &ParamUploadMedia);

   /***** Media file *****/
   HTM_DIV_Begin (NULL);
   fprintf (Gbl.F.Out,"<input id=\"%s_fil\" type=\"file\""	// <id>_fil
	              " name=\"%s\" accept=\"image/,video/\""
	              " class=\"%s\" disabled=\"disabled\""
	              " style=\"display:none;\" />",
	    Id,
            ParamUploadMedia.File,
	    ClassInput);
   HTM_DIV_End ();						// <id>_fil

   /***** Media URL *****/
   HTM_DIV_Begin (NULL);
   fprintf (Gbl.F.Out,"<input id=\"%s_url\" type=\"url\""	// <id>_url
		      " name=\"%s\" placeholder=\"%s\""
                      " class=\"%s\" maxlength=\"%u\" value=\"\""
	              " disabled=\"disabled\""
	              " style=\"display:none;\" />",
	    Id,
            ParamUploadMedia.URL,Txt_Link,
            ClassInput,Cns_MAX_CHARS_WWW);
   HTM_DIV_End ();						// <id>_url

   /***** Media title *****/
   HTM_DIV_Begin (NULL);
   fprintf (Gbl.F.Out,"<input id=\"%s_tit\" type=\"text\""	// <id>_tit
		      " name=\"%s\" placeholder=\"%s\""
                      " class=\"%s\" maxlength=\"%u\" value=\"\""
	              " disabled=\"disabled\""
	              " style=\"display:none;\" />",
	    Id,
            ParamUploadMedia.Title,Txt_Title_attribution,
            ClassInput,Med_MAX_CHARS_TITLE);
   HTM_DIV_End ();						// <id>_tit

   /***** End box *****/
   Box_BoxEnd ();

   /***** End media uploader *****/
   HTM_DIV_End ();				// container <id>_med_upl

   /***** End media uploader container *****/
   HTM_DIV_End ();				// container
  }

/*****************************************************************************/
/*********************** Put an icon in media uploader ***********************/
/*****************************************************************************/

static void Med_PutIconMediaUploader (const char UniqueId[Frm_MAX_BYTES_ID + 1],
				      struct MediaUploader *MediaUploader)
  {
   /***** Icon to activate form in media uploader *****/
   HTM_DIV_Begin ("id=\"%s_%s\" class=\"PREF_OFF\"",		// <id>_IconSuffix
                  UniqueId,MediaUploader->IconSuffix);
   fprintf (Gbl.F.Out,"<a href=\"\" onclick=\"%s('%s');return false;\">",
	    MediaUploader->FunctionName,UniqueId);
   fprintf (Gbl.F.Out,"<img src=\"%s/%s\" alt=\"%s\" title=\"%s\""
                      " class=\"ICO_HIGHLIGHT ICOx16\" />",
            Cfg_URL_ICON_PUBLIC,MediaUploader->Icon,
	    MediaUploader->Title,MediaUploader->Title);
   HTM_A_End ();
   HTM_DIV_End ();						// <id>_IconSuffix
  }

/*****************************************************************************/
/******** Put a hidden input field with form type in media uploader **********/
/*****************************************************************************/

static void Med_PutHiddenFormTypeMediaUploader (const char UniqueId[Frm_MAX_BYTES_ID + 1],
						struct MediaUploader *MediaUploader,
					        struct ParamUploadMedia *ParamUploadMedia)
  {
   /***** Hidden field with form type *****/
   /* Upload file */
   fprintf (Gbl.F.Out,"<input type=\"hidden\" id=\"%s_%s\""	// <id>_ParamSuffix
		      " name=\"%s\" value=\"%u\" disabled=\"disabled\" />",
            UniqueId,MediaUploader->ParamSuffix,
	    ParamUploadMedia->FormType,(unsigned) MediaUploader->FormType);
   }

/*****************************************************************************/
/******************** Get media (image/video) from form **********************/
/*****************************************************************************/
// Media constructor must be called before calling this function
// If NumMediaInForm < 0, params have no suffix
// If NumMediaInForm >= 0, the number is a suffix of the params

void Med_GetMediaFromForm (int NumMediaInForm,struct Media *Media,
                           void (*GetMediaFromDB) (int NumMediaInForm,struct Media *Media),
			   const char *SectionForAlerts)
  {
   extern const char *Txt_Error_sending_or_processing_image_video;
   struct ParamUploadMedia ParamUploadMedia;
   Med_Action_t Action;
   Med_FormType_t FormType;

   /***** Set names of parameters depending on number of media in form *****/
   Med_SetParamNames (&ParamUploadMedia,NumMediaInForm);

   /***** Get action and initialize media (image/video)
          (except title, that will be get after the media file) *****/
   Action = Med_GetMediaActionFromForm (ParamUploadMedia.Action);

   /***** Get the media (image/video) name and the file *****/
   switch (Action)
     {
      case Med_ACTION_NEW_MEDIA:	// Upload new image/video
	 /***** Get form type *****/
	 FormType = Usr_GetFormTypeFromForm (&ParamUploadMedia);

         /***** Get new media *****/
	 switch (FormType)
	   {
	    case Med_FORM_FILE:
	       Media->Action = Med_ACTION_NEW_MEDIA;

	       /* Get image/video (if present ==>
	                           process and create temporary file) */
	       Med_GetAndProcessFileFromForm (ParamUploadMedia.File,Media);

	       /* Check status of media after getting and processing it */
	       if (Media->Status == Med_PROCESSED)
	         {
		  Usr_GetURLFromForm (ParamUploadMedia.URL,Media);
		  Usr_GetTitleFromForm (ParamUploadMedia.Title,Media);
	         }
	       break;
	    case Med_FORM_YOUTUBE:
	       Media->Action = Med_ACTION_NEW_MEDIA;

	       /* Get and process embed YouTube video from form */
	       Med_GetAndProcessYouTubeFromForm (ParamUploadMedia.URL,Media);
	       break;
	    case Med_FORM_EMBED:
	       Media->Action = Med_ACTION_NEW_MEDIA;

	       /* Get and process other embed media from form */
	       Med_GetAndProcessEmbedFromForm (ParamUploadMedia.URL,Media);
	       break;
	    default:	// No media form selected
	       Media->Action = Med_ACTION_NO_MEDIA;
	       break;
	   }

	 /***** Check status of media *****/
	 if (FormType != Med_FORM_NONE &&	// A media form is selected
	     Media->Status != Med_PROCESSED)	// No media successfully processed
	   {
	    /* Create alert with warning */
	    Ale_CreateAlert (Ale_WARNING,SectionForAlerts,
			     Txt_Error_sending_or_processing_image_video);

	    /* Reset media (no media will be saved into database) */
	    Med_ResetMedia (Media);
	   }
	 break;
      case Med_ACTION_KEEP_MEDIA:	// Keep current image/video unchanged
	 Media->Action = Med_ACTION_KEEP_MEDIA;

	 /***** Get media name *****/
	 if (GetMediaFromDB != NULL)
	    GetMediaFromDB (NumMediaInForm,Media);
	 break;
      default:	// Unknown action
	 Media->Action = Med_ACTION_NO_MEDIA;
         break;
     }
  }

/*****************************************************************************/
/********* Set parameters names depending on number of media in form *********/
/*****************************************************************************/
// If NumMediaInForm <  0, params have no suffix
// If NumMediaInForm >= 0, the number is a suffix of the params

void Med_SetParamNames (struct ParamUploadMedia *ParamUploadMedia,int NumMediaInForm)
  {
   if (NumMediaInForm < 0)	// One unique media in form ==> no suffix needed
     {
      Str_Copy (ParamUploadMedia->Action  ,"MedAct",
                Med_MAX_BYTES_PARAM_UPLOAD_MEDIA);
      Str_Copy (ParamUploadMedia->FormType,"MedFrm",
                Med_MAX_BYTES_PARAM_UPLOAD_MEDIA);
      Str_Copy (ParamUploadMedia->File    ,"MedFil",
                Med_MAX_BYTES_PARAM_UPLOAD_MEDIA);
      Str_Copy (ParamUploadMedia->Title   ,"MedTit",
                Med_MAX_BYTES_PARAM_UPLOAD_MEDIA);
      Str_Copy (ParamUploadMedia->URL     ,"MedURL",
                Med_MAX_BYTES_PARAM_UPLOAD_MEDIA);
     }
   else				// Several video/images in form ==> add suffix
     {
      snprintf (ParamUploadMedia->Action  ,sizeof (ParamUploadMedia->Action),
	        "MedAct%u",
		NumMediaInForm);
      snprintf (ParamUploadMedia->FormType,sizeof (ParamUploadMedia->Action),
	        "MedFrm%u",
		NumMediaInForm);
      snprintf (ParamUploadMedia->File    ,sizeof (ParamUploadMedia->File),
	        "MedFil%u",
		NumMediaInForm);
      snprintf (ParamUploadMedia->Title   ,sizeof (ParamUploadMedia->Title),
	        "MedTit%u",
		NumMediaInForm);
      snprintf (ParamUploadMedia->URL     ,sizeof (ParamUploadMedia->URL),
	        "MedURL%u",
		NumMediaInForm);
     }
  }

/*****************************************************************************/
/************************* Get media action from form ************************/
/*****************************************************************************/

static Med_Action_t Med_GetMediaActionFromForm (const char *ParamAction)
  {
   /***** Get parameter with the action to perform on media *****/
   return (Med_Action_t)
	  Par_GetParToUnsignedLong (ParamAction,
                                    0,
                                    Med_NUM_ACTIONS - 1,
                                    (unsigned long) Med_ACTION_DEFAULT);
  }

/*****************************************************************************/
/********************* Get from form the type of form ************************/
/*****************************************************************************/

static Med_FormType_t Usr_GetFormTypeFromForm (struct ParamUploadMedia *ParamUploadMedia)
  {
   return (Med_FormType_t) Par_GetParToUnsignedLong (ParamUploadMedia->FormType,
                                                     0,
                                                     Med_NUM_FORM_TYPES - 1,
                                                     (unsigned long) Med_FORM_NONE);
  }

/*****************************************************************************/
/********************* Get from form the type of form ************************/
/*****************************************************************************/

static void Usr_GetURLFromForm (const char *ParamName,struct Media *Media)
  {
   char URL[Cns_MAX_BYTES_WWW + 1];
   size_t Length;

   /***** Get media URL from form *****/
   Par_GetParToText (ParamName,URL,Cns_MAX_BYTES_WWW);
   /* If the URL coming from the form is empty, keep current media URL unchanged
      If not empty, copy it to current media URL */
   if ((Length = strlen (URL)) > 0)
     {
      /* Overwrite current URL (empty or coming from database)
         with the URL coming from the form */
      Med_FreeMediaURL (Media);
      if ((Media->URL = (char *) malloc (Length + 1)) == NULL)
	 Lay_ShowErrorAndExit ("Error allocating memory for media URL.");
      Str_Copy (Media->URL,URL,
                Length);
     }
  }

/*****************************************************************************/
/********************* Get from form the type of form ************************/
/*****************************************************************************/

static void Usr_GetTitleFromForm (const char *ParamName,struct Media *Media)
  {
   char Title[Med_MAX_BYTES_TITLE + 1];
   size_t Length;

   /***** Get image/video title from form *****/
   Par_GetParToText (ParamName,Title,Med_MAX_BYTES_TITLE);
   /* If the title coming from the form is empty, keep current media title unchanged
      If not empty, copy it to current media title */
   if ((Length = strlen (Title)) > 0)
     {
      /* Overwrite current title (empty or coming from database)
         with the title coming from the form */
      Med_FreeMediaTitle (Media);
      if ((Media->Title = (char *) malloc (Length + 1)) == NULL)
	 Lay_ShowErrorAndExit ("Error allocating memory for media title.");
      Str_Copy (Media->Title,Title,
                Length);
     }
  }

/*****************************************************************************/
/**************************** Get media from form ****************************/
/*****************************************************************************/

static void Med_GetAndProcessFileFromForm (const char *ParamFile,
                                           struct Media *Media)
  {
   struct Param *Param;
   char FileNameImgSrc[PATH_MAX + 1];
   char *PtrExtension;
   size_t LengthExtension;
   char MIMEType[Brw_MAX_BYTES_MIME_TYPE + 1];
   char PathFileOrg[PATH_MAX + 1];	// Full name of original uploaded file

   /***** Set media status *****/
   Media->Status = Med_STATUS_NONE;

   /***** Get filename and MIME type *****/
   Param = Fil_StartReceptionOfFile (ParamFile,FileNameImgSrc,MIMEType);
   if (!FileNameImgSrc[0])	// No file present
      return;

   /* Get filename extension */
   if ((PtrExtension = strrchr (FileNameImgSrc,(int) '.')) == NULL)
      return;
   // PtrExtension now points to last '.' in file

   PtrExtension++;
   // PtrExtension now points to first char in extension

   LengthExtension = strlen (PtrExtension);
   if (LengthExtension < Fil_MIN_BYTES_FILE_EXTENSION ||
       LengthExtension > Fil_MAX_BYTES_FILE_EXTENSION)
      return;

   /* Check extension */
   Media->Type = Med_GetTypeFromExtAndMIME (PtrExtension,MIMEType);
   if (Media->Type == Med_TYPE_NONE)
      return;

   /***** Assign a unique name for the media *****/
   Cry_CreateUniqueNameEncrypted (Media->Name);

   /***** Create private directories if not exist *****/
   /* Create private directory for images/videos if it does not exist */
   Fil_CreateDirIfNotExists (Cfg_PATH_MEDIA_PRIVATE);

   /* Create temporary private directory for images/videos if it does not exist */
   Fil_CreateDirIfNotExists (Cfg_PATH_MEDIA_TMP_PRIVATE);

   /***** End the reception of original not processed media
          (it may be very big) into a temporary file *****/
   Media->Status = Med_STATUS_NONE;
   snprintf (PathFileOrg,sizeof (PathFileOrg),
	     "%s/%s_original.%s",
	     Cfg_PATH_MEDIA_TMP_PRIVATE,Media->Name,PtrExtension);

   if (Fil_EndReceptionOfFile (PathFileOrg,Param))	// Success
     {
      /***** Detect if animated GIF *****/
      if (Media->Type == Med_GIF)
	 if (!Med_DetectIfAnimated (Media,PathFileOrg))
            Media->Type = Med_JPG;

      /***** Process media depending on the media file extension *****/
      switch (Media->Type)
        {
         case Med_JPG:
            Med_ProcessJPG (Media,PathFileOrg);
            break;
         case Med_GIF:
            Med_ProcessGIF (Media,PathFileOrg);
            break;
         case Med_MP4:
         case Med_WEBM:
         case Med_OGG:
            Med_ProcessVideo (Media,PathFileOrg);
            break;
         default:
            break;
        }
     }

   /***** Remove temporary original file *****/
   if (Fil_CheckIfPathExists (PathFileOrg))
      unlink (PathFileOrg);
  }

/*****************************************************************************/
/********************* Detect if a GIF image is animated *********************/
/*****************************************************************************/
// Return true if animated
// Return false if static or error

static bool Med_DetectIfAnimated (struct Media *Media,
			          const char PathFileOrg[PATH_MAX + 1])
  {
   char PathFileTxtTmp[PATH_MAX + 1];
   char Command[128 + PATH_MAX * 2];
   int ReturnCode;
   FILE *FileTxtTmp;	// Temporary file with the output of the command
   int NumFrames = 0;

   /***** Build path to temporary text file *****/
   snprintf (PathFileTxtTmp,sizeof (PathFileTxtTmp),
	     "%s/%s.txt",
             Cfg_PATH_MEDIA_TMP_PRIVATE,Media->Name);

   /***** Execute system command to get number of frames in GIF *****/
   snprintf (Command,sizeof (Command),
	     "identify -format '%%n\n' %s | head -1 > %s",
             PathFileOrg,PathFileTxtTmp);
   ReturnCode = system (Command);
   if (ReturnCode == -1)
      return false;		// Error
   ReturnCode = WEXITSTATUS(ReturnCode);
   if (ReturnCode != 0)
      return false;		// Error

   /***** Read temporary file *****/
   if ((FileTxtTmp = fopen (PathFileTxtTmp,"rb")) == NULL)
      return false;		// Error
   if (fscanf (FileTxtTmp,"%d",&NumFrames) != 1)
      return false;		// Error
   fclose (FileTxtTmp);

   /***** Remove temporary file *****/
   unlink (PathFileTxtTmp);

   return (NumFrames > 1);	// NumFrames > 1 ==> Animated
  }

/*****************************************************************************/
/************* Process original image generating processed JPG ***************/
/*****************************************************************************/

static void Med_ProcessJPG (struct Media *Media,
			    const char PathFileOrg[PATH_MAX + 1])
  {
   extern const char *Txt_The_file_could_not_be_processed_successfully;
   char PathFileJPGTmp[PATH_MAX + 1];	// Full name of temporary processed file

   /***** Convert original media to temporary JPG processed file
	  by calling to program that makes the conversion *****/
   snprintf (PathFileJPGTmp,sizeof (PathFileJPGTmp),
	     "%s/%s.%s",
	     Cfg_PATH_MEDIA_TMP_PRIVATE,Media->Name,Med_Extensions[Med_JPG]);
   if (Med_ResizeImage (Media,PathFileOrg,PathFileJPGTmp) == 0)	// On success ==> 0 is returned
      /* Success */
      Media->Status = Med_PROCESSED;
   else // Error processing media
     {
      /* Remove temporary destination media file */
      if (Fil_CheckIfPathExists (PathFileJPGTmp))
	 unlink (PathFileJPGTmp);

      /* Show error alert */
      Ale_ShowAlert (Ale_ERROR,Txt_The_file_could_not_be_processed_successfully);
     }
  }

/*****************************************************************************/
/******* Process original GIF image generating processed PNG and GIF *********/
/*****************************************************************************/

static void Med_ProcessGIF (struct Media *Media,
			    const char PathFileOrg[PATH_MAX + 1])
  {
   extern const char *Txt_The_file_could_not_be_processed_successfully;
   extern const char *Txt_The_size_of_the_file_exceeds_the_maximum_allowed_X;
   struct stat FileStatus;
   char PathFilePNGTmp[PATH_MAX + 1];	// Full name of temporary processed file
   char PathFileGIFTmp[PATH_MAX + 1];	// Full name of temporary processed file
   char FileSizeStr[Fil_MAX_BYTES_FILE_SIZE_STRING + 1];

   /***** Check size of media file *****/
   if (lstat (PathFileOrg,&FileStatus) == 0)	// On success ==> 0 is returned
     {
      /* Success */
      if (FileStatus.st_size <= (__off_t) Med_MAX_SIZE_GIF)
	{
	 /* File size correct */
	 /***** Get first frame of orifinal GIF file
		and save it on temporary PNG file */
	 snprintf (PathFilePNGTmp,sizeof (PathFilePNGTmp),
		   "%s/%s.png",
		   Cfg_PATH_MEDIA_TMP_PRIVATE,Media->Name);
	 if (Med_GetFirstFrame (PathFileOrg,PathFilePNGTmp) == 0)	// On success ==> 0 is returned
	   {
	    /* Success */
	    /***** Move original GIF file to temporary GIF file *****/
	    snprintf (PathFileGIFTmp,sizeof (PathFileGIFTmp),
		      "%s/%s.%s",
		      Cfg_PATH_MEDIA_TMP_PRIVATE,Media->Name,Med_Extensions[Med_GIF]);
	    if (rename (PathFileOrg,PathFileGIFTmp))	// Fail
	      {
	       /* Remove temporary PNG file */
	       if (Fil_CheckIfPathExists (PathFilePNGTmp))
		  unlink (PathFilePNGTmp);

	       /* Show error alert */
	       Ale_ShowAlert (Ale_ERROR,Txt_The_file_could_not_be_processed_successfully);
	      }
	    else					// Success
	       Media->Status = Med_PROCESSED;
	   }
	 else // Error getting first frame
	   {
	    /* Remove temporary PNG file */
	    if (Fil_CheckIfPathExists (PathFilePNGTmp))
	       unlink (PathFilePNGTmp);

	    /* Show error alert */
	    Ale_ShowAlert (Ale_ERROR,Txt_The_file_could_not_be_processed_successfully);
	   }
	}
      else	// Size exceeded
	{
	 /* Show warning alert */
	 Fil_WriteFileSizeBrief ((double) Med_MAX_SIZE_GIF,FileSizeStr);
	 Ale_CreateAlert (Ale_WARNING,NULL,
	                  Txt_The_size_of_the_file_exceeds_the_maximum_allowed_X,
			  FileSizeStr);
	}
     }
   else // Error getting file data
      /* Show error alert */
      Ale_ShowAlert (Ale_ERROR,Txt_The_file_could_not_be_processed_successfully);
  }

/*****************************************************************************/
/*********** Process original MP4 video generating processed MP4 *************/
/*****************************************************************************/

static void Med_ProcessVideo (struct Media *Media,
			      const char PathFileOrg[PATH_MAX + 1])
  {
   extern const char *Txt_The_file_could_not_be_processed_successfully;
   extern const char *Txt_The_size_of_the_file_exceeds_the_maximum_allowed_X;
   struct stat FileStatus;
   char PathFileTmp[PATH_MAX + 1];	// Full name of temporary processed file
   char FileSizeStr[Fil_MAX_BYTES_FILE_SIZE_STRING + 1];

   /***** Check size of media file *****/
   if (lstat (PathFileOrg,&FileStatus) == 0)	// On success ==> 0 is returned
     {
      /* Success */
      if (FileStatus.st_size <= (__off_t) Med_MAX_SIZE_MP4)
	{
	 /* File size correct */
	 /***** Move original video file to temporary MP4 file *****/
	 snprintf (PathFileTmp,sizeof (PathFileTmp),
		   "%s/%s.%s",
		   Cfg_PATH_MEDIA_TMP_PRIVATE,Media->Name,Med_Extensions[Media->Type]);
	 if (rename (PathFileOrg,PathFileTmp))	// Fail
	    /* Show error alert */
	    Ale_ShowAlert (Ale_ERROR,Txt_The_file_could_not_be_processed_successfully);
	 else						// Success
	    Media->Status = Med_PROCESSED;
	}
      else	// Size exceeded
	{
	 /* Show warning alert */
	 Fil_WriteFileSizeBrief ((double) Med_MAX_SIZE_MP4,FileSizeStr);
	 Ale_CreateAlert (Ale_WARNING,NULL,
	                  Txt_The_size_of_the_file_exceeds_the_maximum_allowed_X,
			  FileSizeStr);
	}
     }
   else // Error getting file data
      /* Show error alert */
      Ale_ShowAlert (Ale_ERROR,Txt_The_file_could_not_be_processed_successfully);
  }

/*****************************************************************************/
/****************************** Resize image *********************************/
/*****************************************************************************/
// Return 0 on success
// Return != 0 on error

static int Med_ResizeImage (struct Media *Media,
                            const char PathFileOriginal[PATH_MAX + 1],
                            const char PathFileProcessed[PATH_MAX + 1])
  {
   char Command[256 + PATH_MAX * 2];
   int ReturnCode;

   snprintf (Command,sizeof (Command),
	     "convert %s -resize '%ux%u>' -quality %u %s",
             PathFileOriginal,
             Media->Width,
             Media->Height,
             Media->Quality,
             PathFileProcessed);
   ReturnCode = system (Command);
   if (ReturnCode == -1)
      Lay_ShowErrorAndExit ("Error when running command to process media.");

   ReturnCode = WEXITSTATUS(ReturnCode);
   return ReturnCode;
  }

/*****************************************************************************/
/************ Process original media generating processed media **************/
/*****************************************************************************/
// Return 0 on success
// Return != 0 on error

static int Med_GetFirstFrame (const char PathFileOriginal[PATH_MAX + 1],
                              const char PathFileProcessed[PATH_MAX + 1])
  {
   char Command[128 + PATH_MAX * 2];
   int ReturnCode;

   snprintf (Command,sizeof (Command),
	     "convert '%s[0]' %s",
             PathFileOriginal,
             PathFileProcessed);
   ReturnCode = system (Command);
   if (ReturnCode == -1)
      Lay_ShowErrorAndExit ("Error when running command to process media.");

   ReturnCode = WEXITSTATUS(ReturnCode);

   return ReturnCode;
  }

/*****************************************************************************/
/************* Get link from form and transform to YouTube code **************/
/*****************************************************************************/

static void Med_GetAndProcessYouTubeFromForm (const char *ParamURL,
                                              struct Media *Media)
  {
   extern const char Str_BIN_TO_BASE64URL[64 + 1];
   char *PtrHost   = NULL;
   char *PtrPath   = NULL;
   char *PtrParams = NULL;
   char *PtrCode   = NULL;
   size_t CodeLength;
   enum
     {
      WRONG,	// Bad formed YouTube URL
      SHORT,	// youtu.be
      FULL,	// www.youtube.com/watch?
      EMBED,	// www.youtube.com/embed/
     } YouTube = WRONG;
   bool CodeFound = false;

   /***** Set media status *****/
   Media->Status = Med_STATUS_NONE;

   /***** Get embed URL from form *****/
   Usr_GetURLFromForm (ParamURL,Media);

   /***** Process URL trying to convert it to a YouTube embed URL *****/
   if (Media->URL)
      if (Media->URL[0])	// URL given by user is not empty
	{
	 /* Examples of valid YouTube URLs:
	    https://www.youtube.com/watch?v=xu9IbeF9CBw
	    https://www.youtube.com/watch?v=xu9IbeF9CBw&t=10
	    https://youtu.be/xu9IbeF9CBw
	    https://youtu.be/xu9IbeF9CBw?t=10
	    https://www.youtube.com/embed/xu9IbeF9CBw
	    https://www.youtube.com/embed/xu9IbeF9CBw?start=10
	 */
	 /***** Step 1: Skip scheme *****/
	 if      (!strncasecmp (Media->URL,"https://",8))	// URL starts by https://
	    PtrHost = &Media->URL[8];
	 else if (!strncasecmp (Media->URL,"http://" ,7))	// URL starts by http://
	    PtrHost = &Media->URL[7];
	 else if (!strncasecmp (Media->URL,"//"      ,2))	// URL starts by //
	    PtrHost = &Media->URL[2];
	 else
	    PtrHost = &Media->URL[0];

	 if (PtrHost[0])
	   {
	    /***** Step 2: Skip host *****/
	    if      (!strncasecmp (PtrHost,"youtu.be/"       , 9))	// Host starts by youtu.be/
	      {
	       YouTube = SHORT;
	       PtrPath = &PtrHost[9];
	      }
	    else if (!strncasecmp (PtrHost,"www.youtube.com/",16))	// Host starts by www.youtube.com/
	      {
	       YouTube = FULL;
	       PtrPath = &PtrHost[16];
	      }
	    else if (!strncasecmp (PtrHost,"youtube.com/"    ,12))	// Host starts by youtube.com/
	      {
	       YouTube = FULL;
	       PtrPath = &PtrHost[12];
	      }

	    /* Check pointer to path */
	    if (PtrPath)
	      {
	       if (!PtrPath[0])
		  YouTube = WRONG;
	      }
	    else
	       YouTube = WRONG;

	    if (YouTube != WRONG)
	      {
	       /***** Step 3: Skip path *****/
	       if (YouTube == FULL)
		 {
		  if      (!strncasecmp (PtrPath,"watch?",6))	// Path starts by watch?
		     PtrParams = &PtrPath[6];
		  else if (!strncasecmp (PtrPath,"embed/",6))	// Path starts by embed/
		    {
		     YouTube = EMBED;
		     PtrParams = &PtrPath[6];
		    }
		  else
		     YouTube = WRONG;
		 }
	       else
		  PtrParams = &PtrPath[0];

	       /* Check pointer to params */
	       if (PtrParams)
		 {
		  if (!PtrParams[0])
		     YouTube = WRONG;
		 }
	       else
		  YouTube = WRONG;

	       if (YouTube != WRONG)
		 {
		  /***** Step 4: Search for video code *****/
		  switch (YouTube)
		    {
		     case SHORT:
			PtrCode = PtrParams;
			break;
		     case FULL:
			/* Search for v= */
			PtrCode = strcasestr (PtrPath,"v=");
			if (PtrCode)
			   PtrCode += 2;
			break;
		     case EMBED:
			PtrCode = PtrParams;
			break;
		     default:
			PtrCode = NULL;
			break;
		    }

		  /* Check pointer to code */
		  if (PtrCode)
		    {
		     if (!PtrCode[0])
			YouTube = WRONG;
		    }
		  else
		     YouTube = WRONG;

		  if (YouTube != WRONG)
		    {
		     /***** Step 5: Get video code *****/
		     CodeLength = strspn (PtrCode,Str_BIN_TO_BASE64URL);
		     if (CodeLength > 0 &&
			 CodeLength <= Med_BYTES_NAME)
			CodeFound = true;	// Success!
		    }
		 }
	      }
	   }
	}

   /***** Set or reset media *****/
   if (CodeFound)
     {
      /* Copy code */
      strncpy (Media->Name,PtrCode,CodeLength);
      Media->Name[CodeLength] = '\0';

      /* Set media type and status */
      Media->Type   = Med_YOUTUBE;
      Media->Status = Med_PROCESSED;
     }
   else
      /* Reset media */
      Med_ResetMedia (Media);
  }

/*****************************************************************************/
/************************ Get embed link from form ***************************/
/*****************************************************************************/

static void Med_GetAndProcessEmbedFromForm (const char *ParamURL,
                                            struct Media *Media)
  {
   extern const char Str_BIN_TO_BASE64URL[64 + 1];
   char *PtrHost = NULL;
   bool URLFound = false;

   /***** Set media status *****/
   Media->Status = Med_STATUS_NONE;

   /***** Get embed URL from form *****/
   Usr_GetURLFromForm (ParamURL,Media);

   /***** Process URL trying to convert it to a YouTube embed URL *****/
   if (Media->URL)
      if (Media->URL[0])	// URL given by user is not empty
	{
	 /* Examples of valid embed URLs:
	    //www.slideshare.net/slideshow/embed_code/key/yngasD9sIZ7GQV
	 */
	 /***** Step 1: Skip scheme *****/
	 if      (!strncasecmp (Media->URL,"https://",8))	// URL starts by https://
	    PtrHost = &Media->URL[8];
	 else if (!strncasecmp (Media->URL,"http://" ,7))	// URL starts by http://
	    PtrHost = &Media->URL[7];
	 else if (!strncasecmp (Media->URL,"//"      ,2))	// URL starts by //
	    PtrHost = &Media->URL[2];

	 /***** Check if a URL is found *****/
	 if (PtrHost)
	    if (PtrHost[0])
	       URLFound = true;	// Success!
	}

   /***** Set or reset media *****/
   if (URLFound)
     {
      /* Set media type and status */
      Media->Type   = Med_EMBED;
      Media->Status = Med_PROCESSED;
     }
   else
      /* Reset media */
      Med_ResetMedia (Media);
  }

/*****************************************************************************/
/**** Remove media, keep media or store media, depending on media action *****/
/*****************************************************************************/

void Med_RemoveKeepOrStoreMedia (long CurrentMedCodInDB,struct Media *Media)
  {
   switch (Media->Action)
     {
      case Med_ACTION_NO_MEDIA:
	 /* Remove possible current media */
	 Med_RemoveMedia (Media->MedCod);

	 /* Reset media data */
	 Med_ResetMedia (Media);
	 break;
      case Med_ACTION_KEEP_MEDIA:
	 /* Keep current media */
	 Media->MedCod = CurrentMedCodInDB;
         Med_GetMediaDataByCod (Media);
	 break;
      case Med_ACTION_NEW_MEDIA:
	 /* Remove possible current media */
	 Med_RemoveMedia (Media->MedCod);

	 /* New media received and processed sucessfully? */
	 if (Media->Status == Med_PROCESSED)		// The new media received has been processed
	   {
	    /* Move processed media to definitive directory */
	    Med_MoveMediaToDefinitiveDir (Media);

	    if (Media->Status == Med_MOVED)
	       /* Store media in database */
	       Med_StoreMediaInDB (Media);	// Set Media->MedCod
	    else
	       /* Reset media data */
	       Med_ResetMedia (Media);
	   }
	 else
	    /* Reset media data */
	    Med_ResetMedia (Media);
	 break;
     }
  }

/*****************************************************************************/
/**** Move temporary processed media file to definitive private directory ****/
/*****************************************************************************/

void Med_MoveMediaToDefinitiveDir (struct Media *Media)
  {
   char PathMedPriv[PATH_MAX + 1];

   /***** Check trivial case *****/
   if (Media->Status == Med_PROCESSED)
     {
      switch (Media->Type)
        {
         case Med_JPG:
         case Med_GIF:
         case Med_MP4:
         case Med_WEBM:
         case Med_OGG:
	    /***** Create private subdirectory for media if it does not exist *****/
	    snprintf (PathMedPriv,sizeof (PathMedPriv),
		      "%s/%c%c",
		      Cfg_PATH_MEDIA_PRIVATE,
		      Media->Name[0],
		      Media->Name[1]);
	    Fil_CreateDirIfNotExists (PathMedPriv);

	    /***** Move files *****/
	    switch (Media->Type)
	      {
	       case Med_JPG:
		  /* Move JPG */
		  if (Med_MoveTmpFileToDefDir (Media,PathMedPriv,
					       Med_Extensions[Med_JPG]))
		     Media->Status = Med_MOVED;	// Success
		  break;
	       case Med_GIF:
		  /* Move PNG */
		  if (Med_MoveTmpFileToDefDir (Media,PathMedPriv,
					       "png"))
		     /* Move GIF */
		     if (Med_MoveTmpFileToDefDir (Media,PathMedPriv,
						  Med_Extensions[Med_GIF]))
			Media->Status = Med_MOVED;	// Success
		  break;
	       case Med_MP4:
	       case Med_WEBM:
	       case Med_OGG:
		  /* Move MP4 or WEBM or OGG */
		  if (Med_MoveTmpFileToDefDir (Media,PathMedPriv,
					       Med_Extensions[Media->Type]))
		     Media->Status = Med_MOVED;	// Success
		  break;
	       default:
		  break;
	      }
            break;
         case Med_YOUTUBE:
         case Med_EMBED:
	    // Nothing to do with files ==> Processing successfully finished
	    Media->Status = Med_MOVED;	// Success
            break;
         default:
            break;
        }
     }

   /***** If fail ==> reset media *****/
   if (Media->Status != Med_MOVED)	// Fail
      Med_ResetMedia (Media);
  }

/*****************************************************************************/
/******* Move temporary processed file to definitive private directory *******/
/*****************************************************************************/
// Return true on success
// Return false on error

static bool Med_MoveTmpFileToDefDir (struct Media *Media,
				     const char PathMedPriv[PATH_MAX + 1],
				     const char *Extension)
  {
   char PathFileTmp[PATH_MAX + 1];	// Full name of temporary processed file
   char PathFile[PATH_MAX + 1];	// Full name of definitive processed file

   /***** Temporary processed media file *****/
   snprintf (PathFileTmp,sizeof (PathFileTmp),
	     "%s/%s.%s",
	     Cfg_PATH_MEDIA_TMP_PRIVATE,Media->Name,Extension);

   /***** Definitive processed media file *****/
   snprintf (PathFile,sizeof (PathFile),
	     "%s/%s.%s",
	     PathMedPriv,Media->Name,Extension);

   /***** Move JPG file *****/
   if (rename (PathFileTmp,PathFile))	// Fail
     {
      Ale_ShowAlert (Ale_ERROR,"Can not move file.");
      return false;
     }

   return true;				// Success
  }

/*****************************************************************************/
/************************ Store media into database **************************/
/*****************************************************************************/

void Med_StoreMediaInDB (struct Media *Media)
  {
   /***** Trivial case *****/
   if (Media->Status != Med_MOVED)
     {
      Med_ResetMedia (Media);	// No media inserted in database
      return;
     }

   /***** Insert media into database *****/
   Media->MedCod = DB_QueryINSERTandReturnCode ("can not create media",
					        "INSERT INTO media"
					        " (Type,Name,URL,Title)"
					        " VALUES"
					        " ('%s','%s','%s','%s')",
					        Med_GetStringTypeForDB (Media->Type),
					        Media->Name  ? Media->Name  : "",
					        Media->URL   ? Media->URL   : "",
					        Media->Title ? Media->Title : "");
   Media->Status = Med_STORED_IN_DB;
  }

/*****************************************************************************/
/****** Show a user uploaded media (in a test question, timeline, etc.) ******/
/*****************************************************************************/

void Med_ShowMedia (struct Media *Media,
                    const char *ClassContainer,const char *ClassMedia)
  {
   bool PutLink;
   char PathMedPriv[PATH_MAX + 1];

   /***** If no media to show ==> nothing to do *****/
   if (Media->Status != Med_STORED_IN_DB)
      return;
   if (Media->Type == Med_TYPE_NONE)
      return;

   /***** Start media container *****/
   HTM_DIV_Begin ("class=\"%s\"",ClassContainer);

   switch (Media->Type)
     {
      case Med_JPG:
      case Med_GIF:
      case Med_MP4:
      case Med_WEBM:
      case Med_OGG:
	 /***** Show uploaded file *****/
	 /* If no media to show ==> nothing to do */
	 if (!Media->Name)
	    return;
	 if (!Media->Name[0])
	    return;

	 /* Start optional link to external URL */
	 PutLink = false;
	 if (Media->URL)
	    if (Media->URL[0])
	       PutLink = true;
	 if (PutLink)
	    fprintf (Gbl.F.Out,"<a href=\"%s\" target=\"_blank\">",Media->URL);

	 /* Create a temporary public directory used to show the media */
	 Brw_CreateDirDownloadTmp ();

	 /* Build path to private directory with the media */
	 snprintf (PathMedPriv,sizeof (PathMedPriv),
		   "%s/%c%c",
		   Cfg_PATH_MEDIA_PRIVATE,
		   Media->Name[0],
		   Media->Name[1]);

	 /* Show media */
	 switch (Media->Type)
	   {
	    case Med_JPG:
	       Med_ShowJPG (Media,PathMedPriv,ClassMedia);
	       break;
	    case Med_GIF:
	       Med_ShowGIF (Media,PathMedPriv,ClassMedia);
	       break;
	    case Med_MP4:
	    case Med_WEBM:
	    case Med_OGG:
	       Med_ShowVideo (Media,PathMedPriv,ClassMedia);
	       break;
	    default:
	       break;
	   }

	 /* End optional link to external URL */
	 if (PutLink)
	    HTM_A_End ();
	 break;
      case Med_YOUTUBE:
	 /***** Show embed YouTube video *****/
	 Med_ShowYoutube (Media,ClassMedia);
	 break;
      case Med_EMBED:
	 /***** Show other embed media *****/
	 Med_ShowEmbed (Media,ClassMedia);
	 break;
      default:
	 break;
     }

   /***** End media container *****/
   HTM_DIV_End ();
  }

/*****************************************************************************/
/************************** Show a user uploaded JPG *************************/
/*****************************************************************************/

static void Med_ShowJPG (struct Media *Media,
			 const char PathMedPriv[PATH_MAX + 1],
			 const char *ClassMedia)
  {
   extern const char *Txt_File_not_found;
   char FileNameMedia[NAME_MAX + 1];
   char FullPathMediaPriv[PATH_MAX + 1];
   char URL_JPG[PATH_MAX + 1];

   /***** Build private path to JPG *****/
   snprintf (FileNameMedia,sizeof (FileNameMedia),
	     "%s.%s",
	     Media->Name,Med_Extensions[Med_JPG]);
   snprintf (FullPathMediaPriv,sizeof (FullPathMediaPriv),
	     "%s/%s",
	     PathMedPriv,FileNameMedia);

   /***** Check if private media file exists *****/
   if (Fil_CheckIfPathExists (FullPathMediaPriv))
     {
      /***** Create symbolic link from temporary public directory to private file
	     in order to gain access to it for showing/downloading *****/
      Brw_CreateTmpPublicLinkToPrivateFile (FullPathMediaPriv,FileNameMedia);

      /***** Build URL pointing to symbolic link *****/
      snprintf (URL_JPG,sizeof (URL_JPG),
		"%s/%s/%s/%s",
		Cfg_URL_FILE_BROWSER_TMP_PUBLIC,
		Gbl.FileBrowser.TmpPubDir.L,
		Gbl.FileBrowser.TmpPubDir.R,
		FileNameMedia);

      /***** Show media *****/
      fprintf (Gbl.F.Out,"<img src=\"%s\" class=\"%s\" alt=\"\"",URL_JPG,ClassMedia);
      if (Media->Title)
	 if (Media->Title[0])
	    fprintf (Gbl.F.Out," title=\"%s\"",Media->Title);
      fprintf (Gbl.F.Out," lazyload=\"on\" />");	// Lazy load of the media
     }
   else
      fprintf (Gbl.F.Out,"%s",Txt_File_not_found);
  }

/*****************************************************************************/
/************************** Show a user uploaded GIF *************************/
/*****************************************************************************/

static void Med_ShowGIF (struct Media *Media,
			 const char PathMedPriv[PATH_MAX + 1],
			 const char *ClassMedia)
  {
   extern const char *Txt_File_not_found;
   char FileNameMedia[NAME_MAX + 1];
   char FullPathMediaPriv[PATH_MAX + 1];
   char URL_GIF[PATH_MAX + 1];
   char URL_PNG[PATH_MAX + 1];

   /***** Build private path to animated GIF image *****/
   snprintf (FileNameMedia,sizeof (FileNameMedia),
	     "%s.%s",
	     Media->Name,Med_Extensions[Med_GIF]);
   snprintf (FullPathMediaPriv,sizeof (FullPathMediaPriv),
	     "%s/%s",
	     PathMedPriv,FileNameMedia);

   /***** Check if private media file exists *****/
   if (Fil_CheckIfPathExists (FullPathMediaPriv))	// The animated GIF image
     {
      /***** Create symbolic link from temporary public directory to private file
	     in order to gain access to it for showing/downloading *****/
      Brw_CreateTmpPublicLinkToPrivateFile (FullPathMediaPriv,FileNameMedia);

      /***** Create URL pointing to symbolic link *****/
      snprintf (URL_GIF,sizeof (URL_GIF),
		"%s/%s/%s/%s",
		Cfg_URL_FILE_BROWSER_TMP_PUBLIC,
		Gbl.FileBrowser.TmpPubDir.L,
		Gbl.FileBrowser.TmpPubDir.R,
		FileNameMedia);

      /***** Build private path to static PNG image *****/
      snprintf (FileNameMedia,sizeof (FileNameMedia),
		"%s.png",
		Media->Name);
      snprintf (FullPathMediaPriv,sizeof (FullPathMediaPriv),
		"%s/%s",
		PathMedPriv,FileNameMedia);

      /***** Check if private media file exists *****/
      if (Fil_CheckIfPathExists (FullPathMediaPriv))	// The static PNG image
	{
	 /***** Create symbolic link from temporary public directory to private file
		in order to gain access to it for showing/downloading *****/
	 Brw_CreateTmpPublicLinkToPrivateFile (FullPathMediaPriv,FileNameMedia);

	 /***** Create URL pointing to symbolic link *****/
	 snprintf (URL_PNG,sizeof (URL_PNG),
		   "%s/%s/%s/%s",
		   Cfg_URL_FILE_BROWSER_TMP_PUBLIC,
		   Gbl.FileBrowser.TmpPubDir.L,
		   Gbl.FileBrowser.TmpPubDir.R,
		   FileNameMedia);

	 /***** Show static PNG and animated GIF *****/
	 HTM_DIV_Begin ("class=\"MED_PLAY\""
			" onmouseover=\"toggleOnGIF(this,'%s');\""
			" onmouseout=\"toggleOffGIF(this,'%s');\"",
			URL_GIF,
			URL_PNG);

	 /* Image */
	 fprintf (Gbl.F.Out,"<img src=\"%s\" class=\"%s\" alt=\"\"",

		  URL_PNG,
		  ClassMedia);
	 if (Media->Title)
	    if (Media->Title[0])
	       fprintf (Gbl.F.Out," title=\"%s\"",Media->Title);
	 fprintf (Gbl.F.Out," lazyload=\"on\" />");	// Lazy load of the media

	 /* Overlay with GIF label */
	 fprintf (Gbl.F.Out,"<span class=\"MED_PLAY_ICO\">"
			    "GIF"
			    "</span>");

	 HTM_DIV_End ();
	}
      else
	 fprintf (Gbl.F.Out,"%s",Txt_File_not_found);
     }
   else
      fprintf (Gbl.F.Out,"%s",Txt_File_not_found);
  }

/*****************************************************************************/
/************************ Show a user uploaded video *************************/
/*****************************************************************************/

static void Med_ShowVideo (struct Media *Media,
			   const char PathMedPriv[PATH_MAX + 1],
			   const char *ClassMedia)
  {
   extern const char *Txt_File_not_found;
   char FileNameMediaPriv[NAME_MAX + 1];
   char FullPathMediaPriv[PATH_MAX + 1];
   char URL_Video[PATH_MAX + 1];

   /***** Build private path to video *****/
   snprintf (FileNameMediaPriv,sizeof (FileNameMediaPriv),
	     "%s.%s",
	     Media->Name,Med_Extensions[Media->Type]);
   snprintf (FullPathMediaPriv,sizeof (FullPathMediaPriv),
	     "%s/%s",
	     PathMedPriv,FileNameMediaPriv);

   /***** Check if private media file exists *****/
   if (Fil_CheckIfPathExists (FullPathMediaPriv))
     {
      /***** Create symbolic link from temporary public directory to private file
	     in order to gain access to it for showing/downloading *****/
      Brw_CreateTmpPublicLinkToPrivateFile (FullPathMediaPriv,FileNameMediaPriv);

      /***** Create URL pointing to symbolic link *****/
      snprintf (URL_Video,sizeof (URL_Video),
		"%s/%s/%s/%s",
		Cfg_URL_FILE_BROWSER_TMP_PUBLIC,
		Gbl.FileBrowser.TmpPubDir.L,
		Gbl.FileBrowser.TmpPubDir.R,
		FileNameMediaPriv);

      /***** Show media *****/
      fprintf (Gbl.F.Out,"<video src=\"%s\""
			 " preload=\"metadata\" controls=\"controls\""
			 " class=\"%s\"",
	       URL_Video,ClassMedia);
      if (Media->Title)
	 if (Media->Title[0])
	    fprintf (Gbl.F.Out," title=\"%s\"",Media->Title);
      fprintf (Gbl.F.Out," lazyload=\"on\">"	// Lazy load of the media
                         "Your browser does not support HTML5 video."
	                 "</video>");
     }
   else
      fprintf (Gbl.F.Out,"%s",Txt_File_not_found);
  }

/*****************************************************************************/
/*********************** Show an embed YouTube video *************************/
/*****************************************************************************/

static void Med_ShowYoutube (struct Media *Media,const char *ClassMedia)
  {
   /***** Check if YouTube code exists *****/
   if (Media->Name[0])	// YouTube code
     {
      if (Gbl.Usrs.Me.UsrDat.Prefs.AcceptThirdPartyCookies)
        {
	 /***** Show linked external media *****/
	 // Example of code given by YouTube:
	 // <iframe width="560" height="315"
	 // 	src="https://www.youtube.com/embed/xu9IbeF9CBw"
	 // 	frameborder="0"
	 // 	allow="accelerometer; autoplay; encrypted-media;
	 // 	gyroscope; picture-in-picture" allowfullscreen>
	 // </iframe>
	 HTM_DIV_Begin ("class=\"MED_VIDEO_CONT\"");
	 fprintf (Gbl.F.Out,"<iframe src=\"https://www.youtube.com/embed/%s\""
			    " frameborder=\"0\""
			    " allow=\"accelerometer; autoplay; encrypted-media;"
			    " gyroscope; picture-in-picture\""
			    " allowfullscreen=\"allowfullscreen\""
			    " class=\"%s\"",
		  Media->Name,ClassMedia);
	 if (Media->Title)
	    if (Media->Title[0])
	       fprintf (Gbl.F.Out," title=\"%s\"",Media->Title);
	 fprintf (Gbl.F.Out,">"
			    "</iframe>");
	 HTM_DIV_End ();
        }
      else
         /***** Alert to inform about third party cookies *****/
	 Med_AlertThirdPartyCookies ();
     }
  }

/*****************************************************************************/
/*************************** Show an embed media *****************************/
/*****************************************************************************/

static void Med_ShowEmbed (struct Media *Media,const char *ClassMedia)
  {
   /***** Check if embed URL exists *****/
   if (Media->URL[0])	// Embed URL
     {
      if (Gbl.Usrs.Me.UsrDat.Prefs.AcceptThirdPartyCookies)
        {
	 /***** Show linked external media *****/
	 // Example of code given by Slideshare:
	 // <iframe src="//www.slideshare.net/slideshow/embed_code/key/yngasD9sIZ7GQV"
	 // 	width="595" height="485" frameborder="0"
	 // 	marginwidth="0" marginheight="0" scrolling="no"
	 // 	style="border:1px solid #CCC; border-width:1px; margin-bottom:5px; max-width: 100%;"
	 // 	allowfullscreen>
	 // </iframe>
	 HTM_DIV_Begin ("class=\"MED_EMBED_CONT\"");
	 fprintf (Gbl.F.Out,"<iframe src=\"%s\""
			    " frameborder=\"0\""
	 	            " marginwidth=\"0\" marginheight=\"0\" scrolling=\"no\""
			    " allowfullscreen=\"allowfullscreen\""
			    " class=\"%s\"",
		 Media->URL,ClassMedia);
	 if (Media->Title)
	    if (Media->Title[0])
	       fprintf (Gbl.F.Out," title=\"%s\"",Media->Title);
	 fprintf (Gbl.F.Out,">"
			    "</iframe>");
	 HTM_DIV_End ();
        }
      else
         /***** Alert to inform about third party cookies *****/
	 Med_AlertThirdPartyCookies ();
     }
  }

/*****************************************************************************/
/********** Remove several media files and entries in database ***************/
/*****************************************************************************/

static void Med_AlertThirdPartyCookies (void)
  {
   extern const char *Txt_To_watch_multimedia_content_from_another_website_you_have_to_accept_third_party_cookies_in_your_personal_settings;
   extern const char *Txt_Settings;

   /***** Alert to inform about third party cookies *****/
   /* Start alert */
   Ale_ShowAlertAndButton1 (Ale_INFO,Txt_To_watch_multimedia_content_from_another_website_you_have_to_accept_third_party_cookies_in_your_personal_settings);

   /* Put form to change cookies preferences */
   if (!Gbl.Form.Inside)
      Lay_PutContextualLinkIconText (ActReqEdiSet,Coo_COOKIES_ID,NULL,
				     "cog.svg",
				     Txt_Settings);

   /* End alert */
   Ale_ShowAlertAndButton2 (ActUnk,NULL,NULL,NULL,Btn_NO_BUTTON,NULL);
  }

/*****************************************************************************/
/********** Remove several media files and entries in database ***************/
/*****************************************************************************/

void Med_RemoveMediaFromAllRows (unsigned NumMedia,MYSQL_RES *mysql_res)
  {
   MYSQL_ROW row;
   unsigned NumMed;
   long MedCod;

   /***** Go over result removing media files *****/
   for (NumMed = 0;
	NumMed < NumMedia;
	NumMed++)
     {
      /***** Get media code (row[0]) *****/
      row = mysql_fetch_row (mysql_res);
      MedCod = Str_ConvertStrCodToLongCod (row[0]);

      /***** Remove media files *****/
      Med_RemoveMedia (MedCod);
     }
  }

/*****************************************************************************/
/********** Remove one media from filesystem and from database ***************/
/*****************************************************************************/

void Med_RemoveMedia (long MedCod)
  {
   char PathMedPriv[PATH_MAX + 1];
   char FullPathMediaPriv[PATH_MAX + 1];
   struct Media Media;

   /***** Trivial case *****/
   if (MedCod <= 0)
      return;

   /***** Initialize media *****/
   Med_MediaConstructor (&Media);

   /***** Get media data *****/
   Media.MedCod = MedCod;
   Med_GetMediaDataByCod (&Media);

   /***** Step 1. Remove media files from filesystem *****/
   switch (Media.Type)
     {
      case Med_JPG:
      case Med_GIF:
      case Med_MP4:
      case Med_WEBM:
      case Med_OGG:
	 if (Media.Name[0])
	   {
	    /***** Build path to private directory with the media *****/
	    snprintf (PathMedPriv,sizeof (PathMedPriv),
		      "%s/%c%c",
		      Cfg_PATH_MEDIA_PRIVATE,
		      Media.Name[0],
		      Media.Name[1]);

	    /***** Remove files *****/
	    switch (Media.Type)
	      {
	       case Med_JPG:
		  /***** Remove private JPG file *****/
		  snprintf (FullPathMediaPriv,sizeof (FullPathMediaPriv),
			    "%s/%s.%s",
			    PathMedPriv,Media.Name,Med_Extensions[Med_JPG]);
		  unlink (FullPathMediaPriv);

		  break;
	       case Med_GIF:
		  /***** Remove private GIF file *****/
		  snprintf (FullPathMediaPriv,sizeof (FullPathMediaPriv),
			    "%s/%s.%s",
			    PathMedPriv,Media.Name,Med_Extensions[Med_GIF]);
		  unlink (FullPathMediaPriv);

		  /***** Remove private PNG file *****/
		  snprintf (FullPathMediaPriv,sizeof (FullPathMediaPriv),
			    "%s/%s.png",
			    PathMedPriv,Media.Name);
		  unlink (FullPathMediaPriv);

		  break;
	       case Med_MP4:
	       case Med_WEBM:
	       case Med_OGG:
		  /***** Remove private video file *****/
		  snprintf (FullPathMediaPriv,sizeof (FullPathMediaPriv),
			    "%s/%s.%s",
			    PathMedPriv,Media.Name,Med_Extensions[Media.Type]);
		  unlink (FullPathMediaPriv);

		  break;
	       default:
		  break;
	      }
	    // Public links are removed automatically after a period
	   }
	 break;
      default:
	 break;
     }

   /***** Step 2. Remove entry for this media from database *****/
   DB_QueryDELETE ("can not remove media",
		   "DELETE FROM media WHERE MedCod=%ld",
		   MedCod);

   /***** Free media *****/
   Med_MediaDestructor (&Media);
  }

/*****************************************************************************/
/************************ Get media type from string *************************/
/*****************************************************************************/

static Med_Type_t Med_GetTypeFromStrInDB (const char *Str)
  {
   Med_Type_t Type;

   for (Type = (Med_Type_t) 0;
        Type < Med_NUM_TYPES;
        Type++)
      if (!strcasecmp (Str,Med_StringsTypeDB[Type]))
         return Type;

   return Med_TYPE_NONE;
  }

/*****************************************************************************/
/************************ Get media type from extension **********************/
/*****************************************************************************/

static Med_Type_t Med_GetTypeFromExtAndMIME (const char *Extension,
                                             const char *MIMEType)
  {
   /***** Extensions and MIME types allowed to convert to JPG *****/
   if (!strcasecmp (Extension,"jpg" ) ||
       !strcasecmp (Extension,"jpeg") ||
       !strcasecmp (Extension,"png" ) ||
       !strcasecmp (Extension,"tif" ) ||
       !strcasecmp (Extension,"tiff") ||
       !strcasecmp (Extension,"bmp" ))
      if (!strcmp (MIMEType,"image/jpeg"              ) ||
          !strcmp (MIMEType,"image/pjpeg"             ) ||
          !strcmp (MIMEType,"image/png"               ) ||
          !strcmp (MIMEType,"image/x-png"             ) ||
	  !strcmp (MIMEType,"image/tiff"              ) ||
	  !strcmp (MIMEType,"image/bmp"               ) ||
          !strcmp (MIMEType,"application/octet-stream") ||
	  !strcmp (MIMEType,"application/octetstream" ) ||
	  !strcmp (MIMEType,"application/octet"       ))
	 return Med_JPG;

   /***** Extensions and MIME types allowed to convert to GIF *****/
   if (!strcasecmp (Extension,"gif"))
      if (!strcmp (MIMEType,"image/gif"               ) ||
          !strcmp (MIMEType,"application/octet-stream") ||
	  !strcmp (MIMEType,"application/octetstream" ) ||
	  !strcmp (MIMEType,"application/octet"       ))
	 return Med_GIF;

   /***** Extensions and MIME types allowed to convert to MP4 *****/
   if (!strcasecmp (Extension,"mp4"))
      if (!strcmp (MIMEType,"video/mp4"               ) ||
          !strcmp (MIMEType,"application/octet-stream") ||
	  !strcmp (MIMEType,"application/octetstream" ) ||
	  !strcmp (MIMEType,"application/octet"       ))
	 return Med_MP4;

   /***** Extensions and MIME types allowed to convert to WEBM *****/
   if (!strcasecmp (Extension,"webm"))
      if (!strcmp (MIMEType,"video/webm"              ) ||
          !strcmp (MIMEType,"application/octet-stream") ||
	  !strcmp (MIMEType,"application/octetstream" ) ||
	  !strcmp (MIMEType,"application/octet"       ))
	 return Med_WEBM;

   /***** Extensions and MIME types allowed to convert to OGG *****/
   if (!strcasecmp (Extension,"ogg"))
      if (!strcmp (MIMEType,"video/ogg"               ) ||
          !strcmp (MIMEType,"application/octet-stream") ||
	  !strcmp (MIMEType,"application/octetstream" ) ||
	  !strcmp (MIMEType,"application/octet"       ))
	 return Med_OGG;

   return Med_TYPE_NONE;
  }

/*****************************************************************************/
/*************** Get string media type in database from type *****************/
/*****************************************************************************/

static const char *Med_GetStringTypeForDB (Med_Type_t Type)
  {
   /***** Check if type is out of valid range *****/
   if (Type > (Med_Type_t) (Med_NUM_TYPES - 1))
      return Med_StringsTypeDB[Med_TYPE_NONE];

   /***** Get string from type *****/
   return Med_StringsTypeDB[Type];
  }
