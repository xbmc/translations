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

#ifndef LANGCODES_H
#define LANGCODES_H

#pragma once

#include <string>
#include <map>
#include <list>
#include "ConfigHandler.h"

struct CLangcodes
{
  std::map<std::string, std::string> mapLangdata;
  int nplurals;
  std::string Pluralform;
};

class CLCodeHandler
{
public:
  CLCodeHandler();
  ~CLCodeHandler();
  void Init(const std::string strLangDatabaseURL, const CResData& ResData);
  std::string GetLangCodeFromAlias(std::string Alias, std::string AliasForm);
  std::string GetLangFromLCode(std::string LangCode, std::string AliasForm);
  int GetnPlurals(std::string LangToLook);
  std::string GetPlurForm(std::string LangToLook);
  std::string VerifyLangCode(std::string LangCode, const std::string &strLangformat);
  void CleanLangform (std::string &strLangform);
  std::map<std::string, std::string>  GetTranslatorsDatabase(const std::string& strContributorType, const std::string& strProjectName,
                                                             const CResData& ResData);
  void  UploadTranslatorsDatabase(std::map<std::string, std::string> &mapOfCoordinators,
                                  std::map<std::string, std::string> &mapOfReviewers,
                                  std::map<std::string, std::string> &mapOfTranslators,
                                  const std::string& strProjectName, const std::string& strTargetTXLFormat);
private:
  std::map<std::string, CLangcodes> ParseTransifexLanguageDatabase(std::string strJSON, const CResData& ResData);
  void AddGeneralRule(std::map<std::string, CLangcodes> &mapTXLangs, const std::string &strLeft, std::string strRight);
  void AddCustomRule(std::map<std::string, CLangcodes> &mapTXLangs, const std::string &strLangformat,
                     const std::string &strLeft, const std::string &strRight);
  void ParseLangDatabaseVersion(const std::string &strJSON, const std::string &strURL);

  std::map <std::string, CLangcodes> m_mapLCodes;
  std::map <std::string, CLangcodes>::iterator itmapLCodes;
};

extern CLCodeHandler g_LCodeHandler;

#endif
