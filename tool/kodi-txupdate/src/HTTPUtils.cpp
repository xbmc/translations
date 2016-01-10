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

#include "Log.h"
#include "HTTPUtils.h"
#include <curl/easy.h>
#include "FileUtils.h"
#include <cctype>
#include "Fileversioning.h"
#include "jsoncpp/json/json.h"
#include "Langcodes.h"
#include "CharsetUtils.h"


CHTTPHandler g_HTTPHandler;

using namespace std;

CHTTPHandler::CHTTPHandler()
{
  m_curlHandle = curl_easy_init();
  m_bSkipCache = false;
  m_bDataFile = false;
};

CHTTPHandler::~CHTTPHandler()
{
  Cleanup();
};

std::string CHTTPHandler::GetHTTPErrorFromCode(int http_code)
{
  if (http_code == 503) return ": Service Unavailable (probably TX server maintenance) please try later.";
  else if (http_code == 400) return ": Bad request (probably an error in the utility or the API changed. Please contact the Developer.";
  else if (http_code == 401) return ": Unauthorized. Please create a .passwords file in the project root dir with credentials.";
  else if (http_code == 403) return ": Forbidden. Service is currently forbidden.";
  else if (http_code == 404) return ": File not found on the URL.";
  else if (http_code == 500) return ": Internal server error. Try again later, or contact the Utility Developer.";
  return "";
}

void CHTTPHandler::HTTPRetry(int nretry)
{
  for (int i = 0; i < nretry*6; i++)
  {
    CLog::Log(logPRINT, " Retry %i: %i  \b\b\b\b\b\b\b\b\b\b\b\b\b", nretry, nretry*6-i);
    if (nretry*6-i > 9)
      CLog::Log(logPRINT, "\b");
    usleep(300000);
  }
  CLog::Log(logPRINT, "             \b\b\b\b\b\b\b\b\b\b\b\b\b");
  g_HTTPHandler.Cleanup();
  g_HTTPHandler.ReInit();
}


std::string CHTTPHandler::GetURLToSTR(std::string strURL)
{
  bool bCacheFileExists, bCacheFileExpired;
  std::string sCacheFileName = CreateCacheFilename(strURL, bCacheFileExists, bCacheFileExpired);
  if (sCacheFileName != "")
  {
    m_mapValidCacheFiles.insert(sCacheFileName + ".version");
    m_mapValidCacheFiles.insert(sCacheFileName + ".time");
    m_mapValidCacheFiles.insert(sCacheFileName);
  }

  std::string strBuffer, strCachedFileVersion, strWebFileVersion;

  strWebFileVersion = g_Fileversion.GetVersionForURL(strURL);

  if (strWebFileVersion != "" && g_File.FileExist(sCacheFileName + ".version"))
    strCachedFileVersion = g_File.ReadFileToStr(sCacheFileName + ".version");

  bool bFileChangedOnWeb = strCachedFileVersion != strWebFileVersion;

  if (!bCacheFileExists || (bCacheFileExpired && (strWebFileVersion == "" || bFileChangedOnWeb)))
  {
    CLog::Log(logPRINT, "%s*%s", KGRN, RESET);
    g_File.DeleteFile(sCacheFileName + ".version");
    g_File.DeleteFile(sCacheFileName + ".time");

    curlURLToCache(strURL, strBuffer);

    if (!m_bSkipCache)
      g_File.WriteFileFromStr(sCacheFileName, strBuffer);

    if (strWebFileVersion != "")
      g_File.WriteFileFromStr(sCacheFileName + ".version", strWebFileVersion);

    if (!m_bSkipCache)
      g_File.WriteNowToFileAgeFile(sCacheFileName);
  }
  else
  {
    strBuffer = g_File.ReadFileToStr(sCacheFileName);
    if (bCacheFileExpired)
      CLog::Log(logPRINT, "%s-%s", KCYN, RESET);
    else
      CLog::Log(logPRINT, "%s.%s", KORNG, RESET);
  }

  return strBuffer;
};

void CHTTPHandler::curlURLToCache(std::string strURL, std::string &strBuffer)
{
  CURLcode curlResult;
  strURL = URLEncode(strURL);

  CLoginData LoginData = GetCredentials(strURL);

    if(m_curlHandle) 
    {
      int nretry = 0;
      bool bSuccess;
      long http_code = 0;
      do
      {
        strBuffer.clear();
        if (nretry > 0)
          HTTPRetry(nretry);

        curl_easy_setopt(m_curlHandle, CURLOPT_URL, strURL.c_str());
        curl_easy_setopt(m_curlHandle, CURLOPT_WRITEFUNCTION, Write_CurlData_String);
        if (!LoginData.strLogin.empty())
        {
          curl_easy_setopt(m_curlHandle, CURLOPT_USERNAME, LoginData.strLogin.c_str());
          curl_easy_setopt(m_curlHandle, CURLOPT_PASSWORD, LoginData.strPassword.c_str());
        }
        curl_easy_setopt(m_curlHandle, CURLOPT_FAILONERROR, true);
        curl_easy_setopt(m_curlHandle, CURLOPT_WRITEDATA, &strBuffer);
        curl_easy_setopt(m_curlHandle, CURLOPT_USERAGENT, sUSERAGENT.c_str());
        curl_easy_setopt(m_curlHandle, CURLOPT_SSL_VERIFYPEER, 0);
        curl_easy_setopt(m_curlHandle, CURLOPT_VERBOSE, 0);
        curl_easy_setopt(m_curlHandle, CURLOPT_FOLLOWLOCATION, true);

        curlResult = curl_easy_perform(m_curlHandle);
        curl_easy_getinfo (m_curlHandle, CURLINFO_RESPONSE_CODE, &http_code);
        nretry++;
        bSuccess = (curlResult == 0 && http_code >= 200 && http_code < 400);
      }
      while (nretry < 5 && !bSuccess);

      if (!bSuccess)
        CLog::Log(logERROR, "HTTPHandler: curlURLToCache finished with error: \ncurl error: %i, %s\nhttp error: %i%s\nURL: %s\n",
                  curlResult, curl_easy_strerror(curlResult), http_code, GetHTTPErrorFromCode(http_code).c_str(),  strURL.c_str());

      return;
    }
    else
      CLog::Log(logERROR, "HTTPHandler: curlURLToCache failed because Curl was not initalized");
    return;
};

void CHTTPHandler::ReInit()
{
  if (!m_curlHandle)
    m_curlHandle = curl_easy_init();
  else
    CLog::Log(logWARNING, "HTTPHandler: Trying to reinitalize an already existing Curl handle");
};

void CHTTPHandler::Cleanup()
{
  if (m_curlHandle)
  {
    curl_easy_cleanup(m_curlHandle);
    m_curlHandle = NULL;
  }
};

size_t Read_CurlData_File(char *bufptr, size_t size, size_t nitems, FILE *stream) 
{
  size_t read;
  read = fread(bufptr, size, nitems, stream);
  return read;
}


size_t Write_CurlData_String(char *data, size_t size, size_t nmemb, string *buffer)
{
  size_t written = 0;
  if(buffer != NULL)
  {
    buffer -> append(data, size * nmemb);
    written = size * nmemb;
  }
  return written;
}

typedef struct
{ 
  std::string * pPOString; 
  size_t pos; 
} Tputstrdata; 


size_t Read_CurlData_String(void * ptr, size_t size, size_t nmemb, void * stream)
{
  if (stream)
  {
    Tputstrdata * pPutStrData = (Tputstrdata*) stream; 

    size_t available = (pPutStrData->pPOString->size() - pPutStrData->pos);

    if (available > 0)
    {
      size_t written = std::min(size * nmemb, available);
      memcpy(ptr, &pPutStrData->pPOString->at(pPutStrData->pos), written); 
      pPutStrData->pos += written;
      return written; 
    }
  }

  return 0; 
}

void CHTTPHandler::SetCacheDir(std::string strCacheDir)
{
  if (!g_File.DirExists(strCacheDir))
    g_File.MakeDir(strCacheDir);
  m_strCacheDir = strCacheDir + DirSepChar;
  CLog::Log(logDEBUG, "HTTPHandler: Cache directory set to: %s", strCacheDir.c_str());
};

bool CHTTPHandler::LoadCredentials (std::string CredentialsFilename)
{
  TiXmlDocument xmlPasswords;

  if (!xmlPasswords.LoadFile(CredentialsFilename.c_str()))
  {
    CLog::Log(logDEBUG, "HTTPHandler: No \".passwords.xml\" file exists in project dir. No password protected web download will be available.");
    return false;
  }

  CLog::Log(logDEBUG, "HTTPHandler: Succesfuly found the .passwsords.xml file: %s", CredentialsFilename.c_str());

  TiXmlElement* pRootElement = xmlPasswords.RootElement();

  if (!pRootElement || pRootElement->NoChildren() || pRootElement->ValueTStr()!="websites")
  {
    CLog::Log(logWARNING, "HTTPHandler: No root element called \"websites\" in xml file.");
    return false;
  }

  CLoginData LoginData;

  const TiXmlElement *pChildElement = pRootElement->FirstChildElement("website");
  while (pChildElement && pChildElement->FirstChild())
  {
    std::string strWebSitePrefix = pChildElement->Attribute("prefix");
    if (pChildElement->FirstChild())
    {
      const TiXmlElement *pChildLOGINElement = pChildElement->FirstChildElement("login");
      if (pChildLOGINElement && pChildLOGINElement->FirstChild())
        LoginData.strLogin = pChildLOGINElement->FirstChild()->Value();
      const TiXmlElement *pChildPASSElement = pChildElement->FirstChildElement("password");
      if (pChildPASSElement && pChildPASSElement->FirstChild())
        LoginData.strPassword = pChildPASSElement->FirstChild()->Value();

      m_mapLoginData [strWebSitePrefix] = LoginData;
      CLog::Log(logDEBUG, "HTTPHandler: found login credentials for website prefix: %s", strWebSitePrefix.c_str());
    }
    pChildElement = pChildElement->NextSiblingElement("website");
  }

  return true;
};

CLoginData CHTTPHandler::GetCredentials (std::string strURL)
{
  CLoginData LoginData;
  for (itMapLoginData = m_mapLoginData.begin(); itMapLoginData != m_mapLoginData.end(); itMapLoginData++)
  {
    if (strURL.find(itMapLoginData->first) != std::string::npos)
    {
      LoginData = itMapLoginData->second;
      return LoginData;
    }
  }

  return LoginData;
};

std::string CHTTPHandler::URLEncode (std::string strURL)
{
  std::string strOut;
  for (std::string::iterator it = strURL.begin(); it != strURL.end(); it++)
  {
    if (*it == ' ')
      strOut += "%20";
    else
      strOut += *it;
  }
  return strOut;
}

bool CHTTPHandler::PutFileToURL(std::string const &sPOFile, std::string const &strURL, bool &bUploaded,
                                size_t &iAddedNew, size_t &iUpdated)
{
  bool bCacheFileExists, bCacheFileExpired;

  std::string sCacheFileName = CreateCacheFilename(strURL, bCacheFileExists, bCacheFileExpired);
  std::string sCacheFile;

  if (bCacheFileExists && !bCacheFileExpired)  //In case cachefile expired, mark it as unvalid for later clean
    sCacheFile = g_File.ReadFileToStr(sCacheFileName);

  m_mapValidCacheFiles.insert(sCacheFileName);
  m_mapValidCacheFiles.insert(sCacheFileName + ".time");


  if (sCacheFile == sPOFile)
  {
    CLog::Log(logDEBUG, "HTTPHandler::PutFileToURL: not necesarry to upload file as it has not changed from last upload.");
    return true;
  }


  long result = curlPUTPOStrToURL(sPOFile, strURL, iAddedNew, iUpdated);
  if (result < 200 || result >= 400)
  {
    CLog::Log(logERROR, "HTTPHandler::PutFileToURL: File upload was unsuccessful, http errorcode: %i", result);
    return false;
  }

  CLog::Log(logDEBUG, "HTTPHandler::PutFileToURL: File upload was successful so creating a copy at the .httpcache directory");

  if (!bCacheFileExists || bCacheFileExpired)
  {
    g_File.WriteFileFromStr(sCacheFileName, sPOFile);
    g_File.WriteNowToFileAgeFile(sCacheFileName);
  }

  bUploaded = true;

  return true;
};

long CHTTPHandler::curlPUTPOStrToURL(std::string const &strPOFile, std::string const &cstrURL, size_t &stradded, size_t &strupd)
{

  CURLcode curlResult;

  std::string strURL = URLEncode(cstrURL);

  std::string strServerResp;
  CLoginData LoginData = GetCredentials(strURL);

  if(m_curlHandle) 
  {
    int nretry = 0;
    bool bSuccess;
    long http_code = 0;
    do
    {
      strServerResp.clear();
      if (nretry > 0)
        HTTPRetry(nretry);

      struct curl_httppost *post1;
      struct curl_httppost *postend;

      post1 = NULL;
      postend = NULL;
      curl_formadd(&post1, &postend,
                  CURLFORM_COPYNAME, "file",
                  CURLFORM_BUFFER, "strings.po",
                  CURLFORM_BUFFERPTR, &strPOFile[0],
                  CURLFORM_BUFFERLENGTH, strPOFile.size(),
                  CURLFORM_CONTENTTYPE, "application/octet-stream",
                  CURLFORM_END);

      curl_easy_setopt(m_curlHandle, CURLOPT_WRITEFUNCTION, Write_CurlData_String);
      curl_easy_setopt(m_curlHandle, CURLOPT_URL, strURL.c_str());
      curl_easy_setopt(m_curlHandle, CURLOPT_NOPROGRESS, 1L);
      curl_easy_setopt(m_curlHandle, CURLOPT_HEADER, 1L);
      curl_easy_setopt(m_curlHandle, CURLOPT_FOLLOWLOCATION, 1L);
      curl_easy_setopt(m_curlHandle, CURLOPT_HTTPPOST, post1);
      curl_easy_setopt(m_curlHandle, CURLOPT_USERAGENT, sUSERAGENT.c_str());
      curl_easy_setopt(m_curlHandle, CURLOPT_MAXREDIRS, 50L);
      curl_easy_setopt(m_curlHandle, CURLOPT_CUSTOMREQUEST, "PUT");

      if (!LoginData.strLogin.empty())
      {
        curl_easy_setopt(m_curlHandle, CURLOPT_USERNAME, LoginData.strLogin.c_str());
        curl_easy_setopt(m_curlHandle, CURLOPT_PASSWORD, LoginData.strPassword.c_str());
      }
      curl_easy_setopt(m_curlHandle, CURLOPT_WRITEDATA, &strServerResp);
      curl_easy_setopt(m_curlHandle, CURLOPT_SSL_VERIFYPEER, 0);
      curl_easy_setopt(m_curlHandle, CURLOPT_VERBOSE, 0);
      curl_easy_setopt(m_curlHandle, CURLOPT_SSL_VERIFYHOST, 0);

      curlResult = curl_easy_perform(m_curlHandle);

      curl_easy_getinfo (m_curlHandle, CURLINFO_RESPONSE_CODE, &http_code);

      curl_formfree(post1);
      post1 = NULL;

      bSuccess = (curlResult == 0 && http_code >= 200 && http_code < 400);
      nretry++;
    }
    while (nretry < 5 && !bSuccess);

    if (bSuccess)
      CLog::Log(logDEBUG, "HTTPHandler::curlFileToURL finished with success from File to URL %s", strURL.c_str());
    else
      CLog::Log(logERROR, "HTTPHandler::curlFileToURL finished with error: \ncurl error: %i, %s\nhttp error: %i%s\nURL: %s\n",
                curlResult, curl_easy_strerror(curlResult), http_code, GetHTTPErrorFromCode(http_code).c_str(), strURL.c_str());

    size_t jsonPos = strServerResp.find_first_of("{");
    if (jsonPos == std::string::npos)
      CLog::Log(logERROR, "HTTPHandler::curlFileToURL no valid Transifex server response received");

    strServerResp = strServerResp.substr(jsonPos);
    ParseUploadedStringsData(strServerResp, stradded, strupd);

    return http_code;
  }
  else
    CLog::Log(logERROR, "HTTPHandler::curlFileToURL failed because Curl was not initalized");
  return 700;
};

std::string CHTTPHandler::CreateCacheFilename(const std::string& strURL, bool &bCacheFileExists, bool &bCacheFileExpired)
{
  if (m_bSkipCache)
    return "";

  std::string sCacheFileName = m_strCacheDir;
  if (!m_sFileLocation.empty())
    sCacheFileName += m_sFileLocation + DirSepChar;
  if (!m_sProjectName.empty())
    sCacheFileName += m_sProjectName + DirSepChar;
  if (!m_sResName.empty())
    sCacheFileName += m_sResName + DirSepChar;

  //If we download from github, get branch into count
  if (strURL.find("raw.github.com/") != std::string::npos || strURL.find("raw2.github.com/") != std::string::npos ||
      strURL.find("raw.githubusercontent.com/") != std::string::npos ||
      strURL.find("raw2.githubusercontent.com/") != std::string::npos)
  {
    CGithubURLData GitData;
    GetGithubData(strURL,GitData);
  }

  if (!m_sLCode.empty())
    sCacheFileName += m_sLCode + DirSepChar;

  if (m_bDataFile)
    sCacheFileName += sCACHEDATADIRNAME + DirSepChar;

  sCacheFileName += m_sFileName;

  bCacheFileExists = g_File.FileExist(sCacheFileName);

  size_t CacheFileAge = bCacheFileExists ? g_File.GetFileAge(sCacheFileName): -1; //in seconds
  size_t MaxCacheFileAge = m_iHTTPCacheExp * 60; // in seconds

  bCacheFileExpired = CacheFileAge > MaxCacheFileAge;

  m_mapValidCacheFiles.insert(sCacheFileName);
  return sCacheFileName;
}

void CHTTPHandler::SetGitPushTime(const std::string& sOwner, const std::string& sRepo, const std::string& sBranch)
{
  std::string sCacheFileName = m_strCacheDir;
  sCacheFileName += "UPS";
  sCacheFileName += DirSepChar + sCACHEDATADIRNAME + DirSepChar;
  sCacheFileName += "0_PushTimes";
  sCacheFileName += DirSepChar + sOwner + DirSepChar  + sRepo + DirSepChar+ sBranch;

  g_File.WriteNowToFileAgeFile(sCacheFileName);
}

size_t CHTTPHandler::GetLastGitPushAge(const string& sOwner, const string& sRepo, const string& sBranch)
{
  std::string sCacheFileName = m_strCacheDir;
  sCacheFileName += "UPS";
  sCacheFileName += DirSepChar + sCACHEDATADIRNAME + DirSepChar;
  sCacheFileName += "0_PushTimes";
  sCacheFileName += DirSepChar + sOwner + DirSepChar  + sRepo + DirSepChar+ sBranch;

  return g_File.GetStoredAgeFromTimeFile(sCacheFileName);
}

void CHTTPHandler::AddValidGitPushTimeCachefile(const std::string& sOwner, const std::string& sRepo, const std::string& sBranch)
{
  std::string sCacheFileName = m_strCacheDir;
  sCacheFileName += "UPS";
  sCacheFileName += DirSepChar + sCACHEDATADIRNAME + DirSepChar;
  sCacheFileName += "0_PushTimes";
  sCacheFileName += DirSepChar + sOwner + DirSepChar  + sRepo + DirSepChar+ sBranch;

  m_mapValidCacheFiles.insert(sCacheFileName + ".time");
}


std::string CHTTPHandler::CreateCacheFilenameGitSource(const std::string& sBranch, bool &bCacheFileExists, bool &bCacheFileExpired)
{
  if (m_bSkipCache)
    return "";

  std::string sCacheFileName = m_strCacheDir;
  if (!m_sFileLocation.empty())
    sCacheFileName += m_sFileLocation + DirSepChar;
  if (!m_sProjectName.empty())
    sCacheFileName += m_sProjectName + DirSepChar;
  if (!m_sResName.empty())
    sCacheFileName += m_sResName + DirSepChar;

  if (!m_sLCode.empty())
    sCacheFileName += m_sLCode + DirSepChar;

  if (m_bDataFile)
    sCacheFileName += sCACHEDATADIRNAME + DirSepChar;

  sCacheFileName += m_sFileName;

  bCacheFileExists = g_File.FileExist(sCacheFileName);

  size_t CacheFileAge = bCacheFileExists ? g_File.GetFileAge(sCacheFileName): -1; //in seconds
  size_t MaxCacheFileAge = m_iHTTPCacheExp * 60; // in seconds

  bCacheFileExpired = CacheFileAge > MaxCacheFileAge;

  m_mapValidCacheFiles.insert(sCacheFileName);

  return sCacheFileName;
}

bool CHTTPHandler::CreateNewResource(const std::string& sPOFile, const CResData& ResData, size_t &iAddedNew)
{


  std::string sURLCreateRes = "https://www.transifex.com/api/2/project/" + ResData.UPD.ProjectName + "/resources/";

  std::string sURLSRCRes = "https://www.transifex.com/api/2/project/" + ResData.UPD.ProjectName + "/resource/" +
                           ResData.UPD.ResName + "/translation/" +
                           g_LCodeHandler.GetLangFromLCode(ResData.sSRCLCode, ResData.UPD.LForm) + "/";

  bool bCacheFileExists, bCacheFileExpired;

  std::string sCacheFileName = CreateCacheFilename(sURLSRCRes, bCacheFileExists, bCacheFileExpired);

  if (bCacheFileExpired)  //In case cachefile expired, delete it so forcing upload of current uploadable
  {
    g_File.DeleteFile(sCacheFileName);
    g_File.DeleteFile(sCacheFileName + ".time");
  }

  std::string sCacheFile;
  if (bCacheFileExists && !bCacheFileExpired)
    sCacheFile = g_File.ReadFileToStr(sCacheFileName);

  if (sCacheFile == sPOFile)
  {
    CLog::Log(logDEBUG, "HTTPHandler::PutFileToURL: not necesarry to upload file as it has not changed from last upload.");
    return true;
  }

  CURLcode curlResult;

  sURLCreateRes = URLEncode(sURLCreateRes);

  std::string strPOJson = CreateNewresJSONStrFromPOStr(ResData.UPD.ResName, sPOFile);

  std::string strServerResp;
  CLoginData LoginData = GetCredentials(sURLCreateRes);

  if(m_curlHandle)
  {
    int nretry = 0;
    bool bSuccess;
    long http_code = 0;
    do
    {
      Tputstrdata PutStrData;
      PutStrData.pPOString = &strPOJson;
      PutStrData.pos = 0;
      strServerResp.clear();

      if (nretry > 0)
        HTTPRetry(nretry);

      struct curl_slist *headers=NULL;
      headers = curl_slist_append( headers, "Content-Type: application/json");
      headers = curl_slist_append( headers, "charsets: utf-8");

      curl_easy_setopt(m_curlHandle, CURLOPT_READFUNCTION, Read_CurlData_String);
      curl_easy_setopt(m_curlHandle, CURLOPT_WRITEFUNCTION, Write_CurlData_String);
      curl_easy_setopt(m_curlHandle, CURLOPT_URL, sURLCreateRes.c_str());
      curl_easy_setopt(m_curlHandle, CURLOPT_POST, 1L);
      if (!LoginData.strLogin.empty())
      {
        curl_easy_setopt(m_curlHandle, CURLOPT_USERNAME, LoginData.strLogin.c_str());
        curl_easy_setopt(m_curlHandle, CURLOPT_PASSWORD, LoginData.strPassword.c_str());
      }
      curl_easy_setopt(m_curlHandle, CURLOPT_FAILONERROR, true);
      curl_easy_setopt(m_curlHandle, CURLOPT_READDATA, &PutStrData);
      curl_easy_setopt(m_curlHandle, CURLOPT_WRITEDATA, &strServerResp);
      curl_easy_setopt(m_curlHandle, CURLOPT_POSTFIELDSIZE_LARGE, (curl_off_t)strPOJson.size());
      curl_easy_setopt(m_curlHandle, CURLOPT_USERAGENT, sUSERAGENT.c_str());
      curl_easy_setopt(m_curlHandle, CURLOPT_HTTPHEADER, headers);
      curl_easy_setopt(m_curlHandle, CURLOPT_SSL_VERIFYPEER, 0);
      curl_easy_setopt(m_curlHandle, CURLOPT_SSL_VERIFYHOST, 0);
      curl_easy_setopt(m_curlHandle, CURLOPT_VERBOSE, 0);

      curlResult = curl_easy_perform(m_curlHandle);

      curl_easy_getinfo (m_curlHandle, CURLINFO_RESPONSE_CODE, &http_code);

      bSuccess = (curlResult == 0 && http_code >= 200 && http_code < 400);
      nretry++;
    }
    while (nretry < 5 && !bSuccess);

    if (bSuccess)
      CLog::Log(logDEBUG, "CHTTPHandler::CreateNewResource finished with success for resource %s from SRC PO file %s to URL %s",
                ResData.sResName.c_str(), sURLSRCRes.c_str(), sURLCreateRes.c_str());
    else
      CLog::Log(logERROR, "CHTTPHandler::CreateNewResource finished with error:\ncurl error: %i, %s\nhttp error: %i%s\nURL: %s\nlocaldir: %s\nREsource: %s",
                curlResult, curl_easy_strerror(curlResult), http_code, GetHTTPErrorFromCode(http_code).c_str(), sURLCreateRes.c_str(),
                sURLSRCRes.c_str(), ResData.sResName.c_str());

    if (!bCacheFileExists || bCacheFileExpired)
    {
      g_File.WriteFileFromStr(sCacheFileName, sPOFile);
      g_File.WriteNowToFileAgeFile(sCacheFileName);
    }

    ParseUploadedStrForNewRes(strServerResp, iAddedNew);

    return true;
  }
  else
    CLog::Log(logERROR, "CHTTPHandler::CreateNewResource failed because Curl was not initalized");

  return false;
};

void CHTTPHandler::GetGithubData (const std::string &strURL, CGithubURLData &GithubURLData)
{
  if (strURL.find("//") >> 7)
    CLog::Log(logERROR, "CHTTPHandler::ParseGitHUBURL: Internal error: // found in Github URL");

  size_t pos1, pos2, pos3, pos4, pos5;

  if (strURL.find("raw.github.com/") != std::string::npos)
    pos1 = strURL.find("raw.github.com/")+15;
  else if (strURL.find("raw2.github.com/") != std::string::npos)
    pos1 = strURL.find("raw2.github.com/")+16;
  else if (strURL.find("raw.githubusercontent.com/") != std::string::npos)
    pos1 = strURL.find("raw.githubusercontent.com/")+26;
  else
    CLog::Log(logERROR, "ResHandler: Wrong Github URL format given");

  pos2 = strURL.find("/", pos1+1);
  pos3 = strURL.find("/", pos2+1);
  pos4 = strURL.find("/", pos3+1);

  GithubURLData.strOwner = strURL.substr(pos1, pos2-pos1);
  GithubURLData.strRepo = strURL.substr(pos2, pos3-pos2);
  GithubURLData.strPath = strURL.substr(pos4, strURL.size() - pos4 - 1);
  GithubURLData.strGitBranch = strURL.substr(pos3+1, pos4-pos3-1);

  if ((pos5 = GithubURLData.strPath.find_last_of("(")) != std::string::npos)
  {
    GithubURLData.strPath = GithubURLData.strPath.substr(0,pos5);
    GithubURLData.strPath = GithubURLData.strPath.substr(0, GithubURLData.strPath.find_last_of("/"));
  }
}

std::string CHTTPHandler::GetGitHUBAPIURL(std::string const & strURL)
{
  CGithubURLData GithubURLData;
  GetGithubData(strURL, GithubURLData);

  std::string strGitHubURL = "https://api.github.com/repos/" + GithubURLData.strOwner + GithubURLData.strRepo;
  strGitHubURL += "/contents";
  strGitHubURL += GithubURLData.strPath;
  strGitHubURL += "?ref=" + GithubURLData.strGitBranch;

  return strGitHubURL;
}

void CHTTPHandler::GetGitCloneURL(std::string const & strURL, std::string &strGitHubURL, CGithubURLData &GithubURLData)
{
  GetGithubData(strURL, GithubURLData);
  strGitHubURL = "git@github.com:" + GithubURLData.strOwner + GithubURLData.strRepo + ".git";
}

bool CHTTPHandler::UploadTranslatorsDatabase(std::string strJson, std::string strURL)
{

  CURLcode curlResult;

  strURL = URLEncode(strURL);


  std::string strServerResp;
  CLoginData LoginData = GetCredentials(strURL);

  if(m_curlHandle) 
  {
    int nretry = 0;
    bool bSuccess;
    long http_code = 0;
    do
    {
      Tputstrdata PutStrData;
      PutStrData.pPOString = &strJson;
      PutStrData.pos = 0;
      strServerResp.clear();

      if (nretry > 0)
        HTTPRetry(nretry);

      struct curl_slist *headers=NULL;
      headers = curl_slist_append( headers, "Content-Type: application/json");
      headers = curl_slist_append( headers, "Accept: text/plain");

      curl_easy_setopt(m_curlHandle, CURLOPT_READFUNCTION, Read_CurlData_String);
      curl_easy_setopt(m_curlHandle, CURLOPT_WRITEFUNCTION, Write_CurlData_String);
      curl_easy_setopt(m_curlHandle, CURLOPT_URL, strURL.c_str());
      curl_easy_setopt(m_curlHandle, CURLOPT_POST, 1L);
      if (!LoginData.strLogin.empty())
      {
        curl_easy_setopt(m_curlHandle, CURLOPT_USERNAME, LoginData.strLogin.c_str());
        curl_easy_setopt(m_curlHandle, CURLOPT_PASSWORD, LoginData.strPassword.c_str());
      }
      curl_easy_setopt(m_curlHandle, CURLOPT_FAILONERROR, true);
      curl_easy_setopt(m_curlHandle, CURLOPT_READDATA, &PutStrData);
      curl_easy_setopt(m_curlHandle, CURLOPT_WRITEDATA, &strServerResp);
      curl_easy_setopt(m_curlHandle, CURLOPT_POSTFIELDSIZE_LARGE, (curl_off_t)strJson.size());
      curl_easy_setopt(m_curlHandle, CURLOPT_USERAGENT, sUSERAGENT.c_str());
      curl_easy_setopt(m_curlHandle, CURLOPT_HTTPHEADER, headers);
      curl_easy_setopt(m_curlHandle, CURLOPT_SSL_VERIFYPEER, 0);
      curl_easy_setopt(m_curlHandle, CURLOPT_SSL_VERIFYHOST, 0);
      curl_easy_setopt(m_curlHandle, CURLOPT_VERBOSE, 0);

      curlResult = curl_easy_perform(m_curlHandle);

      curl_easy_getinfo (m_curlHandle, CURLINFO_RESPONSE_CODE, &http_code);

      bSuccess = (curlResult == 0 && http_code >= 200 && http_code < 400);
      nretry++;
    }
    while (nretry < 5 && !bSuccess);

    return http_code;
  }
  else
    CLog::Log(logERROR, "CHTTPHandler::CreateNewResource failed because Curl was not initalized");
  return 700;
};

std::string CHTTPHandler::CreateNewresJSONStrFromPOStr(std::string strTXResname, std::string const &strPO)
{
  Json::Value root;
  root["content"] = strPO;
  root["slug"] = std::string(strTXResname);
  root["name"] = std::string(strTXResname);
  root["i18n_type"] = std::string("PO");
  Json::StyledWriter writer;
  std::string strJSON = writer.write(root);
  return strJSON;
};

void CHTTPHandler::ParseUploadedStringsData(std::string const &strJSON, size_t &stradded, size_t &strupd)
{
  Json::Value root;   // will contains the root value after parsing.
  Json::Reader reader;

  bool parsingSuccessful = reader.parse(strJSON, root );
  if ( !parsingSuccessful )
  {
    CLog::Log(logERROR, "JSONHandler::ParseUploadedStringsData: Parse upload tx server response: no valid JSON data");
    return;
  }

  stradded = root.get("strings_added", 0).asInt();
  strupd = root.get("strings_updated", 0).asInt();
  return;
};

void CHTTPHandler::ParseUploadedStrForNewRes(std::string const &strJSON, size_t &stradded)
{
  Json::Value root;   // will contains the root value after parsing.
  Json::Reader reader;

  bool parsingSuccessful = reader.parse(strJSON, root );
  if ( !parsingSuccessful )
  {
    CLog::Log(logERROR, "JSONHandler::ParseUploadedStrForNewRes: Parse upload tx server response: no valid JSON data");
    return;
  }

  stradded = root[0].asInt();

  return;
};

std::string CHTTPHandler::GetCurrentGitrevision(const std::string& sGitRootPath, const std::string& sBranch)
{
  return g_File.ReadFileToStr(sGitRootPath + ".git/refs/heads/" + sBranch);
}

std::string CHTTPHandler::GetCurrentGitBranch(const std::string& sGitRootPath)
{
  std::string sHead = g_File.ReadFileToStr(sGitRootPath + "/.git/HEAD");

  if (sHead.find("ref: refs/heads/") != 0) //Git repo is in a detached state
    return "";
  size_t posLF = sHead.find('\n');
  return sHead.substr(16, posLF-16);
}

void CHTTPHandler::GITPullUPSRepos(std::map<std::string, CBasicGITData>& MapGitRepos, bool bSkipGitReset)
{
  std::string sCommand;
  for (std::map<std::string, CBasicGITData>::iterator it = MapGitRepos.begin(); it != MapGitRepos.end(); it++)
  {
    CBasicGITData GitData = it->second;
    std::string sGitHubRootNoBranch = GitData.sUPSLocalPath + GitData.Owner + "/" + GitData.Repo;
    std::string sGitHubRoot = sGitHubRootNoBranch + "/" + GitData.Branch;
    if (!g_File.FileExist(sGitHubRoot +  "/.git/config"))
    {
      //no local directory present, cloning one
      CLog::Log(logPRINT, "\n");
      // clean directory if exists, unless git clone fails
      g_File.DeleteDirectory(sGitHubRoot);
      g_File.MakeDir(sGitHubRoot);

      sCommand = "cd " + sGitHubRootNoBranch + ";";
      sCommand += "git clone git@github.com:" + GitData.Owner + "/" + GitData.Repo + ".git " + GitData.Branch;
      CLog::Log(logPRINT, "%sGIT cloning with the following command:%s\n%s%s%s\n",KMAG, RESET, KORNG, sCommand.c_str(), RESET);
      g_File.SytemCommand(sCommand);

      sCommand = "cd " + sGitHubRoot + ";";
      sCommand += "git checkout " + GitData.Branch;
      CLog::Log(logPRINT, "%sGIT checkout branch: %s%s%s%s\n%s%s%s\n",KMAG, RESET, KCYN,GitData.Branch.c_str(), RESET, KORNG, sCommand.c_str(), RESET);
      g_File.SytemCommand(sCommand);
    }
    else if (!bSkipGitReset)
    {
      // We have an existing git repo. Let's clean it and update if necesarry
      CLog::Log(logPRINT, "\n");

      if (GitData.Branch != GetCurrentGitBranch(sGitHubRoot))
      {
        //Curent branch differs from the neede one, so check out the needed branch
        sCommand = "cd " + sGitHubRoot + ";";
        sCommand += "git checkout " + GitData.Branch;
        CLog::Log(logPRINT, "%sGIT checkout branch: %s%s%s%s\n%s%s%s\n",KMAG, RESET, KCYN,GitData.Branch.c_str(), RESET, KORNG, sCommand.c_str(), RESET);
        g_File.SytemCommand(sCommand);
      }

      sCommand = "cd " + sGitHubRoot + ";";
      sCommand += "git reset --hard origin/" + GitData.Branch;
      CLog::Log(logPRINT, "%sGIT reset existing local repo to branch: %s%s%s%s\n%s%s%s\n",KMAG, RESET, KCYN,GitData.Branch.c_str(), RESET, KORNG, sCommand.c_str(), RESET);
      g_File.SytemCommand(sCommand);

      sCommand = "cd " + sGitHubRoot + ";";
      sCommand += "git clean -f -d -x";
      CLog::Log(logPRINT, "%sRemove untracked files%s\n%s%s%s\n", KMAG, RESET, KORNG, sCommand.c_str(), RESET);
      g_File.SytemCommand(sCommand);

      if (g_File.GetAgeOfGitRepoPull(sGitHubRoot + "/.git/HEAD") > m_iHTTPCacheExp * 60)
      {
        //Git repo pull is outdated, make a fresh pull
        sCommand = "cd " + sGitHubRoot + ";";
        sCommand += "git pull";
        CLog::Log(logPRINT, "%sPull latest git changes%s\n%s%s%s\n", KMAG, RESET, KORNG, sCommand.c_str(), RESET);
        g_File.SytemCommand(sCommand);
      }
    }
  }
}


std::string CHTTPHandler::GetGithubPathToSTR(const std::string& sUPSLocalPath, const CGITData& GitData, const std::string& sPath, bool bForceGitDload)
{
  bool bCacheFileExists, bCacheFileExpired;
  std::string sGitHubRoot = GetGithubCloneRootPath(sUPSLocalPath, GitData);
  std::string sCacheFileName = CreateCacheFilenameGitSource(GitData.Branch, bCacheFileExists, bCacheFileExpired);
  m_mapValidCacheFiles.insert(sCacheFileName + ".version");
  m_mapValidCacheFiles.insert(sCacheFileName + ".time");

  if (bForceGitDload)
    bCacheFileExpired = true; //If we have this option enabled, we consider the cache always outdated

  std::string strBuffer, strCachedFileVersion, strWebFileVersion;

  strWebFileVersion = g_Fileversion.GetVersionForURL("git://" + GitData.Owner + "/" + GitData.Repo + "/" + GitData.Branch + "/" + sPath);

  if (strWebFileVersion == "")
    CLog::Log(logERROR, "HTTPHandler::GetGithubPathToSTR finished with error. File cannot be found: \n%s/%s/%s/%s\n", GitData.Owner.c_str(),
              GitData.Repo.c_str(), GitData.Branch.c_str(), sPath.c_str());

  if (strWebFileVersion != "" && g_File.FileExist(sCacheFileName + ".version"))
    strCachedFileVersion = g_File.ReadFileToStr(sCacheFileName + ".version");

  bool bFileChangedOnWeb = strCachedFileVersion != strWebFileVersion;

  if (!bCacheFileExists || (bCacheFileExpired && (strWebFileVersion == "" || bFileChangedOnWeb)))
  {
    CLog::Log(logPRINT, "%s*%s", KGRN, RESET);
    g_File.DeleteFile(sCacheFileName + ".version");
    g_File.DeleteFile(sCacheFileName + ".time");

    strBuffer = g_File.ReadFileToStr(sUPSLocalPath + GitData.Owner + "/" + GitData.Repo + "/" + GitData.Branch + "/" + sPath);

    if (!m_bSkipCache)
      g_File.WriteFileFromStr(sCacheFileName, strBuffer);

    if (strWebFileVersion != "")
      g_File.WriteFileFromStr(sCacheFileName + ".version", strWebFileVersion);

    g_File.WriteNowToFileAgeFile(sCacheFileName);
  }
  else
  {
    strBuffer = g_File.ReadFileToStr(sCacheFileName);
    if (bCacheFileExpired)
      CLog::Log(logPRINT, "%s-%s", KCYN, RESET);
    else
      CLog::Log(logPRINT, "%s.%s", KORNG, RESET);
  }

  return strBuffer;
}


std::string  CHTTPHandler::GetGitFileListToSTR(const std::string& sUPSLocalPath, const CGITData& GitData, bool bForceGitDload)
{
  bool bCacheFileExists, bCacheFileExpired;
  std::string sGitHubRoot = GetGithubCloneRootPath(sUPSLocalPath, GitData);
  std::string sCacheFileName = CreateCacheFilenameGitSource(GitData.Branch, bCacheFileExists, bCacheFileExpired);
  m_mapValidCacheFiles.insert(sCacheFileName + ".time");


  if (bForceGitDload)
    bCacheFileExpired = true; //If we have this option enabled, we consider the cache for the filelist file always outdated

  if (bCacheFileExpired || !bCacheFileExists)
    CLog::Log(logPRINT, "%s*%s", KGRN, RESET);
  else
    CLog::Log(logPRINT, "%s.%s", KORNG, RESET);

  std::string sGithubPathToList = "/" + g_CharsetUtils.GetRoot(GitData.AXMLPath, g_CharsetUtils.GetFilenameFromURL(GitData.AXMLPath));
  size_t pos;
  if ((pos = sGithubPathToList.find_last_of("(")) != std::string::npos)
  {
    //We have a language addon, so we list from one level higher than the addon.xml files
    sGithubPathToList = sGithubPathToList.substr(0,pos);
    sGithubPathToList = sGithubPathToList.substr(0, sGithubPathToList.find_last_of("/"));
  }

  if (sGithubPathToList.find_first_of('/') == 0)
    sGithubPathToList = sGithubPathToList.substr(1, sGithubPathToList.size()-1);

  std::string sCommand;

  g_File.MakeDir(g_CharsetUtils.GetRoot(sCacheFileName, g_CharsetUtils.GetFilenameFromURL(sCacheFileName)));

  if (!bCacheFileExists || bCacheFileExpired)
  {
    //Git file list for this directory is outdated, generate a fresh one to cache
    sCommand += "cd " + sGitHubRoot + ";";
    sCommand += "git ls-files -s " + sGithubPathToList + " > " + sCacheFileName;
    g_File.SytemCommand(sCommand);
  }
  return g_File.ReadFileToStr(sCacheFileName);
}

std::string CHTTPHandler::GetGithubCloneRootPath(const std::string& sUPSLocalPath, const CGITData& GitData)
{
  return sUPSLocalPath + GitData.Owner + "/" + GitData.Repo + "/" + GitData.Branch;
}

void CHTTPHandler::CleanCacheFiles()
{
  g_File.CleanDir(m_strCacheDir, true, m_mapValidCacheFiles);
  CLog::Log(logPRINT, "\033[s\033[K");
  if (g_File.GetCleanDirOutput().empty())
    CLog::Log(logPRINT, "No unecesarry files found.\n");
  else
    CLog::Log(logPRINT, "Cleaned files:\n%s", g_File.GetCleanDirOutput().c_str());
}