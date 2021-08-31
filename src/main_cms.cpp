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

config::DurationType getDurationType(
    const std::string& d, const std::string& yi)
{
  if (d == "one-year") return config::DurationType::ONE_YEAR;

  if (yi == "0") return config::DurationType::TWO_YEARS_FIRST_YEAR;

  return config::DurationType::TWO_YEARS_SECOND_YEAR;
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
    conf.set_duration_type(getDurationType(
          request().get("d"), request().get("yi")));
    if (hasRestDay(request().get("r"))) {
      conf.add_days_to_rest(getDayOfTheWeekType(request().get("r")));
    }
  } else if (c == "whole-bible") {
    conf.set_duration_type(getDurationType(
          request().get("d"), request().get("yi")));
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
  conf.set_year(2022);
  // TODO: ignore 'i' parameter when rendering PDF
  conf.set_month(stoi(request().get("i")) + 1);

  /*
  conf.set_language(config::Language::ENGLISH);
  conf.set_paper_type(config::PaperType::US_LETTER);

  conf.set_default_font_family("Ubuntu");

  conf.set_month_label_font_family("Merriweather");
  conf.set_month_label_font_size(80);

  conf.set_wday_label_font_size(20);
  conf.set_day_number_font_size(25);

  conf.set_day_plan_font_family("BarlowCondensed");
  conf.set_day_plan_font_size(23);
  */

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

  return conf;
}

void CalendarApp::svg()
{
  response().set_header("Content-Type", "image/svg+xml");
  response().cache_control("public, max-age=3600");

  Calendar calendar(buildConfig());
  calendar.streamSvg(CalendarApp::cairoWriteFunc, this);
}

void CalendarApp::pdf()
{
  response().cache_control("public, max-age=0");
  response().set_header("Content-Type", "application/pdf");

  Calendar calendar(buildConfig());
  calendar.streamPdf(CalendarApp::cairoWriteFunc, this);
}

void CalendarApp::png()
{
  response().cache_control("public, max-age=3600");
  response().set_header("Content-Type", "image/png");

  Calendar calendar(buildConfig());
  calendar.streamPng(CalendarApp::cairoWriteFunc, this);
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
