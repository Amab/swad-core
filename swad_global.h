// swad_global.h: global variables

#ifndef _SWAD_GBL
#define _SWAD_GBL
/*
    SWAD (Shared Workspace At a Distance in Spanish),
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

#include <limits.h>		// For maximum values
#include <stdio.h>		// For FILE
#include <sys/time.h>		// For tz
#include <mysql/mysql.h>	// To access MySQL databases

#include "swad_API.h"
#include "swad_assignment.h"
#include "swad_browser.h"
#include "swad_connected.h"
#include "swad_config.h"
#include "swad_course.h"
#include "swad_enrolment.h"
#include "swad_file.h"
#include "swad_group.h"
#include "swad_mail.h"
#include "swad_record.h"

/*****************************************************************************/
/******************************* Public types ********************************/
/*****************************************************************************/

struct Globals
  {
   struct
     {
      bool WritingHTMLStart;	// Used to avoid writing the HTML head when aborting program on error
      bool HTMLStartWritten;	// Used to avoid writing more than once the HTML head
      bool DivsEndWritten;	// Used to avoid writing more than once the HTML end
      bool HTMLEndWritten;	// Used to avoid writing more than once the HTML end
     } Layout;
   struct
     {
      Lan_Language_t Language;
      unsigned FirstDayOfWeek;
      Dat_Format_t DateFormat;
      Ico_IconSet_t IconSet;
      Mnu_Menu_t Menu;
      The_Theme_t Theme;
      unsigned SideCols;
      PhoSha_Shape_t PhotoShape;
     } Prefs;
   struct
     {
      unsigned NumSessions;
      bool IsOpen;
      bool HasBeenDisconnected;
      char Id[Cns_BYTES_SESSION_ID + 1];
      long UsrCod;
      bool ParamsInsertedIntoDB;	// If parameters are inserted in the database in this session
     } Session;
   struct
     {
      Tab_Tab_t Tab;
      Act_Action_t Act;
      Act_Action_t Original;	// Used in some actions to know what action gave rise to the current action
      bool UsesAJAX;		// Do not generate full HTML page, only the content of a div
      bool IsAJAXAutoRefresh;	// It's an automatic refresh drom time to time
     } Action;
   struct
     {
      HieLvl_Level_t Level;	// Current level in the hierarchy: system, country, institution, center, degree or course
      long Cod;			// Code of the current country, institution, center, degree or course
      struct ListCountries Ctys;// List of countries
      struct Cty_Countr Cty;	// Current country
      struct ListInstits Inss;	// List of institutions in current country
      struct Ins_Instit Ins;	// Current institution
      struct ListCenters Ctrs;	// List of centers in current institution
      struct Ctr_Center Ctr;	// Current center
      struct ListDegrees Degs;	// List of degrees in current center
      struct Deg_Degree Deg;	// Current degree
      struct Crs_ListCourses Crss;	// List of courses in current degree
      struct Crs_Course Crs;	// Current course. Aditional info about course is stored in Gbl.Crs.
     } Hierarchy;
   struct
     {
      Sch_WhatToSearch_t WhatToSearch;
      char Str[Sch_MAX_BYTES_STRING_TO_FIND + 1];
      bool LogSearch;
     } Search;
   struct
     {
      bool IsWebService;	// Must generate HTML output (IsWebService==false) or SOAP-XML output (IsWebService==true)?
      long PlgCod;
      API_Function_t Function;
     } WebService;
   struct
     {
      struct
	{
	 struct Usr_Data UsrDat;
	 struct Usr_Last UsrLast;
	 struct
	   {
	    unsigned Available;
	    Rol_Role_t FromSession;
	    Rol_Role_t Logged;
	    Rol_Role_t LoggedBeforeCloseSession;
	    Rol_Role_t Max;
	    bool HasChanged;	// Set when I have changed my role
	   } Role;
	 char UsrIdLogin[Cns_MAX_BYTES_USR_LOGIN + 1];	// String to store the ID, nickname or email entered in the user's login
         char LoginPlainPassword[Pwd_MAX_BYTES_PLAIN_PASSWORD + 1];
         char LoginEncryptedPassword[Pwd_BYTES_ENCRYPTED_PASSWORD + 1];
         char PendingPassword[Pwd_BYTES_ENCRYPTED_PASSWORD + 1];
	 char PathDir[PATH_MAX + 1];
	 bool Logged;
         bool IBelongToCurrentIns;
         bool IBelongToCurrentCtr;
         bool IBelongToCurrentDeg;
         bool IBelongToCurrentCrs;
         bool MyPhotoExists;
         unsigned NumAccWithoutPhoto;
         char PhotoURL[PATH_MAX + 1];
         time_t TimeLastAccToThisFileBrowser;
         bool ConfirmEmailJustSent;	// An email to confirm my email address has just been sent
         struct
           {
            bool Filled;	// My institutions are already filled?
            unsigned Num;
            struct
              {
               long CtyCod;
               Rol_Role_t MaxRole;
              } Ctys[Cty_MAX_COUNTRS_PER_USR];
           } MyCtys;
         struct
           {
            bool Filled;	// My institutions are already filled?
            unsigned Num;
            struct
              {
               long InsCod;
               Rol_Role_t MaxRole;
              } Inss[Ins_MAX_INSTITS_PER_USR];
           } MyInss;
         struct
           {
            bool Filled;	// My centers are already filled?
            unsigned Num;
            struct
              {
               long CtrCod;
               Rol_Role_t MaxRole;
              } Ctrs[Ctr_MAX_CENTERS_PER_USR];
           } MyCtrs;
         struct
           {
            bool Filled;	// My degrees are already filled?
            unsigned Num;
            struct
              {
               long DegCod;
               Rol_Role_t MaxRole;
              } Degs[Deg_MAX_DEGREES_PER_USR];
           } MyDegs;
         struct
           {
            bool Filled;	// My courses are already filled?
            unsigned Num;
            struct
              {
               long CrsCod;
               Rol_Role_t Role;
               long DegCod;
              } Crss[Crs_MAX_COURSES_PER_USR];
           } MyCrss;
	 Set_ShowUsrsType_t ListType;	// My preference about user's list type
	 unsigned NumFollowers;	// Number of users who follow me
	 unsigned NumFollowing;	// Number of users I follow
	} Me;		// The user logged
      struct
	{
	 struct Usr_Data UsrDat;
	 unsigned NumIDToConfirm;	// Used as hidden parameter to confirm a user's ID
	} Other;		// Another user, used for example to register in / remove from a course
      struct Usr_ListUsrs LstUsrs[Rol_NUM_ROLES];
      struct Usr_SelectedUsrs Selected;
      char *ListOtherRecipients;	// List of ID or nicks of users written explicitely on a form
      struct
	{
	 Rec_UsrsType_t RecsUsrs;	// Students' records or teachers' records
	 unsigned RecsPerPag;		// Number of rows of records in a row (only for printing)
	 bool WithPhotos;
	} Listing;
      struct
	{
	 unsigned Cols;
	 bool AllGroups;
	} ClassPhoto;
      struct
        {
         unsigned long TimeToRefreshInMs;
         unsigned NumUsr;
         unsigned NumUsrs;
         unsigned NumUsrsToList;
         struct Con_ConnectedUsrs Usrs[Rol_NUM_ROLES];
         struct
           {
            long UsrCod;
            bool ThisCrs;
            time_t TimeDiff;
           } Lst[Cfg_MAX_CONNECTED_SHOWN];
        } Connected;
      char FileNamePhoto[NAME_MAX + 1];	// File name (with no path and no .jpg) of the temporary file with the selected face
      Enr_RegRemOneUsrAction_t RegRemAction;	// Enrolment action
     } Usrs;
   struct
     {
      struct Usr_Data *UsrDat;
      Rec_SharedRecordViewType_t TypeOfView;
     } Record;
   struct
     {
      HieLvl_Level_t Current;
      HieLvl_Level_t Default;
      unsigned Allowed;
     } Scope;
   struct
     {
      char PathPriv[PATH_MAX + 1];	// Absolute path to the private directory of the course
      char PathRelPubl[PATH_MAX + 1];   // Relative path to the public directory of the course
      char PathURLPubl[PATH_MAX + 1];   // Abolute URL to the public part of the course
      struct Grp_Groups Grps;
      struct Inf_Info Info;
      struct
	{
	 struct RecordField Field;
	 struct LstRecordFields LstFields;
	} Records;
      struct
	{
	 long HighlightNotCod;	// Notice code of a notice to be highlighted
	} Notices;
     } Crs;
   struct
     {
      unsigned Id;		// Each file browser in the page has a unique identifier
      Brw_FileBrowser_t Type;
      bool FullTree;		// Show full tree?
      bool ShowOnlyPublicFiles;	// Show only public files?
      bool UploadingWithDropzone;
      struct
        {
         unsigned long MaxFiles;
         unsigned long MaxFolds;
         unsigned long long int MaxQuota;
         unsigned NumLevls;
         unsigned long NumFolds;
         unsigned long NumFiles;
         unsigned long long int TotalSiz;
        } Size;
      struct
	{
	 char PathAboveRootFolder[PATH_MAX + 1];
	 char PathRootFolder[PATH_MAX + 1];
	} Priv;
      char NewFilFolLnkName[NAME_MAX + 1];
      struct Brw_FilFolLnk FilFolLnk;
      unsigned Level;
      Brw_IconViewEdit_t IconViewEdit;
      struct
        {
         Brw_FileBrowser_t FileBrowser;	// Type of the file browser
         long Cod;			// Code of the institution/center/degree/course/group related to the file browser with the clipboard
	 long WorksUsrCod;		// User code of the user related to the works file browser with the clipboard
         unsigned Level;
         struct Brw_FilFolLnk FilFolLnk;
         bool IsThisTree;		// When showing a file browser, is it that corresponding to the clipboard?
         bool IsThisFile;		// When showing a row of a file browser, are we in the path of the clipboard?
        } Clipboard;
      struct
        {
         char L[2 + 1];		// Left directory: 2 first chars
         char R[NAME_MAX + 1];	// Right directory: rest of chars
        } TmpPubDir;
      bool HiddenLevels[1 + Brw_MAX_DIR_LEVELS];
      char TxtStyle[64];
      const char *InputStyle;
      struct Asg_Assignment Asg;	// Data of assignment when browsing level 1 or an assignment zone.
				        // TODO: Remove from global?
      struct
        {
	 bool CreateZIP;
         char TmpDir[NAME_MAX + 1];
        } ZIP;
     } FileBrowser;	// Struct used for a file browser

   /* Cache */
   struct
     {
      struct
	{
	 long CtyCod;
	 Lan_Language_t Language;
	 char CtyName[Cty_MAX_BYTES_NAME + 1];
	} CountryName;
      struct
	{
	 long InsCod;
	 char ShrtName[Cns_HIERARCHY_MAX_BYTES_SHRT_NAME + 1];
	 char CtyName[Cns_HIERARCHY_MAX_BYTES_FULL_NAME + 1];
	} InstitutionShrtNameAndCty;
      struct
        {
	 long InsCod;
	 unsigned NumDpts;
        } NumDptsInIns;
      struct
        {
         bool Valid;
	 long CtyCod;
	 unsigned NumInss;
        } NumInssInCty;
      struct
        {
	 long CtyCod;
	 unsigned NumCtrs;
        } NumCtrsInCty;
      struct
        {
	 long InsCod;
	 unsigned NumCtrs;
        } NumCtrsInIns;
      struct
        {
	 long CtyCod;
	 unsigned NumDegs;
        } NumDegsInCty;
      struct
        {
	 long InsCod;
	 unsigned NumDegs;
        } NumDegsInIns;
      struct
        {
	 long CtrCod;
	 unsigned NumDegs;
        } NumDegsInCtr;
      struct
        {
	 long CtyCod;
	 unsigned NumCrss;
        } NumCrssInCty;
      struct
        {
	 long InsCod;
	 unsigned NumCrss;
        } NumCrssInIns;
      struct
        {
	 long CtrCod;
	 unsigned NumCrss;
        } NumCrssInCtr;
      struct
        {
	 long DegCod;
	 unsigned NumCrss;
        } NumCrssInDeg;
      struct
        {
	 bool Valid;
	 unsigned NumUsrs;
        } NumUsrsWhoDontClaimToBelongToAnyCty;
      struct
        {
	 bool Valid;
	 unsigned NumUsrs;
        } NumUsrsWhoClaimToBelongToAnotherCty;
      struct
        {
	 long CtyCod;
	 unsigned NumUsrs;
        } NumUsrsWhoClaimToBelongToCty;
      struct
        {
	 long InsCod;
	 unsigned NumUsrs;
        } NumUsrsWhoClaimToBelongToIns;
      struct
        {
	 long CtrCod;
	 unsigned NumUsrs;
        } NumUsrsWhoClaimToBelongToCtr;
      struct
	{
	 long UsrCod;
	 bool IsSuperuser;
	} UsrIsSuperuser;
      struct
	{
	 long UsrCod;
	 long InsCod;
	 bool Belongs;
	} UsrBelongsToIns;
      struct
	{
	 long UsrCod;
	 long CtrCod;
	 bool Belongs;
	} UsrBelongsToCtr;
      struct
	{
	 long UsrCod;
	 long DegCod;
	 bool Belongs;
	} UsrBelongsToDeg;
      struct
	{
	 long UsrCod;
	 long CrsCod;
	 bool CountOnlyAcceptedCourses;
	 bool Belongs;
	} UsrBelongsToCrs;
      struct
	{
	 long UsrCod;
	 bool Belongs;
	} UsrBelongsToCurrentCrs;
      struct
	{
	 long UsrCod;
	 bool Accepted;
	} UsrHasAcceptedInCurrentCrs;
      struct
	{
	 long UsrCod;
	 bool SharesAnyOfMyCrs;
	} UsrSharesAnyOfMyCrs;
      struct
	{
	 long GrpCod;
	 bool IBelong;
	} IBelongToGrp;
      struct
	{
	 long UsrCod;
	 bool Shares;
	} UsrSharesAnyOfMyGrpsInCurrentCrs;
      struct
	{
	 long UsrCod;
	 long CrsCod;
	 Rol_Role_t Role;
	} RoleUsrInCrs;
      struct
	{
	 bool Cached;
	 Rol_Role_t Role;
	} MyRoleInCurrentCrs;
      struct
	{
	 long PrjCod;
	 unsigned RolesInProject;
	} MyRolesInProject;
      struct
        {
	 long UsrCod;
	 unsigned NumFollowing;
	 unsigned NumFollowers;
        } Follow;
     } Cache;
  };

/*****************************************************************************/
/****************************** Public prototypes ****************************/
/*****************************************************************************/

void Gbl_InitializeGlobals (void);
void Gbl_Cleanup (void);

#endif
