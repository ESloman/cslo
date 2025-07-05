#!/usr/bin/env bash

# simple install script for cslo
# TODO: improve to handle targets, different OS', errors, etc
# should be ran from the root of the directory

set -e

INSTALL_DIR="/usr/local/bin"

SILENT=0
VERBOSE=0
while getopts "sv" opt; do
    case $opt in
        s) SILENT=1 ;;
        v) VERBOSE=1 ;;
    esac
done

shift $((OPTIND - 1))

log() {
    # Usage: log [level] message
    # level: "info" (default), "verbose"
    local level="$1"
    if [[ $# -gt 0 ]]; then
        shift
    fi
    if [[ $SILENT -eq 1 ]]; then
        return
    fi
    if [[ "$level" == "verbose" ]]; then
        if [[ $VERBOSE -eq 1 ]]; then
            echo "$@"
            return
        fi
    else
        echo "$@"
        return
    fi
}

log info "Installing cslo to $INSTALL_DIR"

log verbose "Creating build directory"
mkdir -vp build

if [[ -f "$INSTALL_DIR/slo" ]]; then
    log verbose "Removing existing install files"
    sudo rm -f "$INSTALL_DIR/slo"
    sudo rm -f "$INSTALL_DIR/cslo"
fi

if [[ ! -f build/cslo ]]; then
    log verbose "Couldn't find build file - running 'make'."
    make clean && make
    log verbose "Build completed."
fi

# make sure we're executable
chmod +x build/cslo

sudo cp build/cslo "$INSTALL_DIR/slo"
sudo ln -fs "$INSTALL_DIR/slo" "$INSTALL_DIR/cslo"

log info "Installed slo!"
