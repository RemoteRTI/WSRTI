//
// Created by Ugo Varetto on 5/12/2016.
//
#pragma once

#include <cassert>
#include <stdexcept>
#include <libwebsockets.h>

///Reset log levels: disable logging
inline void ResetLogLevels() {
  lws_set_log_level(LLL_ERR, nullptr);
  lws_set_log_level(LLL_WARN, nullptr);
  lws_set_log_level(LLL_NOTICE, nullptr);
  lws_set_log_level(LLL_INFO, nullptr);
  lws_set_log_level(LLL_DEBUG, nullptr);
  lws_set_log_level(LLL_PARSER, nullptr);
  lws_set_log_level(LLL_HEADER, nullptr);
  lws_set_log_level(LLL_EXT, nullptr);
  lws_set_log_level(LLL_CLIENT, nullptr);
  lws_set_log_level(LLL_LATENCY, nullptr);
}

///Redirect all log messages to user-specified callback
using LWSCBack = void (*)(int level, const char* msg);
inline void SetLogCallback(LWSCBack cback) {
  lws_set_log_level(LLL_ERR
                    | LLL_WARN
                    | LLL_NOTICE
                    | LLL_INFO
                    | LLL_DEBUG
                    | LLL_PARSER
                    | LLL_HEADER
                    | LLL_EXT
                    | LLL_CLIENT
                    | LLL_LATENCY, cback);
}

///Disable logging and convert errors to exceptions
inline void InitWSThrowErrorHandler() {
    lws_set_log_level(int(LLL_ERR),
                      [](int level, const char* line) {
                          assert(level == LLL_ERR);
                          throw std::runtime_error(line);
                      });
}
