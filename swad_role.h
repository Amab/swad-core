// swad_role.h: user's roles

#ifndef _SWAD_ROL
#define _SWAD_ROL
/*
    SWAD (Shared Workspace At a Distance in Spanish),
    is a web platform developed at the University of Granada (Spain),
    and used to support university teaching.

    This file is part of SWAD core.
    Copyright (C) 1999-2016 Antonio Ca�as Vargas

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

/*****************************************************************************/
/******************************** Public types *******************************/
/*****************************************************************************/

// Related with user's roles
/*
   Don't change these numbers!
   They are used for users' permissions and for user's types in database

   Only Rol_ROLE_STUDENT and Rol_ROLE_TEACHER are allowed
   as user permanent roles in courses,
   but a user may be logged temporarily as other roles
*/
#define Rol_NUM_ROLES 9
typedef enum
  {
   Rol_UNKNOWN = 0,	// User not logged in
   Rol__GUEST_ = 1,	// User not belonging to any course
   Rol_VISITOR = 2,	// Student or teacher in other courses...
   	   	   	// ...but not belonging to the current course
   Rol_STUDENT = 3,	// Student in current course
   Rol_TEACHER = 4,	// Teacher in current course
   Rol_DEG_ADM = 5,	// Degree administrator
   Rol_CTR_ADM = 6,	// Centre administrator
   Rol_INS_ADM = 7,	// Institution administrator
   Rol_SYS_ADM = 8,	// System administrator (superuser)
  } Rol_Role_t;

/*****************************************************************************/
/****************************** Public prototypes ****************************/
/*****************************************************************************/

unsigned Rol_GetNumAvailableRoles (void);
Rol_Role_t Rol_GetMaxRole (unsigned Roles);
Rol_Role_t Rol_GetMyMaxRoleInIns (long InsCod);
Rol_Role_t Rol_GetMyMaxRoleInCtr (long CtrCod);
Rol_Role_t Rol_GetMyMaxRoleInDeg (long DegCod);
Rol_Role_t Rol_GetMyRoleInCrs (long CrsCod);
Rol_Role_t Rol_GetRoleInCrs (long CrsCod,long UsrCod);
unsigned Rol_GetRolesInAllCrss (long UsrCod);

Rol_Role_t Rol_ConvertUnsignedStrToRole (const char *UnsignedStr);
unsigned Rol_ConvertUnsignedStrToRoles (const char *UnsignedStr);

void Rol_PutFormToChangeMyRole (bool FormInHead);
void Rol_ChangeMyRole (void);

void Rol_WriteSelectorRoles (unsigned RolesAllowed,unsigned RolesSelected,
                             bool Disabled,bool SendOnChange);
void Rol_PutHiddenParamRoles (unsigned Role);
unsigned Rol_GetSelectedRoles (void);

Rol_Role_t Rol_GetRequestedRole (long UsrCod);

#endif
