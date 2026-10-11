#pragma once

void TraceEnable();
bool TraceEnabled();
void Trace(const char *format, ...) __attribute__((format(printf, 1, 2)));
