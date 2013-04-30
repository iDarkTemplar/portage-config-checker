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

#include "use_flag.h"

#define min(a, b) ((a) < (b))?(a):(b)

const unsigned char UseFlag::MAX_VALUES = 100;

UseFlag::UseFlag()
	: m_enabled(0),
	m_disabled(0),
	m_last_value(true)
{
}

UseFlag::~UseFlag()
{
}

unsigned char UseFlag::getEnabled()
{
	return m_enabled;
}

void UseFlag::setEnabled(unsigned char value)
{
	m_enabled = min(value, MAX_VALUES);
	m_last_value = true;
}

unsigned char UseFlag::getDisabled()
{
	return m_disabled;
}

void UseFlag::setDisabled(unsigned char value)
{
	m_disabled = min(value, MAX_VALUES);
	m_last_value = false;
}

unsigned char UseFlag::getTotalCount()
{
	return m_enabled + m_disabled;
}

bool UseFlag::getLastValue()
{
	return m_last_value;
}

void UseFlag::setLastValue(bool value)
{
	m_last_value = value;
}

UseFlag& UseFlag::operator=(const UseFlag &useFlag)
{
	this->m_enabled    = useFlag.m_enabled;
	this->m_disabled   = useFlag.m_disabled;
	this->m_last_value = useFlag.m_last_value;
	this->m_location   = useFlag.m_location;

	return *this;
}

std::string UseFlag::getLocation()
{
	return m_location;
}

void UseFlag::setLocation(std::string value)
{
	m_location = value;
}
