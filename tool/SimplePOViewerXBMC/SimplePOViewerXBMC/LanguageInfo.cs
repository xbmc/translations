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
using System.Collections.Generic;
using System.Linq;
using System.IO;

namespace XBMC.International
{
    public struct TextResource
    {
        public readonly int NumId;
        public readonly string Key;
        public readonly string Text;
        public readonly string Comment;
        public TextResource(int numid, string key, string text, string comment)
        {
            NumId = numid;
            Key = key;
            Text = text;
            Comment = comment;
        }
    }

    public class LanguageInfo
    {
        private Dictionary<string, string> revision_info = new Dictionary<string, string>();
        private Dictionary<int, TextResource> map = new Dictionary<int, TextResource>();

        public Dictionary<int, TextResource> Text { get { return map; } }
        public Dictionary<string, string> RevisionInfo { get { return revision_info; } }

        public LanguageInfo(string file)
        {
            LoadPO(file);
        }
        public void Load(string file)
        {
            LoadPO(file, true);
        }

        private void LoadPO(string file, bool ignore_revision = false)
        {
            int line = 0;
            int strings = 0;
            string comment = string.Empty;
            string comment_running = comment;
            int min = int.MaxValue;
            int max = int.MinValue;

            string[] folders = System.IO.Path.GetDirectoryName(file).Split(System.IO.Path.DirectorySeparatorChar);
            string language = folders[folders.Count() - 1];

            string header = "msgid \"\"\r\nmsgstr \"\"";
            StreamReader po = new FileInfo(file).OpenText();
            string h = po.ReadLine().Trim(); line += 1;

            Console.Write("Reading '" + file + "' ... ");

            while (h.StartsWith("#"))
            {
                h = po.ReadLine().Trim(); line += 1;
            }

            h += "\r\n" + po.ReadLine(); line += 1;

            if (h == header)
            {
                h = po.ReadLine(); line += 1;
                while (h.Length > 0)
                {
                    string name = h.Substring(0, h.IndexOf(":"));
                    string value = h.Substring(h.IndexOf(":") + 1).Trim();
                    if (name[0] == '\"') name = name.Substring(1);
                    if (value[value.Length - 1] == '\"') value = value.Substring(0, value.Length - 1);
                    if (ignore_revision == false) revision_info.Add(name, value);
                    h = po.ReadLine(); line += 1;
                }
                if (ignore_revision == false) revision_info.Add("Language-Name", language);
                string numid = "";
                string key;
                string text;

                while (po.EndOfStream == false)
                {
                    numid = po.ReadLine().Trim(); line += 1;

                    if (numid.StartsWith("msgctxt \"#") == true)
                    {
                        comment = comment_running;
                        comment_running = string.Empty;

                        numid = numid.Substring("msgctxt \"#".Length);
                        if (numid.EndsWith("\"")) numid = numid.Substring(0, numid.Length - 1);


                        key = po.ReadLine().Trim(); line += 1;
                        if (key.StartsWith("msgid \"") == true)
                        {
                            key = key.Substring("msgid \"".Length);
                            if (key.EndsWith("\"")) key = key.Substring(0, key.Length - 1);
                        }
                        else
                            throw new FileLoadException(string.Format("Expecting textual message id on line {0} in file {1}.", line, file));

                        text = po.ReadLine().Trim(); line += 1;
                        if (text.StartsWith("msgstr \"") == true)
                        {
                            text = text.Substring("msgstr \"".Length);
                            if (text.EndsWith("\"")) text = text.Substring(0, text.Length - 1);
                        }
                        else
                            throw new FileLoadException(string.Format("Expecting resource message on line {0} in file {1}.", line, file));

                        string empty = po.ReadLine(); line += 1;

                        int id = 0;
                        if (int.TryParse(numid, out id) == false)
                            throw new FileLoadException(string.Format("Expecting numeric message id on line {0} in file {1}.", line, file));

                        try
                        {
                            if (id > max) max = id;
                            if (id < min) min = id; 
                            map.Add(id, new TextResource(id, key, text, comment));
                            comment = string.Empty;
                            strings++;
                        }
                        catch (ArgumentException ex)
                        {
                            throw new FileLoadException(string.Format("Duplicate message id on line {0} in file {1}. {2}", line, file, ex));
                        }


                    }
                    else
                        if (numid.Length > 0 && numid.StartsWith("#") == false)
                        {
                            throw new FileLoadException(string.Format("Expecting numeric message id on line {0} in file {1}.", line, file));
                        }
                        else
                        {
                            if (numid.Length > 0 && numid.StartsWith("#empty string") == false)
                            {                                
                                comment_running += numid;
                            }
                        }
                }

                Console.WriteLine(strings.ToString() + " string(s) [" + min.ToString() + " thru " + max.ToString() + "]");
            }
            else
            {
                throw new FileLoadException(string.Format("Expecting message header magic on line {0} in file {1}.", line, file));
            }
        }
    }
}
