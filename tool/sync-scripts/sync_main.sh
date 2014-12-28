cd ../..

echo -e "\nPlease choose which project to sync:"
select yn in "xbmc-main" "xbmc-skins" "xbmc-addons"; do
    case $yn in
        xbmc-main )   PROJECT=xbmc-main;    break;;
        xbmc-skins )  PROJECT=xbmc-skins;   break;;
        xbmc-addons ) PROJECT=xbmc-addons;  break;;
    esac
done

echo -e "\nStarting phase I."
echo -e "Now we download the language files from upstream loacations and Transifex."
echo -e "We merge them and check if there is something we should upload to Transifex later.\n"
sleep 3

xbmc-txupdate translations/$PROJECT/
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

echo -e "git add -A\n"
git add -A
echo -e "git commit -am [$PROJECT] sync\n"
git commit -am "[$PROJECT] sync"
echo -e "\ngit push \"origin master\"\n"
git push origin master

echo -e "\nFinished succesfully!\n"

cd tool/sync-scripts

