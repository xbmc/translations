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
#include "FileUtils/FileUtils.h"

static FILE * m_pLogFile;
static int m_numWarnings;
static int m_ident;
static std::map<std::string, std::string> m_mapStrResultTable;

using namespace std;

CLog::CLog()
{
  m_numWarnings = 0;
  m_ident = 0;
}

CLog::~CLog()
{}

bool CLog::Init(std::string logfile)
{
   m_pLogFile = fopen (logfile.c_str(),"wb");
  if (m_pLogFile == NULL)
  {
    fclose(m_pLogFile);
    printf("Error creating logfile: %s\n", logfile.c_str());
    return false;
  }
  fprintf(m_pLogFile, "XBMC-TXUPDATE v%s Logfile\n\n", VERSION.c_str());

  return true;
};

void CLog::Log(TLogLevel loglevel, const char *format, ... )
{
  if (!m_pLogFile)
    return;

  if (loglevel == logLINEFEED)
  {
    fprintf(m_pLogFile, "\n");
    return;
  }

  if (loglevel == logWARNING)
    m_numWarnings++;

  fprintf(m_pLogFile, g_File.GetCurrTime().c_str());
  std::string strLogType;
  fprintf(m_pLogFile, "\t%s\t", listLogTypes[loglevel].c_str());

  va_list va;
  va_start(va, format);

  std::string strFormat = format;
  std::string strIdent;
  strIdent.assign(m_ident, ' ');

  vfprintf(m_pLogFile, (strIdent + strFormat).c_str(), va);
  fprintf(m_pLogFile, "\n");
  va_end(va);

  if (loglevel == logERROR || loglevel == logWARNING)
  {
    va_list va1;
    va_start(va1, format);
    char cstrLogMessage[1024];
    vsprintf(cstrLogMessage, format, va1);
    va_end(va1);
    if (loglevel == logERROR)
    {
      printf ("\nError message thrown: %s\n\n", cstrLogMessage);
      throw 1;
    }
    else
      printf ("\nWarning log message: %s\n\n", cstrLogMessage);
  }

  return;
};

void CLog::LogTable(TLogLevel loglevel, std::string strTableName, const char *format, ... )
{
  if (loglevel == logADDTABLEHEADER)
  {
    std::string strIdent;
    strIdent.assign(m_ident, ' ');
    std::string strHeader = format;

    m_mapStrResultTable[strTableName] = g_File.GetCurrTime() + "\t" + listLogTypes[logINFO] + "\t" + strIdent +
                                         strHeader + m_mapStrResultTable[strTableName];
    return;
  }

  if (loglevel == logCLOSETABLE)
  {
    std::string strIdent;
    strIdent.assign(m_ident, ' ');

    fprintf(m_pLogFile, "%s", (m_mapStrResultTable[strTableName]).c_str());
    m_mapStrResultTable.erase(m_mapStrResultTable.find(strTableName));
    return;
  }

  m_mapStrResultTable[strTableName] += g_File.GetCurrTime();
  m_mapStrResultTable[strTableName] += "\t" + listLogTypes[loglevel] + "\t";

  va_list va;
  va_start(va, format);

  std::string strFormat = format;
  std::string strIdent;
  strIdent.assign(m_ident, ' ');

  char cstrLogMessage[2048];
  vsprintf(cstrLogMessage, (strIdent + strFormat + "\n").c_str(), va);
  va_end(va);

  m_mapStrResultTable[strTableName] += cstrLogMessage;
  return;
}

void CLog::Close()
{
  if (m_pLogFile)
    fclose(m_pLogFile);
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