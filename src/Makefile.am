%.pb.cc %.pb.h: %.proto
	$(PROTOC) --proto_path=$(srcdir) --cpp_out=$(builddir) $^

dist_noinst_DATA = config.proto
BUILT_SOURCES = config.pb.cc

noinst_PROGRAMS = cli cms

cli_SOURCES = main_cli.cpp config.pb.cc config.pb.h calendar.h calendar.cpp
cli_CPPFLAGS = $(CAIRO_CFLAGS) $(LIBRSVG2_CFLAGS)
cli_LDADD = $(CAIRO_LIBS) $(PROTOBUF_LIBS) $(LIBRSVG2_LIBS) -lgflags

cms_SOURCES = main_cms.cpp config.pb.cc config.pb.h calendar.h calendar.cpp
cms_CPPFLAGS = $(CAIRO_CFLAGS) $(LIBRSVG2_CFLAGS)
cms_LDADD = $(CAIRO_LIBS) $(PROTOBUF_LIBS) $(LIBRSVG2_LIBS) -lcppcms -lgflags
