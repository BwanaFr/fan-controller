#ifndef STATIC_CONTENT_H
#define STATIC_CONTENT_H

#include <Arduino.h>
#include <ESPAsyncWebServer.h>

void registerStaticFiles(AsyncWebServer* webServer);

#endif
