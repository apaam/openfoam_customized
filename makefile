default: 
	bash install.sh

test:
	bash test.sh

clean:
	rm -rvf OpenFOAM-build/build

realclean:
	rm -rvf OpenFOAM-build

.PHONY: default test clean




