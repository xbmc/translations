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

namespace SimplePOViewerXBMC
{
    public partial class frmPrefs : Form
    {
        private Preferences preferences = null;
        public frmPrefs(Preferences p, List<string> languages)
        {
            InitializeComponent();
            preferences = p;

            preferences.Restore();

            foreach (string language in languages)
            {
                if (language != "English")
                lsbAllLanguages.Items.Add(language);
            }

            foreach (string language in preferences.LoadOnStartup)
            {
                lsbOnStartup.Items.Add(language);
            }

            txtFolder.Text = preferences.RootFolder;

        }

        internal frmPrefs()
        {
            ;
        }

        private void cmdBrowse_Click(object sender, EventArgs e)
        {
            folderBrowserDialog1.ShowDialog();
            string path = folderBrowserDialog1.SelectedPath;

            if (path.Length > 0)
            {
                txtFolder.Text = path;
            }
            
        }

        private void cmdOk_Click(object sender, EventArgs e)
        {
            try
            {
                preferences.LoadOnStartup.Clear();
                foreach (string language in lsbOnStartup.Items)
                {
                    preferences.LoadOnStartup.Add(language);
                }

                preferences.Save();

            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message);
            }

            this.Close();
        }

        private void cmdCancel_Click(object sender, EventArgs e)
        {
            this.Close();
        }

        private void cmdAdd_Click(object sender, EventArgs e)
        {
            if (lsbAllLanguages.SelectedIndex > -1)
            {
                string language = lsbAllLanguages.Items[lsbAllLanguages.SelectedIndex].ToString();
                if (lsbOnStartup.Items.Contains(language) == false)
                {
                    lsbOnStartup.Items.Add(language);                    
                }
                lsbAllLanguages.SelectedIndex = -1;
            }
        }

        private void cmdRemove_Click(object sender, EventArgs e)
        {
            if (lsbOnStartup.SelectedIndex > -1)
            {
                string language = lsbOnStartup.Items[lsbOnStartup.SelectedIndex].ToString();
                lsbOnStartup.Items.Remove(language);
                lsbOnStartup.SelectedIndex = -1;
            }
        }

        private void cmdMoveUp_Click(object sender, EventArgs e)
        {
            MoveListBoxItem(lsbOnStartup, -1);
        }

        private void cmdMoveDown_Click(object sender, EventArgs e)
        {
            MoveListBoxItem(lsbOnStartup, 1);
        }

        private void txtFolder_TextChanged(object sender, EventArgs e)
        {
            preferences.RootFolder = ((TextBox)sender).Text;
        }

        private void MoveListBoxItem(ListBox list, int direction)
        {
            if (list.SelectedItem == null || list.SelectedIndex < 0) return; 
            int index = list.SelectedIndex + direction;
            if (index < 0 || index >= list.Items.Count) return; 
            object selected = list.SelectedItem;
            list.Items.Remove(selected);
            list.Items.Insert(index, selected);
            list.SetSelected(index, true);
        }
    }
}
