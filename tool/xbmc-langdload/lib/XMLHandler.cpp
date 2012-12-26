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

#include "XMLHandler.h"
#include "Log.h"
#include "HTTPUtils.h"
#include "FileUtils.h"
#include <list>

using namespace std;

CXMLResdata::CXMLResdata()
{
  Restype = UNKNOWN;
  bWritePO = true;
  bWriteXML = false;
  bHasChangelog = true;
  strLogFormat = "[B]%i[/B]\n\n- Updated language files from Transifex\n\n";
  strLogFilename = "changelog.txt";
  bSkipChangelog = false;
  bSkipEnglishFile = false;
}

CXMLResdata::~CXMLResdata()
{}

CInputData::CInputData()
{
  bSkipChangelog = false;
  bSkipEnglishFile =false;
}

CInputData::~CInputData()
{}

CUpdateXMLHandler::CUpdateXMLHandler()
{};

CUpdateXMLHandler::~CUpdateXMLHandler()
{};

bool CUpdateXMLHandler::DownloadXMLToMap (std::string strURL, std::map<std::string, CXMLResdata> &mapResourceData, std::string const &strTXProjectname)
{
  std::string strURLXMLFile = strURL + "xbmc-txupdate.xml";

  std::string strXMLFile = g_HTTPHandler.GetURLToSTR(strURLXMLFile);
  if (strXMLFile.empty())
    CLog::Log(logERROR, "CUpdateXMLHandler::DownloadXMLToMem: http error getting XML file from upstream url: %s", strURL.c_str());

  TiXmlDocument xmlUpdateXML;

  if (!xmlUpdateXML.Parse(strXMLFile.c_str(), 0, TIXML_DEFAULT_ENCODING))
  {
    CLog::Log(logERROR, "CUpdateXMLHandler::DownloadXMLToMem: UpdateXML file problem: %s %s\n", xmlUpdateXML.ErrorDesc(), strURL.c_str());
    return false;
  }

  TiXmlElement* pRootElement = xmlUpdateXML.RootElement();
  if (!pRootElement || pRootElement->NoChildren() || pRootElement->ValueTStr()!="resources")
  {
    CLog::Log(logERROR, "CUpdateXMLHandler::DownloadXMLToMem: No root element called \"resources\" in xml file. Cannot continue. Please create it");
    return false;
  }

  std::string strProjName;
  if (!pRootElement->Attribute("projectname") || (strProjName = pRootElement->Attribute("projectname")) == "")
    CLog::Log(logERROR, "CUpdateXMLHandler::DownloadXMLToMem: No projectname specified in xbmc-txupdate.xml file. Cannot continue. "
                        "Please contact TeamXBMC about this problem!");

    CLog::Log(logINFO, "Reading xbmc-txupdate.xml file for project: %s", strTXProjectname.c_str());

  std::string strMergedLangfileDir;
  if (!pRootElement->Attribute("merged_langfiledir") || (strMergedLangfileDir = pRootElement->Attribute("merged_langfiledir")) == "")
    strMergedLangfileDir = DEFAULTMERGEDLANGDIR;

  const TiXmlElement *pChildResElement = pRootElement->FirstChildElement("resource");
  if (!pChildResElement || pChildResElement->NoChildren())
  {
    CLog::Log(logERROR, "CUpdateXMLHandler::DownloadXMLToMem: No xml element called \"resource\" exists in the xml file. Please contact TeamXBMC about this problem!");
    return false;
  }

  std::string strType;
  while (pChildResElement && pChildResElement->FirstChild())
  {
    CXMLResdata currResData;
    currResData.strTranslationrepoURL = strURL;
    currResData.strProjName = strProjName;
    currResData.strMergedLangfileDir = strMergedLangfileDir;

    std::string strResName;
    if (!pChildResElement->Attribute("name") || (strResName = pChildResElement->Attribute("name")) == "")
    {
      CLog::Log(logERROR, "CUpdateXMLHandler::DownloadXMLToMem: No name specified for resource. Cannot continue. Please contact TeamXBMC about this problem!");
      return false;
    }

    currResData.strResName = strResName;

    if (pChildResElement->FirstChild())
    {
      const TiXmlElement *pChildURLElement = pChildResElement->FirstChildElement("upstreamURL");
      if (pChildURLElement && pChildURLElement->FirstChild())
        currResData.strUpstreamURL = pChildURLElement->FirstChild()->Value();
      if (currResData.strUpstreamURL.empty())
        CLog::Log(logERROR, "CUpdateXMLHandler::DownloadXMLToMem: UpstreamURL entry is empty or missing for resource %s", strResName.c_str());
      if (pChildURLElement->Attribute("filetype"))
        currResData.strLangFileType = pChildURLElement->Attribute("filetype"); // For PO no need to explicitly specify. Only for XML.
      if (pChildURLElement->Attribute("URLsuffix"))
        currResData.strURLSuffix = pChildURLElement->Attribute("URLsuffix"); // Some http servers need suffix strings after filename(eg. gitweb)
      if (pChildURLElement->Attribute("HasChangelog"))
      {
        std::string strHaschangelog = pChildURLElement->Attribute("HasChangelog"); // Note if the addon has upstream changelog
        currResData.bHasChangelog = strHaschangelog == "true";
      }
      if (pChildURLElement->Attribute("LogFormat"))
      {
        std::string strLogFormat = pChildURLElement->Attribute("LogFormat");
        currResData.strLogFormat = strLogFormat;
      }
      if (pChildURLElement->Attribute("LogFilename"))
      {
        std::string strLogFilename = pChildURLElement->Attribute("LogFilename");
        currResData.strLogFilename = strLogFilename;
      }

      const TiXmlElement *pChildUpstrLElement = pChildResElement->FirstChildElement("upstreamLangs");
      if (pChildUpstrLElement && pChildUpstrLElement->FirstChild())
        currResData.strLangsFromUpstream = pChildUpstrLElement->FirstChild()->Value();

      const TiXmlElement *pChildResTypeElement = pChildResElement->FirstChildElement("resourceType");
      if (pChildResTypeElement->Attribute("AddonXMLSuffix"))
        currResData.strAddonXMLSuffix = pChildResTypeElement->Attribute("AddonXMLSuffix"); // Some addons have unique addon.xml filename eg. pvr addons with .in suffix
      if (pChildResTypeElement && pChildResTypeElement->FirstChild())
      {
        strType = pChildResTypeElement->FirstChild()->Value();
        if (strType == "addon")
         currResData.Restype = ADDON;
        else if (strType == "addon_nostrings")
          currResData.Restype = ADDON_NOSTRINGS;
        else if (strType == "skin")
          currResData.Restype = SKIN;
        else if (strType == "xbmc_core")
          currResData.Restype = CORE;
      }
      if (currResData.Restype == UNKNOWN)
        CLog::Log(logERROR, "CUpdateXMLHandler::DownloadXMLToMem: Unknown type found or missing resourceType field for resource %s", strResName.c_str());

      const TiXmlElement *pChildResDirElement = pChildResElement->FirstChildElement("resourceSubdir");
      if (pChildResDirElement && pChildResDirElement->FirstChild())
        currResData.strResDirectory = pChildResDirElement->FirstChild()->Value();
      if (pChildResDirElement->Attribute("writePO"))
      {
	      std::string strBool = pChildResDirElement->Attribute("writePO");
        currResData.bWritePO = strBool == "true";
      }
      if (pChildResDirElement->Attribute("writeXML"))
      {
	      std::string strBool = pChildResDirElement->Attribute("writeXML");
        currResData.bWriteXML = strBool == "true";
      }
      if (pChildResDirElement->Attribute("DIRprefix"))
        currResData.strDIRprefix = pChildResDirElement->Attribute("DIRprefix"); // If there is any SUBdirectory needed in the tree

      const TiXmlElement *pChildTXNameElement = pChildResElement->FirstChildElement("TXname");
      if (pChildTXNameElement && pChildTXNameElement->FirstChild())
        currResData.strTXResName = pChildTXNameElement->FirstChild()->Value();
      if (currResData.strTXResName.empty())
        CLog::Log(logERROR, "CUpdateXMLHandler::DownloadXMLToMem: Transifex resource name is empty or missing for resource %s", strResName.c_str());

      currResData.strResNameFull = strTXProjectname + "/" + strResName;
      mapResourceData[currResData.strResNameFull] = currResData;
    }
    pChildResElement = pChildResElement->NextSiblingElement("resource");
  }

  return true;
};

CInputXMLHandler::CInputXMLHandler()
{}

CInputXMLHandler::~CInputXMLHandler()
{}

std::list<CInputData> CInputXMLHandler::ReadXMLToMem(string strFileName)
{
  std::string strXMLFile = g_File.ReadFileToStr(strFileName);
  if (strXMLFile.empty())
    CLog::Log(logERROR, "CInputXMLHandler::ReadXMLToMem: http error getting XML file from path: %s", strFileName.c_str());

  TiXmlDocument xmlUpdateXML;

  if (!xmlUpdateXML.Parse(strXMLFile.c_str(), 0, TIXML_DEFAULT_ENCODING))
    CLog::Log(logERROR, "CInputXMLHandler::ReadXMLToMem: UpdateXML file problem: %s %s\n", xmlUpdateXML.ErrorDesc(), strFileName.c_str());

  TiXmlElement* pRootElement = xmlUpdateXML.RootElement();
  if (!pRootElement || pRootElement->NoChildren() || pRootElement->ValueTStr()!="addonlist")
    CLog::Log(logERROR, "CInputXMLHandler::ReadXMLToMem: No root element called \"addonlist\" in xml file. Cannot continue. Please create it");

  const TiXmlElement *pChildResElement = pRootElement->FirstChildElement("addon");
  if (!pChildResElement || pChildResElement->NoChildren())
    CLog::Log(logERROR, "CInputXMLHandler::ReadXMLToMem: No xml element called \"addon\" exists in the xml file. Please contact TeamXBMC about this problem!");

  std::list<CInputData> listInputData;

  while (pChildResElement && pChildResElement->FirstChild())
  {
    CInputData currInputData;

    std::string strResName;
    if (!pChildResElement->Attribute("name") || (strResName = pChildResElement->Attribute("name")) == "")
      CLog::Log(logERROR, "CInputXMLHandler::ReadXMLToMem: No name specified for addon. Cannot continue.");

    currInputData.strAddonName = strResName;

    const TiXmlElement *pChildDirElement = pChildResElement->FirstChildElement("localdir");
    if (pChildDirElement && pChildDirElement->FirstChild())
      currInputData.strAddonDir = pChildDirElement->FirstChild()->Value();
    if (currInputData.strAddonDir.empty())
      CLog::Log(logERROR, "CInputXMLHandler::ReadXMLToMem: Local directory is missing for addon: %s", strResName.c_str());

    std::string strBool;
    const TiXmlElement *pChildSkipchlogElement = pChildResElement->FirstChildElement("skipchangelog");
    if (pChildSkipchlogElement && pChildSkipchlogElement->FirstChild())
      strBool = pChildSkipchlogElement->FirstChild()->Value();
    currInputData.bSkipChangelog = (strBool == "true");

    strBool.clear();
    const TiXmlElement *pChildSkipenglishElement = pChildResElement->FirstChildElement("skipenglish");
    if (pChildSkipenglishElement && pChildSkipenglishElement->FirstChild())
      strBool = pChildSkipenglishElement->FirstChild()->Value();
    currInputData.bSkipEnglishFile = (strBool == "true");

    const TiXmlElement *pChildGittemplElement = pChildResElement->FirstChildElement("gittemplate");
    if (pChildGittemplElement && pChildGittemplElement->FirstChild())
      currInputData.strGittemplate = pChildGittemplElement->FirstChild()->Value();

    listInputData.push_back(currInputData);

    pChildResElement = pChildResElement->NextSiblingElement("addon");
  }
  return listInputData;
}
