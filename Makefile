BUILD_DIR := build

default: cslo

# Remove all build outputs and intermediate files.
clean:
	@ rm -rf $(BUILD_DIR)
	@ rm -rf gen

# used for generating a coverage build
coverage:
	@ $(MAKE) -f util/c.make NAME=cslo MODE=coverage SOURCE_DIR=src

# Compile a debug build of cslo.
debug:
	@ $(MAKE) -f util/c.make NAME=cslod MODE=debug SOURCE_DIR=src

cslo:
	@ $(MAKE) -f util/c.make NAME=cslo MODE=release SOURCE_DIR=src

cppslo:
	@ $(MAKE) -f util/c.make NAME=cppslo MODE=debug CPP=true SOURCE_DIR=src
