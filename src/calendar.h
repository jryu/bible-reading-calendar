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

#include <config.pb.h>

struct Book {
  std::string short_name;
  std::string full_name;
};

class Calendar {
  public:
    Calendar(config::CalendarConfig conf);
    void draw();
    void streamSvg(cairo_write_func_t writeFunc, void *closure);
    void streamPng(cairo_write_func_t writeFunc, void *closure);
    void streamPdf(cairo_write_func_t writeFunc, void *closure);

  private:
    std::string getBookName(const std::string& book_id);

    bool shouldInclude(const struct tm& tm);

    int countDays(int year_index);

    void initMonthIteration(int* y, int* m);
    bool isReadingMonth(int y, int m);
    bool isSelectedMonth(int y, int m);
    void nextMonth(int* y, int* m);

    std::string getPlanFileName(int year_index);

    std::queue<std::string> getBibleReadingPlan();
    void readPlanFile(const std::string file_name,
        std::queue<std::string>* bible_reading_plan);

    double getDayX(int x_index);
    double getDayY(int y_index);

    void drawMonthLabel(int month);

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

    void drawMonthOnSurface(int year, int month,
        std::queue<std::string>* bible_reading_plan,
        cairo_surface_t* surface);

    void streamMonthOnSurface(cairo_surface_t* surface);

    static std::shared_ptr<spdlog::logger> logger_;

    std::map<config::Language, std::map<std::string, Book>> books_;

    config::CalendarConfig conf_;

    cairo_t *cr_;
    double y_offset_;
    int surface_width_;
    int surface_height_;
};
