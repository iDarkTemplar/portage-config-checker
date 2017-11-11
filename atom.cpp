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

#include "atom.h"

#include <ctype.h>
#include <stdlib.h>
#include <dirent.h>
#include <string.h>
#include <libgen.h>

#include <tuple>
#include <regex>

Atom::Atom()
	: m_valid(false),
	m_vop(version_none)
{
}

Atom::Atom(const std::string &const_name)
	: m_valid(false),
	m_vop(version_none)
{
	std::regex reg_expr("^"
		// version sign
		"((?:>|>=|=|<=|<)?)"
		// category
		"([[:alpha:]][[:alnum:]]*(?:[\\-_][[:alnum:]]+)*)"
		// category and name delimiter
		"/"
		// name
		"([[:alpha:]](?:[[:alnum:]]|\\+)*(?:[\\-_\\.][[:alpha:]](?:[[:alnum:]]|\\+)*)*)"
		// version
		"(?:\\-([[:digit:]][[:alnum:]]*(?:[\\-_\\.][[:alnum:]]+)*))?"
		// slot + subslot
		"(?:\\:([[:digit:]][[:alnum:]]*(?:[\\-_\\./][[:alnum:]]+)*))?"
		// repository
		"(?:\\:\\:([[:alpha:]][[:alnum:]]*(?:[\\-_\\.][[:alpha:]][[:alnum:]]*)*))?"
		"$");

	std::smatch reg_results;

	bool result = std::regex_match(const_name, reg_results, reg_expr);
	if (result)
	{
		if (!reg_results.str(1).empty())
		{
			if (reg_results.str(1) == ">")
			{
				m_vop = version_gt;
			}
			else if (reg_results.str(1) == ">=")
			{
				m_vop = version_gt_eq;
			}
			else if (reg_results.str(1) == "=")
			{
				m_vop = version_eq;
			}
			else if (reg_results.str(1) == "<")
			{
				m_vop = version_lt;
			}
			else if (reg_results.str(1) == "<=")
			{
				m_vop = version_lt_eq;
			}
			else
			{
				return;
			}
		}

		m_category   = reg_results.str(2);
		m_name       = reg_results.str(3);
		m_version    = reg_results.str(4);
		std::tie(m_slot, m_subslot) = split_slot_and_subslot(reg_results.str(5));
		m_repository = reg_results.str(6);

		calculate_atom();
		calculate_atom_and_slot();
		calculate_atom_and_slot_and_subslot();
		calculate_full_atom();
		m_valid = true;
	}
}

Atom::~Atom()
{
}

bool Atom::is_valid() const
{
	return m_valid;
}

const std::string& Atom::atom() const
{
	return m_atom;
}

const std::string& Atom::name() const
{
	return m_name;
}

const std::string& Atom::category() const
{
	return m_category;
}

const std::string& Atom::version() const
{
	return m_version;
}

const std::string& Atom::slot() const
{
	return m_slot;
}

const std::string& Atom::subslot() const
{
	return m_subslot;
}

const std::string& Atom::atom_and_slot() const
{
	return m_atom_and_slot;
}

const std::string& Atom::atom_and_slot_and_subslot() const
{
	return m_atom_and_slot_and_subslot;
}

const std::string& Atom::full_atom() const
{
	return m_full_atom;
}

const std::string& Atom::repository() const
{
	return m_repository;
}

Atom::version_op Atom::vop() const
{
	return m_vop;
}

bool Atom::check_installed() const
{
	std::string name_str = m_name;

	{
		size_t index = name_str.rfind('+');

		while (index != std::string::npos)
		{
			name_str.replace(index, 1, "\\+");
			index = name_str.rfind('+', index);
		}
	}

	//                        name,          version
	std::regex reg_expr("^" + name_str + "\\-[[:digit:]][[:alnum:]]*(?:[\\-_\\.][[:alnum:]]+)*$");

	bool result = false;

	if (!m_valid)
	{
		return result;
	}

	std::string pkg_path = std::string("/var/db/pkg/") + m_category;

	struct dirent **namelist;
	int n;

	n = scandir(pkg_path.c_str(), &namelist, NULL, alphasort);
	if (n >= 0)
	{
		try
		{
			while (n--)
			{
				if (std::regex_match(namelist[n]->d_name, reg_expr))
				{
					std::string slot, subslot;
					std::tie(slot, subslot) = split_slot_and_subslot(get_slot_of_package(m_category + std::string("/") + std::string(namelist[n]->d_name)));

					if (((m_slot.empty()) || (m_slot == slot))
						&& ((m_subslot.empty()) || (m_subslot == subslot)))
					{
						result = true;
					}
				}

				free(namelist[n]);
			}

			free(namelist);
		}
		catch (...)
		{
			free(namelist[n]);

			while (n--)
			{
				free(namelist[n]);
			}

			free(namelist);
			throw;
		}
	}

	return result;
}

std::set<Atom> Atom::get_all_installed_packages() const
{
	std::string name_str = m_name;

	{
		size_t index = name_str.rfind('+');

		while (index != std::string::npos)
		{
			name_str.replace(index, 1, "\\+");
			index = name_str.rfind('+', index);
		}
	}

	//                        name,          version
	std::regex reg_expr("^" + name_str + "\\-([[:digit:]][[:alnum:]]*(?:[\\-_\\.][[:alnum:]]+)*)$");

	std::set<Atom> result;

	if (!m_valid)
	{
		return result;
	}

	std::string pkg_path = std::string("/var/db/pkg/") + m_category;

	struct dirent **namelist;
	int n;

	n = scandir(pkg_path.c_str(), &namelist, NULL, alphasort);
	if (n >= 0)
	{
		try
		{
			while (n--)
			{
				std::cmatch reg_results;

				if (std::regex_match(namelist[n]->d_name, reg_results, reg_expr))
				{
					Atom atom;
					atom.m_category = m_category;
					atom.m_name = m_name;
					atom.m_version = reg_results.str(1);
					std::tie(atom.m_slot, atom.m_subslot) = split_slot_and_subslot(get_slot_of_package(m_category + std::string("/") + std::string(namelist[n]->d_name)));
					atom.calculate_atom();
					atom.calculate_atom_and_slot();
					atom.calculate_atom_and_slot_and_subslot();
					atom.calculate_full_atom();
					atom.m_valid = m_valid;
					result.insert(atom);
				}

				free(namelist[n]);
			}

			free(namelist);
		}
		catch (...)
		{
			free(namelist[n]);

			while (n--)
			{
				free(namelist[n]);
			}

			free(namelist);
			throw;
		}
	}

	return result;
}

std::string Atom::get_slot_of_package(const std::string &atom_and_version)
{
	std::string filename = std::string("/var/db/pkg/") + atom_and_version +  std::string("/SLOT");
	std::string result;
	FILE *f;

	f = fopen(filename.c_str(), "r");
	if (f != NULL)
	{
		try
		{
			int c;

			for (;;)
			{
				c = fgetc(f);

				if ((c == '\n') || (c == EOF) || (isspace(c)))
				{
					break;
				}

				result.append(1, c);
			}

			fclose(f);
		}
		catch (...)
		{
			fclose(f);
			throw;
		}
	}

	return result;
}

std::tuple<std::string, std::string> Atom::split_slot_and_subslot(const std::string &input)
{
	std::string::size_type pos = input.find('/');
	if ((pos == std::string::npos) || (pos == input.length() - 1))
	{
		return std::make_tuple(input, std::string());
	}
	else
	{
		return std::make_tuple(input.substr(0, pos), input.substr(pos + 1));
	}
}

void Atom::calculate_atom()
{
	m_atom = m_category + std::string("/") + m_name;
}

void Atom::calculate_atom_and_slot()
{
	std::string slot;

	if (!m_slot.empty())
	{
		slot = ":" + m_slot;
	}

	m_atom_and_slot = m_atom + slot;
}

void Atom::calculate_atom_and_slot_and_subslot()
{
	m_atom_and_slot_and_subslot = m_atom_and_slot;

	if (!m_subslot.empty())
	{
		m_atom_and_slot_and_subslot += std::string("/") + m_subslot;
	}
}

void Atom::calculate_full_atom()
{
	std::stringstream full_atom_stream;

	switch (m_vop)
	{
	case version_gt:
		full_atom_stream << ">";
		break;
	case version_lt:
		full_atom_stream << "<";
		break;
	case version_eq:
		full_atom_stream << "=";
		break;
	case version_gt_eq:
		full_atom_stream << ">=";
		break;
	case version_lt_eq:
		full_atom_stream << "<=";
		break;
	case version_none:
	default:
		break;
	}

	full_atom_stream << m_atom;

	if (!m_version.empty())
	{
		full_atom_stream << "-" << m_version;
	}

	if (!m_slot.empty())
	{
		full_atom_stream << ":" << m_slot;

		if (!m_subslot.empty())
		{
			full_atom_stream << "/" << m_subslot;
		}
	}

	if (!m_repository.empty())
	{
		full_atom_stream << "::" << m_repository;
	}

	m_full_atom = full_atom_stream.str();
}

Atom& Atom::operator=(const Atom &other)
{
	if (this != &other)
	{
		this->m_valid      = other.m_valid;
		this->m_name       = other.m_name;
		this->m_category   = other.m_category;
		this->m_vop        = other.m_vop;
		this->m_version    = other.m_version;
		this->m_slot       = other.m_slot;
		this->m_subslot    = other.m_subslot;
		this->m_repository = other.m_repository;

		this->m_atom          = other.m_atom;
		this->m_atom_and_slot = other.m_atom_and_slot;
		this->m_full_atom     = other.m_full_atom;
	}

	return *this;
}

bool Atom::operator<(const Atom &other) const
{
	return (std::make_tuple(this->m_name, this->m_category, this->m_vop, this->m_version, this->m_slot, this->m_subslot, this->m_repository)
	      < std::make_tuple(other.m_name, other.m_category, other.m_vop, other.m_version, other.m_slot, other.m_subslot, other.m_repository));
}

bool Atom::operator==(const Atom &other) const
{
	return (std::make_tuple(this->m_name, this->m_category, this->m_vop, this->m_version, this->m_slot, this->m_subslot, this->m_repository)
	     == std::make_tuple(other.m_name, other.m_category, other.m_vop, other.m_version, other.m_slot, other.m_subslot, other.m_repository));
}
