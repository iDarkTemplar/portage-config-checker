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

#include <stdlib.h>
#include <stdio.h>
#include <dirent.h>
#include <string.h>
#include <libgen.h>

#include <sys/stat.h>
#include <unistd.h>

#include <algorithm>
#include <map>
#include <set>
#include <string>

#include <errno.h>

#include "atom.h"
#include "read_file.h"
#include "use_flag.h"

#include <regex>

bool make_check = false;
bool full_make_check = false;
bool search_all = false;
bool skip_uses = false;
bool skip_else = false;

bool check_use_name(const std::string &flag)
{
	std::regex reg_expr("[\\-]?[[:alnum:]][\\+[:alnum:]]*(?:[\\-_][[:alnum:]][\\+[:alnum:]]*)*");
	return std::regex_match(flag, reg_expr);
}

void check_use_flag_existance(const std::string &flag, const std::string &location)
{
	std::stringstream query;
	query << "equery hasuse ";

	if (full_make_check)
	{
		query << "-p -o ";
	}

	query << flag << " | wc -l";

	FILE *make_pipe = popen(query.str().c_str(), "r");
	if (make_pipe != NULL)
	{
		std::string result_string;

		try
		{
			int c;

			while ((c = fgetc(make_pipe)) != EOF)
			{
				result_string.append(1,c);
			}
		}
		catch (...)
		{
			pclose(make_pipe);
			throw;
		}

		pclose(make_pipe);

		if (result_string == "0")
		{
			printf("Error in file %s: USE-flag %s doesn't exist\n", location.c_str(), flag.c_str());
		}
	}
	else
	{
		printf("Couldn't query existance of USE-flag %s\n", flag.c_str());
	}
}

std::set<std::string> find_all_files(const std::string &location)
{
	std::set<std::string> files;
	struct stat buffer;

	if (stat(location.c_str(), &buffer) != -1)
	{
		if (S_ISDIR(buffer.st_mode))
		{
			DIR *dirp;
			struct dirent *dp;

			if ((dirp = opendir(location.c_str())) == NULL)
			{
				return files;
			}

			try
			{
				do
				{
					if ((dp = readdir(dirp)) != NULL)
					{
						if ((strcmp(dp->d_name,".") != 0) && (strcmp(dp->d_name,"..") != 0))
						{
							std::set<std::string> res = find_all_files(location + std::string("/") + std::string(dp->d_name));
							files.insert(res.begin(), res.end());
						}
					}
				} while (dp != NULL);
			}
			catch (...)
			{
				closedir(dirp);
				throw;
			}

			closedir(dirp);
		}
		else if (S_ISREG(buffer.st_mode))
		{
			files.insert(location);
		}
	}

	return files;
}

void run_query(const std::string &query, const std::string &atom_string, const Atom &atom, std::map<std::string, std::set<std::string> > &packagesFlagsList)
{
	FILE *make_pipe = popen(query.c_str(), "r");
	if (make_pipe != NULL)
	{
		try
		{
			int c;
			std::string result_string;

			for (;;)
			{
				c = fgetc(make_pipe);

				if ((c != EOF) && (!isspace(c)))
				{
					result_string.append(1,c);
				}
				else
				{
					result_string.erase(0, 1);

					packagesFlagsList[atom.atom_and_slot_and_subslot()].insert(result_string);
					packagesFlagsList[atom.atom_and_slot()].insert(result_string);
					packagesFlagsList[atom.atom()].insert(result_string);

					result_string.clear();

					if (c == EOF)
					{
						break;
					}
				}
			}
		}
		catch (...)
		{
			pclose(make_pipe);
			throw;
		}

		pclose(make_pipe);
	}
	else
	{
		printf("Couldn't query USE-flags for atom %s\n", atom_string.c_str());
	}
}

void check_use_file(const std::string &location, std::map<std::string, UseFlag>& useFlags, std::map<std::pair<std::string, Atom>, UseFlag>& packageUseFlags, std::map<std::string, std::set<std::string> > &packagesFlagsList)
{
	FILE *f = fopen(location.c_str(),"r");
	if (f != NULL)
	{
		bool file_is_empty = true;

		try
		{
			int result;
			unsigned int line = 1;
			bool got_flag;
			std::vector<std::string> data;

			do
			{
				result = parseFileLine(f, data);

				if (!data.empty())
				{
					file_is_empty = false;
					Atom atom(data.at(0));

					if (atom.is_valid() && (atom.vop() == Atom::version_none) && (atom.version().empty()))
					{
						if (!atom.check_installed())
						{
							printf("Error in file %s at line %d: atom %s is not installed\n", location.c_str(), line, atom.full_atom().c_str());
						}

						got_flag = false;

						for (std::vector<std::string>::iterator i = data.begin() + 1; i != data.end(); ++i)
						{
							if (check_use_name(*i))
							{
								got_flag = true;

								bool enabled = true;

								if ((*i)[0] == '-')
								{
									enabled = false;
									(*i).erase(0, 1);
								}

								// if contains same flag in same state in make.conf
								std::map<std::string, UseFlag>::iterator useFlagIter;
								useFlagIter = useFlags.find(*i);

								if (useFlagIter != useFlags.end())
								{
									if (useFlagIter->second.getLastValue() == enabled)
									{
										printf("Error in file %s at line %d: USE-flag %s for atom %s already set to same value in %s\n", location.c_str(), line, (*i).c_str(), atom.atom_and_slot_and_subslot().c_str(), useFlagIter->second.getLocation().c_str());
									}
								}

								// same flag in some other files, then warning
								UseFlag useFlag;

								std::map<std::pair<std::string, Atom>, UseFlag>::iterator packageUseFlagIter;
								packageUseFlagIter = packageUseFlags.find(std::make_pair((*i), atom));

								if (packageUseFlagIter != packageUseFlags.end())
								{
									useFlag = packageUseFlagIter->second;
									printf("Error in file %s at line %d: USE-flag %s for atom %s set to state %s already set in file %s to state %s\n",location.c_str(), line, (*i).c_str(), atom.atom_and_slot_and_subslot().c_str(), enabled?("Enabled"):("Disabled"), packageUseFlagIter->second.getLocation().c_str(), packageUseFlagIter->second.getLastValue()?("Enabled"):("Disabled"));
								}

								int linelen;
								linelen = snprintf(NULL, 0, "%d", line);
								if (linelen > 0)
								{
									char linebuf[linelen + 1];
									int res;

									res = snprintf(linebuf, linelen + 1, "%d", line);

									if (res == linelen)
									{
										useFlag.setLocation(location + std::string(" at line ") + std::string(linebuf));
									}
									else
									{
										useFlag.setLocation(location);
									}
								}
								else
								{
									useFlag.setLocation(location);
								}

								if (enabled)
								{
									useFlag.setEnabled(useFlag.getEnabled()+1);
								}
								else
								{
									useFlag.setDisabled(useFlag.getDisabled()+1);
								}

								// Check if uses query was cached for this flag
								if (packagesFlagsList.find(atom.atom_and_slot_and_subslot()) == packagesFlagsList.end())
								{
									if (search_all)
									{
										std::string query = std::string("equery uses -a ") + atom.atom_and_slot_and_subslot();

										run_query(query, atom.atom_and_slot_and_subslot(), atom, packagesFlagsList);
									}
									else
									{
										std::set<Atom> installed_atoms = atom.get_all_installed_packages();
										bool found_installed = false;

										if (!installed_atoms.empty())
										{
											std::set<Atom>::const_iterator atom_end = installed_atoms.end();
											for (std::set<Atom>::const_iterator atom_iter = installed_atoms.begin(); atom_iter != atom_end; ++atom_iter)
											{
												if (((!atom.slot().empty()) && (atom_iter->slot() != atom.slot()))
													|| ((!atom.subslot().empty()) && (atom_iter->subslot() != atom.subslot())))
												{
													continue;
												}

												std::stringstream querystream;
												std::stringstream queryatomstream;

												queryatomstream << "=" << atom_iter->atom() << "-" << atom_iter->version();

												if (!atom_iter->slot().empty())
												{
													queryatomstream << ":" << atom_iter->slot();

													if (!atom_iter->subslot().empty())
													{
														queryatomstream << "/" << atom_iter->subslot();
													}
												}

												querystream << "equery uses " << queryatomstream.str();
												run_query(querystream.str(), queryatomstream.str(), *atom_iter, packagesFlagsList);
												found_installed = true;
											}
										}

										if (!found_installed)
										{
											std::string query = std::string("equery uses ") + atom.atom_and_slot_and_subslot();

											run_query(query, atom.atom_and_slot_and_subslot(), atom, packagesFlagsList);
										}
									}
								}

								if (packagesFlagsList[atom.atom_and_slot_and_subslot()].find(*i) == packagesFlagsList[atom.atom_and_slot_and_subslot()].end())
								{
									printf("Error in file %s at line %d: USE-flag %s doesn't exist for atom %s\n",location.c_str(),line,(*i).c_str(),atom.atom_and_slot_and_subslot().c_str());
								}

								packageUseFlags[std::make_pair((*i), atom)] = useFlag;
							}
							else
							{
								printf("Error in file %s at line %d: invalid USE-flag %s\n", location.c_str(), line, (*i).c_str());
							}
						}

						if (!got_flag)
						{
							printf("Error in file %s at line %d: atom %s doesn't contain any USE-flags\n",location.c_str(),line, atom.atom_and_slot_and_subslot().c_str());
						}
					}
					else
					{
						printf("Error in file %s at line %d: invalid or formated unsupported way atom %s\n", location.c_str(), line, data.at(0).c_str());
					}
				}

				data.clear();
				++line;
			} while (result == parse_result_eol);
		}
		catch (...)
		{
			fclose(f);
			throw;
		}

		fclose(f);

		if (file_is_empty)
		{
			printf("Error in file %s: file is empty\n", location.c_str());
		}
	}
	else
	{
		printf("Error opening file %s\n", location.c_str());
	}
}

int check_main_use_file(const std::string &location, const std::set<std::string> &filelist, std::map<std::string, UseFlag>& useFlags)
{
	printf("Parsing:           %s...\n", location.c_str());

	std::stringstream query_stream;
	query_stream << "/bin/bash -c '";

	std::set<std::string>::const_iterator list_end = filelist.end();
	for (std::set<std::string>::const_iterator list_item = filelist.begin(); list_item != list_end; ++list_item)
	{
		query_stream << "source " << *list_item << "; ";
	}

	query_stream << "echo $USE'";

	FILE *make_pipe = popen(query_stream.str().c_str(), "r");

	if (make_pipe != NULL)
	{
		try
		{
			std::string flag;
			int c;

			for (;;)
			{
				c = fgetc(make_pipe);

				if ((c != EOF) && (!isspace(c)))
				{
					flag.append(1, c);
				}
				else
				{
					if (flag.length() > 0)
					{
						bool flag_valid;

						flag_valid = check_use_name(flag);

						if (flag_valid)
						{
							bool flag_plus = true;

							if (flag[0] == '-')
							{
								flag_plus = false;
								flag.erase(0, 1);
							}

							UseFlag useFlag;

							std::map<std::string, UseFlag>::iterator useFlagIter;

							useFlagIter = useFlags.find(flag);

							if (useFlagIter != useFlags.end())
							{
								useFlag = useFlagIter->second;
								printf("Error in file %s: USE-flag %s for set %s already set in file %s to state %s\n", location.c_str(), flag.c_str(), flag_plus?("Enabled"):("Disabled"), useFlagIter->second.getLocation().c_str(), useFlagIter->second.getLastValue()?("Enabled"):("Disabled"));
							}
							else
							{
								if (make_check || full_make_check)
								{
									check_use_flag_existance(flag, location);
								}

								useFlag.setLocation(location);
							}

							if (flag_plus)
							{
								useFlag.setEnabled(useFlag.getEnabled()+1);
							}
							else
							{
								useFlag.setDisabled(useFlag.getDisabled()+1);
							}

							useFlags[flag] = useFlag;
						}
						else
						{
							printf("Invalid USE-flag \"%s\" in %s\n", flag.c_str(), location.c_str());
						}

						flag.clear();
					}
				}

				if (c == EOF)
				{
					break;
				}
			}
		}
		catch (...)
		{
			pclose(make_pipe);
			throw;
		}

		pclose(make_pipe);
	}
	else
	{
		printf("Error sourcing %s, errno %d\n", location.c_str(), errno);
	}

	return 0;
}

int check_use_flags()
{
	std::set<std::string> filelist;
	std::map<std::string, UseFlag> useFlags;
	std::map<std::pair<std::string, Atom>, UseFlag> packageUseFlags;
	std::map<std::string, std::set<std::string> > packagesFlagsList;

	printf("Parsing:           USE-flags...\n");

	filelist = find_all_files("/etc/make.conf");
	if (!filelist.empty())
	{
		check_main_use_file("/etc/make.conf", filelist, useFlags);
	}

	filelist = find_all_files("/etc/portage/make.conf");
	if (!filelist.empty())
	{
		check_main_use_file("/etc/portage/make.conf", filelist, useFlags);
	}

	filelist = find_all_files("/etc/portage/package.use");

	printf("Processing:        /etc/portage/package.use\n");

	std::set<std::string>::const_iterator list_end = filelist.end();
	for (std::set<std::string>::const_iterator list_item = filelist.begin(); list_item != list_end; ++list_item)
	{
		printf("Processing file:   %s\n", list_item->c_str());
		check_use_file(*list_item, useFlags, packageUseFlags, packagesFlagsList);
	}

	return 0;
}

void check_aux_file(const std::string &location, std::map<std::string, std::pair<std::string, int> >& atoms, bool check)
{
	FILE *f = fopen(location.c_str(), "r");
	if (f != NULL)
	{
		bool file_is_empty = true;

		try
		{
			int result;
			unsigned int line = 1;
			std::vector<std::string> data;

			do
			{
				result = parseFileLine(f, data);

				if (!data.empty())
				{
					file_is_empty = false;
					Atom atom(data.at(0));
					bool valid = atom.is_valid()
						&& (((atom.vop() == Atom::version_none) && (atom.version().empty()))
							|| ((atom.vop() != Atom::version_none) && (!atom.version().empty())));

					if (valid)
					{
						if (check)
						{
							valid = atom.check_installed();

							if (!valid)
							{
								printf("Error in file %s at line %d: atom %s is not installed\n", location.c_str(), line, atom.full_atom().c_str());
							}
						}

						std::map<std::string, std::pair<std::string, int> >::iterator AtomIter;

						AtomIter = atoms.find(atom.atom_and_slot_and_subslot());

						if (AtomIter != atoms.end())
						{
							printf("Warning in file %s at line %d: atom %s already set in file %s at line %d\n",location.c_str(), line, atom.atom_and_slot_and_subslot().c_str(),  AtomIter->second.first.c_str(), AtomIter->second.second);
						}

						atoms[atom.atom_and_slot_and_subslot()] = std::make_pair(location, line);
					}
					else
					{
						printf("Error in file %s at line %d: invalid or formated unsupported way atom %s\n", location.c_str(), line, data.at(0).c_str());
					}
				}

				data.clear();
				++line;
			} while (result == parse_result_eol);
		}
		catch (...)
		{
			fclose(f);
			throw;
		}

		fclose(f);

		if (file_is_empty)
		{
			printf("Error in file %s: file is empty\n", location.c_str());
		}
	}
	else
	{
		printf("Error opening file %s\n", location.c_str());
	}
}

int check_existance(const std::string &path, bool check = true)
{
	std::set<std::string> filelist;
	std::map<std::string, std::pair<std::string, int> > locations;

	printf("Checking:          %s\n", path.c_str());

	filelist = find_all_files(path);

	std::set<std::string>::const_iterator list_end = filelist.end();
	for (std::set<std::string>::const_iterator list_item = filelist.begin(); list_item != list_end; ++list_item)
	{
		printf("Processing file:   %s\n", list_item->c_str());
		check_aux_file(*list_item, locations, check);
	}

	return 0;
}

/*
 * TODO: tree of atoms: each atom can have one or more parents with some values.
 * If it is more than one parent, give warning.
 * If atom duplicates parent, give warning
 * Do same for package mask/unmask
 */

int main(int argc, char **argv)
{
	for (int i = 1; i < argc; ++i)
	{
		if (strcmp(argv[i],"--help") == 0)
		{
			printf("Options:\n"
					"\t--help - shows this info\n"
					"\t--make-check - quick make.conf USE-flags existance check\n"
					"\t--full-make-check - full make.conf USE-flags existance check\n"
					"\t\tWarning: may take very long time,\n"
					"\t\tbecause it checks whether any package has this USE-flag,\n"
					"\t\teven if the package itself is not installed\n"
					"\t-a - check all use flags, not only for current installed packages\n"
					"\t\twhen searching for obsolete packages\n"
					"\t--skip-uses - skip USE-flags check\n"
					"\t--skip-else - skip other checks:\n"
					"\t\tskips atom existance checks in other portage config files, like package.keywords\n"
				);

			return 0;
		}
		else if (strcmp(argv[i],"--make-check") == 0)
		{
			make_check = true;
		}
		else if (strcmp(argv[i],"--full-make-check") == 0)
		{
			full_make_check = true;
		}
		else if (strcmp(argv[i],"-a") == 0)
		{
			search_all = true;
		}
		else if (strcmp(argv[i],"--skip-uses") == 0)
		{
			skip_uses = true;
		}
		else if (strcmp(argv[i],"--skip-else") == 0)
		{
			skip_else = true;
		}
		else
		{
			printf("Unknown option: %s, try %s --help for more information\n", argv[i], argv[0]);
			return 0;
		}
	}

	if (skip_uses && skip_else)
	{
		printf("Error: both --skip-uses and --skip-else activated\n");
		return 0;
	}

	if (!skip_uses)
	{
		check_use_flags();
	}

	if (!skip_else)
	{
		if (!skip_uses)
		{
			printf("\n");
		}

		check_existance("/etc/portage/package.env");
		printf("\n");
		check_existance("/etc/portage/package.keywords");
		printf("\n");
		check_existance("/etc/portage/package.license");
		printf("\n");
		check_existance("/etc/portage/package.mask", false);
		printf("\n");
		check_existance("/etc/portage/package.provided", false);
		printf("\n");
		check_existance("/etc/portage/package.unmask");
	}

	return 0;
}
