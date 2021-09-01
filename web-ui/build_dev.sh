readonly OUTPUT_PATH=/home/jryu/bible-reading-calendar/static

ng build --output-path=$OUTPUT_PATH --output-hashing none
sed -i 's/Bible Reading Calendar/성경 읽기 달력/g' $OUTPUT_PATH/ko/index.html
