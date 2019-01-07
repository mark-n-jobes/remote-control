#!/bin/bash
stty -F /dev/ttyACM0 cs8 115200 ignbrk -brkint -icrnl -imaxbel -opost -onlcr -isig -icanon -iexten -echo -echoe -echok -echoctl -echoke noflsh -ixon -crtscts
ToolsButton="_E0E0D22D_"
EnterButton="_E0E016E9_"
DownButton="_E0E08679_"
SleepTime=0.25

echo $ToolsButton > /dev/ttyACM0
sleep $SleepTime
echo $EnterButton > /dev/ttyACM0
sleep $SleepTime
echo $EnterButton > /dev/ttyACM0
sleep $SleepTime
echo $DownButton > /dev/ttyACM0
sleep $SleepTime
echo $EnterButton > /dev/ttyACM0
sleep $SleepTime

