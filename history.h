#ifndef HISTORY_H
#define HISTORY_H

#include <vector>
#include <string>
#include <utility>

using std::vector;
using std::string;

class History {
  public:
    static History* getInstance();
    ~History();
    
    void setPreviousDir(char* src);
    char* getPreviousDir();
    
    void push_back(string str);
    bool empty();
    int size();
    string get(int idx);
    int max_size();
    int next_index();

  private:
    History();
    vector<std::pair<int,string>> history; // <index,input>
    static History* instance;
    static int HIST_INDEX;
    int MAX_HIST_SZ = 5;
    char previousDir[200];
};

#endif