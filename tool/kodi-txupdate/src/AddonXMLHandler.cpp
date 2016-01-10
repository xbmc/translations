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

#include "AddonXMLHandler.h"
#include <list>
#include <set>
#include <vector>
#include <algorithm>
#include "HTTPUtils.h"
#include "ConfigHandler.h"
#include "FileUtils.h"
#include "CharsetUtils.h"
#include "Log.h"
#include "Langcodes.h"
#include "Fileversioning.h"
#include "jsoncpp/json/json.h"

using namespace std;

CAddonXMLHandler::CAddonXMLHandler()
{
  m_bBumpAddoXMLVersion = false;
};

CAddonXMLHandler::~CAddonXMLHandler()
{};

void CAddonXMLHandler::FetchAddonDataFiles()
{
  std::string sGitHubURL, sTemp;

  if (m_ResData.bIsLangAddon)
    return; // kodi language-addons have individual addon.xml files

  CLog::Log(logPRINT, " Addxml");
  FetchAddonXMLFileUpstr();
  if (!m_ResData.sChgLogFormat.empty())
  {
    CLog::Log(logPRINT, " Chlog");
    FetchAddonChangelogFile();
  }
}

bool CAddonXMLHandler::FetchAddonXMLFileUpstr ()
{
  g_HTTPHandler.SetFileName("addon.xml");
  g_HTTPHandler.SetDataFile(false);

  TiXmlDocument xmlAddonXML;

  std::string sAXMLFile = g_HTTPHandler.GetGithubPathToSTR (m_ResData.sUPSLocalPath, m_ResData.UPS, m_ResData.UPS.AXMLPath, m_ResData.bForceGitDloadToCache);

  if (sAXMLFile.empty())
    CLog::Log(logERROR, "CAddonXMLHandler::FetchAddonXMLFileUpstr: http error getting XML file from upstream url: %s", m_ResData.UPS.AXMLPath.c_str());

  g_File.ConvertStrLineEnds(sAXMLFile);

  m_strAddonXMLFile = sAXMLFile.substr(0,sAXMLFile.find_last_of(">")+1) + "\n";

  size_t pos = m_strAddonXMLFile.find("library_@PLATFORM@=\"@LIBRARY_FILENAME@\""); //In some addon.xml.in files we have this symbol which is not a valid xml tag
  if (pos != std::string::npos)
    sAXMLFile = m_strAddonXMLFile.substr(0,pos) + m_strAddonXMLFile.substr(pos+39);

  if (!xmlAddonXML.Parse(sAXMLFile.c_str(), 0, TIXML_DEFAULT_ENCODING))
  {
    CLog::Log(logERROR, "AddonXMLHandler: AddonXML file problem: %s %s\n", xmlAddonXML.ErrorDesc(), m_ResData.UPS.AXMLPath.c_str());
    return false;
  }

  std::string addonXMLEncoding;
  m_strResourceData.clear();

  GetEncoding(&xmlAddonXML, addonXMLEncoding);

  TiXmlElement* pRootElement = xmlAddonXML.RootElement();

  if (!pRootElement || pRootElement->NoChildren() || pRootElement->ValueTStr()!="addon")
  {
    CLog::Log(logERROR, "AddonXMLHandler: No root element called: \"addon\" or no child found in AddonXML file: %s\n",
            m_ResData.UPS.AXMLPath.c_str());
    return false;
  }
  const char* pMainAttrId = NULL;

  pMainAttrId=pRootElement->Attribute("name");
  m_strResourceData += "# Addon Name: ";
  if (!pMainAttrId)
  {
    CLog::Log(logWARNING, "AddonXMLHandler: No addon name was available in addon.xml file: %s\n", m_ResData.UPS.AXMLPath.c_str());
    m_strResourceData += "kodi-unnamed\n";
  }
  else
    m_strResourceData += g_CharsetUtils.ToUTF8(addonXMLEncoding, CstrToString(pMainAttrId)) + "\n";

  pMainAttrId=pRootElement->Attribute("id");
  m_strResourceData += "# Addon id: ";
  if (!pMainAttrId)
  {
    CLog::Log(logWARNING, "AddonXMLHandler: No addon name was available in addon.xml file: %s\n", m_ResData.UPS.AXMLPath.c_str());
    m_strResourceData +=  "unknown\n";
  }
  else
    m_strResourceData += g_CharsetUtils.ToUTF8(addonXMLEncoding, CstrToString(pMainAttrId)) + "\n";

  pMainAttrId=pRootElement->Attribute("version");
  if (!pMainAttrId)
  {
    CLog::Log(logWARNING, "AddonXMLHandler: No version name was available in addon.xml file: %s\n", m_ResData.UPS.AXMLPath.c_str());
    m_strAddonVersion = "0.0.1";
  }
  else
  {
    m_strAddonVersion = g_CharsetUtils.ToUTF8(addonXMLEncoding, CstrToString(pMainAttrId));
    BumpVersionNumber();
  }

  pMainAttrId=pRootElement->Attribute("provider-name");
  m_strResourceData += "# Addon Provider: ";
  if (!pMainAttrId)
  {
    CLog::Log(logWARNING, "AddonXMLHandler: Warning: No addon provider was available in addon.xml file: %s\n", m_ResData.UPS.AXMLPath.c_str());
    m_strResourceData += "unknown\n";
  }
  else
    m_strResourceData += g_CharsetUtils.ToUTF8(addonXMLEncoding, CstrToString(pMainAttrId)) + "\n";

  std::string strAttrToSearch = "xbmc.addon.metadata";

  const TiXmlElement *pChildElement = pRootElement->FirstChildElement("extension");
  while (pChildElement && strcmp(pChildElement->Attribute("point"), "xbmc.addon.metadata") != 0)
    pChildElement = pChildElement->NextSiblingElement("extension");

  const TiXmlElement *pChildSummElement = pChildElement->FirstChildElement("summary");
  while (pChildSummElement && pChildSummElement->FirstChild())
  {
    std::string strAlias, strLCode;
    if (pChildSummElement->Attribute("lang"))
      strAlias = pChildSummElement->Attribute("lang");
    else
      strAlias = g_LCodeHandler.GetLangFromLCode(m_ResData.sSRCLCode, m_ResData.UPS.LFormInAXML);
    strLCode = g_LCodeHandler.GetLangCodeFromAlias(strAlias, m_ResData.UPS.LFormInAXML);

    if (pChildSummElement->FirstChild() && strLCode != "")
    {
      std::string strValue = CstrToString(pChildSummElement->FirstChild()->Value());
            strValue = g_CharsetUtils.ToUTF8(addonXMLEncoding, strValue);
      if (m_ResData.bRebrand)
        g_CharsetUtils.reBrandXBMCToKodi(&strValue);
      m_mapAddonXMLData[strLCode].strSummary = strValue;
    }
    pChildSummElement = pChildSummElement->NextSiblingElement("summary");
  }

  const TiXmlElement *pChildDescElement = pChildElement->FirstChildElement("description");
  while (pChildDescElement && pChildDescElement->FirstChild())
  {
    std::string strLCode, strAlias;
    if (pChildDescElement->Attribute("lang"))
      strAlias = pChildDescElement->Attribute("lang");
    else
      strAlias = g_LCodeHandler.GetLangFromLCode(m_ResData.sSRCLCode, m_ResData.UPS.LFormInAXML);
    strLCode = g_LCodeHandler.GetLangCodeFromAlias(strAlias, m_ResData.UPS.LFormInAXML);

    if (pChildDescElement->FirstChild() && strLCode != "")
    {
      std::string strValue = CstrToString(pChildDescElement->FirstChild()->Value());
      strValue = g_CharsetUtils.ToUTF8(addonXMLEncoding, strValue);
      if (m_ResData.bRebrand)
        g_CharsetUtils.reBrandXBMCToKodi(&strValue);
      m_mapAddonXMLData[strLCode].strDescription = strValue;
    }
    pChildDescElement = pChildDescElement->NextSiblingElement("description");
  }

  const TiXmlElement *pChildDisclElement = pChildElement->FirstChildElement("disclaimer");
  while (pChildDisclElement && pChildDisclElement->FirstChild())
  {
    std::string strLCode, strAlias;
    if (pChildDisclElement->Attribute("lang"))
      strAlias = pChildDisclElement->Attribute("lang");
    else
      strAlias = g_LCodeHandler.GetLangFromLCode(m_ResData.sSRCLCode, m_ResData.UPS.LFormInAXML);
    strLCode = g_LCodeHandler.GetLangCodeFromAlias(strAlias, m_ResData.UPS.LFormInAXML);

    if (pChildDisclElement->FirstChild() && strLCode != "")
    {
      std::string strValue = CstrToString(pChildDisclElement->FirstChild()->Value());
      strValue = g_CharsetUtils.ToUTF8(addonXMLEncoding, strValue);
      if (m_ResData.bRebrand)
        g_CharsetUtils.reBrandXBMCToKodi(&strValue);
      m_mapAddonXMLData[strLCode].strDisclaimer = strValue;
    }
    pChildDisclElement = pChildDisclElement->NextSiblingElement("disclaimer");
  }

  const TiXmlElement *pChildLangElement = pChildElement->FirstChildElement("language");
  if (pChildLangElement)
  {
    if (pChildLangElement->FirstChild())
      m_AddonMetadata.strLanguage = pChildLangElement->FirstChild()->Value();
    else
      m_AddonMetadata.strLanguage = " ";
  }

  const TiXmlElement *pChildPlatfElement = pChildElement->FirstChildElement("platform");
  if (pChildPlatfElement)
  {
    if (pChildPlatfElement->FirstChild())
      m_AddonMetadata.strPlatform = pChildPlatfElement->FirstChild()->Value();
    else
      m_AddonMetadata.strPlatform = " ";
  }

  const TiXmlElement *pChildLicElement = pChildElement->FirstChildElement("license");
  if (pChildLicElement)
  {
    if (pChildLicElement->FirstChild())
      m_AddonMetadata.strLicense = pChildLicElement->FirstChild()->Value();
    else
      m_AddonMetadata.strLicense = " ";
  }

  const TiXmlElement *pChildForElement = pChildElement->FirstChildElement("forum");
  if (pChildForElement)
  {
    if (pChildForElement->FirstChild())
      m_AddonMetadata.strForum = pChildForElement->FirstChild()->Value();
    else
      m_AddonMetadata.strForum = " ";
  }

  const TiXmlElement *pChildWebElement = pChildElement->FirstChildElement("website");
  if (pChildWebElement)
  {
    if (pChildWebElement->FirstChild())
      m_AddonMetadata.strWebsite = pChildWebElement->FirstChild()->Value();
    else
      m_AddonMetadata.strWebsite = " ";
  }

  const TiXmlElement *pChildEmlElement = pChildElement->FirstChildElement("email");
  if (pChildEmlElement)
  {
    if (pChildEmlElement->FirstChild())
      m_AddonMetadata.strEmail = pChildEmlElement->FirstChild()->Value();
    else
      m_AddonMetadata.strEmail = " ";
  }

  const TiXmlElement *pChildSrcElement = pChildElement->FirstChildElement("source");
  if (pChildSrcElement)
  {
    if (pChildSrcElement->FirstChild())
      m_AddonMetadata.strSource = pChildSrcElement->FirstChild()->Value();
    else
      m_AddonMetadata.strSource = " ";
  }

  return true;
};

void CAddonXMLHandler::AddAddonXMLLangsToList(std::set<std::string>& listLangs)
{
  for (T_itAddonXMLData it = m_mapAddonXMLData.begin(); it != m_mapAddonXMLData.end(); it++)
  {
    listLangs.insert(it->first);
  }
}

void CAddonXMLHandler::WriteAddonXMLFile (std::string strAddonXMLFilename)
{
  g_File.WriteFileFromStr(strAddonXMLFilename, m_strAddonXMLFile.c_str());
}

void CAddonXMLHandler::GenerateAddonXMLFile ()
{
  if (m_ResData.bSkipVersionBump)
    sleep (1);

  if (m_bBumpAddoXMLVersion && !m_ResData.bSkipVersionBump)
    UpdateVersionNumber();

  std::string strXMLEntry;
  size_t posS1, posE1, posS2, posE2;
  posE1 = 0; posS1 =0;

  do
  {
    posS1 = posE1;
    strXMLEntry = GetXMLEntry("<extension", posS1, posE1);
    if (posS1 == std::string::npos)
      CLog::Log(logERROR, "AddonXMLHandler: UpdateAddonXML file generate problem.\n");
  }
  while (strXMLEntry.find("point") == std::string::npos || strXMLEntry.find("xbmc.addon.metadata") == std::string::npos);

  posS2 = posE1+1;
  GetXMLEntry("</extension", posS2, posE2);
  if (posS2 == std::string::npos)
  CLog::Log(logERROR, "AddonXMLHandler: UpdateAddonXML file generate problem.\n");

  size_t posMetaDataStart = posE1 +1;
  size_t posMetaDataEnd = m_strAddonXMLFile.find_last_not_of("\t ", posS2-1);

  std::string strPrevMetaData = m_strAddonXMLFile.substr(posMetaDataStart, posMetaDataEnd-posMetaDataStart+1);
  std::string strAllign = m_strAddonXMLFile.substr(m_strAddonXMLFile.find_first_not_of("\n\r", posMetaDataStart),
                                                   m_strAddonXMLFile.find("<",posMetaDataStart) - 
                                                   m_strAddonXMLFile.find_first_not_of("\n\r", posMetaDataStart));

  std::list<std::string> listAddonDataLangs;

  for (T_itAddonXMLData itAddonXMLData = m_mapAddonXMLData.begin(); itAddonXMLData != m_mapAddonXMLData.end(); itAddonXMLData++)
    listAddonDataLangs.push_back(g_LCodeHandler.GetLangFromLCode(itAddonXMLData->first, m_ResData.LOC.LFormInAXML));

  listAddonDataLangs.sort();

  std::string strNewMetadata;
  strNewMetadata += "\n";

  for (std::list<std::string>::iterator it = listAddonDataLangs.begin(); it != listAddonDataLangs.end(); it++)
  {
    if (!m_mapAddonXMLData[g_LCodeHandler.GetLangCodeFromAlias(*it, m_ResData.LOC.LFormInAXML)].strSummary.empty())
      strNewMetadata += strAllign + "<summary lang=\"" + *it + "\">" + 
                        g_CharsetUtils.EscapeStringXML(m_mapAddonXMLData[g_LCodeHandler.GetLangCodeFromAlias(*it, m_ResData.LOC.LFormInAXML)].strSummary) + "</summary>\n";
  }
  for (std::list<std::string>::iterator it = listAddonDataLangs.begin(); it != listAddonDataLangs.end(); it++)
  {
    if (!m_mapAddonXMLData[g_LCodeHandler.GetLangCodeFromAlias(*it, m_ResData.LOC.LFormInAXML)].strDescription.empty())
      strNewMetadata += strAllign + "<description lang=\"" + *it + "\">" +
                        g_CharsetUtils.EscapeStringXML(m_mapAddonXMLData[g_LCodeHandler.GetLangCodeFromAlias(*it, m_ResData.LOC.LFormInAXML)].strDescription) + "</description>\n";
  }
  for (std::list<std::string>::iterator it = listAddonDataLangs.begin(); it != listAddonDataLangs.end(); it++)
  {
    if (!m_mapAddonXMLData[g_LCodeHandler.GetLangCodeFromAlias(*it, m_ResData.LOC.LFormInAXML)].strDisclaimer.empty())
      strNewMetadata += strAllign + "<disclaimer lang=\"" + *it + "\">" +
                        g_CharsetUtils.EscapeStringXML(m_mapAddonXMLData[g_LCodeHandler.GetLangCodeFromAlias(*it, m_ResData.LOC.LFormInAXML)].strDisclaimer) + "</disclaimer>\n";
  }

  if (!m_AddonMetadata.strLanguage.empty())
  {
    if (m_AddonMetadata.strLanguage.compare(" ") == 0) m_AddonMetadata.strLanguage.clear();
    strNewMetadata += strAllign + "<language>" + m_AddonMetadata.strLanguage + "</language>\n";
  }
  if (!m_AddonMetadata.strPlatform.empty())
  {
    if (m_AddonMetadata.strPlatform.compare(" ") ==0) m_AddonMetadata.strPlatform.clear();
    strNewMetadata += strAllign + "<platform>" + m_AddonMetadata.strPlatform + "</platform>\n";
  }
  if (!m_AddonMetadata.strLicense.empty())
  {
    if (m_AddonMetadata.strLicense.compare(" ") ==0) m_AddonMetadata.strLicense.clear();
    strNewMetadata += strAllign + "<license>" + m_AddonMetadata.strLicense + "</license>\n";
  }
  if (!m_AddonMetadata.strForum.empty())
  {
    if (m_AddonMetadata.strForum.compare(" ") == 0) m_AddonMetadata.strForum.clear();
    strNewMetadata += strAllign + "<forum>" + m_AddonMetadata.strForum + "</forum>\n";
  }
  if (!m_AddonMetadata.strWebsite.empty())
  {
    if (m_AddonMetadata.strWebsite.compare(" ") == 0) m_AddonMetadata.strWebsite.clear();
    strNewMetadata += strAllign + "<website>" + m_AddonMetadata.strWebsite + "</website>\n";
  }
  if (!m_AddonMetadata.strEmail.empty())
  {
    if (m_AddonMetadata.strEmail .compare(" ") == 0) m_AddonMetadata.strEmail.clear();
    strNewMetadata += strAllign + "<email>" + m_AddonMetadata.strEmail + "</email>\n";
  }
  if (!m_AddonMetadata.strSource.empty())
  {
    if (m_AddonMetadata.strSource.compare(" ") ==0 ) m_AddonMetadata.strSource.clear();
    strNewMetadata += strAllign + "<source>" + m_AddonMetadata.strSource + "</source>\n";
  }

  m_strAddonXMLFile.replace(posMetaDataStart, posMetaDataEnd -posMetaDataStart +1, strNewMetadata);
}

std::string CAddonXMLHandler::GetXMLEntry (std::string const &strprefix, size_t &pos1, size_t &pos2)
{
  pos1 =   m_strAddonXMLFile.find(strprefix, pos1);
  pos2 =   m_strAddonXMLFile.find(">", pos1);
  return m_strAddonXMLFile.substr(pos1, pos2 - pos1 +1);
}

void CAddonXMLHandler::GenerateChangelogFile (std::string strFormat)
{
  size_t pos1;
  if ((pos1 = strFormat.find("%i")) != std::string::npos)
    strFormat.replace(pos1, 2, m_strAddonVersion.c_str());
  if ((pos1 = strFormat.find("%d")) != std::string::npos)
    strFormat.replace(pos1, 2, g_File.GetCurrDay().c_str());
  if ((pos1 = strFormat.find("%m")) != std::string::npos)
    strFormat.replace(pos1, 2, g_File.GetCurrMonth().c_str());
  if ((pos1 = strFormat.find("%y")) != std::string::npos)
    strFormat.replace(pos1, 2, g_File.GetCurrYear().c_str());
  if ((pos1 = strFormat.find("%M")) != std::string::npos)
    strFormat.replace(pos1, 2, g_File.GetCurrMonthText().c_str());

  if  (m_bBumpAddoXMLVersion)
    m_strChangelogFile = strFormat + m_strChangelogFile;

  return;
}

bool CAddonXMLHandler::WriteAddonChangelogFile (const std::string& strFilename)
{
  return g_File.WriteFileFromStr(strFilename, m_strChangelogFile.c_str());
}

bool CAddonXMLHandler::FetchAddonChangelogFile ()
{
  g_HTTPHandler.SetFileName("changelog.txt");
  g_HTTPHandler.SetDataFile(false);

  std::string strChangelogFile = g_HTTPHandler.GetGithubPathToSTR (m_ResData.sUPSLocalPath, m_ResData.UPS, m_ResData.UPS.ChLogPath, m_ResData.bForceGitDloadToCache);

  g_File.ConvertStrLineEnds(strChangelogFile);

  m_strChangelogFile.swap(strChangelogFile);
  return true;
}

void CAddonXMLHandler::BumpVersionNumber()
{
  size_t posLastDot = m_strAddonVersion.find_last_of(".");
  if (posLastDot == string::npos)
  {
    CLog::Log(logWARNING, "AddonXMLHandler::BumpVersionNumber: Wrong Version format, skipping versionbump");
    return;
  }
  std::string strLastNumber = m_strAddonVersion.substr(posLastDot+1);
  if (strLastNumber.find_first_not_of("0123456789") != std::string::npos)
  {
    CLog::Log(logWARNING, "AddonXMLHandler::BumpVersionNumber: Wrong Version format, skipping versionbump");
    return;
  }
  int LastNum = atoi (strLastNumber.c_str());
  strLastNumber = g_CharsetUtils.IntToStr(LastNum +1);
  m_strAddonVersion = m_strAddonVersion.substr(0, posLastDot +1) +strLastNumber;
}

void CAddonXMLHandler::UpdateVersionNumber()
{
  size_t Pos1, Pos2;
  Pos1 = 0;
  std::string strAddonBasicData = GetXMLEntry("<addon", Pos1, Pos2);
  if (strAddonBasicData.empty())
  {
    CLog::Log(logERROR, "AddonXMLHandler::UpdateVersionNumber: Wrong addon.xml file format.");
    return;
  }
  size_t PosSub1, PosSub2;
  PosSub1 = strAddonBasicData.find("version=");
  if (PosSub1 == std::string::npos)
  {
    CLog::Log(logERROR, "AddonXMLHandler::UpdateVersionNumber: Wrong addon.xml file format.");
    return;
  }
  PosSub2 = strAddonBasicData.find_first_of("\"'", PosSub1+10);
  if (PosSub2 == std::string::npos)
  {
    CLog::Log(logERROR, "AddonXMLHandler::UpdateVersionNumber: Wrong addon.xml file format.");
    return;
  }
  m_strAddonXMLFile.replace(Pos1 + PosSub1 + 9, PosSub2 - PosSub1 - 9, m_strAddonVersion.c_str());
}

bool CAddonXMLHandler::GetEncoding(const TiXmlDocument* pDoc, std::string& strEncoding)
{
  const TiXmlNode* pNode=NULL;
  while ((pNode=pDoc->IterateChildren(pNode)) && pNode->Type()!=TiXmlNode::TINYXML_DECLARATION) {}
  if (!pNode) return false;
  const TiXmlDeclaration* pDecl=pNode->ToDeclaration();
  if (!pDecl) return false;
  strEncoding=pDecl->Encoding();
  if (strEncoding.compare("UTF-8") ==0 || strEncoding.compare("UTF8") == 0 ||
    strEncoding.compare("utf-8") ==0 || strEncoding.compare("utf8") == 0)
    strEncoding.clear();
  std::transform(strEncoding.begin(), strEncoding.end(), strEncoding.begin(), ::toupper);
  return !strEncoding.empty(); // Other encoding then UTF8?
};

std::string CAddonXMLHandler::CstrToString(const char * StrToConv)
{
  std::string strIN(StrToConv);
  return strIN;
}
