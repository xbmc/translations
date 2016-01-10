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

#include "ResourceHandler.h"
#include <list>
#include <algorithm>
#include "HTTPUtils.h"
#include "Langcodes.h"
#include "Fileversioning.h"
#include "jsoncpp/json/json.h"
#include "Log.h"
#include "CharsetUtils.h"
#include "FileUtils.h"

using namespace std;

CResourceHandler::CResourceHandler()
{};

CResourceHandler::CResourceHandler(const CResData& Resdata) : m_ResData(Resdata)
{
  m_AddonXMLHandler.SetResData(Resdata);
};

CResourceHandler::~CResourceHandler()
{};

// Download from Transifex related functions
bool CResourceHandler::FetchPOFilesTXToMem()
{
  g_HTTPHandler.SetLocation("TRX");
  g_HTTPHandler.SetResName(m_ResData.sResName);
  g_HTTPHandler.SetLCode("");
  g_HTTPHandler.SetProjectName(m_ResData.TRX.ProjectName);

  std::string strURL = "https://www.transifex.com/api/2/project/" + m_ResData.TRX.ProjectName + "/resource/" + m_ResData.TRX.ResName + "/";
  g_HTTPHandler.Cleanup();
  g_HTTPHandler.ReInit();
  CLog::Log(logPRINT, " Langlist");

  g_HTTPHandler.SetFileName("LanguageList.json");
  g_HTTPHandler.SetDataFile(true);

  std::string strtemp = g_HTTPHandler.GetURLToSTR(strURL + "stats/");
  if (strtemp.empty())
    CLog::Log(logERROR, "ResHandler::FetchPOFilesTXToMem: error getting po file list from transifex.net");

  std::list<std::string> listLCodesTX = ParseAvailLanguagesTX(strtemp, strURL);

  CPOHandler newPOHandler(m_ResData);

  g_HTTPHandler.SetFileName("strings.po");
  g_HTTPHandler.SetDataFile(false);

  for (std::list<std::string>::iterator it = listLCodesTX.begin(); it != listLCodesTX.end(); it++)
  {
    const std::string& sLCode = *it;
    CLog::Log(logPRINT, " %s", sLCode.c_str());
    m_mapTRX[sLCode] = newPOHandler;

    CPOHandler& POHandler = m_mapTRX[sLCode];
    POHandler.SetIfIsSourceLang(sLCode == m_ResData.sSRCLCode);
    POHandler.SetLCode(sLCode);
    g_HTTPHandler.SetLCode(sLCode);

    std::string sLangNameTX = g_LCodeHandler.GetLangFromLCode(*it, m_ResData.TRX.LForm);
    POHandler.FetchPOTXPathToMem(strURL + "translation/" + sLangNameTX + "/?file");
    POHandler.SetIfIsSourceLang(sLCode == m_ResData.sSRCLCode);
  }
  return true;
}

bool CResourceHandler::FetchPOFilesUpstreamToMem()
{
  g_HTTPHandler.Cleanup();
  g_HTTPHandler.ReInit();

  g_HTTPHandler.SetLocation("UPS");
  g_HTTPHandler.SetResName(m_ResData.sResName);
  g_HTTPHandler.SetLCode("");
  g_HTTPHandler.SetProjectName("");

  bool bHasLanguageFiles = !m_ResData.bHasOnlyAddonXML;

  std::set<std::string> listLangs, listLangsWithStringsPO;
  g_HTTPHandler.SetFileName("LocalFileList.txt");
  g_HTTPHandler.SetDataFile(true);

  CLog::Log(logPRINT, " GitDir");
  listLangsWithStringsPO = GetAvailLangsGITHUB();

  if (m_ResData.bIsLangAddon)
  {
    g_HTTPHandler.SetFileName("LocalFileList-SRC.txt");
    GetSRCFilesGitData(); //Get version data for the SRC files reside at a different github repo
    listLangsWithStringsPO.insert(m_ResData.sSRCLCode);
  }

  m_AddonXMLHandler.FetchAddonDataFiles();

  listLangs = listLangsWithStringsPO;

  if (!m_ResData.bHasOnlyAddonXML && listLangs.find(m_ResData.sSRCLCode) == listLangs.end())
    CLog::Log(logERROR, "Could not find source language file at the UPS github repo for resource: %s\n", m_ResData.sResName.c_str());

  m_AddonXMLHandler.AddAddonXMLLangsToList(listLangs); // Add languages that are only in the addon.xml file

  g_HTTPHandler.SetDataFile(false);


  for (std::set<std::string>::iterator it = listLangs.begin(); it != listLangs.end(); it++)
  {
    CPOHandler POHandler(m_ResData);
    const std::string& sLCode = *it;

    bool bLangHasStringsPO = listLangsWithStringsPO.find(sLCode) != listLangsWithStringsPO.end();

    bool bIsSourceLang = sLCode == m_ResData.sSRCLCode;
    POHandler.SetIfIsSourceLang(bIsSourceLang);
    POHandler.SetLCode(sLCode);
    g_HTTPHandler.SetLCode(sLCode);

    if (bLangHasStringsPO && bHasLanguageFiles)
      CLog::Log(logPRINT, " %s", sLCode.c_str());

    if (bLangHasStringsPO && bHasLanguageFiles && m_ResData.bIsLangAddon) // Download individual addon.xml files for language-addons
    {
      g_HTTPHandler.SetFileName("addon.xml");
      std::string sLangXMLPath;
      CGITData GitData;

      if (!bIsSourceLang)
        GitData = m_ResData.UPS;
      else
        GitData = m_ResData.UPSSRC;

      sLangXMLPath = g_CharsetUtils.ReplaceLanginURL (GitData.AXMLPath, g_CharsetUtils.GetLFormFromPath(GitData.AXMLPath), sLCode);
      POHandler.FetchLangAddonXML(sLangXMLPath, GitData);
    }

    if (bLangHasStringsPO && bHasLanguageFiles) // Download language file from upstream for language sLCode
    {
      g_HTTPHandler.SetFileName("strings.po");

      std::string sLPath;
      CGITData GitData;

      if (bIsSourceLang && m_ResData.bIsLangAddon) // If we have a different URL for source language, use that for download
        GitData = m_ResData.UPSSRC;
      else
        GitData = m_ResData.UPS;

      sLPath = g_CharsetUtils.ReplaceLanginURL (GitData.LPath, g_CharsetUtils.GetLFormFromPath(GitData.LPath), sLCode);

      POHandler.FetchPOGitPathToMem(sLPath, GitData);
    }

    if (m_AddonXMLHandler.FindAddonXMLEntry(sLCode))
      POHandler.AddAddonXMLEntries(m_AddonXMLHandler.GetAddonXMLEntry(sLCode), m_AddonXMLHandler.GetAddonXMLEntry(m_ResData.sSRCLCode));

    m_mapUPS[sLCode] = POHandler;

  }

  return true;
}

bool CResourceHandler::ComparePOFiles(CPOHandler& POHandler1, CPOHandler& POHandler2)
{
  if (POHandler1.GetClassEntriesCount() != POHandler2.GetClassEntriesCount())
    return false;

  T_itPOData itPO1 = POHandler1.GetPOMapBeginIterator();
  T_itPOData itPO2 = POHandler2.GetPOMapBeginIterator();
  T_itPOData itPOEnd1 = POHandler1.GetPOMapEndIterator();

  while (itPO1 != itPOEnd1)
  {
    if (!(itPO1->second == itPO2->second))
      return false;
    itPO2++;
    itPO1++;
  }
  return true;
}

void CResourceHandler::MergeResource()
{
  CLog::Log(logPRINT, "%s%s%s\n", KMAG, m_ResData.sResName.c_str(), RESET);

  std::list<std::string> listMergedLangs = CreateMergedLangList();
  CPOHandler& POHandlUPSSRC = m_mapUPS.at(m_ResData.sSRCLCode);
  m_bResChangedFromUPS = false;

  for (std::list<std::string>::iterator itlang = listMergedLangs.begin(); itlang != listMergedLangs.end(); itlang++)
  {
    const std::string& sLCode = *itlang;
    bool bPOChangedFromUPS = false;

    // check if lcode is the source lcode. If so we check if it has changed from the last uploaded one
    // if it has has changed set flag that we need to write a complete update PO file for the SRC language
    bool bWriteUPDFileSRC = false;
    if (sLCode == m_ResData.sSRCLCode)
    {
      if (m_mapTRX.find(m_ResData.sSRCLCode) != m_mapTRX.end())
      {
        CPOHandler& POHandlTRXSRC = m_mapTRX.at(m_ResData.sSRCLCode);
        if (!ComparePOFiles(POHandlTRXSRC, POHandlUPSSRC))       // if the source po file differs from the one at transifex we need to update it
          bWriteUPDFileSRC = true;
      }
      else
        bWriteUPDFileSRC = true;
    }

    T_itPOData itSRCUPSPO = POHandlUPSSRC.GetPOMapBeginIterator();
    T_itPOData itSRCUPSPOEnd = POHandlUPSSRC.GetPOMapEndIterator();

    //Let's iterate by the UPSTREAM source PO file for this resource
    for (; itSRCUPSPO != itSRCUPSPOEnd; itSRCUPSPO++)
    {
      CPOEntry& EntrySRC = itSRCUPSPO->second;

      bool bisInUPS = FindUPSEntry(sLCode, EntrySRC);
      bool bisInTRX = FindTRXEntry(sLCode, EntrySRC);

      //Handle the source language
      if (sLCode == m_ResData.sSRCLCode)
      {
        T_itPOData itPOUPS = GetUPSItFoundEntry();
        m_mapMRG[sLCode].AddItEntry(itPOUPS);
        if (bWriteUPDFileSRC)
          m_mapUPD[sLCode].AddItEntry(itPOUPS);
      }

      //We have non-source language. Let's treat it that way
      else if (bisInTRX)
      {
        T_itPOData itPOTRX = GetTRXItFoundEntry();
        m_mapMRG[sLCode].AddItEntry(itPOTRX);

        if (!bisInUPS) // check if this is a new translation at transifex, if so make changed flag true
          bPOChangedFromUPS = true;
        else
        {
          T_itPOData itPOUPS = GetUPSItFoundEntry();
          bPOChangedFromUPS = bPOChangedFromUPS || (itPOTRX->second.msgStr != itPOUPS->second.msgStr) ||
                              (itPOTRX->second.msgStrPlural != itPOUPS->second.msgStrPlural);
        }
      }

      //Entry was not on Transifex, check if it has a new translation at upstream
      else if (bisInUPS)
      {
        T_itPOData itPOUPS = GetUPSItFoundEntry();
        m_mapMRG[sLCode].AddItEntry(itPOUPS);
        m_mapUPD[sLCode].AddItEntry(itPOUPS);
      }
    }

    //Pass Resource data to the newly created MRG PO classes for later use (PO file creation)
    if (m_mapMRG.find(sLCode) != m_mapMRG.end())
    {
      CPOHandler& MRGPOHandler = m_mapMRG.at(sLCode);
      MRGPOHandler.SetReasData(m_ResData);
      MRGPOHandler.SetIfIsSourceLang(sLCode == m_ResData.sSRCLCode);
      MRGPOHandler.SetPOType(MERGEDPO);
      MRGPOHandler.CreateHeader(m_AddonXMLHandler.GetResHeaderPretext(), sLCode);
      if (m_ResData.bIsLangAddon)
        MRGPOHandler.SetLangAddonXMLString(m_mapUPS[sLCode].GetLangAddonXMLString());

      if (bPOChangedFromUPS || m_ResData.bMajorBump)
      {
        if (m_ResData.bIsLangAddon && sLCode != m_ResData.sSRCLCode)
          MRGPOHandler.BumpLangAddonXMLVersion(m_ResData.bMajorBump);
        m_bResChangedFromUPS = true;
        m_lChangedLangsFromUPS.insert(sLCode);
      }
    }

    //Pass Resource data to the newly created UPD PO classes for later use (PO file creation)
    if (m_mapUPD.find(sLCode) != m_mapUPD.end())
    {
      CPOHandler& UPDPOHandler = m_mapUPD.at(sLCode);
      UPDPOHandler.SetReasData(m_ResData);
      UPDPOHandler.SetIfIsSourceLang(sLCode == m_ResData.sSRCLCode);
      UPDPOHandler.SetPOType(UPDATEPO);
      UPDPOHandler.CreateHeader(m_AddonXMLHandler.GetResHeaderPretext(), sLCode);

      m_lLangsToUPD.insert(sLCode);
    }
  }

  //If resource has been changed in any language, bump the language addon version
  if (m_bResChangedFromUPS && !m_ResData.bIsLangAddon)
    m_AddonXMLHandler.SetBumpAddonVersion();

  return;
}

// Check if there is a POHandler existing in mapUPS for language code sLCode.
// If so, store it as last found POHandler iterator and store last sLCode.
// If a POHandler exist, try to find a PO entry in it.
bool CResourceHandler::FindUPSEntry(const std::string sLCode, CPOEntry &EntryToFind)
{
  if (m_lastUPSLCode == sLCode)
  {
    if (!m_bLastUPSHandlerFound)
      return false;
    else
      return m_lastUPSIterator->second.FindEntry(EntryToFind);
  }

  m_lastUPSLCode = sLCode;
  m_lastUPSIterator = m_mapUPS.find(sLCode);
  if (m_lastUPSIterator == m_mapUPS.end())
  {
    m_bLastUPSHandlerFound = false;
    return false;
  }
  else
  {
    m_bLastUPSHandlerFound = true;
    return m_lastUPSIterator->second.FindEntry(EntryToFind);
  }
}

// Check if there is a POHandler existing in mapTRX for language code sLCode.
// If so, store it as last found POHandler iterator and store last sLCode.
// If a POHandler exist, try to find a PO entry in it.
bool CResourceHandler::FindTRXEntry(const std::string sLCode, CPOEntry &EntryToFind)
{
  if (m_lastTRXLCode == sLCode)
  {
    if (!m_bLastTRXHandlerFound)
      return false;
    else
      return m_lastTRXIterator->second.FindEntry(EntryToFind);
  }

  m_lastTRXLCode = sLCode;
  m_lastTRXIterator = m_mapTRX.find(sLCode);
  if (m_lastTRXIterator == m_mapTRX.end())
  {
    m_bLastTRXHandlerFound = false;
    return false;
  }
  else
  {
    m_bLastTRXHandlerFound = true;
    return m_lastTRXIterator->second.FindEntry(EntryToFind);
  }
}

// Read iterator to the last found entry stored by function FindUPSEntry.
T_itPOData CResourceHandler::GetUPSItFoundEntry()
{
  return m_lastUPSIterator->second.GetItFoundEntry();
}

// Read iterator to the last found entry stored by function FindTRXEntry.
T_itPOData CResourceHandler::GetTRXItFoundEntry()
{
  return m_lastTRXIterator->second.GetItFoundEntry();
}

void CResourceHandler::WriteMergedPOFiles(const std::string& sAddonXMLPath, const std::string& sLangAddonXMLPath, const std::string& sChangeLogPath, const std::string& sLangPath)
{
  if (!m_ResData.bIsLangAddon)
  {
    m_AddonXMLHandler.WriteAddonXMLFile(sAddonXMLPath);
    if (!m_ResData.sChgLogFormat.empty())
      m_AddonXMLHandler.WriteAddonChangelogFile(sChangeLogPath);
  }

  if (m_ResData.bHasOnlyAddonXML)
    return;


  for (T_itmapPOFiles itmapPOFiles = m_mapMRG.begin(); itmapPOFiles != m_mapMRG.end(); itmapPOFiles++)
  {
    const std::string& sLCode = itmapPOFiles->first;
    std::string strPODir, strAddonDir;
    strPODir = g_CharsetUtils.ReplaceLanginURL(sLangPath, g_CharsetUtils.GetLFormFromPath(m_ResData.LOC.LPath), sLCode);

    CPOHandler& POHandler = m_mapMRG.at(sLCode);

    POHandler.WritePOFile(strPODir);

    // Write individual addon.xml files for language-addons
    if (m_ResData.bIsLangAddon)
    {
      strAddonDir = g_CharsetUtils.ReplaceLanginURL(sLangAddonXMLPath, g_CharsetUtils.GetLFormFromPath(m_ResData.LOC.LPath), sLCode);
      POHandler.WriteLangAddonXML(strAddonDir);
    }
  }
 return;
}

void CResourceHandler::WriteLOCPOFiles(CCommitData& CommitData, CCommitData& CommitDataSRC)
{
    std::string sLOCGITDir = m_ResData.sUPSLocalPath + m_ResData.LOC.Owner + "/" + m_ResData.LOC.Repo + "/" + m_ResData.LOC.Branch + "/";
    std::string sLOCSRCGITDir = m_ResData.sUPSLocalPath + m_ResData.LOCSRC.Owner + "/" + m_ResData.LOCSRC.Repo + "/" + m_ResData.LOCSRC.Branch + "/";

    std::string sAddonXMLPath = sLOCGITDir + m_ResData.LOC.AXMLPath;
    std::string sChangeLogPath =  sLOCGITDir + m_ResData.LOC.ChLogPath;
    std::string sLangPath  = sLOCGITDir + m_ResData.LOC.LPath;

    std::string sLangPathSRC  = sLOCSRCGITDir + m_ResData.LOCSRC.LPath;
    std::string sLangAddonXMLPathSRC = sLOCSRCGITDir + m_ResData.LOCSRC.AXMLPath;


  if (!m_ResData.bIsLangAddon)
  {
    m_AddonXMLHandler.WriteAddonXMLFile(sAddonXMLPath);
    if (!m_ResData.sChgLogFormat.empty())
      m_AddonXMLHandler.WriteAddonChangelogFile(sChangeLogPath);
  }

  if (!m_ResData.bHasOnlyAddonXML)
  {
    for (T_itmapPOFiles itmapPOFiles = m_mapMRG.begin(); itmapPOFiles != m_mapMRG.end(); itmapPOFiles++)
    {
      const std::string& sLCode = itmapPOFiles->first;
      std::string strPODir, strAddonDir;

      CPOHandler& POHandler = m_mapMRG.at(sLCode);

      if (sLCode != m_ResData.sSRCLCode || !m_ResData.bIsLangAddon)
      {
        strPODir = g_CharsetUtils.ReplaceLanginURL(sLangPath, g_CharsetUtils.GetLFormFromPath(m_ResData.LOC.LPath), sLCode);
        if (sLCode == m_ResData.sSRCLCode && !POHandler.CheckIfPOIsSameAsTheOverwritten(strPODir))
        {
          CResChangeData ResChangeData;
          ResChangeData.sResName = m_ResData.sResName;
          ResChangeData.sGitCommitTextSRC = m_ResData.sGitCommitTextSRC;
          ResChangeData.sLangPath = g_CharsetUtils.ReplaceLanginURL(m_ResData.LOC.LPath, g_CharsetUtils.GetLFormFromPath(m_ResData.LOC.LPath), sLCode);
          ResChangeData.sLOCGITDir = sLOCGITDir;
          CommitDataSRC.listResWithSRCChange.push_front(ResChangeData); // check if the SRC file differs from the one to be overwritten and note it
        }
      }
      else
      {
        strPODir = g_CharsetUtils.ReplaceLanginURL(sLangPathSRC, g_CharsetUtils.GetLFormFromPath(m_ResData.LOCSRC.LPath), sLCode);
        if (!POHandler.CheckIfPOIsSameAsTheOverwritten(strPODir))
        {
          CResChangeData ResChangeData;
          ResChangeData.sResName = m_ResData.sResName;
          ResChangeData.sGitCommitTextSRC = m_ResData.sGitCommitTextSRC;
          ResChangeData.sLangPath = g_CharsetUtils.ReplaceLanginURL(m_ResData.LOCSRC.LPath, g_CharsetUtils.GetLFormFromPath(m_ResData.LOCSRC.LPath), sLCode);
          ResChangeData.sLOCGITDir = sLOCSRCGITDir;
          CommitDataSRC.listResWithSRCChange.push_front(ResChangeData); // check if the SRC file differs from the one to be overwritten and note it
        }
      }


      POHandler.WritePOFile(strPODir);

      // Write individual addon.xml files for language-addons
      if (m_ResData.bIsLangAddon)
      {
        if (sLCode != m_ResData.sSRCLCode)
          strAddonDir = g_CharsetUtils.ReplaceLanginURL(sAddonXMLPath, g_CharsetUtils.GetLFormFromPath(m_ResData.LOC.LPath), sLCode);
        else
          strAddonDir = g_CharsetUtils.ReplaceLanginURL(sLangAddonXMLPathSRC, g_CharsetUtils.GetLFormFromPath(m_ResData.LOC.LPath), sLCode);

        POHandler.WriteLangAddonXML(strAddonDir);
      }
    }
  }

  if (m_bResChangedFromUPS)
  {
    CResChangeData ResChangeData;
    ResChangeData.sResName = m_ResData.sResName;
    ResChangeData.sLOCGITDir = sLOCGITDir;

    CommitData.listResWithChange.push_front(ResChangeData);
  }

  //GIT commit changes to SRC language for all resources which changed
  std::string sCommand, sPOPathSRC, sGitDir;

  bool bHadChangedSRCChange = (!m_ResData.bIsLangAddon && !CommitDataSRC.listResWithSRCChange.empty());
  bool bHadChangedSRCChangeInLangAddon = (m_ResData.bIsLangAddon && !CommitDataSRC.listResWithSRCChange.empty());

  if (!m_ResData.sGitCommitText.empty() && (bHadChangedSRCChange || bHadChangedSRCChangeInLangAddon))
  {
    CLog::Log(logPRINT, "\n");

    CGITData CurrGITData;

    if (m_ResData.bIsLangAddon)
    {
      sPOPathSRC = g_CharsetUtils.ReplaceLanginURL(sLangPathSRC, g_CharsetUtils.GetLFormFromPath(m_ResData.LOCSRC.LPath), m_ResData.sSRCLCode);
      sGitDir = sLOCSRCGITDir;
      CurrGITData = m_ResData.LOCSRC;
    }
    else
    {
      sPOPathSRC = g_CharsetUtils.ReplaceLanginURL(sLangPath, g_CharsetUtils.GetLFormFromPath(m_ResData.LOC.LPath), m_ResData.sSRCLCode);
      sGitDir = sLOCGITDir;
      CurrGITData = m_ResData.LOC;
    }

    //Fill in the commitdata for later use at git push time
    CommitDataSRC.sCommitMessage = m_ResData.sGitCommitTextSRC;
    if (m_ResData.m_pMapGitRepos->find(CurrGITData.Owner + "/" + CurrGITData.Repo + "/" + CurrGITData.Branch) == m_ResData.m_pMapGitRepos->end())
      CLog::Log(logERROR, "CResourceHandler::WriteLOCPOFiles: internal error: gitdata does not exist in map.");
    m_ResData.m_pMapGitRepos->at(CurrGITData.Owner + "/" + CurrGITData.Repo + "/" + CurrGITData.Branch).listCommitData.push_front(CommitDataSRC);
    m_ResData.m_pMapGitRepos->at(CurrGITData.Owner + "/" + CurrGITData.Repo + "/" + CurrGITData.Branch).bHasBeenAnSRCFileChange = true;

//    std::string sRepoKey = CurrGITData.Owner + "/" + CurrGITData.Repo + "/" + CurrGITData.Branch;


    for (std::list<CResChangeData>::iterator itChange = CommitDataSRC.listResWithSRCChange.begin(); itChange != CommitDataSRC.listResWithSRCChange.end(); itChange++)
    {
      sCommand = "cd " + itChange->sLOCGITDir + ";";
      sCommand += "git add " + itChange->sLangPath;
      CLog::Log(logPRINT, "%sGIT add %sSRC file%s with the following command:%s\n%s%s%s\n",KMAG, KRED, KMAG, RESET, KORNG, sCommand.c_str(), RESET);
      g_File.SytemCommand(sCommand);

      sCommand = "cd " + itChange->sLOCGITDir + ";";
      sCommand += "git commit -m \"" + itChange->sGitCommitTextSRC + "\"";
      CLog::Log(logPRINT, "%sGIT commit SRC file with the following command:%s\n%s%s%s\n",KMAG, RESET, KORNG, sCommand.c_str(), RESET);
      g_File.SytemCommand(sCommand);
    }

    CommitDataSRC.listResWithChange.clear(); CommitDataSRC.listResWithSRCChange.clear(); CommitDataSRC.sCommitMessage.clear();
  }


  //GIT commit changes to non-SRC language for all resources which changed
  if (!m_ResData.sGitCommitText.empty() && !CommitData.listResWithChange.empty())
  {
    CLog::Log(logPRINT, "\n");

    sGitDir = sLOCGITDir;

    sCommand = "cd " + sGitDir + ";";
    sCommand += "git add -A;";
    sCommand += "git commit -am \"" + m_ResData.sGitCommitText + "\"";
    CLog::Log(logPRINT, "%sGIT commit with the following command:%s\n%s%s%s\n",KMAG, RESET, KORNG, sCommand.c_str(), RESET);
    g_File.SytemCommand(sCommand);

    //Fill in the commitdata for later use at git push time
    CGITData CurrGITData = m_ResData.LOC;
    CommitData.sCommitMessage = m_ResData.sGitCommitText;
    if (m_ResData.m_pMapGitRepos->find(CurrGITData.Owner + "/" + CurrGITData.Repo + "/" + CurrGITData.Branch) == m_ResData.m_pMapGitRepos->end())
      CLog::Log(logERROR, "CResourceHandler::WriteLOCPOFiles: internal error: gitdata does not exist in map.");

    m_ResData.m_pMapGitRepos->at(CurrGITData.Owner + "/" + CurrGITData.Repo + "/" + CurrGITData.Branch).listCommitData.push_front(CommitData);

    CommitData.listResWithChange.clear(); CommitData.listResWithSRCChange.clear(); CommitData.sCommitMessage.clear();
  }

 return;
}


void CResourceHandler::WriteUpdatePOFiles(const std::string& strPath)
{
  for (T_itmapPOFiles itmapPOFiles = m_mapUPD.begin(); itmapPOFiles != m_mapUPD.end(); itmapPOFiles++)
  {
    const std::string& sLCode = itmapPOFiles->first;
    std::string strPODir;
    strPODir = g_CharsetUtils.ReplaceLanginURL(strPath, m_ResData.sBaseLForm, sLCode);

    CPOHandler& POHandler = m_mapUPD.at(sLCode);

    POHandler.WritePOFile(strPODir);
  }
  return;
}

void CResourceHandler::GenerateMergedPOFiles()
{

  CLog::Log(logPRINT, "%s", RESET);

  if (!m_ResData.bIsLangAddon)
    m_AddonXMLHandler.ClearAllAddonXMLEntries();


  CLog::Log(logPRINT, "Generating merged and update PO files: %s%s%s\n", KMAG, m_ResData.sResName.c_str(), RESET);
  if (!m_lChangedLangsFromUPS.empty())
  {
    CLog::Log(logPRINT, "  Changed Langs in strings files from upstream: ");
    PrintChangedLangs(m_lChangedLangsFromUPS);
    CLog::Log(logPRINT, "\n");
  }

  for (T_itmapPOFiles itmapPOFiles = m_mapMRG.begin(); itmapPOFiles != m_mapMRG.end(); itmapPOFiles++)
  {
    const std::string& sLCode = itmapPOFiles->first;

    CPOHandler& POHandler = m_mapMRG.at(sLCode);

    POHandler.GeneratePOFile();

    if (!m_ResData.bIsLangAddon)
      m_AddonXMLHandler.SetAddonXMLEntry(POHandler.GetAddonXMLEntry(), sLCode);
  }

  if (!m_ResData.bIsLangAddon)
  {
    m_AddonXMLHandler.GenerateAddonXMLFile();
    m_AddonXMLHandler.GenerateChangelogFile(m_ResData.sChgLogFormat);
  }

  return;
}

void CResourceHandler::GenerateUpdatePOFiles()
{
  if (!m_lLangsToUPD.empty())
  {
    CLog::Log(logPRINT, "%s  Langs to update%s to Transifex from upstream: ", KRED, RESET);
    PrintChangedLangs(m_lLangsToUPD);
    CLog::Log(logPRINT, "\n");
  }

  for (T_itmapPOFiles itmapPOFiles = m_mapUPD.begin(); itmapPOFiles != m_mapUPD.end(); itmapPOFiles++)
  {
    const std::string& sLCode = itmapPOFiles->first;
    CPOHandler& POHandler = m_mapUPD.at(sLCode);

    POHandler.GeneratePOFile();
  }
  return;
}

std::list<std::string> CResourceHandler::ParseAvailLanguagesTX(std::string strJSON, const std::string &strURL)
{
  Json::Value root;   // will contains the root value after parsing.
  Json::Reader reader;
  std::string LCode;
  std::list<std::string> listLangs;

  bool parsingSuccessful = reader.parse(strJSON, root );
  if ( !parsingSuccessful )
  {
    CLog::Log(logERROR, "CJSONHandler::ParseAvailLanguagesTX: no valid JSON data");
    return listLangs;
  }

  const Json::Value langs = root;
  std::string strLangsToFetch;
  std::string strLangsToDrop;

  for(Json::ValueIterator itr = langs.begin() ; itr != langs.end() ; itr++)
  {
    LCode = itr.key().asString();
    if (LCode == "unknown")
      CLog::Log(logERROR, "JSONHandler: ParseLangs: no language code in json data. json string:\n %s", strJSON.c_str());

    LCode = g_LCodeHandler.VerifyLangCode(LCode, m_ResData.TRX.LForm);

    if (LCode == "")
      continue;

    Json::Value valu = *itr;
    std::string strCompletedPerc = valu.get("completed", "unknown").asString();
    std::string strModTime = valu.get("last_update", "unknown").asString();

    // we only add language codes to the list which has a minimum ready percentage defined in the xml file
    // we make an exception with all English derived languages, as they can have only a few srings changed
    if (LCode.find("en_") != std::string::npos || strtol(&strCompletedPerc[0], NULL, 10) > m_ResData.iMinComplPercent-1)
    {
      strLangsToFetch += LCode + ": " + strCompletedPerc + ", ";
      listLangs.push_back(LCode);
      g_Fileversion.SetVersionForURL(strURL + "translation/" + g_LCodeHandler.GetLangFromLCode(LCode, m_ResData.TRX.LForm) + "/?file", strModTime);
    }
    else
      strLangsToDrop += LCode + ": " + strCompletedPerc + ", ";
  };

  return listLangs;
};

std::set<std::string> CResourceHandler::GetAvailLangsGITHUB()
{
  std::string sFileList = g_HTTPHandler.GetGitFileListToSTR(m_ResData.sUPSLocalPath, m_ResData.UPS, m_ResData.bForceGitDloadToCache);

  size_t posLF = 0;
  size_t posNextLF = 0;

  std::string sAXMLLForm = g_CharsetUtils.GetLFormFromPath(m_ResData.UPS.AXMLPath);
  std::string sLForm = g_CharsetUtils.GetLFormFromPath(m_ResData.UPS.LPath);

  std::string sVersion;
  std::set<std::string> listLangs;

  while ((posNextLF = sFileList.find('\n', posLF)) != std::string::npos)
  {
    std::string sLine = sFileList.substr(posLF +1, posNextLF - posLF-1);
    size_t posWS1, posWS2, posWS3;
    posWS1 = sLine.find(' ');
    posWS2 = sLine.find(' ', posWS1+1);
    posWS3 = sLine.find('\t', posWS2+1);

    if (posWS1 == std::string::npos || posWS2 == std::string::npos || posWS3 == std::string::npos)
      CLog::Log(logERROR, "ResHandler::GetAvailLangsGITHUB: Wrong file list format for local github clone filelist, for resource %s", m_ResData.sResName.c_str());

    std::string sSHA = sLine.substr(posWS1 +1, posWS2 - posWS1);
    std::string sReadPath = sLine.substr(posWS3 + 1);

    posLF = posNextLF + 1;

    if (!m_ResData.bHasOnlyAddonXML)
    {
      //Get version of strings.po files
      std::string sMatchedLangalias = g_CharsetUtils.GetLangnameFromPath(sReadPath,  m_ResData.UPS.LPath, sLForm);
      std::string sFoundLangCode = g_LCodeHandler.GetLangCodeFromAlias(sMatchedLangalias, sLForm);

      if (sFoundLangCode != "")
      {
        listLangs.insert(sFoundLangCode);
        CGITData GitData = m_ResData.UPS;
        g_CharsetUtils.replaceAllStrParts(&GitData.LPath, sLForm, sMatchedLangalias);
        g_Fileversion.SetVersionForURL("git://" + GitData.Owner + "/" + GitData.Repo + "/" + GitData.Branch + "/" + GitData.LPath, sSHA);
        continue;
      }
    }
    if (m_ResData.bIsLangAddon)
    {
      //If we have a language addon, we also check if there is any language name dependent addon.xml file
      std::string sMatchedLangalias = g_CharsetUtils.GetLangnameFromPath(sReadPath, m_ResData.UPS.AXMLPath, sAXMLLForm);
      std::string sFoundLangCode = g_LCodeHandler.GetLangCodeFromAlias(sMatchedLangalias, sAXMLLForm);
      if (sFoundLangCode != "")
      {
        listLangs.insert(sFoundLangCode);
        CGITData GitData = m_ResData.UPS;
        g_CharsetUtils.replaceAllStrParts(&GitData.AXMLPath, sAXMLLForm, sMatchedLangalias);
        g_Fileversion.SetVersionForURL("git://" + GitData.Owner + "/" + GitData.Repo + "/" + GitData.Branch + "/" + GitData.AXMLPath, sSHA);
      }
    }
    else if (sReadPath == m_ResData.UPS.AXMLPath)
    {
      //Get version for addon.xml file
      CGITData GitData = m_ResData.UPS;
      g_Fileversion.SetVersionForURL("git://" + GitData.Owner + "/" + GitData.Repo + "/" + GitData.Branch + "/" + GitData.AXMLPath, sSHA);
    }
    else if (sReadPath == m_ResData.UPS.ChLogPath)
    {
      //Get version for changelog.txt file
      CGITData GitData = m_ResData.UPS;
      g_Fileversion.SetVersionForURL("git://" + GitData.Owner + "/" + GitData.Repo + "/" + GitData.Branch + "/" + GitData.ChLogPath, sSHA);
    }
  }

  return listLangs;
};

void CResourceHandler::GetSRCFilesGitData()
{
  std::string sFileList = g_HTTPHandler.GetGitFileListToSTR (m_ResData.sUPSLocalPath, m_ResData.UPSSRC, m_ResData.bForceGitDloadToCache);

  size_t posLF = 0;
  size_t posNextLF = 0;

  std::string sVersion;

  std::string sLPathSRC = g_CharsetUtils.ReplaceLanginURL(m_ResData.UPSSRC.LPath, g_CharsetUtils.GetLFormFromPath(m_ResData.UPSSRC.LPath), m_ResData.sSRCLCode);
  std::string sLAXMLPathSRC = g_CharsetUtils.ReplaceLanginURL(m_ResData.UPSSRC.AXMLPath, g_CharsetUtils.GetLFormFromPath(m_ResData.UPSSRC.AXMLPath), m_ResData.sSRCLCode);

  while ((posNextLF = sFileList.find('\n', posLF)) != std::string::npos)
  {
    std::string sLine = sFileList.substr(posLF +1, posNextLF - posLF-1);
    size_t posWS1, posWS2, posWS3;
    posWS1 = sLine.find(' ');
    posWS2 = sLine.find(' ', posWS1+1);
    posWS3 = sLine.find('\t', posWS2+1);

    if (posWS1 == std::string::npos || posWS2 == std::string::npos || posWS3 == std::string::npos)
      CLog::Log(logERROR, "ResHandler::GetAvailLangsGITHUB: Wrong file list format for local github clone filelist, for resource %s", m_ResData.sResName.c_str());

    std::string sSHA = sLine.substr(posWS1 +1, posWS2 - posWS1);
    std::string sReadPath = sLine.substr(posWS3 + 1);

    posLF = posNextLF + 1;

    if (sReadPath == sLPathSRC)
    {
      //Set version for SRC strings.po
      CGITData GitData = m_ResData.UPSSRC;
      g_Fileversion.SetVersionForURL("git://" + GitData.Owner + "/" + GitData.Repo + "/" + GitData.Branch + "/" + sLPathSRC, sSHA);
    }
    else if (sReadPath == sLAXMLPathSRC)
    {
      //Set version for SRC language addon.xml
      CGITData GitData = m_ResData.UPSSRC;
      g_Fileversion.SetVersionForURL("git://" + GitData.Owner + "/" + GitData.Repo + "/" + GitData.Branch + "/" + sLAXMLPathSRC, sSHA);
    }
  }
}

std::list<std::string> CResourceHandler::CreateMergedLangList()
{
  std::list<std::string> listMergedLangs;

  for (T_itmapPOFiles it = m_mapUPS.begin(); it != m_mapUPS.end(); it++)
  {
    const std::string& sLCode = it->first;

    if (std::find(listMergedLangs.begin(), listMergedLangs.end(), sLCode) == listMergedLangs.end())
      listMergedLangs.push_back(sLCode);
  }

  for (T_itmapPOFiles it = m_mapTRX.begin(); it != m_mapTRX.end(); it++)
  {
    const std::string& sLCode = it->first;

    if (std::find(listMergedLangs.begin(), listMergedLangs.end(), sLCode) == listMergedLangs.end())
      listMergedLangs.push_back(sLCode);
  }

  return listMergedLangs;
}

//TODO use std::set instead of list everywhere
void CResourceHandler::PrintChangedLangs(const std::set<std::string>& lChangedLangs)
{
  std::set<std::string>::iterator itLangs;
  std::size_t counter = 0;
  CLog::Log(logPRINT, "%s", KCYN);
  for (itLangs = lChangedLangs.begin() ; itLangs != lChangedLangs.end(); itLangs++)
  {
    CLog::Log(logPRINT, "%s ", itLangs->c_str());
    counter++;
    if (counter > 10)
    {
      CLog::Log(logPRINT, "+ %i langs ", (int)lChangedLangs.size() - 10);
      break;
    }
  }
  CLog::Log(logPRINT, "%s", RESET);
}

void CResourceHandler::UploadResourceToTransifex(bool bNewResourceOnTRX)
{
  g_HTTPHandler.Cleanup();
  g_HTTPHandler.ReInit();

  CLog::Log(logPRINT, "Uploading files for resource: %s%s%s", KMAG, m_ResData.sResName.c_str(), RESET);


  if (m_mapUPD.empty()) // No update needed for the specific resource (not even an English one)
  {
    CLog::Log(logPRINT, ", no upload was necesarry.\n");
    return;
  }

  g_HTTPHandler.SetLocation("UPD");
  g_HTTPHandler.SetProjectName(m_ResData.UPD.ProjectName);
  g_HTTPHandler.SetResName(m_ResData.sResName);
  g_HTTPHandler.SetFileName("string.po");
  g_HTTPHandler.SetDataFile(false);

  g_HTTPHandler.SetLCode(m_ResData.sSRCLCode);

  if (bNewResourceOnTRX)
  {
    // We create the new resource on transifex and also upload the English source file at once

    m_mapUPD.at(m_ResData.sSRCLCode).CreateNewResource();

    g_HTTPHandler.Cleanup();
    g_HTTPHandler.ReInit();
  }

  CLog::Log(logPRINT, "\n");

  // Upload the source file in case there is one to update
  if (m_mapUPD.find(m_ResData.sSRCLCode) != m_mapUPD.end() && !bNewResourceOnTRX)
    m_mapUPD.at(m_ResData.sSRCLCode).PutSRCFileToTRX();


  for (T_itmapPOFiles it = m_mapUPD.begin(); it!=m_mapUPD.end(); it++)
  {
    const std::string& sLCode = it->first;
    CPOHandler& POHandler = it->second;

    if (sLCode == m_ResData.sSRCLCode) // Let's not upload the Source language file again
      continue;

    g_HTTPHandler.SetLCode(sLCode);

    POHandler.PutTranslFileToTRX();
  }
}
