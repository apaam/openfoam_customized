MAKE_PID := $(shell echo $$PPID)
JOB_FLAG := $(filter -j%, $(subst -j ,-j,$(shell ps T | grep "^\s*$(MAKE_PID).*$(MAKE)")))
JOBS     := $(subst -j,,$(JOB_FLAG))

default: get_jobs
	NUM_CORES=$(JOBS) bash install.sh

test: get_jobs
	NUM_CORES=$(JOBS) bash test.sh

get_jobs:
	@if [ -n "$(JOBS)" ]; then \
		if [ "$(JOBS)" = "" ]; then \
			echo "Parallel jobs are enabled (default jobserver mode)"; \
		else \
			echo "Parallel jobs: $(JOBS)"; \
		fi; \
	else \
		echo "No parallel jobs specified."; \
	fi

clean:
	rm -rf OpenFOAM-build/build

realclean:
	rm -rf OpenFOAM-build

.PHONY: default test clean




