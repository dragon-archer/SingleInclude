/**
 * @file      main.cpp
 * @brief     Main source file of SingleInclude
 * @version   1.0
 * @author    dragon-archer (dragon-archer@outlook.com)
 * @copyright Copyright (c) 2022
 */
#include <filesystem>
#include <fstream>
#include <iostream>
#include <list>
#include <map>
#include <regex>
#include <set>
#include <string>

#if __has_include(<format.h>) // Provided by CMake
#define FMT_HEADER_ONLY
#include <format.h>
using fmt::make_format_args;
using fmt::vformat;
#elif __has_include(<format>)
#include <format>
#elif __has_include(<fmt/format.h>)
#define FMT_HEADER_ONLY
#include <fmt/format.h>
using fmt::make_format_args;
using fmt::vformat;
#else
#error SingleInclude must be compiled with eithor <format> in std library or an external fmtlib
#endif
using namespace std;
namespace fs = filesystem;

enum error_type : int {
	E_NO_ERROR = 0,
	E_TOO_LESS_ARGUMENTS,
	E_FILE_NOT_EXIST,
	E_DIR_NOT_EXIST,
	E_UNKNOWN_OPTION,
	E_TOO_MANY_INPUT,
	E_FILE_ERROR,
	E_FINISH,
	ERROR_COUNT
};

constexpr const char* error_msg[ERROR_COUNT] = {
	"No error occured",
	"Too less arguments",
	"{}: File doesn't exist",
	"{}: Directory doesn't exist",
	"Unkown option {}",
	"Too many input file",
	"File error: {}",
	"Finished"
};

enum option_t : int {
	O_INCLUDE_ALL,
	O_DRY,
	O_HELP,
	O_INCLUDE_PATH,
	O_OUT,
	O_TREE,
	O_VERBOSE,
	OPTION_COUNT
};

const map<char, option_t> short_options = {
	make_pair('a', O_INCLUDE_ALL),
	make_pair('d', O_DRY),
	make_pair('h', O_HELP),
	make_pair('I', O_INCLUDE_PATH),
	make_pair('o', O_OUT),
	make_pair('t', O_TREE),
	make_pair('v', O_VERBOSE)
};

const map<string, option_t> long_options = {
	make_pair("all", O_INCLUDE_ALL),
	make_pair("dry", O_DRY),
	make_pair("help", O_HELP),
	make_pair("include", O_INCLUDE_PATH),
	make_pair("out", O_OUT),
	make_pair("tree", O_TREE),
	make_pair("verbose", O_VERBOSE)
};

const regex	 regex_include { R"+(^\s*#\s*include\s*(<.*>|".*")\s*$)+" };
const regex	 regex_system_include { R"+(^\s*#\s*include\s*<.*>\s*$)+" };
const string header = R"+(// This file is generated automatically by SingleInclude
// It's suggested not to edit anything below
// If you found any issue, please report to https://github.com/dragon-archer/SingleInclude/issues
)+";

string progname;
bool   include_all = false;
bool   verbose	   = false;
bool   tree		   = false;
bool   dry_run	   = false;

struct error_state {
	error_type e;
	string	   w;
	error_state(error_type error = E_NO_ERROR, string what = "")
		: e(error)
		, w(what) { }

	operator error_type() {
		return e;
	}

	string what() {
		return "Error: " + vformat(error_msg[e], make_format_args(w));
	}
};

enum include_state_t {
	I_EXPENDED,
	I_ALREADY_INCLUDED,
	I_NOT_FOUND,
	INCLUDE_STATE_COUNT,
};

constexpr const char* include_msg[INCLUDE_STATE_COUNT] = {
	"expended",
	"already included",
	"not found"
};

struct file_t {
	fs::path		name;
	list<file_t>	includeFiles;
	include_state_t state	 = I_EXPENDED;
	bool			is_angle = false;

	file_t(fs::path p = "", include_state_t istate = I_EXPENDED)
		: name(p)
		, state(istate) { }

	operator fs::path() {
		return name;
	}
};

struct state_t {
	error_state	   error;
	file_t		   file;
	fs::path	   outfilename;
	list<fs::path> includePaths;
	set<fs::path>  includedFiles;

	state_t(error_state e = E_NO_ERROR)
		: error(e) { }
};

void log(string msg) {
	if(verbose) {
		cerr << msg << endl;
	}
}

string add_quote(string name, bool is_angle) {
	constexpr const char* begin_quote[2] = { "\"", "<" };
	constexpr const char* end_quote[2]	 = { "\"", ">" };

	return begin_quote[is_angle] + name + end_quote[is_angle];
}

void print_help() {
	cout << "SingleInclude: A small program to generate a single include file for C/C++\n"
		 << "Usage: " << progname << " [options...] FILE\n"
		 << "Options:\n"
		 << "  -a, --all\t\tExpend all files found, no matter whether it has been expended before\n"
		 << "\t\t\tBy default, if one file has been expended before, it will be omitted later\n"
		 << "\t\t\tThis may be helpful if you use macro to choose which file to include,\n"
		 << "\t\t\tas this program cannot understand macro now\n"
		 << "  -d, --dry\t\tDry run mode, do not output the header file\n"
		 << "  -h, --help\t\tPrint this help message and exit\n"
		 << "  -I, --include PATH\tAdd PATH to include paths\n"
		 << "  -o, --out FILE\tSet the output file name to FILE\n"
		 << "\t\t\tBy default, the output will print to the console\n"
		 << "  -t, --tree\t\tPrint dependent tree\n"
		 << "  -v, --verbose\t\tPrint more information to stderr (implictly include --tree)\n"
		 << endl;
}

error_state parse_option(list<string>& args, state_t& state, option_t op, const string& extra = "") {
	switch(op) {
	case O_INCLUDE_ALL: {
		include_all = true;
		break;
	}
	case O_DRY: {
		dry_run = true;
		break;
	}
	case O_HELP: {
		print_help();
		return E_FINISH;
	}
	case O_INCLUDE_PATH: {
		fs::path arg;
		if(extra.empty()) {
			arg = args.front();
			args.pop_front();
		} else {
			arg = extra;
		}
		if(!fs::is_directory(arg)) {
			return { E_DIR_NOT_EXIST, arg.string() };
		}
		state.includePaths.push_back(fs::canonical(arg));
		break;
	}
	case O_OUT: {
		fs::path arg;
		if(extra.empty()) {
			arg = args.front();
			args.pop_front();
		} else {
			arg = extra;
		}
		state.outfilename = arg;
		break;
	}
	case O_TREE: {
		tree = true;
		break;
	}
	case O_VERBOSE: {
		verbose = true;
		break;
	}
	case OPTION_COUNT: { // Avoid warning, this should never be reached
		return E_UNKNOWN_OPTION;
	}
	}
	return E_NO_ERROR;
}

state_t parse_config(int argc, char* argv[]) {
	state_t state;
	if(argc == 0) {
		print_help();
		return { E_TOO_LESS_ARGUMENTS };
	}
	string		 arg;
	list<string> args;
	for(int i = 0; i < argc; ++i) {
		args.push_back(argv[i]);
	}

	while(!args.empty()) {
		arg = args.front();
		args.pop_front();
		if(arg[0] == '-') { // options
			if(arg[1] == '-') { // long options
				if(auto it = long_options.find(arg.substr(2)); it != long_options.end()) {
					if(auto error = parse_option(args, state, it->second); error != E_NO_ERROR) {
						return error;
					}
				} else {
					return { { E_UNKNOWN_OPTION, arg } };
				}
			} else { // short options
				if(auto it = short_options.find(arg[1]); it != short_options.end()) {
					if(auto error = parse_option(args, state, it->second, arg.substr(2)); error != E_NO_ERROR) {
						return error;
					}
				} else {
					return { { E_UNKNOWN_OPTION, arg } };
				}
			}
		} else {
			if(!state.file.name.empty()) {
				return { E_TOO_MANY_INPUT };
			}
			state.file.name = arg;
			if(!fs::is_regular_file(state.file.name)) {
				return { { E_FILE_NOT_EXIST, state.file.name.string() } };
			}
			state.file.name = fs::canonical(state.file.name);
		}
	}
	return state;
}

error_state parse_include(state_t& config, file_t& file, string& out) {
	ifstream fin;
	fin.open(file.name);
	if(!fin.is_open()) {
		return { E_FILE_ERROR, "Cannot open file " + config.file.name.string() };
	}
	config.includedFiles.insert(file);
	string line;
	string includeFile;
	auto   search_paths = config.includePaths;
	if(!file.is_angle) {
		log("Add current path to search: " + file.name.parent_path().string());
		search_paths.push_front(file.name.parent_path());
	}
	while(!fin.eof()) {
		getline(fin, line);
		if(!regex_match(line, regex_include)) {
			out += line + '\n';
		} else {
			file_t temp;
			if(regex_match(line, regex_system_include)) {
				temp.is_angle = true;
			}
			auto it1 = line.begin();
			while(*it1 != '<' && *it1 != '"') { // Since line match the regex, skip to it directly
				++it1;
			}
			++it1;
			while(isspace(*it1)) {
				++it1;
			}
			auto it2 = it1;
			while(*it2 != '>' && *it2 != '"' && !isspace(*it2)) {
				++it2;
			}
			includeFile.assign(it1, it2);
			log("Found include file " + add_quote(includeFile, temp.is_angle));
			bool	 found = false;
			fs::path canonicalFile;
			string	 content;
			for(auto& p : search_paths) {
				canonicalFile = p / includeFile;
				if(fs::is_regular_file(canonicalFile)) {
					canonicalFile = fs::canonical(canonicalFile);
					found		  = true;
					temp.name	  = canonicalFile;
					log("Include file expends to " + canonicalFile.string());
					if(!include_all && config.includedFiles.find(canonicalFile) != config.includedFiles.end()) {
						log("Include file already exists, ignore");
						temp.state = I_ALREADY_INCLUDED;
					} else {
						temp.state = I_EXPENDED;
						if(auto err = parse_include(config, temp, content); err != E_NO_ERROR) {
							return err;
						}
					}
					file.includeFiles.push_back(temp);
				}
			}
			if(!found) {
				log("Ignore include file " + add_quote(includeFile, temp.is_angle) + " because of not found (may be system header)");
				temp.name  = includeFile;
				temp.state = I_NOT_FOUND;
				file.includeFiles.push_back(temp);
				out += line + '\n';
			} else {
				if(temp.state == I_EXPENDED) {
					out += "// " + line + '\n';
					out += content;
					out += "// End " + line + '\n';
				} else {
					out += "// " + line + " (omitted because it has been expended)\n";
				}
			}
		}
	}
	fin.close();
	return E_NO_ERROR;
}

void dump_tree(const file_t& f, int depth) {
	string prefix(2 * depth, ' ');
	cout << prefix
		 << add_quote(f.name.string(), f.is_angle)
		 << " (" << include_msg[f.state] << ")\n";
	for(auto& i : f.includeFiles) {
		dump_tree(i, depth + 1);
	}
}

void dump(const state_t& config) {
	cout << "Target name: " << config.file.name.string() << endl;
	cout << "Include paths:\n";
	for(auto& f : config.includePaths) {
		cout << '\t' << f.string() << endl;
	}
	cout << "All included files:\n";
	for(auto& f : config.includedFiles) {
		cout << '\t' << f.string() << endl;
	}
	cout << "Tree view:\n";
	dump_tree(config.file, 0);
}

int main(int argc, char* argv[]) {
	progname	   = argv[0];
	state_t config = parse_config(argc - 1, argv + 1);
	if(config.error == E_FINISH) {
		return E_NO_ERROR;
	} else if(config.error != E_NO_ERROR) {
		cerr << config.error.what() << endl;
		return config.error;
	}
	string		content = header;
	error_state error	= parse_include(config, config.file, content);
	if(error == E_FINISH) {
		return E_NO_ERROR;
	} else if(error != E_NO_ERROR) {
		cerr << error.what() << endl;
		return error;
	}
	if(!dry_run) {
		if(!config.outfilename.empty()) {
			ofstream fout;
			fout.open(config.outfilename);
			if(!fout.is_open()) {
				error_state error { E_FILE_ERROR, "Cannot open output file: " + config.outfilename.string() };
				cerr << error.what() << endl;
				return error;
			}
			fout << content;
			fout.close();
		} else {
			cout << content;
		}
	}
	if(verbose) {
		dump(config);
	} else if(tree) {
		dump_tree(config.file, 0);
	}
	return E_NO_ERROR;
}
