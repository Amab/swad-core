// swad_setting_database.h: user's settings / preferences, operations with database

#ifndef _SWAD_SET_DB
#define _SWAD_SET_DB
/*
    SWAD (Shared Workspace At a Distance in Spanish),
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

/*****************************************************************************/
/***************************** Public constants ******************************/
/*****************************************************************************/

/*****************************************************************************/
/******************************* Public types ********************************/
/*****************************************************************************/

/*****************************************************************************/
/***************************** Public prototypes *****************************/
/*****************************************************************************/

//--------------------------- Global user settings ----------------------------
void Set_DB_UpdateMySettingsAboutLanguage (void);
void Set_DB_UpdateMySettingsAboutSideCols (void);
void Set_DB_UpdateMySettingsAboutDateFormat (Dat_Format_t DateFormat);
void Set_DB_UpdateMySettingsAboutFirstDayOfWeek (unsigned FirstDayOfWeek);
void Set_DB_UpdateMySettingsAboutIconSet (const char *IconSetId);
void Set_DB_UpdateMySettingsAboutMenu (Mnu_Menu_t Menu);
void Set_DB_UpdateMySettingsAboutPhoto (void);
void Set_DB_UpdateMySettingsAboutBasicProfile (void);
void Set_DB_UpdateMySettingsAboutExtendedProfile (void);
void Set_DB_UpdateMySettingsAboutNotifyEvents (void);

//-------------------- User settings in the current course --------------------
void Set_DB_InsertUsrInCrsSettings (long UsrCod,long CrsCod);

void Set_DB_UpdateGrpLastAccZone (const char *FieldNameDB,long GrpCod);

void Set_DB_RemUsrFromCrsSettings (long UsrCod,long CrsCod);
void Set_DB_RemUsrFromAllCrssSettings (long UsrCod);
void Set_DB_RemAllUsrsFromCrsSettings (long CrsCod);

//------------------- IP settings (when no user logged) -----------------------
void Set_DB_UpdateSettingsFromIP (void);
void Set_DB_UpdateMySettingsFromIP (void);

unsigned Set_DB_GetSettingsFromIP (MYSQL_RES **mysql_res);

void Set_DB_RemoveOldSettingsFromIP (void);

#endif
