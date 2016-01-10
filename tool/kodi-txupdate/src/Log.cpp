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

#include "Log.h"
#include "FileUtils.h"

static int m_numWarnings;

using namespace std;

CLog::CLog()
{
  m_numWarnings = 0;
}

CLog::~CLog()
{}

void CLog::Log(TLogLevel loglevel, const char *format, ... )
{
  if (loglevel == logWARNING)
    m_numWarnings++;

  std::string strLogType;

  va_list va;
  va_start(va, format);

  std::string strFormat = format;

  if (loglevel == logPRINT)
    vprintf(strFormat.c_str(), va);

  else if (loglevel == logWARNING)
  {
    printf("\n%sWarning log message:", KRED);
    vprintf(strFormat.c_str(), va);
    printf("%s\n\n", RESET);
  }
  else if (loglevel == logERROR)
  {
    printf("\n%sError message thrown:", KRED);
    vprintf(strFormat.c_str(), va);
    printf("%s\n\n", RESET);
    throw 1;
  }
  else if (loglevel == LogHEADLINE)
  {
    printf("\n%s", KGRN);
    std::string strHeader;
    strHeader.assign(strFormat.size()-1, '-');
    strHeader += "\n";
    printf("%s", strHeader.c_str());
    vprintf(strFormat.c_str(), va);
    printf("%s", strHeader.c_str());
    printf("%s\n", RESET);
  }

  va_end(va);

  return;
};

void CLog::ResetWarnCounter()
{
  m_numWarnings = 0;
};

int CLog::GetWarnCount()
{
  return m_numWarnings;
};
