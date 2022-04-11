#!/bin/bash

NUM_CORES=${NUM_CORES:-2}

mkdir -p OpenFOAM-build

if [[ "$OSTYPE" == "linux-gnu"* ]]; then
	cp -urvf OpenFOAM-v2112/* OpenFOAM-build
	cd OpenFOAM-build

	source etc/bashrc
	foamSystemCheck
	./Allwmake -j $(NUM_CORES) -s -q -k; ./Allwmake -j $(NUM_CORES) -s
elif [[ "$OSTYPE" == "darwin"* ]]; then
	rsync -urva OpenFOAM-v2112/* OpenFOAM-build
	rsync -u Brewfile OpenFOAM-build/Brewfile
	rsync -u configure.sh OpenFOAM-build/configure.sh
	cd OpenFOAM-build

	brew bundle -f
	brew bundle check --verbose --no-upgrade
	cat Brewfile.lock.json	
  bash -ex configure.sh
	
	source etc/bashrc
	foamSystemCheck
	./Allwmake -j $(NUM_CORES) -s -q -k; ./Allwmake -j $(NUM_CORES) -s
else
  echo "$OSTYPE not support"
fi