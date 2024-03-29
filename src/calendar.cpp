#include <cairo.h>
#include <cairo-pdf.h>
#include <cairo-svg.h>
#include <fcntl.h>
#include <fstream>
#include <gflags/gflags.h>
#include <google/protobuf/io/zero_copy_stream_impl.h>
#include <google/protobuf/text_format.h>
#include <iomanip>
#include <iostream>
#include <librsvg/rsvg.h>
#include <list>
#include <pango/pangocairo.h>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <sstream>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "calendar.h"
#include "config.pb.h"

#define SECS_PER_DAY (60 * 60 * 24)

DEFINE_string(bible_reading_plans_path,
    "/usr/local/etc/bible-reading-calendar/bible-reading-plans/",
    "A path to bible reading plans directory.");

namespace {

const int days_per_months[] = {
  31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

template <typename Out>
  void split(const std::string &s, char delim, Out result) {
    std::istringstream iss(s);
    std::string item;
    while (std::getline(iss, item, delim)) {
      *result++ = item;
    }
  }

std::vector<std::string> split(const std::string &s, char delim) {
  std::vector<std::string> elems;
  split(s, delim, std::back_inserter(elems));
  return elems;
}

std::string join(const std::vector<std::string>& v,
    const std::string& delimiter)
{
  std::string ret;
  bool first = true;
  for (const auto& s : v) {
    if (first) {
      first = false;
    } else {
      ret += delimiter;
    }
    ret += s;
  }
  return ret;
}

struct tm* get_next_day(time_t *t) {
  *t += SECS_PER_DAY;
  return localtime(t);
}

time_t get_date_in_sec(int year, int month, int day) {
  struct tm timeinfo = {0};

  timeinfo.tm_year = year - 1900;
  timeinfo.tm_mon = month - 1;
  timeinfo.tm_mday = day;

  return mktime(&timeinfo);
}

time_t get_first_day_of_month_in_sec(int year, int month) {
  return get_date_in_sec(year, month, 1);
}

PangoLayout* init_pango_layout(cairo_t *cr,
    const std::string& font_family, double font_size) {
  PangoLayout *layout = pango_cairo_create_layout(cr);
  PangoFontDescription *desc =
    pango_font_description_from_string(font_family.c_str());
  pango_font_description_set_absolute_size(desc, font_size * PANGO_SCALE);
  pango_layout_set_font_description(layout, desc);
  pango_font_description_free(desc);
  return layout;
}

int get_wday_index(struct tm const &timeinfo) {
  return timeinfo.tm_wday;
}

int count_weeks(int year, int month)
{
  time_t t = get_first_day_of_month_in_sec(year, month);
  struct tm timeinfo = *localtime(&t);
  int last_day_1st_week = 7 - get_wday_index(timeinfo);
  int days_after_1st_week = days_per_months[month - 1] - last_day_1st_week;
  int weeks = days_after_1st_week / 7 + 1;
  if (days_after_1st_week % 7 > 0) {
    weeks += 1;
  }
  return weeks;
}

} // namespace

class ReadingUnit {
  public:
    ReadingUnit() {}

    std::string Print(config::Language language, bool use_full_name) const {
      std::string text = getBookName(from_book, language, use_full_name);
      text += " " + from_chapter;

      if (!from_verse.empty()) {
        text += ":" + from_verse;
      }

      if (!to_book.empty()) {
        text += "-";
        if (from_book != to_book) {
          text +=
            getBookName(to_book, language, use_full_name) + " ";
        }

        if (!to_chapter.empty() &&
            (from_book != to_book ||
             from_chapter != to_chapter)) {
          text += to_chapter;
        }

        if (!to_verse.empty()) {
          text += ":" + to_verse;
        }
      }
      return text;
    }

    std::string from_book;
    std::string from_chapter;
    std::string from_verse;

    std::string to_book;
    std::string to_chapter;
    std::string to_verse;

  private:
    static std::string getBookName(
        const std::string& book_id, config::Language language, bool use_full_name)
    {
      const auto& book = books_[language][book_id];
      if (use_full_name) {
        return book.full_name;
      } else {
        return book.short_name;
      }
    }

    static std::map<config::Language, std::map<std::string, Book>> books_;
};

std::map<config::Language, std::map<std::string, Book>> ReadingUnit::books_ = {
  { config::Language::KOREAN,
    {
      {"Genesis", {"창", "창세기"}},
      {"Exodus", {"출", "출애굽기"}},
      {"Leviticus", {"레", "레위기"}},
      {"Numbers", {"민", "민수기"}},
      {"Deuteronomy", {"신", "신명기"}},
      {"Joshua", {"수", "여호수아"}},
      {"Judges", {"삿", "사사기"}},
      {"Ruth", {"룻", "룻기"}},
      {"1 Samuel", {"삼상", "사무엘상"}},
      {"2 Samuel", {"삼하", "사무엘하"}},
      {"1 Kings", {"왕상", "열왕기상"}},
      {"2 Kings", {"왕하", "열왕기하"}},
      {"1 Chronicles", {"대상", "역대상"}},
      {"2 Chronicles", {"대하", "역대하"}},
      {"Ezra", {"스", "에스라"}},
      {"Nehemiah", {"느", "느헤미야"}},
      {"Esther", {"에", "에스더"}},
      {"Job", {"욥", "욥기"}},
      {"Psalms", {"시", "시편"}},
      {"Proverbs", {"잠", "잠언"}},
      {"Ecclesiastes", {"전", "전도서"}},
      {"Song of Solomon", {"아", "아가"}},
      {"Isaiah", {"사", "이사야"}},
      {"Jeremiah", {"렘", "예레미야"}},
      {"Lamentations", {"애", "예레미야애가"}},
      {"Ezekiel", {"겔", "에스겔"}},
      {"Daniel", {"단", "다니엘"}},
      {"Hosea", {"호", "호세아"}},
      {"Joel", {"욜", "요엘"}},
      {"Amos", {"암", "아모스"}},
      {"Obadiah", {"옵", "오바댜"}},
      {"Jonah", {"욘", "요나"}},
      {"Micah", {"미", "미가"}},
      {"Nahum", {"나", "나훔"}},
      {"Habakkuk", {"합", "하박국"}},
      {"Zephaniah", {"습", "스바냐"}},
      {"Haggai", {"학", "학개"}},
      {"Zechariah", {"슥", "스가랴"}},
      {"Malachi", {"말", "말라기"}},
      {"Matthew", {"마", "마태복음"}},
      {"Mark", {"막", "마가복음"}},
      {"Luke", {"눅", "누가복음"}},
      {"John", {"요", "요한복음"}},
      {"Acts", {"행", "사도행전"}},
      {"Romans", {"롬", "로마서"}},
      {"1 Corinthians", {"고전", "고린도전서"}},
      {"2 Corinthians", {"고후", "고린도후서"}},
      {"Galatians", {"갈", "갈라디아서"}},
      {"Ephesians", {"엡", "에베소서"}},
      {"Philippians", {"빌", "빌립보서"}},
      {"Colossians", {"골", "골로새서"}},
      {"1 Thessalonians", {"살전", "데살로니가전서"}},
      {"2 Thessalonians", {"살후", "데살로니가후서"}},
      {"1 Timothy", {"딤전", "디모데전서"}},
      {"2 Timothy", {"딤후", "디모데후서"}},
      {"Titus", {"디", "디도서"}},
      {"Philemon", {"몬", "빌레몬서"}},
      {"Hebrews", {"히", "히브리서"}},
      {"James", {"약", "야고보서"}},
      {"1 Peter", {"벧전", "베드로전서"}},
      {"2 Peter", {"벧후", "베드로후서"}},
      {"1 John", {"요일", "요한일서"}},
      {"2 John", {"요이", "요한이서"}},
      {"3 John", {"요삼", "요한삼서"}},
      {"Jude", {"유", "유다서"}},
      {"Revelation", {"계", "요한계시록"}}
    }},
  { config::Language::ENGLISH,
    {
      {"Genesis", {"Gen", "Genesis"}},
      {"Exodus", {"Ex", "Exodus"}},
      {"Leviticus", {"Lev", "Leviticus"}},
      {"Numbers", {"Num", "Numbers"}},
      {"Deuteronomy", {"Deut", "Deuteronomy"}},
      {"Joshua", {"Josh", "Joshua"}},
      {"Judges", {"Judg", "Judges"}},
      {"Ruth", {"Ruth", "Ruth"}},
      {"1 Samuel", {"1 Sam", "1 Samuel"}},
      {"2 Samuel", {"2 Sam", "2 Samuel"}},
      {"1 Kings", {"1 Ki", "1 Kings"}},
      {"2 Kings", {"2 Ki", "2 Kings"}},
      {"1 Chronicles", {"1 Chr", "1 Chronicles"}},
      {"2 Chronicles", {"2 Chr", "2 Chronicles"}},
      {"Ezra", {"Ezra", "Ezra"}},
      {"Nehemiah", {"Neh", "Nehemiah"}},
      {"Esther", {"Est", "Esther"}},
      {"Job", {"Job", "Job"}},
      {"Psalms", {"Ps", "Psalms"}},
      {"Proverbs", {"Prov", "Proverbs"}},
      {"Ecclesiastes", {"Eccles", "Ecclesiastes"}},
      {"Song of Solomon", {"Song", "Song of Solomon"}},
      {"Isaiah", {"Isa", "Isaiah"}},
      {"Jeremiah", {"Jer", "Jeremiah"}},
      {"Lamentations", {"Lam", "Lamentations"}},
      {"Ezekiel", {"Ezek", "Ezekiel"}},
      {"Daniel", {"Dan", "Daniel"}},
      {"Hosea", {"Hosea", "Hosea"}},
      {"Joel", {"Joel", "Joel"}},
      {"Amos", {"Amos", "Amos"}},
      {"Obadiah", {"Obad", "Obadiah"}},
      {"Jonah", {"Jonah", "Jonah"}},
      {"Micah", {"Micah", "Micah"}},
      {"Nahum", {"Nahum", "Nahum"}},
      {"Habakkuk", {"Hab", "Habakkuk"}},
      {"Zephaniah", {"Zeph", "Zephaniah"}},
      {"Haggai", {"Hag", "Haggai"}},
      {"Zechariah", {"Zech", "Zechariah"}},
      {"Malachi", {"Mal", "Malachi"}},
      {"Matthew", {"Matt", "Matthew"}},
      {"Mark", {"Mark", "Mark"}},
      {"Luke", {"Lu", "Luke"}},
      {"John", {"John", "John"}},
      {"Acts", {"Acts", "Acts"}},
      {"Romans", {"Rom", "Romans"}},
      {"1 Corinthians", {"1 Cor", "1 Corinthians"}},
      {"2 Corinthians", {"2 Cor", "2 Corinthians"}},
      {"Galatians", {"Gal", "Galatians"}},
      {"Ephesians", {"Eph", "Ephesians"}},
      {"Philippians", {"Phil", "Philippians"}},
      {"Colossians", {"Col", "Colossians"}},
      {"1 Thessalonians", {"1 Thess", "1 Thessalonians"}},
      {"2 Thessalonians", {"2 Thess", "2 Thessalonians"}},
      {"1 Timothy", {"1 Tim", "1 Timothy"}},
      {"2 Timothy", {"2 Tim", "2 Timothy"}},
      {"Titus", {"Titus", "Titus"}},
      {"Philemon", {"Philem", "Philemon"}},
      {"Hebrews", {"Heb", "Hebrews"}},
      {"James", {"James", "James"}},
      {"1 Peter", {"1 Peter", "1 Peter"}},
      {"2 Peter", {"2 Peter", "2 Peter"}},
      {"1 John", {"1 John", "1 John"}},
      {"2 John", {"2 John", "2 John"}},
      {"3 John", {"3 John", "3 John"}},
      {"Jude", {"Jude", "Jude"}},
      {"Revelation", {"Rev", "Revelation"}}
    }},
};

class DailyReading {
  public:
    DailyReading() {}

    void PushBack(ReadingUnit reading_unit) {
      reading_units_.push_back(std::move(reading_unit));
    }

    std::string Print(config::Language language) {
      std::vector<std::string> tokens;
      for (const auto& reading_unit : reading_units_) {
        tokens.push_back(reading_unit.Print(language, true));
      }
      return join(tokens, "\\n");
    }

    std::string PrintShort(config::Language language) {
      std::vector<std::string> tokens;
      for (const auto& reading_unit : reading_units_) {
        tokens.push_back(reading_unit.Print(language, false));
      }
      return join(tokens, "\n");
    }

    std::string PrintSingleLine(config::Language language) {
      std::vector<std::string> tokens;
      for (const auto& reading_unit : reading_units_) {
        tokens.push_back(reading_unit.Print(language, false));
      }
      return join(tokens, ", ");
    }

  private:
    std::list<ReadingUnit> reading_units_;
};

class ReadingPlan {
  public:
    ReadingPlan() {
    }

    void PushBack(DailyReading daily_reading) {
      daily_readings_.push_back(std::move(daily_reading));
    }

    DailyReading PopFront() {
      DailyReading front = daily_readings_.front();
      daily_readings_.pop_front();
      return front;
    }

    bool empty() { return daily_readings_.empty(); }

  private:
    std::list<DailyReading> daily_readings_;
};

std::shared_ptr<spdlog::logger> Calendar::logger_ =
  spdlog::stdout_color_mt("calendar");

Calendar::Calendar(config::CalendarConfig conf) :
  conf_(std::move(conf))
{
  switch (conf_.paper_type()) {
    case config::PaperType::US_LETTER:
      surface_width_ = 1100;
      surface_height_ = 850;
      break;
    case config::PaperType::A4:
      surface_width_ = 1175;
      surface_height_ = 825;
      break;
  }
  conf_.set_cell_width(((double)
        surface_width_ - conf_.cell_margin() * 2) / 7);
}

bool Calendar::shouldInclude(const struct tm& tm)
{
  for (const auto& day_to_rest : conf_.days_to_rest()) {
    if (tm.tm_wday == day_to_rest) return false;
  }
  return true;
}

int Calendar::countDays(int year_index)
{
  int start_year = conf_.start_year() + year_index;

  time_t t = get_date_in_sec(
      start_year, conf_.start_month(), conf_.start_day());
  struct tm timeinfo = *localtime(&t);
  int counter = 0;
  while (timeinfo.tm_year + 1900 == start_year ||
      timeinfo.tm_mon + 1 < conf_.start_month() ||
      timeinfo.tm_mday < conf_.start_day()) {
    if (shouldInclude(timeinfo)) {
      ++counter;
    }
    timeinfo = *get_next_day(&t);
  }
  return counter;
}

std::string Calendar::getPlanFileName(int year_index)
{
  std::vector<std::string> file_name_tokens;
  switch (conf_.coverage_type()) {
    case config::CoverageType::NEW_TESTAMENT:
      file_name_tokens.push_back("new-testament");
      break;
    case config::CoverageType::OLD_TESTAMENT:
      file_name_tokens.push_back("old-testament");
      break;
    case config::CoverageType::NEW_TESTAMENT_AND_PSALMS:
      file_name_tokens.push_back("new-testament-and-psalms");
      break;
    case config::CoverageType::WHOLE_BIBLE:
      file_name_tokens.push_back("whole-bible");
      break;
    case config::CoverageType::WHOLE_BIBLE_NEW_TESTAMENT_FIRST:
      file_name_tokens.push_back("whole-bible-new-testament-first");
      break;
    case config::CoverageType::WHOLE_BIBLE_IN_PARALLEL:
      file_name_tokens.push_back("whole-bible-in-parallel");
      break;
    default:
      logger_->error("Unknown CoverageType: {}", conf_.coverage_type());
  }

  switch (conf_.duration_type()) {
    case config::DurationType::ONE_YEAR:
      file_name_tokens.push_back("1-year");
      break;
    case config::DurationType::TWO_YEARS:
      if (year_index == 0) {
        file_name_tokens.push_back("2-years-1st");
      } else {
        file_name_tokens.push_back("2-years-2nd");
      }
      break;
  };
  file_name_tokens.push_back(std::to_string(countDays(year_index)));
  return FLAGS_bible_reading_plans_path +
    join(file_name_tokens, "_") + ".csv";
}

ReadingPlan Calendar::getBibleReadingPlan()
{
  ReadingPlan bible_reading_plan;
  readPlanFile(getPlanFileName(0), &bible_reading_plan);
  if (conf_.duration_type() == config::DurationType::TWO_YEARS) {
    readPlanFile(getPlanFileName(1), &bible_reading_plan);
  }
  return bible_reading_plan;
}

void Calendar::readPlanFile(const std::string file_name,
    ReadingPlan* bible_reading_plan)
{
  logger_->debug(file_name);

  std::ifstream infile(file_name);
  if (!infile) {
    logger_->error("Cannot open [{}]!", file_name);
    return;
  }
  std::string line;
  while (std::getline(infile, line)) {
    auto tokens = split(line, ',');
    DailyReading daily_reading;
    for (int i = 1; i < tokens.size(); i += 6) {
      ReadingUnit reading_unit;

      reading_unit.from_book = tokens[i];
      reading_unit.from_chapter = tokens[i + 1];
      reading_unit.from_verse = tokens[i + 2];

      reading_unit.to_book = tokens[i + 3];
      if (i + 4 < tokens.size()) {
        reading_unit.to_chapter = tokens[i + 4];
      }
      if (i + 5 < tokens.size()) {
        reading_unit.to_verse = tokens[i + 5];
      }

      daily_reading.PushBack(reading_unit);
    }
    bible_reading_plan->PushBack(daily_reading);
  }
}

double Calendar::getDayX(int x_index)
{
  return x_index * conf_.cell_width() + conf_.cell_margin();
}

double Calendar::getDayY(int y_index)
{
  return y_index * conf_.cell_height() + y_offset_;
}

void Calendar::drawMonthLabel(int month) {
  static std::vector<std::string> month_text = {
    "January", "February", "March", "April", "May", "June", "July",
    "August", "September", "October", "November", "December"};

  PangoLayout *layout =
    init_pango_layout(cr_, conf_.has_month_label_font_family() ?
        conf_.month_label_font_family() :
        conf_.default_font_family(),
        conf_.month_label_font_size());
  if (conf_.language() == config::Language::ENGLISH) {
    if (conf_.month_label_uppercase()) {
      auto& str = month_text[month - 1];
      std::transform(str.begin(), str.end(), str.begin(), ::toupper);
    }
    pango_layout_set_text(layout, month_text[month - 1].c_str(), -1);
  } else {
    char buf[3];
    sprintf(buf, "%d", month);
    pango_layout_set_text(layout, buf, -1);
  }

  int width, height;
  pango_layout_get_size(layout, &width, &height);
  cairo_move_to(cr_,
      (surface_width_ - ((double) width / PANGO_SCALE)) / 2,
      conf_.margin_top());
  y_offset_ += (double) height / PANGO_SCALE +
    conf_.margin_top() + conf_.cell_margin();
  logger_->debug("y_offset_: {}" , y_offset_);
  pango_cairo_show_layout(cr_, layout);

  g_object_unref(layout);
}

void Calendar::drawWdayLabel() {
  const char *wday_text[] = {"Sunday", "Monday", "Tuesday",
    "Wednesday", "Thursday", "Friday", "Saturday"};

  const char *wday_text_ko[] = {
    "일", "월", "화", "수", "목", "금", "토"};

  double max_height = 0;
  for (int i = 0; i < 7; ++i) {
    PangoLayout *layout =
      init_pango_layout(cr_, conf_.has_wday_label_font_family() ?
          conf_.wday_label_font_family() :
          conf_.default_font_family(),
          conf_.wday_label_font_size());
    pango_layout_set_text(layout,
        conf_.language() == config::Language::ENGLISH ?
        wday_text[i] : wday_text_ko[i],
        -1);

    int width, height;
    pango_layout_get_size(layout, &width, &height);

    cairo_move_to(cr_, conf_.cell_margin() + i * conf_.cell_width() +
        (conf_.cell_width() - ((double) width / PANGO_SCALE)) /
        2,
        y_offset_ + conf_.cell_margin());
    pango_cairo_show_layout(cr_, layout);

    g_object_unref(layout);

    max_height = std::max(max_height,
        (double) height / PANGO_SCALE);
  }
  y_offset_ += max_height + conf_.cell_margin() * 2;
}

void Calendar::skipMonth(int year, int month,
    ReadingPlan* bible_reading_plan)
{
  time_t t;
  if (year == conf_.start_year() && month == conf_.start_month()) {
    t = get_date_in_sec(year, month, conf_.start_day());
  } else {
    t = get_first_day_of_month_in_sec(year, month);
  }
  struct tm timeinfo = *localtime(&t);

  while (timeinfo.tm_mon == month - 1) {
    if (shouldInclude(timeinfo)) {
      bible_reading_plan->PopFront();
    }
    timeinfo = *get_next_day(&t);
  }
}

void Calendar::drawDaysOfMonth(int year, int month,
    ReadingPlan* bible_reading_plan)
{
  time_t t = get_first_day_of_month_in_sec(year, month);
  struct tm timeinfo = *localtime(&t);

  int x = get_wday_index(timeinfo);
  int y = 0;
  while (timeinfo.tm_mon == month - 1) {
    // Label
    char buf[4];
    sprintf(buf, "%d", timeinfo.tm_mday);
    drawTextOfDayNumber(x, y, buf);

    if (shouldInclude(timeinfo) &&
        !bible_reading_plan->empty() &&
        (year > conf_.start_year() || month > conf_.start_month() ||
         timeinfo.tm_mday >= conf_.start_day())) {
      drawTextOfDayPlan(x, y,
          bible_reading_plan->PopFront().PrintShort(conf_.language()));
    }

    timeinfo = *get_next_day(&t);
    x++;
    if (x >= 7) {
      x = 0;
      y++;
    }
  }
}

void Calendar::drawTextOfDayNumber(int x, int y, const char* text)
{
  PangoLayout *layout = init_pango_layout(cr_,
      conf_.has_day_number_font_family() ?
      conf_.day_number_font_family() :
      conf_.default_font_family(),
      conf_.day_number_font_size());
  pango_layout_set_text(layout, text, -1);

  cairo_move_to(cr_,
      getDayX(x) + conf_.cell_margin(),
      getDayY(y) + conf_.cell_margin());
  pango_cairo_show_layout(cr_, layout);

  g_object_unref(layout);
}

void Calendar::drawTextOfDayPlan(int x, int y,
    const std::string text)
{
  PangoLayout *layout =
    init_pango_layout(cr_, conf_.has_day_plan_font_family() ?
        conf_.day_plan_font_family() : conf_.default_font_family(),
        conf_.day_plan_font_size());
  pango_layout_set_alignment(layout, PANGO_ALIGN_RIGHT);
  pango_layout_set_text(layout, text.c_str(), -1);

  int width, height;
  pango_layout_get_size(layout, &width, &height);
  cairo_move_to(cr_,
      getDayX(x + 1) -
      ((double) width / PANGO_SCALE) - conf_.cell_margin(),
      getDayY(y + 1) -
      ((double) height / PANGO_SCALE) - conf_.cell_margin());
  pango_cairo_show_layout(cr_, layout);

  g_object_unref(layout);
}

void Calendar::drawMonth(int year, int month,
    ReadingPlan* bible_reading_plan)
{
  std::string output_file_name = conf_.output_file_name() + '_' +
    std::to_string(year) + '_' + std::to_string(month);

  cairo_surface_t *surface = NULL;
  switch (conf_.output_type()) {
    case config::OutputType::PDF:
      surface = cairo_pdf_surface_create(
          (output_file_name + ".pdf").c_str(),
          surface_width_, surface_height_);
      break;
    case config::OutputType::PNG:
      surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32,
          surface_width_, surface_height_);
      break;
    default:
      surface = cairo_svg_surface_create(
          (output_file_name + ".svg").c_str(),
          surface_width_, surface_height_);
      break;
  }

  drawMonthOnSurface(year, month, bible_reading_plan, surface);

  if (conf_.output_type() == config::OutputType::PNG) {
    cairo_surface_write_to_png(surface,
        (output_file_name + ".png").c_str());
  }

  cairo_surface_destroy(surface);
}

void Calendar::drawMonthOnSurface(int year, int month,
    ReadingPlan* bible_reading_plan, cairo_surface_t* surface)
{
  y_offset_ = 0;
  cr_ = cairo_create(surface);

  // Paint white background.
  cairo_save(cr_);
  cairo_set_source_rgb(cr_, 1, 1, 1);
  cairo_paint(cr_);
  cairo_restore(cr_);

  drawMonthLabel(month);

  // Draw frame
  cairo_set_line_width(cr_, conf_.line_width());

  cairo_rectangle(cr_, conf_.cell_margin(), y_offset_,
      surface_width_ - conf_.cell_margin() * 2,
      surface_height_ - y_offset_ - conf_.cell_margin());

  for (int x = 1; x < 7; ++x) {
    cairo_move_to(cr_, getDayX(x), y_offset_);
    cairo_line_to(cr_, getDayX(x), surface_height_ - conf_.cell_margin());
  }

  drawWdayLabel();

  // Stroke a line below the labels
  cairo_move_to(cr_, conf_.cell_margin(), y_offset_);
  cairo_line_to(cr_, surface_width_ - conf_.cell_margin(), y_offset_);

  // Horizontal lines below dates
  conf_.set_cell_height(
      ((double) surface_height_ - conf_.cell_margin() - y_offset_) /
      count_weeks(year, month));

  for (int y = 1; y < count_weeks(year, month); ++y) {
    cairo_move_to(cr_, conf_.cell_margin(),
        y_offset_ + y * conf_.cell_height());
    cairo_line_to(cr_, surface_width_ - conf_.cell_margin(),
        y_offset_ + y * conf_.cell_height());
  }

  cairo_stroke(cr_);

  drawDaysOfMonth(year, month, bible_reading_plan);

  cairo_destroy(cr_);
}

void Calendar::initMonthIteration(int* y, int* m)
{
  *y = conf_.start_year();
  *m = conf_.start_month();
}

bool Calendar::isReadingMonth(int y, int m)
{
  int last_year = conf_.start_year() + 1;
  if (conf_.duration_type() == config::DurationType::TWO_YEARS) {
    last_year++;
  }

  if (y < last_year) {
    return true;
  } else if (y == last_year) {
    if (m < conf_.start_month()) {
      return true;
    } else if (m == conf_.start_month() && conf_.start_day() > 1) {
      return true;
    }
    return false;
  }
  return false;
}

bool Calendar::isSelectedMonth(int y, int m)
{
  return y == conf_.year() && m == conf_.month();
}

void Calendar::nextMonth(int* y, int* m)
{
  *m += 1;
  if (*m > 12) {
    *m = 1;
    *y += 1;
  }
}

void Calendar::streamMonthOnSurface(cairo_surface_t* surface) {
  ReadingPlan bible_reading_plan = getBibleReadingPlan();

  int y, m;
  initMonthIteration(&y, &m);
  while (!isSelectedMonth(y, m)) {
    skipMonth(y, m, &bible_reading_plan);
    nextMonth(&y, &m);
  }
  drawMonthOnSurface(y, m, &bible_reading_plan, surface);
}

void Calendar::draw()
{
  ReadingPlan bible_reading_plan = getBibleReadingPlan();

  int y, m;
  initMonthIteration(&y, &m);
  if (conf_.has_month()) {
    while (!isSelectedMonth(y, m)) {
      skipMonth(y, m, &bible_reading_plan);
      nextMonth(&y, &m);
    }
    drawMonth(y, m, &bible_reading_plan);
  } else {
    while (isReadingMonth(y, m)) {
      drawMonth(y, m, &bible_reading_plan);
      nextMonth(&y, &m);
    }
  }
}

void Calendar::streamSvg(cairo_write_func_t writeFunc, void *closure)
{
  cairo_surface_t* surface =
    cairo_svg_surface_create_for_stream(writeFunc, closure,
        surface_width_, surface_height_);

  streamMonthOnSurface(surface);

  cairo_surface_destroy(surface);
}

void Calendar::streamPng(cairo_write_func_t writeFunc, void *closure)
{
  cairo_surface_t* surface = cairo_image_surface_create(
      CAIRO_FORMAT_ARGB32, surface_width_, surface_height_);

  streamMonthOnSurface(surface);

  cairo_surface_write_to_png_stream(surface, writeFunc, closure);
  cairo_surface_destroy(surface);
}

void Calendar::streamPdf(cairo_write_func_t writeFunc, void *closure)
{
  ReadingPlan bible_reading_plan = getBibleReadingPlan();

  cairo_surface_t* surface =
    cairo_pdf_surface_create_for_stream(writeFunc, closure,
        surface_width_, surface_height_);

  int y, m;
  initMonthIteration(&y, &m);
  while (isReadingMonth(y, m)) {
    drawMonthOnSurface(y, m, &bible_reading_plan, surface);
    cairo_surface_show_page(surface);
    nextMonth(&y, &m);
  }
  cairo_surface_destroy(surface);
}


int Calendar::iCalendar(std::ostream* ostream)
{
  *ostream << "BEGIN:VCALENDAR" << std::endl;
  *ostream << "VERSION:2.0" << std::endl;

  *ostream << "PRODID:-//Bible Reading Calendar//biblereadingcalendar.com//";
  switch (conf_.language()) {
    case config::Language::ENGLISH:
      *ostream << "EN";
      break;
    case config::Language::KOREAN:
      *ostream << "KO";
      break;
  }
  *ostream << std::endl;

  *ostream << "CALSCALE:GREGORIAN" << std::endl;
  *ostream << "METHOD:PUBLISH" << std::endl;

  *ostream << "X-WR-CALNAME:";
  switch (conf_.language()) {
    case config::Language::ENGLISH:
      *ostream << "Bible Reading Calendar";
      break;
    case config::Language::KOREAN:
      *ostream << "성경 읽기 달력";
      break;
  }
  *ostream << std::endl;

  *ostream << "X-WR-TIMEZONE:Etc/GMT" << std::endl;

  ReadingPlan bible_reading_plan = getBibleReadingPlan();

  time_t t = get_date_in_sec(
      conf_.start_year(), conf_.start_month(), conf_.start_day());
  struct tm timeinfo = *localtime(&t);

  while (!bible_reading_plan.empty()) {
    if (shouldInclude(timeinfo)) {
      *ostream << "BEGIN:VEVENT" << std::endl;

      auto daily_reading = bible_reading_plan.PopFront();

      *ostream << "SUMMARY:" <<
        daily_reading.PrintSingleLine(conf_.language()) <<
        std::endl;

      *ostream << "DESCRIPTION:" <<
        daily_reading.Print(conf_.language()) << std::endl;

      char yyyymmdd[10];
      strftime(yyyymmdd, 10, "%Y%m%d", &timeinfo);
      *ostream << "DTSTART:" << yyyymmdd << std::endl;
      *ostream << "DTEND:" << yyyymmdd << std::endl;
      *ostream << "UID:" << yyyymmdd <<
        "@biblereadingcalendar.com" << std::endl;
    }

    *ostream << "END:VEVENT" << std::endl;
    timeinfo = *get_next_day(&t);
  }

  *ostream << "END:VCALENDAR" << std::endl;
  return 200;
}
