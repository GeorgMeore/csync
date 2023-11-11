#!/bin/sh -xe

case "$*" in
	'') gcc -g -Wall -Wextra -lpthread -o main main.c ;;
	-c) rm -f main ;;
	*)
		echo "usage: $0 [-c]"
		exit 1
esac
