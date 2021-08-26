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

#include "minizip/mz.h"
#include "minizip/mz_os.h"
#include "minizip/mz_strm_mem.h"
#include "minizip/mz_strm.h"
#include "minizip/mz_zip.h"
#include "calendar.h"
#include "config.pb.h"

auto logger = spdlog::stdout_color_mt("main");

class CalendarApp : public cppcms::application {
  public:
    CalendarApp(cppcms::service &srv) : cppcms::application(srv) {
      dispatcher().assign("/img.svg", &CalendarApp::svg, this);
      dispatcher().assign("/img.pdf", &CalendarApp::pdf, this);
      dispatcher().assign("/img.png", &CalendarApp::png, this);
      dispatcher().assign("/img.zip", &CalendarApp::zip, this);
      dispatcher().assign(".*", &CalendarApp::redirect, this);
    }

    void redirect()
    {
      response().set_redirect_header("/index.html");
    }

    void svg();
    void pdf();
    void png();
    void zip();

    mz_zip_file buildFileInfo(const char* filename);
    void streamZipOutput(void* write_mem_stream);

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
    void copyFromCairoBufferToZipEntry(void* zip_handle,mz_zip_file file_info);

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
  logger->info("pdf");
  calendar.streamPdf(CalendarApp::cairoWriteFunc, this);
}

void CalendarApp::png()
{
  response().cache_control("public, max-age=3600");
  response().set_header("Content-Type", "image/png");

  Calendar calendar(buildConfig());
  calendar.streamPng(CalendarApp::cairoWriteFunc, this);
}

void CalendarApp::zip()
{
  response().set_header("Content-Type", "application/zip");
  response().cache_control("public, max-age=3600");

  void *write_mem_stream = NULL;
  mz_stream_mem_create(&write_mem_stream);
  mz_stream_mem_set_grow_size(write_mem_stream, 128 * 1024);
  mz_stream_open(write_mem_stream, NULL, MZ_OPEN_MODE_CREATE);

  void *zip_handle = NULL;
  mz_zip_create(&zip_handle);
  int32_t err = mz_zip_open(zip_handle, write_mem_stream, MZ_OPEN_MODE_WRITE);
  if (err != MZ_OK) {
    response().status(500);
    logger->error("mz_zip_open error: {}", err);
    return;
  }

  config::CalendarConfig conf = buildConfig();
  cairoBuffer_ = (unsigned char*) malloc(200 *1024);
  for (int i = 0; i < 12 ; ++i) {
    conf.set_month(i + 1);
    cairoBufferOffset_ = 0;

    Calendar calendar(conf);
    calendar.streamPdf(CalendarApp::cairoWriteToMemoryFunc, this);

    char filename[100];
    sprintf(filename, "%02d.pdf", conf.month());
    mz_zip_file file_info = buildFileInfo(filename);
    copyFromCairoBufferToZipEntry(zip_handle, file_info);
  }
  free(cairoBuffer_);

  mz_zip_close(zip_handle);
  mz_zip_delete(&zip_handle);

  streamZipOutput(write_mem_stream);

  mz_stream_mem_close(write_mem_stream);
  mz_stream_mem_delete(&write_mem_stream);
  write_mem_stream = NULL;
}

mz_zip_file CalendarApp::buildFileInfo(const char* filename)
{
  mz_zip_file file_info;
  memset(&file_info, 0, sizeof(file_info));

  file_info.version_madeby = MZ_VERSION_MADEBY;
  file_info.compression_method = MZ_COMPRESS_METHOD_DEFLATE;
  time(&file_info.modified_date);

  uint32_t src_attrib = S_IFREG | 0664;
  uint8_t src_sys = MZ_HOST_SYSTEM(file_info.version_madeby);

  if ((src_sys != MZ_HOST_SYSTEM_MSDOS) &&
      (src_sys != MZ_HOST_SYSTEM_WINDOWS_NTFS)) {
    // High bytes are OS specific attributes,low byte is always DOS attributes.
    uint32_t target_attrib = 0;
    if (mz_zip_attrib_convert(
          src_sys, src_attrib, MZ_HOST_SYSTEM_MSDOS, &target_attrib) == MZ_OK) {
      file_info.external_fa = target_attrib;
    }
    file_info.external_fa |= (src_attrib << 16);
  } else {
    file_info.external_fa = src_attrib;
  }

  file_info.filename = filename;
  file_info.uncompressed_size = cairoBufferOffset_;
#ifdef HAVE_WZAES
  file_info.aes_version = MZ_AES_VERSION;
#endif

  return file_info;
}

void CalendarApp::copyFromCairoBufferToZipEntry(
    void* zip_handle, mz_zip_file file_info)
{
  int32_t err = mz_zip_entry_write_open(
      zip_handle, &file_info, MZ_COMPRESS_LEVEL_DEFAULT, 0, NULL);
  if (err != MZ_OK) {
    response().status(500);
    logger->error("mz_zip_entry_write_open error: {}", err);
    return;
  }

  int32_t written_total = 0;
  do {
    int32_t written = mz_zip_entry_write(zip_handle,
        cairoBuffer_ + written_total,
        cairoBufferOffset_ - written_total);

    if (written < MZ_OK) {
      err = written;
      break;
    }
    written_total += written;
  } while (err == MZ_OK && written_total < cairoBufferOffset_);

  mz_zip_entry_close(zip_handle);
}

void CalendarApp::streamZipOutput(void* write_mem_stream)
{
  const char *buffer_ptr = NULL;
  mz_stream_mem_get_buffer(write_mem_stream, (const void **)&buffer_ptr);
  mz_stream_mem_seek(write_mem_stream, 0, MZ_SEEK_END);

  response().out().write(buffer_ptr, mz_stream_mem_tell(write_mem_stream));
}

int main(int argc,char ** argv)
{
  gflags::ParseCommandLineFlags(&argc, &argv, false);

  try {
    cppcms::service srv(argc,argv);
    srv.applications_pool().mount(cppcms::applications_factory<CalendarApp>());
    logger->info("Running cppcms service...");
    srv.run();
  }
  catch(std::exception const &e) {
    logger->error(e.what());
  }
}
