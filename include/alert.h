#pragma once

#define  ENABLE_CDSTRUCT    0

#include <iostream>
#include <sstream>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#define COL_DEFAULT   "\033[0m"
#define COL_BOLD      "\033[1m"

#define COL_BLACK     "\033[30m"
#define COL_RED       "\033[31m"
#define COL_GREEN     "\033[32m"
#define COL_YELLOW    "\033[33m"
#define COL_BLUE      "\033[34m"
#define COL_MAGENTA   "\033[35m"
#define COL_CYAN      "\033[36;5m"
#define COL_WHITE     "\033[37m"

#define COL_BK_BLACK     "\033[40m"
#define COL_BK_RED       "\033[41m"
#define COL_BK_GREEN     "\033[42m"
#define COL_BK_YELLOW    "\033[43m"
#define COL_BK_BLUE      "\033[44m"
#define COL_BK_MAGENTA   "\033[45m"
#define COL_BK_CYAN      "\033[46;5m"
#define COL_BK_WHITE     "\033[47m"

#define MAKE_COLOR(r,g,b) \
  "\033[38;2;" #r ";" #g ";" #b "m"

#define MAKE_BK_COLOR(r,g,b) \
  "\033[48;2;" #r ";" #g ";" #b "m"

#define _RGB  MAKE_COLOR
#define _BRGB MAKE_BK_COLOR

#define TAG_ALERT       COL_YELLOW  "#alert"
#define TAG_ALERTMSG    COL_MAGENTA "#alertmsg "
#define TAG_ALERTCTOR   _RGB(100,200,255) "#Contruct"
#define TAG_ALERTDTOR   _RGB(200,150,60)  "#Destruct"
#define TAG_TODOIMPL    _RGB(50,255,255)  "#not implemented here"
#define TAG_PANIC       COL_RED "panic! "

#define debug(...) __VA_ARGS__

#define _streamalert(tag,e...) \
  ({ std::stringstream ss; \
    ss << e; \
  _alert_impl(tag, ss.str().c_str(), \
  __FILE__, __LINE__); })

#define alert \
  _alert_impl(TAG_ALERT, nullptr, __FILE__, __LINE__)

#define alertmsg(e...) \
  _streamalert(TAG_ALERTMSG, COL_WHITE << e)

#if ENABLE_CDSTRUCT
#define alert_ctor \
  _streamalert(TAG_ALERTCTOR, \
    __func__ << "  " << this)

#define alert_dtor \
  _streamalert(TAG_ALERTDTOR, __func__ << " " << this)
#else
#define alert_ctor  0
#define alert_dtor  0
#endif

#define todo_impl \
  _alert_impl(TAG_TODOIMPL,nullptr,__FILE__,__LINE__), \
  exit(1)

#define panic(e...) \
  {_streamalert(TAG_PANIC,e); std::exit(222);}

inline char*
_make_location_str(char* buf, char const* file, size_t line) {
  auto sp = strchr(file, '/');

  while( sp )
    if( auto p = strchr(sp + 1, '/'); p )
      sp = p + 1;
    else
      break;

  sprintf(buf, "%s:%zu:", sp ? sp : file, line);
  return buf;
}

inline void
_alert_impl(  char const* tag, char const* msg,
              char const* file, size_t line) {
  char buf[0x100];
  char buf2[0x400] { ' ' };

  _make_location_str(buf, file, line);
  size_t len = sprintf(buf2,
    COL_BOLD _BRGB(20,20,20)
      "        %s%-30s %s %s",
    _RGB(150, 255, 0),
    buf,
    tag,
    msg ? msg : ""
  );

  size_t endpos = 200;

  for(size_t i=0;i<len;){
    if(buf2[i]=='\033'){
      do{
        endpos++;
        i++;
      }while(buf2[i]!='m');
    }
    else {
      i++;
    }
  }

  memset(buf2 + len, ' ', 0x400-len);
  buf2[endpos] = 0;

  fprintf(stderr,
    "%s" COL_DEFAULT "\n", buf2);
}