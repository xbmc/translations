kodi-txupdate
=============

This utility is suitable for keeping KODI upstream language files at github repositories and the fresh translated language files hosted on transifex.com in sync.

What steps it does:
* Checks if all needed configfiles are in place and if Kodi's translations github repo is cloned at the right place. If not, it creates the files and makes the github clone at the right place.
* Let you choose the project you want to sync. There are 3 projects existing. kodi-main, kodi-skins, kodi-addons
* Downloads the defined language database file, with the allowed languages and language codes in it. The location for this file has to be defined in the config file of the project (already there)
* Downloads a fresh set of language files from Transifex with the latest translations. It only downloads the changed files. The rest is used from cache.
* Downloads a fresh set of language files from upstream github repos by cloning them if needed, git pulls them, cleans them and copy the new language files from them to the cache and memory.
* Merge the files coming from these two locations, by going through the entries found in the upstream en_GB file. Create a merged fresh set of language files.
* Upload to Transifex any new upstream change in the en_GB file or any new tranlations appear upstream.
* Write out the merged fresh files to the local clones of the upstream github repos. Also copy the files to the Kodi translations repo for keeping record.
* Make a list on the existing local github repo clones if they have changed and if last push of these repos have passed the update interval given in the config file.
* Here you can check diff files and list of changed files for these repos before making any actual push.
* If everything is fine, you can choose to make the actual git push, or just make a "dry run" push to check if everything is ok.
* After push, the utility cleans up the unused cache files and unused git clones, making it self-maintaining.
* As a last step it you can git push the Kodi translations repo, or check the diff files before doing it.

Important to note, that in case we both have a translation at the upstream repository and we have a translation at Transifex for the same English string, the utility prefers the one at Transifex. This means that new translations modifications can only be pulled from ustream into the merged files in case they are for completely newly introduced English strings which do not have translation existing at Transifex yet.

## Install
Requirements:
* OS: Linux
* Packages: curl, libcurl, libjsoncpp, git, vim

Ubuntu prerequisites installation:
```
sudo apt-get install build-essential curl libcurl4-gnutls-dev libjsoncpp0 libjsoncpp-dev git vim
```
After git cloning the utility, simply run make. The bin file called "kodi-txupdate" will be created.
make install copies this file to ~/bin/.

## Usage


`kodi-txpudate (-c)`


  * **-c** This optional parameter tells the utility to use cache files instead of fresh files. This is useful for debugging some situations.

## Setting files
These files get created by the utility, if you run it the first time.


`~/.config/kodi-txupdate/passwords.xml`

This file stores http password to access protected upstream Github or Transifex language files. For Transifex you must specify your user credentials here.
This file gets auto generated in case it is not there. You just have to type in the passwords for Transifex and Github.


`~/.config/kodi-txupdate/default-dir.conf`

   At this file you can store the desired location of the Kodi translation github repo clone. Default is /home/translator/transifex/

`KODITRANSLATIONS/PROJECTNAME/kodi-txupdate.conf`

   This file resides at the Kodi translations github repo. We have one conf file for each project. This file tells all the info to the utility what resources have to be synced and how, where the files are stored in what format, etc.
   For detailed explanation, please check [README-conf.md](README-conf.md) file.

`vim related configfiles`
  These files get copied at the right locations if they are not there. These help to show the kodi-txupdate.conf files syntax highlight the internal variables and main commands.
  Also for diff view there is a color scheme installed. These config files are:
  * ~/.vim/colors/desert256.vim
  * ~/.vim/syntax/ktx.vim
  * ~/.vim/ftdetect/ktx.vim
  * ~/.vimrc

## Tips and tricks
* Cache files for http download and upload actions are stored in the .httpcache directory in each project folder. If the format of a downloaded file is corrupt upstream, you can find the file here and correct it to continue. If you want to be sure to use the latest upstream files, you can simply delete this directory.

For any questions, please write to: alanwww1@kodi.tv

2016 Attila Jakosa(alanwww1), TeamKODI
