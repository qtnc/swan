// Taken from https://gist.github.com/tknopp/9271244
#ifdef __WIN32
 
#include <stdlib.h>
#include <locale.h>

 
// Cache locale object
static int c_locale_initialized = 0;
static _locale_t c_locale;
 
/* _locale_t get_c_locale()
{
  if(!c_locale_initialized)
  {
    c_locale_initialized = 1;
    c_locale = _create_locale(LC_ALL,"C");
  }
  return c_locale;
}
 
double strtod_c(const char *nptr, char **endptr)
{
  return _strtod_l(nptr, endptr, get_c_locale());
}*/

double strtod_c(const char *nptr, char **endptr) {
if (!c_locale_initialized) {
setlocale(LC_NUMERIC, "c");
c_locale_initialized=1;
}
  return strtod(nptr, endptr);
}
 
#else
 
#include <stdlib.h>
#include <xlocale.h>
 
// Cache locale object
static int c_locale_initialized = 0;
static locale_t c_locale;
 
locale_t get_c_locale()
{
  if(!c_locale_initialized)
  {
    c_locale_initialized = 1;
    c_locale = newlocale(LC_ALL_MASK, NULL, NULL);
  }
  return c_locale;
}
 
double strtod_c(const char *nptr, char **endptr)
{
  return strtod_l(nptr, endptr, get_c_locale());
}
 
#endif