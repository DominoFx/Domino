#killall domino

#echo sleeping for ten seconds
#sleep 10

cd /home/pi/Domino
./dist/Release/GNU-Linux/domino -m  2>>./error_dominoMaster.txt

read -n 1 -s