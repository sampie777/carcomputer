#!/bin/bash

progname=$(basename $0)

function usage {
  cat << HEREDOC

     Usage: $progname [command]

     commands:
       patch                  Release a patch version (0.0.X)
       minor                  Release a minor version (0.X.0)
       major                  Release a major version (X.0.0)
       setversion <version>   Change version to <version>
       -h, --help             Show this help message and exit

HEREDOC
}

function setVersion() {
    version="$1"

    sed -i -e "s/APP_VERSION = \".*\";/APP_VERSION = \"${version}\";/g" ./main/app_version.h
}

function releasePatch {
  git checkout master || exit 1

  # Create patch version
  CURRENT_VERSION=$(cat ./main/app_version.h | grep "APP_VERSION = \"" | awk -F'"' '{print $2}')
  RELEASE_VERSION=$(echo ${CURRENT_VERSION} | sed 's/v//g' | awk -F'.' '{print $1"."$2"."$3+1}')

  git merge develop || exit 1

  setVersion "${RELEASE_VERSION}" || exit 1

  pushAndRelease "${RELEASE_VERSION}"
}

function releaseMinor {
  git checkout master || exit 1
  git merge develop || exit 1

  # Create patch version
  CURRENT_VERSION=$(cat ./main/app_version.h | grep "APP_VERSION = \"" | awk -F'"' '{print $2}')
  RELEASE_VERSION=$(echo ${CURRENT_VERSION} | awk -F'.' '{print $1"."$2".0"}')

  setVersion "${RELEASE_VERSION}" || exit 1

  pushAndRelease "${RELEASE_VERSION}"
}

function releaseMajor {
  git checkout master || exit 1
  git merge develop || exit 1

  # Create patch version
  CURRENT_VERSION=$(cat ./main/app_version.h | grep "APP_VERSION = \"" | awk -F'"' '{print $2}')
  RELEASE_VERSION=$(echo ${CURRENT_VERSION} | awk -F'.' '{print $1+1".0.0"}')

  setVersion "${RELEASE_VERSION}" || exit 1

  pushAndRelease "${RELEASE_VERSION}"
}

function pushAndRelease {
  RELEASE_VERSION="$1"
  echo "Release version: ${RELEASE_VERSION}"

  git add main/app_version.h || exit 1
  git commit -m "version release: ${RELEASE_VERSION}" || exit 1
  git tag "v${RELEASE_VERSION}" || exit 1
  git push -u origin master --tags || exit 1
}

function setNextDevelopmentVersion {
  git checkout develop || exit 1
  git rebase master || exit 1

  # Generate next (minor) development version
  CURRENT_VERSION=$(cat ./main/app_version.h | grep "APP_VERSION = \"" | awk -F'"' '{print $2}')
  DEV_VERSION=$(echo ${CURRENT_VERSION} | sed 's/v//g' | awk -F'.' '{print $1"."$2+1".0-SNAPSHOT"}')

  echo "Next development version: ${DEV_VERSION}"
  setVersion "${DEV_VERSION}" || exit 1

  git add main/app_version.h || exit 1
  git commit -m "next development version" || exit 1
  git push -u origin develop --tags || exit 1
}

command="$1"
case $command in
  patch)
    releasePatch
    setNextDevelopmentVersion
    ;;
  minor)
    releaseMinor
    setNextDevelopmentVersion
    ;;
  major)
    releaseMajor
    setNextDevelopmentVersion
    ;;
  setversion)
    setVersion "$2"
    ;;
  -h|--help)
    usage
    ;;
  *)
    echo "Invalid command"
    exit 1
    ;;
esac
