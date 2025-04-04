// swad_password_database.h: Users' passwords, operations with database

#ifndef _SWAD_PWD_DB
#define _SWAD_PWD_DB
/*
    SWAD (Shared Workspace At a Distance in Spanish),
    is a web platform developed at the University of Granada (Spain),
    and used to support university teaching.

    This file is part of SWAD core.
    Copyright (C) 1999-2025 Antonio Ca�as Vargas

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
/***************************** Public prototypes *****************************/
/*****************************************************************************/

void Pwd_DB_UpdateMyPendingPassword (void);
void Pwd_DB_AssignMyPendingPasswordToMyCurrentPassword (void);

void Pwd_DB_GetPendingPassword (void);
unsigned Pwd_DB_GetNumOtherUsrsWhoUseThisPassword (const char *EncryptedPassword,long UsrCod);

void Pwd_DB_RemoveMyPendingPassword (void);
void Pwd_DB_RemoveExpiredPendingPassword (void);

#endif
