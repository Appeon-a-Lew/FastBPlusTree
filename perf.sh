set -e
set -x

make main-optimized
INT=5e7 FILE=data/urls.txt ./main-optimized