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

#ifndef DT_USE_FLAG_H
#define DT_USE_FLAG_H

#include <string>

class UseFlag
{
public:
	static const unsigned char MAX_VALUES;

	UseFlag();
	~UseFlag();

	unsigned char getEnabled() const;
	void setEnabled(unsigned char value);

	unsigned char getDisabled() const;
	void setDisabled(unsigned char value);

	unsigned char getTotalCount() const;
	bool getLastValue() const;
	void setLastValue(bool value);

	UseFlag& operator=(const UseFlag &useFlag);

	const std::string& getLocation() const;
	void setLocation(const std::string &value);

private:
	unsigned char m_enabled;
	unsigned char m_disabled;
	bool m_last_value;
	std::string m_location;
};

#endif // DT_USE_FLAG_H
