// swad_file_extension.c: file extensions allowed in file browsers

/*
    SWAD (Shared Workspace At a Distance),
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

#include "swad_file_extension.h"
#include "swad_string.h"

/*****************************************************************************/
/*************************** Internal constants ******************************/
/*****************************************************************************/

/* Extensions allowed for uploaded files */
const char *Ext_FileExtensionsAllowed[] =
  {
   "3gp"  ,	// Video Android mobile
   "7z"   ,
   "asm"  ,
   "avi"  ,
   "bas"  ,
   "bat"  ,
   "bbl"  ,
   "bib"  ,
   "bin"  ,
   "bmp"  ,
   "bz2"  ,
   "c"    ,
   "cc"   ,
   "cdr"  ,
   "cpp"  ,
   "css"  ,
   "csv"  ,
   "dmg"  ,
   "doc"  ,
   "docx" ,
   "dotm" ,
   "dwd"  ,
   "eps"  ,
   "fdf"  ,
   "flv"  ,
   "gdb"  ,	// GNU Debugger Script
   "ggb"  ,
   "ggt"  ,
   "gif"  ,
   "gz"   ,
   "h"    ,
   "hex"  ,
   "htm"  ,
   "html" ,
   "img"  ,
   "java" ,
   "jpg"  ,
   "jpeg" ,
   "latex",
   "m"    ,
   "mdb"  ,
   "mht"  ,
   "mhtml",
   "mid"  ,
   "mov"  ,
   "mp3"  ,
   "mp4"  ,
   "mpeg" ,
   "mpg"  ,
   "mpp"  ,
   "mus"  ,
   "nb"   ,
   "odb"  ,
   "odc"  ,
   "odf"  ,
   "odg"  ,
   "odi"  ,
   "odm"  ,
   "odp"  ,
   "ods"  ,
   "odt"  ,
   "otg"  ,
   "otp"  ,
   "ots"  ,
   "ott"  ,
   "pas"  ,
   "pl"   ,
   "pdf"  ,
   "png"  ,
   "pps"  ,
   "ppsx" ,
   "ppt"  ,
   "pptx" ,
   "ps"   ,
   "pss"  ,
   "qt"   ,
   "r"    ,
   "ram"  ,
   "rar"  ,
   "raw"  ,
   "rdata",
   "rm"   ,
   "rp"   ,	// Axure, http://www.axure.com/
   "rtf"  ,
   "s"    ,
   "sav"  ,	// SPSS Data File
   "sbs"  ,
   "sf3"  ,
   "sgp"  ,
   "spp"  ,
   "spo"  ,
   "sps"  ,
   "swf"  ,
   "sws"  ,
   "tar"  ,
   "tex"  ,
   "tgz"  ,
   "tif"  ,
   "txt"  ,
   "voc"  ,
   "vp"   ,	// Justinmind, http://www.justinmind.com/
   "wav"  ,
   "wma"  ,
   "wmv"  ,
   "wxm"  ,
   "wxmx" ,
   "xls"  ,
   "xlsx" ,
   "zip"  ,
  };

const unsigned Ext_NUM_FILE_EXT_ALLOWED = sizeof (Ext_FileExtensionsAllowed) /
					  sizeof (Ext_FileExtensionsAllowed[0]);

/*****************************************************************************/
/******************* Check if file extension is allowed **********************/
/*****************************************************************************/

bool Ext_CheckIfFileExtensionIsAllowed (const char FilFolLnkName[NAME_MAX + 1])
  {
   unsigned Type;

   for (Type = 0;
	Type < Ext_NUM_FILE_EXT_ALLOWED;
	Type++)
      if (Str_FileIs (FilFolLnkName,
	              Ext_FileExtensionsAllowed[Type]))
	 return true;

   return false;
  }
