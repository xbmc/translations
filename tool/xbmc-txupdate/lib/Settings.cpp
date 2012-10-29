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

#include "Settings.h"

CSettings g_Settings;

using namespace std;

CSettings::CSettings()
{
  m_CacheExpire = DEFAULTCACHEEXPIRE;
  m_minComplPercentage = DEFAULTMINCOMPLETION;
  m_strMergedLangfilesDir = DEFAULTMERGEDLANGDIR;
  m_strTXUpdateLangfilesDir = DEFAULTTXUPDLANGDIR;
  m_bForceComm = false;
  m_strSupportEmailAdd = "anonymus";
};

CSettings::~CSettings()
{
};

void CSettings::SetProjectname(string strName)
{
  m_strProjectName = strName;
};

string CSettings::GetProjectname()
{
  return m_strProjectName;
};

void CSettings::SetHTTPCacheExpire(size_t exptime)
{
  m_CacheExpire = exptime;
};

size_t CSettings::GetHTTPCacheExpire()
{
  return m_CacheExpire;
};

void CSettings::SetMinCompletion(int complperc)
{
  m_minComplPercentage = complperc;
};

int CSettings::GetMinCompletion()
{
  return m_minComplPercentage;
};

void CSettings::SetMergedLangfilesDir(std::string const &strMergedLangfilesDir)
{
  m_strMergedLangfilesDir = strMergedLangfilesDir;
};

std::string CSettings::GetMergedLangfilesDir()
{
  return m_strMergedLangfilesDir;
};

void CSettings::SetSupportEmailAdd(std::string const &strEmailAdd)
{
  m_strSupportEmailAdd = strEmailAdd;
}

std::string CSettings::GetSupportEmailAdd()
{
  return m_strSupportEmailAdd;
};

void CSettings::SetTXUpdateLangfilesDir(std::string const &strTXUpdateLangfilesDir)
{
  m_strTXUpdateLangfilesDir = strTXUpdateLangfilesDir;
};

std::string CSettings::GetTXUpdateLangfilesDir()
{
  return m_strTXUpdateLangfilesDir;
};

void CSettings::SetForcePOComments(bool bForceComm)
{
  m_bForceComm = bForceComm;
};

bool CSettings::GetForcePOComments()
{
  return m_bForceComm;
};
