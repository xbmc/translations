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

#include <list>
#include <algorithm>
#include "ResourceHandler.h"
#include "JSONHandler.h"
#include "HTTPUtils.h"
#include "Log.h"
#include "FileUtils.h"

using namespace std;

CResourceHandler::CResourceHandler()
{};

CResourceHandler::~CResourceHandler()
{};

bool CResourceHandler::DloadLangFiles(CXMLResdata &XMLResdata)
{
  g_HTTPHandler.Cleanup();
  g_HTTPHandler.ReInit();

  std::string strLogMessage = "DOWNLOADING RESOURCE: " + XMLResdata.strResNameFull + " FROM XBMC REPO";
  std::string strLogHeader;
  strLogHeader.resize(strLogMessage.size(), '*');
  CLog::Log(logLINEFEED, "");
  CLog::Log(logINFO, "%s", strLogHeader.c_str());
  CLog::Log(logINFO, "%s", strLogMessage.c_str());
  CLog::Log(logINFO, "%s", strLogHeader.c_str());
  CLog::IncIdent(2);

  if (XMLResdata.Restype != CORE)
  {
    std::string strDloadURL = XMLResdata.strTranslationrepoURL;
    g_HTTPHandler.AddToURL(strDloadURL, XMLResdata.strMergedLangfileDir);
    g_HTTPHandler.AddToURL(strDloadURL, XMLResdata.strResDirectory);
    g_HTTPHandler.AddToURL(strDloadURL, XMLResdata.strResName);
    g_HTTPHandler.AddToURL(strDloadURL, XMLResdata.strDIRprefix);
    g_HTTPHandler.AddToURL(strDloadURL, "addon.xml");
    strDloadURL += XMLResdata.strAddonXMLSuffix;

    std::string strFilename = XMLResdata.strResLocalDirectory;
    g_File.AddToFilename(strFilename, "addon.xml");
    strFilename += XMLResdata.strAddonXMLSuffix;

    std::string strAddonXMLFile = g_HTTPHandler.GetURLToSTR(strDloadURL);
    if (!XMLResdata.strGittemplate.empty())
      XMLResdata.strAddonVersion = GetAddonVersion(strAddonXMLFile);
    g_File.WriteFileFromStr(strFilename, strAddonXMLFile);

    CLog::Log(logINFO, "ResHandler: addon.xml downloaded for resource: %s",XMLResdata.strResNameFull.c_str());
  }

  if (!XMLResdata.bSkipChangelog && XMLResdata.bHasChangelog && XMLResdata.Restype != CORE)
  {
    std::string strDloadURL = XMLResdata.strTranslationrepoURL;
    g_HTTPHandler.AddToURL(strDloadURL, XMLResdata.strMergedLangfileDir);
    g_HTTPHandler.AddToURL(strDloadURL, XMLResdata.strResDirectory);
    g_HTTPHandler.AddToURL(strDloadURL, XMLResdata.strResName);
    g_HTTPHandler.AddToURL(strDloadURL, XMLResdata.strDIRprefix);
    g_HTTPHandler.AddToURL(strDloadURL, XMLResdata.strLogFilename);

    std::string strFilename = XMLResdata.strResLocalDirectory;
    g_File.AddToFilename(strFilename, XMLResdata.strLogFilename);

    g_HTTPHandler.DloadURLToFile(strDloadURL, strFilename);
    CLog::Log(logINFO, "ResHandler: changelog.txt downloaded for resource: %s",XMLResdata.strResNameFull.c_str());
  }

  if (XMLResdata.Restype == ADDON_NOSTRINGS)
  {
    CLog::DecIdent(2);
    return true;
  }

  std::list<std::string> listLangs;

  std::string strtemp = g_HTTPHandler.GetURLToSTR(g_HTTPHandler.GetGithubAPIURL(XMLResdata));

  if (strtemp.empty())
    CLog::Log(logERROR, "ResHandler::DloadLangFiles: error getting langfile list from xbmc translation github repo");

  listLangs = g_Json.ParseAvailDirsGITHUB(strtemp);

  std::string strDloadURLPre = XMLResdata.strTranslationrepoURL;
  g_HTTPHandler.AddToURL(strDloadURLPre, XMLResdata.strMergedLangfileDir);
  g_HTTPHandler.AddToURL(strDloadURLPre, XMLResdata.strResDirectory);
  if (XMLResdata.Restype != CORE)
    g_HTTPHandler.AddToURL(strDloadURLPre, XMLResdata.strResName);
  g_HTTPHandler.AddToURL(strDloadURLPre, XMLResdata.strDIRprefix);
  g_HTTPHandler.AddToURL(strDloadURLPre, GetLangURLSuffix(XMLResdata));

  std::string strFilenamePre = XMLResdata.strResLocalDirectory;
  g_File.AddToFilename(strFilenamePre, GetLangDir(XMLResdata));

  CLog::Log(logINFO, "ResHandler: Downloading language files:");

  for (std::list<std::string>::iterator it = listLangs.begin(); it != listLangs.end(); it++)
  {
    if (XMLResdata.bSkipEnglishFile && *it == "English")
      continue;

    printf (" %s", it->c_str());

    if (XMLResdata.bWriteXML)
    {
      std::string strDloadURL = strDloadURLPre;
      g_HTTPHandler.AddToURL(strDloadURL, *it);
      g_HTTPHandler.AddToURL(strDloadURL, "strings.xml");

      std::string strFilename = strFilenamePre;
      g_File.AddToFilename(strFilename, *it);
      g_File.AddToFilename(strFilename, "strings.xml");

      g_HTTPHandler.DloadURLToFile(strDloadURL, strFilename);   
    }
    else if (XMLResdata.bWritePO)
    {
      std::string strDloadURL = strDloadURLPre;
      g_HTTPHandler.AddToURL(strDloadURL, *it);
      g_HTTPHandler.AddToURL(strDloadURL, "strings.po");

      std::string strFilename = strFilenamePre;
      g_File.AddToFilename(strFilename, *it);
      g_File.AddToFilename(strFilename, "strings.po");

      g_HTTPHandler.DloadURLToFile(strDloadURL, strFilename);   
    }
  }
  int langcount = listLangs.size();
  if (XMLResdata.bSkipEnglishFile)
    langcount--;
  printf ("\n\n");
  CLog::Log(logINFO, "ResHandler: %i language files were downloaded for resource: %s",langcount, XMLResdata.strResName.c_str());
  CLog::DecIdent(2);

  return true;
}

std::string CResourceHandler::GetLangDir(CXMLResdata const &XMLResdata)
{
  std::string strLangDir;
  switch (XMLResdata.Restype)
  {
    case ADDON: case ADDON_NOSTRINGS:
      g_File.AddToFilename(strLangDir, "resources");
      g_File.AddToFilename(strLangDir, "language");
      break;
    case SKIN:
      g_File.AddToFilename(strLangDir, "language");
      break;
    case CORE:
      g_File.AddToFilename(strLangDir, "language");
      break;
    default:
      CLog::Log(logERROR, "ResHandler: No resourcetype defined for resource: %s",XMLResdata.strResName.c_str());
  }
  return strLangDir;
}

std::string CResourceHandler::GetLangURLSuffix(CXMLResdata const &XMLResdata)
{
  std::string strLangURLSuffix;
  switch (XMLResdata.Restype)
  {
    case ADDON:
      strLangURLSuffix = "resources/language/";
      break;
    case SKIN:
      strLangURLSuffix = "language/";
      break;
    case CORE:
      strLangURLSuffix = "language/";
      break;
    default:
      CLog::Log(logERROR, "ResHandler: No resourcetype defined for resource: %s",XMLResdata.strResName.c_str());
  }
  return strLangURLSuffix;
}

std::string CResourceHandler::GetAddonVersion(std::string const &strAddonXMLFile)
{
  if (strAddonXMLFile.empty())
    CLog::Log(logERROR, "CResourceHandler::GetAddonVersion: Error reading the addon.xml file. File is empty.");

  TiXmlDocument xmlAddonXML;

  if (!xmlAddonXML.Parse(strAddonXMLFile.c_str(), 0, TIXML_DEFAULT_ENCODING))
    CLog::Log(logERROR, "CResourceHandler::GetAddonVersion: XML file problem: %s\n", xmlAddonXML.ErrorDesc());

  TiXmlElement* pRootElement = xmlAddonXML.RootElement();
  if (!pRootElement || pRootElement->NoChildren() || pRootElement->ValueTStr()!="addon")
    CLog::Log(logERROR, "CResourceHandler::GetAddonVersion: No root element called \"addon\" in xml file. Cannot continue.");

  std::string strAddonVersion;
  if (!pRootElement->Attribute("version") || (strAddonVersion = pRootElement->Attribute("version")) == "")
    CLog::Log(logERROR, "CResourceHandler::GetAddonVersion: No addon version is specified in the addon.xml file. Cannot continue. "
    "Please contact the addon developer about this problem!");

  return strAddonVersion;
}