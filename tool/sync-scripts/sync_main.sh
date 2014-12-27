cd ../..
echo "Starting download phase"
xbmc-txupdate translations/xbmc-main/
while true; do
    read -p "Please in another terminal check if all went fine to start upload to TX! Can we continue? (y/n)" yn
    case $yn in
        [Yy]* ) break;;
        [Nn]* ) exit;;
        * ) echo "Please answer yes or no.";;
    esac
done

echo "Starting upload phase"
xbmc-txupdate translations/xbmc-main/ -u

while true; do
    read -p "Please check again if all went fine and we can commit changes! Can we continue? (y/n)" yn
    case $yn in
        [Yy]* ) break;;
        [Nn]* ) exit;;
        * ) echo "Please answer yes or no.";;
    esac
done

echo -e "git add -A\n"
git add -A
echo -e "git commit -am [xbmc-main] sync\n"
git commit -am "[xbmc-main] sync"
echo -e "git push \"origin master\"\n"
git push origin master

cd tool/sync-scripts

