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

PATTERN_FILE="/tmp/proc_pattern_file"

man_usage()
{
  echo "USAGE:  run_procanalyzer.sh {start|stop} {args}"
}

if [ $# -lt 1 ]; then
   man_usage
   exit 1
fi

arg_val=$1
case $arg_val in
        start)
                 nice -n 19 /usr/bin/cpuprocanalyzer
                 exit 0
        ;;
        stop)
            echo "*.tgz" > $PATTERN_FILE   # .tgz should be excluded while tar
            mkdir /tmp/extender_procanalyzer
            tar -X $PATTERN_FILE -cvzf /tmp/extender_procanalyzer/extender_procanalyzer.tgz /tmp/cpuprocanalyzer
            rm $PATTERN_FILE
            sleep 1
            chmod 777 -R /tmp/extender_procanalyzer
            rm -rf /tmp/cpuprocanalyzer
            exit 0
        ;;
        *)
            man_usage
            exit 0
esac

