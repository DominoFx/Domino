
#
# Launch program
#

cd /home/pi/Domino
echo `date` >>./error_sound_master.txt
sclang ./SuperCollider/mother_181209.scd


# Wait for user input; if launched via lxterminal,
# should prevent window from closing on ctrl-c, but never worked, hmm
#read -n 1 -s
