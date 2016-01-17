kodi-txupdate.conf files help
=============================

These configuarzion files are telling the kodi-txupdate utility all the important information about what resources have to be synced, where the upstream files can be found, what Transifex projects are related to them, what lanaguage codes to use, how to git commit the changes, what commit texts to use etc.

These files are located at the kodi translations git repo, under kodi-translations folder for 3 projects: kodi-main, kodi-skins, kodi-addons.
You can always check how a sample fo the file look, [here](https://github.com/xbmc/translations/blob/master/kodi-translations/kodi-skins/kodi-txupdate.conf)

It is always the best to view and edit these config files with vim, using the bundled and auto copied syntax highlighting vim config files.

## Main commands in the config files
* **`setvar`** creates a new "external" variable. the name is up for you to choose. Best use for this is to pre-define variables at the beggining of the config files and later re-use them at several places.
* **`set`** sets an "internal" variable. The internal variables control the working of the utility. Check the list below, which one controls what.
* **`pset`** permanently sets an internal variable re-doing the assignment at each create of new resource.
* **`tset`** temporary sets an internal variable only until a new resource is created. After the creation, it is not valid anymore for the next resource.
* **`clear`** clears an internal variable. Sets it to an empty state.
* **`create resource`** creates the resource with the previously given internal variables and the arguments given after this command (see later)

To use a variable called "VAR" just refer to it as $VAR in the assignment.

## Internal variables

First of all, the name of these variables are usually conatain the location of the file or resource it refers to.
These locations are defined as the following:
* **`UPS`** - upstream git location
* **`LOC`** - local clone of the upstream git repositories
* **`TRX`** - Transifex source location
* **`UPD`** - Transifex target location
* **`MRG`** - merged language files at the local kodi translations repo clone
* **`UPSSRC`** - like UPS, just in the special case of the en_GB language addon which is at a different place than the rest of the language files
* **`LOCSRC`** - like LOC, just in the special case of the en_GB language addon which is at a different place than the rest of the language files

**General internal variables**

* **`ChgLogFormat`** - Contains the text which goes into the changelog.txt file for addons (if requested). Can contain parameters like %i for the version of the addon, %Y current year, %m current month, %d current day
* **`GitCommitText`** - Contains the text which will be used at as git commit text. Variable $(RESNAME) can be used to include the resource name in it.
* **`GitCommitTextSRC`** - Same as the previous, just for the case when the en_GB file gets changed (due to a syntax mistake) it will be git commited separately and this variable holds the message for that kind of change.
* **`MRGLFilesDir`** - Path to the merged files store in the kodi translations git repo. It is currently "merged-langflies" in the repo.
* **`UPSLocalPath`** - This is a path where all the git clones of the upstream git repos will be cloned and stored. Currently used path: /home/translator/transifex/upstream-kodi-txupdate/PROJECTNAME/
* **`UPDLFilesDir`** - Path to the temporary update files created in the kodi translations git repo. These files get used for updating Transifex with new en_GB files or new translations coming from upstrem. Currently used foldername: "tempfiles_txupdate"
* **`SupportEmailAddr`** - This email address gets written in all string.po file as a reference if someone discovers any problems with the files. Currently used address: http://trac.kodi.tv/
* **`SRCLCode`** - Defines the language code for the source language. For Kodi we use "en_GB".
* **`BaseLForm`** - Defines the default language code system to use. For Kodi we use "$(LCODE)". This is also used to tell the path naming of the update language files creation.
* **`LTeamLFormat`** - Defines the lanaguage code system we use in the strings.po file at line language team. At Kodi we use $(TXLNAME). It is, because this language format gets appeared also in Transifex when translators navigate there.
* **`LDatabaseURL`** - Tells where to get the language database file from. We have one database created [here](https://raw.github.com/xbmc/translations/master/tool/lang-database/kodi-languages.json):
* **`MinComplPercent`** - Defines a minimum completion percentage for languages to be considered and downloaded. We use 1% at the moment.
* **`CacheExpire`** - Tells the utility an expire time (in minutes) from when the cached files gets checked, if there is a newer version upstream or at Transifex. If we are within this time, NO new files get downloaded, only the previously cached files get used. This is usefull for debugging reasons, and in case of an error we immediately re-run the utility. We use value "10" here.
* **`GitPushInterval`** - An inerval (in days) to update and push the new files to the original upstream github repositories. We use 5 days by default.
* **`ForceComm`** - Fore comments to appear in non source language files. Default is false.
* **`ForceGitDloadToCache`** - Force cache to be considered expired, so always get a fresh file from git.
* **`SkipGitReset`** - Git reset could be time consuming. With this option if we know what we do, we can turn it off for upstream locations. Default is false.
* **`SkipGitPush`** - For each resource we can force not to git push the git repo, even if it passed the interval. This state can be manually overwritten at push time, where there is a menu to choose.
* **`ForceGitPush`** - The opposite of the previous option.
* **`Rebrand`** - Change all xbmc strings to Kodi. Default is false.
* **`ForceTXUpd`** - Force upload of all language files to Transifex.
* **`IsLangAddon`** - Tell the utility that the resource is a language-addon. It is only used for the main language file.
* **`HasOnlyAddonXML`** - This tells the utility that the resource (addon) has no language file, just the addon.xml file.

**Location dependent internal variables**

* For locations UPS, UPS, MRG, UPSSRC, LOC, LOCSRC. The following variables are defined as a combination of keywords (eg. UPSOwner) (not all combinations exist. See table below)
  * **`Owner, Repo, Branch`** - Basic git data for identifying the github repo.
  * **`LPath`** - Path where the strings.po files are found in the git repo. This is language dependent so it has to be parametric.
  * **`AXMLPath`** - Path where the addon.xml file is found in the git repo. This is only language dependent for the main language addons.
  * **`LFormInAXML`** - Tells what language code format we use in the addon.xml file for description, summary and disclaimer.
  * **`ChLogPath`** - Path where the changelog.txt file is found in the git repo.

* For locations TRX, UPD. The following variables are defined as a combination of keywords (eg. TRXLForm) (not all combinations exist. See table below)
  * **`ProjectName`** - Defines the projectname used on Transifex (eg. kodi-main, kodi-skins, kodi-addons).
  * **`LongProjectName`** - Defines the LONG projectname used on Transifex.
  * **`LForm`** - Tells what language code forma we use at Transifex.
  * **`ResName`** - Defines the resource name used at Transifex. Please note that TRXResName is given in the line of creating the resource, UPDResName can be specified separately.

##Resource creation specific variables

* **`ResName`** - Defines the name of the resource.We use the addon id here.
* **`TRXResName`** - Defines the resource name used on Transifex. This is usually different, because for example dots are not allowed at Transifex in the resource name.
* **`GITCommit`** -  We tell the utility that we want to create a git commit after updating this resource.
* **`SkipVersionBump`** - We set this true if we want to leave the addons's version number and changelog.txt unchanged. Default is false.
* **`MajorVersionBump`** - We force the version number to get a major bump  for the addon version: eg. x.y.z changes to x+1.0.0

##Summary tables of location dependent variables

Table 1: Possible location dependent variables for UPS, UPSSRC, LOC, LOCSRC, MRG

| UPS           | UPSSRC         | LOC           | LOCSRC         | MRG          |
| ------------- | -------------- | ------------- | -------------- | ------------ |
| UPSOwner      | UPSSRCOwner    | LOCOwner      | LOCSRCOwner    | -            |
| UPSRepo       | UPSSRCRepo     | LOCRepo       | LOCSRCRepo     | -            |
| UPSBranch     | UPSSRCBranch   | LOCBranch     | LOCSRCBranch   | -            |
| UPSLPath      | UPSSRCLPath    | LOCLPath      | LOCSRCLPath    | MRGLPath     |
| UPSAXMLPath   | UPSSRCAXMLPath | LOCAXMLPath   | LOCSRCAXMLPath | MRGAXMLPath  |
| UPSLFormInAXM | -              | LOCLFormInAXM | -              | -            |
| UPSChLogPath  | -              | LOCChLogPath  | -              | MRGChLogPath |

Table 2: Possible location dependent variables for TRX, UPD

| TRX                | UPD                |
| -----------------  | ------------------ |
| TRXProjectName     | UPSProjectName     |
| TRXLongProjectName | UPDLongProjectName |
| TRXLForm           | UPDLForm           |
| TRXResName*        | UPDResName         |

  Note: TRXResName is only specified in the create resource line.