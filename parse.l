%{
#include "parse.h"
/*
  
*/
%}

%%

"history"           return HISTORY;
"exit"              return EXIT;
"alias"             return ALIAS;
"cd"                return CD;
"<"                 return REDIRECT_IN;
[12&]?>(&[12])?      return REDIRECT_OUT;
[^ \t\r|&;)(><]+    return TOKEN;
 /*.                   ;*/

%%

int yywrap(void) {return 1;}
