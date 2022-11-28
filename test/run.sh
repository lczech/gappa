#!/bin/bash

# ==================================================================================================
#      Initialization
# ==================================================================================================

# Change to the directory where this script is located, so that we can call it from anywhere.
cd `dirname ${0}`

# Prepare output directories for log files and files produced by the commands.
BASEOUTDIR="out"
LOGDIR="log"
rm -r ${BASEOUTDIR}/
rm -r ${LOGDIR}/
mkdir ${BASEOUTDIR}
mkdir ${LOGDIR}

# This script needs bc.
if [ -z "`which bc`" ] ; then
    echo "Program 'bc' not found. Cannot run this script."
    exit
fi

# On MacOS, the default `date` does not support nanoseconds, which we however use here...
# So make sure that we have the gnu core utils version available, and alias this.
# See https://apple.stackexchange.com/a/47181
if [[ $OSTYPE == 'darwin'* ]]; then
    if [ -z "`which gdate`" ] ; then
        echo "Program 'gdate' not found. Cannot run this script on MacOS."
        exit
    fi
fi

# Color the spectrum!
COLOR_RED="\033[31m"
COLOR_GREEN="\033[32m"
COLOR_END="\033[0m"

# ==================================================================================================
#      Helper Functions
# ==================================================================================================

# Test that file ${1} exists and has correct size ${2}, or between ${2} and ${3}.
# The latter is needed for gz files, which can differ slighly in size depending on content,
# which changes due to creation time stamps being part of many of our output files,
# or for files that involve some randomness (k-means results for example).
# Unfortunately, this can only be determined via try and error by running the tests multiple times...
# So, we also simply also use an error margin, to make our lives simple...
# (that was introduced later, so some test files are still using the size interval)
testfile() {
    # See if file exists and get its size.
    local RESULT=0
    [[ -f ${1} ]] || RESULT=1
    local SIZE=`stat --printf="%s" ${1}`

    # If three args given, use ${2} and ${3} and min max, otherwise expect exactly ${2}
    local MIN=${2}
    local MAX=${2}
    [[ "${3}" ]] && local MAX=${3}

    # Allow a 2% error margin for file sizes. This allows for some variation due to randomness,
    # but still is close enough so that we catch files that are completely different.
    # Bash can only do intergers, so we need a bit of math here.
    local MIN=$(( ${MIN} - ${MIN} / 50 ))
    local MAX=$(( ${MAX} + ${MAX} / 50 ))

    # Test the filesize.
    [[ ${SIZE} -ge ${MIN} && ${SIZE} -le ${MAX} ]] || local RESULT=1
    if [[ ${RESULT} != 0 ]]; then
        echo -e "\nError in test file ${1}:"
        echo "File does not exist or does not have size between ${MIN} and ${MAX}, but ${SIZE}"
        ls -l ${1}
    fi
    return ${RESULT}
}

# For macos, we need a different date function that is gnu compatible and supports nanoseconds.
nanotime() {
    if [[ $OSTYPE == 'darwin'* ]]; then
        gdate +%s%N
    else
        date +%s%N
    fi
}

# ==================================================================================================
#      Find Binary
# ==================================================================================================

# Try to use $PATH by default, then try local binary
GAPPA=$(which gappa 2> /dev/null)
[[ -z "${GAPPA}" ]] && GAPPA="../bin/gappa"
# [[ "${1}" ]] && GAPPA="${1}"

# Check that we got an executable
if [[ ! -x "${GAPPA}" ]] ; then
    echo "gappa binary not found or is not executable"
    exit
fi

# ==================================================================================================
#      Launch Tests
# ==================================================================================================

# Either get all scripts that we have, or the use provided ones via wildcard inclusion
SCRIPTS=`ls ./scripts/*.sh`
[[ "${1}" ]] && SCRIPTS=`ls ./scripts/*${1}*.sh`

# Some user output. Ugly: We except paths names without spaces here...
# But we just assume that the user does not have directories with spaces in them.
# Worst case: the test counter is wrong. We can live with that.
TESTCOUNT=`echo ${SCRIPTS} | sed 's/ /\n/g' | wc -l`
printf "${COLOR_GREEN}[==========]${COLOR_END} ${TESTCOUNT} tests\n\n"

# Status: How often did we fail? How long did we need in total?
FAILCOUNT=0
STARTTIME=`nanotime`

# Run all 'em scripts!
for SCRIPT in ${SCRIPTS} ; do
    if [[ ! -f ${SCRIPT} ]]; then
        echo "Test case ${SCRIPT} not found"
        exit
    fi

    # Get pure file name as our test name, and print for user
    TESTNAME=$(basename "${SCRIPT%.*}")
    printf "${COLOR_GREEN}[ RUN      ]${COLOR_END} ${TESTNAME}\n"

    # Make sure that each test writes to its own out and log directories
    OUTDIR=${BASEOUTDIR}/${TESTNAME}
    mkdir ${OUTDIR}

    # Run the test, embedded in measurement tools and user printouts.
    # We source the test scripts, so that they have access to all helper functions and vars here.
    STIME=`nanotime`
    source "${SCRIPT}" "${GAPPA}" > ${LOGDIR}/${TESTNAME}.log 2>&1
    RESULT=$?
    ETIME=`nanotime`
    DURATION=`echo "scale=3;(${ETIME} - ${STIME})/(10^09)" | bc | awk '{printf "%.3f", $0}'`

    # Final user printout for the test
    if [[ ${RESULT} == 0 ]]; then
        printf "${COLOR_GREEN}[     PASS ]${COLOR_END} ${DURATION}s\n"
    else
        printf "${COLOR_RED}[     FAIL ]${COLOR_END} ${DURATION}s\n"
        FAILCOUNT=$((FAILCOUNT+1))
    fi
done

# Some user output.
ENDTIME=`nanotime`
TOTALDURATION=`echo "scale=3;(${ENDTIME} - ${STARTTIME})/(10^09)" | bc | awk '{printf "%.3f", $0}'`
printf "\n${COLOR_GREEN}[==========]${COLOR_END} ${TOTALDURATION}s\n"

if [[ ${FAILCOUNT} == 0 ]]; then
    printf "${COLOR_GREEN}[   PASSED ]${COLOR_END} ${TESTCOUNT} tests\n"
else
    printf "${COLOR_RED}[   FAILED ]${COLOR_END} ${FAILCOUNT} tests\n"
fi

exit ${FAILCOUNT}
