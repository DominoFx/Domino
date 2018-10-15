#killall domino

#echo sleeping for ten seconds
#sleep 10

cd /home/pi/Domino
./dist/Release/GNU-Linux/domino -w  2>>./error_dominoWorker.txt

read -n 1 -s
