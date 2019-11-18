// This #include statement was automatically added by the Spark IDE.
#include <TinyGPS.h>
//===========================================================
int armed_led = D7;
int triggered_led = D5;
int gps_power = D6;
int switch_trigger = D0; // Trap trigger switch connected to D0
int armed = 1; // variable to store whether the trap is armed
int arming_alert_sent = 0; //variable to store whether the arming alert has already been sent
int triggered_alert_sent = 0; //variable to store whether the trap trigger alert has already been sent
int triggered_queue = 0; //variable to queue up trap-triggered alert
int arming_queue = 0; //variable to queue up trap-triggered alert
String LastLocation;

FuelGauge fuel;
TinyGPS gps;

char szInfo[64];

void setup(){
    Serial1.begin(9600);
    pinMode(armed_led, OUTPUT);
    pinMode(triggered_led, OUTPUT);
    pinMode(switch_trigger, INPUT_PULLUP);

    pinMode(gps_power, OUTPUT);
    digitalWrite(gps_power, LOW);

    digitalWrite(armed_led, LOW);
    digitalWrite(triggered_led, LOW);
    Particle.function("pubstatus", PubStatus);
    fuel.quickStart();
    Particle.variable("armed", armed);
}

void loop(){

    checkgps();

    check_trap();
    
    send_alerts();
}

void check_trap() {

  if ((armed == 0) && (digitalRead(switch_trigger) == LOW)) {
    armed = 1;
    statled(1);
    arming_queue = 1;
    }

  if ((armed == 1) && (digitalRead(switch_trigger) == HIGH)) {
    armed = 0;
    statled(0);
    triggered_queue = 1;
    }

  //if ((analogRead(battery_monitor) < battery_low_limit) && (battery_alert_sent == 0)) {
    //low_bat_queue = 1;
    //battery_alert_sent = 1;
    //}

  //if ((analogRead(battery_monitor) > battery_ok_limit) && (battery_alert_sent == 1)) {
    //ok_bat_queue = 1;
    //battery_alert_sent = 0;
    //}

}

void send_alerts() {
    if (arming_queue == 1) {
      int result;
      result = PubStatus("");
      arming_queue = 0;
    }

    if (triggered_queue == 1) {
      PubStatus("");
      triggered_queue = 0;
    }
}

int PubStatus(String checkstatus) {
	String CurrentState = "";
	if (digitalRead(switch_trigger) == LOW) {
		CurrentState = "TrapArmed";
	}
	if (digitalRead(switch_trigger) == HIGH) {
		CurrentState = "TrapTriggered";
	}
    String str = LastLocation + " - Battery: " + String(long(fuel.getSoC())) + "%";
    
    bool success;
    success = Particle.publish(CurrentState, str);
    if (!success) {
          // get here if event publish did not work
          return -1;
    }
    
	//Particle.publish(CurrentState, str);
	//We got this far, so publish must have worked...
	return 1;
}

int statled(int status) {
  if (status == 1) {
    digitalWrite(armed_led, HIGH);
    digitalWrite(triggered_led, LOW);
  }
  if (status == 0) {
    digitalWrite(armed_led, LOW);
    digitalWrite(triggered_led, HIGH);
  }
}

void checkgps() {
    bool isValidGPS = false;
    for (unsigned long start = millis(); millis() - start < 1000;){
        // Check GPS data is available
        while (Serial1.available()){
            char c = Serial1.read();

            // parse GPS data
            if (gps.encode(c))
                isValidGPS = true;
        }
    }

    // If we have a valid GPS location then publish it
    if (isValidGPS){
        float lat, lon;
        unsigned long age;
        gps.f_get_position(&lat, &lon, &age);
        sprintf(szInfo, "%.6f,%.6f", (lat == TinyGPS::GPS_INVALID_F_ANGLE ? 0.0 : lat), (lon == TinyGPS::GPS_INVALID_F_ANGLE ? 0.0 : lon));
    }
    else{
        sprintf(szInfo, "0.0,0.0");
    }

    String LocationTemp(szInfo);
    if (LocationTemp != "0.0,0.0") {
        String str = "http://www.google.com/search?q=";
        LastLocation = str + LocationTemp;
    }
}
