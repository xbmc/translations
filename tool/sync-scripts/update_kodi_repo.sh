DIRKODIGITREPO=/home/translator/transifex/upstream/xbmc-master
LANGDLOADXMLDIR=/home/translator/transifex/xbmc-translations/tool/langdload-projectfiles
XMLFILENAME=xbmc-master.xml
WORKINGDIR=$(pwd)
BRANCHNAME=Helix

echo -e "\nPlease choose if you want to update branch master or branch $BRANCHNAME:"
select yn in "master" "$BRANCHNAME" ; do
    case $yn in
        master )       WORKBRANCH=master;    break;;
        $BRANCHNAME )  WORKBRANCH=$BRANCHNAME;   break;;
    esac
done

echo -e "\nStarting phase I."
echo -e "Now we cd into the local Kodi repo, set branch and make a git pull.\n"
sleep 3

cd $DIRKODIGITREPO
git checkout $WORKBRANCH

if (($? != 0)); then
    echo "An error occured in a git process. Please check the output!"
    exit
fi

git pull

if (($? != 0)); then
    echo "An error occured in a git process. Please check the output!"
    exit
fi

echo -e "\nStarting phase II."
echo -e "Now we download the language files from Kodi Translations Github repo to the local Kodi repo clone.\n"
sleep 3

cd $LANGDLOADXMLDIR
xbmc-langdload $XMLFILENAME

if (($? != 0)); then
    echo "An error occured in xbmc-landload process. Please check the output!"
    exit
fi

echo -e "\nStarting phase III."
echo -e "First we check if download and git commits resulted the expected changes.\n"

cd $DIRKODIGITREPO 
echo -e "\nPlease choose between some debug options or continue to push changes."
select yn in "changed-files" "changed-in-english" "changed-in-addons" "changed-in-confl-english" "git-log" "git-push"; do
    case $yn in
        changed-files )       echo -e "git diff --name-status HEAD^^^\n" ;  git diff --name-status HEAD^^^ ;;
        changed-in-english )  echo -e "git diff HEAD^^^ language/English/\n" ; git diff HEAD^^^ language/English/ ;;
        changed-in-addons )   echo -e "git diff HEAD^^^ addons/\n" ; git diff HEAD^^^ addons/ ;;
        changed-in-confl-english ) echo -e "git diff HEAD^^^ addons/skin.confluence/language/English/\n" ; git diff HEAD^^^ addons/skin.confluence/language/English/ ;;
        git-log ) echo -e "git log -n 4" ; git log -n 4 ;;
        git-push )        break;;
    esac
done

echo -e "\ngit push origin $WORKBRANCH\n"
git push origin $WORKBRANCH

echo -e "\nFinished succesfully!\n"

cd $WORKINGDIR

