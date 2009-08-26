#!/bin/sh

CHLOGBKP="/tmp/$$-chlog"
BASEVERSION="0.93.0"
REPOVERSION="git-`date +%Y%m%d-%H%M`"

CHANGELOG="Automatic build from the GIT on `date`"
BUILDLOG="${HOME}/log/wmaker-testing-nightly.log"

STATUSCMD="git log -1 --pretty=oneline"
PULLCMD="git pull"
FORCE="0"

if [ $# -gt 0 ]; then
  case $1 in
    -f|--force)
      FORCE="1"
      ;;
    *)
      echo "Usage $0 [-f|--force]"
      exit 0
      ;;
  esac
fi

cleanup() {
  test -f ${CHLOGBKP} && mv ${CHLOGBKP} debian/changelog 
}

errorExit() {
  echo "$@" 1>&2
  cleanup
  exit 2
}

doPull() {
  RC="1"
  LATEST="`$STATUSCMD`"
  $PULLCMD >>$BUILDLOG 2>&1 || errorExit "Error pulling from the repo"
  CURRENT="`$STATUSCMD`"
  if [ "$FORCE" = "1" -o "$LATEST" != "$CURRENT" ]; then
    echo "last revision: $LATEST" >>$BUILDLOG 
    echo "new revision: $CURRENT" >>$BUILDLOG 
    echo "FORCE: $FORCE" >>$BUILDLOG 
    RC="0"
  else
    echo "No changes to build, and FORCE not given." >>$BUILDLOG
  fi
  return $RC
}

doEnv() {
  for var in `env | grep GNUSTEP | sed "s/=.*//"`; do
    unset $var
  done
}

doChlog() {
  cp debian/changelog ${CHLOGBKP}
  debchange -v ${BASEVERSION}-${REPOVERSION} "${CHANGELOG}" || return 1
}

doBuild() {
  ARGS=""
  if [ -n "${WM_GPG_KEY}" ]; then
    ARGS="-k${WM_GPG_KEY}"
  fi
  fakeroot dpkg-buildpackage $ARGS >>${BUILDLOG} 2>&1
}

install -d `dirname $BUILDLOG`
>$BUILDLOG
trap 'cleanup; exit 2' 2
trap 'cleanup; exit 2' 9
trap 'cleanup; exit 2' 15

test -d debian || errorExit "This script must be called from the toplevel source dir, ./debian must exist."

RC=0
if doPull ; then
  doEnv
  doChlog || errorExit "Error adding new changelog entry."
  doBuild || errorExit "Error during build of .deb package."
else
  RC=1
fi

cleanup
exit $RC
