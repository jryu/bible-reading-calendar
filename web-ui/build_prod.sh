readonly OUTPUT_PATH=/var/www/html/

sudo ng build --output-path=$OUTPUT_PATH --optimization
sudo sed -i 's/Bible Reading Calendar/성경 읽기 달력/g' $OUTPUT_PATH/ko/index.html
