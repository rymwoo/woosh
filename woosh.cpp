#include "woosh.h"
#include <list>
#include <unordered_map>
#include <iostream>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string>
#include <utility>
#include <fstream>
#include "parse.h"
#include "history.h"

/////////////////////////////
/*
  Built-in functions
*/
/////////////////////////////

void builtInHistory(History *history) {
  if (history->empty()) {
    cout<<"No history to show\n";
  } else {
    int i;
    for(i=0;i<history->size();i++) {
      if (history->next_index()>history->size()) {
        int idx = history->next_index()-history->size()+i;
        cout<<idx<<" "<<history->get(idx)<<std::endl;
      } else {
        cout<<i+1<<" "<<history->get(i+1)<<std::endl;
      }
    }
  }
}

void builtInCd(string token, History* history) {
  char *cwd = getcwd(0,0);
  int dirStatus;
  if (token.find("..")==0) { //up one level
    //how many levels up are we going
    int levels=0, count=0;
    while(token.find("..",count)!=string::npos) {
      levels++;
      count=token.find("..",count)+2;
    }
    //copy part after ../ to endPath
    int trimIdx=(levels-1)*3+2;
    if (token[trimIdx+1]=='/')
      trimIdx++;
    string endPath=token.substr(trimIdx,token.length()-trimIdx);
    char *dest = getcwd(0,0);
    int idx;
    count=0;
    for(idx=strlen(dest)-1;idx>=0;--idx) {
      if (dest[idx]=='/')
        count++;
      if (count==levels)
        break;
    }
    dest[idx+1]='\0';
    token.assign(dest);
    free(dest);
    dirStatus = chdir((token+endPath).c_str());
  } else if (token.compare("~")==0) { //go home
    char* home = getenv("HOME");
    if (home==NULL) {
      std::cerr<<"HOME variable not set";
      return;
    }
    dirStatus = chdir(home);
  } else if (token.compare("-")==0) { //toggle previous
    dirStatus = chdir(history->getPreviousDir());
  } else if (token[0]=='/') { //go to specific dir
    dirStatus = chdir(token.c_str());
  } else { //go to sub dir
    string dest(cwd);
    dest = dest+"/"+token;
    dirStatus = chdir(dest.c_str());
  }
  if (dirStatus==0) {
    history->setPreviousDir(cwd);
  } else {
    perror("cd");
  }
  free(cwd);
}

void builtInAlias(string key, unordered_map<string,string> &aliases) {
  size_t idx=key.find("=");
  if(idx==string::npos) {
    std::cerr<<"malformed alias. requires an '=' sign\n";
  } else {
    string value = key.substr(idx+1,key.length()-idx-1);
    key = key.substr(0,idx);

    std::clog<<"aliasing["<<key<<"]["<<value<<"]\n";
    aliases[key]=value;
  }
}

void builtInSource(unordered_map<string,string> &aliases, string rcFile=".woosh/woosh.rc") {
  
  std::ifstream rc;
  rc.open(rcFile);
  if (!rc.is_open()) {
    cout<<"Unable to load resource file. Using defaults instead.\n";
    return;
  }
  string input;
  while (!rc.eof()) {
    getline(rc,input);
    if (input.find("alias")==0) {
      input = input.substr(input.find(" ")+1,input.length()-input.find(" ")-1);
      size_t splitIdx = input.find("=");
      if (splitIdx == string::npos) {
        cout<<"Malformed resource file. "<<input<<" does not appear to be a valid alias.\n";
        return;
      }
      builtInAlias(input, aliases);
    }
  }
  rc.close();
}
/////////////////////////////
/*
  Built-in functions end
*/
/////////////////////////////

void redirectOutputFile(char origin, string file) {
  const int CHAR_OFFSET = 48;
  if (file[0]=='"' && file[file.length()-1]=='"')
    file = file.substr(1,file.length()-2);
  int fd = open(file.c_str(), O_WRONLY|O_TRUNC|O_CREAT);
  if (fd<0) {
    perror("failed to open file");
    _exit(EXIT_FAILURE);
  } else {
    //successfully created; make accessible
    fchmod(fd,S_IROTH|S_IRGRP|S_IRUSR|S_IWUSR);
  }
  if (origin!='&') {
    int descrNumber = ((int)origin)-CHAR_OFFSET;
    dup2(fd,descrNumber);
    close(fd);
  } //descriptorSource!='&'
  else { //redirecting both STDOUT and STDERR
    dup2(fd,STDOUT_FILENO);
    dup2(fd,STDERR_FILENO);
    close(fd);
  } //redirecting both STDOUT and STDERR
}

void redirectOutputDescriptor(char origin, char descriptor) {
  const int CHAR_OFFSET = 48;
  //destinations for redirect already validated in lexer, must be [12]
  int descrNumber = (int)descriptor-CHAR_OFFSET;
  if (origin!='&') {
    close(((int)origin)-CHAR_OFFSET);
    dup(descrNumber);
  } else { //redirecting both STDOUT and STDERR
    dup2(descrNumber,STDOUT_FILENO);
    dup2(descrNumber,STDERR_FILENO);
  }
}

void showPrompt() {
  char name[200];
  getlogin_r(name,200);
  char *cwd = getcwd(0,0);
  string prompt(cwd);
  free(cwd);
  size_t pos = prompt.find(name);
  if (pos!=string::npos) {
    prompt.replace(0,pos+strlen(name),"~");
  }
  cout<<"[@"<<name<<"]"<<prompt<<" >> ";
}

llist<std::pair<string,int>> tokenizeInput(string str) {
  const string tokenNames[] = {"", "HISTORY","EXIT","ALIAS", "SOURCE", "CD", "REDIRECT_IN", "REDIRECT_OUT", "TOKEN"};
  llist<std::pair<string,int>> input;
  int token;
  yy_scan_string(str.c_str());
  token = yylex();
  while(token){
    std::clog<<"["<<yytext<<":"<<tokenNames[token]<<"]";
    std::pair<string,int> pr(yytext,token);
    input.push_back(pr);
    token = yylex();
  }
  std::clog<<"\n";
  return input;
}

void replaceAliases(string &input, unordered_map<string,string> &aliases) {
  for(auto x:aliases) {
    string alKey = x.first;
    int alLength = alKey.length();
    int idx=0;
    do {
      // if alias found at idx; check if it it a standalone token vs prefix and in-bounds || end of string
      if (alKey.compare(input.substr(idx,alLength))==0) {
        if ((idx==0 || input[idx-1]==' ') && //beginning is separated
        (idx+alLength<input.length() && input[idx+alLength]==' ' || idx+alLength==input.length())) {//end is separated
          if (x.second[0]=='"' && x.second[x.second.length()-1]=='"')
            input.replace(idx,alLength,x.second.substr(1,x.second.length()-2));
          else
            input.replace(idx,alLength,x.second);
          std::clog<<"replaced ["<<x.first<<"] alias in input\n";
          idx+=x.second.length();
        }
      }
      idx++;
    } while (idx < input.length());
  }
}

void historyExpansion(string &input, History* history) {
  int i=0;
  bool neg=false;
  while (i<input.length()) {
    if (input[i]=='!') {
      if (input[i+1]=='!') { //expand to !-1
        input.replace(i,2,"!-1");
        continue;
      }
      if (input[i+1]=='-') {
        neg=true;
        i++;
      }
      int numEnd=i;
      //check for number
      while(input[numEnd+1]>='0' && input[numEnd+1]<='9') {
        numEnd++;
      }
      string tmp=input.substr(i+1,numEnd-i);
      if (tmp.length()>0) {
        int histNum = stoi(tmp);
        cout<<"histNUM["<<histNum<<"]\n";
        if (neg) {
          histNum = -histNum;
          i--;
        }
        input.replace(i,numEnd-i+1,history->get(histNum));
        i=numEnd;
      } else {
        cout<<"valid history not found\n";
      }
      
    }
    i++;
  }
}

void debugPrintList(llist<std::pair<string,int>> list) {
  llist<std::pair<string,int>>::iterator iter=list.begin();
  while (iter!=list.end()) {
    std::clog<<"["<<(*iter).first<<"]";
    ++iter;
  }
  std::clog<<" dbgEND\n";
}

int main(){
  unordered_map<string,string> aliases;
  History *history = history->getInstance();
  builtInSource(aliases);
  
  while (true) {

    string input;
    do {
      showPrompt();
      std::getline(std::cin,input);
    } while (input.empty());
    historyExpansion(input, history);
    history->push_back(input);
    replaceAliases(input, aliases);

    llist<std::pair<string,int>> inp = tokenizeInput(input);

//    debugPrintList(inp);

    //execute commands
    //built-in commands
    switch ((*inp.begin()).second) {
      case EXIT:
          exit(0);
      case HISTORY: {
          builtInHistory(history);
          break;
        } //case HISTORY
      case ALIAS: {
          if (inp.size()<2) {
            std::cerr<<"malformed alias. missing required arguments\n";
          } else {
            //inp of form [alias][x=y]
            llist<std::pair<string,int>>::iterator it = inp.begin();
            it++;
            builtInAlias((*it).first, aliases);
          }
          break;
        } //case ALIAS
      case SOURCE: {
          if (inp.size()>1) {
            llist<std::pair<string,int>>::iterator it = inp.begin();
            it++;
            builtInSource(aliases,(*it).first);
          } else
            builtInSource(aliases);
          break;
        } //case SOURCE
      case CD: {
          if (inp.size()<2) {
            builtInCd("~",history);
          } else { //cd using next arg
            llist<std::pair<string,int>>::iterator it = inp.begin();
            it++;
            builtInCd((*it).first,history);
          }
          break;
        } //case CD
      default: { //not a built in command
          //find where commands + args end / redirection io modifiers start
          llist<std::pair<string,int>>::iterator redirectIter=inp.begin();
          int redirectIdx=0;
          do {
            redirectIter++;
            redirectIdx++;
            if ((*redirectIter).second==REDIRECT_IN || (*redirectIter).second==REDIRECT_OUT)
              break;
          } while (redirectIter!=inp.end());
          //parse commands and args into char** - ignore io_modifiers
          char** argv = (char**)malloc(sizeof(char*)*redirectIdx);
          int idx=0;
          for(llist<std::pair<string,int>>::iterator it=inp.begin(); idx<redirectIdx;++idx,++it) {
            int strLen = (*it).first.length();
            argv[idx]=(char*)malloc(strLen+1);
            (*it).first.copy(argv[idx],strLen);
            argv[idx][strLen]='\0';
          }
          argv[idx]=NULL;

          pid_t pid = fork();
          if (pid<0) {
            std::cerr<<"Fork failed\n";
          } else if (pid==0) { //body of child process
            //handling redirects
            do { //while (redirectIter != inp.end());
              if ((*redirectIter).second == REDIRECT_IN) {
                redirectIter++;
                if (redirectIter == inp.end()) {
                  cout<<"Redirect error: '<' must be followed by a valid filename\n";
                  _exit(EXIT_FAILURE);
                } else {
                  int fd = open((*redirectIter).first.c_str(),O_RDONLY);
                  if (fd==-1) {
                    perror("redirect failed");
                    _exit(EXIT_FAILURE);
                  } else {
                    dup2(fd,STDIN_FILENO);
                    close(fd);
                  }
                }
              } //REDIRECT_IN
              else if ((*redirectIter).second == REDIRECT_OUT) {
                // [12&]?>(&[12])? 
                // parse into [origin] > [destination]
                string destination=(*redirectIter).first;
                char origin = destination[0];
                if (origin == '>') {   //default redirects STDOUT 
                  origin='1';
                }
                int redirIdx = destination.find(">");
                destination = destination.substr(redirIdx+1,destination.length()-redirIdx-1);

                if (destination.find("&")== string::npos) { 
                  redirectIter++;
                  if (redirectIter == inp.end()) {
                    cout<<"Redirect error: '>' must be followed by a valid file descriptor\n";
                    _exit(EXIT_FAILURE);
                  }
                  redirectOutputFile(origin, (*redirectIter).first);
                } else {
                  char descriptor = destination[destination.length()-1];
                  redirectOutputDescriptor(origin, descriptor);
                }
                
              } //REDIRECT_OUT
              redirectIter++;
            } while (redirectIter != inp.end());

            std::clog<<"Child Process; executing command ["<<argv[0]<<"]...\n";
            execvp(argv[0],argv);
            cout<<argv[0]<<": "<<strerror(errno)<<"\n";
            perror(argv[0]);
            _exit(EXIT_FAILURE);
          } //child process
          else { //parent process
            int status;
            std::clog<<"Parent Process; childPID=["<<std::to_string(pid)<<"] - waiting...\n";
            if (waitpid(pid,&status,0)<0) {
              std::cerr<<"WaitPID Error\n";
              return 1;
            }
            if WIFEXITED(status) {
              std::clog<<"Child terminated okay\n";
            } else {
              std::clog<<"Child termination error\n";
            }
            
          llist<std::pair<string,int>>::iterator it=inp.begin();
          int redirectIdx=0;
          do {
            it++;
            redirectIdx++;
            if ((*it).second==REDIRECT_IN || (*it).second==REDIRECT_OUT)
              break;
          } while (it!=inp.end());

            for(int i=0;i<redirectIdx;++i) {
              free(argv[i]);
            }
            free(argv);
          } // parent process
          break;
        } //case default
    }
  }
  return 0;
}