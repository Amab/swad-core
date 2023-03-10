// swad_logo.h: logo of institution, center or degree

#ifndef _SWAD_LGO
#define _SWAD_LGO
/*
    SWAD (Shared Workspace At a Distance in Spanish),
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
/***************************** Public prototypes *****************************/
/*****************************************************************************/

void Lgo_DrawLogo (HieLvl_Level_t Scope,long Cod,const char *AltText,
                   unsigned Size,const char *Class,bool PutIconIfNotExists);
void Lgo_PutIconToChangeLogo (HieLvl_Level_t Scope);
void Lgo_RequestLogo (HieLvl_Level_t Scope);
void Lgo_ReceiveLogo (HieLvl_Level_t Scope);
void Lgo_RemoveLogo (HieLvl_Level_t Scope);

#endif
