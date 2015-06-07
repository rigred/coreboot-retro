#!/bin/sh

# This file is part of the coreboot project.
#
# Copyright (C) 2014 Sage Electronic Engineering, LLC.
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; version 2 of the License.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc.
#

DATE=""
GITREV=""
TIMESOURCE=""

export LANG=C
export TZ=UTC

if [ -d "${top}/.git" ] && [ -f "$(command -v git)" ]; then
	GITREV=$(LANG= git log remotes/origin/master -1 --format=format:%h)
	TIMESOURCE=git
	DATE=$(git log --pretty=format:%ct -1)
else
	GITREV=Unknown
	TIMESOURCE="LANG=C LC_ALL=C TZ=UTC date"
	DATE=$(date +%s)
fi

our_date() {
case $(uname) in
NetBSD|OpenBSD|DragonFly|FreeBSD)
	date -r $1 $2
	;;
*)
	date -d @$1 $2
esac
}

#Print out the information that goes into build.h
printf "/* build system definitions (autogenerated) */\n"
printf "#ifndef __BUILD_H\n"
printf "#define __BUILD_H\n\n"
printf "#define COREBOOT_VERSION %s\n" "\"$KERNELVERSION\""

#See if the build is running in a git repo and the git command is available
printf "/* timesource: $TIMESOURCE */\n"
printf "#define COREBOOT_VERSION_TIMESTAMP $DATE\n"
printf "#define COREBOOT_ORIGIN_GIT_REVISION \"$GITREV\"\n"

printf "#define COREBOOT_EXTRA_VERSION \"%s\"\n" "$COREBOOT_EXTRA_VERSION"
printf "#define COREBOOT_BUILD \"$(our_date "$DATE")\"\n"
printf "#define COREBOOT_BUILD_YEAR_BCD 0x$(our_date "$DATE" +%y)\n"
printf "#define COREBOOT_BUILD_MONTH_BCD 0x$(our_date "$DATE" +%m)\n"
printf "#define COREBOOT_BUILD_DAY_BCD 0x$(our_date "$DATE" +%d)\n"
printf "#define COREBOOT_BUILD_WEEKDAY_BCD 0x$(our_date "$DATE" +%w)\n"
printf "#define COREBOOT_DMI_DATE \"$(our_date "$DATE" +%m/%d/%Y)\"\n"
printf "\n"
printf "#define COREBOOT_COMPILE_TIME \"$(our_date "$DATE" +%T)\"\n"
printf "#endif\n"
