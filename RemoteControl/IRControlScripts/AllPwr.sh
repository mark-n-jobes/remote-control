#!/bin/bash
stty -F /dev/ttyACM0 cs8 115200 ignbrk -brkint -icrnl -imaxbel -opost -onlcr -isig -icanon -iexten -echo -echoe -echok -echoctl -echoke noflsh -ixon -crtscts
echo _C2CA807F_ > /dev/ttyACM0
echo _E0E040BF_ > /dev/ttyACM0


