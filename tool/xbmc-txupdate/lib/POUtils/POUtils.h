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

#include <string>
#include <vector>
#include <stdint.h>
#include <stdio.h>
#include "../Log.h"
#include "../xbmclangcodes.h"
#include "../FileUtils/FileUtils.h"
#include "../CharsetUtils/CharsetUtils.h"

enum
{
  ID_FOUND = 200, // We have an entry with a numeric (previously XML) identification number.
  MSGID_FOUND = 201, // We have a classic gettext entry with textual msgid. No numeric ID.
  MSGID_PLURAL_FOUND = 202, // We have a classic gettext entry with textual msgid in plural form.
  COMMENT_ENTRY_FOUND = 203, // We have a separate comment entry
  HEADER_FOUND = 204, // We have a header entry
  UNKNOWN_FOUND = 205 // Unknown entrytype found
};

enum Boolean
{
  ISSOURCELANG=true
};

enum
{
  SKIN = 100,
  ADDON = 101,
  CORE = 102,
  ADDON_NOSTRINGS = 103,
  UNKNOWN = 104
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
  std::string Content;
  bool operator == (const CPOEntry &poentry) const;
};

class CPODocument
{
public:
  CPODocument();
  ~CPODocument();

  bool SaveFile(const std::string &pofilename);
  bool GetNextEntry(bool bSkipError);
  int GetEntryType() const {return m_Entry.Type;}
  void ParseEntry();
  CPOEntry GetEntryData() const {return m_Entry;}
  void WriteHeader(const std::string &strHeader);
  void WritePOEntry(const CPOEntry &currEntry, unsigned int nplurals);
  void SetIfIsEnglish(bool bIsENLang) {m_bIsForeignLang = !bIsENLang;}
  void SetIfIsUpdDoc(bool bIsUpdTx) {m_bIsUpdateTxDoc = bIsUpdTx;}
  bool FetchURLToMem(const std::string &strURL, bool bSkipError);
  bool ParseStrToMem(const std::string &strPOData, std::string const &strFilePath);

protected:
  std::string IntToStr(int number);
  std::string UnescapeString(const std::string &strInput);
  bool FindLineStart(const std::string &strToFind);
  bool ParseNumID(const std::string &strLineToCheck, size_t xIDPos);
  void ConvertLineEnds(const std::string &filename);
  bool ReadStringLine(const std::string &line, std::string * pStrToAppend, int skip);
  const bool HasPrefix(const std::string &strLine, const std::string &strPrefix);
  void WriteLF();
  void WriteMultilineComment(std::vector<std::string> vecCommnts, std::string prefix);

  std::string m_strBuffer;
  size_t m_POfilelength;
  size_t m_CursorPos;
  size_t m_nextEntryPos;
  CPOEntry m_Entry;

  std::string m_strOutBuffer;
  bool m_bhasLFWritten;
  bool m_bIsForeignLang;
  bool m_bIsUpdateTxDoc;
  int m_previd;
  int m_writtenEntry;
};
