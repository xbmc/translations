xbmc-txupdate
=============

This utility is suitable for keeping XBMC upstream language files and the language files hosted on transifex.com in sync.

What it does:
* Downloads the fresh files from upstream http URLs specified in the xbmc-txupdate.xml file and also downloads the fresh translations from transifex.net and makes a merge of the files. 
* With this merge process it creates fresh files containing all changes upstream and on transifex. 
* These updated files than can be commited to upstream repositories for end user usage.
* During the merge process it also creates update files which only contain the new upstream version if the English language files and also contain the new English strings translations introduced for different languagees on the upstream repository. These update PO files than can automatically uploaded to transifex with this utility.

Important to note that in case we both have a translation at the upstream repository and we have a translation at transifex.net for the same English string, the utility prefers the one at transifex.net. This means that new translations modifications can only be pulled from ustream into the merged files in case they are for completely newly introduced English strings which do not have translation existing at transifex yet.

## Install
Requirements:
* OS: Linux
* Packages: curl, libcurl, libjsoncpp

Ubuntu prerequisites installation:
```
sudo apt-get install build-essential curl libcurl4-gnutls-dev libjsoncpp0 libjsoncpp-dev git
```
After git cloning the utility, simply run make. The bin file called "xbmc-txupdate" will be created.

## Usage


  **xbmc-txpudate PROJECTDIR [working mode]**


  * **PROJECTDIR:** the directory which contains the xbmc-txupdate.xml settings file and the .passwords file. This will be the directory where your merged and transifex update files get generated.
  * **Working modes:**
    * **-d**    Only download to local cache, without performing a merge.
    * **-dm**    Download and create merged and tx update files, but no upload performed.
    * **-dmu**    Download, merge and upload the new files to transifex.
    * **-u**    Only upload the previously prepared files. Note that this needs downlad and merge ran before.
    * No working mode arguments used, performs as -dm

Please note that the utility needs at least a projet setup xml file in the PROJECTDIR to work. Additionally it needs to have a passwords xml file which holds credentials for Transifex.com and for the http places you are using as upstream repositories.

## Setting files
In you PROJECTDIR folder you need to have the following files:

**I. xbmc-txupdate.xml**

This file stores basic data needed for the translation project.
The format of the file looks like this:

```xml
<?xml version="1.0" ?>
<resources projectname="">
      <resource name="">
        <TXname></TXname>
        <upstreamURL></upstreamURL>
        <upstreamLangs></upstreamLangs>
        <resourceType></resourceType>
        <resourceSubdir></resourceSubdir>
    </resource>
</resources>
```
Where:
  * projectname: The Transifex projectname. Use the exact same string as on Transifex.
    Optional attributes:
      * http_cache_expire - (default: 360) expirity time for cached files in minutes. If cache file is younger than the given time, no actual http download will happen. In that case the cached file gets used.
      * min_completion - (default: 10%) a limit for the translated percentage to actually download a translation file.
      * merged_langfiledir - (default: merged-langfiles) the directory under PROJECTDIR, where the fresh merged, cleaned translations will be locally created.
      * temptxupdate_langfiledir - (default: tempfiles_txupdate) the directory under PROJECTDIR, where the language files to update Transifex will be locally created.
      * forcePOComm - (default: false) Force program to write comments into the non-English PO files, not only into the English ones.
      * support_email - (default: anonymus) Specify the support email address written in PO file headers, which users can use in case of PO file format problems.
  * name: The name of the resource(eg. addon). This is the name which will be used as a directory name where the language files will be created. So the best is to use the same directory name here which is used at the upstream repo. This field must be filled.
  * TXname: The Transifex name you want to have for the resource(addon). No special characters (or dots) are alowed. This field must be filled.
  * upsreamURL: The http URL where the upstream files are maintained. For a plugin, use the URL for the directory where the addon.xml file exists.
    Optional attributes:
      * filetype - (default: use PO files) adding attribute "xml" here will make the utility use the old xml file format for the upstream file read.
      * URLsuffix - (default: no suffix needed) some websites need a suffix text after filename in the URL (eg. gitweb needs to specify the branch here)
  * upstreamLangs: Specify what languages exist on the upstream repository to pull. Leaving this empty will mean English only. The best is if you have the upstream files at a github repo, because using the API, the util can fetch a directory listing to determine the possible languages.
    Special values:
      * github_all: If your repo is stored at github, you can fetch the available languages automatically.
      * tx_all: Fetch files from upstream for lanuguages that exist on Transifex.
  * resourceType: The type of the resource ehich can be the following:
      * addon: A regular addon with an addon.xml file AND language files
      * addon_nostrings: Special addon with an addon.xml file, but NO language files
      * skin: A skin addon with an addon.xml file AND language files
      * xbmc-core: Language files for xbmc-core
  * resourceSubdir: The subdirectory to put the language files of the resource in. (optional)
    Optional attributes:
      * writeXML: (default: false) write merged string files in the old XML file format.
      * writePO: (default: true) write merged string files in the new PO file format.

**II. .passwords.xml**

This file stores http password to access protected upstream or Transifex language files. For Transifex you must specify your user credentials here.
The format of the file looks like this:

```xml
<?xml version="1.0" ?>
<websites>
    <website prefix="">
        <login></login>
        <password></password>
    </website>
</websites>
```

Where:
   * Attribute prefix: The URL prefix of the website you want to use a password with. For example for Transifex, you have to use : "https://www.transifex.com/api/2/project/"

## Tips and tricks
* Cache files for http download and upload actions are stored in the .httpcache directory. If the format of a downloaded file is corrupt upstream, you can find the file here and correct it to continue. If you want to be sure to use the latest upstream files, you can simply delete this directory.
* Don't run in upload mode, before doing a succesful download and merge. The status is stored in a file called ".dload_merge_status" in the .httpcache directory. If there is an OK in it, download and merge has run successfully before.
* The xbmc-txupdate.xml file is monitored if it has been changed before upload. The old state of the file is stored in the ".last_xbmc-txupdate.xml" file in .httpcache. If you want to force-run upload mode, you can copy the changed xml file as this name to cheat the utility. Only do this, if you know what you are doing!

For any questions, please write to: alanwww1@xbmc.org

2012 Attila Jakosa, Team XBMC
