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

#include "ProjectHandler.h"
#include <list>
#include "HTTPUtils.h"
#include "jsoncpp/json/json.h"
#include <algorithm>
#include "ConfigHandler.h"
#include "Log.h"
#include "CharsetUtils.h"
#include "FileUtils.h"
#include "Langcodes.h"
#include <iostream>

CProjectHandler::CProjectHandler()
{};

CProjectHandler::~CProjectHandler()
{};

void CProjectHandler::LoadConfigToMem(bool bForceUseCache)
{
  CConfigHandler ConfigHandler;
  ConfigHandler.LoadResDataToMem(m_strProjDir, m_mapResData, &m_MapGitRepos, m_mapResOrder);

  size_t iCacheExpire = -1; //in case we are in force cache use, we set cache expiration to the highest possible value
  if (!bForceUseCache)
    iCacheExpire = m_mapResData.begin()->second.iCacheExpire;

  g_HTTPHandler.SetHTTPCacheExpire(iCacheExpire);
  m_BForceUseCache = bForceUseCache;
}

bool CProjectHandler::FetchResourcesFromTransifex()
{

  g_HTTPHandler.Cleanup();
  g_HTTPHandler.ReInit();
  CLog::Log(logPRINT, "TXresourcelist");

  //TODO multiple projects
  const std::string& sProjectName = m_mapResData.begin()->second.TRX.ProjectName;
  g_HTTPHandler.SetLocation("TRX");
  g_HTTPHandler.SetProjectName(sProjectName);
  g_HTTPHandler.SetResName("");
  g_HTTPHandler.SetLCode("");
  g_HTTPHandler.SetFileName("TXResourceList.json");
  g_HTTPHandler.SetDataFile(true);

  std::string strtemp = g_HTTPHandler.GetURLToSTR("https://www.transifex.com/api/2/project/" + sProjectName + "/resources/");
  if (strtemp.empty())
    CLog::Log(logERROR, "ProjectHandler::FetchResourcesFromTransifex: error getting resources from transifex.net");

  CLog::Log(logPRINT, "\n\n");

  std::set<std::string> listResNamesAvailOnTX = ParseResources(strtemp);
//TODO collect out txprojectnames, check all resources in them, if we need to download
  for (T_itResData it = m_mapResData.begin(); it != m_mapResData.end(); it++)
  {
    const std::string& sResName = it->second.sResName;

    CLog::Log(logPRINT, "%s%s%s (", KMAG, sResName.c_str(), RESET);

    if (listResNamesAvailOnTX.find(sResName) == listResNamesAvailOnTX.end())
    {
      CLog::Log(logPRINT, " ) Not yet available at Transifex\n");
      continue;
    }

    const CResData& ResData = m_mapResData[sResName];
    CResourceHandler NewResHandler(ResData);

    if (!m_BForceUseCache)
      g_HTTPHandler.SetHTTPCacheExpire(ResData.iCacheExpire);

    m_mapResources[sResName] = NewResHandler;
    m_mapResources[sResName].FetchPOFilesTXToMem();
    CLog::Log(logPRINT, " )\n");
  }
  return true;
};

void CProjectHandler::GITPushLOCGitRepos()
{
  unsigned int iCounter = 0;
  std::map<unsigned int, CBasicGITData> MapReposToPush;
  for (std::map<std::string, CBasicGITData>::iterator it = m_MapGitRepos.begin(); it != m_MapGitRepos.end(); it++)
  {
    iCounter++;
    CBasicGITData GitData = it->second;
    MapReposToPush[iCounter] = GitData;
  }

  bool bFirstRun = true;
  std::string strInput;
  CLog::Log(logPRINT, "\n");

  do
  {
    if (!bFirstRun)
    {
      //Jump back to the start of list with the cursor
      int iLines = MapReposToPush.size() +4;
      std::string sNumOfLines = g_CharsetUtils.IntToStr(iLines);
      std::string sEscapeCode = "\033[" +sNumOfLines + "A";
      CLog::Log(logPRINT, "%s", sEscapeCode.c_str());
    }

    std::set<int> listReposToIncludeInLists;
    //Print current state
    for (std::map<unsigned int, CBasicGITData>::iterator it = MapReposToPush.begin(); it != MapReposToPush.end(); it++)
    {
      CBasicGITData GitData = it->second;
      std::list<CCommitData>& listCommitData = GitData.listCommitData;
      size_t iNumOfCommits = listCommitData.size();
      bool bRepoHasCommit = iNumOfCommits != 0;

      size_t iLastPushAge = g_HTTPHandler.GetLastGitPushAge(GitData.Owner, GitData.Repo, GitData.Branch);
      bool bGitPush = ((GitData.iGitPushInterval * 24 * 60 * 60) < iLastPushAge);
      bGitPush = bGitPush||GitData.bForceGitPush;
      bGitPush = bGitPush&&!GitData.bSkipGitPush;
      bGitPush = bGitPush && bRepoHasCommit;

      float fLastPushAge = (float)iLastPushAge / (float)86400;
      unsigned int iLastPushAgeDays = (unsigned int)fLastPushAge;

      if (iLastPushAgeDays > 100)
        iLastPushAgeDays = 99;

      if (bGitPush)
        listReposToIncludeInLists.insert(it->first);

      CLog::Log(logPRINT, "%s%i|%s%s%s|%s%i /%s%i|%s|%s|%s%s%s\n",
                (it->first < 10)?" ":"", it->first,
                bGitPush?KRED:KLGRAY, bGitPush?"*":" ", RESET,
                (iLastPushAgeDays < 10)?" ":"", iLastPushAgeDays,
                (GitData.iGitPushInterval < 10)?" ":"", GitData.iGitPushInterval,
                (GitData.bForceGitPush||GitData.bSkipGitPush)?(GitData.bForceGitPush?"Force":" Skip"):"     ",
                GitData.bHasBeenAnSRCFileChange?"SRC":"   ",
                bRepoHasCommit?KMAG:KGRAY, (GitData.Owner + "/" + GitData.Repo + "/" + GitData.Branch).c_str(), RESET);
    }

    CLog::Log(logPRINT, "\n\n\033[2A");
    CLog::Log(logPRINT, "\n\n\033[2A");
    CLog::Log(logPRINT, "\n\n\033[2A");
    CLog::Log(logPRINT, "\n\n\033[2A");

    CLog::Log(logPRINT, "\n%sChoose option:%s\n", KRED, RESET);
    CLog::Log(logPRINT, "%sfp%s: force push (eg. fp 3-6,8), %ssp%s: skip push (eg. sp3,8-9), %sdr%s: dry run push\n", KMAG, RESET, KMAG, RESET, KMAG, RESET);
    CLog::Log(logPRINT, "%spush%s:Continue with pushing to Github %sdf%s:Check diff lists by repo (you can also use filter here) %ss%s:Skip. Your Choice:                           \b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b", KMAG, RESET, KMAG, RESET, KRED, RESET);
    cin >> strInput;

    if (strInput.find("fp") == 0 || strInput.find("sp") == 0)
    {
      // We have a forcepush or skippush request
      bool bForceGitPush = strInput.find("fp") ==0;
      bool bSkipGitPush = !bForceGitPush;

      std::set<int> ListRepos;

      if (!ParseRepoList(strInput, ListRepos))
        CLog::Log(logERROR, "Unable to parse command, with wrong syntax: %s", strInput.c_str());

      for (std::set<int>::iterator it = ListRepos.begin(); it!= ListRepos.end(); it++)
      {
        if (MapReposToPush.find(*it) == MapReposToPush.end())
          CLog::Log(logERROR, "Wrong projectnumber given in command %s, wrong number: %i", strInput.c_str(), *it);

        MapReposToPush[*it].bForceGitPush = bForceGitPush;
        MapReposToPush[*it].bSkipGitPush = bSkipGitPush;
      }
    }

    else if (strInput.find("df") == 0)
    {
      std::set<int> ListRepos = listReposToIncludeInLists;

      if (strInput != "df")
      {
        ListRepos.clear();
        if (!ParseRepoList(strInput, ListRepos))
          CLog::Log(logERROR, "Unable to parse command, with wrong syntax: %s", strInput.c_str());
      }

      //Clean cache Directory
      std::string sCachePath = g_File.GetHomePath();
      sCachePath += "/.cache/kodi-txupdate";
      g_File.DeleteDirectory(sCachePath);
      g_File.MakeDir(sCachePath);

      GenerateDiffListsPerRepo(sCachePath + DirSepChar + "1_Separate diff Lists", ListRepos);
      GenerateCombinedDiffLists(sCachePath + DirSepChar + "2_Combined diff Lists", ListRepos);
      GenerateCombinedDiffListsSRC(sCachePath + DirSepChar + "3_Combined diff Lists (SRC)", ListRepos);
      std::string sCommand = "vi " + sCachePath;
      g_File.SytemCommand(sCommand);
    }

    if (strInput == "s")
      return;

    bFirstRun = false;
  }
  while (strInput != "push" && strInput != "dr");

  bool bDryRun = strInput == "dr";

  //Make the actual push
  for (std::map<unsigned int, CBasicGITData>::iterator it = MapReposToPush.begin(); it != MapReposToPush.end(); it++)
  {
    CBasicGITData GitData = it->second;

    std::list<CCommitData>& listCommitData = GitData.listCommitData;
    size_t iNumOfCommits = listCommitData.size();
    bool bRepoHasCommit = iNumOfCommits != 0;

    size_t iLastPushAge = g_HTTPHandler.GetLastGitPushAge(GitData.Owner, GitData.Repo, GitData.Branch);
    bool bGitPush = ((GitData.iGitPushInterval * 24 * 60 * 60) < iLastPushAge);
    bGitPush = bGitPush||GitData.bForceGitPush;
    bGitPush = bGitPush&&!GitData.bSkipGitPush;
    bGitPush = bGitPush && bRepoHasCommit;

    if (bGitPush)
    {
      if (!bDryRun)
        g_HTTPHandler.SetGitPushTime(GitData.Owner, GitData.Repo, GitData.Branch);

      CLog::Log(logPRINT, "\nPushing%s: %s%s%s\n", bDryRun?"(dry run)":"", KMAG, (GitData.Owner + "/" + GitData.Repo + "/" + GitData.Branch).c_str(), RESET);

      std::string sGitHubRootNoBranch = GitData.sUPSLocalPath + GitData.Owner + "/" + GitData.Repo;
      std::string sGitHubRoot = sGitHubRootNoBranch + "/" + GitData.Branch;
      std::string sCommand;

      if (!g_File.FileExist(sGitHubRoot +  "/.git/config"))
        CLog::Log(logERROR, "Error while pushing. Directory is not a  GIT repo for Owner: %s, Repo: %s, Branch: %s\n", GitData.Owner.c_str(), GitData.Repo.c_str(), GitData.Branch.c_str());

      sCommand = "cd " + sGitHubRoot + ";";
      std::string sDryRun = bDryRun?"--dry-run ":"";
      sCommand += "git push " + sDryRun + "origin " + GitData.Branch;
      CLog::Log(logPRINT, "%s%s%s\n", KORNG, sCommand.c_str(), RESET);

      g_File.SytemCommand(sCommand);
    }
  }
}

void CProjectHandler::GenerateDiffListsPerRepo(std::string sPath, std::set<int> listReposToInclude)
{
  //Create a numeric repo list
  unsigned int iCounter = 0;
  std::map<unsigned int, CBasicGITData> MapReposToPush;
  for (std::map<std::string, CBasicGITData>::iterator it = m_MapGitRepos.begin(); it != m_MapGitRepos.end(); it++)
  {
    iCounter++;
    CBasicGITData GitData = it->second;
    if (listReposToInclude.find(iCounter) != listReposToInclude.end() && !GitData.listCommitData.empty())
      MapReposToPush[iCounter] = GitData;
  }

  std::string sPathListFiles = sPath;
  g_File.MakeDir(sPathListFiles);

  for (std::map<unsigned int, CBasicGITData>::iterator it = MapReposToPush.begin(); it != MapReposToPush.end(); it++)
  {
    const CBasicGITData& RepoData = it->second;
    const int& iRepoIndex = it->first;
    std::string sRepoName = ((iRepoIndex < 10)?"0":"") + g_CharsetUtils.IntToStr(iRepoIndex) + "_" + RepoData.Owner + "_" + RepoData.Repo + "_" + RepoData.Branch + (RepoData.bHasBeenAnSRCFileChange?" (SRC)":"");

    g_File.MakeDir(sPathListFiles + DirSepChar + sRepoName);
    int iNumOfCommit = 0;
    for (std::list<CCommitData>::const_iterator itcommlist = RepoData.listCommitData.begin(); itcommlist != RepoData.listCommitData.end(); itcommlist++)
    {
      const CCommitData& CommitData = *itcommlist;
      std::string sFilePath = sPathListFiles + DirSepChar + sRepoName + DirSepChar + CommitData.sCommitMessage + (!CommitData.listResWithSRCChange.empty()?" (SRC)":"") + ".diff";

      //write header
      std::string sHeader = "Repo: " + sRepoName + "\n";
      g_File.WriteFileFromStr(sFilePath, sHeader);

      //write git log
      std::string sGitCommand = "git log -n ";
      sGitCommand += g_CharsetUtils.IntToStr(RepoData.listCommitData.size()+2);
      sGitCommand += " --oneline";
      RunGitCommandIntoFile(RepoData, sGitCommand, sFilePath, "GIT LOG");

      //write short list of changed files
      sGitCommand = "git diff --name-status HEAD~";
      sGitCommand += g_CharsetUtils.IntToStr(iNumOfCommit+1);
      sGitCommand += "..HEAD~" + g_CharsetUtils.IntToStr(iNumOfCommit);
      RunGitCommandIntoFile(RepoData, sGitCommand, sFilePath, "LIST OF CHANGED FILES:");

      //write git diff of changed files
      sGitCommand = "git diff HEAD~";
      sGitCommand += g_CharsetUtils.IntToStr(iNumOfCommit+1);
      sGitCommand += "..HEAD~" + g_CharsetUtils.IntToStr(iNumOfCommit);
      RunGitCommandIntoFile(RepoData, sGitCommand, sFilePath, "DIFF OF CHANGED FILES:");
      iNumOfCommit++;
    }
  }
}

void CProjectHandler::GenerateCombinedDiffLists(std::string sPath, set< int > listReposToInclude)
{
  //Create a numeric repo list
  unsigned int iCounter = 0;
  std::map<unsigned int, CBasicGITData> MapReposToPush;
  for (std::map<std::string, CBasicGITData>::iterator it = m_MapGitRepos.begin(); it != m_MapGitRepos.end(); it++)
  {
    iCounter++;
    CBasicGITData GitData = it->second;
    if (listReposToInclude.find(iCounter) != listReposToInclude.end() && !GitData.listCommitData.empty())
      MapReposToPush[iCounter] = GitData;
  }

  std::string sPathListFiles = sPath;
  g_File.MakeDir(sPathListFiles);


  //write header
  std::string sPathToCombinedFile = sPathListFiles + DirSepChar + "GIT Logs.diff";

  std::string sHeader = "Combined GIT Logs of all repos:\n";
  g_File.WriteFileFromStr(sPathToCombinedFile, sHeader);

  for (std::map<unsigned int, CBasicGITData>::iterator it = MapReposToPush.begin(); it != MapReposToPush.end(); it++)
  {
    const CBasicGITData& RepoData = it->second;

    //write git log
    std::string sGitCommand = "git log -n ";
    sGitCommand += g_CharsetUtils.IntToStr(RepoData.listCommitData.size()+2);
    sGitCommand += " --oneline";
    RunGitCommandIntoFile(RepoData, sGitCommand, sPathToCombinedFile, "GIT LOG");
  }

  //write header
  sPathToCombinedFile = sPathListFiles + DirSepChar + "Changed Files.diff";

  sHeader = "Combined list of changed files for all repos:\n";
  g_File.WriteFileFromStr(sPathToCombinedFile, sHeader);

  for (std::map<unsigned int, CBasicGITData>::iterator it = MapReposToPush.begin(); it != MapReposToPush.end(); it++)
  {
    const CBasicGITData& RepoData = it->second;

    //write short list of changed files
    std::string sGitCommand = "git diff --name-status HEAD~";
    sGitCommand += g_CharsetUtils.IntToStr(RepoData.listCommitData.size());
    sGitCommand += "..HEAD~0";
    RunGitCommandIntoFile(RepoData, sGitCommand, sPathToCombinedFile, "LIST OF CHANGED FILES:");
  }

  //write header
  sPathToCombinedFile = sPathListFiles + DirSepChar + "Diffs of changed files.diff";

  sHeader = "Combined diffs of all changed files for all repos:\n";
  g_File.WriteFileFromStr(sPathToCombinedFile, sHeader);

  for (std::map<unsigned int, CBasicGITData>::iterator it = MapReposToPush.begin(); it != MapReposToPush.end(); it++)
  {
    const CBasicGITData& RepoData = it->second;

    //write git diff of changed files
    std::string sGitCommand = "git diff HEAD~";
    sGitCommand += g_CharsetUtils.IntToStr(RepoData.listCommitData.size());
    sGitCommand += "..HEAD~0";
    RunGitCommandIntoFile(RepoData, sGitCommand, sPathToCombinedFile, "DIFF OF CHANGED FILES:");
  }

}

void CProjectHandler::GenerateCombinedDiffListsSRC(std::string sPath, set< int > listReposToInclude)
{
  //Create a numeric repo list
  unsigned int iCounter = 0;
  std::map<unsigned int, CBasicGITData> MapReposToPush;
  for (std::map<std::string, CBasicGITData>::iterator it = m_MapGitRepos.begin(); it != m_MapGitRepos.end(); it++)
  {
    iCounter++;
    CBasicGITData GitData = it->second;
    if (listReposToInclude.find(iCounter) != listReposToInclude.end() && !GitData.listCommitData.empty())
      MapReposToPush[iCounter] = GitData;
  }

  std::string sPathListFiles = sPath;
  g_File.MakeDir(sPathListFiles);


  //write header
  std::string sPathToCombinedFile = sPathListFiles + DirSepChar + "GIT Logs (SRC).diff";

  std::string sHeader = "Combined GIT Logs of SRC changes for all repos:\n";
  g_File.WriteFileFromStr(sPathToCombinedFile, sHeader);

  for (std::map<unsigned int, CBasicGITData>::iterator it = MapReposToPush.begin(); it != MapReposToPush.end(); it++)
  {
    const CBasicGITData& RepoData = it->second;

    int iNumOfCommit = 0;
    for (std::list<CCommitData>::const_iterator itcommlist = RepoData.listCommitData.begin(); itcommlist != RepoData.listCommitData.end(); itcommlist++)
    {
      CCommitData CommitData = *itcommlist;
      if (!CommitData.listResWithSRCChange.empty())
      {
        //write git log
        std::string sGitCommand = "git log HEAD~";
        sGitCommand += g_CharsetUtils.IntToStr(iNumOfCommit+1);
        sGitCommand += "..HEAD~" + g_CharsetUtils.IntToStr(iNumOfCommit);

        RunGitCommandIntoFile(RepoData, sGitCommand, sPathToCombinedFile, "GIT LOG (SRC)");
      }
      iNumOfCommit++;
    }
  }

  //write header
  sPathToCombinedFile = sPathListFiles + DirSepChar + "Changed Files (SRC).diff";

  sHeader = "Combined list of changed SRC files for all repos:\n";
  g_File.WriteFileFromStr(sPathToCombinedFile, sHeader);

  for (std::map<unsigned int, CBasicGITData>::iterator it = MapReposToPush.begin(); it != MapReposToPush.end(); it++)
  {
    const CBasicGITData& RepoData = it->second;

    int iNumOfCommit = 0;

    for (std::list<CCommitData>::const_iterator itcommlist = RepoData.listCommitData.begin(); itcommlist != RepoData.listCommitData.end(); itcommlist++)
    {
      const CCommitData& CommitData = *itcommlist;

      if (!CommitData.listResWithSRCChange.empty())
      {

        //write short list of changed files
        std::string sGitCommand = "git diff --name-status HEAD~";
        sGitCommand += g_CharsetUtils.IntToStr(iNumOfCommit+1);
        sGitCommand += "..HEAD~" + g_CharsetUtils.IntToStr(iNumOfCommit);
        RunGitCommandIntoFile(RepoData, sGitCommand, sPathToCombinedFile, "LIST OF CHANGED SRC FILES:");
      }
      iNumOfCommit++;
    }
  }

  //write header
  sPathToCombinedFile = sPathListFiles + DirSepChar + "Diffs of changed files (SRC).diff";

  sHeader = "Combined diffs of all changed SRC files for all repos:\n";
  g_File.WriteFileFromStr(sPathToCombinedFile, sHeader);

  for (std::map<unsigned int, CBasicGITData>::iterator it = MapReposToPush.begin(); it != MapReposToPush.end(); it++)
  {
    const CBasicGITData& RepoData = it->second;

    int iNumOfCommit = 0;
    for (std::list<CCommitData>::const_iterator itcommlist = RepoData.listCommitData.begin(); itcommlist != RepoData.listCommitData.end(); itcommlist++)
    {

      const CCommitData& CommitData = *itcommlist;

      if (!CommitData.listResWithSRCChange.empty())
      {
        //write git diff of changed files
        std::string sGitCommand = "git diff HEAD~";
        sGitCommand += g_CharsetUtils.IntToStr(iNumOfCommit+1);
        sGitCommand += "..HEAD~" + g_CharsetUtils.IntToStr(iNumOfCommit);
        RunGitCommandIntoFile(RepoData, sGitCommand, sPathToCombinedFile, "DIFF OF CHANGED SRC FILES:");
      }
      iNumOfCommit++;
    }
  }
}

void CProjectHandler::CleanGitRepos()
{
  std::map<unsigned int, CBasicGITData> MapReposToPush;
  std::set<std::string> listValidGitRepoPaths;
  for (std::map<std::string, CBasicGITData>::iterator it = m_MapGitRepos.begin(); it != m_MapGitRepos.end(); it++)
  {
    CBasicGITData GitData = it->second;
    std::string sGitHubRootNoBranch = GitData.sUPSLocalPath + GitData.Owner + "/" + GitData.Repo;
    std::string sGitHubRoot = sGitHubRootNoBranch + "/" + GitData.Branch + "/";
    if (g_File.FileExist(sGitHubRoot +  ".git/config"))
      listValidGitRepoPaths.insert(sGitHubRoot);
  }
  g_File.ClearCleandDirOutput();
  g_File.CleanGitRepoDir(m_MapGitRepos.begin()->second.sUPSLocalPath, true, listValidGitRepoPaths);

  CLog::Log(logPRINT, "\033[s\033[K");
  if (g_File.GetCleanDirOutput().empty())
    CLog::Log(logPRINT, "No unecesarry files found.\n");
  else
    CLog::Log(logPRINT, "Cleaned files:\n%s", g_File.GetCleanDirOutput().c_str());
}

void CProjectHandler::RunGitCommandIntoFile(const CBasicGITData& RepoData, std::string sGitCommand, std::string sFilePath, std::string sHeader)
{
  std::string sGitHubRootNoBranch = RepoData.sUPSLocalPath + RepoData.Owner + "/" + RepoData.Repo;
  std::string sGitHubRoot = sGitHubRootNoBranch + "/" + RepoData.Branch;
  std::string sCommand;

  if (!g_File.FileExist(sGitHubRoot +  "/.git/config"))
    CLog::Log(logERROR, "Error while creatin diff lists. Directory is not a  GIT repo for Owner: %s, Repo: %s, Branch: %s\n", RepoData.Owner.c_str(), RepoData.Repo.c_str(), RepoData.Branch.c_str());

  sCommand = "cd " + sGitHubRoot + ";";
  sCommand += sGitCommand + " >> '" + sFilePath + "'";
  std::string sFile = g_File.ReadFileToStr(sFilePath);
  sFile += "\n\n------------------------------------------------------------------------";
  sFile += "\n" + sHeader + "\n";
  sFile += "Path: " + sGitHubRoot + "\n";
  sFile += "Git command: " + sGitCommand + "\n\n";
  g_File.WriteFileFromStr(sFilePath, sFile);

  g_File.SytemCommand(sCommand);
}


bool CProjectHandler::ParseRepoList(const std::string& sStringToParse, std::set<int>& ListRepos)
{
  if (sStringToParse.size() == 2)
    return false;

  size_t pos = 2;
  size_t nextPos;
  do
  {
    nextPos = sStringToParse.find_first_of(",-", pos);
    if (nextPos == std::string::npos || sStringToParse.at(nextPos) == ',')
    {
      std::string sNumber = sStringToParse.substr(pos, nextPos-pos);
      if (!isdigit(sNumber.at(0))) // verify if the first char is digit
        return false;

      int iNum  = strtol(sNumber.c_str(), NULL, 10);
      ListRepos.insert(iNum);
    }
    else
    {
      std::string sNumberFrom = sStringToParse.substr(pos, nextPos-pos);
      if (!isdigit(sNumberFrom.at(0))) // verify if the first char is digit
        return false;

      int iNumFrom  = strtol(sNumberFrom.c_str(), NULL, 10);

      pos =nextPos +1;
      nextPos = sStringToParse.find_first_of(",", pos);
      std::string sNumberTo = sStringToParse.substr(pos, (nextPos == std::string::npos)?std::string::npos:nextPos-pos);
      if (!isdigit(sNumberTo.at(0))) // verify if the first char is digit
        return false;

      int iNumTo  = strtol(sNumberTo.c_str(), NULL, 10);
      for (int i=iNumFrom; i<=iNumTo; i++)
        ListRepos.insert(i);
    }
    pos = nextPos +1;
  }
  while (nextPos != std::string::npos);

  return true;
}


bool CProjectHandler::FetchResourcesFromUpstream()
{
  g_HTTPHandler.GITPullUPSRepos(m_MapGitRepos, m_mapResData.begin()->second.bSkipGitReset);

  for (std::map<std::string, CResData>::iterator it = m_mapResData.begin(); it != m_mapResData.end(); it++)
  {
    const CResData& ResData = it->second;
    const std::string& sResName = it->first;

    CLog::Log(logPRINT, "%s%s%s (", KMAG, sResName.c_str(), RESET);

    if (m_mapResources.find(sResName) == m_mapResources.end()) // if it was not created in the map (by TX pull), make a new entry
    {
      CResourceHandler NewResHandler(ResData);
      m_mapResources[sResName] = NewResHandler;
    }

    if (!m_BForceUseCache)
      g_HTTPHandler.SetHTTPCacheExpire(ResData.iCacheExpire);

    m_mapResources[sResName].FetchPOFilesUpstreamToMem();
    CLog::Log(logPRINT, " )\n");
  }
  return true;
};


bool CProjectHandler::WriteResourcesToFile(std::string strProjRootDir)
{
//TODO
  g_File.DeleteDirectory(strProjRootDir + m_mapResData.begin()->second.sMRGLFilesDir);
  g_File.DeleteDirectory(strProjRootDir + m_mapResData.begin()->second.sUPDLFilesDir);

  for (T_itmapRes itmapResources = m_mapResources.begin(); itmapResources != m_mapResources.end(); itmapResources++)
  {
    const std::string& sResName = itmapResources->first;
    CResourceHandler& ResHandler = itmapResources->second;
    const CResData& ResData = m_mapResData[sResName];

    std::string sMergedLangDir = ResData.sProjRootDir + DirSepChar + ResData.sMRGLFilesDir + DirSepChar;
    std::string sAddonXMLPath = sMergedLangDir + ResData.MRG.AXMLPath;
    std::string sChangeLogPath =  sMergedLangDir + ResData.MRG.ChLogPath;
    std::string sLangPath  = sMergedLangDir + ResData.MRG.LPath;
    std::string sLangAddonXMLPath = sMergedLangDir + ResData.MRG.AXMLPath;
    ResHandler.GenerateMergedPOFiles ();
    ResHandler.WriteMergedPOFiles (sAddonXMLPath, sLangAddonXMLPath, sChangeLogPath, sLangPath);

    std::string sPathUpdate = ResData.sProjRootDir + ResData.sUPDLFilesDir + DirSepChar + ResData.sResName + DirSepChar + ResData.sBaseLForm + DirSepChar + "strings.po";
    ResHandler.GenerateUpdatePOFiles ();

    if (!m_BForceUseCache)
      g_HTTPHandler.SetHTTPCacheExpire(ResData.iCacheExpire);

    ResHandler.WriteUpdatePOFiles (sPathUpdate);
  }

  CLog::Log(logPRINT, "\n\n");
  return true;
};

bool CProjectHandler::WriteResourcesToLOCGitRepos(std::string strProjRootDir)
{
//TODO
  for (std::map<int, std::string>::iterator itResOrder = m_mapResOrder.begin(); itResOrder != m_mapResOrder.end(); itResOrder++)
  {
    const std::string& sResName = itResOrder->second;
    CResourceHandler& ResHandler = m_mapResources[sResName];

    ResHandler.WriteLOCPOFiles(m_CommitData, m_CommitDataSRC);
  }
  CLog::Log(logPRINT, "\n\n");
  return true;
};


bool CProjectHandler::CreateMergedResources()
{
  CLog::Log(LogHEADLINE, "MERGING RESOURCES\n");

  for (T_itmapRes it = m_mapResources.begin(); it != m_mapResources.end(); it++)
  {
    CResourceHandler& ResHandler = it->second;
    ResHandler.MergeResource();
  }

  return true;
}

void CProjectHandler::UploadTXUpdateFiles(std::string strProjRootDir)
{
  char charInput;
  CLog::Log(logPRINT, "\n");
  do
  {
    CLog::Log(logPRINT, "%sChoose option:%s %s0%s:Continue with update Transifex %s1%s:VIM UPD files %s2%s:VIM MRG files %ss%s:Skip. Your Choice:   \b\b", KRED, RESET, KMAG, RESET, KMAG, RESET, KMAG, RESET, KRED, RESET);
    cin >> charInput;

    if (charInput == '1')
    {
      std::string sCommand = "vim " + strProjRootDir + m_mapResData.begin()->second.sUPDLFilesDir;
      g_File.SytemCommand(sCommand);
    }
    else if (charInput == '2')
    {
      std::string sCommand = "vim " + strProjRootDir + m_mapResData.begin()->second.sMRGLFilesDir;
      g_File.SytemCommand(sCommand);
    }
    else if (charInput == 's')
      return;

    CLog::Log(logPRINT, "\e[A");
  }
  while (charInput != '0');

  CLog::Log(logPRINT, "                                                                                                                                        \n\e[A");

  g_HTTPHandler.Cleanup();
  g_HTTPHandler.ReInit();
  CLog::Log(logPRINT, "TXresourcelist");

  //TODO
  const std::string& strTargetProjectName = m_mapResData.begin()->second.UPD.ProjectName;

  //TODO ditry fix for always getting a fresh txlist here
  g_HTTPHandler.SetSkipCache(true);

  std::string strtemp = g_HTTPHandler.GetURLToSTR("https://www.transifex.com/api/2/project/" + strTargetProjectName + "/resources/");
  if (strtemp.empty())
    CLog::Log(logERROR, "ProjectHandler::FetchResourcesFromTransifex: error getting resources from transifex.net");

  g_HTTPHandler.SetSkipCache(false);
  CLog::Log(logPRINT, "\n\n");

  std::set<std::string> lResourcesAtTX = ParseResources(strtemp);

  g_HTTPHandler.Cleanup();
  g_HTTPHandler.ReInit();

  for (T_itmapRes it = m_mapResources.begin(); it != m_mapResources.end(); it++)
  {
    CResourceHandler& ResHandler = it->second;
    const std::string& sResName = it->first;
    const CResData& ResData = m_mapResData[sResName];

    if (!m_BForceUseCache)
      g_HTTPHandler.SetHTTPCacheExpire(ResData.iCacheExpire);

    ResHandler.UploadResourceToTransifex(lResourcesAtTX.find(sResName) == lResourcesAtTX.end());
  }

  return;
}

void CProjectHandler::GitCommitTranslationRepo(std::string sWorkingDir)
{
    char charInput;

    std::string sCachePath = g_File.GetHomePath();
    sCachePath += "/.cache/kodi-txupdate";
    std::string sDiffPath = sCachePath + "/MRGDiff.diff";

    CLog::Log(logPRINT, "\n");
    do
    {
      CLog::Log(logPRINT, "%sChoose option:%s %s0%s:Continue with git Commit %s1%s:GIT diff MRG files %ss%s:Skip. Your Choice:   \b\b", KRED, RESET, KMAG, RESET, KMAG, RESET, KRED, RESET);
      cin >> charInput;

      if (charInput == '1')
      { 
        std::string sCommand = "cd " + sWorkingDir + ";";
        sCommand += "git diff " + m_mapResData.begin()->second.sMRGLFilesDir + " > " + sDiffPath + " ;";
        sCommand += "vi " + sDiffPath;

        g_File.SytemCommand(sCommand);
      }
      else if (charInput == 's')
        return;

      CLog::Log(logPRINT, "\e[A");
    }
    while (charInput != '0');

    std::string sCommand = "cd " + sWorkingDir + ";";
    sCommand += "git add -A;";
    sCommand += "git commit -am \"[" + m_mapResData.begin()->second.UPD.ProjectName + "] sync\";";
    sCommand += "git push origin master";

    CLog::Log(logPRINT, "%s%s%s\n", KORNG, sCommand.c_str(), RESET);
    g_File.SytemCommand(sCommand);
}

void CProjectHandler::MigrateTranslators()
{
  //TODO
  CResData Resdata = m_mapResData.begin()->second;
  const std::string& strProjectName = Resdata.TRX.ProjectName;
  const std::string& strTargetProjectName = Resdata.UPD.ProjectName;
  const std::string& strTargetTXLangFormat = Resdata.UPD.LForm;

  if (strProjectName.empty() || strTargetProjectName.empty() || strProjectName == strTargetProjectName)
    CLog::Log(logERROR, "Cannot tranfer translators database. Wrong projectname and/or target projectname,");

  std::map<std::string, std::string> mapCoordinators, mapReviewers, mapTranslators;

  CLog::Log(logPRINT, "\n%sCoordinators:%s\n", KGRN, RESET);
  mapCoordinators = g_LCodeHandler.GetTranslatorsDatabase("coordinators", strProjectName, Resdata);

  CLog::Log(logPRINT, "\n%sReviewers:%s\n", KGRN, RESET);
  mapReviewers = g_LCodeHandler.GetTranslatorsDatabase("reviewers", strProjectName, Resdata);

  CLog::Log(logPRINT, "\n%sTranslators:%s\n", KGRN, RESET);
  mapTranslators = g_LCodeHandler.GetTranslatorsDatabase("translators", strProjectName, Resdata);

  CLog::Log(LogHEADLINE, "PUSH TRANSLATION GROUPS TO TX\n");

  g_LCodeHandler.UploadTranslatorsDatabase(mapCoordinators, mapReviewers, mapTranslators, strTargetProjectName, strTargetTXLangFormat);
};

void CProjectHandler::InitLCodeHandler()
{
//TODO
  CResData Resdata = m_mapResData.begin()->second;
  g_LCodeHandler.Init(Resdata.sLDatabaseURL, Resdata);
}

std::set<std::string> CProjectHandler::ParseResources(std::string strJSON)
{
  Json::Value root;   // will contains the root value after parsing.
  Json::Reader reader;
  std::string sTXResName, sResName;
  std::set<std::string> listResources;

  bool parsingSuccessful = reader.parse(strJSON, root );
  if ( !parsingSuccessful )
  {
    CLog::Log(logERROR, "JSONHandler: Parse resource: no valid JSON data");
    return listResources;
  }

  for(Json::ValueIterator itr = root.begin() ; itr != root.end() ; itr++)
  {
    Json::Value valu = *itr;
    sTXResName = valu.get("slug", "unknown").asString();

    if (sTXResName.size() == 0 || sTXResName == "unknown")
      CLog::Log(logERROR, "JSONHandler: Parse resource: no valid JSON data while iterating");

    //TODO check if a resource only exists at transifex and warn user
    if ((sResName = GetResNameFromTXResName(sTXResName)) != "")
      listResources.insert(sResName);
  };
  return listResources;
};

std::string CProjectHandler::GetResNameFromTXResName(const std::string& strTXResName)
{
  for (T_itResData itResdata = m_mapResData.begin(); itResdata != m_mapResData.end(); itResdata++)
  {
    if (itResdata->second.TRX.ResName == strTXResName)
      return itResdata->first;
  }
  return "";
}
