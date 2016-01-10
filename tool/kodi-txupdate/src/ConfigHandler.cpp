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

#include "ConfigHandler.h"
#include "Log.h"
#include <stdlib.h>
#include <sstream>
#include "HTTPUtils.h"
#include "CharsetUtils.h"
#include "FileUtils.h"


using namespace std;

CResData::CResData()
{
 iMinComplPercent = 40;
 iCacheExpire = 60; //minutes
 iGitPushInterval = 5; //days
 bForceComm = false;
 bRebrand = false;
 bForceTXUpd = false;
 bForceGitDloadToCache = false;
 bSkipGitReset = false;
 bSkipVersionBump = false;
 bMajorBump = false;
 bHasOnlyAddonXML = false;
 bIsLangAddon = false;
 bForceGitPush = false;
 bSkipGitPush = false;
};

CResData::~CResData()
{};

CBasicGITData::CBasicGITData()
{
  bHasBeenAnSRCFileChange = false;
};

CBasicGITData::~CBasicGITData()
{};

CConfigHandler::CConfigHandler()
{};

CConfigHandler::~CConfigHandler()
{};

void CConfigHandler::SubstituteExternalVariables(std::string& sVal, bool bIgnoreMissing)
{
  size_t iCurrPos = 0;
  size_t iNextPos = 0;
  while ((iNextPos = sVal.find('$', iCurrPos)) != std::string::npos)
  {
    // If the char is at the end of string, or if it is an internal var like $(LCODE), skip
    if (iNextPos + 1 == sVal.size() || sVal.at(iNextPos +1) == '(')
    {
      iCurrPos = iNextPos +1;
      continue;
    }
    size_t iVarLength = 1;
    while (iVarLength + iNextPos < sVal.size())
    {
      size_t iMatchedEntries = FindVariable(sVal.substr(iNextPos+1, iVarLength));
      if (iMatchedEntries == std::string::npos)
      {
        if (!bIgnoreMissing)
          CLog::Log(logERROR, "Undefined variable in value: %s", sVal.c_str());
        sVal = "";
        return;
      }
      if (iMatchedEntries == 0)
        break;
      iVarLength++;
    }

    if ((iVarLength + iNextPos) == sVal.size())
      CLog::Log(logERROR, "Undefined variable in value: %s", sVal.c_str());

    std::string sVarToReplace = sVal.substr(iNextPos+1, iVarLength);
    std::string sReplaceString = m_MapOfVariables.at(sVarToReplace);
    sVal.replace(iNextPos,iVarLength +1, sReplaceString);

    iCurrPos = iNextPos + sReplaceString.size();
  }
}

size_t CConfigHandler::FindVariable(const std::string& sVar)
{
  size_t iMatchedEntries = 0;
  bool bExactMatchFound = false;

  for (std::map<std::string, std::string>::iterator it = m_MapOfVariables.begin(); it != m_MapOfVariables.end(); it++)
  {
    if (it->first.find(sVar) != std::string::npos)
    {
      iMatchedEntries++;
      if (sVar.size() == it->first.size())
        bExactMatchFound = true;
    }
  }
  if (iMatchedEntries == 0)
    return std::string::npos;
  if (iMatchedEntries == 1 && bExactMatchFound)
    return 0;
  return iMatchedEntries;
}

void CConfigHandler::SetExternalVariables(const std::string& sLine)
{
  size_t iPosVar1 = 0;
  size_t iPosVar2 = sLine.find(" = ", iPosVar1);

  if (iPosVar2 == std::string::npos)
    CLog::Log(logERROR, "ConfHandler: Wrong line in conf file. variable = value format is wrong for line:\n%s", sLine.c_str());

  std::string sVar = sLine.substr(iPosVar1, iPosVar2-iPosVar1);

  std::string sVal = g_CharsetUtils.UnescapeCPPString(sLine.substr(iPosVar2 + 3));

  SubstituteExternalVariables(sVal, false);

  m_MapOfVariables[sVar] = sVal;
}

void CConfigHandler::SetInternalVariables(const std::string& sLine, CResData& ResData)
{
  size_t iPosVar1 = sLine.find(" ",0) +1;
  size_t iPosVar2 = sLine.find(" = ", iPosVar1);

  if (iPosVar2 == std::string::npos)
    CLog::Log(logERROR, "ConfHandler: Wrong line in conf file. set variable = value format is wrong for line:\n%s", sLine.c_str());

  std::string sVar = sLine.substr(iPosVar1, iPosVar2-iPosVar1);

  std::string sVal = g_CharsetUtils.UnescapeCPPString(sLine.substr(iPosVar2 + 3));

  std::string sVarDerived, sValDerived;
  if (sVar == "MRG" || sVar == "LOC" || sVar == "UPS" || sVar == "UPSSRC" || sVar == "LOCSRC")
  {
    //Examine if we have a simplified assign of variables, by only referring to the as groups like MRG, LOC, UPS etc.
    sValDerived = sVal + "Owner"; sVarDerived = sVar + "Owner";
    SubstituteExternalVariables(sValDerived, true); SetInternalVariable(sVarDerived, sValDerived, ResData, true);
    sValDerived = sVal + "Repo"; sVarDerived = sVar + "Repo";
    SubstituteExternalVariables(sValDerived, true); SetInternalVariable(sVarDerived, sValDerived, ResData, true);
    sValDerived = sVal + "Branch"; sVarDerived = sVar + "Branch";
    SubstituteExternalVariables(sValDerived, true); SetInternalVariable(sVarDerived, sValDerived, ResData, true);
    sValDerived = sVal + "LPath"; sVarDerived = sVar + "LPath";
    SubstituteExternalVariables(sValDerived, true); SetInternalVariable(sVarDerived, sValDerived, ResData, true);
    sValDerived = sVal + "AXMLPath"; sVarDerived = sVar + "AXMLPath";
    SubstituteExternalVariables(sValDerived, true); SetInternalVariable(sVarDerived, sValDerived, ResData, true);
    sValDerived = sVal + "LFormInAXML"; sVarDerived = sVar + "LFormInAXML";
    SubstituteExternalVariables(sValDerived, true); SetInternalVariable(sVarDerived, sValDerived, ResData, true);
    sValDerived = sVal + "ChLogPath"; sVarDerived = sVar + "ChLogPath";
    SubstituteExternalVariables(sValDerived, true); SetInternalVariable(sVarDerived, sValDerived, ResData, true);

    return;
  }

  if (sVar == "UPD" || sVar == "TRX")
  {
    //Examine if we have a simplified assing of variables, by only referring to the as groups like TRX, UPD etc.
    sValDerived = sVal + "ProjectName"; sVarDerived = sVar + "ProjectName";
    SubstituteExternalVariables(sValDerived, true); SetInternalVariable(sVarDerived, sValDerived, ResData, true);
    sValDerived = sVal + "LongProjectName"; sVarDerived = sVar + "LongProjectName";
    SubstituteExternalVariables(sValDerived, true); SetInternalVariable(sVarDerived, sValDerived, ResData, true);
    sValDerived = sVal + "LForm"; sVarDerived = sVar + "LForm";
    SubstituteExternalVariables(sValDerived, true); SetInternalVariable(sVarDerived, sValDerived, ResData, true);
    return;
  }

  SubstituteExternalVariables(sVal, false);

  return SetInternalVariable(sVar, sVal, ResData, false);
}

void CConfigHandler::ClearVariables(const std::string& sLine, CResData& ResData)
{
  std::string sVar = sLine.substr(6);

  if (sVar.empty())
    CLog::Log(logERROR, "ConfHandler: Wrong line in conf file. Clear variable name is empty.");

  if (sVar.find('*') != sVar.size()-1)
    SetInternalVariable(sVar, "", ResData, false);

  //We clear variables that has a match at the begining with our string
  sVar = sVar.substr(0,sVar.size()-1);

  for (std::map<std::string, std::string>::iterator it = m_MapOfVariables.begin(); it != m_MapOfVariables.end(); it++)
  {
    if (it->first.find(sVar) == 0)
      SetInternalVariable(it->first, "", ResData, false);
  }
}

void CConfigHandler::SetInternalVariable(const std::string& sVar, const std::string sVal, CResData& ResData, bool bIgnoreMissing)
{
  if (sVar == "UPSOwner")                   ResData.UPS.Owner = sVal;
  else if (sVar == "UPSRepo")               ResData.UPS.Repo = sVal;
  else if (sVar == "UPSBranch")             ResData.UPS.Branch = sVal;

  else if (sVar == "UPSLPath")              ResData.UPS.LPath = sVal;
  else if (sVar == "UPSAXMLPath")           ResData.UPS.AXMLPath = sVal;
  else if (sVar == "UPSLFormInAXML")        ResData.UPS.LFormInAXML = sVal;
  else if (sVar == "UPSChLogPath")          ResData.UPS.ChLogPath = sVal;

  else if (sVar == "LOCOwner")              ResData.LOC.Owner = sVal;
  else if (sVar == "LOCRepo")               ResData.LOC.Repo = sVal;
  else if (sVar == "LOCBranch")             ResData.LOC.Branch = sVal;

  else if (sVar == "LOCLPath")              ResData.LOC.LPath = sVal;
  else if (sVar == "LOCAXMLPath")           ResData.LOC.AXMLPath = sVal;
  else if (sVar == "LOCLFormInAXML")        ResData.LOC.LFormInAXML = sVal;
  else if (sVar == "LOCChLogPath")          ResData.LOC.ChLogPath = sVal;

  else if (sVar == "MRGLPath")              ResData.MRG.LPath = sVal;
  else if (sVar == "MRGAXMLPath")           ResData.MRG.AXMLPath = sVal;
  else if (sVar == "MRGChLogPath")          ResData.MRG.ChLogPath = sVal;

  else if (sVar == "UPSSRCOwner")           ResData.UPSSRC.Owner = sVal;
  else if (sVar == "UPSSRCRepo")            ResData.UPSSRC.Repo = sVal;
  else if (sVar == "UPSSRCBranch")          ResData.UPSSRC.Branch = sVal;

  else if (sVar == "UPSSRCLPath")           ResData.UPSSRC.LPath = sVal;
  else if (sVar == "UPSSRCAXMLPath")        ResData.UPSSRC.AXMLPath = sVal;

  else if (sVar == "LOCSRCOwner")           ResData.LOCSRC.Owner = sVal;
  else if (sVar == "LOCSRCRepo")            ResData.LOCSRC.Repo = sVal;
  else if (sVar == "LOCSRCBranch")          ResData.LOCSRC.Branch = sVal;

  else if (sVar == "LOCSRCLPath")           ResData.LOCSRC.LPath = sVal;
  else if (sVar == "LOCSRCAXMLPath")        ResData.LOCSRC.AXMLPath = sVal;
  else if (sVar == "LOCSRCLFormInAXML")     ResData.LOCSRC.LFormInAXML = sVal;
  else if (sVar == "LOCSRCChLogPath")       ResData.LOCSRC.ChLogPath = sVal;

  else if (sVar == "TRXProjectName")        ResData.TRX.ProjectName = sVal;
  else if (sVar == "TRXLongProjectName")    ResData.TRX.LongProjectName = sVal;
  else if (sVar == "TRXResName")            ResData.TRX.ResName = sVal;
  else if (sVar == "TRXLForm")              ResData.TRX.LForm = sVal;

  else if (sVar == "UPDProjectName")        ResData.UPD.ProjectName = sVal;
  else if (sVar == "UPDLongProjectName")    ResData.UPD.LongProjectName = sVal;
  else if (sVar == "UPDResName")            ResData.UPD.ResName = sVal;
  else if (sVar == "UPDLForm")              ResData.UPD.LForm = sVal;

  else if (sVar == "ResName")               ResData.sResName = sVal;
  else if (sVar == "ChgLogFormat")          ResData.sChgLogFormat = sVal;
  else if (sVar == "GitCommitText")         ResData.sGitCommitText = sVal;
  else if (sVar == "GitCommitTextSRC")      ResData.sGitCommitTextSRC = sVal;
  else if (sVar == "MRGLFilesDir")          ResData.sMRGLFilesDir = sVal;
  else if (sVar == "UPSLocalPath")          ResData.sUPSLocalPath = sVal;
  else if (sVar == "UPDLFilesDir")          ResData.sUPDLFilesDir = sVal;
  else if (sVar == "SupportEmailAddr")      ResData.sSupportEmailAddr = sVal;
  else if (sVar == "SRCLCode")              ResData.sSRCLCode = sVal;
  else if (sVar == "BaseLForm")             ResData.sBaseLForm = sVal;
  else if (sVar == "LTeamLFormat")          ResData.sLTeamLFormat = sVal;
  else if (sVar == "LDatabaseURL")          ResData.sLDatabaseURL = sVal;
  else if (sVar == "MinComplPercent")       ResData.iMinComplPercent = strtol(&sVal[0], NULL, 10);
  else if (sVar == "CacheExpire")           ResData.iCacheExpire = strtol(&sVal[0], NULL, 10); //in minutes
  else if (sVar == "GitPushInterval")       ResData.iGitPushInterval = strtol(&sVal[0], NULL, 10); //in minutes
  else if (sVar == "ForceComm")             ResData.bForceComm = (sVal == "true");
  else if (sVar == "ForceGitDloadToCache")  ResData.bForceGitDloadToCache = (sVal == "true");
  else if (sVar == "SkipGitReset")          ResData.bSkipGitReset = (sVal == "true");
  else if (sVar == "SkipGitPush")           ResData.bSkipGitPush = (sVal == "true");
  else if (sVar == "ForceGitPush")          ResData.bForceGitPush = (sVal == "true");
  else if (sVar == "Rebrand")               ResData.bRebrand = (sVal == "true");
  else if (sVar == "ForceTXUpd")            ResData.bForceTXUpd = (sVal == "true");
  else if (sVar == "IsLangAddon")           ResData.bIsLangAddon = (sVal == "true");
  else if (sVar == "HasOnlyAddonXML")       ResData.bHasOnlyAddonXML = (sVal == "true");
  else if (bIgnoreMissing)
    return;

  else
    CLog::Log(logERROR, "ConfHandler: Unrecognised internal variable name: \"%s\"", sVar.c_str());

  m_MapOfVariables[sVar] = sVal;
}

void CConfigHandler::LoadResDataToMem (std::string rootDir, std::map<std::string, CResData> & mapResData, std::map<std::string, CBasicGITData> * pMapGitRepos,
                                          std::map<int, std::string>& mapResOrder)
{
  iResCounter = 0;
  std::string sConfFile = g_File.ReadFileToStr(rootDir + "kodi-txupdate.conf");
  if (sConfFile == "")
    CLog::Log(logERROR, "Confhandler: erroe: missing conf file.");

  size_t iPos1 = 0;
  size_t iPos2 = 0;

  CResData ResData;

  ResData.sProjRootDir = rootDir;
  ResData.m_pMapGitRepos = pMapGitRepos;


  while ((iPos2 = sConfFile.find('\n', iPos1)) != std::string::npos)
  {
    std::string sLine = sConfFile.substr(iPos1, iPos2-iPos1);
    iPos1 = iPos2 +1;

    if (sLine.empty() || sLine.find('#') == 0) // If line is empty or a comment line, ignore
      continue;

    if (sLine.find("set ") == 0)
      SetInternalVariables(sLine, ResData);
    else if (sLine.find("pset ") == 0)
      m_vecPermVariables.push_back(sLine);
    else if (sLine.find("tset ") == 0)
      m_vecTempVariables.push_back(sLine);

    else if (sLine.find("clear ") == 0)
      ClearVariables(sLine, ResData);
    else if (sLine.find("create resource ") == 0)
      CreateResource(ResData, sLine, mapResData, mapResOrder);
    else if (sLine.find("setvar ") == 0)
      SetExternalVariables(sLine.substr(7));
    else
      CLog::Log(logERROR, "ConfHandler: Unrecognised command in line: %s", sLine.c_str());

  }
}

void CConfigHandler::HandlePermanentVariables(CResData& ResData)
{
  for (std::vector<std::string>::iterator itvec = m_vecPermVariables.begin(); itvec != m_vecPermVariables.end(); itvec++)
  {
    SetInternalVariables(*itvec, ResData);
  }
}

void CConfigHandler::HandleTemporaryVariables(CResData& ResData)
{

  for (std::vector<std::string>::iterator itvec = m_vecTempVariables.begin(); itvec != m_vecTempVariables.end(); itvec++)
  {
    SetInternalVariables(*itvec, ResData);
  }
}


std::string CConfigHandler::ReplaceResName(std::string sVal, const CResData& ResData)
{
  g_CharsetUtils.replaceAllStrParts (&sVal, "$(RESNAME)", ResData.sResName);
  g_CharsetUtils.replaceAllStrParts (&sVal, "$(TRXRESNAME)", ResData.TRX.ResName);
  return sVal;
}

void CConfigHandler::CreateResource(CResData& ResData, const std::string& sLine, std::map<std::string, CResData> & mapResData, std::map<int, std::string>& mapResOrder)
{
  HandlePermanentVariables(ResData); //Handle Permanent variable assignements, which get new values after each create resource

  //save variable and resdata state before we handle the temporary assignments. After creating the resource we will reset this state
  CResData SavedResdata = ResData;
  map <string, string >  Saved_MapOfVariables = m_MapOfVariables;
  HandleTemporaryVariables(ResData);

  CResData ResDataToStore;

  //Parse the resource names
  size_t posResName, posTRXResName;
  if ((posResName = sLine.find("ResName = ")) == std::string::npos)
    CLog::Log(logERROR, "Confhandler: Cannot create resource, missing ResName");
  posResName = posResName +10;

  ResDataToStore.sResName = sLine.substr(posResName, sLine.find(' ', posResName)-posResName);

  if ((posTRXResName = sLine.find("TRXResName = ")) == std::string::npos)
    CLog::Log(logERROR, "Confhandler: Cannot create resource, missing TRXResName");

  posTRXResName = posTRXResName +13;
  ResDataToStore.TRX.ResName = sLine.substr(posTRXResName, sLine.find_first_of(" ,\n", posTRXResName)-posTRXResName);


  if (sLine.find("GITCommit") != std::string::npos)
    ResDataToStore.sGitCommitText = ReplaceResName(ResData.sGitCommitText, ResDataToStore);

  //Changes in SRC file will be handled for ALL resources separately no matter where we have GITCommit command
  ResDataToStore.sGitCommitTextSRC = ReplaceResName(ResData.sGitCommitTextSRC, ResDataToStore);

  if (sLine.find("SkipVersionBump") != std::string::npos)
    ResDataToStore.bSkipVersionBump = true;

  if (sLine.find("MajorVersionBump") != std::string::npos)
    ResDataToStore.bMajorBump = true;

  ResDataToStore.UPS.Owner            = ReplaceResName(ResData.UPS.Owner, ResDataToStore);
  ResDataToStore.UPS.Repo             = ReplaceResName(ResData.UPS.Repo, ResDataToStore);
  ResDataToStore.UPS.Branch           = ReplaceResName(ResData.UPS.Branch, ResDataToStore);

  ResDataToStore.UPS.LPath            = ReplaceResName(ResData.UPS.LPath, ResDataToStore);
  ResDataToStore.UPS.AXMLPath         = ReplaceResName(ResData.UPS.AXMLPath, ResDataToStore);
  ResDataToStore.UPS.LFormInAXML      = ReplaceResName(ResData.UPS.LFormInAXML, ResDataToStore);
  ResDataToStore.UPS.ChLogPath        = ReplaceResName(ResData.UPS.ChLogPath, ResDataToStore);

  ResDataToStore.LOC.Owner            = ReplaceResName(ResData.LOC.Owner, ResDataToStore);
  ResDataToStore.LOC.Repo             = ReplaceResName(ResData.LOC.Repo, ResDataToStore);
  ResDataToStore.LOC.Branch           = ReplaceResName(ResData.LOC.Branch, ResDataToStore);

  ResDataToStore.LOC.LPath            = ReplaceResName(ResData.LOC.LPath, ResDataToStore);
  ResDataToStore.LOC.AXMLPath         = ReplaceResName(ResData.LOC.AXMLPath, ResDataToStore);
  ResDataToStore.LOC.LFormInAXML      = ReplaceResName(ResData.LOC.LFormInAXML, ResDataToStore);
  ResDataToStore.LOC.ChLogPath        = ReplaceResName(ResData.LOC.ChLogPath, ResDataToStore);

  ResDataToStore.MRG.LPath            = ReplaceResName(ResData.MRG.LPath, ResDataToStore);
  ResDataToStore.MRG.AXMLPath         = ReplaceResName(ResData.MRG.AXMLPath, ResDataToStore);
//ResDataToStore.MRG.LFormInAXML      = ReplaceResName(ResData.MRG.LFormInAXML, ResDataToStore);
  ResDataToStore.MRG.ChLogPath        = ReplaceResName(ResData.MRG.ChLogPath, ResDataToStore);

  ResDataToStore.UPSSRC.Owner         = ReplaceResName(ResData.UPSSRC.Owner, ResDataToStore);
  ResDataToStore.UPSSRC.Repo          = ReplaceResName(ResData.UPSSRC.Repo, ResDataToStore);
  ResDataToStore.UPSSRC.Branch        = ReplaceResName(ResData.UPSSRC.Branch, ResDataToStore);

  ResDataToStore.UPSSRC.LPath         = ReplaceResName(ResData.UPSSRC.LPath, ResDataToStore);
  ResDataToStore.UPSSRC.AXMLPath      = ReplaceResName(ResData.UPSSRC.AXMLPath, ResDataToStore);
//ResDataToStore.UPSSRC.LFormInAXML   = ReplaceResName(ResData.UPSSRC.LFormInAXML, ResDataToStore);
//ResDataToStore.UPSSRC.ChLogPath     = ReplaceResName(ResData.UPSSRC.ChLogPath, ResDataToStore);

  ResDataToStore.LOCSRC.Owner         = ReplaceResName(ResData.LOCSRC.Owner, ResDataToStore);
  ResDataToStore.LOCSRC.Repo          = ReplaceResName(ResData.LOCSRC.Repo, ResDataToStore);
  ResDataToStore.LOCSRC.Branch        = ReplaceResName(ResData.LOCSRC.Branch, ResDataToStore);

  ResDataToStore.LOCSRC.LPath         = ReplaceResName(ResData.LOCSRC.LPath, ResDataToStore);
  ResDataToStore.LOCSRC.AXMLPath      = ReplaceResName(ResData.LOCSRC.AXMLPath, ResDataToStore);
  ResDataToStore.LOCSRC.LFormInAXML   = ReplaceResName(ResData.LOCSRC.LFormInAXML, ResDataToStore);
  ResDataToStore.LOCSRC.ChLogPath     = ReplaceResName(ResData.LOCSRC.ChLogPath, ResDataToStore);

  ResDataToStore.TRX.ProjectName      = ReplaceResName(ResData.TRX.ProjectName, ResDataToStore);
  ResDataToStore.TRX.LongProjectName  = ReplaceResName(ResData.TRX.LongProjectName, ResDataToStore);
//ResDataToStore.TRX.ResName          = ReplaceResName(ResData.TRX.ResName, ResDataToStore);
  ResDataToStore.TRX.LForm            = ReplaceResName(ResData.TRX.LForm, ResDataToStore);

  ResDataToStore.UPD.ProjectName      = ReplaceResName(ResData.UPD.ProjectName, ResDataToStore);
  ResDataToStore.UPD.LongProjectName  = ReplaceResName(ResData.UPD.LongProjectName, ResDataToStore);
  ResDataToStore.UPD.ResName          = ReplaceResName(ResData.UPD.ResName, ResDataToStore);
  ResDataToStore.UPD.LForm            = ReplaceResName(ResData.UPD.LForm, ResDataToStore);


//ResDataToStore.sResName             = ReplaceResName(ResData.sResName, ResDataToStore);
  ResDataToStore.sChgLogFormat        = ReplaceResName(ResData.sChgLogFormat, ResDataToStore);

  ResDataToStore.sProjRootDir         = ReplaceResName(ResData.sProjRootDir, ResDataToStore);
  ResDataToStore.sMRGLFilesDir        = ReplaceResName(ResData.sMRGLFilesDir, ResDataToStore);
  ResDataToStore.sUPSLocalPath        = ReplaceResName(ResData.sUPSLocalPath, ResDataToStore);
  ResDataToStore.sUPDLFilesDir        = ReplaceResName(ResData.sUPDLFilesDir, ResDataToStore);
  ResDataToStore.sSupportEmailAddr    = ReplaceResName(ResData.sSupportEmailAddr, ResDataToStore);
  ResDataToStore.sSRCLCode            = ReplaceResName(ResData.sSRCLCode, ResDataToStore);
  ResDataToStore.sBaseLForm           = ReplaceResName(ResData.sBaseLForm, ResDataToStore);
  ResDataToStore.sLTeamLFormat        = ReplaceResName(ResData.sLTeamLFormat, ResDataToStore);
  ResDataToStore.sLDatabaseURL        = ReplaceResName(ResData.sLDatabaseURL, ResDataToStore);
  ResDataToStore.iMinComplPercent     = ResData.iMinComplPercent;
  ResDataToStore.iCacheExpire         = ResData.iCacheExpire;
  ResDataToStore.iGitPushInterval     = ResData.iGitPushInterval;
  ResDataToStore.bForceComm           = ResData.bForceComm;
  ResDataToStore.bRebrand             = ResData.bRebrand;
  ResDataToStore.bForceTXUpd          = ResData.bForceTXUpd;
  ResDataToStore.bForceGitDloadToCache= ResData.bForceGitDloadToCache;
  ResDataToStore.bSkipGitReset        = ResData.bSkipGitReset;
  ResDataToStore.bSkipGitPush         = ResData.bSkipGitPush;
  ResDataToStore.bForceGitPush        = ResData.bForceGitPush;
  ResDataToStore.bIsLangAddon         = ResData.bIsLangAddon;
  ResDataToStore.bHasOnlyAddonXML     = ResData.bHasOnlyAddonXML;
  ResDataToStore.m_pMapGitRepos       = ResData.m_pMapGitRepos;

  //If we don't have a different target trx resource name, use the source trx resource name
  if (ResDataToStore.UPD.ResName.empty())
    ResDataToStore.UPD.ResName = ResDataToStore.TRX.ResName;


  mapResData[ResDataToStore.sResName] = ResDataToStore;
  iResCounter++;
  mapResOrder[iResCounter] = ResDataToStore.sResName;

  ResData = SavedResdata;
  m_MapOfVariables = Saved_MapOfVariables;
  ResData.UPD.ResName.clear();
  m_vecTempVariables.clear();

  //Store git data for git clone the repositories needed for upstream push and pull handling
  if (ResDataToStore.sUPSLocalPath.empty())
    CLog::Log(logERROR, "Confhandler: missing folder path for local upstream git clone data.");

  AddGitRepoToList(ResDataToStore, ResDataToStore.UPS);
  AddGitRepoToList(ResDataToStore, ResDataToStore.LOC);

  if (ResDataToStore.bIsLangAddon)
  {
    AddGitRepoToList(ResDataToStore, ResDataToStore.UPSSRC);
    AddGitRepoToList(ResDataToStore, ResDataToStore.LOCSRC);
  }

}

void CConfigHandler::AddGitRepoToList(CResData& ResDataToStore, CGITData& GITData)
{
  if (GITData.Repo.empty() || GITData.Branch.empty() || GITData.Owner.empty())
    CLog::Log(logERROR, "Config handler: Insufficient git data. Missing Owner or Repo or Branch data.\n"
                        "Resname: %s\nRepo: %s\nBranch:%s\nOwner: %s\n",
                        ResDataToStore.sResName.c_str(), GITData.Repo.c_str(), GITData.Branch.c_str(), GITData.Owner.c_str());

  CBasicGITData BasicGitData;
  BasicGitData.Owner = GITData.Owner; BasicGitData.Repo = GITData.Repo; BasicGitData.Branch = GITData.Branch;
  BasicGitData.sUPSLocalPath = ResDataToStore.sUPSLocalPath; BasicGitData.iGitPushInterval = ResDataToStore.iGitPushInterval;
  BasicGitData.bSkipGitPush = ResDataToStore.bSkipGitPush; BasicGitData.bForceGitPush = ResDataToStore. bForceGitPush;
  ResDataToStore.m_pMapGitRepos->operator[](GITData.Owner + "/" + GITData.Repo + "/" + GITData.Branch) = BasicGitData;
  g_HTTPHandler.AddValidGitPushTimeCachefile(GITData.Owner, GITData.Repo, GITData.Branch);
}

bool CConfigHandler::GetParamsFromURLorPath (string const &strURL, string &strLangFormat, string &strFileName,
                                                 string &strURLRoot, const char strSeparator)
{
  if (strURL.empty())
    return false;

  size_t pos0, posStart, posEnd;

  pos0 = strURL.find_last_of("$");
  if (((posStart = strURL.find("$("), pos0) != std::string::npos) && ((posEnd = strURL.find(")",posStart)) != std::string::npos))
    strLangFormat = strURL.substr(posStart, posEnd - posStart +1);

  return GetParamsFromURLorPath (strURL, strFileName, strURLRoot, strSeparator);
}

bool CConfigHandler::GetParamsFromURLorPath (string const &strURL, string &strFileName,
                                                 string &strURLRoot, const char strSeparator)
{
  if (strURL.empty())
    return false;

  if (strURL.find(strSeparator) == std::string::npos)
    return false;

  strFileName = strURL.substr(strURL.find_last_of(strSeparator)+1);
  strURLRoot = g_CharsetUtils.GetRoot(strURL, strFileName);
  return true;
}