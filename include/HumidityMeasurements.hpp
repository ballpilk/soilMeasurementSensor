#pragma once
#include <Arduino.h>
#include <NTPClient.h>
char daysOfTheWeek[7][13] = {"Niedziela", "Poniedzialek", "Wtorek", "Sroda", "Czwartek", "Piatek", "Sobota"};
struct HumidityMeas
{
 int day;
 int hour;
 int minute;
 double humidity; 
};
struct MeasOutput{
  String time;
  double meas;
};

struct MeasurementBuffer{
  static const int len = 100; 
  HumidityMeas meas[len];
  int curr = 0;
  MeasOutput retval[len];
  double getHumidity()
  {
      double hum_sum = 0;
      for (int i = 0; i < 50; ++i)
        hum_sum += analogRead(PIN_A0);
      hum_sum /= 50.;
      return hum_sum* -0.005 + 3.5;
  }

  void addMeasurement(const NTPClient& timeClient)
  {
    HumidityMeas toAdd;
    toAdd.day = timeClient.getDay();
    toAdd.hour = timeClient.getHours();
    toAdd.minute = timeClient.getMinutes();
    toAdd.humidity = getHumidity();
    addMeasurement(toAdd);
  }
  MeasOutput* getMeasurements()
  {
    for(int i=0;i<len;++i)
    {
      retval[i].time = String(daysOfTheWeek[ meas[(curr - i + len) % len].day ]) + " " + \
                       (meas[(curr - i + len) % len].hour < 10 ? "0" : "") + String(meas[(curr - i + len) % len].hour) + \
                        ":" + (meas[(curr - i + len) % len].minute < 10 ? "0" : "") + String(meas[(curr - i + len) % len].minute);
      retval[i].meas = meas[(curr - i + len) % len].humidity;
    }
    return retval;
  }
  void addMeasurement(const HumidityMeas& toAdd)
  {
    curr = (curr + 1) % len;
    meas[curr] = toAdd;
  }
};
