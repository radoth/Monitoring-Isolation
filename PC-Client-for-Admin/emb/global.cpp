#pragma once
#include <QWidget>
#include <QList>
struct Weather
{
   QString date,temp,pressure,humid;
};

struct Visit
{
    QString date,id,name;
};

extern QList<Weather> weatherData;
extern QList<Visit> visitData;
