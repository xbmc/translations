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

#include "POUtils.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdio.h>
#include <sstream>
#include "../HTTPUtils.h"
#include "../Settings.h"

using namespace std;

CPOEntry::CPOEntry()
{
  numID = 0;
  Type = UNKNOWN_FOUND;
}

CPOEntry::~CPOEntry()
{}


bool CPOEntry::operator==(const CPOEntry& poentry) const
{
  bool bhasMatch = true;
  if (!poentry.msgCtxt.empty())
    bhasMatch = bhasMatch && (poentry.msgCtxt == msgCtxt);
  if (!poentry.msgID.empty())
    bhasMatch = bhasMatch && (poentry.msgID == msgID);
  if (!poentry.msgIDPlur.empty())
    bhasMatch = bhasMatch && (poentry.msgIDPlur == msgIDPlur);
  if (!poentry.msgStr.empty())
    bhasMatch = bhasMatch && (poentry.msgStr == msgStr);
  if (!poentry.msgStrPlural.empty())
    bhasMatch = bhasMatch && (poentry.msgStrPlural == msgStrPlural);
  if (!poentry.Type == ID_FOUND)
    bhasMatch = bhasMatch && (poentry.numID == numID);
  if (poentry.Type != UNKNOWN_FOUND && poentry.Type != 0)
    bhasMatch = bhasMatch && (poentry.Type == Type);
  return bhasMatch;
};

CPODocument::CPODocument()
{
  m_CursorPos = 0;
  m_nextEntryPos = 0;
  m_POfilelength = 0;
  m_bhasLFWritten = false;
  m_previd = -1;
  m_writtenEntry = 0;
};

CPODocument::~CPODocument() {};

std::string CPODocument::IntToStr(int number)
{
  std::stringstream ss;//create a stringstream
  ss << number;//add number to the stream
  return ss.str();//return a string with the contents of the stream
};

bool CPODocument::FetchURLToMem(const std::string &strURL, bool bSkipError)
{
  m_strBuffer.clear();
  m_strBuffer = g_HTTPHandler.GetURLToSTR(strURL);
  if (m_strBuffer.empty())
  {
    if (bSkipError)
      return false;
    else
      CLog::Log(logERROR, "CPODocument::FetchURLToMem: http error reading po file from url: %s", strURL.c_str());
  }

  m_strBuffer = "\n" + m_strBuffer;
  ConvertLineEnds(strURL);

  // we make sure, to have an LF at beginning and at end of buffer
  m_strBuffer[0] = '\n';
  if (*m_strBuffer.rbegin() != '\n')
  {
    m_strBuffer += "\n";
  }
  m_POfilelength = m_strBuffer.size();

  if (GetNextEntry(true) && m_Entry.Type == MSGID_FOUND &&
    m_Entry.Content.find("msgid \"\"")  != std::string::npos &&
    m_Entry.Content.find("msgstr \"\"") != std::string::npos)
  {
    m_Entry.Type = HEADER_FOUND;
    return true;
  }

  if (bSkipError)
    CLog::Log(logINFO, "POParser: unable to read PO file header from URL: %s", strURL.c_str());
  else
    CLog::Log(logERROR, "POParser: unable to read PO file header from URL: %s", strURL.c_str());
  return false;
};

bool CPODocument::ParseStrToMem(const std::string &strPOData, std::string const &strFilePath)
{
  m_strBuffer.clear();
  m_strBuffer = strPOData;
  if (m_strBuffer.empty())
      CLog::Log(logERROR, "CPODocument::ParseStrToMem: PO fle to parse has a zero length for file: %s", strFilePath.c_str());

  m_strBuffer = "\n" + m_strBuffer;
  ConvertLineEnds(strFilePath);

  // we make sure, to have an LF at beginning and at end of buffer
  m_strBuffer[0] = '\n';
  if (*m_strBuffer.rbegin() != '\n')
  {
    m_strBuffer += "\n";
  }
  m_POfilelength = m_strBuffer.size();

  if (GetNextEntry(true) && m_Entry.Type == MSGID_FOUND &&
    m_Entry.Content.find("msgid \"\"")  != std::string::npos &&
    m_Entry.Content.find("msgstr \"\"") != std::string::npos)
  {
    m_Entry.Type = HEADER_FOUND;
    return true;
  }

  CLog::Log(logERROR, "POParser: unable to read PO file header from file: %s", strFilePath.c_str());
  return false;
};

bool CPODocument::GetNextEntry(bool bSkipError)
{
  do
  {
    // if we don't find LFLF, we reached the end of the buffer and the last entry to check
    // we indicate this with setting m_nextEntryPos to the end of the buffer
    if ((m_nextEntryPos = m_strBuffer.find("\n\n", m_CursorPos)) == std::string::npos)
      m_nextEntryPos = m_POfilelength-1;

    // now we read the actual entry into a temp string for further processing
    m_Entry.Content.assign(m_strBuffer, m_CursorPos, m_nextEntryPos - m_CursorPos +1);
    size_t oldCursorPos = m_CursorPos;
    m_CursorPos = m_nextEntryPos+1; // jump cursor to the second LF character

    if (FindLineStart ("\nmsgid "))
    {
      if (FindLineStart ("\nmsgctxt \"#"))
      {
        size_t ipos = m_Entry.Content.find("\nmsgctxt \"#");
        if (isdigit(m_Entry.Content[ipos+11]))
        {
          m_Entry.Type = ID_FOUND; // we found an entry with a valid numeric id
          return true;
        }
      }

      if (FindLineStart ("\nmsgid_plural "))
      {
        m_Entry.Type = MSGID_PLURAL_FOUND; // we found a pluralized entry
        return true;
      }

      m_Entry.Type = MSGID_FOUND; // we found a normal entry, with no numeric id
      return true;
    }
    if (FindLineStart ("\n#"))
    {
      size_t ipos = m_Entry.Content.find("\n#");
      if (m_Entry.Content[ipos+2] != ' ' && m_Entry.Content[ipos+2] != '.' &&
          m_Entry.Content[ipos+2] != ':' && m_Entry.Content[ipos+2] != ',' &&
          m_Entry.Content[ipos+2] != '|')
      {
        m_Entry.Type = COMMENT_ENTRY_FOUND; // we found a pluralized entry
        return true;
      }
    }
    if (m_nextEntryPos != m_POfilelength-1 && !bSkipError)
    {
      if (m_Entry.Content.find_first_not_of("\n") == std::string::npos)
        CLog::Log(logINFO, "POParser: Empty line(s) found at position: %i, ignored.", oldCursorPos);
      else
        CLog::Log(logWARNING, "POParser: unknown entry found at position %i, Failed entry: %s", oldCursorPos,
                  m_Entry.Content.substr(0,m_Entry.Content.size()-1).c_str());
    }
  }
  while (m_nextEntryPos != m_POfilelength-1);
  // we reached the end of buffer AND we have not found a valid entry

  return false;
};

void CPODocument::ParseEntry()
{
  m_Entry.msgStrPlural.clear();
  m_Entry.translatorComm.clear();
  m_Entry.referenceComm.clear();
  m_Entry.interlineComm.clear();
  m_Entry.extractedComm.clear();
  m_Entry.numID = 0;
  m_Entry.msgID.clear();
  m_Entry.msgStr.clear();
  m_Entry.msgIDPlur.clear();
  m_Entry.msgCtxt.clear();

  size_t LineCursor = 1;
  size_t NextLineStart = 0;
  std::string strLine;
  std::string * pPlaceToParse = NULL;

  while ((NextLineStart = m_Entry.Content.find('\n', LineCursor)) != std::string::npos)
  {

    std::string strTemp;
    strTemp.assign(m_Entry.Content, LineCursor, NextLineStart - LineCursor +1);
    size_t strStart = strTemp.find_first_not_of("\n \t");
    size_t strEnd = strTemp.find_last_not_of("\n \t");

    if (strStart != std::string::npos && strEnd != std::string::npos)
      strLine.assign(strTemp, strStart, strEnd - strStart +1);
    else
      strLine = "";

    LineCursor = NextLineStart +1;

    if (pPlaceToParse && ReadStringLine(strLine, pPlaceToParse,0)) continue; // we are reading a continous multilne string
      else pPlaceToParse= NULL; // end of reading the multiline string

      if (HasPrefix(strLine, "msgctxt") && !HasPrefix(strLine, "msgctxt \"#") && strLine.size() > 9)
      {
        pPlaceToParse = &m_Entry.msgCtxt;
        if (!ReadStringLine(strLine, pPlaceToParse,8))
        {
          CLog::Log(logWARNING, "POParser: wrong msgctxt format. Failed entry: %s", m_Entry.Content.c_str());
          pPlaceToParse = NULL;
        }
      }

      else if (HasPrefix(strLine, "msgid") && strLine.size() > 7)
      {
        pPlaceToParse = &m_Entry.msgID;
        if (!ReadStringLine(strLine, pPlaceToParse,6))
        {
          CLog::Log(logWARNING, "POParser: wrong msgid format. Failed entry: %s", m_Entry.Content.c_str());
          pPlaceToParse = NULL;
        }
      }

      else if (HasPrefix(strLine, "msgid_plural") && strLine.size() > 14)
      {
        pPlaceToParse = &m_Entry.msgIDPlur;
        if (!ReadStringLine(strLine, pPlaceToParse,13))
        {
          CLog::Log(logWARNING, "POParser: wrong msgid_plural format. Failed entry: %s", m_Entry.Content.c_str());
          pPlaceToParse = NULL;
        }
      }

      else if (HasPrefix(strLine, "msgstr") && strLine.size() > 8)
      {
        pPlaceToParse = &m_Entry.msgStr;
        if (!ReadStringLine(strLine, pPlaceToParse,7))
        {
          CLog::Log(logWARNING, "POParser: wrong msgstr format. Failed entry: %s", m_Entry.Content.c_str());
          pPlaceToParse = NULL;
        }
      }

      else if (HasPrefix(strLine, "msgstr[") && strLine[8] == ']'&& strLine.size() > 11)
      {
        m_Entry.msgStrPlural.push_back("");
        pPlaceToParse = &m_Entry.msgStrPlural[m_Entry.msgStrPlural.size()-1];
        if (!ReadStringLine(strLine, pPlaceToParse,10))
        {
          CLog::Log(logWARNING, "POParser: wrong msgstr[] format. Failed entry: %s", m_Entry.Content.c_str());
          pPlaceToParse = NULL;
        }
      }

      else if (HasPrefix(strLine, "msgctxt \"#") && strLine.size() > 10 && isdigit(strLine[10]))
        ParseNumID(strLine, 10);

      else if (HasPrefix(strLine, "#:") && strLine.size() > 2)
      {
        std::string strCommnt = strLine.substr(2);
        m_Entry.referenceComm.push_back(strCommnt);
      }

      else if (HasPrefix(strLine, "#.") && strLine.size() > 2)
      {
        std::string strCommnt = strLine.substr(2);
        m_Entry.extractedComm.push_back(strCommnt);
      }

      else if (HasPrefix(strLine, "#") && strLine.size() > 1 && strLine[1] != '.' &&
        strLine[1] != ':' && strLine[1] != ' ')
      {
        std::string strCommnt = strLine.substr(1);
        if (strCommnt.substr(0,5) != "empty")
          m_Entry.interlineComm.push_back(strCommnt);
      }
      else if (HasPrefix(strLine, "# "))
      {
        std::string strCommnt = strLine.substr(2);
        m_Entry.translatorComm.push_back(strCommnt);
      }
      else
        CLog::Log(logWARNING, "POParser: unknown line type found. Failed entry: %s", m_Entry.Content.c_str());
  }
  return;
};

bool CPODocument::ReadStringLine(const std::string &line, std::string * pStrToAppend, int skip)
{
  int linesize = line.size(); 
  if (line[linesize-1] != '\"' || line[skip] != '\"') return false;
  std::string strToAppend;
  strToAppend.append(line, skip + 1, linesize - skip - 2);
  pStrToAppend->append(g_CharsetUtils.UnescapeCPPString(strToAppend));
  return true;
};

const bool CPODocument::HasPrefix(const std::string &strLine, const std::string &strPrefix)
{
  if (strLine.length() < strPrefix.length())
    return false;
  else
    return strLine.compare(0, strPrefix.length(), strPrefix) == 0;
};


bool CPODocument::FindLineStart(const std::string &strToFind)
{

  if (m_Entry.Content.find(strToFind) == std::string::npos)
    return false; // if we don't find the string or if we don't have at least one char after it

  return true;
};

bool CPODocument::ParseNumID(const std::string &strLineToCheck, size_t xIDPos)
{
  if (isdigit(strLineToCheck.at(xIDPos))) // verify if the first char is digit
  {
    // we check for the numeric id for the fist 10 chars (uint32)
    m_Entry.numID = strtol(&strLineToCheck[xIDPos], NULL, 10);
    return true;
  }

  CLog::Log(logWARNING, "POParser: Found numeric id descriptor, but no valid id can be read, "
         "entry was handled as normal msgid entry");
  CLog::Log(logWARNING, "POParser: The problematic entry: %s", m_Entry.Content.c_str());
  return false;
};

void CPODocument::ConvertLineEnds(const std::string &filename)
{
  size_t foundPos = m_strBuffer.find_first_of("\r");
  if (foundPos == std::string::npos)
    return; // We have only Linux style line endings in the file, nothing to do

  if (foundPos+1 >= m_strBuffer.size() || m_strBuffer[foundPos+1] != '\n')
    CLog::Log(logINFO, "POParser: PO file has Mac Style Line Endings. "
           "Converted in memory to Linux LF for file: %s", filename.c_str());
  else
    CLog::Log(logINFO, "POParser: PO file has Win Style Line Endings. "
           "Converted in memory to Linux LF for file: %s", filename.c_str());

  std::string strTemp;
  strTemp.reserve(m_strBuffer.size());
  for (std::string::const_iterator it = m_strBuffer.begin(); it < m_strBuffer.end(); it++)
  {
    if (*it == '\r')
    {
      if (it+1 == m_strBuffer.end() || *(it+1) != '\n')
        strTemp.push_back('\n'); // convert Mac style line ending and continue
      continue; // we have Win style line ending so we exclude this CR now
    }
    strTemp.push_back(*it);
  }
  m_strBuffer.swap(strTemp);
  m_POfilelength = m_strBuffer.size();
};

// ********* SAVE part

bool CPODocument::SaveFile(const std::string &pofilename)
{
  std::string strDir = g_File.GetPath(pofilename);
  g_File.MakeDir(strDir);

  // Initalize the output po document
  FILE * pPOTFile = fopen (pofilename.c_str(),"wb");
  if (pPOTFile == NULL)
  {
    CLog::Log(logERROR, "POParser: Error opening output file: %s\n", pofilename.c_str());
    return false;
  }
  fprintf(pPOTFile, "%s", m_strOutBuffer.c_str());
  fclose(pPOTFile);

  return true;
};

void CPODocument::WriteHeader(const std::string &strHeader)
{
  m_bhasLFWritten = false;

  m_strOutBuffer.clear();
  m_strOutBuffer += strHeader;
};

void CPODocument::WritePOEntry(const CPOEntry &currEntry)
{
  m_bhasLFWritten = false;

  if ((!m_bIsForeignLang || g_Settings.GetForcePOComments()) && currEntry.Type == ID_FOUND)
  {
    int id = currEntry.numID;
    if (id-m_previd >= 2 && m_previd > -1 && !m_bIsForeignLang)
    {
      WriteLF();
      if (id-m_previd == 2)
        m_strOutBuffer += "#empty string with id "  + IntToStr(id-1) + "\n";
      if (id-m_previd > 2)
        m_strOutBuffer += "#empty strings from id " + IntToStr(m_previd+1) + " to " + IntToStr(id-1) + "\n";
    }
    WriteMultilineComment(currEntry.interlineComm, "#");
    m_previd =id;
  }
  m_bhasLFWritten = false;

  if (!m_bIsForeignLang || g_Settings.GetForcePOComments())
  {
    WriteMultilineComment(currEntry.translatorComm, "# ");
    WriteMultilineComment(currEntry.extractedComm,  "#.");
    WriteMultilineComment(currEntry.referenceComm,  "#:");
  }

  WriteLF();
  if (currEntry.Type == ID_FOUND)
    m_strOutBuffer += "msgctxt \"#" + IntToStr(currEntry.numID) + "\"\n";
  else if (!currEntry.msgCtxt.empty())
    m_strOutBuffer += "msgctxt \"" + g_CharsetUtils.EscapeStringCPP(currEntry.msgCtxt) + "\"\n";

  WriteLF();
  m_strOutBuffer += "msgid \""  + g_CharsetUtils.EscapeStringCPP(currEntry.msgID) +  "\"\n";
  if (m_bIsForeignLang)
    m_strOutBuffer += "msgstr \"" + g_CharsetUtils.EscapeStringCPP(currEntry.msgStr) + "\"\n";
  else
    m_strOutBuffer += "msgstr \"\"\n";

  m_writtenEntry++;
};

void CPODocument::WriteLF()
{
  if (!m_bhasLFWritten)
  {
    m_bhasLFWritten = true;
    m_strOutBuffer += "\n";
  }
};

void CPODocument::WriteMultilineComment(std::vector<std::string> vecCommnts, std::string prefix)
{
  if (vecCommnts.empty())
    return;

  for (size_t i=0; i < vecCommnts.size(); i++)
  {
    WriteLF();
    m_strOutBuffer += prefix + vecCommnts[i] + "\n";
  }
  return;
};
