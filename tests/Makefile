PROGRAM_EXEC=../src/$(PROGRAM_NAME)

.PHONY: all

all:
	for t in [0-9]*.sh ; do \
	  echo $$t; \
	  bash ./$$t ../src/diff-dd; \
	  # Testing of the return value must return true on the test failure. \
	  # If the return value is zero, it means a successful test. \
	  [ $$? -ne 0 ] && exit 1 || true; \
	done
