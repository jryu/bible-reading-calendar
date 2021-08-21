set -e

cd build
cmake --build .
cd ..
build/cli --bible_reading_plans_path=/home/jryu/bible-reading-calendar/bible-reading-plans/
