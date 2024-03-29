#include <cppcms/application.h>
#include <cppcms/applications_pool.h>
#include <cppcms/http_response.h>
#include <cppcms/http_request.h>
#include <cppcms/service.h>
#include <cppcms/url_dispatcher.h>
#include <fstream>
#include <gflags/gflags.h>
#include <iostream>
#include <time.h>

#include "calendar.h"
#include "config.pb.h"

auto logger = spdlog::stdout_color_mt("main");

class CalendarApp : public cppcms::application {
  public:
    CalendarApp(cppcms::service &srv) : cppcms::application(srv) {
      dispatcher().assign("/img.svg", &CalendarApp::svg, this);
      dispatcher().assign("/img.pdf", &CalendarApp::pdf, this);
      dispatcher().assign("/img.png", &CalendarApp::png, this);
      dispatcher().assign("/c.ics", &CalendarApp::ics, this);
      dispatcher().assign(".*", &CalendarApp::redirect, this);
    }

    void redirect()
    {
      const auto language = request().http_accept_language();
      logger->debug(language);
      if (language.rfind("ko", 0) == 0) {
        response().set_redirect_header("/ko/");
      } else {
        response().set_redirect_header("/en-US/");
      }
    }

    void svg();
    void pdf();
    void png();
    void ics();

    static cairo_status_t cairoWriteFunc(
        void* closure, const unsigned char* data, unsigned int length) {
      auto& ostream = ((CalendarApp*)closure)->response().out();
      ostream.write((const char*)data, length);
      ostream.flush();
      return CAIRO_STATUS_SUCCESS;
    }

    static cairo_status_t cairoWriteToMemoryFunc(
        void* closure, const unsigned char* data, unsigned int length) {
      ((CalendarApp*)closure)->writeToCairoBuffer(data, length);
      return CAIRO_STATUS_SUCCESS;
    }

    void writeToCairoBuffer(
        const unsigned char* data, unsigned int length) {
      memcpy(cairoBuffer_ + cairoBufferOffset_, data, length);
      cairoBufferOffset_ += length;
    }

  private:
    config::CalendarConfig buildConfig();
    void initResponse();

    unsigned char* cairoBuffer_;
    unsigned int cairoBufferOffset_;
};

config::DayOfTheWeek getDayOfTheWeekType(const std::string& d)
{
  if (d == "sunday") return config::DayOfTheWeek::SUN;
  if (d == "monday") return config::DayOfTheWeek::MON;
  if (d == "tuesday") return config::DayOfTheWeek::TUES;
  if (d == "wednesday") return config::DayOfTheWeek::WED;
  if (d == "thursday") return config::DayOfTheWeek::THURS;
  if (d == "friday") return config::DayOfTheWeek::FRI;
  return config::DayOfTheWeek::SAT;
}

bool hasRestDay(const std::string& d)
{
  return d != "everyday";
}

config::DurationType getDurationType(const std::string& d)
{
  if (d == "one-year") return config::DurationType::ONE_YEAR;
  return config::DurationType::TWO_YEARS;
}

config::CalendarConfig CalendarApp::buildConfig()
{
  config::CalendarConfig conf;

  const auto& c = request().get("c");
  if (c == "new-testament") {
    conf.set_coverage_type(config::CoverageType::NEW_TESTAMENT);
    conf.add_days_to_rest(getDayOfTheWeekType(request().get("r1")));
    conf.add_days_to_rest(getDayOfTheWeekType(request().get("r2")));
  } else if (c == "old-testament") {
    conf.set_coverage_type(config::CoverageType::OLD_TESTAMENT);
    conf.set_duration_type(getDurationType(request().get("d")));
    if (hasRestDay(request().get("r"))) {
      conf.add_days_to_rest(getDayOfTheWeekType(request().get("r")));
    }
  } else if (c == "whole-bible") {
    conf.set_duration_type(getDurationType(request().get("d")));
    if (hasRestDay(request().get("r"))) {
      conf.add_days_to_rest(getDayOfTheWeekType(request().get("r")));
    }

    const auto& o = request().get("o");
    if (o == "old-testament-first") {
      conf.set_coverage_type(config::CoverageType::WHOLE_BIBLE);
    } else if (o == "new-testament-first") {
      conf.set_coverage_type(
          config::CoverageType::WHOLE_BIBLE_NEW_TESTAMENT_FIRST);
    } else {
      conf.set_coverage_type(config::CoverageType::WHOLE_BIBLE_IN_PARALLEL);
    }
  } else if (c == "new-testament-and-psalms") {
    conf.set_coverage_type(config::CoverageType::NEW_TESTAMENT_AND_PSALMS);

    if (hasRestDay(request().get("r"))) {
      conf.add_days_to_rest(getDayOfTheWeekType(request().get("r")));
    }
  } else {
    response().status(404);
  }
  conf.set_year(stoi(request().get("y")));
  // TODO: ignore 'i' parameter when rendering PDF
  conf.set_month(stoi(request().get("m")));

  // Parse YYYYMMDD
  long s = stol(request().get("s"));
  conf.set_start_day(s % 100);
  s /= 100;
  conf.set_start_month(s % 100);
  conf.set_start_year(s / 100);

  if (request().get("l") == "ko") {
    conf.set_language(config::Language::KOREAN);
    conf.set_paper_type(config::PaperType::A4);

    conf.set_default_font_family("Gothic A1");

    conf.set_margin_top(25);
    conf.set_month_label_font_family("Gothic A1 ExtraBold");
    conf.set_month_label_font_size(100);

    conf.set_wday_label_font_family("Gothic A1 Bold");
    conf.set_wday_label_font_size(18);

    conf.set_day_number_font_family("Mulish Bold");
    conf.set_day_number_font_size(28);

    conf.set_day_plan_font_size(20);
  } else {
    conf.set_language(config::Language::ENGLISH);
    conf.set_paper_type(config::PaperType::US_LETTER);

    conf.set_default_font_family("Roboto");

    conf.set_margin_top(-10);
    conf.set_month_label_font_family("Playfair Display");
    conf.set_month_label_font_size(90);

    conf.set_wday_label_font_family("Roboto Medium");
    conf.set_wday_label_font_size(20);

    conf.set_day_number_font_size(28);

    conf.set_day_plan_font_family("BarlowCondensed");
    conf.set_day_plan_font_size(23);
  }

  return conf;
}

void CalendarApp::initResponse()
{
  response().cache_control("public, max-age=3600");
}

void CalendarApp::svg()
{
  initResponse();
  response().set_header("Content-Type", "image/svg+xml");

  Calendar calendar(buildConfig());
  calendar.streamSvg(CalendarApp::cairoWriteFunc, this);
}

void CalendarApp::pdf()
{
  initResponse();
  response().set_header("Content-Type", "application/pdf");

  Calendar calendar(buildConfig());
  calendar.streamPdf(CalendarApp::cairoWriteFunc, this);
}

void CalendarApp::png()
{
  initResponse();
  response().set_header("Content-Type", "image/png");

  Calendar calendar(buildConfig());
  calendar.streamPng(CalendarApp::cairoWriteFunc, this);
}

void CalendarApp::ics()
{
  initResponse();
  auto& ostream = response().out();
  response().set_header("Content-Type", "text/calendar");

  Calendar calendar(buildConfig());
  int status = calendar.iCalendar(&ostream);
  if (status != 200) {
    response().status(status);
  }
}

int main(int argc,char ** argv)
{
  gflags::ParseCommandLineFlags(&argc, &argv, false);
  // spdlog::set_level(spdlog::level::debug);

  try {
    cppcms::service srv(argc,argv);
    srv.applications_pool().mount(cppcms::applications_factory<CalendarApp>());
    logger->debug("Running cppcms service...");
    srv.run();
  }
  catch(std::exception const &e) {
    logger->error(e.what());
  }
}
