set -e
set -x

make main

NO_SCAN=1 INT=10  ./main
NO_SCAN=1 INT=1000  ./main
NO_SCAN=1 LONG1=480 ./main
NO_SCAN=1 LONG2=480 ./main
INT=10  ./main
INT=1e6  ./main
# you can get the urls file from moodle and gunzip it
FILE=data/urls ./main