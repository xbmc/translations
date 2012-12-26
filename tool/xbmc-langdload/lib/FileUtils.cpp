/*
 *      Copyright (C) 2012 Team XBMC
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

#include "FileUtils.h"
#include <stdio.h>
#include <sys/stat.h>
#include <ctime>
#include <iostream>
#include <fstream>
#include <sys/types.h>
#include <sys/stat.h>
#include <sstream>
#include "Log.h"
#ifdef _MSC_VER
#include <Windows.h>
#else
#include <ftw.h>
#include <unistd.h>
#endif

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
  #ifdef _MSC_VER
  return CreateDirectory (Path.c_str(), NULL) != 0;
  #else
  return mkdir(Path.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) == 0;
  #endif
};

bool CFile::DirExists(std::string Path)
{
  #ifdef _MSC_VER
  return (GetFileAttributes(Path.c_str()) == FILE_ATTRIBUTE_DIRECTORY);
  #else 
  struct stat st;
  return (stat(Path.c_str(), &st) == 0);
  #endif
};

bool CFile::FileExist(std::string filename) 
{
  FILE* pfileToTest = fopen (filename.c_str(),"rb");
  if (pfileToTest == NULL)
    return false;
  fclose(pfileToTest);
  return true;
};

void CFile::DelFile(std::string filename)
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

void CFile::CpFile(std::string strSourceFileName, std::string strDestFileName)
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
  if (!stat(strFileName.c_str(), &b)) 
  {
    time_t now = std::time(0);
    return static_cast<size_t>(now-b.st_mtime);
  }
  else
  {
    CLog::Log(logWARNING, "FileUtils: Unable to determine the last modify date for file: %s", strFileName.c_str());
    return 0;
  }
};

std::string CFile::ReadFileToStr(std::string strFileName)
{
  FILE * file;
  std::string strRead;
  file = fopen(strFileName.c_str(), "rb");
  if (!file)
    CLog::Log(logERROR, "FileUtils: ReadFileToStr: unable to read file: %s", strFileName.c_str());

  fseek(file, 0, SEEK_END);
  size_t fileLength = ftell(file);
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
    CLog::Log(logINFO, "FileUtils: string has Mac Style Line Endings. Converted in memory to Linux LF");
  else
    CLog::Log(logINFO, "FileUtils: string has Win Style Line Endings. Converted in memory to Linux LF.");

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

int CFile::DelDirectory(std::string strDirPath)
{
#ifdef _MSC_VER
  return RemoveDirectory(strDirPath.c_str());
#else
  return nftw(strDirPath.c_str(), unlink_cb, 64, FTW_DEPTH | FTW_PHYS);
#endif
}

void CFile::AddToFilename (std::string &strFilename, std::string strAddendum)
{
  if (strAddendum.empty())
    return;

  if (strFilename.find_last_of(DirSepChar) != strFilename.size()-1)
    strFilename += DirSepChar;
  if (strAddendum.find_first_of(DirSepChar) == 0)
    strAddendum = strAddendum.substr(1);
  if (strAddendum.find_last_of(DirSepChar) == strAddendum.size()-1)
    strAddendum = strAddendum.substr(0, strAddendum.size()-1);
  strFilename += strAddendum;
}

std::string CFile::IntToStr(int number)
{
  std::stringstream ss;//create a stringstream
  ss << number;//add number to the stream
  return ss.str();//return a string with the contents of the stream
};