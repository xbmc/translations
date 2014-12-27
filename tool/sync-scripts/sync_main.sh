cd ../..
xbmc-txupdate translations/xbmc-main/
while true; do
    read -p "Please in another terminal check if all went fine! Can we continue? " yn
    case $yn in
        [Yy]* ) break;;
        [Nn]* ) exit;;
        * ) echo "Please answer yes or no.";;
    esac
done

xbmc-txupdate translations/xbmc-main/ -u

while true; do
    read -p "Please check again if all went fine and we can commit changes! Can we continue? " yn
    case $yn in
        [Yy]* ) break;;
        [Nn]* ) exit;;
        * ) echo "Please answer yes or no.";;
    esac
done

git add -A
git commit -am "[xbmc-main] sync"
git push origin master

cd tool/sync-scripts

