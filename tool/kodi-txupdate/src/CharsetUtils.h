/*
 *      Copyright (C) 2005-2014 Team Kodi
 *      http://xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Kodi; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#ifndef CHARSETUTILS_H
#define CHARSETUTILS_H

#pragma once

#include <string>
#include <string.h>
#include <iconv.h>
#include <stdio.h>
#include <stdlib.h>

#define ICONV_PREPARE(iconv) iconv=(iconv_t)-1
#define UTF8_DEST_MULTIPLIER 6

class CCharsetUtils
{
public:
  std::string IntToStr(int number);
  std::string ChrToStr(char chr);
  std::string UnescapeCPPString(const std::string &strInput);
  std::string EscapeStringCPP(const std::string &strInput);
  std::string EscapeStringXML(const std::string &strInput);
  std::string ToUTF8(std::string strEncoding, const std::string& str);
  std::string UnWhitespace(std::string strInput);
  std::string CleanTranslatorlist(std::string strInput);
  std::string stringCharsetToUtf8(const std::string& strCP, std::string strIn);
  bool IsValidUTF8(std::string const &strToCheck);
  size_t GetCharCountInStr(std::string const &strToCheck, unsigned char chrToFInd);
  bool replaceAllStrParts(std::string * pstr, const std::string& from, const std::string& to);
  std::string replaceStrParts(std::string strToReplace, const std::string& from, const std::string& to);
  void reBrandXBMCToKodi(std::string * pstrtorebrand);
  std::string GetRoot(const std::string &strPath,const std::string &strFilename);
  std::string GetFilenameFromURL(const std::string &sURL);
  std::string GetLangnameFromPath(std::string strName, std::string strURL, std::string strLangformat);
  std::string ReplaceLanginURL(const std::string &strURL,const std::string &strLangFormat, const std::string &strLCode);
  void ConvertLineEnds(std::string &strBuffer);
  std::string GetLFormFromPath(const std::string& sPath);
};

extern CCharsetUtils g_CharsetUtils;

#endif