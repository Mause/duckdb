//
// Created by me on 6/06/23.
//

#ifndef DUCKDB_PATH_H
#define DUCKDB_PATH_H

#include <string>
class Path {
public:
	Path(const std::string &p_string) {
	}
	Path(char *path) {
	}
	Path strip_prefix(const std::string &p_string) {
	}

	static Path from(const std::string &path);
};

#endif // DUCKDB_PATH_H
