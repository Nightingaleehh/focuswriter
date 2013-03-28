/***********************************************************************
 *
 * Copyright (C) 2012, 2013 Graeme Gott <graeme@gottcode.org>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 ***********************************************************************/

#include "dictionary_ref.h"

#include "../abstract_dictionary.h"

#include <QStringRef>
#include <QStringList>

//-----------------------------------------------------------------------------

QStringRef DictionaryRef::check(const QString& string, int start_at) const
{
	return d ? (*d)->check(string, start_at) : QStringRef();
}

//-----------------------------------------------------------------------------

QStringList DictionaryRef::suggestions(const QString& word) const
{
	return d ? (*d)->suggestions(word) : QStringList();
}

//-----------------------------------------------------------------------------

void DictionaryRef::addToPersonal(const QString& word)
{
	if (d) {
		(*d)->addToPersonal(word);
	}
}

//-----------------------------------------------------------------------------
