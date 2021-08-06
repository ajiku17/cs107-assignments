using namespace std;
#include <sys/types.h>
#include <sys/stat.h>
#include <iostream>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include "imdb.h"
#include <string.h>

const char *const imdb::kActorFileName = "actordata";
const char *const imdb::kMovieFileName = "moviedata";

/* A struct to encapsulate the key value and the address of the data file*/
struct key{
  const void* value;
  const void* baseArray;
};

imdb::imdb(const string& directory)
{
  const string actorFileName = directory + "/" + kActorFileName;
  const string movieFileName = directory + "/" + kMovieFileName;
  
  actorFile = acquireFileMap(actorFileName, actorInfo);
  movieFile = acquireFileMap(movieFileName, movieInfo);
}

bool imdb::good() const
{
  return !( (actorInfo.fd == -1) || 
	    (movieInfo.fd == -1) ); 
}

/**
  int compareActors();
    compares actrors for bsearch
  int compareFilms();
    compares Films for bsearch.
*/

int compareActors(const void* ptr1, const void* ptr2){
  key keyPlayer = *(key*)ptr1;
  char* s2 = (char*)((char*)keyPlayer.baseArray + *(int*)ptr2);
  return strcmp((char*)keyPlayer.value, s2);   
}

int compareFilms(const void* ptr1, const void* ptr2){
  key keyMovie = *(key*)ptr1;
  film movie1 = *(film*)(keyMovie.value);
  film movie2;
  char* record = (char*)((char*)keyMovie.baseArray + *(int*)ptr2);
  movie2.title = record;
  record += (strlen(record) + 1);
  movie2.year = 1900 + *record;
  if(movie1 < movie2){
    return -1;
  }else if(movie1 == movie2){
    return 0;
  }
  return 1;
}


bool imdb::getCredits(const string& player, vector<film>& films) const {
  key toCompare{
    &(player[0]), // void* value;
    actorFile,    // void* baseArray;
  };
  int* offset_array = (int*)actorFile + 1;
  int player_num = *(int*)actorFile;
  void* start = bsearch(&toCompare, offset_array, player_num, sizeof(int), compareActors);
  if(start == NULL) return false;
  char* player_record = (char*)actorFile + *(int*)start;
  player_record += (strlen(player_record) + 1);
  if(((char*)player_record - (char*)actorFile) % 2 == 1) 
    player_record++;
  int movie_num = *(short*)player_record;
  player_record += 2;
  if(((char*)player_record - (char*)actorFile) % 4 != 0) 
    player_record += 2;
  int* offset = (int*)player_record;
  for(int i = 0; i < movie_num; i++){
    film toAdd;
    char* movie_record = (char*)movieFile + *offset;
    toAdd.title = movie_record;
    movie_record += (strlen(movie_record) + 1);
    toAdd.year = 1900 + *(char*)movie_record;
    films.push_back(toAdd);
    offset++;
  }

  return true;
}



bool imdb::getCast(const film& movie, vector<string>& players) const {
  key toCompare{
    &movie,   // void* value;
    movieFile,// void* baseArray;
  };
  int *offset_array = (int*)movieFile + 1;
  void* ptr = bsearch(&toCompare, offset_array, *(int*)movieFile, sizeof(int), compareFilms);
  if(ptr == NULL) return false;
	int offset = *(int*)ptr;
	char* start = (char*)movieFile + offset;
  start += (strlen(start) + 2);
  if(((char*)start - (char*)movieFile) % 2 == 1)
  	start++;
  short num_cast = *(short*)start;
  start += 2;
  if(((char*)start - (char*)movieFile) % 4 != 0)
  	start += 2;
  int* player_offsets = (int*)start;
  for(int i = 0; i < num_cast; i++){ //pushes player names into a vector;
  	char* name = (char*)actorFile + *player_offsets;
  	players.push_back(name);
  	player_offsets++;
  }
  return true;
}

imdb::~imdb()
{
  releaseFileMap(actorInfo);
  releaseFileMap(movieInfo);
}

// ignore everything below... it's all UNIXy stuff in place to make a file look like
// an array of bytes in RAM.. 
const void *imdb::acquireFileMap(const string& fileName, struct fileInfo& info)
{
  struct stat stats;
  stat(fileName.c_str(), &stats);
  info.fileSize = stats.st_size;
  info.fd = open(fileName.c_str(), O_RDONLY);
  return info.fileMap = mmap(0, info.fileSize, PROT_READ, MAP_SHARED, info.fd, 0);
}

void imdb::releaseFileMap(struct fileInfo& info)
{
  if (info.fileMap != NULL) munmap((char *) info.fileMap, info.fileSize);
  if (info.fd != -1) close(info.fd);
}
