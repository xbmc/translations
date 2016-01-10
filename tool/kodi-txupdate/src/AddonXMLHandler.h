/*
 *      Copyright (C) 2014 Team Kodi
 *      http://www.xbmc.org
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
#pragma once

#include "POHandler.h"
#include "TinyXML/tinyxml.h"
#include "ConfigHandler.h"
#include <set>

struct COtherAddonMetadata
{
  std::string strLanguage;
  std::string strPlatform;
  std::string strLicense;
  std::string strForum;
  std::string strWebsite;
  std::string strEmail;
  std::string strSource;
};

class CAddonXMLHandler
{
public:
  CAddonXMLHandler();
  ~CAddonXMLHandler();
  void SetResData (const CResData& ResData) {m_ResData = ResData;}
  void SetBumpAddonVersion () {m_bBumpAddoXMLVersion = true;}
  void GenerateAddonXMLFile ();
  void WriteAddonXMLFile (std::string strAddonXMLFilename);

  void GenerateChangelogFile (std::string strFormat);
  bool WriteAddonChangelogFile (const std::string& strFilename);
  bool FetchAddonChangelogFile ();
  void FetchAddonDataFiles();
  std::string GetResHeaderPretext () const {return m_strResourceData;}
  void AddAddonXMLLangsToList(std::set<std::string>& listLangs);
  void SetAddonXMLEntry (const CAddonXMLEntry& AddonXMLEntry, const std::string& sLang) {m_mapAddonXMLData[sLang] = AddonXMLEntry;}
  void ClearAllAddonXMLEntries () {m_mapAddonXMLData.clear();}
  const CAddonXMLEntry& GetAddonXMLEntry(const std::string& sLang) const {return m_mapAddonXMLData.at(sLang);}
  bool FindAddonXMLEntry(const std::string& sLang) const {return m_mapAddonXMLData.find(sLang) != m_mapAddonXMLData.end();}

protected:
  bool FetchAddonXMLFileUpstr ();

  bool GetEncoding(const TiXmlDocument* pDoc, std::string& strEncoding);
  void BumpVersionNumber();
  void UpdateVersionNumber();
  std::string CstrToString(const char * StrToEscape);
  std::string GetXMLEntry (std::string const &strprefix, size_t &pos1, size_t &pos2);

  std::map<std::string, CAddonXMLEntry> m_mapAddonXMLData;
  typedef std::map<std::string, CAddonXMLEntry>::iterator T_itAddonXMLData;

  std::string m_strResourceData;
  std::string m_strAddonXMLFile;
  std::string m_strAddonVersion;
  std::string m_strChangelogFile;
  bool m_bBumpAddoXMLVersion;
  COtherAddonMetadata m_AddonMetadata;
  CResData m_ResData;
};
