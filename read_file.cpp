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

#include "read_file.h"

int parseFileLine(FILE *f, std::vector<std::string> **results)
{
	if ((f == NULL) || (results == NULL))
	{
		return parse_error_einval;
	}

	try
	{
		*results = new std::vector<std::string>;
		if (*results == NULL)
		{
			throw std::bad_alloc();
		}
	}
	catch (std::bad_alloc&)
	{
		return parse_error_memory;
	}

	int c;
	bool comment = false;
	std::string atom;
	bool process_value = false;

	for (;;)
	{
		c = fgetc(f);

		if (!comment)
		{
			if ((c != '\n') && (c != EOF) && (!isspace(c)) && (c != '#'))
			{
				try
				{
					atom.append(1, c);
				}
				catch (std::bad_alloc&)
				{
					delete *results;
					return parse_error_memory;
				}
			}

			if (c == '#')
			{
				comment = true;
				process_value = true;
			}
		}

		if ((c == '\n') || (c == EOF) || (isspace(c)))
		{
			process_value = true;
		}

		if (process_value)
		{
			process_value = false;

			if (atom.length() > 0)
			{
				try
				{
					(*results)->push_back(atom);
					atom.clear();
				}
				catch (std::bad_alloc&)
				{
					delete *results;
					return parse_error_memory;
				}
			}

			if (c == '\n')
			{
				return parse_result_eol;
			}

			if (c == EOF)
			{
				return parse_result_eof;
			}
		}
	}
}
