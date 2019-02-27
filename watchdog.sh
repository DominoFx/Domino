#!/bin/sh

# Toggle one for Master mode or Worker mode
# Toggle both for Hybrid mode
MASTER=0
WORKER=1

CONNECTED="0"
SLEEP="1"
MEDIA=""


#
# Basic configuration setup
#

sudo sh -c "echo performance > /sys/devices/system/cpu/cpu0/cpufreq/scaling_governor"
sudo sh -c "echo performance > /sys/devices/system/cpu/cpu1/cpufreq/scaling_governor"
sudo sh -c "echo performance > /sys/devices/system/cpu/cpu2/cpufreq/scaling_governor"
sudo sh -c "echo performance > /sys/devices/system/cpu/cpu3/cpufreq/scaling_governor"


#
# Helper
#

on_usb_connect()
{
    echo "Copying autorun script from $MEDIA/Domino to /home/pi..." 
    echo "Patching windows line endings..."
    sed 's/\r$//g' "$MEDIA/Domino/autorun.sh" >/home/pi/autorun.sh
    chmod 777 /home/pi/autorun.sh

    echo "Starting autorun script..."
    echo "Output redirected to autorun_output.txt, please be patient..."
    /home/pi/autorun.sh >/home/pi/autorun_output.txt 2>&1

    echo "Finished autorun script..."
    echo "Copying autorun results to Domino folder..."
    mv /home/pi/autorun* /home/pi/Domino
    
#    if [ grep -Fxq update_replace /media/pi/*/Domino/command.txt ]
#    then
#        echo "Command: Copy ..."
#    else
#        echo "Missing update command"
#    fi
}


#
# Check if USB connected
# Perform the "on usb connect" action
## Toggle on the CONNECTED flag to prevent further action until disconnect
#

check_usb_connect()
{
    if [ -e "/dev/sda" ];
    then

        cd /home
        cd /media/pi/* 2>/dev/null
        MEDIA=`pwd`
        if [ "$MEDIA" != "/home" ];
        then
            echo "Found USB drive mounted at $MEDIA"
            CONNECTED="1"

            if [ -d "$MEDIA/Domino" ];
            then
                echo "Found Domino folder..."
                on_usb_connect
            else
                echo "Missing Domino folder..."
            fi

            echo "Unmounting USB drive..."
            cd /home
            umount "$MEDIA"

            echo "Waiting for USB drive to disconnect..."
        fi
    else
        if [ "$CONNECTED" = "1" ];
        then
            CONNECTED="0"
            echo "Disconnected USB drive..."
            echo "Waiting for USB drive to connect..."
        fi
    fi
}


#
# Listen for sleep command from domino program
#

check_msg_sleep()
{
    # TODO: How to listen with "netcat" for finite amount of time, then quit?
    # Solution here is to use helper script with timeout command, but
    # script runs in a subshell, can't set variables, only returns an integer
    timeout 2 /home/pi/Domino/watchdog_helper_listen.sh
    # return value of last command executed...
    SLEEP_CHECK=`echo $?`

    if [ "$SLEEP_CHECK" = "1" ] || [ "$SLEEP_CHECK" = "2" ];
    then
        # Sleep or Awake command received
        if [ "$SLEEP_CHECK" != "$SLEEP" ];
        then
            # Command value and doesn't match current value; change mode

            if [ "$SLEEP_CHECK" = "1" ];
            then
                echo "Entering AWAKE mode"
            elif [ "$SLEEP_CHECK" = "2" ];
            then
                echo "Entering SLEEP mode"
            fi

            SLEEP=$SLEEP_CHECK

            # Kill all programs, should happen twice per day
            # Will be restarted immediately via "check run"
            echo "Stopping all master and worker processes..."
            killall domino
            killall sclang
            killall scsynth
        fi
    fi
}


#
# Restart any programs if no longer running
#

check_run()
{
    if [ -f /home/pi/Domino/config_module.json ];
    then
        MODE_CHECK=`cat /home/pi/Domino/config_module.json`
        
        MODE_COUNT=`echo $MODE_CHECK | grep -c "Master"`
        if [ "$MODE_COUNT" = "1" ];
        then
            MASTER=1
            WORKER=0
        fi

        MODE_COUNT=`echo $MODE_CHECK | grep -c "Worker"`
        if [ "$MODE_COUNT" = "1" ];
        then
            MASTER=0
            WORKER=1
        fi

        MODE_COUNT=`echo $MODE_CHECK | grep -c "Hybrid"`
        if [ "$MODE_COUNT" = "1" ];
        then
            MASTER=1
            WORKER=1
        fi
        
    fi

    PROCESS_CHECK=`ps g`

    PROCESS_COUNT=`echo "$PROCESS_CHECK" | grep -c "launch_soundMaster"`
    if [ "$PROCESS_COUNT" = "0" ] && [ "$SLEEP" = "1" ] && [ $MASTER = 1 ];
    then
        echo "Starting sound master in five seconds..."
        SCRIPT="/home/pi/Domino/launch_soundMaster.sh"
        script_run
    fi

    PROCESS_COUNT=`echo "$PROCESS_CHECK" | grep -c "launch_soundWorker"`
    if [ "$PROCESS_COUNT" = "0" ] && [ "$SLEEP" = "1" ] && [ $WORKER = 1 ];
    then
        echo "Starting sound worker in five seconds..."
        SCRIPT="/home/pi/Domino/launch_soundWorker.sh"
        script_run
    fi

    PROCESS_COUNT=`echo "$PROCESS_CHECK" | grep -c "launch_dominoMaster"`
    if [ "$PROCESS_COUNT" = "0" ] && [ $MASTER = 1 ];
    then
        echo "Starting domino master in five seconds..."
        SCRIPT="/home/pi/Domino/launch_dominoMaster.sh"
        script_run
    fi

    PROCESS_COUNT=`echo "$PROCESS_CHECK" | grep -c "launch_dominoWorker"`
    if [ "$PROCESS_COUNT" = "0" ] && [ $WORKER = 1 ];
    then
        echo "Starting domino worker in five seconds..."
        SCRIPT="/home/pi/Domino/launch_dominoWorker.sh"
        script_run
    fi
}

# Helper
script_run()
{
    sleep 5
    if [ -f "$SCRIPT" ];
    then
        chmod 777 "$SCRIPT"
        lxterminal -e "$SCRIPT"
    fi
}


#
# Main loop
#

echo "Waiting for USB drive to connect..."

while [ 1 ];
do
    check_usb_connect
    # *** DISABLED IN THIS VERSION ***
    #     Sleep mode not supported
    #check_msg_sleep
    check_run
    sleep 1
done
