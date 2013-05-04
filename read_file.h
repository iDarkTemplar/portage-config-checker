/*
 * Copyright (C) 2016 i.Dark_Templar <darktemplar@dark-templar-archives.net>
 *
 * This file is part of Portage Config Checker.
 *
 * Portage Config Checker is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Portage Config Checker is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Portage Config Checker.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#ifndef DT_READ_FILE_H
#define DT_READ_FILE_H

#include <stdio.h>
#include <string>
#include <vector>

enum parse_result
{
	parse_result_eof = 1,
	parse_result_eol = 0,
	parse_error_einval = -1
};

int parseFileLine(FILE *f, std::vector<std::string> &results);

#endif // DT_READ_FILE_H
