// swad_zip.c: compress files in file browsers

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

#include <dirent.h>		// For scandir, etc.
#include <errno.h>		// For errno
#include <linux/limits.h>	// For PATH_MAX
#include <stdlib.h>		// For system...
#include <string.h>		// For string functions...
#include <sys/stat.h>		// For mkdir...
#include <sys/types.h>		// For mkdir...
#include <unistd.h>		// For chdir...

#include "swad_box.h"
#include "swad_config.h"
#include "swad_file_browser.h"
#include "swad_form.h"
#include "swad_global.h"
#include "swad_HTML.h"
#include "swad_ID.h"
#include "swad_parameter.h"
#include "swad_string.h"
#include "swad_theme.h"

/*****************************************************************************/
/****************************** Public constants *****************************/
/*****************************************************************************/

/*****************************************************************************/
/***************************** Internal constants ****************************/
/*****************************************************************************/

#define ZIP_MiB (1024ULL * 1024ULL)
#define ZIP_MAX_SIZE_UNCOMPRESSED (1024ULL * ZIP_MiB)

/*****************************************************************************/
/****************************** Internal types *******************************/
/*****************************************************************************/

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

static void ZIP_PutLinkToCreateZIPAsgWrkParams (void);

static void ZIP_CreateTmpDirForCompression (void);
static void ZIP_CreateDirCompressionUsr (struct UsrData *UsrDat);

static void ZIP_CompressFolderIntoZIP (void);
static unsigned long long ZIP_CloneDir (const char *Path,const char *PathClone,const char *PathInTree);
static void ZIP_ShowLinkToDownloadZIP (const char *FileName,const char *URL,
                                       off_t FileSize,unsigned long long UncompressedSize);

/*****************************************************************************/
/*********** Put link to create ZIP file of assignments and works ************/
/*****************************************************************************/

void ZIP_PutLinkToCreateZIPAsgWrk (void)
  {
   extern const char *Txt_Create_ZIP_file;

   Lay_PutContextualLinkIconText (ActAdmAsgWrkCrs,NULL,
				  ZIP_PutLinkToCreateZIPAsgWrkParams,
				  "download.svg",
				  Txt_Create_ZIP_file);
  }

static void ZIP_PutLinkToCreateZIPAsgWrkParams (void)
  {
   Usr_PutHiddenParSelectedUsrsCods ();
   Brw_PutHiddenParamFullTreeIfSelected ();
   Par_PutHiddenParamChar ("CreateZIP",'Y');
  }

/*****************************************************************************/
/****************** Get whether to create ZIP file from form *****************/
/*****************************************************************************/

bool ZIP_GetCreateZIPFromForm (void)
  {
   return Par_GetParToBool ("CreateZIP");
  }

/*****************************************************************************/
/*************** Create the zip file with assignments and works **************/
/*************** and put a link to download it                  **************/
/*****************************************************************************/

void ZIP_CreateZIPAsgWrk (void)
  {
   extern const char *Txt_works_ZIP_FILE_NAME;
   struct UsrData UsrDat;
   const char *Ptr;
   char StrZip[128 + PATH_MAX];
   char Path[PATH_MAX + 1 +
             NAME_MAX + 1 +
	     NAME_MAX + 1];
   int Result;
   char FileNameZIP[NAME_MAX + 1];
   char PathFileZIP[PATH_MAX + 1];
   struct stat FileStatus;
   char URLWithSpaces[PATH_MAX + 1];
   char URL[PATH_MAX + 1];

   /***** Create zip file
	  with the assignments and works
	  of the selected users *****/
   /* Create temporary directory
      for the compression of assignments and works */
   ZIP_CreateTmpDirForCompression ();

   /* Initialize structure with user's data */
   Usr_UsrDataConstructor (&UsrDat);

   /* Create temporary directory for each selected user
      inside the directory used for compression */
   Ptr = Gbl.Usrs.Selected.List[Rol_UNK];
   while (*Ptr)
     {
      Par_GetNextStrUntilSeparParamMult (&Ptr,UsrDat.EncryptedUsrCod,
					 Cry_BYTES_ENCRYPTED_STR_SHA256_BASE64);
      Usr_GetUsrCodFromEncryptedUsrCod (&UsrDat);


      if (Usr_ChkUsrCodAndGetAllUsrDataFromUsrCod (&UsrDat,Usr_DONT_GET_PREFS))	// Get user's data from database
	 if (Usr_CheckIfUsrBelongsToCurrentCrs (&UsrDat))
	    ZIP_CreateDirCompressionUsr (&UsrDat);
     }

   /* Free memory used for user's data */
   Usr_UsrDataDestructor (&UsrDat);

   /***** Create a temporary public directory
          used to download the zip file *****/
   Brw_CreateDirDownloadTmp ();

   /***** Relative path of the directory with the works to compress *****/
   snprintf (Path,sizeof (Path),
	     "%s/%s",
	     Cfg_PATH_ZIP_PRIVATE,
	     Gbl.FileBrowser.ZIP.TmpDir);

   /***** Change to directory of the assignments and works
          in order to start the path in the zip file from there *****/
   if (chdir (Path))
      Lay_ShowErrorAndExit ("Can not change to temporary folder for compression.");

   /***** Create public zip file with the assignment and works *****/
   snprintf (FileNameZIP,sizeof (FileNameZIP),
	     "%s.zip",
	     Txt_works_ZIP_FILE_NAME);
   snprintf (PathFileZIP,sizeof (PathFileZIP),
	     "%s/%s/%s/%s",
	     Cfg_PATH_FILE_BROWSER_TMP_PUBLIC,
             Gbl.FileBrowser.TmpPubDir.L,
             Gbl.FileBrowser.TmpPubDir.R,
             FileNameZIP);
   snprintf (StrZip,sizeof (StrZip),
	     "nice -n 19 zip -q -r '%s' *",
             PathFileZIP);
   Result = system (StrZip);

   /***** Return to the CGI directory *****/
   if (chdir (Cfg_PATH_CGI_BIN))
      Lay_ShowErrorAndExit ("Can not change to cgi-bin folder.");

   /***** If the zip command has been sucessful, write the link to zip file *****/
   if (Result == 0)
     {
      /***** Get file size *****/
      if (lstat (PathFileZIP,&FileStatus))	// On success ==> 0 is returned
	 Lay_ShowErrorAndExit ("Can not get information about a file or folder.");
      else
	{
	 /***** Create URL pointing to ZIP file *****/
	 snprintf (URLWithSpaces,sizeof (URLWithSpaces),
	           "%s/%s/%s/%s",
		   Cfg_URL_FILE_BROWSER_TMP_PUBLIC,
		   Gbl.FileBrowser.TmpPubDir.L,
		   Gbl.FileBrowser.TmpPubDir.R,
		   FileNameZIP);
	 Str_CopyStrChangingSpaces (URLWithSpaces,URL,PATH_MAX);	// In HTML, URL must have no spaces

	 /****** Link to download file *****/
	 ZIP_ShowLinkToDownloadZIP (FileNameZIP,URL,FileStatus.st_size,0);
	}
     }
   else
      Lay_ShowErrorAndExit ("Can not compress files into zip file.");

   /***** Remove the directory of compression *****/
   Fil_RemoveTree (Path);
  }

/*****************************************************************************/
/********* Create temporary directory to put the works to compress ***********/
/*****************************************************************************/

static void ZIP_CreateTmpDirForCompression (void)
  {
   char PathDirTmp[PATH_MAX + 1];

   /***** If the private directory does not exist, create it *****/
   Fil_CreateDirIfNotExists (Cfg_PATH_ZIP_PRIVATE);

   /***** Create a new temporary directory *****/
   Str_Copy (Gbl.FileBrowser.ZIP.TmpDir,Gbl.UniqueNameEncrypted,
             NAME_MAX);
   snprintf (PathDirTmp,sizeof (PathDirTmp),
	     "%s/%s",
	     Cfg_PATH_ZIP_PRIVATE,Gbl.FileBrowser.ZIP.TmpDir);
   if (mkdir (PathDirTmp,(mode_t) 0xFFF))
      Lay_ShowErrorAndExit ("Can not create temporary folder for compression.");
  }

/*****************************************************************************/
/**************** Create link to a user's works zone        ******************/
/**************** in the temporary directory of compression ******************/
/*****************************************************************************/

static void ZIP_CreateDirCompressionUsr (struct UsrData *UsrDat)
  {
   char FullNameAndUsrID[NAME_MAX + 1];
   char PathFolderUsrInsideCrs[128 + PATH_MAX + NAME_MAX];
   char LinkTmpUsr[PATH_MAX + 1];
   char Link[PATH_MAX + 1];
   unsigned NumTry;
   bool Success;

   /***** Create a link in the tree of compression
          with a name that identifies the owner
          of the assignments and works *****/
   /* Create link name for this user */
   Str_Copy (FullNameAndUsrID,UsrDat->Surname1,
             NAME_MAX);
   if (UsrDat->Surname1[0] &&
       UsrDat->Surname2[0])
      Str_Concat (FullNameAndUsrID,"_",	// Separation between surname 1 and surname 2
                  NAME_MAX);
   Str_Concat (FullNameAndUsrID,UsrDat->Surname2,
               NAME_MAX);
   if ((UsrDat->Surname1[0] ||
	UsrDat->Surname2[0]) &&
       UsrDat->FirstName[0])
      Str_Concat (FullNameAndUsrID,"_",	// Separation between surnames and first name
                  NAME_MAX);
   Str_Concat (FullNameAndUsrID,UsrDat->FirstName,
               NAME_MAX);
   if ((UsrDat->Surname1[0] ||
	UsrDat->Surname2[0] ||
	UsrDat->FirstName[0]) &&
       UsrDat->IDs.Num)
      Str_Concat (FullNameAndUsrID,"-",	// Separation between name and ID
                  NAME_MAX);
   if (UsrDat->IDs.Num)	// If this user has at least one ID
      Str_Concat (FullNameAndUsrID,UsrDat->IDs.List[0].ID,
                  NAME_MAX);	// First user's ID
   Str_ConvertToValidFileName (FullNameAndUsrID);

   /* Create path to folder and link */
   snprintf (PathFolderUsrInsideCrs,sizeof (PathFolderUsrInsideCrs),
	     "%s/usr/%02u/%ld",
	     Gbl.Crs.PathPriv,
	     (unsigned) (UsrDat->UsrCod % 100),
	     UsrDat->UsrCod);
   snprintf (LinkTmpUsr,sizeof (LinkTmpUsr),
	     "%s/%s/%s",
	     Cfg_PATH_ZIP_PRIVATE,
	     Gbl.FileBrowser.ZIP.TmpDir,
	     FullNameAndUsrID);

   /* Try to create a link named LinkTmpUsr to PathFolderUsrInsideCrs */
   if (symlink (PathFolderUsrInsideCrs,LinkTmpUsr) != 0)
     {
      for (Success = false, NumTry = 2;
	   !Success && errno == EEXIST && NumTry<=1000;
	   NumTry++)
	{
	 // Link exists ==> a former user share the same name and ID
	 // (probably a unique user has created two or more accounts)
	 snprintf (Link,sizeof (Link),
	          "%s-%u",
		  LinkTmpUsr,NumTry);
	 if (symlink (PathFolderUsrInsideCrs,Link) == 0)
	    Success = true;
	}

      if (!Success)
	 Lay_ShowErrorAndExit ("Can not create temporary link for compression.");
     }
  }

/*****************************************************************************/
/********************* Compress a folder into ZIP file ***********************/
/*****************************************************************************/

void ZIP_CompressFileTree (void)
  {
   /***** Get parameters related to file browser *****/
   Brw_GetParAndInitFileBrowser ();

   /***** Compress folder into ZIP *****/
   ZIP_CompressFolderIntoZIP ();

   /***** Show again file browser *****/
   Brw_ShowAgainFileBrowserOrWorks ();
  }

/*****************************************************************************/
/*************** Create the zip file with assignments and works **************/
/*************** and put a link to download it                  **************/
/*****************************************************************************/

static void ZIP_CompressFolderIntoZIP (void)
  {
   extern const char *Txt_ROOT_FOLDER_EXTERNAL_NAMES[Brw_NUM_TYPES_FILE_BROWSER];
   extern const char *Txt_The_folder_is_empty;
   extern const char *Txt_The_contents_of_the_folder_are_too_big;
   unsigned long long UncompressedSize;
   char StrZip[128 + PATH_MAX];
   char Path[PATH_MAX + 1 +
             PATH_MAX + 1];
   char PathCompression[PATH_MAX + 1 +
                        NAME_MAX + 1 +
                        NAME_MAX + 1];
   int Result;
   char FileNameZIP[NAME_MAX + 1];
   char PathFileZIP[PATH_MAX + 1];
   struct stat FileStatus;
   char URLWithSpaces[PATH_MAX + 1];
   char URL[PATH_MAX + 1];

   /***** Create temporary private directory
          for the compression of folder *****/
   ZIP_CreateTmpDirForCompression ();

   /***** Create a temporary public directory
          used to download the zip file *****/
   Brw_CreateDirDownloadTmp ();

   /***** Create a copy of the directory to compress *****/
   snprintf (Path,sizeof (Path),
	     "%s/%s",
	     Gbl.FileBrowser.Priv.PathAboveRootFolder,
	     Gbl.FileBrowser.FilFolLnk.Full);
   snprintf (PathCompression,sizeof (PathCompression),
	     "%s/%s",
	     Cfg_PATH_ZIP_PRIVATE,
	     Gbl.FileBrowser.ZIP.TmpDir);	// Example: /var/www/swad/zip/<temporary_dir>

   UncompressedSize = ZIP_CloneDir (Path,PathCompression,Gbl.FileBrowser.FilFolLnk.Full);

   if (UncompressedSize == 0)					// Nothing to compress
      Ale_ShowAlert (Ale_WARNING,Txt_The_folder_is_empty);
   else if (UncompressedSize > ZIP_MAX_SIZE_UNCOMPRESSED)	// Uncompressed size is too big
      Ale_ShowAlert (Ale_WARNING,Txt_The_contents_of_the_folder_are_too_big);
   else
     {
      /***** Change to directory of the clone folder
	     in order to start the path in the zip file from there *****/
      if (chdir (PathCompression))
	 Lay_ShowErrorAndExit ("Can not change to temporary folder for compression.");

      /***** Create public zip file with the assignment and works *****/
      snprintf (FileNameZIP,sizeof (FileNameZIP),
	        "%s.zip",
	        strcmp (Gbl.FileBrowser.FilFolLnk.Name,".") ? Gbl.FileBrowser.FilFolLnk.Name :
							    Txt_ROOT_FOLDER_EXTERNAL_NAMES[Gbl.FileBrowser.Type]);
      snprintf (PathFileZIP,sizeof (PathFileZIP),
	        "%s/%s/%s/%s",
	        Cfg_PATH_FILE_BROWSER_TMP_PUBLIC,
	        Gbl.FileBrowser.TmpPubDir.L,
	        Gbl.FileBrowser.TmpPubDir.R,
	        FileNameZIP);
      snprintf (StrZip,sizeof (StrZip),
	        "nice -n 19 zip -q -5 -r '%s' *",
	        PathFileZIP);
      Result = system (StrZip);

      /***** Return to the CGI directory *****/
      if (chdir (Cfg_PATH_CGI_BIN))
	 Lay_ShowErrorAndExit ("Can not change to cgi-bin folder.");

      /***** If the zip command has been sucessful, write the link to zip file *****/
      if (Result == 0)
	{
	 /***** Get file size *****/
	 if (lstat (PathFileZIP,&FileStatus))	// On success ==> 0 is returned
	    Lay_ShowErrorAndExit ("Can not get information about a file or folder.");
	 else
	   {
	    /***** Create URL pointing to ZIP file *****/
	    snprintf (URLWithSpaces,sizeof (URLWithSpaces),
		      "%s/%s/%s/%s",
		      Cfg_URL_FILE_BROWSER_TMP_PUBLIC,
		      Gbl.FileBrowser.TmpPubDir.L,
		      Gbl.FileBrowser.TmpPubDir.R,
		      FileNameZIP);
	    Str_CopyStrChangingSpaces (URLWithSpaces,URL,PATH_MAX);	// In HTML, URL must have no spaces

	    /****** Link to download file *****/
	    ZIP_ShowLinkToDownloadZIP (FileNameZIP,URL,FileStatus.st_size,UncompressedSize);
	   }
	}
      else
	 Lay_ShowErrorAndExit ("Can not compress files into zip file.");
     }

   /***** Remove the directory of compression *****/
   Fil_RemoveTree (PathCompression);
  }

/*****************************************************************************/
/*************** Clone a directory recursively to compress it ****************/
/*****************************************************************************/

/* Example:
 *
 * Example starting directory with document files: /var/www/swad/crs/1000/descarga/lectures/lecture_1
 * We want to compress all files inside lecture_1 into a ZIP file
 * Path = /var/www/swad/crs/1000/descarga/lectures/lecture_1
 * PathClone = /var/www/swad/zip/<unique_dir>
 * PathInTree = "descarga/lectures/lecture_1"

 * Example directory inside starting directory with document files: /var/www/swad/crs/1000/descarga/lectures/lecture_1/slides
 * Path = /var/www/swad/crs/1000/descarga/lectures/lecture_1/slides
 * PathClone: /var/www/swad/zip/<unique_dir>/slides
 * PathInTree = "descarga/lectures/lecture_1/slides
 */
// Return: full size of directory contents

static unsigned long long ZIP_CloneDir (const char *Path,const char *PathClone,const char *PathInTree)
  {
   struct dirent **FileList;
   int NumFile;
   int NumFiles;
   char PathFile[PATH_MAX + 1];
   char PathFileClone[PATH_MAX + 1];
   char PathFileInTree[PATH_MAX + 1];
   struct stat FileStatus;
   Brw_FileType_t FileType;
   bool Hidden;
   bool SeeDocsZone = Gbl.FileBrowser.Type == Brw_SHOW_DOC_INS ||
                      Gbl.FileBrowser.Type == Brw_SHOW_DOC_CTR ||
                      Gbl.FileBrowser.Type == Brw_SHOW_DOC_DEG ||
                      Gbl.FileBrowser.Type == Brw_SHOW_DOC_CRS ||
                      Gbl.FileBrowser.Type == Brw_SHOW_DOC_GRP;
   bool SeeMarks    = Gbl.FileBrowser.Type == Brw_SHOW_MRK_CRS ||
                      Gbl.FileBrowser.Type == Brw_SHOW_MRK_GRP;
   unsigned long long FullSize = 0;

   /***** Scan directory *****/
   if ((NumFiles = scandir (Path,&FileList,NULL,alphasort)) >= 0)	// No error
     {
      /***** List files *****/
      for (NumFile = 0;
	   NumFile < NumFiles;
	   NumFile++)
	 if (strcmp (FileList[NumFile]->d_name,".") &&
	     strcmp (FileList[NumFile]->d_name,".."))	// Skip directories "." and ".."
	   {
	    snprintf (PathFileInTree,sizeof (PathFileInTree),
		      "%s/%s",
	              PathInTree,FileList[NumFile]->d_name);
	    snprintf (PathFile,sizeof (PathFile),
		      "%s/%s",
		      Path,FileList[NumFile]->d_name);
	    snprintf (PathFileClone,sizeof (PathFileClone),
		      "%s/%s",
		      PathClone,FileList[NumFile]->d_name);

	    FileType = Brw_IS_UNKNOWN;
	    if (lstat (PathFile,&FileStatus))	// On success ==> 0 is returned
	       Lay_ShowErrorAndExit ("Can not get information about a file or folder.");
	    else if (S_ISDIR (FileStatus.st_mode))		// It's a directory
	       FileType = Brw_IS_FOLDER;
	    else if (S_ISREG (FileStatus.st_mode))	// It's a regular file
	       FileType = Str_FileIs (FileList[NumFile]->d_name,"url") ? Brw_IS_LINK :	// It's a link (URL inside a .url file)
									 Brw_IS_FILE;	// It's a file

	    Hidden = (SeeDocsZone || SeeMarks) ? Brw_CheckIfFileOrFolderIsSetAsHiddenInDB (FileType,PathFileInTree) :
						 false;

	    if (!Hidden)	// If file/folder is not hidden
	      {
	       if (FileType == Brw_IS_FOLDER)	// It's a directory
		 {
		  FullSize += (unsigned long long) FileStatus.st_size;

		  /***** Create clone of subdirectory *****/
		  if (mkdir (PathFileClone,(mode_t) 0xFFF))
		     Lay_ShowErrorAndExit ("Can not create temporary subfolder for compression.");

		  /***** Clone subtree starting at this this directory *****/
		  FullSize += ZIP_CloneDir (PathFile,PathFileClone,PathFileInTree);
		 }
	       else if (FileType == Brw_IS_FILE ||
			FileType == Brw_IS_LINK)	// It's a regular file
		 {
		  FullSize += (unsigned long long) FileStatus.st_size;

		  /***** Create a symbolic link (clone) to original file *****/
		  if (symlink (PathFile,PathFileClone) != 0)
		     Lay_ShowErrorAndExit ("Can not create temporary link for compression.");

		  /***** Update number of my views of this file *****/
		  Brw_UpdateMyFileViews (Brw_GetFilCodByPath (PathFileInTree,false));	// Any file, public or not
		 }
	      }
	   }
     }
   else
      Lay_ShowErrorAndExit ("Error while scanning directory.");

   return FullSize;
  }

/*****************************************************************************/
/********************* Show link to download a ZIP file **********************/
/*****************************************************************************/

static void ZIP_ShowLinkToDownloadZIP (const char *FileName,const char *URL,
                                       off_t FileSize,unsigned long long UncompressedSize)
  {
   extern const char *The_ClassFormInBox[The_NUM_THEMES];
   extern const char *Txt_ZIP_file;
   extern const char *Txt_Download;
   extern const char *Txt_Filename;
   extern const char *Txt_File_size;
   extern const char *Txt_FILE_uncompressed;
   char FileSizeStr[Fil_MAX_BYTES_FILE_SIZE_STRING + 1];

   /***** Begin box and table *****/
   Box_StartBoxTableShadow (NULL,NULL,NULL,NULL,2);

   /***** Link to download the file *****/
   HTM_TR_Begin (NULL);
   HTM_TD_Begin ("colspan=\"2\" class=\"FILENAME_TXT CM\"");
   HTM_A_Begin ("href=\"%s\" class=\"FILENAME_TXT\" title=\"%s\" target=\"_blank\"",
                URL,FileName);
   HTM_IMG (CfG_URL_ICON_FILEXT_PUBLIC "32x32","zip32x32.gif",Txt_ZIP_file,
	    "class=\"ICO40x40\"");
   fprintf (Gbl.F.Out,"&nbsp;%s&nbsp;",FileName);
   Ico_PutIcon ("download.svg",Txt_Download,"ICO40x40");
   HTM_A_End ();
   HTM_TD_End ();
   HTM_TR_End ();

   /***** Filename *****/
   HTM_TR_Begin (NULL);

   HTM_TD_Begin ("class=\"%s RM\"",The_ClassFormInBox[Gbl.Prefs.Theme]);
   fprintf (Gbl.F.Out,"%s:",Txt_Filename);
   HTM_TD_End ();

   HTM_TD_Begin ("class=\"DAT LM\"");
   HTM_A_Begin ("href=\"%s\" class=\"DAT\" title=\"%s\" target=\"_blank\"",
	        URL,FileName);
   fprintf (Gbl.F.Out,"%s",FileName);
   HTM_A_End ();
   HTM_TD_End ();

   HTM_TR_End ();

   /***** Write the file size *****/
   Fil_WriteFileSizeFull ((double) FileSize,FileSizeStr);
   HTM_TR_Begin (NULL);

   HTM_TD_Begin ("class=\"%s RM\"",The_ClassFormInBox[Gbl.Prefs.Theme]);
   fprintf (Gbl.F.Out,"%s:",Txt_File_size);
   HTM_TD_End ();

   HTM_TD_Begin ("class=\"DAT LM\"");
   fprintf (Gbl.F.Out,"%s",FileSizeStr);
   if (UncompressedSize)
     {
      Fil_WriteFileSizeFull ((double) UncompressedSize,FileSizeStr);
      fprintf (Gbl.F.Out," (%s %s)",
               FileSizeStr,Txt_FILE_uncompressed);
     }
   HTM_TD_End ();

   HTM_TR_End ();

   /***** End table and box *****/
   Box_EndBoxTable ();
  }
