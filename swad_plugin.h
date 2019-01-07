// swad_plugin.h: plugins called from SWAD using web services

#ifndef _SWAD_PLG
#define _SWAD_PLG
/*
    SWAD (Shared Workspace At a Distance in Spanish),
    is a web platform developed at the University of Granada (Spain),
    and used to support university teaching.

    This file is part of SWAD core.
    Copyright (C) 1999-2019 Antonio Ca�as Vargas

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
/********************************** Headers **********************************/
/*****************************************************************************/

/*****************************************************************************/
/****************************** Public constants *****************************/
/*****************************************************************************/

#define Plg_MAX_CHARS_PLUGIN_NAME	(32 - 1)	// 31
#define Plg_MAX_BYTES_PLUGIN_NAME	((Plg_MAX_CHARS_PLUGIN_NAME + 1) * Str_MAX_BYTES_PER_CHAR - 1)	// 511

#define Plg_MAX_CHARS_PLUGIN_DESCRIPTION	(128 - 1)	// 127
#define Plg_MAX_BYTES_PLUGIN_DESCRIPTION	(Plg_MAX_CHARS_PLUGIN_DESCRIPTION * Str_MAX_BYTES_PER_CHAR)	// 2047

#define Plg_MAX_CHARS_PLUGIN_LOGO	(32 - 1)	// 31
#define Plg_MAX_BYTES_PLUGIN_LOGO	Plg_MAX_CHARS_PLUGIN_LOGO	// 31

#define Plg_MAX_CHARS_PLUGIN_APP_KEY	(32 - 1)	// 31
#define Plg_MAX_BYTES_PLUGIN_APP_KEY	Plg_MAX_CHARS_PLUGIN_APP_KEY	// 31

/*****************************************************************************/
/******************************* Public types ********************************/
/*****************************************************************************/

struct Plugin
  {
   long PlgCod;
   char Name[Plg_MAX_BYTES_PLUGIN_NAME + 1];
   char Description[Plg_MAX_BYTES_PLUGIN_DESCRIPTION + 1];
   char Logo[Plg_MAX_BYTES_PLUGIN_LOGO + 1];
   char AppKey[Plg_MAX_BYTES_PLUGIN_APP_KEY + 1];
   char URL[Cns_MAX_BYTES_WWW + 1];
   char IP[Cns_MAX_BYTES_IP + 1];
  };

/*****************************************************************************/
/****************************** Public prototypes ****************************/
/*****************************************************************************/

void Plg_ListPlugins (void);
void Plg_EditPlugins (void);
bool Plg_GetDataOfPluginByCod (struct Plugin *Plg);
void Plg_FreeListPlugins (void);
long Plg_GetParamPlgCod (void);
void Plg_RemovePlugin (void);
void Plg_RenamePlugin (void);
void Plg_ChangePlgDescription (void);
void Plg_ChangePlgLogo (void);
void Plg_ChangePlgAppKey (void);
void Plg_ChangePlgURL (void);
void Plg_ChangePlgIP (void);
void Plg_RecFormNewPlg (void);
void Plg_WebService (void);

#endif
