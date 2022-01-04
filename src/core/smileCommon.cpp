/*F***************************************************************************
 * This file is part of openSMILE.
 * 
 * Copyright (c) audEERING GmbH. All rights reserved.
 * See the file COPYING for details on license terms.
 ***************************************************************************E*/


#include <core/smileCommon.hpp>
#include <core/versionInfo.hpp>
#include <cmath>
#include <locale.h>

void smilePrintHeader()
{
  SMILE_PRINT(" ");
  SMILE_PRINT(" =============================================================== ");
  SMILE_PRINT("   %s version %s (Rev. %s)", APPNAME, APPVERSION, OPENSMILE_SOURCE_REVISION);
  SMILE_PRINT("   Build date: %s", OPENSMILE_BUILD_DATE);
  SMILE_PRINT("   Build branch: '%s'", OPENSMILE_BUILD_BRANCH);
  SMILE_PRINT("   (c) %s by %s", APPCPYEAR, APPCPAUTHOR);
  SMILE_PRINT("   All rights reserved. See the file COPYING for license terms.");
  SMILE_PRINT(" =============================================================== ");
  SMILE_PRINT(" ");
}

/* check if all elements in the vector are finite. If not, set the element to 0 */
int checkVectorFinite(FLOAT_DMEM *x, long N) 
{
  int i; int f = 1;
  int ret = 1;
  for (i=0; i<N; i++) {
    if ((std::isnan)(x[i])) {
      //printf("ERRR!\n X:%f istart: %f iend %f gend %f gstart %f \n iVI %i II %i end %f start %f step %f startV %f\n",X,istart,iend,gend,gstart,ivI,curRange[i].isInit,curRange[i].end,curRange[i].start,curRange[i].step,startV);
      //SMILE_ERR(2,"#NAN# value encountered in output (This value is now automatically set to 0, but somewhere there is a bug)! (index %i)",i);
      x[i] = 0.0; f=0;
      ret = 0;
    } 
    if ((std::isinf)(x[i])) {
      //printf("ERRR!\n X:%f istart: %f iend %f gend %f gstart %f \n iVI %i II %i end %f start %f step %f startV %f\n",X,istart,iend,gend,gstart,ivI,curRange[i].isInit,curRange[i].end,curRange[i].start,curRange[i].step,startV);
      //SMILE_ERR(2,"#INF# value encountered in output (This value is now automatically set to 0, but somewhere there is a bug)! (index %i)",i);
      x[i] = 0.0; f=0;
      ret = 0;
    }
  }
  return ret;
}

// realloc and initialize additional memory with zero, like calloc
void * crealloc(void *a, size_t size, size_t old_size)
{
  a = realloc(a,size);
  if ((old_size < size)&&(a!=NULL)) {
    char *b = (char *)a + old_size;
    //fill with zeros:
    bzero((void*)b, size-old_size);
  }
  return a;
}

/* allocate a string and expand vararg format string expression into it */
// WARNING: memory allocated by myvprint must freed by the code calling myvprint!!
char *myvprint(const char *fmt, ...) {
  char *s= (char *)malloc(sizeof(char)*(MIN_LOG_STRLEN+4));
  int LL = MIN_LOG_STRLEN+1;
  //TODO: use vasprintf(); -> only available on linux :(
  if (s==NULL) return NULL;
  va_list ap;
  va_start(ap, fmt);
  int len = vsnprintf(s,MIN_LOG_STRLEN+1, fmt, ap);
  if (len > MIN_LOG_STRLEN) {
    free(s);
    s = (char *)malloc(sizeof(char)*(len+4));
    va_end(ap);
    va_start(ap, fmt);
    len = vsnprintf(s,len+1, fmt, ap);
    LL = len+1;
  } else
  if (len == -1) { // we are probably on windows...
    #ifdef _MSC_VER
    free(s);
    len = _vscprintf(fmt, ap);
    s = (char *)malloc(sizeof(char)*(len+4));
    va_end(ap);
    va_start(ap, fmt);
    len = vsnprintf(s,len+1, fmt, ap);
    LL = len+1;
    #else
    s[0] = 0; // return a null string
    #endif
  }
  va_end(ap);
  // ensure the last two characters are 0 !:
  s[LL] = 0;
  s[LL+1] = 0;
  return s;
}

void * memdup(const void *in, size_t size)
{
  void *ret = malloc(size);
  if (ret!=NULL) memcpy(ret,in,size);
  return ret;
}

#ifdef __MACOS
int clock_gettime(int clock_id, struct timespec *tp)
{
	struct timeval tv;
	int retval = gettimeofday (&tv, NULL);
	if (retval == 0)
		TIMEVAL_TO_TIMESPEC (&tv, tp); // Convert into `timespec'.
	return retval;
}
#endif

#define MIN_CHUNK 64
#if defined(__HAVENT_GNULIBS) || defined(__ANDROID__)

long getstr(char **linePointer, size_t *n, FILE *stream,
    char eolCharacter, int offset)
{
  // number of allocated, but unused chars in *linePointer.
  int nCharactersAvailable;
  // position where reading into *linePointer.
  char *readPosition;
  if (n == NULL || linePointer == NULL || stream == NULL) {
    return -1;
  }
  if (*linePointer == NULL) {
    *n = MIN_CHUNK;
    *linePointer = (char *)malloc(*n);
    if (*linePointer == NULL) {
      return -1;
    }
  }
  nCharactersAvailable = (int)(*n) - offset;
  readPosition = *linePointer + offset;
  for ( ; ; ) {
    int c = getc(stream);
    // need at least 1 character in buffer to NULL-terminate string
    if (nCharactersAvailable < 2) {
      if (*n <= MIN_CHUNK) {
        *n += MIN_CHUNK;
      } else {
        *n *= 2;
      }
      nCharactersAvailable = (int)((long long)(*n) + (long long)*linePointer
          - (long long)readPosition );
      *linePointer = (char *)realloc(*linePointer, *n);
      if (*linePointer == NULL) {
        return -1;
      }
      readPosition = *n + *linePointer - nCharactersAvailable;
    }
    if (ferror(stream)) {
      return -1;
    }
    if (c == EOF) {
      // return part of line, if available
      if (readPosition == *linePointer) {
        return -1;
      } else {
        break;
      }
    }
    *readPosition++ = c;
    nCharactersAvailable--;
    if (c == eolCharacter) {
      // return the line when separator (newline) found
      break;
    }
  }
  *readPosition = 0;  // NULL-terminate string
  return (long)((long long)readPosition
      - ((long long)*linePointer + (long long)offset));
}

long smile_getline(char **linePointer, size_t *n, FILE *stream) {
  return getstr(linePointer, n, stream, '\n', 0);
}

#endif

// buffer pointer will be updated to match current position
// caller must have a copy in order to properly free it
long smile_getline_frombuffer(char **linePointer, size_t *n,
      char **buf, int *lenBuf) {
  char eolCharacter = '\n';
  int offset = 0;
  // number of allocated, but unused chars in *linePointer.
  int nCharactersAvailable;
  if (n == NULL || linePointer == NULL || buf == NULL || lenBuf == NULL) {
    return -1;
  }
  if (*lenBuf <= 0)
    return -1;
  if (*linePointer == NULL) {
    *n = MIN_CHUNK;
    *linePointer = (char *)malloc(*n);
    if (*linePointer == NULL) {
      return -1;
    }
  }
  nCharactersAvailable = (int)(*n) - offset;
  // position where reading into *linePointer.
  char *readPosition = *linePointer + offset;
  int bufIdx = 0;
  for ( ; bufIdx < *lenBuf; bufIdx++) {
    int c = *((*buf)++);
    //printf("%c", c);
    // need at least 1 character in buffer to NULL-terminate string
    if (nCharactersAvailable < 2) {
      if (*n <= MIN_CHUNK) {
        *n += MIN_CHUNK;
      } else {
        *n *= 2;
      }
      nCharactersAvailable = (int)((long long)(*n) + (long long)*linePointer
          - (long long)readPosition );
      *linePointer = (char *)realloc(*linePointer, *n);
      if (*linePointer == NULL) {
        return -1;
      }
      readPosition = *n + *linePointer - nCharactersAvailable;
    }
    if (bufIdx == *lenBuf - 1) {  // end of buffer
      // return part of line, if available
      if (readPosition == *linePointer) {
        *lenBuf = 0;
        return -1;
      } else {
        *readPosition++ = c;
        break;
      }
    }
    *readPosition++ = c;
    nCharactersAvailable--;
    if (c == eolCharacter) {
      // return the line when separator (newline) found
      break;
    }
  }
  *lenBuf -= bufIdx + 1;
  *readPosition = 0;  // NULL-terminate string
  return (long)((long long)readPosition
      - ((long long)*linePointer + (long long)offset));
}

void smileCommon_fixLocaleEnUs()
{
  setlocale(LC_NUMERIC, "en_US");
}

void smileCommon_enableVTInWindowsConsole()
{
#if defined(__WINDOWS) && defined(ENABLE_VIRTUAL_TERMINAL_PROCESSING)
  HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
  if (hOut != INVALID_HANDLE_VALUE) {
    DWORD dwMode = 0;
    if (GetConsoleMode(hOut, &dwMode)) {
      dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
      SetConsoleMode(hOut, dwMode);
    }
  }
#endif
}
