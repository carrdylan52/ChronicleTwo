#pragma once

// The map flag that records a town chest as opened, given the main map number, the chest's function point name
// and the flag the disc gives it. Returns flag_no unchanged except for chests whose retail flag another chest
// of the same map also uses (see port/src/map.cpp).
int PortTreasureBoxFlag(int map_no, const char *point_name, int flag_no);
