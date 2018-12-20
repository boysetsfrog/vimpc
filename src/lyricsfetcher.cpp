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

#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <vector>

#include "project.hpp"
#include "lyricsfetcher.hpp"
#include "regex.hpp"

namespace Curl
{
	size_t write_data(char *buffer, size_t size, size_t nmemb, void *data)
	{
		size_t result = size*nmemb;
		static_cast<std::string *>(data)->append(buffer, result);
		return result;
	}

	CURLcode perform(std::string &data, const std::string &URL, const std::string &referer, bool follow_redirect, unsigned timeout)
	{
		CURLcode result;
		CURL *c = curl_easy_init();
		curl_easy_setopt(c, CURLOPT_URL, URL.c_str());
		curl_easy_setopt(c, CURLOPT_WRITEFUNCTION, write_data);
		curl_easy_setopt(c, CURLOPT_WRITEDATA, &data);
		curl_easy_setopt(c, CURLOPT_CONNECTTIMEOUT, timeout);
		curl_easy_setopt(c, CURLOPT_NOSIGNAL, 1);
		curl_easy_setopt(c, CURLOPT_USERAGENT, "vimpc");
	   if (follow_redirect)
		curl_easy_setopt(c, CURLOPT_FOLLOWLOCATION, 1L);
		if (!referer.empty())
			curl_easy_setopt(c, CURLOPT_REFERER, referer.c_str());
		result = curl_easy_perform(c);
		curl_easy_cleanup(c);
		return result;
	}

	std::string escape(const std::string &s)
	{
		char *cs = curl_easy_escape(0, s.c_str(), s.length());
		std::string result(cs);
		curl_free(cs);
		return result;
	}
}

LyricsFetcher *lyricsPlugins[] =
{
	new LyricwikiFetcher(),
	new AzLyricsFetcher(),
	new Sing365Fetcher(),
	new LyricsmaniaFetcher(),
	new MetrolyricsFetcher(),
	new JustSomeLyricsFetcher(),
	new InternetLyricsFetcher(),
	0
};

const char LyricsFetcher::msgNotFound[] = "Not found";

LyricsFetcher::Result LyricsFetcher::fetch(const std::string &artist, const std::string &title)
{
	Result result;
	result.first = false;

	Regex::RE artist_exp("%artist%");
	Regex::RE title_exp("%title%");

	std::string url = urlTemplate();

	artist_exp.ReplaceAll(artist, url);
	title_exp.ReplaceAll(title, url);

	std::string data;
	CURLcode code = Curl::perform(data, url);

	if (code != CURLE_OK)
	{
		result.second = curl_easy_strerror(code);
		return result;
	}

	auto lyrics = getContent(regex(), data);

	if (lyrics.empty() || notLyrics(data))
	{
		result.second = msgNotFound;
		return result;
	}

	data.clear();
	for (auto it = lyrics.begin(); it != lyrics.end(); ++it)
	{
		postProcess(*it);
		if (!it->empty())
		{
			data += *it;
			if (it != lyrics.end()-1)
				data += "\n\n----------\n\n";
		}
	}

	result.second = data;
	result.first = true;
	return result;
}

std::vector<std::string> LyricsFetcher::getContent(std::string regex_, const std::string &data)
{
	std::string match;
	std::string input = data.c_str();
	std::vector<std::string> result;
	Regex::RE regex(regex_);

	while (regex.Capture(input, &match))
	{
		result.push_back(match);
		regex.Replace("", input);
	}

	return result;
}

void LyricsFetcher::postProcess(std::string &data)
{
	stripHtmlTags(data);
	Regex::RE::Trim(data);
}

/***********************************************************************/

LyricsFetcher::Result LyricwikiFetcher::fetch(const std::string &artist, const std::string &title)
{
	LyricsFetcher::Result result = LyricsFetcher::fetch(artist, title);
	if (result.first == true)
	{
		Regex::RE br("<br />");

		result.first = false;
		std::string data;
		CURLcode code = Curl::perform(data, result.second, "", true);

		if (code != CURLE_OK)
		{
			result.second = curl_easy_strerror(code);
			return result;
		}
		
		auto lyrics = getContent("<div class='lyricbox'>(.*?)</div>", data);
		
		if (lyrics.empty())
		{
			result.second = msgNotFound;
			return result;
		}
		std::transform(lyrics.begin(), lyrics.end(), lyrics.begin(), unescapeHtmlUtf8);
		bool license_restriction = std::any_of(lyrics.begin(), lyrics.end(), [](const std::string &s) {
			return s.find("Unfortunately, we are not licensed to display the full lyrics for this song at the moment.") != std::string::npos;
		});
		if (license_restriction)
		{
			result.second = "License restriction";
			return result;
		}

		data.clear();
		for (auto it = lyrics.begin(); it != lyrics.end(); ++it)
		{
			br.ReplaceAll("\n", *it);

			stripHtmlTags(*it);
			Regex::RE::Trim(*it);
			if (!it->empty())
			{
				data += *it;
				if (it != lyrics.end()-1)
					data += "\n\n----------\n\n";
			}
		}

		result.second = data;
		result.first = true;
	}
	return result;
}

bool LyricwikiFetcher::notLyrics(const std::string &data)
{
	return data.find("action=edit") != std::string::npos;
}

/**********************************************************************/

LyricsFetcher::Result GoogleLyricsFetcher::fetch(const std::string &artist, const std::string &title)
{
	Result result;
	result.first = false;
	
	std::string search_str;
    search_str = "lyrics";
	search_str += "+";
	search_str += artist;
	search_str += "+";
	search_str += title;
	
	std::string google_url = "http://www.google.com/search?hl=en&ie=UTF-8&oe=UTF-8&q=";
	google_url += search_str;
	google_url += "&btnI=I%27m+Feeling+Lucky";

	std::string data;
	CURLcode code = Curl::perform(data, google_url, google_url);

	if (code != CURLE_OK)
	{
		result.second = curl_easy_strerror(code);
		return result;
	}

	auto urls = getContent("<A HREF=\"(.*?)\">here</A>", data);

	if (urls.empty() || !isURLOk(urls[0]))
	{
		result.second = msgNotFound;
		return result;
	}

	data = unescapeHtmlUtf8(urls[0]);

	URL = data.c_str();
	return LyricsFetcher::fetch("", "");
}

bool GoogleLyricsFetcher::isURLOk(const std::string &url)
{
	return url.find(siteKeyword()) != std::string::npos;
}

/**********************************************************************/

void Sing365Fetcher::postProcess(std::string &data)
{
	// throw away ad
	Regex::RE regex("<div.*</div>");
	while (regex.Matches(data))
	{
		regex.Replace("", data);
	}
	LyricsFetcher::postProcess(data);
}

/**********************************************************************/

void MetrolyricsFetcher::postProcess(std::string &data)
{
	// some of lyrics have both \n chars and <br />, html tags
	// are always present whereas \n chars are not, so we need to
	// throw them away to avoid having line breaks doubled.
	Regex::RE end("&#10;");
	Regex::RE br("<br />");

	end.ReplaceAll("", data);
	br.ReplaceAll("\n", data);

	data = unescapeHtmlUtf8(data);
	LyricsFetcher::postProcess(data);
}

bool MetrolyricsFetcher::isURLOk(const std::string &url)
{
	// it sometimes return link to sitemap.xml, which is huge so we need to discard it
	return GoogleLyricsFetcher::isURLOk(url) && url.find("sitemap") == std::string::npos;
}

/**********************************************************************/

LyricsFetcher::Result InternetLyricsFetcher::fetch(const std::string &artist, const std::string &title)
{
	GoogleLyricsFetcher::fetch(artist, title);
	LyricsFetcher::Result result;
	result.first = false;
	result.second = "The following site may contain lyrics for this song: ";
	result.second += URL;
	return result;
}

bool InternetLyricsFetcher::isURLOk(const std::string &url)
{
	URL = url;
	return false;
}

std::string unescapeHtmlUtf8(const std::string &data)
{
	std::string result;
	for (size_t i = 0, j; i < data.length(); ++i)
	{
		if (data[i] == '&' && data[i+1] == '#' && (j = data.find(';', i)) != std::string::npos)
		{
			int n = atoi(&data.c_str()[i+2]);
			if (n >= 0x800)
			{
				result += (0xe0 | ((n >> 12) & 0x0f));
				result += (0x80 | ((n >> 6) & 0x3f));
				result += (0x80 | (n & 0x3f));
			}
			else if (n >= 0x80)
			{
				result += (0xc0 | ((n >> 6) & 0x1f));
				result += (0x80 | (n & 0x3f));
			}
			else
				result += n;
			i = j;
		}
		else
			result += data[i];
	}
	return result;
}

void stripHtmlTags(std::string &s)
{
	Regex::RE single("&#039;");
	Regex::RE amp("&amp;");
	Regex::RE quot("&quot;");
	Regex::RE nbsp("&nbsp;");

	bool erase = 0;
	for (size_t i = s.find("<"); i != std::string::npos; i = s.find("<"))
	{
		size_t j = s.find(">", i)+1;
		s.replace(i, j-i, "");
	}

	single.ReplaceAll("'", s);
	amp.ReplaceAll("&", s);
	quot.ReplaceAll("\"", s);
	nbsp.ReplaceAll(" ", s);

	for (size_t i = 0; i < s.length(); ++i)
	{
		if (erase)
		{
			s.erase(s.begin()+i);
			erase = 0;
		}
		if (s[i] == 13) // ascii code for windows line ending
		{
			s[i] = '\n';
			erase = 1;
		}
		else if (s[i] == '\t')
			s[i] = ' ';
	}
}
