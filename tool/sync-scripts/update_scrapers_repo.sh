DIRGITREPO=/home/translator/transifex/upstream/repo-scrapers
LANGDLOADXMLDIR=/home/translator/transifex/xbmc-translations/tool/langdload-projectfiles
WORKINGDIR=$(pwd)
SCRAPERLIST1=xbmc-scrapers_part1.xml
SCRAPERLIST2=xbmc-scrapers_part2.xml
SCRAPERLIST3=xbmc-scrapers_all.xml
WORKBRANCH=frodo

echo -e "\nPlease choose which scrapers would you like to update.\nNote that the kodi addons servers cannot take all of the addons at once."
select yn in "$SCRAPERLIST1" "$SCRAPERLIST2" "$SCRAPERLIST3" ; do
    case $yn in
        $SCRAPERLIST1 )       XMLFILENAME=$SCRAPERLIST1; COMMITS=4 ; COMMITSPL=5 ; break;;
        $SCRAPERLIST2 )       XMLFILENAME=$SCRAPERLIST2; COMMITS=5 ; COMMITSPL=6 ;   break;;
        $SCRAPERLIST3 )       XMLFILENAME=$SCRAPERLIST3; COMMITS=9 ; COMMITSPL=10;    break;;
    esac
done

echo -e "\nStarting phase I."
echo -e "Now we cd into the local scrapers git repo, and make a git pull.\n"
sleep 3

cd $DIRGITREPO
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
echo -e "Now we download the language files from Kodi Translations Github repo to the local scrapers repo clone.\n"
sleep 3

cd $LANGDLOADXMLDIR
xbmc-langdload $XMLFILENAME

if (($? != 0)); then
    echo "An error occured in xbmc-landload process. Please check the output!"
    exit
fi

echo -e "\nStarting phase III."
echo -e "First we check if download and git commits resulted the expected changes.\n"

cd $DIRGITREPO 
echo -e "\nPlease choose between some debug options or continue to push changes."
select yn in "git-push" "changed-files" "git-log" ; do
    case $yn in
        git-push )        break;;
        changed-files )   echo -e "git diff --name-status HEAD~$COMMITS\n" ;  git diff --name-status HEAD~$COMMITS ;;
        git-log ) echo -e "git log -n $COMMITSPL" ; git log -n $COMMITSPL ;;
    esac
done

echo -e "\ngit push origin $WORKBRANCH\n"
git push origin $WORKBRANCH

echo -e "\nFinished succesfully!\n"

cd $WORKINGDIR

