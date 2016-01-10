/*
 *      Copyright (C) 2014 Team Kodi
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

#include <string>
#include <stdio.h>
#include "src/ProjectHandler.h"
#include "src/HTTPUtils.h"
#include "src/Langcodes.h"
#include "jsoncpp/json/json.h"
#include "src/Log.h"
#include "src/FileUtils.h"
#include <iostream>
#include "src/CharsetUtils.h"

using namespace std;

void PrintUsage()
{
  CLog::Log(logPRINT,
  "Usage: kodi-txpudate PROJECTDIR (CMDARG)\n\n"
  "PROJECTDIR: the directory which contains the kodi-txupdate.conf settings file.\n"
  "            This will be the directory where your merged and transifex update files get generated.\n"
  "CMDARG:     Command line argument to control special modes. Available:\n"
  "            -c or --ForceUseCache : ensures to use the previously cached UPS, TRX files.\n\n"

  );
  return;
};

int main(int argc, char* argv[])
{
  CLog::Log(logPRINT, "\nKODI-TXUPDATE v%s by Team Kodi\n", VERSION.c_str());

  bool bNullArgMode = false;
  std::string sArg1;

  if (argc ==2)
    sArg1 = argv[1];

  if (argc == 1 || (argc ==2 && sArg1 == "-c"))
    bNullArgMode = true;

  setbuf(stdout, NULL);
  if (argc > 3)
  {
    CLog::Log(logPRINT, "\nBad arguments given\n\n");
    PrintUsage();
    return 1;
  }

  std::string WorkingDir, sCMDOption;

  bool bTransferTranslators = false; // In a VERY special case when transfer of translator database needed, set this to true
  bool bForceUseCache = false;
  std::string sHomePath = g_File.GetHomePath();

  if (!g_File.FileExist(sHomePath + "/.config/kodi-txupdate/passwords.xml"))
  {
    std::string sPWTR, sPWGH;
    CLog::Log(logPRINT, "\n%sMissing ~/.config/kodi-txupdate/passwords.xml file, creating one%s\n\n    Please enter password for Transifex: ", KMAG, RESET);
    cin >> sPWTR;
    CLog::Log(logPRINT, "\n    Please enter password for Github: ");
    cin >> sPWGH;
    std::string sXMLFile(sPasswordsXML);
    sXMLFile = g_CharsetUtils.replaceStrParts(sXMLFile, "$PWTR", sPWTR);
    sXMLFile = g_CharsetUtils.replaceStrParts(sXMLFile, "$PWGH", sPWGH);
    g_File.WriteFileFromStr(sHomePath + "/.config/kodi-txupdate/passwords.xml", sXMLFile);
    CLog::Log(logPRINT, "\n");
  }


  if (!bNullArgMode)
  {
    if (argv[1])
    WorkingDir = argv[1];
    if (WorkingDir.empty() || !g_File.DirExists(WorkingDir))
    {
      CLog::Log(logPRINT, "\nMissing or wrong project directory specified: %s, stopping.\n\n", WorkingDir.c_str());
      PrintUsage();
      return 1;
    }

    if (argc == 3) //we have an additional command line argument
    {
      if ( argv[2])
        sCMDOption = argv[2];
      if (WorkingDir.empty())
      {
        CLog::Log(logPRINT, "\nMissing command line argument, stopping.\n\n");
        PrintUsage();
        return 1;
      }
      if (sCMDOption == "-c" || sCMDOption == "--ForceUseCache")
        bForceUseCache = true;
      else
      {
        CLog::Log(logPRINT, "\nWrong command line argument: %s, stopping.\n\n", sCMDOption.c_str());
        PrintUsage();
        return 1;
      }
    }

    if (WorkingDir.find('/') != 0) //We have a relative path, make it absolute
    {
      std::string sCurrentPath = g_File.getcwd_string();
      WorkingDir = sCurrentPath + "/" + WorkingDir;
    }
  }
  else
  {
    if (sArg1 == "-c")
      bForceUseCache = true;

    std::string sPathToDefaultDir = sHomePath + "/.config/kodi-txupdate/default-dir.conf";
    std::string sDefaultDir;

    if (!g_File.FileExist(sPathToDefaultDir))
    {
      CLog::Log(logPRINT, "\n%sMissing default directory file at ~/.config/kodi-txupdate/default-dir.conf%s\n\n"
      "    Please enter an absolute path where you would like to place the translations git repo clone\n"
      "    Default: /home/translator/transifex/ (just hit enter if ok) :", KMAG, RESET);
      cin.ignore();
      getline(cin, sDefaultDir);

      if (sDefaultDir.empty())
        sDefaultDir = "/home/translator/transifex/";

      g_File.WriteFileFromStr(sPathToDefaultDir, sDefaultDir);
      CLog::Log(logPRINT, "\n");
    }

    sDefaultDir = g_File.ReadFileToStr(sPathToDefaultDir);

    if (!g_File.FileExist(sDefaultDir + "translations/.git/config"))
    {
      g_File.DeleteDirectory(sDefaultDir + "translations");
      std::string sCommand = "cd " + sDefaultDir + ";";
      sCommand += "git clone git@github.com:xbmc/translations.git";
      CLog::Log(logPRINT, "\n%sMissing the kodi translations GIT clone, so cloning one with the following command:\n%s\n\n%s%s%s\n",KMAG, RESET, KORNG, sCommand.c_str(), RESET);
      g_File.SytemCommand(sCommand);
    }

    //check if all vim related files are in place. If not create them
    if (!g_File.FileExist(sHomePath + "/.vimrc"))
      g_File.CopyFile(sDefaultDir + "translations/tool/kodi-txupdate/vim/vimrc", sHomePath + "/.vimrc");
    if (!g_File.FileExist(sHomePath + "/.vim/colors/desert256.vim"))
    {
      g_File.MakeDir(sHomePath + "/.vim/colors");
      g_File.CopyFile(sDefaultDir + "translations/tool/kodi-txupdate/vim/colors/desert256.vim", sHomePath + "/.vim/colors/desert256.vim");
    }
    if (!g_File.FileExist(sHomePath + "/.vim/syntax/ktx.vim"))
    {
      g_File.MakeDir(sHomePath + "/.vim/syntax");
      g_File.CopyFile(sDefaultDir + "translations/tool/kodi-txupdate/vim/syntax/ktx.vim", sHomePath + "/.vim/syntax/ktx.vim");
    }
    if (!g_File.FileExist(sHomePath + "/.vim/ftdetect/ktx.vim"))
    {
      g_File.MakeDir(sHomePath + "/.vim/ftdetect");
      g_File.CopyFile(sDefaultDir + "translations/tool/kodi-txupdate/vim/ftdetect/ktx.vim", sHomePath + "/.vim/ftdetect/ktx.vim");
    }

    CLog::Log(logPRINT, "\n%sPlease choose project:%s\n", KMAG, RESET);
    std::map<int, std::string> listOfDirs;
    g_File.ReadDirStructure(sDefaultDir + "translations/kodi-translations/", listOfDirs);

    for (std::map<int, std::string>::iterator it = listOfDirs.begin(); it != listOfDirs.end(); it++)
    {
      std::string sProjectName = g_CharsetUtils.GetFilenameFromURL(it->second.substr(0,it->second.size()-1));
      CLog::Log(logPRINT, "\n    %i. %s", it->first, sProjectName.c_str());
    }
    CLog::Log(logPRINT, "\n\n    Project to sync: ");
    unsigned int iChoice;
    cin >> iChoice;
    if (iChoice > listOfDirs.size())
    {
      CLog::Log(logPRINT, "\n%sInvalid project number chosen ! Exiting !%s\n\n", KRED, RESET);
      return 1;
    }
    WorkingDir = listOfDirs[iChoice];
  }

  try
  {
    g_HTTPHandler.LoadCredentials(sHomePath + "/.config/kodi-txupdate/passwords.xml");
    g_HTTPHandler.SetCacheDir(WorkingDir + ".httpcache");

    if (WorkingDir[WorkingDir.length()-1] != DirSepChar)
      WorkingDir.append(&DirSepChar);

    CLog::Log(logDEBUG, "Root Directory: %s", WorkingDir.c_str());

    CProjectHandler TXProject;
    TXProject.SetProjectDir(WorkingDir);
    TXProject.LoadConfigToMem(bForceUseCache);

    TXProject.InitLCodeHandler();

    if (!bTransferTranslators)
    {
      CLog::Log(LogHEADLINE, "DOWNLOADING RESOURCES FROM TRANSIFEX.NET\n");
      TXProject.FetchResourcesFromTransifex();

      CLog::Log(LogHEADLINE, "DOWNLOADING RESOURCES FROM UPSTREAM\n");
      TXProject.FetchResourcesFromUpstream();

      TXProject.CreateMergedResources();

      CLog::Log(LogHEADLINE, "WRITING MERGED RESOURCES TO HDD\n");
      TXProject.WriteResourcesToFile(WorkingDir);

      CLog::Log(LogHEADLINE, "UPLOADING LANGUAGE FILES TO TRANSIFEX.NET\n");
      TXProject.UploadTXUpdateFiles(WorkingDir);

      CLog::Log(LogHEADLINE, "WRITING RESOURCES TO LOCAL GITHUB REPOS\n");
      TXProject.WriteResourcesToLOCGitRepos(WorkingDir);

      CLog::Log(LogHEADLINE, "GIT PUSHING LOCAL GIT REPOS\n");
      TXProject.GITPushLOCGitRepos();
    }

    if (bTransferTranslators)
    {
      CLog::Log(LogHEADLINE, "GET TRANSLATION GROUPS\n");
      TXProject.MigrateTranslators();
    }

    CLog::Log(LogHEADLINE, "CLEANING CACHE FILE DIRECTORY FROM UNUSED FILES\n");
    g_HTTPHandler.CleanCacheFiles();

    CLog::Log(LogHEADLINE, "CLEANING GIT REPOS FROM UNUSED REPO DIRECTORIES\n");
    TXProject.CleanGitRepos();

    CLog::Log(LogHEADLINE, "GIT COMMIT TO KODI TRANSLATION REPO\n");
    TXProject.GitCommitTranslationRepo(WorkingDir);


    if (CLog::GetWarnCount() ==0)
      CLog::Log(LogHEADLINE, "PROCESS FINISHED SUCCESFULLY WITHOUT WARNINGS\n");
    else
      CLog::Log(LogHEADLINE, "%sPROCESS FINISHED WITH %i WARNINGS%s\n", KRED, CLog::GetWarnCount(), KGRN);

    g_HTTPHandler.Cleanup();
    return 0;
  }
  catch (const int calcError)
  {
    g_HTTPHandler.Cleanup();
    return 100;
  }
}
