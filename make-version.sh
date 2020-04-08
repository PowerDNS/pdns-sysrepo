#!/bin/sh

VERSION="${BUILDER_VERSION}"

if [ -z "${VERSION}" ]; then
  VERSION="$(builder/gen-version)"
fi

if [ "${VERSION}" = "unknown" ]; then
  if [ -r ".version" ]; then
    # the dist-fix script ensures a .version is created
    VERSION="$(cat .version)"
  fi
fi

printf $VERSION
