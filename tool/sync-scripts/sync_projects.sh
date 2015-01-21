DIRGITHUB=/home/translator/transifex/xbmc-translations
WORKINGDIR=$(pwd)
MAINPROJECT=xbmc-main
SKINSPROJECT=xbmc-skins
ADDONSPROJECT=xbmc-addons

cd $DIRGITHUB

echo -e "\nPlease choose which project to sync:"
select yn in "main" "skins" "addons"; do
    case $yn in
        main )   PROJECT=$MAINPROJECT;    break;;
        skins )  PROJECT=$SKINSPROJECT;   break;;
        addons ) PROJECT=$ADDONSPROJECT;  break;;
    esac
done

echo -e "\nStarting phase I."
echo -e "Now we download the language files from upstream loacations and Transifex."
echo -e "We merge them and check if there is something we should upload to Transifex later.\n"
sleep 3

xbmc-txupdate translations/$PROJECT/

if (($? != 0)); then
    echo "An error occured in xbmc-txupdate process. Please check the output, or the logfile!"
    exit
fi

while true; do
    read -p "Please in another terminal check if all went fine to start upload to TX! Can we continue? (y/n)" yn
    case $yn in
        [Yy]* ) break;;
        [Nn]* ) exit;;
        * ) echo "Please answer yes or no.";;
    esac
done

echo -e "\nStarting phase II."
echo -e "Now we upload the new strings to transifex which are coming from upstream locations.\n"
sleep 3

xbmc-txupdate translations/$PROJECT/ -u

if (($? != 0)); then
    echo "An error occured in xbmc-txupdate process. Please check the output, or the logfile!"
    exit
fi

echo -e "\nStarting phase III."
echo -e "Now we commit changes to the Kodi-Translations github project and push them."
sleep 3

echo -e "git add -A translations/$PROJECT\n"
git add -A translations/$PROJECT
echo -e "git commit -m [$PROJECT] sync\n"
git commit -m "[$PROJECT] sync"

echo -e "\n\nBefore push, we check if download and git commits resulted the expected changes.\n"
echo -e "\nPlease choose between some debug options or continue to push changes."
select yn in "git-push" "changed-files" "changed-in-merged" "changed-in-temp" ; do
    case $yn in
        git-push )        break;;
        changed-files )       echo -e "git diff --name-status HEAD^\n" ;  git diff --name-status HEAD^ ;;
        changed-in-merged )   echo -e "git diff --name-status HEAD^ translations/$PROJECT/merged-langfiles\n" ; git diff --name-status HEAD^ translations/$PROJECT/merged-langfiles ;;
        changed-in-temp )     echo -e "git diff --name-status HEAD^ translations/$PROJECT/tempfiles_txupdate\n" ; git diff --name-status HEAD^ translations/$PROJECT/tempfiles_txupdate ;;
    esac
done

echo -e "\ngit push \"origin master\"\n"
git push origin master

echo -e "\nFinished succesfully!\n"

cd $WORKINGDIR

