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

private:
	unsigned char m_enabled;
	unsigned char m_disabled;
	bool m_last_value;
};

UseFlag::UseFlag()
	: m_last_value(true),
	m_enabled(0),
	m_disabled(0)
{
}

UseFlag& UseFlag::operator=(const UseFlag& useFlag)
{
	this->m_enabled    = useFlag.m_enabled;
	this->m_disabled   = useFlag.m_disabled;
	this->m_last_value = useFlag.m_last_value;

	return *this;
}

int check_use_flags()
{
	std::map<std::string, UseFlag> useFlags;
	useFlags.clear();

	bool flag_valid;

	printf("Parsing USE-flags...\n");

	printf("Parsing /etc/make.conf...\n");

	FILE *make_pipe = popen("/bin/bash -c 'source /etc/make.conf && echo $USE'", "r");
	if (make_pipe != NULL)
	{
		std::string flag;
		flag.clear();

		int c;
		while ((c = fgetc(make_pipe)) != EOF)
		{
			if (!isspace(c))
			{
				flag.append(1, c);
			}
			else
			{
				if (flag.length() > 0)
				{
					flag_valid = true;

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

	return 0;
}

int main(int argc, char **argv)
{
	check_use_flags();

	return 0;
}
