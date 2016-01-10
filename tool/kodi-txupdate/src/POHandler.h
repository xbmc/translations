/*
 *      Copyright (C) 2014 Team Kodi
 * 
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
#pragma once

#include <map>
#include "TinyXML/tinyxml.h"
#include "ConfigHandler.h"
#include <vector>
#include <stdint.h>
#include <stdio.h>

enum
{
  NUMID = 200, // We have an entry with a numeric (previously XML) identification number.
  MSGID = 201, // We have a classic gettext entry with textual msgid. No numeric ID.
  MSGID_PLURAL = 202, // We have a classic gettext entry with textual msgid in plural form.
  COMMENT_ENTRY = 203, // We have a separate comment entry
  HEADER = 204, // We have a header entry
  UNKNOWN = 205 // Unknown entrytype found
};

enum
{
  UNKNOWNPO  = 0, // Type not set yet
  MERGEDPO = 1, // This POHandler contains merged entries from upstream and transifex
  UPDATEPO = 2, // This POHandler only contains entries to be uploaded to transifex
};

enum Boolean
{
  ISSOURCELANG=true
};

struct CAddonXMLEntry
{
  std::string strSummary;
  std::string strDescription;
  std::string strDisclaimer;
};

// Struct to collect all important data of the current processed entry.
class CPOEntry
{
public:
  CPOEntry();
  ~CPOEntry();
  int Type;
  uint32_t numID;
  std::string msgCtxt;
  std::string msgID;
  std::string msgIDPlur;
  std::string msgStr;
  std::vector<std::string> msgStrPlural;
  std::vector<std::string> extractedComm;   // #. extracted comment
  std::vector<std::string> referenceComm;   // #: reference
  std::vector<std::string> translatorComm;  // # translator comment
  std::vector<std::string> interlineComm;   // #comment between lines
  bool operator == (const CPOEntry &poentry) const;
  bool MatchMsgid(const CPOEntry& poentry) const;
};

class CPOHandler
{
public:
  CPOHandler();
  CPOHandler(const CResData& Resdata);
  ~CPOHandler();
  typedef std::map <unsigned long long, CPOEntry>::iterator T_itPOData;
  typedef std::map <unsigned long long, T_itPOData>::iterator T_itPOItData;
  typedef std::map <std::string, unsigned long long>::iterator T_itClassicPOData;
  typedef std::map <size_t, unsigned long long>::iterator T_itSequenceIndex;

  void SetReasData (const CResData& ResData) {m_ResData = ResData;}
  void SetLCode (const std::string& sLCode) {m_sLCode = sLCode;}

  bool FetchPOGitPathToMem(std::string strURL, CGITData& GitData);
  bool FetchPOTXPathToMem (std::string sLPath);

  void FetchLangAddonXML (const std::string &strURL, CGITData& GitData);
  void WriteLangAddonXML(const std::string &strPath);
  void GeneratePOFile();

  void WritePOFile(const std::string &strOutputPOFilename);
  bool FindEntry (const CPOEntry &EntryToFind);
  T_itPOData GetItFoundEntry() {return m_itLastFound;}

  bool CheckIfPOIsSameAsTheOverwritten(const std::string& strOutputPOFilename);

  void AddAddonXMLEntries (const CAddonXMLEntry& AddonXMLEntry, const CAddonXMLEntry& AddonXMLEntrySRC);
  void AddItEntry (T_itPOData it) {m_mapItPOData[it->first] = it;}
  bool ModifyClassicEntry (CPOEntry &EntryToFind, CPOEntry EntryNewValue);

  void CreateHeader (const std::string &strPreText, const std::string& sLCode);
  CAddonXMLEntry GetAddonXMLEntry () {return m_AddonXMLEntry;}

  size_t const GetClassEntriesCount() {return m_mapPOData.size();}
  size_t const GetCommntEntriesCount() {return m_CommsCntr;}
  std::map <unsigned long long, CPOEntry>::iterator GetPOMapBeginIterator() {return m_mapPOData.begin();}
  std::map <unsigned long long, CPOEntry>::iterator GetPOMapEndIterator() {return m_mapPOData.end();}

  void SetIfIsSourceLang(bool bIsSRCLang) {m_bIsSRCLang = bIsSRCLang;}
  void SetPOType(int type) {m_POType = type;}
  void SetLangAddonXMLString(const std::string& strXMLfile) {m_strLangAddonXML = strXMLfile;}
  std::string& GetLangAddonXMLString () {return m_strLangAddonXML;}
  void BumpLangAddonXMLVersion(bool bMajorBump);
  std::string BumpMinorVersion(const std::string& sVersion);
  std::string BumpMajorVersion(const std::string& sVersion);
  void CreateNewResource();
  void PutSRCFileToTRX();
  void PutTranslFileToTRX();

protected:
  void ClearCPOEntry (CPOEntry &entry);
  bool ProcessPOFile();
  unsigned int GetPluralNumOfVec(std::vector<std::string> &vecPluralStrings);
  void AddPOEntryToMaps (const CPOEntry& Entry);

  std::string m_strHeader;
  std::string m_CurrentEntryText;
  std::string m_sLCode;
  unsigned int m_nplurals;

  std::map <unsigned long long, CPOEntry> m_mapPOData;
  std::map <unsigned long long, T_itPOData> m_mapItPOData;
  std::map <std::string, unsigned long long> m_mapClassicDataIndex;
  std::map <size_t, unsigned long long> m_mapSequenceIndex;

  size_t m_CommsCntr;
  bool m_bIsSRCLang;
  int m_POType;
  std::string m_strLangAddonXML;
  CResData m_ResData;
  T_itPOData m_itLastFound;



  bool GetNextEntry(bool bSkipError);
  void ParseEntry();
  void WritePOEntry(const CPOEntry &currEntry);

  bool FindLineStart(const std::string &strToFind);
  bool ParseNumID(const std::string &strLineToCheck, size_t xIDPos);
  bool ReadStringLine(const std::string &line, std::string * pStrToAppend, int skip);
  const bool HasPrefix(const std::string &strLine, const std::string &strPrefix);
  void WriteLF();
  void WriteMultilineComment(std::vector<std::string> vecCommnts, std::string prefix);
  void ClearVariables();

  std::string m_strBuffer;
  size_t m_POfilelength;
  size_t m_CursorPos;
  size_t m_nextEntryPos;
  CPOEntry m_Entry;

  std::string m_strOutBuffer;
  bool m_bhasLFWritten;
  int m_previd;
  int m_writtenEntry;
  CAddonXMLEntry m_AddonXMLEntry;
};
