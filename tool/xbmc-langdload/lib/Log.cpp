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

#include "Log.h"
#include "FileUtils.h"

static int m_ident;
static int m_numWarnings;

using namespace std;

CLog::CLog()
{
  m_ident = 0;
  printf("XBMC-TXUPDATE v%s Logfile\n\n", VERSION.c_str());
}

CLog::~CLog()
{}

void CLog::Log(TLogLevel loglevel, const char *format, ... )
{

  if (loglevel == logLINEFEED)
  {
    printf("\n");
    return;
  }

  if (loglevel == logWARNING)
    m_numWarnings++;

  printf(g_File.GetCurrTime().c_str());
  std::string strLogType;
  printf("\t%s\t", listLogTypes[loglevel].c_str());

  va_list va;
  va_start(va, format);

  std::string strFormat = format;
  std::string strIdent;
  strIdent.assign(m_ident, ' ');

  vprintf((strIdent + strFormat).c_str(), va);
  printf("\n");
  va_end(va);

  if (loglevel == logERROR)
    throw 1;

  return;
};

void CLog::IncIdent(int numident)
{
  m_ident += numident;
}

void CLog::DecIdent(int numident)
{
  if ((m_ident +1) > numident)
    m_ident -= numident;
}

void CLog::ClearIdent()
{
  m_ident = 0;
}

void CLog::ResetWarnCounter()
{
  m_numWarnings = 0;
};

int CLog::GetWarnCount()
{
  return m_numWarnings;
};