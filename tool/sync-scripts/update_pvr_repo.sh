DIRPVRGITREPO=/home/translator/transifex/upstream/xbmc-pvr-addons
LANGDLOADXMLDIR=/home/translator/transifex/xbmc-translations/tool/langdload-projectfiles
XMLFILENAME=xbmc-pvr.xml
WORKINGDIR=$(pwd)

echo -e "\nStarting phase I."
echo -e "Now we cd into the local pvr repo and make a git pull.\n"
sleep 3

cd $DIRPVRGITREPO

git pull

if (($? != 0)); then
    echo "An error occured in a git process. Please check the output!"
    exit
fi

echo -e "\nStarting phase II."
echo -e "Now we download the language files from Kodi Translations Github repo to the local pvr repo clone.\n"
sleep 3

cd $LANGDLOADXMLDIR
xbmc-langdload $XMLFILENAME

if (($? != 0)); then
    echo "An error occured in xbmc-landload process. Please check the output!"
    exit
fi

echo -e "\nStarting phase III."
echo -e "First we check if download and git commits resulted the expected changes.\n"

cd $DIRPVRGITREPO 
echo -e "\nPlease choose between some debug options or continue to push changes."
select yn in "git-push" "changed-files" "git-log" ; do
    case $yn in
        git-push )        break;;
        changed-files )       echo -e "git diff --name-status HEAD^^^\n" ;  git diff --name-status HEAD^^^ ;;
        git-log ) echo -e "git log -n 4" ; git log -n 4 ;;
    esac
done

echo -e "\ngit push origin master\n"
git push origin master

echo -e "\nFinished succesfully!\n"

cd $WORKINGDIR

