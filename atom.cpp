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

#include <boost/regex.hpp>

Atom::Atom(std::string const_name)
	: m_valid(false),
	m_vop(version_none)
{
	//                       version sign,    category,                            name,                                version,                                    slot
	boost::regex reg_expr("^([>|>=|=|<=|<]?)([[:alnum:]]+(?:[\\-_][[:alnum:]]+)*)/([[:alnum:]]+(?:[\\-_][[:alnum:]]+)*)(?:\\-([[:digit:]]+(?:[\\-_\\.][[:alnum:]]+)*))*(?:\\:([[:digit:]]+(?:[\\-_\\.][[:alnum:]]+)*))*$");
	boost::smatch reg_results;

	bool result = boost::regex_match(const_name, reg_results, reg_expr);
	if (result)
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

		m_category = reg_results.str(2);
		m_name     = reg_results.str(3);
		m_version  = reg_results.str(4);
		m_slot     = reg_results.str(5);
		m_valid    = true;
	}
}

Atom::~Atom()
{
}

bool Atom::is_valid()
{
	return m_valid;
}

const std::string Atom::atom()
{
	return m_category + std::string("/") + m_name;
}

const std::string Atom::name()
{
	return m_name;
}

const std::string Atom::category()
{
	return m_category;
}

const std::string Atom::version()
{
	return m_version;
}

const std::string Atom::slot()
{
	return m_slot;
}


const std::string Atom::atom_and_slot()
{
	std::string slot;

	if (!m_slot.empty())
	{
		slot = ":" + m_slot;
	}

	return m_category + std::string("/") + m_name + slot;
}

const std::string Atom::full_atom()
{
	std::string v;
	std::string slot;

	switch (m_vop)
	{
	case version_gt:
		v = std::string(">");
		break;
	case version_lt:
		v = std::string("<");
		break;
	case version_eq:
		v = std::string("=");
		break;
	case version_gt_eq:
		v = std::string(">=");
		break;
	case version_lt_eq:
		v = std::string("<=");
		break;
	case version_none:
	default:
		break;
	}

	if (!m_slot.empty())
	{
		slot = ":" + m_slot;
	}

	return v + m_category + std::string("/") + m_name + std::string("-") + m_version + m_slot;
}

int Atom::vop()
{
	return m_vop;
}

bool Atom::check_name(std::string cname)
{
	int pos;
	std::string spec_smbs = "-_.";

	for (int i = 0; i < spec_smbs.size(); ++i)
	{
		pos = -1;

		while ((pos = cname.find(spec_smbs[i], pos+1)) != cname.npos)
		{
			if ((pos == 0) || (pos == cname.size() - 1))
			{
				return false;
			}

			for (int j = 0; j < spec_smbs.size(); ++j)
			{
				if (cname[pos-1] == spec_smbs[j])
				{
					return false;
				}
			}
		}
	}

	return true;
}

bool Atom::check_installed()
{
	if (!m_valid)
	{
		return false;
	}

	bool result = false;

	std::string pkg_path = std::string("/var/db/pkg/") + atom();

	char *path = (char*) malloc(pkg_path.length()+1);
	if (path == NULL)
	{
		return false;
	}

	strcpy(path, pkg_path.c_str());

	char *basedir = dirname(path);

	struct dirent **namelist;
	int n;

	n = scandir(basedir, &namelist, NULL, alphasort);
	if (n < 0)
	{
	}
	else
	{
		while (n--)
		{
			if (strncasecmp(m_name.c_str(), namelist[n]->d_name, m_name.length()) == 0)
			{
				// TODO: version check
				result = true;
			}

			free(namelist[n]);
		}

		free(namelist);
	}

	free(path);

	return result;
}
