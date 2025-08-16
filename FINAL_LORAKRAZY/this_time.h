
void unixToDateTime(unsigned long t, int &__year, int &__month, int &__day, int &__hour, int &__minute, int &__second) {
  // Days since Unix epoch
  unsigned long days = t / 86400;
  unsigned long secs = t % 86400;

  // Calculate time parts
  __hour = secs / 3600;
  __minute = (secs % 3600) / 60;
  __second = secs % 60;

  // Calculate date parts (Gregorian calendar, ignoring leap seconds)
  // Algorithm from https://howardhinnant.github.io/date_algorithms.html#civil_from_days
  long L = days + 68569 + 2440588;
  long N = (4 * L) / 146097;
  L = L - (146097 * N + 3) / 4;
  long I = (4000 * (L + 1)) / 1461001;
  L = L - (1461 * I) / 4 + 31;
  long J = (80 * L) / 2447;
  __day = L - (2447 * J) / 80;
  L = J / 11;
  __month = J + 2 - (12 * L);
  __year = 100 * (N - 49) + I + L;
}

// Check if year is a leap year
bool isLeapYear(int __year) {
  return (__year % 4 == 0 && (__year % 100 != 0 || __year % 400 == 0));
}

// Number of days per month
const int daysInMonth[] = { 31,28,31,30,31,30,31,31,30,31,30,31 };

unsigned long gpsToUnixMillis(int __year, int __month, int __day, int __hour, int __minute, int __second) {
  // Days from 1970/01/01 to date (year, month, day)
  unsigned long days = 0;

  // Count full years
  for (int y = 1970; y < __year; y++) {
    days += isLeapYear(y) ? 366 : 365;
  }

  // Count full months of current year
  for (int m = 1; m < __month; m++) {
    days += daysInMonth[m - 1];
    if (m == 2 && isLeapYear(__year)) {
      days += 1; // Leap day
    }
  }

  // Add days of current month
  days += __day - 1;

  // Convert everything to seconds
  unsigned long seconds = days * 86400UL + __hour * 3600UL + __minute * 60UL + __second;
  return seconds;
}
