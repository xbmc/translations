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
#ifndef CONFIGHANDLER_H
#define CONFIGHANDLER_H

#pragma once

#include "TinyXML/tinyxml.h"
#include <string>
#include <map>
#include <vector>
#include <list>

struct CResChangeData
{
  std::string sResName;
  std::string sLangPath;
  std::string sLOCGITDir;
  std::string sGitCommitTextSRC;
};

struct CCommitData
{
  std::string sCommitMessage;
  std::list<CResChangeData> listResWithSRCChange;
  std::list<CResChangeData> listResWithChange;
};

class CBasicGITData
{
public:
  CBasicGITData();
  ~CBasicGITData();
  std::string Owner, Repo, Branch;
  std::string sUPSLocalPath;
  size_t iGitPushInterval; //in days
  size_t bSkipGitPush, bForceGitPush;
  std::list<CCommitData> listCommitData;
  bool bHasBeenAnSRCFileChange;
};

struct CGITData
{
  // L=Language, A = Addon
  std::string Owner, Repo, Branch;
  std::string LPath;
  std::string AXMLPath, LFormInAXML;
  std::string ChLogPath;
};

struct CTRXData
{
  std::string ProjectName;
  std::string LongProjectName;
  std::string ResName;
  std::string LForm;
};

class CResData
{
public:
  CResData();
  ~CResData();
  std::string sResName;

  //NEW
  CGITData UPS, UPSSRC;
  CGITData LOC, LOCSRC;
  CGITData MRG;
  CTRXData TRX, UPD;

  std::string sChgLogFormat;
  std::string sGitCommitText, sGitCommitTextSRC;
  bool bIsLangAddon;
  bool bHasOnlyAddonXML;

  std::string sProjRootDir;
  std::string sMRGLFilesDir;
  std::string sUPSLocalPath;
  std::string sUPDLFilesDir;
  std::string sSupportEmailAddr;
  std::string sSRCLCode;
  std::string sBaseLForm;
  std::string sLTeamLFormat;
  std::string sLDatabaseURL;
  int iMinComplPercent;
  int iCacheExpire;
  int iGitPushInterval; //in days
  bool bForceComm;
  bool bRebrand;
  bool bForceTXUpd;
  bool bForceGitDloadToCache;
  bool bSkipGitReset, bSkipGitPush, bForceGitPush;
  bool bSkipVersionBump;
  bool bMajorBump;
  std::map<std::string, CBasicGITData> * m_pMapGitRepos;
};

class CConfigHandler
{
public:
  CConfigHandler();
  ~CConfigHandler();
  void LoadResDataToMem (std::string rootDir, std::map<std::string, CResData> & mapResData, std::map<std::string, CBasicGITData> * pMapGitRepos,
                         std::map<int, std::string>& mapResOrder);

private:
  bool GetParamsFromURLorPath (std::string const &strURL, std::string &strLangFormat, std::string &strFileName,
                               std::string &strURLRoot, const char strSeparator);
  bool GetParamsFromURLorPath (std::string const &strURL, std::string &strFileName,
                               std::string &strURLRoot, const char strSeparator);

  std::map<std::string, std::string> m_MapOfVariables;
  std::vector<std::string> m_vecPermVariables;
  std::vector<std::string> m_vecTempVariables;
  size_t FindVariable(const std::string& sVar);
  void SetInternalVariables(const std::string& sLine, CResData& ResData);
  void SetExternalVariables(const std::string& sLine);
  void SubstituteExternalVariables(std::string& sVar, bool bIgnoreMissing);
protected:
  void CreateResource(CResData& ResData, const std::string& sLine, std::map<std::string, CResData> & mapResData, std::map<int, std::string>& mapResOrder);
  void HandlePermanentVariables(CResData& ResData);
  void HandleTemporaryVariables(CResData& ResData);
  std::string ReplaceResName(std::string sVal, const CResData& ResData);
  void ClearVariables(const std::string& sLine, CResData& ResData);
  void SetInternalVariable(const std::string& sVar, const std::string sVal, CResData& ResData, bool bIgnoreMissing);
  void AddGitRepoToList(CResData& ResDataToStore, CGITData& GITData);
  int iResCounter;
};

#endif