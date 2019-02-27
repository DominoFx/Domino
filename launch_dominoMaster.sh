
#
# Launch program
#

cd /home/pi/Domino
echo `date` >>./error_domino_master.txt
chmod 777 ./dist/Release/GNU-Linux/domino
./dist/Release/GNU-Linux/domino -m 2>>./error_domino_master.txt


# Wait for user input, to prevent window from closing after ctrl-c,
# if lxterminal was launched at startup, but this never worked, hmm
#read -n 1 -s
