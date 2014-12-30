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

while true; do
    read -p "Please check again if all went fine and we can commit changes! Can we continue? (y/n)" yn
    case $yn in
        [Yy]* ) break;;
        [Nn]* ) exit;;
        * ) echo "Please answer yes or no.";;
    esac
done

echo -e "\nStarting phase III."
echo -e "Now we commit changes to the Kodi-Translations github project and push them."
sleep 3

echo -e "git add -A translations/$PROJECT\n"
git add -A translations/$PROJECT
echo -e "git commit -m [$PROJECT] sync\n"
git commit -m "[$PROJECT] sync"
echo -e "\ngit push \"origin master\"\n"
git push origin master

echo -e "\nFinished succesfully!\n"

cd $WORKINGDIR

