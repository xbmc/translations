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

#include "Log.h"
#include "HTTPUtils.h"
#include <curl/easy.h>
#include "FileUtils/FileUtils.h"
#include <cctype>
#include "Settings.h"
#include "JSONHandler.h"

CHTTPHandler g_HTTPHandler;

using namespace std;

CHTTPHandler::CHTTPHandler()
{
  m_curlHandle = curl_easy_init();
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
    printf (" Retry %i: %i  \b\b\b\b\b\b\b\b\b\b\b\b\b", nretry, nretry*6-i);
    if (nretry*6-i > 9)
      printf("\b");
    usleep(300000);
  }
  printf ("             \b\b\b\b\b\b\b\b\b\b\b\b\b");
  g_HTTPHandler.Cleanup();
  g_HTTPHandler.ReInit();
}

std::string CHTTPHandler::GetURLToSTR(std::string strURL)
{
  std::string strBuffer;
  std::string strCacheFile = CacheFileNameFromURL(strURL);
  strCacheFile = m_strCacheDir + "GET/" + strCacheFile;

  if (!g_File.FileExist(strCacheFile) || g_File.GetFileAge(strCacheFile) > g_Settings.GetHTTPCacheExpire() * 60)
  {
    long result = curlURLToCache(strCacheFile, strURL, strBuffer);
    if (result < 200 || result >= 400)
      return "";
  }
  else
    strBuffer = g_File.ReadFileToStr(strCacheFile);

  return strBuffer;
};

long CHTTPHandler::curlURLToCache(std::string strCacheFile, std::string strURL, std::string &strBuffer)
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
        curl_easy_setopt(m_curlHandle, CURLOPT_USERAGENT, strUserAgent.c_str());
        curl_easy_setopt(m_curlHandle, CURLOPT_SSL_VERIFYPEER, 0);
        curl_easy_setopt(m_curlHandle, CURLOPT_SSL_VERIFYHOST, 0);
        curl_easy_setopt(m_curlHandle, CURLOPT_VERBOSE, 0);
        curl_easy_setopt(m_curlHandle, CURLOPT_SSLVERSION, 3);
        curl_easy_setopt(m_curlHandle, CURLOPT_FOLLOWLOCATION, true);

        curlResult = curl_easy_perform(m_curlHandle);
        curl_easy_getinfo (m_curlHandle, CURLINFO_RESPONSE_CODE, &http_code);
        nretry++;
        bSuccess = (curlResult == 0 && http_code >= 200 && http_code < 400);
      }
      while (nretry < 5 && !bSuccess);

      if (bSuccess)
        CLog::Log(logINFO, "HTTPHandler: curlURLToCache finished with success from URL %s to cachefile %s, read filesize: %ibytes",
                  strURL.c_str(), strCacheFile.c_str(), strBuffer.size());
      else
        CLog::Log(logERROR, "HTTPHandler: curlURLToCache finished with error: \ncurl error: %i, %s\nhttp error: %i%s\nURL: %s\nlocaldir: %s",
                  curlResult, curl_easy_strerror(curlResult), http_code, GetHTTPErrorFromCode(http_code).c_str(),  strURL.c_str(), strCacheFile.c_str());

      g_File.WriteFileFromStr(strCacheFile, strBuffer);
      return http_code;
    }
    else
      CLog::Log(logERROR, "HTTPHandler: curlURLToCache failed because Curl was not initalized");
    return 0;
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

size_t Write_CurlData_File(void *ptr, size_t size, size_t nmemb, FILE *stream)
{
  size_t written = fwrite(ptr, size, nmemb, stream);
  return written;
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
  CLog::Log(logINFO, "HTTPHandler: Cache directory set to: %s", strCacheDir.c_str());
};

std::string CHTTPHandler::CacheFileNameFromURL(std::string strURL)
{
  std::string strResult;
  std::string strCharsToKeep  = "/.-=_() ";
  std::string strReplaceChars = "/.-=_() ";

  std::string hexChars = "01234567890abcdef"; 

  for (std::string::iterator it = strURL.begin(); it != strURL.end(); it++)
  {
    if (isalnum(*it))
      strResult += *it;
    else
    {
      size_t pos = strCharsToKeep.find(*it);
      if (pos != std::string::npos)
        strResult += strReplaceChars[pos];
      else
      {
        strResult += "%";
        strResult += hexChars[(*it >> 4) & 0x0F];
        strResult += hexChars[*it & 0x0F];
      }
    }
  }

  if (strResult.at(strResult.size()-1) == '/')
    strResult[strResult.size()-1] = '-';

  return strResult;
};

bool CHTTPHandler::LoadCredentials (std::string CredentialsFilename)
{
  TiXmlDocument xmlPasswords;

  if (!xmlPasswords.LoadFile(CredentialsFilename.c_str()))
  {
    CLog::Log(logINFO, "HTTPHandler: No \".passwords.xml\" file exists in project dir. No password protected web download will be available.");
    return false;
  }

  CLog::Log(logINFO, "HTTPHandler: Succesfuly found the .passwsords.xml file: %s", CredentialsFilename.c_str());

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
      CLog::Log(logINFO, "HTTPHandler: found login credentials for website prefix: %s", strWebSitePrefix.c_str());
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



bool CHTTPHandler::PutFileToURL(std::string const &strFilePath, std::string const &strURL, bool &buploaded,
                                size_t &stradded, size_t &strupd)
{
  std::string strBuffer;
  std::string strCacheFile = CacheFileNameFromURL(strURL);
  strCacheFile = m_strCacheDir + "PUT/" + strCacheFile;

  if (g_File.FileExist(strCacheFile) && ComparePOFiles(strCacheFile, strFilePath))
  {
    CLog::Log(logINFO, "HTTPHandler::PutFileToURL: not necesarry to upload file as it has not changed from last upload. File: %s",
              strFilePath.c_str());
    return true;
  }

  CLog::Log(logINFO, "HTTPHandler::PutFileToURL: Uploading file to Transifex: %s", strFilePath.c_str());

  long result = curlPUTPOFileToURL(strFilePath, strURL, stradded, strupd);
  if (result < 200 || result >= 400)
  {
    CLog::Log(logERROR, "HTTPHandler::PutFileToURL: File upload was unsuccessful, http errorcode: %i", result);
    return false;
  }

  CLog::Log(logINFO, "HTTPHandler::PutFileToURL: File upload was successful so creating a copy at the .httpcache directory");
  g_File.CopyFile(strFilePath, strCacheFile);

  buploaded = true;

  return true;
};

long CHTTPHandler::curlPUTPOFileToURL(std::string const &strFilePath, std::string const &cstrURL, size_t &stradded, size_t &strupd)
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
                  CURLFORM_FILE, strFilePath.c_str(),
                  CURLFORM_CONTENTTYPE, "application/octet-stream",
                  CURLFORM_END);

      curl_easy_setopt(m_curlHandle, CURLOPT_WRITEFUNCTION, Write_CurlData_String);
      curl_easy_setopt(m_curlHandle, CURLOPT_URL, strURL.c_str());
      curl_easy_setopt(m_curlHandle, CURLOPT_NOPROGRESS, 1L);
      curl_easy_setopt(m_curlHandle, CURLOPT_HEADER, 1L);
      curl_easy_setopt(m_curlHandle, CURLOPT_FOLLOWLOCATION, 1L);
      curl_easy_setopt(m_curlHandle, CURLOPT_HTTPPOST, post1);
      curl_easy_setopt(m_curlHandle, CURLOPT_USERAGENT, strUserAgent.c_str());
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
      curl_easy_setopt(m_curlHandle, CURLOPT_SSLVERSION, 3);

      curlResult = curl_easy_perform(m_curlHandle);

      curl_easy_getinfo (m_curlHandle, CURLINFO_RESPONSE_CODE, &http_code);

      curl_formfree(post1);
      post1 = NULL;

      bSuccess = (curlResult == 0 && http_code >= 200 && http_code < 400);
      nretry++;
    }
    while (nretry < 5 && !bSuccess);

    if (bSuccess)
      CLog::Log(logINFO, "HTTPHandler::curlFileToURL finished with success from File %s to URL %s",
                strFilePath.c_str(), strURL.c_str());
    else
      CLog::Log(logERROR, "HTTPHandler::curlFileToURL finished with error: \ncurl error: %i, %s\nhttp error: %i%s\nURL: %s\nlocaldir: %s",
                curlResult, curl_easy_strerror(curlResult), http_code, GetHTTPErrorFromCode(http_code).c_str(), strURL.c_str(), strFilePath.c_str());

    size_t jsonPos = strServerResp.find_first_of("{");
    if (jsonPos == std::string::npos)
      CLog::Log(logERROR, "HTTPHandler::curlFileToURL no valid Transifex server response received");

    strServerResp = strServerResp.substr(jsonPos);
    g_Json.ParseUploadedStringsData(strServerResp, stradded, strupd);

    return http_code;
  }
  else
    CLog::Log(logERROR, "HTTPHandler::curlFileToURL failed because Curl was not initalized");
  return 700;
};

bool CHTTPHandler::ComparePOFiles(std::string strPOFilePath1, std::string strPOFilePath2) const
{
  CPOHandler POHandler1, POHandler2;
  POHandler1.ParsePOStrToMem(g_File.ReadFileToStr(strPOFilePath1), strPOFilePath1);
  POHandler2.ParsePOStrToMem(g_File.ReadFileToStr(strPOFilePath2), strPOFilePath2);
  return ComparePOFilesInMem(&POHandler1, &POHandler2, false);
}

bool CHTTPHandler::ComparePOFilesInMem(CPOHandler * pPOHandler1, CPOHandler * pPOHandler2, bool bLangIsEN) const
{
  if (!pPOHandler1 || !pPOHandler2)
    return false;
  if (pPOHandler1->GetNumEntriesCount() != pPOHandler2->GetNumEntriesCount())
    return false;
  if (pPOHandler1->GetClassEntriesCount() != pPOHandler2->GetClassEntriesCount())
    return false;

  for (size_t POEntryIdx = 0; POEntryIdx != pPOHandler1->GetNumEntriesCount(); POEntryIdx++)
  {
    CPOEntry POEntry1 = *(pPOHandler1->GetNumPOEntryByIdx(POEntryIdx));
    const CPOEntry * pPOEntry2 = pPOHandler2->GetNumPOEntryByID(POEntry1.numID);
    if (!pPOEntry2)
      return false;
    CPOEntry POEntry2 = *pPOEntry2;

    if (bLangIsEN)
    {
      POEntry1.msgStr.clear();
      POEntry1.msgStrPlural.clear();
      POEntry2.msgStr.clear();
      POEntry2.msgStrPlural.clear();
    }

    if (!(POEntry1 == POEntry2))
      return false;
  }

  for (size_t POEntryIdx = 0; POEntryIdx != pPOHandler1->GetClassEntriesCount(); POEntryIdx++)
  {
    const CPOEntry * POEntry1 = pPOHandler1->GetClassicPOEntryByIdx(POEntryIdx);
    CPOEntry POEntryToFind = *POEntry1;
    if (!pPOHandler2->LookforClassicEntry(POEntryToFind))
      return false;
  }
  return true;
}

bool CHTTPHandler::CreateNewResource(std::string strResname, std::string strENPOFilePath, std::string strURL, size_t &stradded,
                                     std::string const &strURLENTransl)
{
  std::string strCacheFile = CacheFileNameFromURL(strURLENTransl);
  strCacheFile = m_strCacheDir + "PUT/" + strCacheFile;

  CURLcode curlResult;

  strURL = URLEncode(strURL);

  std::string strPO = g_File.ReadFileToStr(strENPOFilePath);

  std::string strPOJson = g_Json.CreateNewresJSONStrFromPOStr(strResname, strPO);

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
      curl_easy_setopt(m_curlHandle, CURLOPT_POSTFIELDSIZE_LARGE, (curl_off_t)strPOJson.size());
      curl_easy_setopt(m_curlHandle, CURLOPT_USERAGENT, strUserAgent.c_str());
      curl_easy_setopt(m_curlHandle, CURLOPT_HTTPHEADER, headers);
      curl_easy_setopt(m_curlHandle, CURLOPT_SSL_VERIFYPEER, 0);
      curl_easy_setopt(m_curlHandle, CURLOPT_SSL_VERIFYHOST, 0);
      curl_easy_setopt(m_curlHandle, CURLOPT_VERBOSE, 0);
      curl_easy_setopt(m_curlHandle, CURLOPT_SSLVERSION, 3);

      curlResult = curl_easy_perform(m_curlHandle);

      curl_easy_getinfo (m_curlHandle, CURLINFO_RESPONSE_CODE, &http_code);

      bSuccess = (curlResult == 0 && http_code >= 200 && http_code < 400);
      nretry++;
    }
    while (nretry < 5 && !bSuccess);

    if (bSuccess)
      CLog::Log(logINFO, "CHTTPHandler::CreateNewResource finished with success for resource %s from EN PO file %s to URL %s",
                strResname.c_str(), strENPOFilePath.c_str(), strURL.c_str());
    else
      CLog::Log(logERROR, "CHTTPHandler::CreateNewResource finished with error:\ncurl error: %i, %s\nhttp error: %i%s\nURL: %s\nlocaldir: %s\nREsource: %s",
                curlResult, curl_easy_strerror(curlResult), http_code, GetHTTPErrorFromCode(http_code).c_str(), strURL.c_str(), strENPOFilePath.c_str(), strResname.c_str());

    g_File.CopyFile(strENPOFilePath, strCacheFile);
    g_Json.ParseUploadedStrForNewRes(strServerResp, stradded);

    return http_code;
  }
  else
    CLog::Log(logERROR, "CHTTPHandler::CreateNewResource failed because Curl was not initalized");
  return 700;
};

void CHTTPHandler::DeleteCachedFile (std::string const &strURL, std::string strPrefix)
{
  std::string strCacheFile = CacheFileNameFromURL(strURL);

  strCacheFile = m_strCacheDir + strPrefix + "/" + strCacheFile;
  if (g_File.FileExist(strCacheFile))
    g_File.DeleteFile(strCacheFile);
}
