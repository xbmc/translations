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
#pragma once

#include "POUtils/POUtils.h"
#include <map>
#include "TinyXML/tinyxml.h"

typedef std::map<uint32_t, CPOEntry>::iterator itStrings;
typedef std::vector<CPOEntry>::iterator itClassicEntries;


class CPOHandler
{
public:
  CPOHandler();
  ~CPOHandler();
  bool FetchPOURLToMem(std::string strURL, bool bSkipError);
  bool FetchXMLURLToMem (std::string strURL);
  bool ParsePOStrToMem (std::string const &strPOData, std::string const &strFilePath);
  bool WritePOFile(const std::string &strOutputPOFilename);
  bool WriteXMLFile(const std::string &strOutputPOFilename);
  bool LookforClassicEntry (CPOEntry &EntryToFind);
  const CPOEntry*  PLookforClassicEntry (CPOEntry &EntryToFind);
  bool AddClassicEntry (CPOEntry EntryToAdd, CPOEntry const &POEntryEN, bool bCopyComments);
  bool ModifyClassicEntry (CPOEntry &EntryToFind, CPOEntry EntryNewValue);
  bool DeleteClassicEntry (CPOEntry &EntryToFind);

  const CPOEntry* GetNumPOEntryByID(uint32_t numid);
  bool AddNumPOEntryByID(uint32_t numid, CPOEntry const &POEntry, CPOEntry const &POEntryEN, bool bCopyComments);
  const CPOEntry* GetClassicPOEntryByIdx(size_t pos) const;

  const CPOEntry* GetNumPOEntryByIdx(size_t pos) const;
  void SetHeader (std::string strHeader) {m_strHeader = strHeader;}
  void SetHeaderNEW (std::string strLangCode);
  std::string GetHeader () {return m_strHeader;}

  void SetAddonMetaData (CAddonXMLEntry const &AddonXMLEntry, CAddonXMLEntry const &PrevAddonXMLEntry,
                         CAddonXMLEntry const &AddonXMLEntryEN, std::string const &strLang);
  void GetAddonMetaData (CAddonXMLEntry &AddonXMLEntry, CAddonXMLEntry &AddonXMLEntryEN);
  void SetPreHeader (std::string &strPreText);
  size_t const GetNumEntriesCount() {return m_mapStrings.size();}
  size_t const GetClassEntriesCount() {return m_vecClassicEntries.size();}
  size_t const GetCommntEntriesCount() {return m_CommsCntr;}
  void SetIfIsEnglish(bool bIsENLang) {m_bPOIsEnglish = bIsENLang;}
  void SetIfPOIsUpdTX(bool bIsUpdTX) {m_bPOIsUpdateTX = bIsUpdTX;}
  bool GetIfSourceIsXML () {return m_bIsXMLSource;}

protected:
  void ClearCPOEntry (CPOEntry &entry);
  bool ProcessPOFile(CPODocument &PODoc);
  itStrings IterateToMapIndex(itStrings it, size_t index);
  bool GetXMLEncoding(const TiXmlDocument* pDoc, std::string& strEncoding);
  void GetXMLComment(std::string strXMLEncoding, const TiXmlNode *pCommentNode, CPOEntry &currEntry);
  int GetPluralNumOfVec(std::vector<std::string> &vecPluralStrings);
  void ParsePOHeader();

  std::string m_strHeader;
  std::string m_strLangCode;
  int m_nplurals;

  std::map<uint32_t, CPOEntry> m_mapStrings;
  std::vector<CPOEntry> m_vecClassicEntries;

  CPOEntry m_prevCommEntry;
  bool m_bIsXMLSource;
  size_t m_CommsCntr;
  bool m_bPOIsEnglish;
  bool m_bPOIsUpdateTX;
};
