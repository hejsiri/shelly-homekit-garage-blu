#include "shelly_blu_bthome.hpp"
using namespace shelly::blu;
static Sensor sensors[2];
extern "C" {
void blu_reset() { sensors[0] = Sensor(); sensors[1] = Sensor(); }
int blu_receive(int role, const unsigned char *data, unsigned long size, double now) {
  if (role < 0 || role > 1) return 0;
  Reading reading;
  if (!DecodeAdvertisement(data, size, &reading)) return 0;
  sensors[role].Update(reading, now, -58 - role * 7);
  return 1;
}
int blu_ready(double now, int timeout) { return Ready(sensors[0], sensors[1], now, timeout); }
int blu_active(int role) { return sensors[role].active; }
int blu_seen(int role) { return sensors[role].seen; }
double blu_age(int role, double now) { return sensors[role].seen ? now - sensors[role].last_seen : -1; }
}
