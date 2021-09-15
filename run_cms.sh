set -e

cd build
#cmake -DCMAKE_BUILD_TYPE=Debug ../src
cmake --build .
cd ..

ulimit -c unlimited
rm -f core

build/bible-reading-calendar --undefok=c --bible_reading_plans_path=/home/jryu/bible-reading-calendar/bible-reading-plans/ -c src/conf-dev.js
