/*
 *      Copyright (C) 2014 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#ifndef FILEUTILS_H
#define FILEUTILS_H

#pragma once

#include <string>
#include <ctime>
#include <set>
#include <map>

#include <unistd.h>
#define GetCurrentDir getcwd

static const char DirSepChar = '/';
#include <dirent.h>

class CFile
{
public:
  bool MakeDir(std::string Path);
  bool MakeOneDir(std::string Path);
  std::string GetPath(std::string const &strFilename);
  bool DirExists(std::string Path);
  bool FileExist(std::string filename);
  void DeleteFile(std::string filename);
  void CopyFile(std::string strSourceFileName, std::string strDestFileName);
  size_t GetFileAge(std::string strFileName);
  size_t GetAgeOfGitRepoPull(std::string strFileName);
  std::string ReadFileToStr(std::string strFileName);
  std::string ReadFileToStrE(std::string const &strFileName);
  bool WriteFileFromStr(const std::string &pofilename, std::string const &strToWrite);
  void ConvertStrLineEnds(std::string &strToConvert);
  int DeleteDirectory(std::string strDirPath);
  std::string GetCurrTime();
  std::string GetCurrYear();
  std::string GetCurrMonth();
  std::string GetCurrDay();
  std::string GetCurrMonthText();
  std::string GetHomePath();
  time_t GetFileAgeFromFile(std::string strFileName);
  size_t GetStoredAgeFromTimeFile(std::string strTimeFileName);
  bool WriteFileAgeToFile(std::string strFileName, time_t FileAgeTime);
  void WriteNowToFileAgeFile(std::string strFileName);
  void SytemCommand (const std::string &strCommand);
  bool isDir(std::string dir);
  void CleanDir(std::string baseDir, bool recursive, const std::set<std::string>& mapValidCacheFiles);
  void ClearCleandDirOutput() {m_sCleandDirOutput.clear();}
  std::string GetCleanDirOutput() {return m_sCleandDirOutput;}
  std::string m_sCleandDirOutput;
  void CleanGitRepoDir(std::string baseDir, bool recursive, const std::set<std::string>& listValidGitRepoPaths);
  void IsValidGitPath(const std::set<std::string>& listValidGitRepoPaths, const std::string& sPath, bool &bHasMatch, bool &bHasExactMatch);
  void ReadDirStructure(std::string baseDir, std::map<int, std::string>& listOfDirs);

  std::string getcwd_string();
  bool IsValidPath(const std::set<std::string>& mapValidCacheFiles, const std::string& sPath);
};

extern CFile g_File;
#endif