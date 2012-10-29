/*
 *      Copyright (C) 2005-2008 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include <string>
#include <stdio.h>
#include "xbmclangcodes.h"
#include "Log.h"
#include "HTTPUtils.h"
#include "JSONHandler.h"

using namespace std;

CLCodeHandler g_LCodeHandler;

enum
{
  CUST_LANG_COUNT = 6,
};

CLangcodes CustomLangcodes [CUST_LANG_COUNT] =
{
  {"Chinese (Simple)", "zh", 0, ""},
  {"Chinese (Traditional)", "zh_TW", 0, ""},
  {"English (US)", "en_US", 0, ""},
  {"Hindi (Devanagiri)", "hi", 0, ""},
  {"Serbian (Cyrillic)", "sr_RS", 0, ""},
  {"Spanish", "es", 0, ""}
};

CLCodeHandler::CLCodeHandler()
{}

CLCodeHandler::~CLCodeHandler()
{}

void CLCodeHandler::Init(std::string strURL)
{
  g_HTTPHandler.Cleanup();
  g_HTTPHandler.ReInit();
  std::string strtemp = g_HTTPHandler.GetURLToSTR(strURL);
  if (strtemp.empty())
    CLog::Log(logERROR, "XBMCLangCode::Init: error getting available language list from transifex.net");

  m_mapLCodes = g_Json.ParseTransifexLanguageDatabase(strtemp);

  for (int i=0; i<CUST_LANG_COUNT ; i++)
  {
    if (m_mapLCodes.find(CustomLangcodes[i].Langcode) != m_mapLCodes.end())
      m_mapLCodes[CustomLangcodes[i].Langcode].Langname = CustomLangcodes[i].Langname;
  }

  CLog::Log(logINFO, "LCodeHandler: Succesfully fetched %i language codes from Transifex", m_mapLCodes.size());
}

int CLCodeHandler::GetnPlurals(std::string LangCode)
{
  if (m_mapLCodes.find(LangCode) != m_mapLCodes.end())
    return m_mapLCodes[LangCode].nplurals;
  CLog::Log(logERROR, "LangCodes: GetnPlurals: unable to find langcode: %s", LangCode.c_str());
  return 0;
}

std::string CLCodeHandler::GetPlurForm(std::string LangCode)
{
  if (m_mapLCodes.find(LangCode) != m_mapLCodes.end())
    return m_mapLCodes[LangCode].Pluralform;
  CLog::Log(logERROR, "LangCodes: GetPlurForm: unable to find langcode: %s", LangCode.c_str());
  return "(n != 1)";
}

std::string CLCodeHandler::FindLang(std::string LangCode)
{
  if (m_mapLCodes.find(LangCode) != m_mapLCodes.end())
    return m_mapLCodes[LangCode].Langname;
  CLog::Log(logERROR, "LangCodes: FindLang: unable to find language for langcode: %s", LangCode.c_str());
  return "UNKNOWN";
}

std::string CLCodeHandler::FindLangCode(std::string Lang)
{
  for (itmapLCodes = m_mapLCodes.begin(); itmapLCodes != m_mapLCodes.end() ; itmapLCodes++)
  {
    if (Lang == itmapLCodes->second.Langname)
      return itmapLCodes->first;
  }
  CLog::Log(logERROR, "LangCodes: FindLangCode: unable to find langcode for language: %s", Lang.c_str());
  return "UNKNOWN";
}
