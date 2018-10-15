killall domino

#echo sleeping for ten seconds
#sleep 10

cd /home/pi/Domino
./dist/Release/GNU-Linux/domino -h  2>>./error_dominoHybrid.txt

read -n 1 -s