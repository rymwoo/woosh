#ifndef WOOSH_H
#define WOOSH_H

#include <unordered_map>
#include <list>
#include <iostream>

using std::unordered_map;
using std::string;
using std::cout;
#define llist std::list

extern int yylex();
extern char* yytext;
extern int yy_scan_string ( const char *str );

void builtInHistory();
void builtInCd(string token);
void builtInAlias(string key, unordered_map<string,string> &aliases);
void showPrompt();
llist<std::pair<string,int>> tokenizeInput(string str);
void replaceAliases(string &input, unordered_map<string,string> &aliases);
void historyExpansion(string &input);


#endif