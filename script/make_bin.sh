#!/bin/sh
cd ..
mkdir bin -p
cp team/src/* bin/ -r
cd bin
rm auto.sh *.o *.cpp *.h chain_action goalie move_def move_off neck roles setplay -r
rm train*
rm Makefile*
rm debug*
rm start.sh.in start-offline.sh
rm cyrus_trainer
#sed -i 's/use_sync_mode : on/#/g' player.conf
mkdir lib -p
cp /usr/local/cyruslib/lib/librcsc* lib/
cp ../script/start .
cp ../script/kill .
cd ..
mv bin CYRUS -f
tar -czvf CYRUS.tar.gz CYRUS
rm CYRUS -rf
