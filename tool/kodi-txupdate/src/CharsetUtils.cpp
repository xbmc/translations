/*
 *      Copyright (C) 2005-2014 Team Kodi
 *      http://xbmc.org
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

#include <string>
#include "CharsetUtils.h"
#include <errno.h>
#include "Log.h"
#include <sstream>
#include <algorithm>
#include "Langcodes.h"
#include "FileUtils.h"

CCharsetUtils g_CharsetUtils;

std::string CCharsetUtils::IntToStr(int number)
{
  std::stringstream ss;//create a stringstream
  ss << number;//add number to the stream
  return ss.str();//return a string with the contents of the stream
};

std::string CCharsetUtils::ChrToStr(char chr)
{
  std::stringstream ss;
  ss << chr;
  return ss.str(); //return a string with the contents of the stream
};

std::string CCharsetUtils::UnescapeCPPString(const std::string &strInput)
{
  std::string strOutput;
  if (strInput.empty())
    return strOutput;

  char oescchar;
  strOutput.reserve(strInput.size());
  std::string::const_iterator it = strInput.begin();
  while (it < strInput.end())
  {
    oescchar = *it++;
    if (oescchar == '\\')
    {
      if (it == strInput.end())
      {
        CLog::Log(logWARNING, "CharsetUtils: Unhandled escape character at line-end. Problematic string: %s", strInput.c_str());
        continue;
      }
      switch (*it++)
      {
        case 'a':  oescchar = '\a'; break;
        case 'b':  oescchar = '\b'; break;
        case 'v':  oescchar = '\v'; break;
        case 'n':  oescchar = '\n'; break;
        case 't':  oescchar = '\t'; break;
        case 'r':  oescchar = '\r'; break;
        case '"':  oescchar = '"' ; break;
        case '0':  oescchar = '\0'; break;
        case 'f':  oescchar = '\f'; break;
        case '?':  oescchar = '\?'; break;
        case '\'': oescchar = '\''; break;
        case '\\': oescchar = '\\'; break;

        default: 
        {
          CLog::Log(logWARNING, "CharsetUtils: Unhandled escape character. Problematic string: %s",strInput.c_str());
          continue;
        }
      }
    }
    strOutput.push_back(oescchar);
  }
  return strOutput;
};

std::string CCharsetUtils::EscapeStringCPP(const std::string &strInput)
{
  std::string strOutput;
  if (strInput.empty())
    return strOutput;

  strOutput.reserve(strInput.size());
  for (std::string::const_iterator it = strInput.begin(); it != strInput.end();it++)
  {
    switch (*it)
    {
      case '\n':  strOutput += "\\n"; break;
      case '\"':  strOutput += "\\\""; break;
      case '\r':  strOutput += "\\r"; break;
      case '\\':  strOutput += "\\\\"; break;
      default: strOutput.push_back(*it);
    }
  }
  return strOutput;
};

std::string CCharsetUtils::EscapeStringXML(const std::string &strInput)
{
  std::string strOutput;
  if (strInput.empty())
    return strOutput;

  strOutput.reserve(strInput.size());
  for (std::string::const_iterator it = strInput.begin(); it != strInput.end();it++)
  {
    switch (*it)
    {
      case '\n':  strOutput += "&#10;";  break;
      case '<':   strOutput += "&lt;";   break;
      case '>':   strOutput += "&gt;";   break;
      case '&':   strOutput += "&amp;";  break;
      default: strOutput.push_back(*it);
    }
  }
  return strOutput;
};

// remove trailing and leading whitespaces
std::string CCharsetUtils::UnWhitespace(std::string strInput)
{
  int offset_end = strInput.size();
  int offset_start = 0;

  while (strInput[offset_start] == ' ')
    offset_start++; // check first non-whitespace char
  while (strInput[offset_end-1] == ' ')
    offset_end--; // check last non whitespace char

  strInput = strInput.substr(offset_start, offset_end - offset_start);
  return strInput;
}

// cleaning string of \n and {} characters
std::string CCharsetUtils::CleanTranslatorlist(std::string strInput)
{
  std::string strTarget;
  bool SpacePassed = false;
  for (std::string::iterator it = strInput.begin(); it != strInput.end(); it++)
  {
    if (*it == ' ' && SpacePassed)
      continue;
    if (*it == ' ' && !SpacePassed)
      SpacePassed = true;
    else
      SpacePassed = false;

    if (*it != '{' && *it != '}' && *it != '\n')
      strTarget += *it;
  }
  return strTarget;
}

std::string CCharsetUtils::ToUTF8(std::string strEncoding, const std::string& str)
{
  if (strEncoding.empty() || strEncoding == "utf8" || strEncoding == "UTF8" || strEncoding == "utf-8" || strEncoding == "UTF-8")
  {
    if (IsValidUTF8(str))
      return str;

    CLog::Log(logWARNING, "CharsetUtils::ToUTF8: Invalid character sequence in UTF8 string, trying windows-1252 for string: %s", str.c_str());
    strEncoding = "windows-1252"; // The most common is that user thinks that he is typing utf8, but he uses cp-1252 codepage
  }

  std::string ret = stringCharsetToUtf8(strEncoding, str);

  if (!IsValidUTF8(ret))
    CLog::Log(logERROR, "CharsetUtils::ToUTF8: Wrong character encoding given, not able to convert to UTF8. String is: %s", str.c_str());

  return ret;
}

std::string CCharsetUtils::stringCharsetToUtf8(const std::string& strCP, std::string strIn)
{
  //Part1 Initalize
  iconv_t conv_desc;
  conv_desc = iconv_open ("UTF-8", strCP.c_str());
  if (conv_desc == (iconv_t)-1) /* Initialization failure. */
  {
    if (errno == EINVAL)
      CLog::Log(logERROR, "Libiconv: Conversion from '%s' to UTF-8 is not supported by libiconv\n", strCP.c_str());
    else
      CLog::Log(logERROR, "Libiconv Initialization failure: %s\n", strerror (errno));
  }

  //Part2 Convert
  size_t iconv_value;
  std::string strOut;
  strOut.resize(strIn.size()*4 + 1, '\x00');

  size_t lenStrIn = strIn.size();
  size_t lenStrOut = strOut.size();

  char * pStrOut = &strOut[0];
  strIn  += '\x00';
  char * pStrIn = &strIn[0];

  iconv_value = iconv (conv_desc, &pStrIn, &lenStrIn, &pStrOut, &lenStrOut);

  strOut.resize(strOut.size()-lenStrOut);

  if (iconv_value == (size_t) -1) /* Handle failures. */
  {
    CLog::Log(logWARNING, "Liconv failed: with codepage: %s, in string: %s\n", strCP.c_str(), strOut.c_str());
    switch (errno)
    {
      case EILSEQ:
        CLog::Log(logERROR, "Invalid multibyte sequence.\n");
        break;
      case EINVAL:
        CLog::Log(logERROR, "Incomplete multibyte sequence.\n");
        break;
      case E2BIG:
        CLog::Log(logERROR, "No more room.\n");
        break;
      default:
        CLog::Log(logERROR, "Error: %s.\n", strerror (errno));
    }
  }

  //Part3: Close the library
  if (iconv_close (conv_desc) != 0)
    CLog::Log(logERROR,  "Libiconv_close failed: %s\n", strerror (errno));

  return strOut;
}

bool CCharsetUtils::IsValidUTF8(std::string const &strToCheck)
{
  if (strToCheck.find('\x00') != std::string::npos)
    CLog::Log(logERROR, "CharsetUtils::IsValidUTF8: zero byte detected in string: %s", strToCheck.c_str());

  int numContBExpected =0;
  for (std::string::const_iterator it = strToCheck.begin(); it != strToCheck.end(); it++)
  {
    unsigned char ch = (unsigned char) *it;

    if (ch <= 0x7f) //We are at the ASCII range
    {
      if (numContBExpected ==0)
        continue;
      else
        return false; // we were expecting a continuation byte, but we are not getting one
    }

    if (ch==0xc0 || ch==0xc1 || ch>=0xf5) //invalid characters in an utf8 string
      return false;

    if ((ch & 0xc0) == 0xc0 && (numContBExpected !=0)) // we have a code entry start, but
      return false; // we are still expecting a continuation byte not a new code start

    if ((ch & 0xc0) == 0x80)
    {
      if (numContBExpected == 0) // we have a continuation byte, but
        return false; // we are not expecting one
      numContBExpected--;
      continue;
    }

    if ((ch & 0xe0) == 0xc0) // we have a 2byte long code entry start
      numContBExpected = 1;
    else if ((ch & 0xf0) == 0xe0) // we have a 3byte long code entry start
      numContBExpected = 2;
    else if ((ch & 0xf8) == 0xf0) // we have a 4byte long code entry start
      numContBExpected = 3;
    else if ((ch & 0xfc) == 0xf8) // we have a 5byte long code entry start which is invalid by new standards
      return false;
    else if ((ch & 0xfe) == 0xfc) // we have a 6byte long code entry start which is invalid by new standards
      return false;
  }
  return true;
};

size_t CCharsetUtils::GetCharCountInStr(std::string const &strToCheck, unsigned char chrToFInd)
{
  return std::count(strToCheck.begin(), strToCheck.end(), chrToFInd);
}

bool CCharsetUtils::replaceAllStrParts(std::string * pstr, const std::string& from, const std::string& to)
{
  if (pstr->find(from) == std::string::npos)
    return false;

  size_t start_pos = 0;
  while((start_pos = pstr->find(from, start_pos)) != std::string::npos)
  {
      pstr->replace(start_pos, from.length(), to);
      start_pos += to.length();
  }
  return true;
};

std::string CCharsetUtils::replaceStrParts(std::string strToReplace, const std::string& from, const std::string& to)
{
  replaceAllStrParts(&strToReplace, from, to);
  return strToReplace;
};

//TODO use reference instead of pointer
void CCharsetUtils::reBrandXBMCToKodi(std::string * pstrtorebrand)
{
  replaceAllStrParts(pstrtorebrand, "XMBC.org", "Kodi.tv");
  replaceAllStrParts(pstrtorebrand, "XMBC.ORG", "Kodi.tv");
  replaceAllStrParts(pstrtorebrand, "xbmc.org", "kodi.tv");
  replaceAllStrParts(pstrtorebrand, "xmbc.org", "kodi.tv");
  replaceAllStrParts(pstrtorebrand, "XBMC.org", "Kodi.tv");
  replaceAllStrParts(pstrtorebrand, "XBMC.ORG", "Kodi.tv");
  replaceAllStrParts(pstrtorebrand, "Xbmc.org", "Kodi.tv");
  replaceAllStrParts(pstrtorebrand, "XBMC", "Kodi");
  replaceAllStrParts(pstrtorebrand, "XMBC", "Kodi");
  replaceAllStrParts(pstrtorebrand, "xbmc", "Kodi");
  replaceAllStrParts(pstrtorebrand, "xmbc", "Kodi");
  replaceAllStrParts(pstrtorebrand, "Xbmc", "Kodi");
}

std::string CCharsetUtils::GetRoot(const std::string &strPath,const std::string &strFilename)
{
  return strPath.substr(0, strPath.size()-strFilename.size());
}

std::string CCharsetUtils::GetFilenameFromURL(const std::string &sURL)
{
  size_t lastpos = sURL.find_last_of(DirSepChar);
  if (lastpos != std::string::npos)
    return sURL.substr(lastpos+1);
  return sURL;
}

std::string CCharsetUtils::GetLangnameFromPath(std::string sExtractPath, std::string sPath, std::string sLForm)
{
  size_t pos1 = sPath.find(sLForm);
  if (pos1 == std::string::npos)
    return "";

  size_t pos2 = pos1 + sLForm.size();
  if (pos2 > sPath.size())
    return "";

  std::string strPre = sPath.substr(0, pos1);
  std::string strPost = sPath.substr(pos2);

  if (sExtractPath.find(strPre) != 0 || sExtractPath.rfind(strPost) != sExtractPath.size()-strPost.size())
    return "";

  return sExtractPath.substr(strPre.size(), sExtractPath.size()-strPre.size()-strPost.size());
}

std::string CCharsetUtils::ReplaceLanginURL(const std::string& strURL, const std::string& strLangFormat, const std::string& strLCode)
{
  return replaceStrParts(strURL, strLangFormat, g_LCodeHandler.GetLangFromLCode(strLCode, strLangFormat));
}

void CCharsetUtils::ConvertLineEnds(std::string &strBuffer)
{
  size_t foundPos = strBuffer.find_first_of("\r");
  if (foundPos == std::string::npos)
    return; // We have only Linux style line endings in the file, nothing to do

  std::string strTemp;
  strTemp.reserve(strBuffer.size());
  for (std::string::const_iterator it = strBuffer.begin(); it < strBuffer.end(); it++)
  {
    if (*it == '\r')
    {
      if (it+1 == strBuffer.end() || *(it+1) != '\n')
        strTemp.push_back('\n'); // convert Mac style line ending and continue
        continue; // we have Win style line ending so we exclude this CR now
    }
    strTemp.push_back(*it);
  }
  strBuffer.swap(strTemp);
};

std::string CCharsetUtils::GetLFormFromPath(const std::string& sPath)
{
  std::string sLForm;
  size_t posLForm1 = sPath.find("$(");
  size_t posLForm2 = sPath.find(')',posLForm1);
  if (posLForm1 != std::string::npos && posLForm2 != std::string::npos)
    sLForm = sPath.substr(posLForm1, posLForm2 - posLForm1 + 1);
  return sLForm;
}