#!/bin/bash
#
# siege GUI script for 6atranj distro
#Siege is an HTTP load testing and benchmarking utility. It has a similar interface to ab, which will make transitioning to the tool almost 
#seamless. Also, instead of testing against a single URL, Siege allows you to test against multiple. This allows for a more real-world simulation #of how a user would use your system.
#
# Author: Mehrdad Dadkhah
# Email: dadkhah.ir at gmail.com
# Licence: GPL
# fixed icon display in systray, move zenity,
# based now on yad.
#
Encoding=UTF-8
#
#
# define some variables
#
TITLE='Siege benchmarking'
VERSION=2.7
ICON=/usr/share/icons/6atranj-icons/siege.gif

#
#questions
#
function menu {
data=($(yad --form --title="$TITLE"" $VERSION" --window-icon=$ICON \
			--on-top \
			--center \
			--margins=10 \
			--image=$ICON \
			--image-on-top \
			--borders=15 \
			--item-separator=, \
			--separator="," \
			--field="url to test (if want to use list of urls, leave it empty):TEXT" \
			--field="how many users hitting the resource simultaneously:NUM" \
			--field="delay between requests:NUM" \
			--field="time to benchmark/minutes:NUM" \
			--button=$"attach file of URLs:7" \
               --button=$"Test:8" \
			--button="more options:9" \
               --button="gtk-help:10" \
               --button="gtk-close:11"
               
      ))

ret=$?

IFS=',' read -a array <<< "$data"

#
#file of URLs
#
if [[ $ret -eq 7 ]]; then
    CHANGE=$(yad --title="$TITLE"" $VERSION" --window-icon=$ICON \
		--file --width=600 --height=500 \
		--text=$"<b>Choose your own audio file as alert!</b>
________________________________________________")
		if [ "$CHANGE" ];then 
			if [[ ! -z "${array[2]}" &&  ! -z "${array[1]}" &&  ! -z "${array[3]}" ]];then
				siege -b -d"${array[2]}" -c"${array[1]}" -t"${array[3]}"M  -i -f "$CHANGE"
			else
				yad --title $"$TITLE"" $VERSION" \
			    --button="gtk-ok:0" \
			    --width 300 \
			    --window-icon=$ICON \
			    --text=$"Your own file of URLs add successfully ... "
			fi
		else
			zenity --warning \
			--title='oh! your file could not attach ... ' \
			--text='please enter url or import txt file of urls ... '
		fi
menu		
fi
#
#Test url
#
if [[ $ret -eq 8 ]]; then
	if [[ -z "$CHANGE" ]]; then
		if [[ -z "${array[0]}" ]]; then
			zenity --warning \
			--title='wrong data ... !' \
			--text='please enter url or import txt file of urls ... '
		else
			siege -b -d"${array[2]}" -c"${array[1]}" -t"${array[3]}"M "${array[0]}" | zenity --progress --pulsate
		fi
	else
		siege -b -d"${array[2]}" -c"${array[1]}" -t"${array[3]}"M  -i -f "$CHANGE"
	fi
menu                 
fi

#
#more options and help ...
#
if [[ $ret -eq 9 || $ret -eq 10 ]]; then
	siege --help
fi

[[ $ret -eq 11 ]] && exit 0


}
menu