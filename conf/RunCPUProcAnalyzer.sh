##########################################################################
# If not stated otherwise in this file or this component's "LICENSE"
# file the following copyright and licenses apply:
#
# Copyright 2018 RDK Management
#
# Licensed under the Apache License, Version 2.0 (the "LICENSE");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
##########################################################################

#!/bin/sh

source /etc/device.properties
source /etc/log_timestamp.sh

if [ "$BOX_TYPE" = "HUB4" ] || [ "$BOX_TYPE" = "SR213" ]; then
    #Interface name varies based on the type of connection.
    #It can be erouter0, vdsl0 or a different name. Better to get it from the DM.
    WANINTERFACE=`dmcli eRT getv Device.X_RDK_WanManager.CurrentActiveInterface | grep value | awk '{print $NF}'`
else
    WANINTERFACE="erouter0"
fi

PATTERN_FILE="/tmp/proc_pattern_file"

man_usage()
{
  echo "USAGE:   RunCPUProcAnalyzer.sh {start|stop} {args}"
}

# Get the MAC address of the machine
getMacAddressOnly()
{
    mac=`ifconfig $WANINTERFACE | grep 'HWaddr' | awk '{print $NF}' | sed 's/://g'`
    echo $mac
}

if [ $# -lt 1 ]; then
   man_usage
   exit 1
fi

arg_val=$1
NeedUpload=$2
case $arg_val in
        start)
            touch /tmp/PROC_ANALYZER_ENABLE
            exit 0
        ;;
        stop)
            if [ "$NeedUpload" -eq 1 ]; then
                 MAC=`getMacAddressOnly`
                 dt=`date "+%m-%d-%y-%I-%M%p"`
                 echo "*.tgz" > $PATTERN_FILE   # .tgz should be excluded while tar
                 mkdir /tmp/$dt
                 tar -X $PATTERN_FILE -cvzf /tmp/$dt/$MAC"_CPAstats_"$dt".tgz" /tmp/cpuprocanalyzer
                 rm $PATTERN_FILE
                 sleep 1
                 chmod 777 -R /tmp/$dt
                 /rdklogger/uploadRDKBLogs.sh "" HTTP "" false "" /tmp/$dt
                 sleep 1;
            fi

            rm -rf /tmp/cpuprocanalyzer

	    if [ "$NeedUpload" -eq 1 ]; then
                 rm -rf /tmp/$dt
	    fi
            exit 0
        ;;
        *)
            man_usage
            exit 0
esac
