#include <cairo.h>
#include <cairo-pdf.h>
#include <cairo-svg.h>
#include <fcntl.h>
#include <fstream>
#include <google/protobuf/io/zero_copy_stream_impl.h>
#include <google/protobuf/text_format.h>
#include <iomanip>
#include <iostream>
#include <librsvg/rsvg.h>
#include <pango/pangocairo.h>
#include <queue>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <sstream>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "config.pb.h"

struct Book {
  std::string short_name;
  std::string full_name;
};

class Calendar {
  public:
    Calendar(config::CalendarConfig conf);
    void draw();

  private:
    std::string getBookName(const std::string& book_id);

    bool shouldInclude(const struct tm& tm);

    int countDays(int year);

    std::string getPlanFileName();

    std::queue<std::string> getBibleReadingPlan();

    double getDayX(int x_index);
    double getDayY(int y_index);

    void drawMonthLabel(int month, int surface_width);

    void drawWdayLabel();

    void drawDaysOfMonth(int year, int month,
        std::queue<std::string>* bible_reading_plan);

    void drawTextOfDayNumber(int x, int y,
        const char* text);

    void drawTextOfDayPlan(int x, int y, const
        std::string text);

    void skipMonth(int year, int month,
        std::queue<std::string>* bible_reading_plan);

    void drawMonth(int year, int month,
        std::queue<std::string>* bible_reading_plan);

    std::shared_ptr<spdlog::logger> logger_;
    std::map<config::Language, std::map<std::string, Book>> books_;

    config::CalendarConfig conf_;

    double y_offset_;
    cairo_t *cr_;
};
