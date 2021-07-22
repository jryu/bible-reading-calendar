#include <cairo.h>
#include <fcntl.h>
#include <fstream>
#include <gflags/gflags.h>
#include <google/protobuf/io/zero_copy_stream_impl.h>
#include <google/protobuf/text_format.h>
#include <iomanip>
#include <iostream>
#include <pango/pangocairo.h>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <sstream>
#include <stdio.h>

#include "calendar.h"
#include "config.pb.h"

auto console = spdlog::stdout_color_mt("main");

bool parse_config(config::CalendarConfig *conf) {
  // Verify that the version of the library that we linked
  // against is compatible with the version of the headers we
  // compiled against.
  GOOGLE_PROTOBUF_VERIFY_VERSION;

  int fd = open("config.txt", O_RDONLY);
  if (fd < 0) {
    console->error(strerror(errno));
    return false;
  }
  google::protobuf::io::FileInputStream fileInput(fd);
  fileInput.SetCloseOnDelete(true);

  if (!google::protobuf::TextFormat::Parse(&fileInput, conf)) {
    // protobuf prints error message
    return false;
  }
  return true;
}

void list_fonts()
{
  PangoFontMap * fontmap = pango_cairo_font_map_get_default();

  PangoFontFamily **families;
  int n_families;
  pango_font_map_list_families(fontmap, &families, &n_families);

  for (int i = 0; i < n_families; i++) {
    PangoFontFamily* family = families[i];

    PangoFontFace** faces;
    int n_faces;
    pango_font_family_list_faces(family, &faces, &n_faces);

    std::cout << pango_font_family_get_name(family) <<
      " (" << n_faces << ')' << std::endl;

    for (int j = 0; j < n_faces; ++j) {
      std::cout << "\t" <<
        pango_font_face_get_face_name(faces[j]) << std::endl;
    }
    g_free(faces);
  }
  g_free(families);
}

int main(int argc, char *argv[])
{
  gflags::ParseCommandLineFlags(&argc, &argv, true);

  config::CalendarConfig conf;
  if (!parse_config(&conf)) {
    console->error("Config parsing error");
    return EXIT_FAILURE;
  }
  Calendar calendar(std::move(conf));
  calendar.draw();
  return EXIT_SUCCESS;
}
