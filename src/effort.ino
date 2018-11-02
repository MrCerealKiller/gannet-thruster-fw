/**
 * ######################################
 * ## Effort Controller for the Roboat ##
 * ######################################
 *
 * Notes:
 *   - When using the RPi commands, the raw commands are interpretted,
 *     limited, and sent to the ESC's without any transform
 *   - When using RC commands, there is a mathematical transform that must be
 *     used applied to handle multi-axis commands on the diff-drive
 *
 * TODO:
 *   - Resolve notes and Questions
 *   - Pull some constants out to either params or defs to make the system more
 *     generic and able to be configured at a moment's notice
 */

#include <Arduino.h>
#include <Servo.h>

#define spinRate 1000 // Delay between loop cycles (applied at the end)
#define lPin 11
#define rPin 10

/**
 * Servo Objects control the ESC's using the writeMicroseconds function
 * One servo object is created per thruster
 */
Servo myservoL;
Servo myservoR;

/**
 * Signals for the ESC's
 * Initialized to 1500 us --> Stop, as per BR-T200 specs
 */
int nL = 1500;
int nR = 1500;

/**
 * Stores incoming PWM data from RPi or RC module
 * Ch 3 --> pin 3 : is for controlling the Motion (forward / reverse)
 * ch 4 --> pin 4 : is for controlling the Rottion (left / right)
 */
int ch3;
int ch4;

/**
 * Designates whether the arduino is receiving commands from the RPi or RC unit
 */
int rc_enabled = 1;

/**
 * Used to specify any forward motion, for mathematical transformation
 */
int forward = 0;

/**
 * Data to store final movement data and motor signals
 */
int motion = 0;
int turnL = 0;
int turnR = 0;


/**
 * Strings to hold and differentiate incoming data
 */
String inputString = "";  // Raw Input
String servoL;            // Left side substring
String servoR;            // Right side substring

/**
 * Logical flag for whether or not the arduino is ready to interpret a command
 */
boolean stringComplete = false;

// =============================================================================

void setup() {

  /**
   * Initialize Channel Input Pins
   */
  pinMode(3, INPUT);
  pinMode(4, INPUT);

  /**
   * Initialize Servos on their appropriate pin
   */
  myservoL.attach(lPin);
  myservoR.attach(rPin);

  /**
   * Set Initial Positions
   */
  myservoL.writeMicroseconds(nL);
  myservoR.writeMicroseconds(nR);
  //delay(1000);

  Serial.begin(115200);

  /**
   * Reserve 200 bytes for input string
   */
  inputString.reserve(9);

  // Serial.println("Thrusters ready");
}

// =============================================================================

void loop() {

  /**
   * Read the Pulse-width of each channel
   */
  ch3 = pulseIn(3, HIGH, 25000);
  ch4 = pulseIn(4, HIGH, 25000);
  // delay(10);
  // Serial.print("Ch3: ");
  // Serial.println(ch3);

  /**
   * If there is no input on the ch3, then the RPi assumes control
   * (Manual teleop is prioritized)
   */
  if (ch3 == 0) {

    rc_enabled = 0; // Set flag
    //Serial.println("RC is disabled");

    /**
     * Only continue if an appropriate string has been received
     * from the serial event (see below)
     *
     * The arduino expects input like "07002100" that has both thruster
     * commands embedded.
     */
    if (stringComplete) {
      // Serial.println(inputString);

      servoL = inputString.substring(0, 4); // First 4 chars --> Left cmd
      servoR = inputString.substring(4, 8); // Next 4 chars --> Right cmd

      /**
       * Magically convert strings to useable ints
       */
      char carray1[6];
      servoL.toCharArray(carray1, sizeof(carray1));
      nL = atoi(carray1);

      char carray2[6];
      servoR.toCharArray(carray2, sizeof(carray2));
      nR = atoi(carray2);
      // MAGIC ENDS ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~

      /**
       * ORIGINAL
       * !=!=!=!=!=!=!=!=!=!=!=!=!=!=!=!=!=!=!=!=!=!=!=!=!=!=!=!=!=!=!=!=!=!=!=!
       *
       * Why do these thresholds need to be in place? The values are more
       * appropriately truncated later on?
       *
       *  if ((nL < 1200) || (nL > 1800) || (nR < 1200) || (nR > 1800)) {
       *   nL = 1500;
       *   nR = 1500;
       * }
       *
       * !=!=!=!=!=!=!=!=!=!=!=!=!=!=!=!=!=!=!=!=!=!=!=!=!=!=!=!=!=!=!=!=!=!=!=!
       */

      /**
       * Reset the input (String, buffer, and flags)
       */
      inputString = "";
      Serial.flush();
      stringComplete = false;
   }

   /**
    * The input has not been properly received;
    * Don't attempt new commands
    */
   else {
      nL = 1500;
      nR = 1500;
   }
  }

  /**
   * ch3 contains some Data, so the RC modules commands are used
   */
  else {
    //Serial.print("Ch3: ");
    //Serial.println(ch3);
    //Serial.print("Ch4: ");
    //Serial.println(ch4);

    rc_enabled = 1; // Set flag

    motion = 1500;  // Default Motion

    /**
     * When ch3 contains more than 1550, there is forward movement
     */
    if (ch3 > 2200) {
      forward = 1;  // Set flag

      motion = map(ch3, 2200, 2600, 1500, 1800); // Cap at 1800 (Max is 1900)
    }

    /**
     * When ch3 contains less than 1300, there is reverse movement
     */
    else if (ch3 < 1800) {
      forward = 0;  // Set flag

      motion = map(ch3, 1800, 1400, 1500, 1200); // Cap at 1200 (Min is 1100)
    }

    /**
     * When ch3 is between 1300 and 1550, there is no linear motion
     */
    else {
      forward = 0;
    }

    /**
     * Reset Turn Motion Values
     */
    turnL = 0;
    turnR = 0;

    // Serial.println(ch4);

    /**
     * If ch4 contains more than 1350, a left turn is evoked
     */
    if (ch4 > 1900) {
      turnL = map(ch4, 1900, 2400, 1, 300); // 1-300 Radial transform factors
    }

    /**
     * If ch4 contains less than 1250, a right turn is evoked
     */
    else if (ch4 < 1500) {
      turnR = map(ch4, 1500, 1200, 1, 300); // 1-300 Radial transform factors
    }

    /**
     * Decide the final efforts from the RC commands using a simple transform:
     *
     * When going in the positive (forward) direction, slightly reduce the
     * effort on the appropriate side to cause the desired torque
     *
     * When going in the negative (reverse) direction, slightly increase the
     * effort on the appropraite side to cause the desired torque
     */
    if (forward) {
      nL = motion - turnL;
      nR = motion - turnR;
    } else {
      nL = motion + turnR;
      nR = motion + turnL;
    }

    /**
     * Reset RC flag, in case connection drops out so system can revert to
     * the RPi's commands
     */
    rc_enabled = 0;
    Serial.flush();
  }

  /**
   * ##################################
   * ## LIMIT AND SEND FINAL EFFORTS ##
   * ##################################
   */
  //Serial.print("Left Cmd: ");
  //Serial.println(nL);
  //Serial.print("Right Cmd: ");
  //Serial.println(nR);
  //Serial.println("-----------");
  if (nL > 1800) { nL = 1800; }
  if (nL < 1200) { nL = 1200; }
  if (nR > 1800) { nR = 1800; }
  if (nR < 1200) { nR = 1200; }
  myservoL.writeMicroseconds(nL);
  myservoR.writeMicroseconds(nR);
  delay(spinRate);
}

// =============================================================================

/**
 * SerialEvent occurs whenever a new data is received on the hardware serial RX
 *
 * This routine is run between each time loop() runs, so using delay inside
 * loop can delay response
 *
 * Multiple bytes of data may be available
 */
void serialEvent() {
  /**
   * If in RC command mode, do nothing...
   */
  if(rc_enabled == 0) {
    /**
     * While there is an incoming serial packet, decode each character and
     * add it to the input string
     *
     * The input is delimited by a ";" which is ignored by the rest of the code
     */
    while (Serial.available() && !stringComplete) {
      char inChar = (char)Serial.read();
      inputString += inChar;

      if (inChar == ';') {
        if (inputString.length() == 9) {
          stringComplete = true;
        }

        /**
         * Invalid input...
         */
        else {
          stringComplete = false;
          inputString = "";
        }
      }

      /**
       * Second barrier for invalid input
       */
      if (inputString.length() == 9 && !stringComplete) {
        inputString = "";
      }
    } // END LOOP
  } // DO NOTHING
}
