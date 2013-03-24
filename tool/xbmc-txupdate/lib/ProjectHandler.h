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

#include "ResourceHandler.h"
#include "UpdateXMLHandler.h"
#include <list>

struct CresourceAvail
{
  bool bhasUpstream;
  bool bhasonTX;
  bool bhasLocal;
};

class CProjectHandler
{
public:
  CProjectHandler();
  ~CProjectHandler();
  bool FetchResourcesFromTransifex();
  bool FetchResourcesFromUpstream();
  bool CreateMergedResources();
  bool WriteResourcesToFile(std::string strProjRootDir);
  void InitUpdateXMLHandler(std::string strProjRootDir);
  void UploadTXUpdateFiles(std::string strProjRootDir);

protected:
  const CPOEntry * SafeGetPOEntry(std::map<std::string, CResourceHandler> &mapResHandl, const std::string &strResname,
                                  std::string &strLangCode, size_t numID);
  const CPOEntry * SafeGetPOEntry(std::map<std::string, CResourceHandler> &mapResHandl, const std::string &strResname,
                                  std::string &strLangCode, CPOEntry const &currPOEntryEN);
  CPOHandler * SafeGetPOHandler(std::map<std::string, CResourceHandler> &mapResHandl, const std::string &strResname,
                                std::string &strLangCode);
  std::list<std::string> CreateMergedLanguageList(std::string strResname, bool bOnlyTX);
  std::map<std::string, CResourceHandler> * ChoosePrefResMap(std::string strResname);
  std::list<std::string> CreateResourceList();
  CAddonXMLEntry * const GetAddonDataFromXML(std::map<std::string, CResourceHandler> * pmapRes,
                                             const std::string &strResname, const std::string &strLangCode) const;
  void MergeAddonXMLEntry(CAddonXMLEntry const &EntryToMerge, CAddonXMLEntry &MergedAddonXMLEntry,
                                           CAddonXMLEntry const &SourceENEntry, CAddonXMLEntry const &CurrENEntry);
  bool FindResInList(std::list<std::string> const &listResourceNamesTX, std::string strTXResName);
  std::list<std::string> GetLangsFromDir(std::string const &strLangDir);
  void CheckPOEntrySyntax(const CPOEntry * pPOEntry, std::string const &strLangCode, const CPOEntry * pcurrPOEntryEN);
  std::string GetEntryContent(const CPOEntry * pPOEntry, std::string const &strLangCode);
  void CheckCharCount(const CPOEntry * pPOEntry, std::string const &strLangCode, const CPOEntry * pcurrPOEntryEN, char chrToCheck);

  std::map<std::string, CResourceHandler> m_mapResourcesTX;
  std::map<std::string, CResourceHandler> m_mapResourcesUpstr;

  std::map<std::string, CResourceHandler> m_mapResMerged;
  std::map<std::string, CResourceHandler> m_mapResUpdateTX;
  typedef std::map<std::string, CResourceHandler>::iterator T_itmapRes;
  std::map<std::string, std::string> m_mapResourceNamesTX;
  int m_resCount;
  CUpdateXMLHandler m_UpdateXMLHandler;
};
