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

#include "POHandler.h"
#include "AddonXMLHandler.h"
#include "UpdateXMLHandler.h"
#include <list>

typedef std::map<std::string, CPOHandler>::iterator T_itmapPOFiles;

class CResourceHandler
{
public:
  CResourceHandler();
  ~CResourceHandler();
  bool FetchPOFilesTXToMem(std::string strURL, bool bIsXBMCCore);
  bool FetchPOFilesUpstreamToMem(CXMLResdata XMLResdata, std::list<std::string> listLangsAll);
  bool WritePOToFiles(std::string strProjRootDir, std::string strPrefixDir, std::string strResName, CXMLResdata XMLResdata, bool bTXUpdFile);
  size_t GetLangsCount() const {return m_mapPOFiles.size();}
  std::string GetLangCodeFromPos(size_t pos) {T_itmapPOFiles it = IterateToMapIndex (m_mapPOFiles.begin(), pos); return it->first;}
  CPOHandler* GetPOData(std::string strLang);
  void AddPOData(CPOHandler &POHandler, std::string strLang) {m_mapPOFiles[strLang] = POHandler;}
  CAddonXMLHandler * GetXMLHandler () {return &m_AddonXMLHandler;}
  void SetXMLHandler (CAddonXMLHandler XMLHandler) {m_AddonXMLHandler = XMLHandler;}

protected:
  void CreateMissingDirs(std::string strResRootDir, int resType);
  T_itmapPOFiles IterateToMapIndex(T_itmapPOFiles it, size_t index);

  std::map<std::string, CPOHandler> m_mapPOFiles;
  CAddonXMLHandler m_AddonXMLHandler;
};
