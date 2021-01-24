/* *
 *
 * Alsong Lyrics Fetcher
 * version 1.0.0
 * https://github.com/moonk5/alsong-lyrics-fetcher.git
 *
 * Copyright (C) 2018 moonk5
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#ifndef ALSONG_LYRICS_FETCHER_H
#define ALSONG_LYRICS_FETCHER_H

#include <filesystem>
#include <fstream>
#include <iomanip>
#include <locale>
#include <regex>
#include <vector>

#include <boost/algorithm/string/case_conv.hpp>
#include <boost/algorithm/string/replace.hpp>
#include <curl/curl.h>
#include <nlohmann/json.hpp>
#include <tinyxml2.h>

#define ALSONG_LYRICS_FETCHER_MAJOR 1
#define ALSONG_LYRICS_FETCHER_MINOR 0
#define ALSONG_LYRICS_FETCHER_PATCH 0

namespace moonk5
{
  namespace time_conversion
  {
    std::string to_simple_string(unsigned int time_in_ms) {
      const char time_fmt[] = "%02d:%02d.%02d";
      std::vector<char> buff(sizeof(time_fmt));

      if (time_in_ms <= 0)
        return "00:00.00";

      unsigned int minutes = 0;
      unsigned int seconds = 0;
      unsigned int milliseconds = time_in_ms % 1000;
      time_in_ms -= milliseconds;
      minutes = time_in_ms / (60 * 1000);
      time_in_ms -= minutes * (60 * 1000); 
      seconds = time_in_ms / 1000; 

      std::snprintf(&buff[0], buff.size(), time_fmt,
          minutes, seconds, milliseconds);

      return &buff[0];
    }

    unsigned int to_milliseconds(const std::string& time_in_str) {
      unsigned int milliseconds = 0;
      milliseconds += stoi(time_in_str.substr(0, 2)) * 60 * 1000;  // mins
      milliseconds += stoi(time_in_str.substr(3, 2)) * 1000;       // secs
      milliseconds += stoi(time_in_str.substr(6, 2));              // mils

      return milliseconds;
    }
  }

  namespace alsong
  {
    const std::string ALSONG_LYRICS_FETCHER_VERSION =
      std::to_string(ALSONG_LYRICS_FETCHER_MAJOR) + "." +
      std::to_string(ALSONG_LYRICS_FETCHER_MINOR) + "." +
      std::to_string(ALSONG_LYRICS_FETCHER_PATCH);

    struct time_lyrics
    {
      unsigned int time = 0; // unit in milliseconds
      std::vector<std::string> lyrics;

      std::string to_json_string() {
        std::string str_json = "{\"time\":\""
          + time_conversion::to_simple_string(time)
          + "\",\"lyrics\":[";
        for (std::string l : lyrics)
          str_json += "\"" + l + "\",";
        str_json.replace(str_json.size()-1, 1, 1, ']');
        str_json += "}";
        return str_json;
      }
    }; // struct moonk5::alsong::time_lyrics

    struct song_list
    {
      std::string lyric_id = "";
      std::string title = "";
      std::string artist = "";
      std::string album = "";
      
      std::string to_json_string() {
        std::string str_json = "{";
        str_json += "\"lyric_id\":\"" + lyric_id + "\",";
        str_json += "\"title\":\"" + title + "\",";
        str_json += "\"artist\":\"" + artist + "\",";
        str_json += "\"album\":\"" + album + "\",";
        str_json += "}";
        return str_json;
      }

    }; // struct moonk5::alsong::song_list

    struct song_info
    {
      std::string lyric_id = "";
      std::string title = "";
      std::string artist = "";
      std::string album = "";
      std::string written_by = "";
      int delay = 0;
      unsigned int language_count = 0;
      std::vector<time_lyrics> lyrics_collection;

      void add_lyrics(const std::string& time, const std::string& lyrics) {
        // convert string time (mm:ss.SS) to milliseconds
        unsigned int ms = time_conversion::to_milliseconds(time);
        // compare time with previous timestamp
        // NOTE : if curr time is equal to prev time then assume it as
        // the user wants to support multi-languages or multi-lines
        if (lyrics_collection.size() > 0 && ms == lyrics_collection.back().time) {
          // case #1 : multi-languages or multi-lines
          lyrics_collection.back().lyrics.push_back(lyrics);
        } else {
          // case #2 : either first or only one line of lyrics for specific time
          alsong::time_lyrics obj;
          obj.time = ms;
          obj.lyrics.push_back(lyrics);
          lyrics_collection.push_back(obj);
        }
      }

      std::string to_json_string() {
        std::string str_json = "{";
        str_json += "\"lyric_id\":\"" + lyric_id + "\",";
        str_json += "\"title\":\"" + title + "\",";
        str_json += "\"artist\":\"" + artist + "\",";
        str_json += "\"album\":\"" + album + "\",";
        str_json += "\"written_by\":\"" + written_by + "\",";
        str_json += "\"delay\":" + std::to_string(delay) + ",";
        str_json += "\"lyrics\":[";
        for (alsong::time_lyrics tl : lyrics_collection)
          str_json += tl.to_json_string() + ",";
        str_json.replace(str_json.size()-1, 1, 1, ']');
        str_json += "}";
        return str_json;
      }
    }; // struct moonk5::alsong::song_info

    struct lyrics_fetcher
    {
      const std::string URL =
        "http://lyrics.alsong.co.kr/alsongwebservice/service1.asmx";

      const std::string ENC_DATA =
        "7c2d15b8f51ac2f3b2a37d7a445c3158455defb8a58d621eb77a3ff8ae4921318e49cefe24e515f79892a4c29c9a3e204358698c1cfe79c151c04f9561e945096ccd1d1c0a8d8f265a2f3fa7995939b21d8f663b246bbc433c7589da7e68047524b80e16f9671b6ea0faaf9d6cde1b7dbcf1b89aa8a1d67a8bbc566664342e12";

      const std::string SOAP_TEMPLATE_LYRIC_LIST =
        R"(<SOAP-ENV:Envelope
           xmlns:SOAP-ENV="http://www.w3.org/2003/05/soap-envelope"
           xmlns:SOAP-ENC="http://www.w3.org/2003/05/soap-encoding"
           xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance"
           xmlns:xsd="http://www.w3.org/2001/XMLSchema"
           xmlns:ns2="ALSongWebServer/Service1Soap"
           xmlns:ns1="ALSongWebServer"
           xmlns:ns3="ALSongWebServer/Service1Soap12">
           <SOAP-ENV:Body>
           <ns1:GetResembleLyricList2>
           <ns1:encData>$encdata</ns1:encData>
           <ns1:title>$title</ns1:title>
           <ns1:artist>$artist</ns1:artist>
           <ns1:pageNo>1</ns1:pageNo>
           </ns1:GetResembleLyricList2>
           </SOAP-ENV:Body>
           </SOAP-ENV:Envelope>
          )";
      
      const std::string SOAP_TEMPLATE_LYRIC_BY_ID =
        R"(<SOAP-ENV:Envelope
           xmlns:SOAP-ENV="http://www.w3.org/2003/05/soap-envelope"
           xmlns:SOAP-ENC="http://www.w3.org/2003/05/soap-encoding"
           xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance"
           xmlns:xsd="http://www.w3.org/2001/XMLSchema"
           xmlns:ns2="ALSongWebServer/Service1Soap"
           xmlns:ns1="ALSongWebServer"
           xmlns:ns3="ALSongWebServer/Service1Soap12">
           <SOAP-ENV:Body>
           <ns1:GetLyricByID2>
           <ns1:encData>$encdata</ns1:encData>
           <ns1:lyricID>$lyricId</ns1:lyricID>
           </ns1:GetLyricByID2>
           </SOAP-ENV:Body>
           </SOAP-ENV:Envelope>
          )";

     
      CURLcode _fetch(const std::string& soap, std::string &output, unsigned timeout=10) {
        CURLcode result;
        CURL  *curl = curl_easy_init();
        
        struct curl_slist *headers = NULL;
        headers = curl_slist_append(headers,
            "Content-Type: application/soap+xml; charset=utf-8");
        headers = curl_slist_append(headers, "Accept: text/plain");
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, soap.length());
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, soap.c_str());
        curl_easy_setopt(curl, CURLOPT_URL, URL.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &output);
        curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, timeout);
        result = curl_easy_perform(curl);
        
        curl_easy_cleanup(curl);
        
        return result;
      }

      CURLcode fetch_lyric_list(const std::string& title, const std::string& artist,
          std::string &output) {
        CURLcode result;
        if (title.empty() || artist.empty())
          return result;
        
        std::string soap(SOAP_TEMPLATE_LYRIC_LIST);
        soap = std::regex_replace(soap, std::regex("\\$encdata"), ENC_DATA);
        soap = std::regex_replace(soap, std::regex("\\$title"), title);
        soap = std::regex_replace(soap, std::regex("\\$artist"), artist);
        
        return _fetch(soap, output, 20);
      }

      CURLcode fetch_lyric(const std::string& lyric_id, std::string &output) {
        CURLcode result;
        if (lyric_id.empty()) 
          return result;  

        std::string soap(SOAP_TEMPLATE_LYRIC_BY_ID);
        soap = std::regex_replace(soap, std::regex("\\$encdata"), ENC_DATA);
        soap = std::regex_replace(soap, std::regex("\\$lyricId"), lyric_id);
        
        return _fetch(soap, output, 20);
      }

    private:
      static size_t write_data(char *buffer, size_t size,
          size_t nmemb, void *data) {
        size_t result = size * nmemb;
        static_cast<std::string *>(data)->append(buffer, result);
        return result;
      }  
    }; // struct moonk5::alsong::lyrics_fetcher

    class lyrics_serializer
    {
      public:
        std::vector<alsong::song_list> song_list_collection;
        std::vector<alsong::song_info> song_collection;

      public:
        lyrics_serializer(const std::string& lyrics_path,
            unsigned int lyrics_count=3) {
          set_lyrics_folder_path(lyrics_path);
          max_lyrics_count = lyrics_count;
        }

        lyrics_serializer(unsigned int lyrics_count=3)
          : lyrics_serializer(std::string(getenv("HOME")) + "/.alsong",
              lyrics_count) {

          }

        bool parse_lyric_list(const std::string& alsong_raw) {
          int count = 0;
          tinyxml2::XMLDocument doc;
          doc.Parse(alsong_raw.c_str(), alsong_raw.size());
          
          if (alsong_raw.find("GetResembleLyricList2Result") == std::string::npos) {
            std::cerr << "SOAP Fault: missing tag, GetResembleLyricList2Result\n";
            return false;
          }
          
          tinyxml2::XMLElement* result = 
            doc.FirstChildElement("soap:Envelope")
            ->FirstChildElement("soap:Body")
            ->FirstChildElement("GetResembleLyricList2Response")
            ->FirstChildElement("GetResembleLyricList2Result");

          for (tinyxml2::XMLNode* child = result->FirstChildElement("ST_SEARCHLYRIC_LIST")
              ; child
              ; child = child->NextSiblingElement("ST_SEARCHLYRIC_LIST")) {
            alsong::song_list list;
            list.lyric_id = find_child(&child, "lyricID");
            list.title = find_child(&child, "title");
            list.artist = find_child(&child, "artist");
            list.album = find_child(&child, "album");
            song_list_collection.push_back(list);

            ++count;
            if (count >= 50)
              break;
          }

          std::cout << "count = " << count << std::endl;
          
          return true;
        }
        
        bool parse_lyric(const std::string& alsong_raw) {
          tinyxml2::XMLDocument doc;
          doc.Parse(alsong_raw.c_str(), alsong_raw.size());
          
          if (alsong_raw.find("GetLyricByID2Result") == std::string::npos) {
            std::cerr << "SOAP Fault: missing tag, GetLyricByID2Result\n";
            return false;
          }

          tinyxml2::XMLElement* result = 
            doc.FirstChildElement("soap:Envelope")
            ->FirstChildElement("soap:Body")
            ->FirstChildElement("GetLyricByID2Response")
            ->FirstChildElement("GetLyricByID2Result");
          
          tinyxml2::XMLNode* child = result->NextSibling();

          alsong::song_info song;
          song.lyric_id = find_child(&child, "lyricID");
          song.title = find_child(&child, "title");
          song.artist = find_child(&child, "artist");
          song.album = find_child(&child, "album");
          song.written_by = find_child(&child, "registerName");
          song.delay = 0;
          std::string lyrics_raw = find_child(&child, "lyric");
          parse_lyrics(lyrics_raw, song);

          //std::cout << song.to_json_string() << std::endl;
          song_collection.push_back(song);
          
          return true;
        }

        // serialization
        // transformates a song_info object in memory to a file
        bool write(const std::string& title, const std::string& artist,
            bool overwrite=false) {
          if (song_collection.size() <= 0)
            return false;
          
          // file nameing convention
          // 'artist - title.lyrics' 
          std::string filename = create_filename(artist, title);
          std::filesystem::path lyrics_path = lyrics_folder_path / filename;
          if (std::filesystem::exists(lyrics_path) && overwrite == false) {
            std::cerr << "moonk5::alsong::lyrics_serializer::write() - "
              << "file already exists\n";
            return false;
          } else {
            std::ofstream ofs(lyrics_path);
            ofs << to_json_string() << std::endl;
            ofs.close();
          }
          return true;
        }
        
        bool write(bool overwrite=false) {
          if (song_collection.size() <= 0)
            return false;
          return write(song_collection[0].title, song_collection[0].artist, overwrite);
        }
        
        //deserialize
        // transformates a lyrics file into a song_info object
        bool read(const std::string& title, const std::string& artist) {
          std::string filename = create_filename(artist, title);
          std::filesystem::path lyrics_path = lyrics_folder_path / filename;
          if (std::filesystem::exists(lyrics_path)) {
            // file exists ... de-serialize
            std::ifstream ifs(lyrics_path);
            nlohmann::json j;
            ifs >> j;

            song_collection.clear();

            auto& collection = j.at("song_collection");
            for (auto&& s : collection) {
              alsong::song_info song;
              song.title = s.at("title");
              song.artist = s.at("artist");
              song.album = s.at("album");
              song.written_by = s.at("written_by");
              song.delay = s.at("delay");
              auto& tls = s.at("lyrics");
              for (auto&& tl : tls) {
                auto& ls = tl.at("lyrics");
                for (auto&& l : ls) {
                  song.add_lyrics(tl.at("time"), l);
                }
              }
              song_collection.push_back(song);
            }
            ifs.close(); 
          }

          return true;
        }

        void set_lyrics_folder_path(const std::string& path) {
          lyrics_folder_path = path;
          if (!std::filesystem::exists(lyrics_folder_path)) {
            std::filesystem::create_directory(lyrics_folder_path);
          }
        }

        std::string to_json_string() {
          std::string str_json = "{\"song_collection\":[";
          for (int i = 0; i < song_collection.size(); ++i) {
            song_info &song = song_collection[i];
            str_json += song.to_json_string();
            if (i < song_collection.size() - 1)
              str_json += ",";
          }
          str_json += "]}";
          return str_json;
        }

      private:
        std::string create_filename(std::string artist, std::string title) {
          boost::to_upper(artist);
          boost::to_upper(title);
          return artist + " - " + title + ".lyrics";
        }

        std::string find_child(tinyxml2::XMLNode** root,
            const std::string& value) {
          std::string text = "";
          tinyxml2::XMLElement* element =
            (*root)->FirstChildElement(value.c_str());
          if (element != nullptr && element->GetText() != nullptr)
            text = element->GetText();
          return text;
        }

        void parse_lyrics(std::string& input, alsong::song_info& output) {
          const std::string REGEX_TIME =
            "\\[([0-5][0-9]:[0-5][0-9].[0-9][0-9])\\]";
          const std::string REGEX_TIME_LYRICS =
            "(" + REGEX_TIME + ")(.*)(\n)";
          std::regex reg_ex(REGEX_TIME_LYRICS);
          std::smatch match;

          // replace unnecessary XML tags;
          boost::replace_all(input, "<br>", "\n");
          boost::replace_all(input, "\"", "\\\"");
          boost::replace_all(input, "[00:00.00]\n", "");

          // search for time and lyrics
          while (std::regex_search(input, match, reg_ex)) {
            output.add_lyrics(match[2], match[3]);
            input = match.suffix().str();
          }
        }

        std::filesystem::path lyrics_folder_path;
        unsigned int max_lyrics_count; 
    }; // class moonk5::alsong::lyrics_serializer
  }
}
#endif // ALSONG_LYRICS_FETCHER_H
