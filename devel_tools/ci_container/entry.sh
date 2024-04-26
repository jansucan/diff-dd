#!/bin/sh -ex

# This script is used for running commands inside the container as a user with
# specified UID and GID so the files created from inside the container have the
# same owner and group as the files created outside of the container. This makes
# it easy to manipulate the files as the user outside the container.

if [ $# -lt 3 ]; then
    echo "Usage: $0 UID GID CMD [ARGS...] " >&2
    exit 1
fi

UID=$1
GID=$2
USER_NAME=new-user
GROUP_NAME=new-group
shift 2

if ! getent passwd $UID >/dev/null; then
    # The UID does not exist, create it
    useradd -m -u $UID $USER_NAME
else
    USER_NAME=$(id -n -u $UID)
fi

if ! getent group $GID >/dev/null; then
    # The GID does not exist, create it
    groupadd -g $GID $GROUP_NAME
else
    GROUP_NAME=$(getent group $GID | cut -d: -f1)
fi

# Make sure the group is the user's primary group
usermod -g $GROUP_NAME $USER_NAME

# Check that the user has expected UID and GID
ACTUAL_UID=$(su -c 'id -u' $USER_NAME)
if [ $ACTUAL_UID -ne $UID ]; then
   echo "Error: Actual UID $ACTUAL_UID != $UID"
   exit 1
fi

ACTUAL_GID=$(su -c 'id -g' $USER_NAME)
if [ $ACTUAL_GID -ne $GID ]; then
   echo "Error: Actual GID $ACTUAL_GID != $GID"
   exit 1
fi

if [ ! -e /.cache ]; then
    # Needed for running pre-commit
    mkdir /.cache
    chown $USER_NAME: /.cache
fi

su -c "$*" $USER_NAME
