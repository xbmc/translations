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

#include "JSONHandler.h"
#include "Log.h"
#include <list>
#include <stdlib.h>
#include "Settings.h"

CJSONHandler g_Json;

using namespace std;

CJSONHandler::CJSONHandler()
{};

CJSONHandler::~CJSONHandler()
{};

std::list<std::string> CJSONHandler::ParseResources(std::string strJSON)
{
  Json::Value root;   // will contains the root value after parsing.
  Json::Reader reader;
  std::string resName;
  std::list<std::string> listResources;

  bool parsingSuccessful = reader.parse(strJSON, root );
  if ( !parsingSuccessful )
  {
    CLog::Log(logERROR, "JSONHandler: Parse resource: no valid JSON data");
    return listResources;
  }

  for(Json::ValueIterator itr = root.begin() ; itr != root.end() ; itr++)
  {
    Json::Value valu = *itr;
    resName = valu.get("slug", "unknown").asString();

    if (resName.size() == 0 || resName == "unknown")
      CLog::Log(logERROR, "JSONHandler: Parse resource: no valid JSON data while iterating");
    listResources.push_back(resName);
    CLog::Log(logINFO, "JSONHandler: found resource on Transifex server: %s", resName.c_str());
  };
  CLog::Log(logINFO, "JSONHandler: Found %i resources at Transifex server", listResources.size());
  return listResources;
};

std::list<std::string> CJSONHandler::ParseAvailLanguagesTX(std::string strJSON, bool bIsXBMCCore)
{
  Json::Value root;   // will contains the root value after parsing.
  Json::Reader reader;
  std::string lang;
  std::list<std::string> listLangs;

  bool parsingSuccessful = reader.parse(strJSON, root );
  if ( !parsingSuccessful )
  {
    CLog::Log(logERROR, "CJSONHandler::ParseAvailLanguagesTX: no valid JSON data");
    return listLangs;
  }

  const Json::Value langs = root;
  std::string strLangsToFetch;
  std::string strLangsToDrop;

  for(Json::ValueIterator itr = langs.begin() ; itr != langs.end() ; itr++)
  {
    lang = itr.key().asString();
    if (lang == "unknown")
      CLog::Log(logERROR, "JSONHandler: ParseLangs: no language code in json data. json string:\n %s", strJSON.c_str());

    Json::Value valu = *itr;
    std::string strCompletedPerc = valu.get("completed", "unknown").asString();

    // we only add language codes to the list which has a minimum ready percentage defined in the xml file
    // we make an exception with all English derived languages, as they can have only a few srings changed
    if (lang.find("en_") != std::string::npos || strtol(&strCompletedPerc[0], NULL, 10) > g_Settings.GetMinCompletion()-1 || !bIsXBMCCore)
    {
      strLangsToFetch += lang + ": " + strCompletedPerc + ", ";
      listLangs.push_back(lang);
    }
    else
      strLangsToDrop += lang + ": " + strCompletedPerc + ", ";
  };
  CLog::Log(logINFO, "JSONHandler: ParseAvailLangs: Languages to be Fetcehed: %s", strLangsToFetch.c_str());
  CLog::Log(logINFO, "JSONHandler: ParseAvailLangs: Languages to be Dropped (not enough completion): %s",
            strLangsToDrop.c_str());
  return listLangs;
};

std::list<std::string> CJSONHandler::ParseAvailLanguagesGITHUB(std::string strJSON)
{
  Json::Value root;   // will contains the root value after parsing.
  Json::Reader reader;
  std::string lang;
  std::list<std::string> listLangs;

  bool parsingSuccessful = reader.parse(strJSON, root );
  if ( !parsingSuccessful )
    CLog::Log(logERROR, "CJSONHandler::ParseAvailLanguagesGITHUB: no valid JSON data downloaded from Github");

  const Json::Value JLangs = root;

  for(Json::ValueIterator itr = JLangs.begin() ; itr !=JLangs.end() ; itr++)
  {
    Json::Value JValu = *itr;
    std::string strType =JValu.get("type", "unknown").asString();
    if (strType == "unknown")
      CLog::Log(logERROR, "CJSONHandler::ParseAvailLanguagesGITHUB: no valid JSON data downloaded from Github");
    else if (strType != "dir")
    {
      CLog::Log(logWARNING, "CJSONHandler::ParseAvailLanguagesGITHUB: unknown file found in language directory");
      continue;
    }
    lang =JValu.get("name", "unknown").asString();
    if (strType == "unknown")
      CLog::Log(logERROR, "CJSONHandler::ParseAvailLanguagesGITHUB: no valid JSON data downloaded from Github");
    listLangs.push_back(g_LCodeHandler.FindLangCode(lang));
  };

  return listLangs;
};

std::map<std::string, CLangcodes> CJSONHandler::ParseTransifexLanguageDatabase(std::string strJSON)
{
  Json::Value root;   // will contains the root value after parsing.
  Json::Reader reader;
  std::string lang;

  bool parsingSuccessful = reader.parse(strJSON, root );
  if ( !parsingSuccessful )
  {
    CLog::Log(logERROR, "JSONHandler: ParseTXLanguageDB: no valid JSON data");
  }

  std::map<std::string, CLangcodes> mapTXLangs;

  const Json::Value JLangs = root;

  for(Json::ValueIterator itr = JLangs.begin() ; itr !=JLangs.end() ; itr++)
  {
    Json::Value JValu = *itr;
    const Json::Value JFields =JValu.get("fields", "unknown");

    CLangcodes LangData;
    LangData.Langcode = JFields.get("code", "unknown").asString();
    LangData.Langname = JFields.get("name", "unknown").asString();
    LangData.Pluralform = JFields.get("pluralequation", "unknown").asString();
    LangData.nplurals = JFields.get("nplurals", 0).asInt();
    if (LangData.Langcode != "unknown" && LangData.Langname != "unknown")
      mapTXLangs[LangData.Langcode] = LangData;
    else
      CLog::Log(logWARNING, "JSONHandler: ParseTXLanguageDB: corrupt JSON data found while parsing Language Database from Transifex");
  };
  return mapTXLangs;
};

std::string CJSONHandler::CreateJSONStrFromPOStr(std::string const &strPO)
{
  Json::Value root;
  root["content"] = strPO;
  root["mimetype"] = std::string("text/x-po");
  Json::StyledWriter writer;
  std::string strJSON = writer.write(root);
  return strJSON;
};

std::string CJSONHandler::CreateNewresJSONStrFromPOStr(std::string strTXResname, std::string const &strPO)
{
  Json::Value root;
  root["content"] = strPO;
  root["slug"] = std::string(strTXResname);
  root["name"] = std::string(strTXResname);
  root["i18n_type"] = std::string("PO");
  Json::StyledWriter writer;
  std::string strJSON = writer.write(root);
  return strJSON;
};

void CJSONHandler::ParseUploadedStringsData(std::string const &strJSON, size_t &stradded, size_t &strupd)
{
  Json::Value root;   // will contains the root value after parsing.
  Json::Reader reader;

  bool parsingSuccessful = reader.parse(strJSON, root );
  if ( !parsingSuccessful )
  {
    CLog::Log(logERROR, "JSONHandler::ParseUploadedStringsData: Parse upload tx server response: no valid JSON data");
    return;
  }

  stradded = root.get("strings_added", 0).asInt();
  strupd = root.get("strings_updated", 0).asInt();
  return;
};

void CJSONHandler::ParseUploadedStrForNewRes(std::string const &strJSON, size_t &stradded)
{
  Json::Value root;   // will contains the root value after parsing.
  Json::Reader reader;

  bool parsingSuccessful = reader.parse(strJSON, root );
  if ( !parsingSuccessful )
  {
    CLog::Log(logERROR, "JSONHandler::ParseUploadedStrForNewRes: Parse upload tx server response: no valid JSON data");
    return;
  }

  stradded = root[0].asInt();

  return;
};

std::string CJSONHandler::ParseLongProjectName(std::string const &strJSON)
{
  Json::Value root;   // will contains the root value after parsing.
  Json::Reader reader;

  bool parsingSuccessful = reader.parse(strJSON, root );
  if ( !parsingSuccessful )
  {
    CLog::Log(logERROR, "JSONHandler::ParseLongProjectName: no valid JSON data");
    return "";
  }

  return root.get("name", "").asString();
}