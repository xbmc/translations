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
#include <cctype>
#include "FileUtils.h"

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

void CHTTPHandler::DloadURLToFile(std::string strURL, std::string strFilename)
{
 g_File.WriteFileFromStr(strFilename, GetURLToSTR(strURL)); 
}

std::string CHTTPHandler::GetURLToSTR(std::string strURL, bool bSkiperror /*=false*/)
{
  std::string strBuffer;
  strURL = URLEncode(strURL);

  CURLcode curlResult;

    if(m_curlHandle) 
    {
      curl_easy_setopt(m_curlHandle, CURLOPT_URL, strURL.c_str());
      curl_easy_setopt(m_curlHandle, CURLOPT_WRITEFUNCTION, Write_CurlData_String);
      curl_easy_setopt(m_curlHandle, CURLOPT_WRITEDATA, &strBuffer);
      curl_easy_setopt(m_curlHandle, CURLOPT_FAILONERROR, true);
      curl_easy_setopt(m_curlHandle, CURLOPT_USERAGENT, "libcurl-agent/1.0");
      curl_easy_setopt(m_curlHandle, CURLOPT_SSL_VERIFYPEER, 0);
      curl_easy_setopt(m_curlHandle, CURLOPT_FOLLOWLOCATION, true);

      curlResult = curl_easy_perform(m_curlHandle);
      long http_code = 0;
      curl_easy_getinfo (m_curlHandle, CURLINFO_RESPONSE_CODE, &http_code);

      if (curlResult != 0 || http_code < 200 || http_code >= 400 || strBuffer.empty())
      {
        if (!bSkiperror)
        CLog::Log(logERROR, "HTTPHandler:curlURLToStr finished with error code: %i from URL %s",
                  http_code, strURL.c_str());
        return "";
      }

      return strBuffer;
    }
    else
      CLog::Log(logERROR, "HTTPHandler:curlURLToStr failed because Curl was not initalized");

    return "";
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

void CHTTPHandler::AddToURL (std::string &strURL, std::string strAddendum)
{
  if (strAddendum.empty())
    return;

  if (strURL.find_last_of("/") != strURL.size()-1)
    strURL += "/";
  if (strAddendum.find_first_of("/") == 0)
    strAddendum = strAddendum.substr(1);
  if (strAddendum.find_last_of("/") == strAddendum.size()-1)
    strAddendum = strAddendum.substr(0, strAddendum.size()-1);
  strURL += strAddendum;
}

std::string CHTTPHandler::GetGithubAPIURL (CXMLResdata const &XMLResdata)
{
  std::string strGitHubURL, strGitBranch;
  strGitHubURL = XMLResdata.strTranslationrepoURL;
  GetGithubAPIURLFromURL(strGitHubURL, strGitBranch);

  AddToURL(strGitHubURL, XMLResdata.strMergedLangfileDir);
  AddToURL(strGitHubURL, XMLResdata.strResDirectory);
  if (XMLResdata.Restype != CORE)
    AddToURL(strGitHubURL, XMLResdata.strResName);
  AddToURL(strGitHubURL, XMLResdata.strDIRprefix);

  if (XMLResdata.Restype == SKIN || XMLResdata.Restype == CORE)
    AddToURL(strGitHubURL, "language");
  else if (XMLResdata.Restype == ADDON)
    AddToURL(strGitHubURL, "resources/language");

  strGitHubURL += "?ref=" + strGitBranch;
  return strGitHubURL;
}

void CHTTPHandler::GetGithubAPIURLFromURL (std::string &strUrl, std::string &strGitBranch)
{
  size_t pos1, pos2, pos3;
  std::string strGitHubURL;
  if (strUrl.find("raw.github.com/") == std::string::npos)
    CLog::Log(logERROR, "ResHandler: Wrong Github URL format");
  pos1 = strUrl.find("raw.github.com/")+15;
  pos2 = strUrl.find("/", pos1+1);
  pos2 = strUrl.find("/", pos2+1);
  pos3 = strUrl.find("/", pos2+1);
  strGitHubURL = "https://api.github.com/repos/" + strUrl.substr(pos1, pos2-pos1);
  strGitHubURL += "/contents";
  strGitHubURL += strUrl.substr(pos3, strUrl.size() - pos3 - 1);
  strGitBranch = strUrl.substr(pos2+1, pos3-pos2-1);
  strUrl = strGitHubURL;
}
