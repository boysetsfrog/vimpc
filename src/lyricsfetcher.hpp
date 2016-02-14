/***************************************************************************
 *   Copyright (C) 2008-2014 by Andrzej Rybczak                            *
 *   electricityispower@gmail.com                                          *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.              *
 ***************************************************************************/

#ifndef LYRICS_FETCHER_H
#define LYRICS_FETCHER_H

#include <curl/curl.h>
#include <string>

namespace Curl
{
	CURLcode perform(std::string &data, const std::string &URL, const std::string &referer = "", bool follow_redirect = false, unsigned timeout = 10);

	std::string escape(const std::string &s);
}

struct LyricsFetcher
{
	typedef std::pair<bool, std::string> Result;

	virtual std::string name() = 0;
	virtual Result fetch(const std::string &artist, const std::string &title);

protected:
	virtual std::string urlTemplate() = 0;
	virtual std::string regex() = 0;

	virtual bool notLyrics(const std::string &) { return false; }
	virtual void postProcess(std::string &data);

	std::vector<std::string> getContent(std::string regex, const std::string &data);

	static const char msgNotFound[];
};

struct LyricwikiFetcher : public LyricsFetcher
{
	virtual std::string name() { return "lyricwiki.com"; }
	virtual Result fetch(const std::string &artist, const std::string &title);

protected:
	virtual std::string urlTemplate() { return "http://lyrics.wikia.com/api.php?action=lyrics&fmt=xml&func=getSong&artist=%artist%&song=%title%"; }
	virtual std::string regex() { return "<url>(.*?)</url>"; }

	virtual bool notLyrics(const std::string &data);
};

/**********************************************************************/

struct GoogleLyricsFetcher : public LyricsFetcher
{
	virtual Result fetch(const std::string &artist, const std::string &title);

protected:
	virtual std::string urlTemplate() { return URL; }
	virtual std::string siteKeyword() { return name(); }

	virtual bool isURLOk(const std::string &url);

private:
	std::string URL;
};

struct MetrolyricsFetcher : public GoogleLyricsFetcher
{
	virtual std::string name() { return "metrolyrics.com"; }

protected:
	virtual std::string regex() { return "<div id=\"lyrics-body\">(.*?)</div>"; }

	virtual bool isURLOk(const std::string &url);
	virtual void postProcess(std::string &data);
};

struct LyricsmaniaFetcher : public GoogleLyricsFetcher
{
	virtual std::string name() { return "lyricsmania.com"; }

protected:
	virtual std::string regex() { return "<div class=\"lyrics-body\".*?</strong>(.*?)</div>"; }
};

struct Sing365Fetcher : public GoogleLyricsFetcher
{
	virtual std::string name() { return "sing365.com"; }

protected:
	virtual std::string regex() { return "<script src=\"//srv.tonefuse.com/showads/showad.js\"></script>(.*?)<script>\n/\\* Sing365 - Below Lyrics"; }

	virtual void postProcess(std::string &data);
};

struct JustSomeLyricsFetcher : public GoogleLyricsFetcher
{
	virtual std::string name() { return "justsomelyrics.com"; }

protected:
	virtual std::string regex() { return "<div class=\"core-left\">(.*?)</div>"; }
};

struct AzLyricsFetcher : public GoogleLyricsFetcher
{
	virtual std::string name() { return "azlyrics.com"; }

protected:
	virtual std::string regex() { return "<!-- start of lyrics -->(.*?)<!-- end of lyrics -->"; }
};

struct InternetLyricsFetcher : public GoogleLyricsFetcher
{
	virtual std::string name() { return "the Internet"; }
	virtual Result fetch(const std::string &artist, const std::string &title);

protected:
	virtual std::string siteKeyword() { return "lyrics"; }
	virtual std::string regex() { return ""; }

	virtual bool isURLOk(const std::string &url);

private:
	std::string URL;
};

extern LyricsFetcher *lyricsPlugins[];

std::string unescapeHtmlUtf8(const std::string &s);

void stripHtmlTags(std::string &s);

#endif // LYRICS_FETCHER_H
