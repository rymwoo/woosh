#define DEBUG
#ifdef DEBUG
#define DBG(x) x
#else
#define DBG(x)
#endif

#include "../include/woosh.h"
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
#include "../include/parse.h"
#include "../include/history.h"
#include "../include/jobs.h"

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

string builtInCd(string token, History* history) {
  char *cwd = getcwd(0,0);
  string targetDir;
  if (token.find("..")==0) { //up one level
    //how many levels up are we going
    int levels=0, count=0;
    while(token.find("..",count)!=string::npos) {
      levels++;
      count=token.find("..",count)+2;
    }
    //copy part after ../ to endPath
    int trimIdx=(levels-1)*3+2;
    if (token[trimIdx]=='/')
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
    targetDir = token+endPath;
  } else if (token.compare("~")==0) { //go home
    char* home = getenv("HOME");
    if (home==NULL) {
      std::cerr<<"HOME variable not set";
      return NULL;
    }
    targetDir=home;
  } else if (token.compare("-")==0) { //toggle previous
    targetDir = history->getPreviousDir();
  } else if (token[0]=='/') { //go to specific dir
    targetDir = token;
  } else { //go to sub dir
    string dest(cwd);
    dest = dest+"/"+token;
    targetDir = dest;
  }
  int dirStatus = chdir(targetDir.c_str());
  if (dirStatus==0) {
    history->setPreviousDir(cwd);
  } else {
    perror("cd");
  }
  free(cwd);
  return targetDir;
}

void builtInAlias(string key, unordered_map<string,string> &aliases) {
  size_t idx=key.find("=");
  if(idx==string::npos) {
    std::cerr<<"malformed alias. requires an '=' sign\n";
  } else {
    string value = key.substr(idx+1,key.length()-idx-1);
    key = key.substr(0,idx);

    DBG(std::clog<<"aliasing["<<key<<"]["<<value<<"]\n";)
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
    DBG(std::clog<<"["<<yytext<<":"<<tokenNames[token]<<"]";)
    string text = yytext;
    if (token==TOKEN) {
      if (text[0]=='"' && text[text.length()-1]=='"')
        text = text.substr(1,text.length()-2);
    }
    std::pair<string,int> pr(text,token);
    input.push_back(pr);
    token = yylex();
  }
  DBG(std::clog<<"\n";)
  return input;
}

void replaceAliases(string &input, unordered_map<string,string> &aliases) {
  for(auto x:aliases) {
    string alKey = x.first;
    int alLength = alKey.length();
    int idx=0;
    bool insideQuotes = false;
    do {
      if (input[idx]=='"')
        insideQuotes = !insideQuotes;
      // if alias found at idx and not in quotes; check if it it a standalone token vs prefix and in-bounds || end of string
      if (alKey.compare(input.substr(idx,alLength))==0 && !insideQuotes) {
        if ((idx==0 || input[idx-1]==' ') && //beginning is separated
        (idx+alLength<input.length() && input[idx+alLength]==' ' || idx+alLength==input.length())) {//end is separated
          if (x.second[0]=='"' && x.second[x.second.length()-1]=='"')
            input.replace(idx,alLength,x.second.substr(1,x.second.length()-2));
          else
            input.replace(idx,alLength,x.second);
          DBG(std::clog<<"replaced ["<<x.first<<"] alias in input\n";)
          idx+=x.second.length();
        }
      }
      idx++;
    } while (idx < input.length());
  }
}

void historyExpansion(string &input, History* history) {
  int idx=0;
  bool insideQuotes = false;
  bool neg=false;
  while (idx<input.length()) {
    if (input[idx]=='"')
      insideQuotes = !insideQuotes;
    if (input[idx]=='!' && !insideQuotes) {
      if (input[idx+1]=='!') { //expand to !-1
        input.replace(idx,2,"!-1");
        continue;
      }
      if (input[idx+1]=='-') {
        neg=true;
        idx++;
      }
      int numEnd=idx;
      //check for number
      while(input[numEnd+1]>='0' && input[numEnd+1]<='9') {
        numEnd++;
      }
      string tmp=input.substr(idx+1,numEnd-idx);
      if (tmp.length()>0) {
        int histNum = stoi(tmp);
        DBG(cout<<"histNUM["<<histNum<<"]\n";)
        if (neg) {
          histNum = -histNum;
          idx--;
        }
        input.replace(idx,numEnd-idx+1,history->get(histNum));
      } else {
        cout<<"valid history not found\n";
      }
      
    }
    idx++;
  }
}

int countNumArgsPlusCmd(llist<std::pair<string,int>> &input) {
  //find where commands + args end / redirection io modifiers start
  llist<std::pair<string,int>>::iterator redirectIter=input.begin();
  int redirectIdx=0;
  do {
    redirectIdx++;
    if ((*redirectIter).second==REDIRECT_IN || (*redirectIter).second==REDIRECT_OUT || (*redirectIter).first.compare("&")==0)
      break;
    redirectIter++;
  } while (redirectIter!=input.end());
  return redirectIdx;
}

llist<std::pair<string,int>>::iterator moveIterToEndOfArgs(llist<std::pair<string,int>> &input) {
  llist<std::pair<string,int>>::iterator redirectIter=input.begin();
  int redirectIdx=0;
  do {
    redirectIdx++;
    if ((*redirectIter).second==REDIRECT_IN || (*redirectIter).second==REDIRECT_OUT || (*redirectIter).first.compare("&")==0)
      break;
    redirectIter++;
  } while (redirectIter!=input.end());
  return redirectIter;
}

char **parseArgsIntoCharArray(int numArgs, llist<std::pair<string,int>> &input) {
  //parse commands and args into char** - ignore io_modifiers
  char** argv = (char**)malloc(sizeof(char*)*(numArgs+1));
  if (argv==NULL) {
    perror("malloc");
    exit(EXIT_FAILURE);
  }
  int idx=0;
  for(llist<std::pair<string,int>>::iterator it=input.begin(); idx<numArgs;++idx,++it) {
    int strLen = (*it).first.length();
    argv[idx]=(char*)malloc(strLen+1);
    if (argv[idx]==NULL) {
      perror("malloc");
      exit(EXIT_FAILURE);
    }
    (*it).first.copy(argv[idx],strLen);
    argv[idx][strLen]='\0';
  }
  argv[idx]=NULL;
  return argv;
}

void setSignals(__sighandler_t value) {
  signal(SIGINT,value);
  signal(SIGQUIT,value);
  signal(SIGTSTP,value);
  signal(SIGTTIN,value);
  signal(SIGTTOU,value);
}

void initializeShellForJobControl() {
  pid_t self_PGid = getpgrp();
  pid_t currentTermFgPG = tcgetpgrp(STDIN_FILENO);

  if (self_PGid != currentTermFgPG){ //we are not a foreground process
    cout<<"Shell must be started in foreground to handle job control\n";
    exit(EXIT_FAILURE);
  }
  
  pid_t self_pid = getpid(); 
  if (setpgid(self_pid,self_pid) < 0){ // put self in new process group
    perror("Could not put shell in new process group");
    exit(EXIT_FAILURE);
  }
  tcsetpgrp(STDIN_FILENO,self_PGid); // make new self process ground in foreground

  //ignore incoming signals
  setSignals(SIG_IGN);
}

string readUserInput() {
  string input;
  fd_set descriptorSet;
  FD_ZERO(&descriptorSet);
  FD_SET(STDIN_FILENO, &descriptorSet);
  timeval tv;
  tv.tv_sec=0;
  tv.tv_usec=0;
  int cinStatus = select(1,&descriptorSet, NULL,NULL,&tv);
  if (cinStatus<0) {
    perror("select()");
  } else if (cinStatus>0) {
    showPrompt();
    std::getline(std::cin,input);
    cout<<input<<"\n";
  } else {
    do {
    showPrompt();
    std::getline(std::cin,input);
    } while (input.empty());
  }
  return input;
}
void debugPrintList(llist<std::pair<string,int>> list) {
  llist<std::pair<string,int>>::iterator iter=list.begin();
  while (iter!=list.end()) {
    std::clog<<"["<<(*iter).first<<"]";
    ++iter;
  }
  std::clog<<" dbgEND\n";
}

int woosh() {
  unordered_map<string,string> aliases;
  History *history = history->getInstance();
  JobController *jobs = jobs->getInstance();
  builtInSource(aliases);

  initializeShellForJobControl();

  while (true) {
    string input = readUserInput();
    historyExpansion(input, history);
    history->push_back(input);
    replaceAliases(input, aliases);

    llist<std::pair<string,int>> inp = tokenizeInput(input);

//    DBG(debugPrintList(inp);)

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
       case JOBS: {
          jobs->printJobList();
          break;
        } //case JOBS
      default: { //not a built in command
          int numArgs=countNumArgsPlusCmd(inp);
          char **argv = parseArgsIntoCharArray(numArgs,inp);

          bool backgroundProcess = (inp.back().first.compare("&")==0 && inp.back()!=inp.front());
          DBG(backgroundProcess?cout<<"forking process to run in background\n":cout<<"";)
          pid_t pid = fork();
          if (pid<0) {
            std::cerr<<"Fork failed\n";
          } else if (pid==0) { //body of child process
            setSignals(SIG_DFL); //reset signals to defaults
            //handling redirects
            llist<std::pair<string,int>>::iterator redirectIter = moveIterToEndOfArgs(inp);
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

            DBG(std::clog<<"Child Process; executing command ["<<argv[0]<<"]...\n";)
            execvp(argv[0],argv);
            cout<<argv[0]<<": "<<strerror(errno)<<"\n";
            //failed to terminate, so clean up memory
            for(int i=0;i<numArgs;++i) {
              free(argv[i]);
            }
            free(argv);
            _exit(EXIT_FAILURE);
          } //child process
          else { //parent process
            int status;

            DBG(std::clog<<"Parent Process; childPID=["<<std::to_string(pid)<<"] - waiting...\n";)
            if (backgroundProcess) {
              jobs->addJob(pid, argv[0]);
            } else {
              if (waitpid(pid,&status,0)<0) {

                std::cerr<<"WaitPID Error\n";
                return 1;
              }
              if WIFEXITED(status) {
                DBG(std::clog<<"Child terminated okay\n";)
              } else {
                std::clog<<"Child process termination error\n";
              }
            }
            
            //cleaning up linked list of args
            for(int i=0;i<numArgs;++i) {
              free(argv[i]);
            }
            free(argv);
          } // parent process
          break;
        } //case default
    }
  } //while (true);
  return 0;
}