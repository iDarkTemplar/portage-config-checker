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

#include "read_file.h"
#include "use_flag.h"

bool search_all = false;

bool check_atom_name(const std::string& atom)
{

	for (int i = 0; i < atom.length(); ++i)
	{
		if ((atom[i] != '-') && (atom[i] != '_') && (atom[i] != '/') && (!isalnum(atom[i])))
		{
			return false;
		}
	}

	int count = 0;
	int pos = -1;

	while ((pos = atom.find('/', pos+1)) != atom.npos)
	{
		++count;
	}

	if (count != 1)
	{
		return false;
	}

	pos = -1;

	while ((pos = atom.find('-', pos+1)) != atom.npos)
	{
		if ((pos > 0) && (atom[pos-1] == '-'))
		{
			return false;
		}
	}

	pos = -1;

	while ((pos = atom.find('_', pos+1)) != atom.npos)
	{
		if ((pos > 0) && (atom[pos-1] == '_'))
		{
			return false;
		}
	}

	return true;
}

bool check_atom_installed(const std::string& atom)
{
	bool result = false;

	std::string pkg_name = atom;
	pkg_name.erase(0, pkg_name.rfind('/'));

	if (pkg_name.length() < 2)
	{
		return false;
	}

	pkg_name.erase(0, 1);

	std::string pkg_path = std::string("/var/db/pkg/") + atom;

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
			if (strncasecmp(pkg_name.c_str(), namelist[n]->d_name, pkg_name.length()) == 0)
			{
				result = true;
			}

			free(namelist[n]);
		}

		free(namelist);
	}

	free(path);

	return result;
}

bool check_use_name(const std::string& flag)
{
	for (int i = 0; i < flag.length(); ++i)
	{
		if ((flag[i] != '-') && (!isalnum(flag[i])))
		{
			return false;
		}
	}

	int pos = -1;

	while ((pos = flag.find('-', pos+1)) != flag.npos)
	{
		if ((pos > 0) && (flag[pos-1] == '-'))
		{
			return false;
		}
	}

	if ((flag[0] == '-') && (flag.length() == 1))
	{
		return false;
	}

	return true;
}

void work_on_file(std::string location, std::map<std::string, UseFlag>& useFlags, std::map<std::pair<std::string, std::string>, UseFlag>& packageUseFlags, std::map<std::string, std::set<std::string> > &packagesFlagsList);
void search_entry_rec(std::string location, std::map<std::string, UseFlag>& useFlags, std::map<std::pair<std::string, std::string>, UseFlag>& packageUseFlags, std::map<std::string, std::set<std::string> > &packagesFlagsList);

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

				bool valid = check_atom_name(data->at(0));

				if (!valid)
				{
					printf("Error in file %s at line %d: invalid atom %s\n", location.c_str(), line, data->at(0).c_str());
				}

				valid = check_atom_installed(data->at(0));

				if (!valid)
				{
					printf("Error in file %s at line %d: atom %s is not installed\n", location.c_str(), line, data->at(0).c_str());
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
								printf("Error in file %s at line %d: USE-flag %s for atom %s already set to same value in %s\n", location.c_str(), line, (*i).c_str(), data->at(0).c_str(), useFlagIter->second.getLocation().c_str());
							}
						}

						// same flag in some other files, then warning
						UseFlag useFlag;

						std::map<std::pair<std::string, std::string>, UseFlag>::iterator packageUseFlagIter;
						packageUseFlagIter = packageUseFlags.find(std::make_pair((*i), data->at(0)));

						if (packageUseFlagIter != packageUseFlags.end())
						{
							useFlag = packageUseFlagIter->second;
							printf("Error in file %s at line %d: USE-flag %s for atom %s set %s already set in file %s to state %s\n",location.c_str(), line, (*i).c_str(), data->at(0).c_str(), enabled?("Enabled"):("Disabled"), packageUseFlagIter->second.getLocation().c_str(), packageUseFlagIter->second.getLastValue()?("Enabled"):("Disabled"));
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
						if (packagesFlagsList.find(data->at(0)) == packagesFlagsList.end())
						{
							std::string query = std::string("equery uses ") + std::string(search_all?("-a "):("")) + data->at(0);

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
										packagesFlagsList[data->at(0)].insert(result_string);
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
								printf("Couldn't query USE-flags for atom %s\n",data->at(0).c_str());
							}
						}

						if (packagesFlagsList[data->at(0)].find(*i) == packagesFlagsList[data->at(0)].end())
						{
							printf("Error in file %s at line %d: USE-flag %s doesn't exist for atom %s\n",location.c_str(),line,(*i).c_str(),data->at(0).c_str());
						}

						packageUseFlags[std::make_pair((*i), data->at(0))] = useFlag;
					}
					else
					{
						printf("Error in file %s at line %d: invalid USE-flag %s\n", location.c_str(), line, (*i).c_str());
					}
				}

				if (!got_flag)
				{
					printf("Error in file %s at line %d: atom %s doesn't contain any USE-flags\n",location.c_str(),line, data->at(0).c_str());
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

			do {
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

int main(int argc, char **argv)
{
	for (int i = 1; i < argc; ++i)
	{
		if (strcmp(argv[i],"--help") == 0)
		{
			printf("Options:\n"
					"\t--help - shows this info\n"
					"\t-a - check all use flags, not only for current installed packages\n"
					"\t\twhen searching for obsolete packages\n");

			return 0;
		}
		else if (strcmp(argv[i],"-a") == 0)
		{
			search_all = true;
		}
		else
		{
			printf("Unknown option: %s, try %s --help for more information\n", argv[i], argv[0]);
			return 0;
		}
	}

	check_use_flags();

	return 0;
}
