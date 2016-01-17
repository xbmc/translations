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

#include "FileUtils.h"
#include <stdio.h>
#include <sys/stat.h>
#include <ctime>
#include <iostream>
#include <fstream>
#include <sys/types.h>
#include <sys/stat.h>
#include "Log.h"
#include <ftw.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>

CFile g_File;

using namespace std;

bool CFile::MakeDir(std::string Path)
{
  if (DirExists(Path))
    return true;
  if (*Path.rbegin() != DirSepChar)
    Path += DirSepChar;

  size_t pos = 0;
  while ((pos = Path.find(DirSepChar,pos)) != std::string::npos)
  {
   if (!DirExists(Path.substr(0,pos+1)) && !MakeOneDir(Path.substr(0,pos+1)))
     return false;
   pos++;
  }
  return true;
}

bool CFile::MakeOneDir(std::string Path)
{
  return mkdir(Path.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) == 0;
};

bool CFile::DirExists(std::string Path)
{
  struct stat st;
  return (stat(Path.c_str(), &st) == 0);
};

bool CFile::FileExist(std::string filename) 
{
  FILE* pfileToTest = fopen (filename.c_str(),"rb");
  if (pfileToTest == NULL)
    return false;
  fclose(pfileToTest);
  return true;
};

void CFile::DeleteFile(std::string filename)
{
  if (!FileExist(filename))
    return;
  if (remove(filename.c_str()) != 0)
    CLog::Log(logERROR, "FileUtils: DeleteFile: unable to delete file: %s", filename.c_str());
};

std::string CFile::GetCurrTime()
{
  std::string strTime(64, '\0');
  time_t now = std::time(0);
  struct std::tm* gmtm = std::gmtime(&now);

  if (gmtm != NULL)
  {
    sprintf(&strTime[0], "%04i-%02i-%02i %02i:%02i+0000", gmtm->tm_year + 1900, gmtm->tm_mon + 1,
            gmtm->tm_mday, gmtm->tm_hour, gmtm->tm_min);
  }
  std::string strTimeCleaned = strTime.c_str();
  return strTimeCleaned;
};

std::string CFile::GetCurrYear()
{
  std::string strTime(64, '\0');
  time_t now = std::time(0);
  struct std::tm* gmtm = std::gmtime(&now);

  if (gmtm != NULL)
  {
    sprintf(&strTime[0], "%04i", gmtm->tm_year + 1900);
  }
  std::string strTimeCleaned = strTime.c_str();
  return strTimeCleaned;
};

std::string CFile::GetCurrMonth()
{
  std::string strTime(64, '\0');
  time_t now = std::time(0);
  struct std::tm* gmtm = std::gmtime(&now);

  if (gmtm != NULL)
  {
    sprintf(&strTime[0], "%02i", gmtm->tm_mon + 1);
  }
  std::string strTimeCleaned = strTime.c_str();
  return strTimeCleaned;
};

std::string CFile::GetCurrDay()
{
  std::string strTime(64, '\0');
  time_t now = std::time(0);
  struct std::tm* gmtm = std::gmtime(&now);

  if (gmtm != NULL)
  {
    sprintf(&strTime[0], "%02i", gmtm->tm_mday);
  }
  std::string strTimeCleaned = strTime.c_str();
  return strTimeCleaned;
};

std::string CFile::GetCurrMonthText()
{
  time_t now = std::time(0);
  struct std::tm* gmtm = std::gmtime(&now);

  std::string strTimeCleaned;
  if (gmtm != NULL)
  {
    switch (gmtm->tm_mon + 1)
    {
      case 1:  strTimeCleaned = "Jan"; break;
      case 2:  strTimeCleaned = "Feb"; break;
      case 3:  strTimeCleaned = "Mar"; break;
      case 4:  strTimeCleaned = "Apr"; break;
      case 5:  strTimeCleaned = "May"; break;
      case 6:  strTimeCleaned = "Jun"; break;
      case 7:  strTimeCleaned = "Jul"; break;
      case 8:  strTimeCleaned = "Aug"; break;
      case 9:  strTimeCleaned = "Sep"; break;
      case 10:  strTimeCleaned = "Oct"; break;
      case 11:  strTimeCleaned = "Nov"; break;
      case 12:  strTimeCleaned = "Dec"; break;
      default: CLog::Log(logWARNING, "FileUtils::GetCurrMonthText: wrong month numer");
    }
  }
  return strTimeCleaned;
};

void CFile::CopyFile(std::string strSourceFileName, std::string strDestFileName)
{
  ifstream source(strSourceFileName.c_str(), std::ios::binary);
  ofstream dest(strDestFileName.c_str(), std::ios::binary);

  dest << source.rdbuf();

  source.close();
  dest.close();
};

size_t CFile::GetFileAge(std::string strFileName)
{
  struct stat b;
  time_t now = std::time(0);

  if (g_File.FileExist(strFileName + ".time"))
  {
    time_t ReadFileAge = GetFileAgeFromFile(strFileName);
    return now-ReadFileAge;
  }

  else if (!stat(strFileName.c_str(), &b))
  {
    WriteFileAgeToFile(strFileName, b.st_mtime);
    return now-b.st_mtime;
  }

  else
  {
    CLog::Log(logWARNING, "FileUtils: Unable to determine the last modify date for file: %s", strFileName.c_str());
    return 0;
  }
};

size_t CFile::GetStoredAgeFromTimeFile(std::string strTimeFileName)
{
  if (!FileExist(strTimeFileName + ".time"))
    return -1;

  time_t now = std::time(0);

  time_t ReadFileAge = GetFileAgeFromFile(strTimeFileName);
  return now-ReadFileAge;
}

size_t CFile::GetAgeOfGitRepoPull(std::string strFileName)
{
  struct stat b;
  time_t now = std::time(0);

  if (!stat(strFileName.c_str(), &b))
    return now-b.st_mtime;
  else
    CLog::Log(logERROR, "FileUtils: Unable to determine the last modify date for file: %s", strFileName.c_str());

  return 0;
}

std::string CFile::ReadFileToStr(std::string strFileName)
{
  FILE * file;
  std::string strRead;
  file = fopen(strFileName.c_str(), "rb");
  if (!file)
    CLog::Log(logERROR, "FileUtils: ReadFileToStr: unable to read file: %s", strFileName.c_str());

  fseek(file, 0, SEEK_END);
  int64_t fileLength = ftell(file);
  fseek(file, 0, SEEK_SET);

  strRead.resize(static_cast<size_t> (fileLength));

  unsigned int readBytes =  fread(&strRead[0], 1, fileLength, file);
  fclose(file);

  if (readBytes != fileLength)
  {
    CLog::Log(logERROR, "FileUtils: actual read data differs from file size, for string file: %s",strFileName.c_str());
  }
  return strRead;
};

std::string CFile::ReadFileToStrE(std::string const &strFileName)
{
  std::string strEmpty;
  if (!FileExist(strFileName))
    return strEmpty;
  return ReadFileToStr(strFileName);
}

bool CFile::WriteFileFromStr(const std::string &pofilename, std::string const &strToWrite)
{
  std::string strDir = GetPath(pofilename);
  MakeDir(strDir);

  FILE * pFile = fopen (pofilename.c_str(),"wb");
  if (pFile == NULL)
  {
    CLog::Log(logERROR, "FileUtils: WriteFileFromStr failed for file: %s\n", pofilename.c_str());
    return false;
  }
  fprintf(pFile, "%s", strToWrite.c_str());
  fclose(pFile);

  return true;
};

void CFile::ConvertStrLineEnds(std::string &strToConvert)
{
  size_t foundPos = strToConvert.find_first_of("\r");
  if (foundPos == std::string::npos)
    return; // We have only Linux style line endings in the file, nothing to do

  if (foundPos+1 >= strToConvert.size() || strToConvert[foundPos+1] != '\n')
    CLog::Log(logDEBUG, "FileUtils: string has Mac Style Line Endings. Converted in memory to Linux LF");
  else
    CLog::Log(logDEBUG, "FileUtils: string has Win Style Line Endings. Converted in memory to Linux LF.");

  std::string strTemp;
  strTemp.reserve(strToConvert.size());
  for (std::string::const_iterator it = strToConvert.begin(); it < strToConvert.end(); it++)
  {
    if (*it == '\r')
    {
      if (it+1 == strToConvert.end() || *(it+1) != '\n')
        strTemp.push_back('\n'); // convert Mac style line ending and continue
        continue; // we have Win style line ending so we exclude this CR now
    }
    strTemp.push_back(*it);
  }
  strToConvert.swap(strTemp);
};

std::string CFile::GetPath(std::string const &strFilename)
{
  size_t posLastDirSepChar = strFilename.find_last_of(DirSepChar);
  if (posLastDirSepChar != std::string::npos)
    return strFilename.substr(0, posLastDirSepChar+1);
  return strFilename;
}

int unlink_cb(const char *fpath, const struct stat *sb, int typeflag, struct FTW *ftwbuf)
{
  int rv = remove(fpath);

  if (rv)
    CLog::Log(logWARNING, "FileUtils: unable to delete Directory: %s", fpath);

  return rv;
}

int CFile::DeleteDirectory(std::string strDirPath)
{
  return nftw(strDirPath.c_str(), unlink_cb, 64, FTW_DEPTH | FTW_PHYS);
}

time_t CFile::GetFileAgeFromFile(std::string strFileName)
{
  strFileName = strFileName + ".time";

  FILE * file;
  time_t readTime;
  file = fopen(strFileName.c_str(), "rb");
  if (!file)
    CLog::Log(logERROR, "FileUtils::GetFileAgeFromFile: unable to read file: %s", strFileName.c_str());

  fseek(file, 0, SEEK_END);
  int64_t fileLength = ftell(file);
  fseek(file, 0, SEEK_SET);

  if (fileLength != sizeof(time_t))
    CLog::Log(logERROR, "FileUtils::GetFileAgeFromFile: wrong file length for time file: %s", strFileName.c_str());

  unsigned int readBytes =  fread(&readTime, sizeof(time_t), 1, file);
  fclose(file);

  if (readBytes != 1)
  {
    CLog::Log(logERROR, "FileUtils::GetFileAgeFromFile actual read data differs from file size, for string file: %s",strFileName.c_str());
  }
  return readTime;
};

bool CFile::WriteFileAgeToFile(std::string strFileName, time_t FileAgeTime)
{
  strFileName = strFileName + ".time";

  std::string strDir = GetPath(strFileName);
  MakeDir(strDir);

  FILE * pFile = fopen (strFileName.c_str(),"wb");
  if (pFile == NULL)
  {
    CLog::Log(logERROR, "FileUtils::WriteFileAgeToFile failed for file: %s\n", strFileName.c_str());
    return false;
  }
  fwrite(&FileAgeTime, sizeof(time_t), 1, pFile);
  fclose(pFile);

  return true;
};

void CFile::WriteNowToFileAgeFile(std::string strFileName)
{
  time_t now = std::time(0);
  WriteFileAgeToFile(strFileName, now);
}

void CFile::SytemCommand (const std::string &strCommand)
{
  int status = system (strCommand.c_str());
  if (status == 32768)
    CLog::Log(logERROR, "System command failed with return value %i", WEXITSTATUS(status));
  if (status == 2) //|| status == 256)
    CLog::Log(logERROR, "System command aborted by user with return code %i", status);
}

bool CFile::isDir(string dir)
{
  struct stat fileInfo;
  stat(dir.c_str(), &fileInfo);
  if (S_ISDIR(fileInfo.st_mode)) {
    return true;
  } else {
    return false;
  }
}

void CFile::CleanDir(string baseDir, bool recursive, const std::set<std::string>& mapValidCacheFiles)
{
  CLog::Log(logPRINT, "\033[s\033[K%s\033[u", baseDir.c_str());
//  CLog::Log(logPRINT, "\033[1A");

  DIR *dp;
  struct dirent *dirp;

  if ((dp = opendir(baseDir.c_str())) == NULL)
    CLog::Log(logERROR, "Could not read cache directory: %s", baseDir.c_str());
  else
  {
    while ((dirp = readdir(dp)) != NULL)
    {
      if (dirp->d_name != string(".") && dirp->d_name != string(".."))
      {
        if (isDir(baseDir + dirp->d_name) == true && recursive == true)
        {
          std::string sDir = baseDir + dirp->d_name + "/";
          if (IsValidPath(mapValidCacheFiles, sDir))
            CleanDir(sDir, true, mapValidCacheFiles);
          else
          {
            m_sCleandDirOutput += sDir + "\n";
            DeleteDirectory(sDir);
          }
        }
        else
        {
          std::string sFileName = baseDir + dirp->d_name;
          if (!IsValidPath(mapValidCacheFiles, sFileName))
          {
            m_sCleandDirOutput += sFileName + "\n";
            DeleteFile(sFileName);
          }
        }
      }
    }
    closedir(dp);
  }
}

void CFile::ReadDirStructure(string baseDir, std::map<int, std::string>& listOfDirs)
{
  DIR *dp;
  struct dirent *dirp;

  if ((dp = opendir(baseDir.c_str())) == NULL)
    CLog::Log(logERROR, "Could not read directory: %s", baseDir.c_str());

  int iCounter = 1;
  std::set<std::string> setProjectNames;
  while ((dirp = readdir(dp)) != NULL)
  {
    if (dirp->d_name != string(".") && dirp->d_name != string(".."))
    {
      if (isDir(baseDir + dirp->d_name) == true)
      {
        std::string sDir = baseDir + dirp->d_name + "/";
        setProjectNames.insert(sDir);
      }
    }
  }

    closedir(dp);

  for (std::set<std::string>::iterator it = setProjectNames.begin(); it != setProjectNames.end(); it++)
  {
    listOfDirs[iCounter] = *it;
    iCounter++;
  }
}

void CFile::CleanGitRepoDir(string baseDir, bool recursive, const std::set<std::string>& listValidGitRepoPaths)
{
  CLog::Log(logPRINT, "\033[s\033[K%s\033[u", baseDir.c_str());

  DIR *dp;
  struct dirent *dirp;

  if ((dp = opendir(baseDir.c_str())) == NULL)
    CLog::Log(logERROR, "Could not read cache directory: %s", baseDir.c_str());
  else
  {
    while ((dirp = readdir(dp)) != NULL)
    {
      if (dirp->d_name != string(".") && dirp->d_name != string(".."))
      {
        if (isDir(baseDir + dirp->d_name) == true && recursive == true)
        {
          std::string sDir = baseDir + dirp->d_name + "/";
          bool bHasMatch, bHasExactMatch;
          IsValidGitPath(listValidGitRepoPaths, sDir, bHasMatch, bHasExactMatch);
          if (bHasMatch && !bHasExactMatch)
            CleanGitRepoDir(sDir, true, listValidGitRepoPaths);
          else if (!bHasMatch)
          {
            m_sCleandDirOutput += sDir + "\n";
            DeleteDirectory(sDir);
          }
        }
        else
        {
          std::string sFileName = baseDir + dirp->d_name;
          bool bHasMatch, bHasExactMatch;
          IsValidGitPath(listValidGitRepoPaths, sFileName, bHasMatch, bHasExactMatch);

          if (!bHasMatch)
          {
            m_sCleandDirOutput += sFileName + "\n";
            DeleteFile(sFileName);
          }
        }
      }
    }
    closedir(dp);
  }
}

void CFile::IsValidGitPath(const std::set<std::string>& listValidGitRepoPaths, const std::string& sPath, bool &bHasMatch, bool &bHasExactMatch)
{
  bHasMatch = false;
  bHasExactMatch = false;

  for (std::set<std::string>::iterator it = listValidGitRepoPaths.begin(); it != listValidGitRepoPaths.end(); it++)
  {
    if (*it == sPath)
    {
      bHasExactMatch = true;
      bHasMatch = true;
      return;
    }

    if (it->find(sPath) == 0)
      bHasMatch = true;
  }
  return;
}



std::string CFile::getcwd_string()
{
 char buff[PATH_MAX];
 getcwd( buff, PATH_MAX );
 std::string cwd( buff );
 return cwd;
}

bool CFile::IsValidPath(const std::set<std::string>& mapValidCacheFiles, const std::string& sPath)
{
  for (std::set<std::string>::iterator it = mapValidCacheFiles.begin(); it != mapValidCacheFiles.end(); it++)
  {
    if (it->find(sPath) == 0)
      return true;
  }
  return false;
}

string CFile::GetHomePath()
{
    const char* home = getenv("HOME");
    if (!home)
      CLog::Log(logERROR, "unable to determine HOME environment variable");

    std::string sHomePath(home);
    return sHomePath;
}
