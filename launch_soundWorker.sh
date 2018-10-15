#killall sclang
#killall scsynth

#echo sleeping for five seconds...
#sleep 5

cd /home/pi/Domino
sclang ./SuperCollider/demo_181012-worker.scd  2>>error_soundWorker.txt

read -n 1 -s