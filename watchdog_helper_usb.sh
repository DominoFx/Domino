#!/bin/sh


#
# **** INSTRUCTIONS ****
# 1) Rename this file to "autorun.sh"
# 2) Place in folder named "Domino" in the root of a USB drive
# 3) Executed by watchdog script when USB drive is connected
#


echo "In autorun script, started..."

MODULE="UNNAMED"
MEDIA=""
UPDATE_TIMESTAMP=""
UPDATE_SRC_FOLDER=""
UPDATE_DEST_FOLDER=""
UPDATE_BACKUP_FOLDER=""
UPDATE_BACKUP_FILE=""

cd /media/pi/*
MEDIA=`pwd`
# No error handling if MEDIA is missing...

#
# Make a backup of dest folder
# Skip if dest folder missing
#

cd /home
cd /home/pi/Domino
UPDATE_DEST_FOLDER=`pwd`
if [ "$UPDATE_DEST_FOLDER" = "/home" ];
then
    echo "Missing Domino old folder for backup: /home/pi/Domino..."
    echo "Creating folder: /home/pi/Domino..."
    mkdir "$UPDATE_DEST_FOLDER"
else
    # Existing Domino folder, back it up
    echo "Found Domino folder for backup: $UPDATE_DEST_FOLDER..."
    
    UPDATE_BACKUP_FOLDER="$MEDIA/Domino/Backup"

    # Read module name from the module identification file
    if [ -f "$UPDATE_DEST_FOLDER/config_module.json" ];
    then
        MODULE=`cat "$UPDATE_DEST_FOLDER"/config_module.json | grep -E "[A-Za-z0-9_]+"`
        echo "Identified Domino module: [$MODULE]..."
    else
        echo "Missing Domino module identification..."
    fi

    UPDATE_TIMESTAMP=`date +%Y%m%d_%H%M`
    UPDATE_BACKUP_FILE="$UPDATE_BACKUP_FOLDER"/Backup_Domino_"$MODULE"_"$UPDATE_TIMESTAMP".zip

    echo "Creating backup zipfile: $ZIPFILE..."
    zip -r "$UPDATE_BACKUP_FILE" "$UPDATE_DEST_FOLDER"/* -x */build/* */dist/*
fi


#
# Helper for copying files
#

copy_files()
{
    echo "Copying files from $UPDATE_SRC_FOLDER to $UPDATE_DEST_FOLDER..."
    
    cp -r "$UPDATE_SRC_FOLDER"/* "$UPDATE_DEST_FOLDER"
    chmod 777 "$UPDATE_DEST_FOLDER"/*.sh
    
    if [ -f "%UPDATE_SRC_FOLDER/autostart" ];
    then
        echo "Copying autostart file..."
        cp "%UPDATE_SRC_FOLDER/autostart" /home/pi/.config/lxsession/LXDE-pi
    fi
}


#
# Copy common files into dest folder if possible
# Skip if source folder missing
#

cd /home
cd "$MEDIA/Domino/Module_Common"
UPDATE_SRC_FOLDER=`pwd`
if [ "$UPDATE_SRC_FOLDER" = "/home" ];
then
    echo "Missing Domino source common folder, $MEDIA/Domino/Module_Common ..."
else
    echo "Deleting files from $UPDATE_DEST_FOLDER..."
    rm -r "$UPDATE_DEST_FOLDER"/*
    
    copy_files
fi


#
# Copy module-specific files into dest folder if possible
# Skip if source folder missing
#

cd /home
cd "$MEDIA/Domino/$MODULE"
UPDATE_SRC_FOLDER=`pwd`
if [ "$UPDATE_SRC_FOLDER" = "/home" ];
then
    echo "Missing Domino source module folder, $MEDIA/Domino/$MODULE ..."
else
    copy_files
fi


#
# Ensure the module identification file exists
#

if [ ! -f "$UPDATE_DEST_FOLDER/config_module.json" ];
then
    echo "$MODULE" > "$UPDATE_DEST_FOLDER/config_module.json"
fi


#
# Build from source files in dest folder if necessary
# Skip if source folder missing
#

cd /home
cd /home/pi/Domino
UPDATE_DEST_FOLDER=`pwd`
if [ "$UPDATE_DEST_FOLDER" = "/home" ];
then
    echo "Missing Domino destination folder, /home/pi/Domino/ ..."
else
    if [ ! -f "./dist/Release/GNU-Linux/domino" ];
    then
        echo "Missing Domino binaries, building ..."
        make all
    fi
fi


#
# Kill processes so new changes take effect
# Watchdog will restart processes
#

echo "Stopping all master and worker processes..."
killall domino
killall sclang
killall scsynth


echo "In autorun script, finished..."
