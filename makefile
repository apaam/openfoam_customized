default: 
	bash install.sh

test:
	bash test.sh

clean:
	rm -rf OpenFOAM-build/build

realclean:
	rm -rf OpenFOAM-build

.PHONY: default test clean




