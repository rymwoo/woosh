#ifndef HISTORY_H
#define HISTORY_H

#include <vector>
#include <string>
#include <utility>

using std::vector;
using std::string;

namespace history{
  namespace {
    extern vector<std::pair<int,string>> history; // <index,input>
    extern int MAX_HIST_SZ;
    extern char previousDir[200];
  }
  void push_back(string str);
  bool empty();
  int size();
  string get(int idx);
  int max_size();
  int next_index();
  void setPreviousDir(char* src);
  char* getPreviousDir();
}

#endif