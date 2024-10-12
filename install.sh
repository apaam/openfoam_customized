#!/bin/bash

NUM_JOBS=${NUM_JOBS:-2}
echo "NUM_JOBS: $NUM_JOBS"

mkdir -p OpenFOAM-build

if [[ "$OSTYPE" == "linux-gnu"* ]]; then
	rsync -ura OpenFOAM-v2112/* OpenFOAM-build
	cd OpenFOAM-build

	source etc/bashrc
	foamSystemCheck
	./Allwmake -j $NUM_JOBS -s -q -k
	./Allwmake -j $NUM_JOBS -s
elif [[ "$OSTYPE" == "darwin"* ]]; then
	rsync -ura OpenFOAM-v2112/* OpenFOAM-build
	rsync -u Brewfile OpenFOAM-build/Brewfile
	rsync -u configure.sh OpenFOAM-build/configure.sh
	cd OpenFOAM-build

	brew bundle -f
	brew bundle check --verbose --no-upgrade
	cat Brewfile.lock.json
	bash -ex configure.sh

	source etc/bashrc
	foamSystemCheck
	./Allwmake -j $NUM_JOBS -s -q -k
	./Allwmake -j $NUM_JOBS -s
else
	echo "$OSTYPE not support"
fi
