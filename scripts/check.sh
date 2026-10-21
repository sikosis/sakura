#!/bin/sh

set -eu

repository_root=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
cd "$repository_root"

required_files="
LICENSE
Makefile
resources/Sakura.rdef
resources/icon.png
src/AboutWindow.cpp
src/AboutWindow.h
src/MainWindow.cpp
src/MainWindow.h
src/SakuraApp.cpp
src/SakuraApp.h
"

for required_file in $required_files; do
	if [ ! -s "$required_file" ]; then
		echo "Missing or empty required file: $required_file" >&2
		exit 1
	fi
done

if ! grep -q 'application/x-vnd.sikosis-Sakura' resources/Sakura.rdef; then
	echo "Application signature is missing from resources/Sakura.rdef" >&2
	exit 1
fi

if ! grep -q 'application/x-vnd.sikosis-Sakura' src/SakuraApp.h; then
	echo "Application signature is missing from src/SakuraApp.h" >&2
	exit 1
fi

if grep -n 'B_TRANSLATE([^)]*)[[:space:]]*B_UTF8_ELLIPSIS' src/*.cpp; then
	echo "B_UTF8_ELLIPSIS must be inside B_TRANSLATE()." >&2
	exit 1
fi

echo "Sakura source and resource checks passed."
