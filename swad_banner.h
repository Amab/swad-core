// swad_banner.h: banners

#ifndef _SWAD_BAN
#define _SWAD_BAN
/*
    SWAD (Shared Workspace At a Distance),
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
/********************************* Headers ***********************************/
/*****************************************************************************/

#include "swad_hidden_visible.h"
#include "swad_name.h"
#include "swad_string.h"
#include "swad_www.h"

/*****************************************************************************/
/************************** Public types and constants ***********************/
/*****************************************************************************/

#define Ban_MAX_CHARS_IMAGE	NAME_MAX		// 255
#define Ban_MAX_BYTES_IMAGE	Ban_MAX_CHARS_IMAGE	// 255

struct Ban_Banner
  {
   long BanCod;
   HidVis_HiddenOrVisible_t Hidden;
   char ShrtName[Nam_MAX_BYTES_SHRT_NAME + 1];
   char FullName[Nam_MAX_BYTES_FULL_NAME + 1];
   char Img[Ban_MAX_BYTES_IMAGE + 1];
   char WWW[WWW_MAX_BYTES_WWW + 1];
  };

/*****************************************************************************/
/***************************** Public prototypes *****************************/
/*****************************************************************************/

void Ban_ShowAllBanners (void);
void Ban_EditBanners (void);

void Ban_PutIconToViewBanners (void);

void Ban_GetBannerDataByCod (struct Ban_Banner *Ban);
void Ban_RemoveBanner (void);
void Ban_UnhideBanner (void);
void Ban_HideBanner (void);
void Ban_RenameBannerShort (void);
void Ban_RenameBannerFull (void);
void Ban_ChangeBannerImg (void);
void Ban_ChangeBannerWWW (void);
void Ban_ContEditAfterChgBan (void);
void Ban_ReceiveNewBanner (void);
void Ban_WriteMenuWithBanners (void);

void Ban_ClickOnBanner (void);
long Ban_GetBanCodClicked (void);

#endif
