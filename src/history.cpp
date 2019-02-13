#include "../include/history.h"
#include <iostream>
#include <string.h>


History* History::instance = NULL;
int History::HIST_INDEX = 1;

History::History() {}

History* History::getInstance() {
  if (instance==NULL)
    return new History();
  else
    return instance;
}

void History::setPreviousDir(char* src) {
  strcpy(previousDir,src);
}
char* History::getPreviousDir() {
  return previousDir;
}

void History::push_back(string str) {
  if (HIST_INDEX>MAX_HIST_SZ) {
    history[(HIST_INDEX-1)%MAX_HIST_SZ].first=HIST_INDEX;
    history[(HIST_INDEX-1)%MAX_HIST_SZ].second=str;
  } else {
    std::pair<int,string> pr(HIST_INDEX,str);
    history.push_back(pr);
  }
  HIST_INDEX++;
}
bool History::empty() {
  return history.empty();
}
int History::size() {
  return history.size();
}
string History::get(int idx) {
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
int History::max_size() {
  return MAX_HIST_SZ;
}
int History::next_index(){
  return HIST_INDEX;
}

void History::clearHistory(){
  HIST_INDEX = 1;
  history.clear();
}