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
#pragma once

#include "POHandler.h"
#include "AddonXMLHandler.h"
#include "ConfigHandler.h"
#include <list>

typedef std::map<std::string, CPOHandler>::iterator T_itmapPOFiles;
typedef std::map <unsigned long long, CPOEntry>::iterator T_itPOData;

class CResourceHandler
{
public:
  CResourceHandler();
  CResourceHandler(const CResData& Resdata);
  ~CResourceHandler();
  bool FetchPOFilesTXToMem();
  bool FetchPOFilesUpstreamToMem();
  void MergeResource();

  void GenerateMergedPOFiles();
  void GenerateUpdatePOFiles();
  void WriteMergedPOFiles(const std::string& sAddonXMLPath, const std::string& sLangAddonXMLPath, const std::string& sChangeLogPath, const std::string& sLangPath);
  void WriteLOCPOFiles(CCommitData& CommitData, CCommitData& CommitDataSRC);
  void WriteUpdatePOFiles(const std::string& strPath);

  bool FindUPSEntry(const CPOEntry &EntryToFind);
  bool FindSRCEntry(const CPOEntry &EntryToFind);

  void UploadResourceToTransifex(bool bNewResourceOnTRX);

protected:
  bool ComparePOFiles(CPOHandler& POHandler1, CPOHandler& POHandler2);
  std::list<std::string> ParseAvailLanguagesTX(std::string strJSON, const std::string &strURL);
  std::set<std::string> GetAvailLangsGITHUB();
  void GetSRCFilesGitData();
  std::list<std::string> CreateMergedLangList();
  bool FindUPSEntry(const std::string sLCode, CPOEntry &EntryToFind);
  bool FindTRXEntry(const std::string sLCode, CPOEntry &EntryToFind);
  T_itPOData GetUPSItFoundEntry();
  T_itPOData GetTRXItFoundEntry();
  void PrintChangedLangs(const std::set<std::string>& lChangedLangs);

  std::map<std::string, CPOHandler> m_mapUPS, m_mapTRX, m_mapUPD, m_mapMRG;

  CAddonXMLHandler m_AddonXMLHandler;
  std::set<std::string> m_lChangedLangsFromUPS, m_lLangsToUPD;

  CResData m_ResData;

  bool m_bResChangedFromUPS;
  bool m_bLastUPSHandlerFound;
  bool m_bLastTRXHandlerFound;
  T_itmapPOFiles m_lastUPSIterator;
  T_itmapPOFiles m_lastTRXIterator;
  std::string m_lastUPSLCode;
  std::string m_lastTRXLCode;
};
