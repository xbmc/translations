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

#include "TinyXML/tinyxml.h"
#include <string>
#include <map>
#include <list>

const std::string DEFAULTMERGEDLANGDIR = "merged-langfiles";

class CInputData
{
public:
  CInputData();
  ~CInputData();
  std::string strAddonName;
  std::string strAddonDir;
  std::string strGittemplate;
  bool bSkipChangelog;
  bool bSkipEnglishFile;
};

class CXMLResdata
{
public:
  CXMLResdata();
  ~CXMLResdata();
  std::string strTranslationrepoURL;
  std::string strUpstreamURL;
  std::string strProjName;
  std::string strResNameFull;
  std::string strMergedLangfileDir;
  std::string strLangsFromUpstream;
  int Restype;
  std::string strResDirectory;
  std::string strTXResName;
  std::string strLangFileType;
  std::string strURLSuffix;
  std::string strDIRprefix;
  std::string strAddonXMLSuffix;
  std::string strLogFormat;
  std::string strLogFilename;
  std::string strResLocalDirectory;
  std::string strResName;
  bool bWritePO, bWriteXML, bHasChangelog;
  bool bSkipChangelog;
  bool bSkipEnglishFile;
};

class CUpdateXMLHandler
{
public:
  CUpdateXMLHandler();
  ~CUpdateXMLHandler();
  bool DownloadXMLToMap (std::string strURL, std::map<std::string, CXMLResdata> &mapResourceData, std::string const &strTXProjectname);
};

class CInputXMLHandler
{
public:
  CInputXMLHandler();
  ~CInputXMLHandler();
  std::list<CInputData> ReadXMLToMem (std::string strFileName);
};
