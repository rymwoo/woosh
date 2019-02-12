#include "history.h"
#include <iostream>
#include <string.h>

namespace history{
  namespace {
    vector<std::pair<int,string>> history; // <index,input>
    static int HIST_INDEX=1;
    int MAX_HIST_SZ = 5;
    char previousDir[200];
  }
}

void history::push_back(string str) {
  if (HIST_INDEX>MAX_HIST_SZ) {
    history[(HIST_INDEX-1)%MAX_HIST_SZ].first=HIST_INDEX;
    history[(HIST_INDEX-1)%MAX_HIST_SZ].second=str;
  } else {
    std::pair<int,string> pr(HIST_INDEX,str);
    history.push_back(pr);
  }
  HIST_INDEX++;
}
bool history::empty() {
  return history.empty();
}
int history::size() {
  return history.size();
}
string history::get(int idx) {
  if (idx<0)
    idx = HIST_INDEX+idx;
  int mod = (idx-1)%MAX_HIST_SZ;

  if (mod<0 || mod>=history.size()) {
    std::cerr<<"err: tried to access invalid index"; //array out of bounds
    return "";
  }
  if (history[mod].first==idx) {
    return history[mod].second;
  } else {
    std::cerr<<"could not locate valid history. index "<<idx<<" no longer stored in memory\n";
    return "";
  }
}
int history::max_size() {
  return MAX_HIST_SZ;
}
int history::next_index(){
  return HIST_INDEX;
}

void history::setPreviousDir(char* src) {
  strcpy(previousDir,src);
}
char* history::getPreviousDir() {
  return previousDir;
}