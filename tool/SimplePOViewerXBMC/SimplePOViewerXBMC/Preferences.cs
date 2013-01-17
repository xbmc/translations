using System;
using System.Collections.Generic;
using System.Linq;
using System.Xml.Linq;
using System.IO;

namespace SimplePOViewerXBMC
{
    public class Preferences
    {
        public List<string> LoadOnStartup = new List<string>();
        public string RootFolder = string.Empty;

        private string profile = Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.ApplicationData),
                                 ".xbmc.translations.SimplePOViewer.profile");


        public void Restore()
        {
            try
            {
				Console.WriteLine ("Reading from profile '" + profile +"'");
                //if (System.IO.File.Exists(profile) == true )
                //{
                    XDocument cfg = XDocument.Load(profile, LoadOptions.None); // for good measure? http://stackoverflow.com/questions/9135604/xdocument-saving-error-in-mono-this-xmlwriter-does-not-accept-text-at-this-stat
                    RootFolder = cfg.Element("profile").Element("xbmcroot").Value.Trim();
					
					if (RootFolder.Length>0 && RootFolder.Substring(RootFolder.Length-1,1) != Path.DirectorySeparatorChar.ToString())
						RootFolder += Path.DirectorySeparatorChar;
                    
					LoadOnStartup = GetLanguages(cfg.Element("profile").Element("languages").Value);
                //}
            }
            catch (Exception ex)
            {
                Console.WriteLine("Error accessing profile: " + ex.Message);
            }
        }

        public void Save()
        {
            try
            {  
				Console.WriteLine ("Saving to profile '" + profile +"'");
                XDocument cfg = new XDocument(
                                               new XElement("profile",
                                               new XElement("xbmcroot"), new XElement("languages")));

                cfg.Element("profile").Element("xbmcroot").Value = RootFolder;
                cfg.Element("profile").Element("languages").Value = SetLanguages(LoadOnStartup);
                cfg.Save(profile);
            }
            catch (Exception ex)
            {
                Console.WriteLine("Error accessing profile: " + ex.Message);
                throw ex;
            }
        }

        private List<string> GetLanguages(string text)
        {
			List<string> list = new List<string>();
			
			if (text.Trim ().Length >0)
			{
				list = text.Split(';').ToList();
			}
            return list; 
        }

        private string SetLanguages(List<string> list)
        {
            string result = string.Empty;

            if (list.Count > 0)
            {
                foreach (string s in list)
                {
                    result += s + ";";
                }
                result = result.Substring(0, result.Length - 1);
            }

            return result;
        }
    }
}
