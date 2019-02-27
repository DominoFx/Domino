
#
# Launch program
#

cd /home/pi/Domino
echo `date` >>/home/pi/Domino/error_sound_worker.txt
sclang /home/pi/Domino/SuperCollider/dominoWorker_181209.scd 2>>/home/pi/Domino/error_sound_worker.txt


# Wait for user input; if lxterminal was launched at startup,
# prevent the window from closing on ctrl-c, but this never worked, hmm
#read -n 1 -s
