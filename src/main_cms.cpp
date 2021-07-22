#include <cppcms/application.h>
#include <cppcms/applications_pool.h>
#include <cppcms/http_response.h>
#include <cppcms/http_request.h>
#include <cppcms/service.h>
#include <cppcms/url_dispatcher.h>
#include <gflags/gflags.h>
#include <iostream>
#include <fstream>

#include "calendar.h"
#include "config.pb.h"

auto logger = spdlog::stdout_color_mt("main");

class CalendarApp : public cppcms::application {
  public:
    CalendarApp(cppcms::service &srv) : cppcms::application(srv) {
      dispatcher().assign("/img.svg", &CalendarApp::img, this);
      dispatcher().assign(".*", &CalendarApp::redirect, this);
    }

    void redirect()
    {
      response().set_redirect_header("/index.html");
    }

    void img();

    static cairo_status_t cairoWriteFunc(
        void* closure, const unsigned char* data, unsigned int length) {
      ((CalendarApp*)closure)->response().out().write(
        (const char*)data, length);
      return CAIRO_STATUS_SUCCESS;
    }
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

  // TODO: Make another parameter to get the second year.
  return config::DurationType::TWO_YEARS_FIRST_YEAR;
}

void CalendarApp::img()
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
  conf.set_language(config::Language::ENGLISH);
  conf.set_paper_type(config::PaperType::US_LETTER);
  conf.set_year(2022);
  conf.set_month(7);
  conf.set_default_font_family("Glass Antiqua Bold");
  conf.set_month_label_font_size(100);

  response().set_header("Content-Type", "image/svg+xml");

  Calendar calendar(std::move(conf));
  calendar.streamSvg(CalendarApp::cairoWriteFunc, this);
}

int main(int argc,char ** argv)
{
  gflags::ParseCommandLineFlags(&argc, &argv, false);

  try {
    cppcms::service srv(argc,argv);
    srv.applications_pool().mount(cppcms::applications_factory<CalendarApp>());
    srv.run();
  }
  catch(std::exception const &e) {
    std::cerr << e.what() << std::endl;
  }
}
