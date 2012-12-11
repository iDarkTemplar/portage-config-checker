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

#include <sys/stat.h>
#include <unistd.h>

#include <string>
#include <map>

#include <errno.h>

bool search_all = false;

#define min(a, b) ((a) < (b))?(a):(b)

class UseFlag
{
public:
	static const unsigned char MAX_VALUES = 100;
	UseFlag();

	unsigned char getEnabled() { return m_enabled; }
	void setEnabled(unsigned char value) { m_enabled = min(value, MAX_VALUES); m_last_value = true; }

	unsigned char getDisabled() { return m_disabled; }
	void setDisabled(unsigned char value) { m_disabled = min(value, MAX_VALUES); m_last_value = false; }

	unsigned char getTotalCount() { return m_enabled + m_disabled; }
	bool getLastValue() { return m_last_value; }
	void setLastValue(bool value) { m_last_value = value;}

	UseFlag& operator=(const UseFlag& useFlag);

	std::string getLocation() { return m_location; }
	void setLocation(std::string value) { m_location = value; }

private:
	unsigned char m_enabled;
	unsigned char m_disabled;
	bool m_last_value;
	std::string m_location;
};

UseFlag::UseFlag()
	: m_last_value(true),
	m_enabled(0),
	m_disabled(0),
	m_location()
{
}

UseFlag& UseFlag::operator=(const UseFlag& useFlag)
{
	this->m_enabled    = useFlag.m_enabled;
	this->m_disabled   = useFlag.m_disabled;
	this->m_last_value = useFlag.m_last_value;
	this->m_location   = useFlag.m_location;

	return *this;
}

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

void work_on_file(std::string location, std::map<std::string, UseFlag>& useFlags, std::map<std::pair<std::string, std::string>, UseFlag>& packageUseFlags);
void search_entry_rec(std::string location, std::map<std::string, UseFlag>& useFlags, std::map<std::pair<std::string, std::string>, UseFlag>& packageUseFlags);

void work_on_file(std::string location, std::map<std::string, UseFlag>& useFlags, std::map<std::pair<std::string, std::string>, UseFlag>& packageUseFlags)
{
	printf("Processing file:   %s\n", location.c_str());

	FILE *f = fopen(location.c_str(),"r");
	if (f != NULL)
	{
		int c;
		bool comment = false;
		std::string packet;
		std::string flag;
		unsigned char stage = 0;
		unsigned int line = 1;
		bool process_value = false;

		for (;;)
		{
			c = fgetc(f);

			if (!comment)
			{
				if ((c != '\n') && (c != EOF) && (!isspace(c)) && (c != '#'))
				{
					if (stage == 0)
					{
						packet.append(1, c);
					}
					else
					{
						flag.append(1, c);
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

				if (stage == 0)
				{
					if (packet.length() > 0)
					{
						stage = 1;

						bool valid = check_atom_name(packet);

						if (!valid)
						{
							printf("Error in file %s at line %d: invalid atom %s\n", location.c_str(), line, packet.c_str());
						}
					}
				}
				else
				{
					if (flag.length() > 0)
					{
						bool valid = check_use_name(flag);

						if (valid)
						{
							bool enabled = true;

							if (flag[0] == '-')
							{
								enabled = false;
								flag.erase(0, 1);
							}

							// if contains same flag in same state in make.conf
							std::map<std::string, UseFlag>::iterator useFlagIter;
							useFlagIter = useFlags.find(flag);

							if (useFlagIter != useFlags.end())
							{
								if (useFlagIter->second.getLastValue() == enabled)
								{
									printf("Error in file %s at line %d: USE-flag %s for atom %s already set with same value in %s\n", location.c_str(), line, flag.c_str(), packet.c_str(), useFlagIter->second.getLocation().c_str());
								}
							}

							// same flag in some other files, then warning
							UseFlag useFlag;

							std::map<std::pair<std::string, std::string>, UseFlag>::iterator packageUseFlagIter;
							packageUseFlagIter = packageUseFlags.find(std::make_pair(flag, packet));

							if (packageUseFlagIter != packageUseFlags.end())
							{
								useFlag = packageUseFlagIter->second;
								printf("Error in file %s at line %d: USE-flag %s for atom %s set %s already set in file %s to state %s\n",location.c_str(), line, flag.c_str(), packet.c_str(), enabled?("Enabled"):("Disabled"), packageUseFlagIter->second.getLocation().c_str(), packageUseFlagIter->second.getLastValue()?("Enabled"):("Disabled"));
							}

							useFlag.setLocation(location/* + std::string(", line: ") + std::string()*/); // TODO: write line

							if (enabled)
							{
								useFlag.setEnabled(useFlag.getEnabled()+1);
							}
							else
							{
								useFlag.setDisabled(useFlag.getDisabled()+1);
							}

							std::string query = std::string("equery uses ") + std::string(search_all?("-a "):("")) + packet + std::string(" | grep ") + flag;

							FILE *make_pipe = popen(query.c_str(), "r");
							if (make_pipe != NULL)
							{
								int c;
								std::string result_string;
								bool valid_flag = false;

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

										if (result_string.compare(flag) == 0)
										{
											valid_flag = true;
										}

										result_string.clear();
										if (c == EOF)
										{
											break;
										}
									}
								}

								fclose(make_pipe);

								if (!valid_flag)
								{
									printf("Error in file %s at line %d: USE-flag %s doesn't exist for atom %s\n",location.c_str(),line,flag.c_str(),packet.c_str());
								}
							}
							else
							{
								printf("Couldn't query USE-flags for atom %s\n",packet.c_str());
							}

							packageUseFlags[std::make_pair(flag, packet)] = useFlag;
						}
						else
						{
							printf("Error in file %s at line %d: invalid USE-flag %s\n", location.c_str(), line, flag.c_str());
						}

						flag.clear();
					}
				}

				if (c == '\n')
				{
					comment = false;
					stage = 0;
					++line;
					packet.clear();
				}

				if (c == EOF)
				{
					break;
				}
			}
		}

		fclose(f);
	}
	else
	{
		printf("Error opening file %s\n", location.c_str());
	}
}

void search_entry_rec(std::string location, std::map<std::string, UseFlag>& useFlags, std::map<std::pair<std::string, std::string>, UseFlag>& packageUseFlags)
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
						search_entry_rec(location + std::string("/") + std::string(dp->d_name), useFlags, packageUseFlags);
					}
				}
			} while (dp != NULL);

			closedir(dirp);
		}
		else if ((buffer.st_mode & __S_IFMT) == __S_IFREG)
		{
			work_on_file(location, useFlags, packageUseFlags);
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

		fclose(make_pipe);
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

	printf("Parsing:           USE-flags...\n");

	check_main_use_file(std::string("/etc/make.conf"), useFlags);
	check_main_use_file(std::string("/etc/portage/make.conf"), useFlags);

	search_entry_rec(std::string("/etc/portage/package.use"), useFlags, packageUseFlags);

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
