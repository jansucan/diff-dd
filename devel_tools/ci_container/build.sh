#!/bin/sh

docker buildx build -t ghcr.io/jansucan/diff-dd-ci:latest -f devel_tools/ci_container/Dockerfile .
