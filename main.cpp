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

	return *this;
}

void work_on_file(std::string location, std::map<std::string, UseFlag>& useFlags, std::map<std::pair<std::string, std::string>, UseFlag>& packageUseFlags);
void search_entry_rec(std::string location, std::map<std::string, UseFlag>& useFlags, std::map<std::pair<std::string, std::string>, UseFlag>& packageUseFlags);

void work_on_file(std::string location, std::map<std::string, UseFlag>& useFlags, std::map<std::pair<std::string, std::string>, UseFlag>& packageUseFlags)
{
	// TODO: work on file
	printf("Processing file:   %s\n", location.c_str());

	FILE *f = fopen(location.c_str(),"r");
	if (f != NULL)
	{
		int c;

		for (;;)
		{
			c = fgetc(f);

			if (c == EOF)
			{
				break;
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

int check_use_flags()
{
	std::map<std::string, UseFlag> useFlags;
	useFlags.clear();

	std::map<std::pair<std::string, std::string>, UseFlag> packageUseFlags;
	packageUseFlags.clear();

	printf("Parsing:           USE-flags...\n");
	printf("Parsing:           /etc/make.conf...\n");

	FILE *make_pipe = popen("/bin/bash -c 'source /etc/make.conf && echo $USE'", "r");
	if (make_pipe != NULL)
	{
		std::string flag;
		flag.clear();

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
					bool flag_valid = true;

					if (flag[0] == '-')
					{
						if ((flag.length() == 1) || (!isalpha(flag[1])))
						{
							flag_valid = false;
						}
					}
					else if (!isalpha(flag[0]))
					{
						flag_valid = false;
					}

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
						}
						else
						{
							useFlag.setLocation(std::string("/etc/make.conf"));
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
						printf("Invalid USE-flag \"%s\" in make.conf\n", flag.c_str());
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
		printf("Error sourcing /etc/make.conf, errno %d\n",errno);
	}

	for (std::map<std::string, UseFlag>::iterator useFlagIter = useFlags.begin(); useFlagIter != useFlags.end(); ++useFlagIter)
	{
		UseFlag useFlag = useFlagIter->second;

		unsigned char enabled = useFlag.getEnabled();
		unsigned char disabled = useFlag.getDisabled();
		unsigned char total = useFlag.getTotalCount();

		if (total != 1)
		{
			printf("USE-flag %s set more than once! Last value is: %s\n", useFlagIter->first.c_str(), useFlag.getLastValue()?("Enabled"):("Disabled"));
		}
	}

	search_entry_rec(std::string("/etc/portage/package.use"), useFlags, packageUseFlags);

	return 0;
}

int main(int argc, char **argv)
{
	check_use_flags();

	return 0;
}
