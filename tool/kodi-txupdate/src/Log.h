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

#include <stdarg.h>
#include <stdio.h>
#include <string>
#include <map>

using namespace std;

enum TLogLevel { logPRINT, logERROR, logWARNING, logDEBUG, LogHEADLINE};

const std::string VERSION = "0.650";

// std::out colorcodes
#define KNRM  "\x1B[0m"
#define KRED  "\x1B[31m"
#define KGRN  "\x1B[32m"
#define KORNG "\x1B[33m"
#define KBLU  "\x1B[34m"
#define KMAG  "\x1B[35m"
#define KCYN  "\x1B[36m"
#define KLGRAY "\x1B[37m"
#define KGRAY  "\x1B[1;30m"
#define KLRED  "\x1B[1;31m"
#define KLGRN  "\x1B[1;32m"
#define KYEL   "\x1B[1;33m"
#define KLBLU  "\x1B[1;34m"
#define KLMAG  "\x1B[1;35m"
#define KLCYN  "\x1B[1;36m"
#define KLWHT  "\x1B[1;37m"
#define RESET "\033[0m"

class CLog
{
public:
  CLog();
  ~CLog();
  static void Log(TLogLevel loglevel, const char *format, ... );
  static void ResetWarnCounter();
  static int GetWarnCount();
};
