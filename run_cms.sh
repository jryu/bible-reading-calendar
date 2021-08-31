set -e

cd build
cmake --build .
cd ..
build/bible-reading-calendar --undefok=c --bible_reading_plans_path=/home/jryu/bible-reading-calendar/bible-reading-plans/ -c src/conf-dev.js
