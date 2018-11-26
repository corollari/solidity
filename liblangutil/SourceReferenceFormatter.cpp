/*
	This file is part of solidity.

	solidity is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	solidity is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with solidity.  If not, see <http://www.gnu.org/licenses/>.
*/
/**
 * @author Christian <c@ethdev.com>
 * @date 2014
 * Formatting functions for errors referencing positions and locations in the source.
 */

#include <liblangutil/SourceReferenceFormatter.h>
#include <liblangutil/Scanner.h>
#include <liblangutil/Exceptions.h>
#include <libdevcore/termcolor.h>
#include <cmath>
#include <iomanip>

using namespace std;
using namespace dev;
using namespace langutil;

namespace
{
inline ostream& frameColor(ostream& os) { return os << termcolor::bold << termcolor::blue; };
inline ostream& messageColor(ostream& os) { return os << termcolor::bold << termcolor::white; };
inline ostream& errorColor(ostream& os) { return os << termcolor::bold << termcolor::red; };
inline ostream& diagColor(ostream& os) { return os << termcolor::bold << termcolor::yellow; };
inline ostream& highlightColor(ostream& os) { return os << termcolor::yellow; };
}

void SourceReferenceFormatter::printSourceLocation(SourceLocation const* _location, std::string const& _msg)
{
	if (!_location || !_location->sourceName)
		return; // Nothing we can print here

	auto const& scanner = m_scannerFromSourceName(*_location->sourceName);

	int startLine;
	int startColumn;
	tie(startLine, startColumn) = scanner.translatePositionToLineColumn(_location->start);

	int endLine;
	int endColumn;
	tie(endLine, endColumn) = scanner.translatePositionToLineColumn(_location->end);

	int const leftpad = static_cast<int>(log10(startLine)) + 1;

	printSourceName(_location, leftpad);
	if (!_msg.empty())
		m_stream << messageColor << _msg << termcolor::reset << '\n';
	else
		m_stream << '\n';

	string line = scanner.lineAtPosition(_location->start);

	int locationLength = endColumn - startColumn;
	if (locationLength > 150)
	{
		line = line.substr(0, startColumn + 35) + " ... " + line.substr(endColumn - 35);
		endColumn = startColumn + 75;
		locationLength = 75;
	}
	if (line.length() > 150)
	{
		int len = line.length();
		line = line.substr(max(0, startColumn - 35), min(startColumn, 35) + min(locationLength + 35, len - startColumn));
		if (startColumn + locationLength + 35 < len)
			line += " ...";
		if (startColumn > 35)
		{
			line = " ... " + line;
			startColumn = 40;
		}
		endColumn = startColumn + locationLength;
	}

	if (startLine == endLine)
	{
		// line 1:
		m_stream << string(leftpad, ' ') << frameColor << " |" << termcolor::reset << '\n';

		// line 2:
		m_stream << frameColor << startLine << " | " << termcolor::reset
				 << line.substr(0, startColumn)
				 << highlightColor << line.substr(startColumn, locationLength) << termcolor::reset
				 << line.substr(endColumn) << '\n';

		// line 3:
		m_stream << string(leftpad, ' ') << frameColor << " | " << termcolor::reset;
		for_each(
			line.cbegin(),
			line.cbegin() + startColumn,
			[this](char const& ch) { m_stream << (ch == '\t' ? '\t' : ' '); }
		);
		m_stream << diagColor << string(locationLength, '^') << termcolor::reset << '\n';
	}
	else
	{
		// line 1:
		m_stream << string(leftpad, ' ') << frameColor << " |" << termcolor::reset << '\n';

		// line 2:
		m_stream << frameColor << startLine << " | " << termcolor::reset
				 << line.substr(0, startColumn)
				 << highlightColor << line.substr(startColumn) << termcolor::reset << '\n';

		// line 3:
		m_stream << string(leftpad, ' ') << frameColor << " | " << termcolor::reset
				 << string(startColumn, ' ')
				 << diagColor << "^ (Relevant source part starts here and spans across multiple lines)."
				 << termcolor::reset << '\n';
	}

	m_stream << '\n';
}

void SourceReferenceFormatter::printSourceName(SourceLocation const* _location, int leftpad)
{
	if (!_location || !_location->sourceName)
		return; // Nothing we can print here

	auto const& scanner = m_scannerFromSourceName(*_location->sourceName);
	int startLine;
	int startColumn;
	tie(startLine, startColumn) = scanner.translatePositionToLineColumn(_location->start);
	m_stream << string(leftpad, ' ') << frameColor << "--> " << termcolor::reset;
	m_stream << *_location->sourceName << ":" << (startLine + 1) << ":" << (startColumn + 1) << ": ";
}

void SourceReferenceFormatter::printExceptionInformation(
	dev::Exception const& _exception,
	string const& _name
)
{
	SourceLocation const* location = boost::get_error_info<errinfo_sourceLocation>(_exception);
	auto secondarylocation = boost::get_error_info<errinfo_secondarySourceLocation>(_exception);

	// exception header line
	m_stream << errorColor << _name;
	if (string const* description = boost::get_error_info<errinfo_comment>(_exception))
		m_stream << messageColor << ": " << *description << endl;
	else
		m_stream << termcolor::reset << '\n';

	if (!location || !location->sourceName)
		m_stream << '\n';

	printSourceLocation(location);

	if (secondarylocation && !secondarylocation->infos.empty())
		for (auto info: secondarylocation->infos)
			printSourceLocation(&info.second, info.first);
}
