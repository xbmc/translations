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

#include <stdarg.h>
#include <stdio.h>
#include <string>
#include <map>

enum TLogLevel { logERROR, logWARNING, logINFO, logDEBUG, logLINEFEED, logCLOSETABLE, logADDTABLEHEADER};

const std::string listLogTypes[] = {"ERROR", "WARNING", "INFO", "DEBUG"};
const std::string VERSION = "0.8";

struct CLogIdent
{
  std::string logPrefix;
  int ident;
};

class CLog
{
public:
  CLog();
  ~CLog();
  static void Log(TLogLevel loglevel, const char *format, ... );
  static void IncIdent(int numident);
  static void DecIdent(int numident);
  static void ClearIdent();
  static void ResetWarnCounter();
  static int GetWarnCount();
};
