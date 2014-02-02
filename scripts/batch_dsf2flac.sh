#!/bin/bash

dsf2flac_command='dsf2flac'
dsf2flac_options='-r88200' #please don't add an -i or -o option!

if [ -n "$1" ]
then
	working_dir=$1
else
	working_dir=.
fi

export IFS=$'\n'

for dsdfile in $(find "$working_dir" -iname "*.dsf" -o -iname "*.dff" | sort)
do
	$dsf2flac_command $dsf2flac_options -i $dsdfile
done



