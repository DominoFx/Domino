
#
# Launch program
#

cd /home/pi/Domino
echo `date` >>./error_domino_worker.txt
chmod 777 ./dist/Release/GNU-Linux/domino
./dist/Release/GNU-Linux/domino -w 2>>./error_domino_worker.txt

# Redirect both standard and error output
#./dist/Release/GNU-Linux/domino -w  >./error_domino_worker_full.txt 2>&1


# Wait for user input, to prevent window from closing after ctrl-c,
# if lxterminal was launched at startup, but this never worked, hmm
#read -n 1 -s
