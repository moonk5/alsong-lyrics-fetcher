#include <iostream>
#include <string>

#include <AlsongLyricsFetcher.h>

int main(int argc, char *argv[]) 
{
  std::string title = "dead boy's poem", artist = "nightwish";

  std::string resp = "";
  moonk5::alsong::lyrics_fetcher lyrics_fetcher;
  moonk5::alsong::lyrics_serializer lyrics_serializer;

  if (argc > 2) {
    title = argv[1];
    artist = argv[2];
  } 

  std::cout << "\t- Title : " << title << std::endl;
  std::cout << "\t- Artist : " << artist << std::endl;

  lyrics_fetcher.fetch_lyric_list(title, artist, resp);
  lyrics_serializer.parse_lyric_list(resp);

  if (lyrics_serializer.song_list_collection.empty()) {
    std::cout << "No song list found\n";
    return 0;
  }

  resp = "";
  lyrics_fetcher.fetch_lyric(lyrics_serializer.song_list_collection[0].lyric_id, resp);
  lyrics_serializer.parse_lyric(resp);

  lyrics_serializer.write(title, artist);
  lyrics_serializer.read(title, artist);

  std::cout << lyrics_serializer.to_json_string() << std::endl;

  return 0;
}
