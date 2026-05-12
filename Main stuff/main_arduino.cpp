#include <Arduino.h>
#include <maxx6675.h>

// set all the main pins for the arduino, maybe I can add into the ui later
const int clk = 13;
const int Do = 12;
const int reactor_cs = 10;
const int precA_cs = 8;
const int precB_cs = 7;
const int heaterR_cs = 15;
const int heaterA_cs = 16;
const int heaterB_cs = 17;

//here are all the heaters

const int heaterReactor = 9;
const int heaterPrecA   = 6;
const int heaterPrecB   = 5;
const int heaterHeaterR = 4;
const int heaterHeaterA = 3;
const int heaterHeaterB = 2;


//set up some important booleans
bool reactorOn = false;
bool precAOn   = false;
bool precBOn   = false;
bool emergencyStop  = false;
bool processRunning = false;


// set up the thermocouples
MAX6675 thermoReactor (clk, reactor_cs,  Do);
MAX6675 thermoPrecA   (clk, precA_cs,    Do);
MAX6675 thermoPrecB   (clk, precB_cs,    Do);
MAX6675 thermoHeaterR (clk, heaterR_cs,  Do);
MAX6675 thermoHeaterA (clk, heaterA_cs,  Do);
MAX6675 thermoHeaterB (clk, heaterB_cs,  Do);

//valves
const int valveA      = 2;
const int valveB      = 3;
const int valvePurge  = 4;
const int valveVacuum = 11;
const int valveVent   = 14;

//other important stuff
float reactorSetpoint = 0;
float precASetpoint   = 0;
float precBSetpoint   = 0;




void setup() {
    Serial.begin(9600);

    pinMode(heaterReactor, OUTPUT);
    pinMode(heaterPrecA,   OUTPUT);
    pinMode(heaterPrecB,   OUTPUT);
    digitalWrite(heaterReactor, HIGH);
    digitalWrite(heaterPrecA,   HIGH);
    digitalWrite(heaterPrecB,   HIGH);

    pinMode(valveA,      OUTPUT);
    pinMode(valveB,      OUTPUT);
    pinMode(valvePurge,  OUTPUT);
    pinMode(valveVacuum, OUTPUT);
    pinMode(valveVent,   OUTPUT);

    digitalWrite(valveA,      HIGH);
    digitalWrite(valveB,      HIGH);
    digitalWrite(valvePurge,  HIGH);
    digitalWrite(valveVacuum, HIGH);
}

void handleCommand(String cmd) {
    if (cmd.startsWith("R:")) {
        reactorSetpoint = cmd.substring(2).toFloat();
        reactorOn = true;
        Serial.print("Reactor set to ");
        Serial.println(reactorSetpoint);
    }
    else if (cmd.startsWith("A:")) {
        precASetpoint = cmd.substring(2).toFloat();
        precAOn = true;
        Serial.print("PrecA set to ");
        Serial.println(precASetpoint);
    }
    else if (cmd.startsWith("B:")) {
        precBSetpoint = cmd.substring(2).toFloat();
        precBOn = true;
        Serial.print("PrecB set to ");
        Serial.println(precBSetpoint);
    }
    else if (cmd.startsWith("VALVE:")) {
        int c1 = cmd.indexOf(':');
        int c2 = cmd.indexOf(':', c1 + 1);
        String valveName = cmd.substring(c1 + 1, c2);
        String action    = cmd.substring(c2 + 1);
        controlValve(valveName, action);
    }
    else if (cmd == "START_PROCESS") {
        processRunning = true;
        Serial.println("Process started");
    }
    else if (cmd == "STOP_PROCESS") {
        processRunning = false;
        Serial.println("Process stopped");
    }
    else if (cmd == "STOP") {
        reactorOn = false;
        precAOn   = false;
        precBOn   = false;
        digitalWrite(heaterReactor, HIGH);
        digitalWrite(heaterPrecA,   HIGH);
        digitalWrite(heaterPrecB,   HIGH);
        Serial.println("All stopped");
    }
    else if (cmd == "EMERGENCY_STOP") {
        emergencyStop = true;
        handleEmergencyStop();
    }
    else if (cmd == "TEST") {
        Serial.println("Test OK");
    }
    else if (cmd == "STATUS") {
        sendStatus();
    }
    else {
        Serial.print("Unknown command: ");
        Serial.println(cmd);
    }
}

void controlValve(String valveName, String action) {
    bool open = false;
    if (action == "OPEN") {
        open = true;
    }
    int pin = -1;

      if (valveName == "Precursor_A") {
        pin = valveA;
    }
    else if (valveName == "Precursor_B") {
        pin = valveB;
    }
    else if (valveName == "Purge_Gas") {
        pin = valvePurge;
    }
    else if (valveName == "Vacuum") {
        pin = valveVacuum;
    }
    else if (valveName == "Vent") {
        pin = valveVent;
    }


    if (pin != -1) {
        digitalWrite(pin, open ? LOW : HIGH);
        Serial.print("Valve ");
        Serial.print(valveName);
        Serial.print(" ");
        Serial.println(action);
    } else {
        Serial.print("Unknown valve: ");
        Serial.println(valveName);
    }
}



void loop() {
    // Check for emergency stop first

    if (emergencyStop) { 
        handleEmergencyStop(); 
        return; 
    }

    //handle the  inputs from the python app
    if (Serial.available()) {
        String cmd = Serial.readStringUntil('\n');
        cmd.trim();
        handleCommand(cmd);
    }

    static unsigned long lastReport = 0;
    if (millis() - lastReport >= 500) {
        lastReport = millis();

        float reactorTemp = thermoReactor.readCelsius();
        float precATemp   = thermoPrecA.readCelsius();
        float precBTemp   = thermoPrecB.readCelsius();
        float heaterRTemp = thermoHeaterR.readCelsius();
        float heaterATemp = thermoHeaterA.readCelsius();
        float heaterBTemp = thermoHeaterB.readCelsius();

        //I added the 1 to the end so that it rounds

        Serial.print("TEMP:Reactor:");       
        Serial.println(reactorTemp, 1);
        Serial.print("TEMP:Precursor_A:"); 
        Serial.println(precATemp,   1);
        Serial.print("TEMP:Precursor_B:");   
        Serial.println(precBTemp,   1);
        Serial.print("TEMP:Heater_Reactor:"); 
        Serial.println(heaterRTemp, 1);
        Serial.print("TEMP:Heater_PrecA:");   
        Serial.println(heaterATemp, 1);
        Serial.print("TEMP:Heater_PrecB:");   
        Serial.println(heaterBTemp, 1);

        digitalWrite(heaterReactor, (reactorOn && reactorTemp < reactorSetpoint) ? LOW : HIGH);
        digitalWrite(heaterPrecA,   (precAOn   && precATemp   < precASetpoint)   ? LOW : HIGH);
        digitalWrite(heaterPrecB,   (precBOn   && precBTemp   < precBSetpoint)   ? LOW : HIGH);
        //this should jsut turn on the heaters is the temperature is too low and turn them off otherwise
        //PID loop didn't work as well as this
    }
}


void handleEmergencyStop() {
    //since they're off high, this just turns everything off, seemed to work well in testing
    digitalWrite(heaterReactor, HIGH);
    digitalWrite(heaterPrecA,   HIGH);
    digitalWrite(heaterPrecB,   HIGH);

    reactorOn = false;
    precAOn   = false;
    precBOn   = false;

    digitalWrite(valveA,      HIGH);
    digitalWrite(valveB,      HIGH);
    digitalWrite(valvePurge,  HIGH);
    digitalWrite(valveVacuum, HIGH);
    digitalWrite(valveVent,   HIGH);

    processRunning = false;
    Serial.println("EMERGENCY STOP");

    while (emergencyStop) {
        if (Serial.available()) {
            String cmd = Serial.readStringUntil('\n');
            cmd.trim();
            if (cmd == "RESET_EMERGENCY") {
                emergencyStop = false;
                Serial.println("Reset");
            }
        }
        delay(100);
    }
}

void sendStatus() {
    //found out how to do the bool ? something: something else and it's pretty cool
    Serial.println("STATUS_START");
    Serial.print("Reactor_On:");       Serial.println(reactorOn ? "YES" : "NO");
    Serial.print("Reactor_Setpoint:"); Serial.println(reactorSetpoint, 1);
    Serial.print("PrecA_On:");         Serial.println(precAOn   ? "YES" : "NO");
    Serial.print("PrecA_Setpoint:");   Serial.println(precASetpoint,   1);
    Serial.print("PrecB_On:");         Serial.println(precBOn   ? "YES" : "NO");
    Serial.print("PrecB_Setpoint:");   Serial.println(precBSetpoint,   1);
    Serial.print("Process_Running:");  Serial.println(processRunning ? "YES" : "NO");
    Serial.println("STATUS_END");
}
