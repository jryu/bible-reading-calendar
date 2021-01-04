%.pb.cc %.pb.h: %.proto
	$(PROTOC) --proto_path=$(srcdir) --cpp_out=$(builddir) $^

dist_noinst_DATA = config.proto
BUILT_SOURCES = config.pb.cc

noinst_PROGRAMS = bible_reading_calendar
bible_reading_calendar_SOURCES = main.cpp config.pb.cc config.pb.h
bible_reading_calendar_CPPFLAGS = $(CAIRO_CFLAGS) $(LIBRSVG2_CFLAGS)
bible_reading_calendar_LDADD = $(CAIRO_LIBS) $(PROTOBUF_LIBS) $(LIBRSVG2_LIBS)
