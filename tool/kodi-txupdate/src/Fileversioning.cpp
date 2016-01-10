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

#include "Fileversioning.h"
#include "HTTPUtils.h"

CFileversion g_Fileversion;

using namespace std;

CFileversion::CFileversion()
{
  m_mapVersions.clear();
};

CFileversion::~CFileversion()
{
};

void CFileversion::SetVersionForURL(const string& strURL, const string& strVersion)
{
//  CLog::Log(logPRINT, "%s:%s\n", strURL.c_str(), strVersion.c_str());
  m_mapVersions[strURL] = strVersion;
}

std::string CFileversion::GetVersionForURL(const string& strURL)
{
  if (m_mapVersions.find(strURL) != m_mapVersions.end())
  {
    return m_mapVersions[strURL];
  }
  return "";
}
