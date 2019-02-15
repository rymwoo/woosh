#ifndef WOOSH_H
#define WOOSH_H

#include <unordered_map>
#include <utility>
#include <list>
#include <iostream>
#include <signal.h>
#include "history.h"

using std::unordered_map;
using std::string;
using std::cout;
#define llist std::list

extern int yylex();
extern char* yytext;
extern int yy_scan_string ( const char *str );

void builtInHistory(History *history);
string builtInCd(string token, History* history);
void builtInAlias(string key, unordered_map<string,string> &aliases);
void builtInSource(unordered_map<string,string> &aliases, string rcFile);

void redirectOutputFile(char origin, string file);
void redirectOutputDescriptor(char origin, char descriptor);

void showPrompt();
llist<std::pair<string,int>> tokenizeInput(string str);
void replaceAliases(string &input, unordered_map<string,string> &aliases);
void historyExpansion(string &input, History* history);

int countNumArgsPlusCmd(llist<std::pair<string,int>> &input);
llist<std::pair<string,int>>::iterator moveIterToEndOfArgs(llist<std::pair<string,int>> &input);
char **parseArgsIntoCharArray(int numArgs, llist<std::pair<string,int>> &input);

void setSignals(__sighandler_t value);
void handlerSIGCHLD(int signal);
void blockSIGCHLD(bool block);

void initializeShellForJobControl();
string waitForInput();

int woosh();

#endif