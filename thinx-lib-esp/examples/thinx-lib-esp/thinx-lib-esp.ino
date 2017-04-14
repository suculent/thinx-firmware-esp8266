#include "THiNX.h"

THiNX thx();

void setup() {
  
}

void loop() {
  testCheckinWithTHINX(&thx);
  testUpdateWithTHINX(&thx);  
  delay(1000);
}

void testCheckinWithTHINX(THiNX* THiNX) {
  THiNX->checkin();
}

void testUpdateWithTHINX(THiNX* THiNX) {
  THiNX->update();
}
