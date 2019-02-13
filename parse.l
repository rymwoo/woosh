%{
#include "parse.h"
/*
*/
%}

%%

"history"           return HISTORY;
"exit"              return EXIT;
"alias"             return ALIAS;
"source"            return SOURCE;
"cd"                return CD;
"<"                 return REDIRECT_IN;
[12&]?>(&[12])?     return REDIRECT_OUT;
[^ <>]*\"([^"\\]|\\.)*\"[^ ]*|[^ <>]+  return TOKEN;
.                   ;

%%

int yywrap(void) {return 1;}
