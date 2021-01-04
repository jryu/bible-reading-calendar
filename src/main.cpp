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

#define SECS_PER_DAY (60 * 60 * 24)

config::CalendarConfig conf;
auto console = spdlog::stdout_color_mt("console");

const int days_per_months[] = {
	31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

class Book {
	public:
		std::string short_name;
		std::string full_name;
};

std::map<std::string, Book> books = {
	{"Gen", {"창", "창세기"}},
	{"Ex", {"출", "출애굽기"}},
	{"Lev", {"레", "레위기"}},
	{"Num", {"민", "민수기"}},
	{"Deut", {"신", "신명기"}},
	{"Josh", {"수", "여호수아"}},
	{"Judg", {"삿", "사사기"}},
	{"Ruth", {"룻", "룻기"}},
	{"1 Sam", {"삼상", "사무엘상"}},
	{"2 Sam", {"삼하", "사무엘하"}},
	{"1 Ki", {"왕상", "열왕기상"}},
	{"2 Ki", {"왕하", "열왕기하"}},
	{"1 Chr", {"대상", "역대상"}},
	{"2 Chr", {"대하", "역대하"}},
	{"Ezra", {"스", "에스라"}},
	{"Neh", {"느", "느헤미야"}},
	{"Est", {"에", "에스더"}},
	{"Job", {"욥", "욥기"}},
	{"Prov", {"잠", "잠언"}},
	{"Eccles", {"전", "전도서"}},
	{"Song", {"아", "아가"}},
	{"Isa", {"사", "이사야"}},
	{"Jer", {"렘", "예레미야"}},
	{"Lam", {"애", "예레미야애가"}},
	{"Ezek", {"겔", "에스겔"}},
	{"Dan", {"단", "다니엘"}},
	{"Hosea", {"호", "호세아"}},
	{"Joel", {"욜", "요엘"}},
	{"Amos", {"암", "아모스"}},
	{"Obadiah", {"옵", "오바댜"}},
	{"Jonah", {"욘", "요나"}},
	{"Micah", {"미", "미가"}},
	{"Nahum", {"나", "나훔"}},
	{"Hab", {"합", "하박국"}},
	{"Zeph", {"습", "스바냐"}},
	{"Hag", {"학", "학개"}},
	{"Zech", {"슥", "스가랴"}},
	{"Mal", {"말", "말라기"}},
	{"Matt", {"마", "마태복음"}},
	{"Mark", {"막", "마가복음"}},
	{"Lu", {"눅", "누가복음"}},
	{"John", {"요", "요한복음"}},
	{"Ps", {"시", "시편"}},
	{"Acts", {"행", "사도행전"}},
	{"Rom", {"롬", "로마서"}},
	{"1 Cor", {"고전", "고린도전서"}},
	{"2 Cor", {"고후", "고린도후서"}},
	{"Gal", {"갈", "갈라디아서"}},
	{"Eph", {"엡", "에베소서"}},
	{"Phil", {"빌", "빌립보서"}},
	{"Col", {"골", "골로새서"}},
	{"1 Thess", {"살전", "데살로니가전서"}},
	{"2 Thess", {"살후", "데살로니가후서"}},
	{"1 Tim", {"딤전", "디모데전서"}},
	{"2 Tim", {"딤후", "디모데후서"}},
	{"Titus", {"디", "디도서"}},
	{"Philem", {"몬", "빌레몬서"}},
	{"Heb", {"히", "히브리서"}},
	{"James", {"약", "야고보서"}},
	{"1 Peter", {"벧전", "베드로전서"}},
	{"2 Peter", {"벧후", "베드로후서"}},
	{"1 John", {"요일", "요한일서"}},
	{"2 John", {"요이", "요한이서"}},
	{"3 John", {"요삼", "요한삼서"}},
	{"Jude", {"유", "유다서"}},
	{"Rev", {"계", "요한계시록"}}
};

double y_offset = 0;

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

struct tm* get_next_day(time_t *t) {
	*t += SECS_PER_DAY;
	return localtime(t);
}

time_t get_first_day_of_month_in_sec(int year, int month) {
	struct tm timeinfo = {0};

	timeinfo.tm_year = year - 1900;
	timeinfo.tm_mon = month - 1;
	timeinfo.tm_mday = 1;

	return mktime(&timeinfo);
}

std::string get_book_name(const std::vector<std::string>& tokens, int index) {
	if (conf.language() == config::Language::ENGLISH) {
		return tokens[index];
	}
	return books[tokens[index]].short_name;
}

std::queue<std::string> get_bible_reading_plan_of_month()
{
	std::queue<std::string> bible_reading_plan;

	std::ifstream infile("bible-reading-plan.csv");
	std::string line;
	while (std::getline(infile, line)) {
		auto tokens = split(line, ',');
		std::string text;
		if (std::stoi(tokens[0]) == conf.month()) {
			if (conf.plan_type() == config::PlanType::ONE_YEAR) {
				text =
					get_book_name(tokens, 10) + " " + tokens[11] + "\n";

				if (!tokens[12].empty()) {
					text +=
						get_book_name(tokens, 12) + " " + tokens[13] + "\n";
				}
			}
			text +=
				get_book_name(tokens, 4)+ " " + tokens[5];

			bible_reading_plan.push(text);
		}
	}
	return bible_reading_plan;
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

double get_day_x(int x_index) {
	return x_index * conf.cell_width() + conf.cell_margin();
}

double get_day_y(int y_index) {
	return y_index * conf.cell_height() + y_offset;
}

void draw_text_of_day_number(cairo_t *cr, int x, int y, const char* text)
{
	PangoLayout *layout = init_pango_layout(cr,
			conf.has_day_number_font_family() ?
			conf.day_number_font_family() :
			conf.default_font_family(),
			conf.day_number_font_size());
	pango_layout_set_text(layout, text, -1);

	cairo_move_to(cr,
			get_day_x(x) + conf.cell_margin(),
			get_day_y(y) + conf.cell_margin());
	pango_cairo_show_layout(cr, layout);

	g_object_unref(layout);
}

void draw_text_of_day_plan(cairo_t *cr, int x, int y, const std::string text)
{
	PangoLayout *layout =
	init_pango_layout(cr, conf.has_day_plan_font_family() ?
			conf.day_plan_font_family() : conf.default_font_family(),
			conf.day_plan_font_size());
	pango_layout_set_text(layout, text.c_str(), -1);

	int width, height;
	pango_layout_get_size(layout, &width, &height);
	cairo_move_to(cr,
			get_day_x(x + 1) -
			((double) width / PANGO_SCALE) - conf.cell_margin(),
			get_day_y(y + 1) -
			((double) height / PANGO_SCALE) - conf.cell_margin());
	pango_cairo_show_layout(cr, layout);

	g_object_unref(layout);
}

void month_label(cairo_t *cr, int surface_width) {
	static std::vector<std::string> month_text = {
		"January", "February", "March", "April", "May", "June", "July",
		"August", "September", "October", "November", "December"};

	PangoLayout *layout =
	init_pango_layout(cr, conf.has_month_label_font_family() ?
			conf.month_label_font_family() :
			conf.default_font_family(),
			conf.month_label_font_size());
	if (conf.language() == config::Language::ENGLISH) {
		if (conf.month_label_uppercase()) {
			auto& str = month_text[conf.month() - 1];
			std::transform(str.begin(), str.end(), str.begin(), ::toupper);
		}
		pango_layout_set_text(layout, month_text[conf.month() - 1].c_str(), -1);
	} else {
		char buf[3];
		sprintf(buf, "%d", conf.month());
		pango_layout_set_text(layout, buf, -1);
	}

	int width, height;
	pango_layout_get_size(layout, &width, &height);
	cairo_move_to(cr,
			(surface_width - ((double) width / PANGO_SCALE)) / 2,
			conf.cell_margin());
	y_offset += (double) height / PANGO_SCALE + conf.cell_margin() * 2;
	console->info("y_offset: {}" , y_offset);
	pango_cairo_show_layout(cr, layout);

	g_object_unref(layout);
}

void wday_label(cairo_t *cr) {
	const char *wday_text[] = {"Sunday", "Monday", "Tuesday",
		"Wednesday", "Thursday", "Friday", "Saturday"};

	const char *wday_text_ko[] = {
		"일", "월", "화", "수", "목", "금", "토"};

	double max_height = 0;
	for (int i = 0; i < 7; ++i) {
		PangoLayout *layout =
			init_pango_layout(cr, conf.has_wday_label_font_family() ?
					conf.wday_label_font_family() :
					conf.default_font_family(),
					conf.wday_label_font_size());
		pango_layout_set_text(layout,
				conf.language() == config::Language::ENGLISH ?
				wday_text[i] : wday_text_ko[i],
				-1);

		int width, height;
		pango_layout_get_size(layout, &width, &height);

		cairo_move_to(cr, conf.cell_margin() + i * conf.cell_width() +
				(conf.cell_width() - ((double) width / PANGO_SCALE)) /
				2,
				y_offset + conf.cell_margin());
		pango_cairo_show_layout(cr, layout);

		g_object_unref(layout);

		max_height = std::max(max_height,
				(double) height / PANGO_SCALE);
	}
	y_offset += max_height + conf.cell_margin() * 2;
}

int get_wday_index(struct tm const &timeinfo) {
	return timeinfo.tm_wday;
}

void days_of_month(cairo_t *cr) {
	std::queue<std::string> bible_reading_plan =
		get_bible_reading_plan_of_month();
	time_t t = get_first_day_of_month_in_sec(
			conf.year(), conf.month());
	struct tm timeinfo = *localtime(&t);

	int x = get_wday_index(timeinfo);
	int y = 0;
	while (timeinfo.tm_mon == conf.month() - 1) {
		// Label
		char buf[4];
		sprintf(buf, "%d", timeinfo.tm_mday);
		draw_text_of_day_number(cr, x, y, buf);

		draw_text_of_day_plan(cr, x, y, bible_reading_plan.front());
		bible_reading_plan.pop();

		timeinfo = *get_next_day(&t);
		x++;
		if (x >= 7) {
			x = 0;
			y++;
		}
	}
}

int count_weeks()
{
	time_t t = get_first_day_of_month_in_sec(conf.year(), conf.month());
	struct tm timeinfo = *localtime(&t);
	int last_day_1st_week = 7 - get_wday_index(timeinfo);
	int days_after_1st_week = days_per_months[conf.month() - 1] - last_day_1st_week;
	int weeks = days_after_1st_week / 7 + 1;
	if (days_after_1st_week % 7 > 0) {
		weeks += 1;
	}
	return weeks;
}

void month() {
	y_offset = 0;

	int surface_width;
	int surface_height;
	switch (conf.paper_type()) {
		case config::PaperType::US_LETTER:
			surface_width = 1100;
			surface_height = 850;
			break;
		case config::PaperType::A4:
			surface_width = 1175;
			surface_height = 825;
			break;
	}
	conf.set_cell_width(((double) surface_width - conf.cell_margin() * 2) / 7);

	std::string output_file_name = conf.output_file_name() + '_' +
		std::to_string(conf.month());

	cairo_surface_t *surface = NULL;
	switch (conf.output_type()) {
		case config::OutputType::PDF:
			surface = cairo_pdf_surface_create(
					(output_file_name + ".pdf").c_str(),
					surface_width, surface_height);
			break;
		case config::OutputType::PNG:
			surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32,
					surface_width, surface_height);
			break;
		default:
			surface = cairo_svg_surface_create(
					(output_file_name + ".svg").c_str(),
					surface_width, surface_height);
			break;
	}
	cairo_t *cr = cairo_create(surface);

	month_label(cr, surface_width);

	// Draw frame
	cairo_set_line_width(cr, conf.line_width());

	cairo_rectangle(cr, conf.cell_margin(), y_offset,
			surface_width - conf.cell_margin() * 2,
			surface_height - y_offset - conf.cell_margin());

	for (int x = 1; x < 7; ++x) {
		cairo_move_to(cr, get_day_x(x), y_offset);
		cairo_line_to(cr, get_day_x(x), surface_height - conf.cell_margin());
	}

	wday_label(cr);

	// Stroke a line below the labels
	cairo_move_to(cr, conf.cell_margin(), y_offset);
	cairo_line_to(cr, surface_width - conf.cell_margin(), y_offset);

	// Horizontal lines below dates
	conf.set_cell_height(
			((double) surface_height - conf.cell_margin() - y_offset) /
			count_weeks());

	for (int y = 1; y < count_weeks(); ++y) {
		cairo_move_to(cr, conf.cell_margin(),
				y_offset + y * conf.cell_height());
		cairo_line_to(cr, surface_width - conf.cell_margin(),
				y_offset + y * conf.cell_height());
	}

	cairo_stroke(cr);

	days_of_month(cr);

	if (conf.output_type() == config::OutputType::PNG) {
		cairo_surface_write_to_png(surface,
				(output_file_name + ".png").c_str());
	}

	cairo_destroy(cr);
	cairo_surface_destroy(surface);
}

bool parse_config() {
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
	fileInput.SetCloseOnDelete( true );

	if (!google::protobuf::TextFormat::Parse(&fileInput, &conf)) {
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
	if (!parse_config()) {
		console->error("Config parsing error");
		return EXIT_FAILURE;
	}

	if (conf.has_month()) {
		month();
	} else {
		for (int i = 1; i <= 12; ++i) {
			conf.set_month(i);
			month();
		}
	}
	return EXIT_SUCCESS;
}
