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

#include "POHandler.h"
#include "HTTPUtils.h"
#include <algorithm>
#include <list>
#include <sstream>
#include "Settings.h"

CPOHandler::CPOHandler()
{
  m_bIsXMLSource = false;
};

CPOHandler::~CPOHandler()
{};

bool CPOHandler::FetchPOURLToMem (std::string strURL, bool bSkipError)
{
  CPODocument PODoc;
  if (!PODoc.FetchURLToMem(strURL, bSkipError))
    return false;
  return ProcessPOFile(PODoc);
};

bool CPOHandler::ParsePOStrToMem (std::string const &strPOData, std::string const &strFilePath)
{
  CPODocument PODoc;
  if (!PODoc.ParseStrToMem(strPOData, strFilePath))
    return false;
  return ProcessPOFile(PODoc);
};

bool CPOHandler::ProcessPOFile(CPODocument &PODoc)
{
  if (PODoc.GetEntryType() != HEADER_FOUND)
    CLog::Log(logERROR, "POHandler: No valid header found for this language");

  m_strHeader = PODoc.GetEntryData().Content.substr(1);

  m_mapStrings.clear();
  m_vecClassicEntries.clear();
  m_CommsCntr = 0;

  bool bMultipleComment = false;
  std::vector<std::string> vecILCommnts;
  CPOEntry currEntry;
  int currType = UNKNOWN_FOUND;

  while ((PODoc.GetNextEntry(false)))
  {
    PODoc.ParseEntry();
    currEntry = PODoc.GetEntryData();
    currType = PODoc.GetEntryType();

    if (currType == COMMENT_ENTRY_FOUND)
    {
      if (!vecILCommnts.empty())
        bMultipleComment = true;
      vecILCommnts = currEntry.interlineComm;
      if (!bMultipleComment && !vecILCommnts.empty())
        m_CommsCntr++;
      continue;
    }

    if (currType == ID_FOUND || currType == MSGID_FOUND || currType == MSGID_PLURAL_FOUND)
    {
      if (bMultipleComment)
        CLog::Log(logWARNING, "POHandler: multiple comment entries found. Using only the last one "
        "before the real entry. Entry after comments: %s", currEntry.Content.c_str());
      if (!currEntry.interlineComm.empty())
        CLog::Log(logWARNING, "POParser: interline comments (eg. #comment) is not alowed inside "
        "a real po entry. Cleaned it. Problematic entry: %s", currEntry.Content.c_str());
      currEntry.interlineComm = vecILCommnts;
      bMultipleComment = false;
      vecILCommnts.clear();

      if (currType == ID_FOUND)
        m_mapStrings[currEntry.numID] = currEntry;
      else
      {
        m_vecClassicEntries.push_back(currEntry);
      }
      ClearCPOEntry(currEntry);
    }
  }

  return true;
};

void CPOHandler::ClearCPOEntry (CPOEntry &entry)
{
  entry.msgStrPlural.clear();
  entry.referenceComm.clear();
  entry.extractedComm.clear();
  entry.translatorComm.clear();
  entry.interlineComm.clear();
  entry.numID = 0;
  entry.msgID.clear();
  entry.msgStr.clear();
  entry.msgIDPlur.clear();
  entry.msgCtxt.clear();
  entry.Type = UNKNOWN_FOUND;
  entry.Content.clear();
};


bool CPOHandler::GetXMLEncoding( const TiXmlDocument* pDoc, std::string& strEncoding)
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
}

void CPOHandler::GetXMLComment(std::string strXMLEncoding, const TiXmlNode *pCommentNode, CPOEntry &currEntry)
{
  int nodeType;
  CPOEntry prevCommEntry;
  while (pCommentNode)
  {
    nodeType = pCommentNode->Type();
    if (nodeType == TiXmlNode::TINYXML_ELEMENT)
      break;
    if (nodeType == TiXmlNode::TINYXML_COMMENT)
    {
      if (pCommentNode->m_CommentLFPassed)
        prevCommEntry.interlineComm.push_back(g_CharsetUtils.ToUTF8(strXMLEncoding, g_CharsetUtils.UnWhitespace(pCommentNode->Value())));
      else
        currEntry.extractedComm.push_back(g_CharsetUtils.ToUTF8(strXMLEncoding, g_CharsetUtils.UnWhitespace(pCommentNode->Value())));
    }
    pCommentNode = pCommentNode->NextSibling();
  }
  currEntry.interlineComm = m_prevCommEntry.interlineComm;
  m_prevCommEntry.interlineComm = prevCommEntry.interlineComm;

  if (currEntry.interlineComm.size() != 0)
    m_CommsCntr++;
}

bool CPOHandler::FetchXMLURLToMem (std::string strURL)
{
  std::string strXMLBuffer = g_HTTPHandler.GetURLToSTR(strURL);
  if (strXMLBuffer.empty())
    CLog::Log(logERROR, "CPOHandler::FetchXMLURLToMem: http error reading XML file from url: %s", strURL.c_str());

  m_bIsXMLSource = true;
  m_CommsCntr = 0;
  TiXmlDocument XMLDoc;

  strXMLBuffer.push_back('\n'); // with some addon.xml files, EOF mark is missing. That gives us a TinyXML failure. To avoid, we add an LF
  g_File.ConvertStrLineEnds(strXMLBuffer);
  strXMLBuffer += "\n";

  if (!XMLDoc.Parse(strXMLBuffer.c_str(), 0, TIXML_DEFAULT_ENCODING))
  {
    CLog::Log(logERROR, "CPOHandler::FetchXMLURLToMem: strings.xml file problem: %s %s\n", XMLDoc.ErrorDesc(), strURL.c_str());
    return false;
  }

  std::string strXMLEncoding;
  GetXMLEncoding(&XMLDoc, strXMLEncoding);

  TiXmlElement* pRootElement = XMLDoc.RootElement();
  if (!pRootElement || pRootElement->NoChildren() || pRootElement->ValueTStr()!="strings")
  {
    CLog::Log(logERROR, "CPOHandler::FetchXMLURLToMem: No root element called: \"strings\" or no child found in input XML file: %s", strURL.c_str());
    return false;
  }

  CPOEntry currEntry, commHolder, prevcommHolder;

  if (m_bPOIsEnglish)
    GetXMLComment(strXMLEncoding, pRootElement->FirstChild(), currEntry);

  const TiXmlElement *pChildElement = pRootElement->FirstChildElement("string");
  const char* pAttrId = NULL;
  const char* pValue = NULL;
  std::string valueString;
  int id;

  while (pChildElement)
  {
    pAttrId=pChildElement->Attribute("id");
    if (pAttrId && !pChildElement->NoChildren())
    {
      id = atoi(pAttrId);
      if (m_mapStrings.find(id) == m_mapStrings.end())
      {
        currEntry.Type = ID_FOUND;
        pValue = pChildElement->FirstChild()->Value();
        valueString = pValue;
        currEntry.numID = id;
        std::string strUtf8 = g_CharsetUtils.ToUTF8(strXMLEncoding, valueString).c_str();

        if (m_bPOIsEnglish)
          currEntry.msgID = strUtf8;
        else
          currEntry.msgStr = strUtf8;

        if (m_bPOIsEnglish)
          GetXMLComment(strXMLEncoding, pChildElement->NextSibling(), currEntry);

        m_mapStrings[id] = currEntry;
      }
    }
    ClearCPOEntry(currEntry);

    pChildElement = pChildElement->NextSiblingElement("string");
  }
  // Free up the allocated memory for the XML file
  XMLDoc.Clear();
  return true;
}


bool CPOHandler::WritePOFile(const std::string &strOutputPOFilename)
{
  CPODocument PODoc;

  PODoc.SetIfIsEnglish(m_bPOIsEnglish);

  PODoc.WriteHeader(m_strHeader);

  CPOEntry POEntry;
  POEntry.msgCtxt = "Addon Summary";
  if (LookforClassicEntry(POEntry))
    PODoc.WritePOEntry(POEntry);

  ClearCPOEntry(POEntry);
  POEntry.msgCtxt = "Addon Description";
  if (LookforClassicEntry(POEntry))
    PODoc.WritePOEntry(POEntry);

  ClearCPOEntry(POEntry);
  POEntry.msgCtxt = "Addon Disclaimer";
  if (LookforClassicEntry(POEntry))
    PODoc.WritePOEntry(POEntry);

  for (itStrings it = m_mapStrings.begin(); it != m_mapStrings.end(); it++)
  {
    CPOEntry currEntry = it->second;
    PODoc.WritePOEntry(currEntry);
  }

  PODoc.SaveFile(strOutputPOFilename);

  return true;
};

// Data manipulation functions

bool CPOHandler::LookforClassicEntry (CPOEntry &EntryToFind)
{
  for (itClassicEntries it = m_vecClassicEntries.begin(); it != m_vecClassicEntries.end(); it++)
  {
    if (*it == EntryToFind)
    {
      EntryToFind = *it;
      return true;
    }
  }
  return false;
}

void CPOHandler::AddClassicEntry (CPOEntry &EntryToAdd)
{
  m_vecClassicEntries.push_back(EntryToAdd);
};

bool CPOHandler::ModifyClassicEntry (CPOEntry &EntryToFind, CPOEntry EntryNewValue)
{
  for (itClassicEntries it = m_vecClassicEntries.begin(); it != m_vecClassicEntries.end(); it++)
  {
    if (*it == EntryToFind)
    {
      *it = EntryNewValue;
      return true;
    }
  }
  m_vecClassicEntries.push_back(EntryNewValue);
  return false;
}

bool CPOHandler::DeleteClassicEntry (CPOEntry &EntryToFind)
{
  for (itClassicEntries it = m_vecClassicEntries.begin(); it != m_vecClassicEntries.end(); it++)
  {
    if (*it == EntryToFind)
    {
      m_vecClassicEntries.erase(it);
      return true;
    }
  }
  return false;
}

void CPOHandler::SetAddonMetaData (CAddonXMLEntry const &AddonXMLEntry, CAddonXMLEntry const &PrevAddonXMLEntry,
                                   CAddonXMLEntry const &AddonXMLEntryEN, std::string const &strLang)
{
  CPOEntry POEntryDesc, POEntryDiscl, POEntrySumm;
  POEntryDesc.Type = MSGID_FOUND;
  POEntryDiscl.Type = MSGID_FOUND;
  POEntrySumm.Type = MSGID_FOUND;
  POEntryDesc.msgCtxt = "Addon Description";
  POEntryDiscl.msgCtxt = "Addon Disclaimer";
  POEntrySumm.msgCtxt = "Addon Summary";

  CPOEntry newPOEntryDesc = POEntryDesc;
  CPOEntry newPOEntryDisc = POEntryDiscl;
  CPOEntry newPOEntrySumm = POEntrySumm;

  newPOEntryDesc.msgID = AddonXMLEntryEN.strDescription;
  newPOEntryDisc.msgID = AddonXMLEntryEN.strDisclaimer;
  newPOEntrySumm.msgID = AddonXMLEntryEN.strSummary;

  if (strLang != "en")
  {
    if (!AddonXMLEntry.strDescription.empty() && AddonXMLEntry.strDescription != PrevAddonXMLEntry.strDescription)
      newPOEntryDesc.msgStr = AddonXMLEntry.strDescription;
    if (!AddonXMLEntry.strDisclaimer.empty() && AddonXMLEntry.strDisclaimer != PrevAddonXMLEntry.strDisclaimer)
      newPOEntryDisc.msgStr = AddonXMLEntry.strDisclaimer;
    if (!AddonXMLEntry.strSummary.empty() && AddonXMLEntry.strSummary != PrevAddonXMLEntry.strSummary)
      newPOEntrySumm.msgStr = AddonXMLEntry.strSummary;
  }

  if (!newPOEntryDesc.msgID.empty() && (strLang == "en" || !newPOEntryDesc.msgStr.empty()))
    ModifyClassicEntry(POEntryDesc, newPOEntryDesc);
  if (!newPOEntryDisc.msgID.empty() && (strLang == "en" || !newPOEntryDisc.msgStr.empty()))
    ModifyClassicEntry(POEntryDiscl, newPOEntryDisc);
  if (!newPOEntrySumm.msgID.empty() && (strLang == "en" || !newPOEntrySumm.msgStr.empty()))
    ModifyClassicEntry(POEntrySumm, newPOEntrySumm);
  return;
}

void CPOHandler::GetAddonMetaData (CAddonXMLEntry &AddonXMLEntry, CAddonXMLEntry &AddonXMLEntryEN)
{
  CAddonXMLEntry newAddonXMLEntry, newENAddonXMLEntry;
  CPOEntry POEntry;
  POEntry.msgCtxt = "Addon Summary";
  if (LookforClassicEntry(POEntry))
  {
    newAddonXMLEntry.strSummary = POEntry.msgStr;
    newENAddonXMLEntry.strSummary = POEntry.msgID;
  }

  ClearCPOEntry(POEntry);
  POEntry.msgCtxt = "Addon Description";
  if (LookforClassicEntry(POEntry))
  {
    newAddonXMLEntry.strDescription = POEntry.msgStr;
    newENAddonXMLEntry.strDescription = POEntry.msgID;
  }

  ClearCPOEntry(POEntry);
  POEntry.msgCtxt = "Addon Disclaimer";
  if (LookforClassicEntry(POEntry))
  {
    newAddonXMLEntry.strDisclaimer = POEntry.msgStr;
    newENAddonXMLEntry.strDisclaimer = POEntry.msgID;
  }
  AddonXMLEntry = newAddonXMLEntry;
  AddonXMLEntryEN = newENAddonXMLEntry;
  return;
}

void CPOHandler::SetPreHeader (std::string &strPreText)
{
  if (strPreText.empty())
    return;

  m_strHeader = "# XBMC Media Center language file\n";
  m_strHeader += strPreText;
}

void CPOHandler::SetHeaderNEW (std::string strLangCode)
{
  m_strLangCode = strLangCode;
  std::stringstream ss;//create a stringstream
  ss << g_LCodeHandler.GetnPlurals(strLangCode);
  std::string strnplurals = ss.str();

  m_strHeader += "msgid \"\"\n";
  m_strHeader += "msgstr \"\"\n";
  m_strHeader += "\"Project-Id-Version: " + g_Settings.GetProjectnameLong() + "\\n\"\n";
  m_strHeader += "\"Report-Msgid-Bugs-To: " + g_Settings.GetSupportEmailAdd() + "\\n\"\n";
  m_strHeader += "\"POT-Creation-Date: YEAR-MO-DA HO:MI+ZONE\\n\"\n";
  m_strHeader += "\"PO-Revision-Date: YEAR-MO-DA HO:MI+ZONE\\n\"\n";
  m_strHeader += "\"Last-Translator: XBMC Translation Team\\n\"\n";
  m_strHeader += "\"Language-Team: " + g_LCodeHandler.FindLang(strLangCode) + " (http://www.transifex.com/projects/p/" + g_Settings.GetProjectname() +"/language/"
                 + strLangCode +"/)" + "\\n\"\n";
  m_strHeader += "\"MIME-Version: 1.0\\n\"\n";
  m_strHeader += "\"Content-Type: text/plain; charset=UTF-8\\n\"\n";
  m_strHeader += "\"Content-Transfer-Encoding: 8bit\\n\"\n";
  m_strHeader +=  "\"Language: " + strLangCode + "\\n\"\n";
  m_strHeader +=  "\"Plural-Forms: nplurals=" + strnplurals + "; plural=" + g_LCodeHandler.GetPlurForm(strLangCode) + ";\\n\"\n";
}

bool CPOHandler::AddNumPOEntryByID(uint32_t numid, CPOEntry const &POEntry, CPOEntry const &POEntryEN, bool bCopyComments)
{
  if (m_mapStrings.find(numid) != m_mapStrings.end())
    return false;
  m_mapStrings[numid] = POEntry;
  m_mapStrings[numid].msgID = POEntryEN.msgID;
  if (bCopyComments)
  {
    m_mapStrings[numid].extractedComm = POEntryEN.extractedComm;
    m_mapStrings[numid].interlineComm = POEntryEN.interlineComm;
    m_mapStrings[numid].referenceComm = POEntryEN.referenceComm;
    m_mapStrings[numid].translatorComm = POEntryEN.translatorComm;
  }
  else
  {
    m_mapStrings[numid].extractedComm.clear();
    m_mapStrings[numid].interlineComm.clear();
    m_mapStrings[numid].referenceComm.clear();
    m_mapStrings[numid].translatorComm.clear();
  }
  return true;
}

const CPOEntry* CPOHandler::GetNumPOEntryByID(uint32_t numid)
{
  if (m_mapStrings.find(numid) == m_mapStrings.end())
    return NULL;
  return &(m_mapStrings[numid]);
}

const CPOEntry* CPOHandler::GetNumPOEntryByIdx(size_t pos) const
{
  std::map<uint32_t, CPOEntry>::const_iterator it_mapStrings;
  it_mapStrings = m_mapStrings.begin();
  advance(it_mapStrings, pos);
  return &(it_mapStrings->second);
}

const CPOEntry* CPOHandler::GetClassicPOEntryByIdx(size_t pos) const
{
  std::vector<CPOEntry>::const_iterator it_vecPOEntry;
  it_vecPOEntry = m_vecClassicEntries.begin();
  advance(it_vecPOEntry, pos);
  return &(*it_vecPOEntry);
}

itStrings CPOHandler::IterateToMapIndex(itStrings it, size_t index)
{
  for (size_t i = 0; i != index; i++)
    it++;
  return it;
}

bool CPOHandler::WriteXMLFile(const std::string &strOutputPOFilename)
{
  std::string strDir = g_File.GetPath(strOutputPOFilename);
  g_File.MakeDir(strDir);

  std::string strXMLDoc;

  strXMLDoc += "<?xml version=\"1.0\" encoding=\"utf-8\" standalone=\"yes\"?>\n";
  strXMLDoc += "<!-- Translated using Transifex web application. For support, or if you would like to to help out, please visit your language team! -->\n";
  strXMLDoc += "<!-- " + g_LCodeHandler.FindLang(m_strLangCode) + " language-Team URL: " + "http://www.transifex.com/projects/p/" + g_Settings.GetProjectname() +"/language/"
  + m_strLangCode +"/ -->\n";
  strXMLDoc += "<!-- Report language file syntax bugs at: " + g_Settings.GetSupportEmailAdd() + " -->\n\n";
  strXMLDoc += "<strings>\n";

  for (itStrings it = m_mapStrings.begin(); it != m_mapStrings.end(); it++)
  {
    CPOEntry currEntry = it->second;

    if (!currEntry.interlineComm.empty())
    {
      if (it != m_mapStrings.begin())
        strXMLDoc += "\n";
      for (std::vector<std::string>::iterator itvec = currEntry.interlineComm.begin(); itvec != currEntry.interlineComm.end(); itvec++)
      {
        strXMLDoc += "    <!-- " + *itvec + " -->\n";
      }
    }

    std::string strEntry;
    if (m_bPOIsEnglish)
      strEntry = currEntry.msgID;
    else
      strEntry = currEntry.msgStr;

    strXMLDoc += "    <string id=\"" + g_CharsetUtils.IntToStr(currEntry.numID) + "\">" + g_CharsetUtils.EscapeStringXML(strEntry) + "</string>";

    if (!currEntry.extractedComm.empty())
    {
      for (std::vector<std::string>::iterator itvec = currEntry.extractedComm.begin(); itvec != currEntry.extractedComm.end(); itvec++)
      {
        strXMLDoc += " <!--" + *itvec + " -->";
      }
    }
    strXMLDoc += "\n";
  }

  strXMLDoc += "</strings>\n";

  g_File.WriteFileFromStr(strOutputPOFilename, strXMLDoc);

  return true;
};