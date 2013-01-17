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
using System.Windows.Forms;

using XBMC.International;

namespace SimplePOViewerXBMC
{
    public partial class frmEvalID : Form
    {
        private Dictionary<string,LanguageInfo> lng = null;
        public frmEvalID()
        {
            InitializeComponent();
        }
        public frmEvalID(Dictionary<string,LanguageInfo> languages)
        {
            InitializeComponent();
            lng = languages;
        }
        private void Form2_Load(object sender, EventArgs e)
        {

        }

        private void Print(string text)
        {
            listBox1.Items.Add(text);
            Console.WriteLine(text);
        }

        private void textBox1_KeyPress(object sender, KeyPressEventArgs e)
        {
            if (e.KeyChar == '\r')
            {
                string idtext = textBox1.Text;

                if (idtext.Trim().Length > 0)
                {
                    try
                    {
                        Print("Localization strings :" + idtext);
                        string[] ids = textBox1.Text.Split(new char[] { ' ' });
                        foreach (LanguageInfo l in lng.Values)
                        {
                            string result = string.Empty;
                            foreach (string id in ids)
                            {
                                int i = 0;
                                if (int.TryParse(id, out i))
                                {
                                    if (l.Text.ContainsKey(i) == true)
                                        if (l.Text[i].Text.Length > 0)
                                        {
                                            result = result + " " + l.Text[i].Text;
                                        }
                                        else
                                        {
                                            if (l.Text[i].Key.Length > 0)
                                            {
                                                result = result + " " + l.Text[i].Key;
                                            }
                                            else
                                            {
                                                result = result + " " + lng["English"].Text[i].Key;
                                            }
                                        }
                                }
                            }
                            Print(l.RevisionInfo["Language"].Replace(@"\n", "") + ": " + result);
                        }
                    }
                    catch (Exception ex)
                    {
                        MessageBox.Show(ex.Message);
                    }
                }
            }
        }

        private void copyToClipboardToolStripMenuItem_Click(object sender, EventArgs e)
        {
            string paste = string.Empty;
            foreach (string t in listBox1.Items)
                paste += t + Environment.NewLine;
            Clipboard.SetText(paste, TextDataFormat.UnicodeText);
        }
    }
}
