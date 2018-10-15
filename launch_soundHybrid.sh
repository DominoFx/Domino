killall sclang
killall scsynth

#echo sleeping for five seconds...
#sleep 5

cd /home/pi/Domino
sclang ./SuperCollider/demo_181012-hybrid.scd  2>>error_soundHybrid.txt

read -n 1 -s