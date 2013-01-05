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

#ifndef DT_ATOM_H
#define DT_ATOM_H

#include <string>

class Atom
{
public:
	Atom(std::string const_name);
	~Atom();

	enum version_op
	{
		version_gt = 1,
		version_lt = 2,
		version_eq = 4,
		version_gt_eq = version_gt | version_eq,
		version_lt_eq = version_lt | version_eq,
		version_none = 0
	};

	bool is_valid();
	const std::string atom();
	const std::string name();
	const std::string category();
	const std::string version();
	const std::string full_atom();
	int vop();
	bool check_installed();

private:
	bool m_valid;
	std::string m_name;
	std::string m_category;
	int m_vop;
	std::string m_version;

	bool check_name(std::string cname);
};

#endif // DT_ATOM_H
