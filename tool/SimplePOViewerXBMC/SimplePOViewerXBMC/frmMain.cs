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
using System.Linq;
using System.IO;
using System.Collections.Generic;
using System.ComponentModel;
using System.Windows.Forms;

using XBMC.International;

namespace SimplePOViewerXBMC
{
    public partial class frmMain : Form
    {

        private DirectoryInfo xbmc = null;
        private Preferences preferences = null;

        private bool self_init = true;
        private int sorted = -1;
        private bool filterpercent = false;
        private bool filter_changed = false;

        private Dictionary<string, LanguageInfo> languages = new Dictionary<string, LanguageInfo>();


        string GetFileStringsPO(string addon, string Language)
        {
            string root_folder = string.Empty;

            if (addon == string.Empty)
            {
                root_folder = "language";
            }
            else
            {
                root_folder = System.IO.Path.Combine("addons", addon);
            }

            try
            {
                DirectoryInfo language = null;
                DirectoryInfo en_folder = null;

                // specificy path to strings.po without using platform path specifier (and assume case-sensitive filenames)
                if (addon == string.Empty)
                {
                    language = xbmc.GetDirectories(root_folder).First();
                }
                else
                {
                    language = new DirectoryInfo(System.IO.Path.Combine(xbmc.FullName, root_folder, "language"));
                    if (language.Exists == false) language = new DirectoryInfo(System.IO.Path.Combine(xbmc.FullName, root_folder, "resources", "language"));
                }

                en_folder = language.GetDirectories(Language).First();
                FileInfo[] strings_po = en_folder.GetFiles("strings.po");

                return strings_po[0].FullName;
            }
            catch
            {
                throw new FileNotFoundException(string.Format("Cannot find the language file [{2}] for {0} in XBMC_ROOT: {1}", Language, xbmc.FullName, root_folder));
            }
        }

        string GetFileStringsPO(string Language)
        {
            return GetFileStringsPO(string.Empty, Language);
        }

        private LanguageInfo LoadPO(string language, string addon_resource)
        {
            LanguageInfo lng = new LanguageInfo(GetFileStringsPO(language));
            lng.Load(GetFileStringsPO(addon_resource, language));

            if (language == "English")
            {
                foreach (TextResource t in lng.Text.Values)
                {
                    if (t.Text.Length > 0 && t.Text != t.Key)
                    {
                        MessageBox.Show("There should be no translations in the English message file?");
                        break;
                    }

                }
            }
            return lng;
        }

        private void AddLanguageToView(string addon_resource, string language, ListView list)
        {
            try
            {
                if (languages.ContainsKey(language))
                {
                    languages[language] = LoadPO(language, addon_resource);
                }
                else
                {
                    ColumnHeader[] ch = new ColumnHeader[] { new ColumnHeader() };

                    LanguageInfo lng = LoadPO(language, addon_resource);

                    languages.Add(language, lng);

                    ch[0].Text = string.Format("Translation({0})", lng.RevisionInfo["Language"].Replace(@"\n", ""));
                    ch[0].Width = list.Columns[2].Width;
                    list.Columns.AddRange(ch);
                }
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message);
            }
        }

        private void LoadWithAddon(string addon_resource, ListView list, ComboBox set_language)
        {
            try
            {
                list.Items.Clear();
                list.ListViewItemSorter = null;
                list.Sorting = SortOrder.None;

                if (languages.Count == 0)
                {
                    languages.Add("English", LoadPO("English", addon_resource));
                }
                else
                {
                    foreach (string lng in languages.Keys.ToList())
                    {
                        languages[lng] = LoadPO(lng, addon_resource);
                    }
                }

                set_language.SelectedItem = set_language.Items[set_language.Items.IndexOf("English")];
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message);
            }
        }

        List<string> GetPOAddons()
        {
            List<string> addons = new List<string>();

            // specificy path to strings.po without using platform path specifier (and assume case-sensitive filenames)
            DirectoryInfo[] folders = xbmc.GetDirectories("addons").First().GetDirectories();

            foreach (DirectoryInfo di in folders)
            {
                DirectoryInfo en_folder = null;

                en_folder = new DirectoryInfo(System.IO.Path.Combine(di.FullName, "language", "English"));

                if (en_folder.Exists)
                {
                    FileInfo[] strings_po = en_folder.GetFiles("strings.po");
                    if (strings_po.Count() > 0)
                    {
                        addons.Add(di.Name);
                    }
                }
                else
                {
                    en_folder = new DirectoryInfo(System.IO.Path.Combine(di.FullName, "resources", "language", "English"));

                    if (en_folder.Exists)
                    {
                        FileInfo[] strings_po = en_folder.GetFiles("strings.po");
                        if (strings_po.Count() > 0)
                        {
                            addons.Add(di.Name);
                        }
                    }
                }
            }

            return addons;
        }

        private string ResourceText(Dictionary<int, TextResource> map, int key)
        {
            if (map.ContainsKey(key) == true)
                return map[key].Text;
            else
                return "";
        }

        private void PopulateListView(ListView control)
        {
            try
            {

                bool ignore_case = chkIgnore.Checked;
                string match_text = txtFilter.Text.Trim();

                if (ignore_case) match_text = match_text.ToLower();

                LanguageInfo en = languages["English"];


                if (en != null)
                {
                    control.Items.Clear();
                    control.ListViewItemSorter = null;
                    control.Sorting = SortOrder.None;

                    foreach (TextResource t in en.Text.Values)
                    {
                        bool match = false;
                        if (match_text.Length > 0)
                        {
                            for (int i = 0; i < languages.Count && match == false; i++)
                            {
                                string r;
                                if (i == 0)
                                {
                                    r = t.Key;
                                }
                                else
                                {
                                    r = ResourceText(languages.Values.ElementAt(i).Text, t.NumId);
                                }

                                if (ignore_case)
                                {
                                    if (r.ToLower().Contains(match_text))
                                        match = true;
                                }
                                else
                                {
                                    if (r.Contains(match_text))
                                        match = true;
                                }
                            }
                        }
                        else
                            match = true;

                        if (match)
                        {
                            if (filterpercent)
                            {
                                if (t.Key.Contains('%'))
                                {
                                    ListViewItem lvi = control.Items.Add(t.NumId.ToString());
                                    lvi.SubItems.Add(t.Comment);

                                    lvi.SubItems.Add(t.Key);

                                    for (int i = 1; i < languages.Count; i++)
                                    {
                                        lvi.SubItems.Add(ResourceText(languages.Values.ElementAt(i).Text, t.NumId));

                                    }
                                }
                            }
                            else
                            {
                                ListViewItem lvi = control.Items.Add(t.NumId.ToString());
                                lvi.SubItems.Add(t.Comment);
                                lvi.SubItems.Add(t.Key);

                                for (int i = 1; i < languages.Count; i++)
                                {
                                    lvi.SubItems.Add(ResourceText(languages.Values.ElementAt(i).Text, t.NumId));
                                }
                            }
                        }
                    }
                }
                string status = (filterpercent) ? "Show only items with %" : "Show all items ";
                status += ((match_text.Length > 0) ? " containing '" + txtFilter.Text.Trim() + "'" : "");
                toolStripStatusLabel1.Text = status;
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message);
            }
        }


        private void ExportToFile(string filename)
        {
            try
            {
                ListView lv = listView1;
                StreamWriter sw = new StreamWriter(filename);
                foreach (ListViewItem li in lv.Items)
                {
                    foreach (ListViewItem.ListViewSubItem si in li.SubItems)
                    {
                        sw.Write("\t" + si.Text);
                    }
                    sw.WriteLine();
                }
                sw.Close();
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message);
            }
        }


        #region Form entry points

        public frmMain(DirectoryInfo XBMC_ROOT, Preferences p)
        {
            InitializeComponent();

            xbmc = XBMC_ROOT;
            preferences = p;

        }
        internal frmMain()
        {
            ;
        }

        private void Form1_Load(object sender, EventArgs e)
        {
            try
            {
                // specificy path to strings.po without using platform path specifier (and assume case-sensitive filenames)
                DirectoryInfo[] language = xbmc.GetDirectories("language");
                if (language.Count() == 1)
                {

                    DirectoryInfo[] lang = language[0].GetDirectories();

                    foreach (DirectoryInfo di in lang)
                    {
                        cmbLanguage.Items.Add(di.Name);
                    }

                    List<string> addons = GetPOAddons();

                    foreach (string addon in addons)
                    {
                        cmbAddon.Items.Add(addon);
                    }

                    int index = cmbAddon.Items.IndexOf("skin.confluence");
                    if (index > -1)
                    {
                        cmbAddon.SelectedItem = cmbAddon.Items[index]; // setting the item triggers the LoadWithAddon call
                    }
                    
                    foreach (string lng in preferences.LoadOnStartup)
                    {
                        AddLanguageToView("skin.confluence", lng, listView1);
                    }

                    PopulateListView(listView1);
                    
                }
                else
                {
                    MessageBox.Show("Unable to find the language folder. Did you set XBMC_ROOT or specify the folder on the commandline?");
                }
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message);
            }

            self_init = false;
        }

        private void showOnlyItemsWithToolStripMenuItem_Click(object sender, EventArgs e)
        {
            ToolStripMenuItem ctl = (ToolStripMenuItem)sender;
            ctl.Checked = !ctl.Checked;
            filterpercent = ctl.Checked;

            LoadWithAddon(cmbAddon.SelectedItem.ToString(), listView1, cmbLanguage);

            PopulateListView(listView1);

        }
        private void saveFileDialog1_FileOk(object sender, CancelEventArgs e)
        {
            ExportToFile(((SaveFileDialog)sender).FileName);
        }

        private void exportToolStripMenuItem_Click(object sender, EventArgs e)
        {
            saveFileDialog1.ShowDialog();
        }
        private void button1_Click(object sender, EventArgs e)
        {
            AddLanguageToView(cmbAddon.SelectedItem.ToString(), cmbLanguage.SelectedItem.ToString(), listView1);

            PopulateListView(listView1);
        }

        private void cmbAddon_SelectedIndexChanged(object sender, EventArgs e)
        {
            LoadWithAddon(cmbAddon.SelectedItem.ToString(), listView1, cmbLanguage);
            if (self_init == false)
            {
                PopulateListView(listView1);
            }
        }
        private void lookupByNumericIDToolStripMenuItem_Click(object sender, EventArgs e)
        {
            frmEvalID evalform = new frmEvalID(languages);
            evalform.ShowDialog();
        }

        private void preferencesToolStripMenuItem_Click(object sender, EventArgs e)
        {
            List<string> languages = new List<string>();
            foreach (string lang in cmbLanguage.Items)
                languages.Add(lang);

            frmPrefs prefs = new frmPrefs(preferences, languages);
            prefs.ShowDialog();
        }

        private void txtFilter_TextChanged(object sender, EventArgs e)
        {
            filter_changed = true;
            button2.Enabled = filter_changed;
        }
        private void chkIgnore_CheckedChanged(object sender, EventArgs e)
        {
            if (txtFilter.Text.Trim().Length > 0)
                filter_changed = true;
        }

        private void button2_Click(object sender, EventArgs e)
        {
            PopulateListView(listView1);
            filter_changed = false;
            ((Button)sender).Enabled = filter_changed;
        }

        private void listView1_ColumnClick(object sender, System.Windows.Forms.ColumnClickEventArgs e)
        {
            ListView ctl = (ListView)sender;
            if (e.Column != sorted)
            {
                sorted = e.Column;
                ctl.Sorting = SortOrder.Ascending;
            }
            else
            {
                ctl.Sorting = (ctl.Sorting == SortOrder.Ascending) ? SortOrder.Descending : SortOrder.Ascending;
            }

            if (e.Column != 0)
            {
                ctl.ListViewItemSorter = new ListViewItemComparer(e.Column, ctl.Sorting, typeof(string));
            }
            else
            {
                ctl.ListViewItemSorter = new ListViewItemComparer(e.Column, ctl.Sorting, typeof(int));
            }
            ctl.Sort();
        }

        #endregion

    }
}

