xbmc-langdload
==============

Downloader utility to help addon developers, pulling XBMC translations from XBMC translations github repo, to local direectories.

## Usage

**1.Simple mode**

  **xbmc-langdload PROJECTID/ADDONID LOCALDIR [options]**

  * **PROJECTID:** The id of the project defined on the xbmc repo. eg. xbmc-main-frodo
  * **ADDONID:** The id of the addon which is defined in the "id" tag in the addon.xml file
  * **LOCALDIR:** The local directory to copy the files to. This is where the addon.xml file gets.
  * options:
    * -c:  skip download of changelog.txt (not recommended)
    * -e:  skip download of cleaned English language file (not recommended)
    * -ce: combination of the two above

  * Example: xbmc-langdload xbmc-addons/plugin.video.coolplugin /home/myname/somedir/

**2.Batch mode with xml file usage**

  In batch mode you can download the language files for multiple addons.
  You can still use the advanced options, like skip the download of the English file, or skip download of changelog.txt.
  Skipping download of any files is NOT recommended by the way.
  In this mode you can also automatically git commit your changes. Of course for that, you need git installed.
  Only use this feature, if you know what you are doing.

  **xbmc-langdload XMLFILE**

  * **XMLFILE:** The path and filename of the input XML file which holds the download data

  * Example: xbmc-langdload xbmc-langdload.xml

xbmc-langdload.xml example (linux):
```xml
<?xml version="1.0" ?>
<addonlist>
    <addon name="xbmc-addons/plugin.video.coolvideoplugin">
        <localdir>/home/user/dir/coolvideplugin</localdir>
    </addon>
    <addon name="xbmc-skins/skin.coolskin">
        <localdir>skin-coolskin</localdir>
    </addon>
    <addon name="xbmc-skins/skin.coolerskin">
        <localdir>skin-coolerskin</localdir>
        <skipchangelog>true</skipchangelog>
        <skipenglish>true</skipenglish>
    </addon>
    <addon name="xbmc-skins/skin.coolestskin">
        <localdir>skin-coolestskin</localdir>
        <gittemplate>Updated addon %n to version %v</gittemplate>
    </addon>
</addonlist>
```
For windows of course you have to use backslash in the directory name.
The default search path for git-bash executables is C:\Program Files (x86)\Git\bin\
to change that, just use attribute "gitexecpath" like this:
```xml
<gittemplate gitexecpath="C:\Program Files\NewGitDir\bin\">Updated addon %n to version %v</gittemplate>
```
Currently only the official git client is supported. Download it from here: http://git-scm.com/downloads

**3.List addons mode**

  In this mode you can fetch a current list of the available hosted addons on xbmc translations github repo.
  This list also shows what language fileformat is used (XML or PO) and if the addon has a changelog.txt hosted.

  **xbmc-langdload list addons**

## Install

Requirements:
* OS: Linux, Windows

**Linux**
Needed packages: curl, libcurl, libjsoncpp and the developer packages

Ubuntu prerequisites installation:
```
sudo apt-get install build-essential curl libcurl4-gnutls-dev libjsoncpp0 libjsoncpp-dev git
```

After git cloning the utility, simply run:
```
make
sudo make install
```

**Windows**
Just download the precompiled exe file and run it in a command prompt.

## Support

Note for Windows users: In case you have whitespace or any special character
in the directory/file argument, please use apostrophe around them. For example:
```
xbmc-langdload.exe xbmc-skins/skin.essence "C:\some dir\"
```
Also make sure you have write access to the local directory!
Please run the command prompt in admin mode!


For any questions, please write to: alanwww1@xbmc.org

2012 Attila Jakosa, Team XBMC
