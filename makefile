default: 
	bash install.sh

test:
	bash test.sh

clean:
	rm -rvf build

.PHONY: default test clean




