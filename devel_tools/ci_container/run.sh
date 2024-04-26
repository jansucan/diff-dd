#!/bin/sh

USER_UID=$(id -u)
USER_GID=$(id -g)

# Intended to run from root directory of the project
docker run -v .:/diff-dd ghcr.io/jansucan/diff-dd-ci $USER_UID $USER_GID "$@"
