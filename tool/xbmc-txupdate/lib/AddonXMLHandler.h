/*
 *      Copyright (C) 2012 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */
#pragma once

#include "POHandler.h"
#include "TinyXML/tinyxml.h"

class CAddonXMLHandler
{
public:
  CAddonXMLHandler();
  ~CAddonXMLHandler();
  bool LoadAddonXMLFile (std::string strAddonXMLFilename);
  bool UpdateAddonXMLFile (std::string strAddonXMLFilename);
  bool FetchAddonXMLFileUpstr (std::string strURL);
  bool UpdateAddonChangelogFile (std::string strFilename, std::string strFormat);
  bool FetchAddonChangelogFile (std::string strURL);
  bool LoadCoreVersion(std::string filename);
  bool FetchCoreVersionUpstr(std::string strURL);
  std::string GetResHeaderPretext () const {return m_strResourceData;}
  std::map<std::string, CAddonXMLEntry> * GetMapAddonXMLData () {return &m_mapAddonXMLData;}
  void SetMapAddonXMLData (std::map<std::string, CAddonXMLEntry> mapData) {m_mapAddonXMLData = mapData;}
  std::string GetStrAddonXMLFile() const {return m_strAddonXMLFile;}
  void SetStrAddonXMLFile(std::string const &strAddonXMLFile) {m_strAddonXMLFile = strAddonXMLFile;}
  std::string GetAddonVersion () const {return m_strAddonVersion;}
  void SetAddonVersion(std::string const &strAddonVersion) {m_strAddonVersion = strAddonVersion;}
  std::string GetAddonChangelogFile () const {return m_strChangelogFile;}
  void SetAddonChangelogFile(std::string const &strAddonChangelogFile) {m_strChangelogFile = strAddonChangelogFile;}
  std::string GetAddonLogFilename () const {return m_strLogFilename;}
  void SetAddonLogFilename(std::string const &strAddonLogFilename) {m_strLogFilename = strAddonLogFilename;}

protected:
  bool ProcessAddonXMLFile (std::string AddonXMLFilename, TiXmlDocument &xmlAddonXML);
  bool ProcessCoreVersion(std::string filename, std::string &strBuffer);
  bool GetEncoding(const TiXmlDocument* pDoc, std::string& strEncoding);
  void BumpVersionNumber();
  void UpdateVersionNumber();
  std::string CstrToString(const char * StrToEscape);
  std::string GetXMLEntry (std::string const &strprefix, size_t &pos1, size_t &pos2);
  void CleanWSBetweenXMLEntries (std::string &strXMLString);
  std::map<std::string, CAddonXMLEntry> m_mapAddonXMLData;
  std::map<std::string, CAddonXMLEntry>::iterator itAddonXMLData;
  std::string m_strResourceData;
  std::string m_strAddonXMLFile;
  std::string m_strAddonVersion;
  std::string m_strChangelogFile;
  std::string m_strLogFilename;
};
