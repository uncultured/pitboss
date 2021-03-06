#include <ctime>
#include <WString.h>

namespace PitBoss {

String getTime(const char * format = "%c") {
  time_t rawTime;
  time(&rawTime);
  tm * localTime;
  localTime = localtime(&rawTime);
  int size = 64;
  char * buffer = new char[size];
  while (0 == strftime(buffer, size, format, localTime)) {
    size *= 2;
    buffer = new char[size];
  }
  return String(buffer);
}

}