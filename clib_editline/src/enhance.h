#include <ncurses.h>
#include "TROOT.h"

void setKeywordColors(char* colorTab, char* colorBracket, char* colorBadBracket);
int selectColor(TString str);
void highlightKeywords(EditLine * el);
int matchParentheses(EditLine * el);
void colorWord(EditLine * el, int first, int last, int color);
void colorBrackets(EditLine * el, int open, int close, int color);

