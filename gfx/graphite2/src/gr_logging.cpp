/*  GRAPHITE2 LICENSING

    Copyright 2010, SIL International
    All rights reserved.

    This library is free software; you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as published
    by the Free Software Foundation; either version 2.1 of License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should also have received a copy of the GNU Lesser General Public
    License along with this library in the file named "LICENSE".
    If not, write to the Free Software Foundation, 51 Franklin Street, 
    Suite 500, Boston, MA 02110-1335, USA or visit their web page on the 
    internet at http://www.fsf.org/licenses/lgpl.html.

Alternatively, the contents of this file may be used under the terms of the
Mozilla Public License (http://mozilla.org/MPL) or the GNU General Public
License, as published by the Free Software Foundation, either version 2
of the License or (at your option) any later version.
*/
#include <stdio.h>

#include "graphite2/Log.h"
#include "inc/debug.h"
#include "inc/CharInfo.h"
#include "inc/Slot.h"
#include "inc/Segment.h"

#if defined _WIN32
#include "Windows.h"
#endif

using namespace graphite2;


extern "C" {


bool gr_start_logging(gr_face * face, const char *log_path)
{
	if (!face || !log_path)	return false;

#if !defined GRAPHITE2_NTRACING
	gr_stop_logging(face);
#if defined _WIN32
	int n = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, log_path, -1, 0, 0);
	if (n == 0 || n > MAX_PATH - 12) return false;

	LPWSTR wlog_path = gralloc<WCHAR>(n);
	FILE *log = 0;
	if (wlog_path && MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, log_path, -1, wlog_path, n))
		log = _wfopen(wlog_path, L"wt");

	free(wlog_path);
#else
	FILE *log = fopen(log_path, "wt");
#endif
	if (!log)	return false;

	face->setLogger(log);
	if (!face->logger()) return false;

	*face->logger() << json::array;
	return true;
#else
	return false;
#endif
}

bool graphite_start_logging(FILE * /* log */, GrLogMask /* mask */)
{
//#if !defined GRAPHITE2_NTRACING
//	graphite_stop_logging();
//
//    if (!log) return false;
//
//    dbgout = new json(log);
//    if (!dbgout) return false;
//
//    *dbgout << json::array;
//    return true;
//#else
    return false;
//#endif
}

void gr_stop_logging(gr_face * face)
{
	if (!face)	return;

#if !defined GRAPHITE2_NTRACING
	if (face->logger())
	{
		FILE * log = face->logger()->stream();
		face->setLogger(0);
		fclose(log);
	}
#endif
}

void graphite_stop_logging()
{
//    if (dbgout) delete dbgout;
//    dbgout = 0;
}

} // extern "C"

#if !defined GRAPHITE2_NTRACING

json & graphite2::operator << (json & j, const CharInfo & ci) throw()
{
	return j << json::object
				<< "offset"			<< ci.base()
				<< "unicode"		<< ci.unicodeChar()
				<< "break"			<< ci.breakWeight()
				<< "flags"          << ci.flags()
				<< "slot" << json::flat << json::object
					<< "before"	<< ci.before()
					<< "after"	<< ci.after()
					<< json::close
				<< json::close;
}


json & graphite2::operator << (json & j, const dslot & ds) throw()
{
	assert(ds.first);
	assert(ds.second);
	const Segment & seg = *ds.first;
	const Slot & s = *ds.second;

	j << json::object
		<< "id"				<< objectid(ds)
		<< "gid"			<< s.gid()
		<< "charinfo" << json::flat << json::object
			<< "original"		<< s.original()
			<< "before"			<< s.before()
			<< "after" 			<< s.after()
			<< json::close
		<< "origin"			<< s.origin()
		<< "shift"			<< Position(float(s.getAttr(0, gr_slatShiftX, 0)),
										float(s.getAttr(0, gr_slatShiftY, 0)))
		<< "advance"		<< s.advancePos()
		<< "insert"			<< s.isInsertBefore()
		<< "break"			<< s.getAttr(&seg, gr_slatBreak, 0);
	if (s.just() > 0)
		j << "justification"	<< s.just();
	if (s.getBidiLevel() > 0)
		j << "bidi"		<< s.getBidiLevel();
	if (!s.isBase())
		j << "parent" << json::flat << json::object
			<< "id"				<< objectid(dslot(&seg, s.attachedTo()))
			<< "level"			<< s.getAttr(0, gr_slatAttLevel, 0)
			<< "offset"			<< s.attachOffset()
			<< json::close;
	j << "user" << json::flat << json::array;
	for (int n = 0; n!= seg.numAttrs(); ++n)
		j	<< s.userAttrs()[n];
		j 	<< json::close;
	if (s.firstChild())
	{
		j	<< "children" << json::flat << json::array;
		for (const Slot *c = s.firstChild(); c; c = c->nextSibling())
		    j   << objectid(dslot(&seg, c));
		j		<< json::close;
	}
	return j << json::close;
}


graphite2::objectid::objectid(const dslot & ds) throw()
{
    const Slot * const p = ds.second;
	uint32 s = reinterpret_cast<size_t>(p);
	sprintf(name, "%.4x-%.2x-%.4hx", uint16(s >> 16), uint16(p ? p->userAttrs()[ds.first->silf()->numUser()] : 0), uint16(s));
	name[sizeof name-1] = 0;
}

graphite2::objectid::objectid(const Segment * const p) throw()
{
	uint32 s = reinterpret_cast<size_t>(p);
	sprintf(name, "%.4x-%.2x-%.4hx", uint16(s >> 16), 0, uint16(s));
	name[sizeof name-1] = 0;
}

#endif
