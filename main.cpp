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

#include <boost/regex.hpp>

bool search_all = false;
bool skip_uses = false;
bool skip_else = false;

bool check_use_name(const std::string& flag)
{
	boost::regex reg_expr("[\\-]?[[:alnum:]]+(?:[\\-_][[:alnum:]]+)*");
	//boost::regex reg_expr("^([>|>=|=|<=|<]?)([[:alnum:]]+(?:[\\-_][[:alnum:]]+)*)/([[:alnum:]]+(?:[\\-_][[:alnum:]]+)*)(?:\\-([[:digit:]]+(?:[\\-_\\.][[:alnum:]]+)*))*(?:\\:([[:digit:]]+(?:[\\-_\\.][[:alnum:]]+)*))*$");

	return boost::regex_match(flag, reg_expr);
}

void work_on_file(std::string location, std::map<std::string, UseFlag>& useFlags, std::map<std::pair<std::string, std::string>, UseFlag>& packageUseFlags, std::map<std::string, std::set<std::string> > &packagesFlagsList)
{
	printf("Processing file:   %s\n", location.c_str());

	FILE *f = fopen(location.c_str(),"r");
	if (f != NULL)
	{
		int result;
		unsigned int line = 1;
		bool got_flag;
		bool file_is_empty = true;
		std::vector<std::string> *data;

		do
		{
			result = parseFileLine(f, &data);

			if ((result != parse_result_eol) && (result != parse_result_eof))
			{
				break;
			}

			if (!data->empty())
			{
				file_is_empty = false;
				Atom atom(data->at(0));
				bool valid = atom.is_valid() && (atom.vop() == Atom::version_none) && (atom.version().size() == 0);

				if (!valid)
				{
					printf("Error in file %s at line %d: invalid atom %s\n", location.c_str(), line, atom.atom_and_slot().c_str());
				}

				valid = atom.check_installed();

				if (!valid)
				{
					printf("Error in file %s at line %d: atom %s is not installed\n", location.c_str(), line, atom.atom_and_slot().c_str());
				}

				got_flag = false;

				for (std::vector<std::string>::iterator i = data->begin() + 1; i != data->end(); ++i)
				{
					valid = check_use_name(*i);

					if (valid)
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
								printf("Error in file %s at line %d: USE-flag %s for atom %s already set to same value in %s\n", location.c_str(), line, (*i).c_str(), atom.atom_and_slot().c_str(), useFlagIter->second.getLocation().c_str());
							}
						}

						// same flag in some other files, then warning
						UseFlag useFlag;

						std::map<std::pair<std::string, std::string>, UseFlag>::iterator packageUseFlagIter;
						packageUseFlagIter = packageUseFlags.find(std::make_pair((*i), atom.atom_and_slot()));

						if (packageUseFlagIter != packageUseFlags.end())
						{
							useFlag = packageUseFlagIter->second;
							printf("Error in file %s at line %d: USE-flag %s for atom %s set %s already set in file %s to state %s\n",location.c_str(), line, (*i).c_str(), atom.atom_and_slot().c_str(), enabled?("Enabled"):("Disabled"), packageUseFlagIter->second.getLocation().c_str(), packageUseFlagIter->second.getLastValue()?("Enabled"):("Disabled"));
						}

						char linebuf[100];
						int res;
						res = snprintf(linebuf,sizeof(linebuf)-1,"%d",line);

						if (res > 0)
						{
							linebuf[res] = 0;
							useFlag.setLocation(location + std::string(" at line ") + std::string(linebuf));
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
						if (packagesFlagsList.find(atom.atom_and_slot()) == packagesFlagsList.end())
						{
							std::string query = std::string("equery uses ") + std::string(search_all?("-a "):("")) + atom.atom_and_slot();

							FILE *make_pipe = popen(query.c_str(), "r");
							if (make_pipe != NULL)
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
										packagesFlagsList[atom.atom_and_slot()].insert(result_string);
										result_string.clear();

										if (c == EOF)
										{
											break;
										}
									}
								}

								pclose(make_pipe);
							}
							else
							{
								printf("Couldn't query USE-flags for atom %s\n",atom.atom_and_slot().c_str());
							}
						}

						if (packagesFlagsList[atom.atom_and_slot()].find(*i) == packagesFlagsList[atom.atom_and_slot()].end())
						{
							printf("Error in file %s at line %d: USE-flag %s doesn't exist for atom %s\n",location.c_str(),line,(*i).c_str(),atom.atom_and_slot().c_str());
						}

						packageUseFlags[std::make_pair((*i), atom.atom_and_slot())] = useFlag;
					}
					else
					{
						printf("Error in file %s at line %d: invalid USE-flag %s\n", location.c_str(), line, (*i).c_str());
					}
				}

				if (!got_flag)
				{
					printf("Error in file %s at line %d: atom %s doesn't contain any USE-flags\n",location.c_str(),line, atom.atom_and_slot().c_str());
				}
			}

			delete data;
			++line;

		} while (result == parse_result_eol);

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

void search_entry_rec(std::string location, std::map<std::string, UseFlag>& useFlags, std::map<std::pair<std::string, std::string>, UseFlag>& packageUseFlags, std::map<std::string, std::set<std::string> > &packagesFlagsList)
{
	struct stat buffer;
	int status;

	status = stat(location.c_str(), &buffer);

	if (status != -1)
	{
		if ((buffer.st_mode & __S_IFMT) == __S_IFDIR)
		{
			DIR *dirp;
			struct dirent *dp;

			printf("Opening directory: %s\n", location.c_str());

			if ((dirp = opendir(location.c_str())) == NULL)
			{
				printf("Error: couldn't open directory %s\n", location.c_str());
				return;
			}

			do
			{
				if ((dp = readdir(dirp)) != NULL)
				{
					if ((strcmp(dp->d_name,".") != 0) && (strcmp(dp->d_name,"..") != 0))
					{
						search_entry_rec(location + std::string("/") + std::string(dp->d_name), useFlags, packageUseFlags, packagesFlagsList);
					}
				}
			} while (dp != NULL);

			closedir(dirp);
		}
		else if ((buffer.st_mode & __S_IFMT) == __S_IFREG)
		{
			work_on_file(location, useFlags, packageUseFlags, packagesFlagsList);
		}
		else
		{
			printf("Unsupported %s type %o\n",location.c_str(), buffer.st_mode & __S_IFMT);
		}
	}
	else
	{
		printf("Couldn't stat %s\n", location.c_str());
	}
}

int check_main_use_file(std::string location, std::map<std::string, UseFlag>& useFlags)
{
	printf("Parsing:           %s...\n", location.c_str());

	std::string query = std::string("/bin/bash -c 'source ") + location.c_str() + std::string(" && echo $USE'");

	FILE *make_pipe = popen(query.c_str(), "r");

	query.clear();

	if (make_pipe != NULL)
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
	std::map<std::string, UseFlag> useFlags;
	std::map<std::pair<std::string, std::string>, UseFlag> packageUseFlags;
	std::map<std::string, std::set<std::string> > packagesFlagsList;

	printf("Parsing:           USE-flags...\n");

	check_main_use_file(std::string("/etc/make.conf"), useFlags);
	check_main_use_file(std::string("/etc/portage/make.conf"), useFlags);

	search_entry_rec(std::string("/etc/portage/package.use"), useFlags, packageUseFlags, packagesFlagsList);

	return 0;
}

void work_on_location(std::string location, std::map<std::string, std::pair<std::string, int> >& atoms, bool check)
{
	printf("Processing file:   %s\n", location.c_str());

	FILE *f = fopen(location.c_str(),"r");
	if (f != NULL)
	{
		int result;
		unsigned int line = 1;
		bool file_is_empty = true;
		std::vector<std::string> *data;

		do
		{
			result = parseFileLine(f, &data);

			if ((result != parse_result_eol) && (result != parse_result_eof))
			{
				break;
			}

			if (!data->empty())
			{
				file_is_empty = false;
				Atom atom(data->at(0));
				bool valid = atom.is_valid();

				if (!valid)
				{
					printf("Error in file %s at line %d: invalid atom %s\n", location.c_str(), line, atom.atom_and_slot().c_str());
				}

				if (check)
				{
					valid = atom.check_installed();

					if (!valid)
					{
						printf("Error in file %s at line %d: atom %s is not installed\n", location.c_str(), line, atom.atom_and_slot().c_str());
					}
				}

				std::map<std::string, std::pair<std::string, int> >::iterator AtomIter;

				AtomIter = atoms.find(atom.atom_and_slot());

				if (AtomIter != atoms.end())
				{
					printf("Warning in file %s at line %d: atom %s already set in file %s at line %d\n",location.c_str(), line, atom.atom_and_slot().c_str(),  AtomIter->second.first.c_str(), AtomIter->second.second);
				}

				atoms[atom.atom_and_slot()] = std::make_pair(location, line);
			}

			delete data;
			++line;

		} while (result == parse_result_eol);

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

void search_existance_rec(std::string location, std::map<std::string, std::pair<std::string, int> >& atoms, bool check)
{
	struct stat buffer;
	int status;

	status = stat(location.c_str(), &buffer);

	if (status != -1)
	{
		if ((buffer.st_mode & __S_IFMT) == __S_IFDIR)
		{
			DIR *dirp;
			struct dirent *dp;

			printf("Opening directory: %s\n", location.c_str());

			if ((dirp = opendir(location.c_str())) == NULL)
			{
				printf("Error: couldn't open directory %s\n", location.c_str());
				return;
			}

			do {
				if ((dp = readdir(dirp)) != NULL)
				{
					if ((strcmp(dp->d_name,".") != 0) && (strcmp(dp->d_name,"..") != 0))
					{
						search_existance_rec(location + std::string("/") + std::string(dp->d_name), atoms, check);
					}
				}
			} while (dp != NULL);

			closedir(dirp);
		}
		else if ((buffer.st_mode & __S_IFMT) == __S_IFREG)
		{
			work_on_location(location, atoms, check);
		}
		else
		{
			printf("Unsupported %s type %o\n",location.c_str(), buffer.st_mode & __S_IFMT);
		}
	}
	else
	{
		printf("Couldn't stat %s\n", location.c_str());
	}
}

int check_existance(std::string path, bool check = true)
{
	std::map<std::string, std::pair<std::string, int> > locations;

	printf("Checking:          %s\n", path.c_str());

	search_existance_rec(path, locations, check);

	return 0;
}

int main(int argc, char **argv)
{
	for (int i = 1; i < argc; ++i)
	{
		if (strcmp(argv[i],"--help") == 0)
		{
			printf("Options:\n"
					"\t--help - shows this info\n"
					"\t-a - check all use flags, not only for current installed packages\n"
					"\t\twhen searching for obsolete packages\n"
					"\t--skip-uses - skip USE-flags check\n"
					"\t--skip-else - skip other checks\n"
				);

			return 0;
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
