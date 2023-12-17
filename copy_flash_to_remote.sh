#!/bin/bash

if [[ "$1" == "" ]]
then
    echo "no host given; call with copy_flash_to_remote host"
fi

HOST=$1

rsync -a .pio/build/esp32/*.bin root@${HOST}:/tmp
rsync -a flash_on_remote.sh root@${HOST}:/tmp
rsync -a filesystem_on_remote.sh root@${HOST}:/tmp

ssh root@${HOST} "cd /tmp; ./flash_on_remote.sh"

date
