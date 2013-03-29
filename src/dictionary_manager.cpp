/***********************************************************************
 *
 * Copyright (C) 2009, 2010, 2011, 2012, 2013 Graeme Gott <graeme@gottcode.org>
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

#include "dictionary_manager.h"

#ifndef Q_OS_MAC
#include "hunspell/dictionary_provider_hunspell.h"
#include "voikko/dictionary_provider_voikko.h"
#else
#include "nsspellchecker/dictionary_provider_nsspellchecker.h"
#endif
#include "dictionary_ref.h"
#include "smart_quotes.h"

#include <QDir>
#include <QFile>
#include <QTextStream>

//-----------------------------------------------------------------------------

namespace
{
	bool compareWords(const QString& s1, const QString& s2)
	{
		return s1.localeAwareCompare(s2) < 0;
	}
}

QString DictionaryManager::m_path;

//-----------------------------------------------------------------------------

DictionaryManager& DictionaryManager::instance()
{
	static DictionaryManager manager;
	return manager;
}

//-----------------------------------------------------------------------------

QStringList DictionaryManager::availableDictionaries() const
{
	QStringList result;
	foreach (AbstractDictionaryProvider* provider, m_providers) {
		result += provider->availableDictionaries();
	}
	result.sort();
	result.removeDuplicates();
	return result;
}

//-----------------------------------------------------------------------------

void DictionaryManager::add(const QString& word)
{
	QStringList words = personal();
	if (words.contains(SmartQuotes::revert(word))) {
		return;
	}
	words.append(word);
	setPersonal(words);
}

//-----------------------------------------------------------------------------

DictionaryRef DictionaryManager::requestDictionary(const QString& language)
{
	if (language.isEmpty()) {
		// Fetch shared default dictionary
		if (!m_default_dictionary) {
			m_default_dictionary = *requestDictionaryData(m_default_language);
		}
		return &m_default_dictionary;
	} else {
		// Fetch specific dictionary
		return requestDictionaryData(language);
	}
}

//-----------------------------------------------------------------------------

void DictionaryManager::setDefaultLanguage(const QString& language)
{
	if (language == m_default_language) {
		return;
	}

	m_default_language = language;
	m_default_dictionary = *requestDictionaryData(m_default_language);

	// Re-check documents
	emit changed();
}

//-----------------------------------------------------------------------------

void DictionaryManager::setIgnoreNumbers(bool ignore)
{
	foreach (AbstractDictionaryProvider* provider, m_providers) {
		provider->setIgnoreNumbers(ignore);
	}

	// Re-check documents
	emit changed();
}

//-----------------------------------------------------------------------------

void DictionaryManager::setIgnoreUppercase(bool ignore)
{
	foreach (AbstractDictionaryProvider* provider, m_providers) {
		provider->setIgnoreUppercase(ignore);
	}

	// Re-check documents
	emit changed();
}

//-----------------------------------------------------------------------------

QString DictionaryManager::installedPath()
{
#ifndef Q_OS_MAC
	return m_path;
#else
	return QDir::homePath() + "/Library/Spelling/";
#endif
}

//-----------------------------------------------------------------------------

void DictionaryManager::setPath(const QString& path)
{
	m_path = path;
}

//-----------------------------------------------------------------------------

void DictionaryManager::setPersonal(const QStringList& words)
{
	// Check if new
	QStringList personal = SmartQuotes::revert(words);
	qSort(personal.begin(), personal.end(), compareWords);
	if (personal == m_personal) {
		return;
	}

	// Remove current personal dictionary
	foreach (AbstractDictionary* dictionary, m_dictionaries) {
		dictionary->removeFromSession(m_personal);
	}

	// Update and store personal dictionary
	m_personal = personal;
	QFile file(m_path + "/personal");
	if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
		QTextStream stream(&file);
		stream.setCodec("UTF-8");
		foreach (const QString& word, m_personal) {
			stream << word << "\n";
		}
	}

	// Add personal dictionary
	foreach (AbstractDictionary* dictionary, m_dictionaries) {
		dictionary->addToSession(m_personal);
	}

	// Re-check documents
	emit changed();
}

//-----------------------------------------------------------------------------

DictionaryManager::DictionaryManager()
{
#ifndef Q_OS_MAC
	addProvider(new DictionaryProviderHunspell);
	addProvider(new DictionaryProviderVoikko);
#else
	addProvider(new DictionaryProviderNSSpellChecker);
#endif

	// Load personal dictionary
	QFile file(m_path + "/personal");
	if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
		QTextStream stream(&file);
		stream.setCodec("UTF-8");
		while (!stream.atEnd()) {
			m_personal.append(stream.readLine());
		}
		qSort(m_personal.begin(), m_personal.end(), compareWords);
	}
}

//-----------------------------------------------------------------------------

DictionaryManager::~DictionaryManager()
{
	foreach (AbstractDictionary* dictionary, m_dictionaries) {
		delete dictionary;
	}
	m_dictionaries.clear();

	qDeleteAll(m_providers);
	m_providers.clear();
}

//-----------------------------------------------------------------------------

void DictionaryManager::addProvider(AbstractDictionaryProvider* provider)
{
	if (provider->isValid()) {
		m_providers.append(provider);
	} else {
		delete provider;
		provider = 0;
	}
}

//-----------------------------------------------------------------------------

AbstractDictionary** DictionaryManager::requestDictionaryData(const QString& language)
{
	if (!m_dictionaries.contains(language)) {
		AbstractDictionary* dictionary = 0;
		foreach (AbstractDictionaryProvider* provider, m_providers) {
			dictionary = provider->requestDictionary(language);
			if (dictionary && dictionary->isValid()) {
				break;
			} else {
				delete dictionary;
				dictionary = 0;
			}
		}

		if (!dictionary) {
			return 0;
		}
		dictionary->addToSession(m_personal);
		m_dictionaries[language] = dictionary;
	}
	return &m_dictionaries[language];
}

//-----------------------------------------------------------------------------
