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
#include <set>

class Atom
{
public:
	Atom(const std::string &const_name);
	~Atom();

	Atom& operator=(const Atom &other);

	bool operator<(const Atom &other) const;
	bool operator==(const Atom &other) const;

	enum version_op
	{
		version_gt = 1,
		version_lt = 2,
		version_eq = 4,
		version_gt_eq = version_gt | version_eq,
		version_lt_eq = version_lt | version_eq,
		version_none = 0
	};

	bool is_valid() const;
	const std::string& atom() const;
	const std::string& name() const;
	const std::string& category() const;
	const std::string& version() const;
	const std::string& slot() const;
	const std::string& atom_and_slot() const;
	const std::string& full_atom() const;
	const std::string& repository() const;
	version_op vop() const;
	bool check_installed() const;

	std::set<Atom> get_all_installed_packages() const;

private:
	Atom();

	static std::string get_slot_of_package(const std::string &atom_and_version);

	bool m_valid;
	std::string m_name;
	std::string m_category;
	version_op m_vop;
	std::string m_version;
	std::string m_slot;
	std::string m_repository;

	// constructed values
	std::string m_atom;
	std::string m_atom_and_slot;
	std::string m_full_atom;

	// construction functions
	void calculate_atom();
	void calculate_atom_and_slot();
	void calculate_full_atom();
};

#endif // DT_ATOM_H
