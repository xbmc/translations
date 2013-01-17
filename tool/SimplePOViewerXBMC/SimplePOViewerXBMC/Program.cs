/*
 *      Copyright (C) 2012 Team XBMC
 *      http://xbmc.org
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

using System;
using System.Windows.Forms;

namespace SimplePOViewerXBMC
{
    static class Program
    {
        /// <summary>
        /// The main entry point for the application.
        /// </summary>
        [STAThread]
        static void Main(string[] args)
        {
            Preferences prefs = new Preferences();
            prefs.Restore();

            string xbmc = Environment.CurrentDirectory;

            if (args.Length == 0)
            {
                string env = Environment.GetEnvironmentVariable("XBMC_ROOT");
                if (env == null)
                {
                    if (prefs.RootFolder.Length > 0)
                    {
                        xbmc = prefs.RootFolder;
                    }
                    else
                    {
                        ; // try the current directory
                    }
                }
                else
                {
                    xbmc = env;
                }
            }
            else
            {
                xbmc = args[0];
            }

            System.IO.DirectoryInfo root = new System.IO.DirectoryInfo(xbmc);
            Application.EnableVisualStyles();
            Application.SetCompatibleTextRenderingDefault(false);
            Application.Run(new frmMain(root, prefs));
        }
    }
}
