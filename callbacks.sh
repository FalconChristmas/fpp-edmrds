#!/bin/bash

# Marks this as a C++ (.so) plugin so FPP loads libfpp-edmrds.so.

for var in "$@"
do
	case $var in
		-l|--list)
			echo "c++"
			exit 0
		;;
		*)
			exit 0
		;;
	esac
done
