#!/bin/bash
#
# Copyright (c) 2018 Samsung Electronics Co., Ltd. All Rights Reserved
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#    http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

FWTEST_RUN_TEST_SH=
FWTEST_DRIVER_BIN=
FWTEST_REPORT_DIR=
FWTEST_TAP_NAME=
FWTEST_LOG_NAME=
FWTEST_TEST_NAME=

function Usage()
{
    # TODO: Fill this
    echo "Usage: LD_LIBRARY_PATH=Product/out/lib ./$0 --reportdir=report"
}

for i in "$@"
do
    case $i in
        -h|--help|help)
            Usage
            exit 1
            ;;
        --runtestsh=*)
            FWTEST_RUN_TEST_SH=${i#*=}
            ;;
        --driverbin=*)
            FWTEST_DRIVER_BIN=${i#*=}
            ;;
        --reportdir=*)
            FWTEST_REPORT_DIR=${i#*=}
            ;;
        --tapname=*)
            FWTEST_TAP_NAME=${i#*=}
            ;;
        --logname=*)
            FWTEST_LOG_NAME=${i#*=}
            ;;
        --testname=*)
            FWTEST_TEST_NAME=${i#*=}
            ;;
        --frameworktest_list_file=*)
            FRAMEWORKTEST_LIST_FILE=${i#*=}
            ;;
    esac
    shift
done

# TODO: handle exceptions for params

if [ ! -e "$FWTEST_REPORT_DIR" ]; then
    mkdir -p $FWTEST_REPORT_DIR
fi

echo ""
echo "============================================"
echo "$FWTEST_TEST_NAME with $(basename $FWTEST_DRIVER_BIN) ..."

if [ ! -z "$FRAMEWORKTEST_LIST_FILE" ]; then
    MODELLIST=$(cat "${FRAMEWORKTEST_LIST_FILE}")
fi

$FWTEST_RUN_TEST_SH --driverbin=$FWTEST_DRIVER_BIN \
    --reportdir=$FWTEST_REPORT_DIR \
    --tapname=$FWTEST_TAP_NAME \
    ${MODELLIST:-} \
    > $FWTEST_REPORT_DIR/$FWTEST_LOG_NAME 2>&1
FWTEST_RESULT=$?
if [[ $FWTEST_RESULT -ne 0 ]]; then
    echo ""
    cat $FWTEST_REPORT_DIR/$FWTEST_TAP_NAME
    echo ""
    echo "$FWTEST_TEST_NAME failed... exit code: $FWTEST_RESULT"
    echo "============================================"
    echo ""
    exit $FWTEST_RESULT
fi

echo ""
cat $FWTEST_REPORT_DIR/$FWTEST_TAP_NAME
echo "============================================"
echo ""
