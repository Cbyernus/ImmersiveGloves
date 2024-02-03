#include "tundra_mapped_input.h"
#include "BNO085.h"
#include "Quaternion.h"
#include <cmath>

// Create TMI object to communicate with Tundra Tracker
TMI tundra_tracker;
void csn_irq( uint gpio, uint32_t event_mask );  // Chip select IRQ must be top level so let's make one and connect it later in setup

BNO085 bnoIndex;
BNO085 bnoMiddle;
BNO085 bnoRing;
BNO085 bnoPinky;

Quaternion handQuaternion;
Quaternion relativeQuaternion;

Quaternion indexQuaternion;
Quaternion middleQuaternion;
Quaternion ringQuaternion;
Quaternion pinkyQuaternion;

float getCurl(Quaternion q){
  return atan2(2 * (q.x * q.w - q.y * q.z), 1 - 2 * (q.x * q.x + q.y * q.y));
}

float getSplay(Quaternion q){
  return atan2(2 * (q.x * q.y + q.z * q.w), 1 - 2 * (q.y * q.y + q.z * q.z));
}

float getRoll(Quaternion q){
  return atan2(2 * (q.y * q.z + q.w * q.x), 1 - 2 * (q.x * q.x + q.y * q.y));
}

Quaternion quaternionFromAngle(float angle, int axis){
  // angle is in radians, x axis is 0, y axis is 1, z axis is 2
  if (axis == 0){
    Quaternion q;
    q.w = cos(angle/2);
    q.x = sin(angle/2);
    q.y = 0;
    q.z = 0;
    return q;
  }
  if (axis == 1){
    Quaternion q;
    q.w = cos(angle/2);
    q.x = 0;
    q.y = sin(angle/2);
    q.z = 0;
    return q;
  }
  if (axis == 2){
    Quaternion q;
    q.w = cos(angle/2);
    q.x = 0;
    q.y = 0;
    q.z = sin(angle/2);
    return q;
  }
}

// this will be sent by serial to the main MCU - 14 bytes are enough for 112 bits, this struct has 108
typedef struct __attribute__( ( packed, aligned( 1 ) ) )
{
  uint8_t       pinky           : 1;  //0
  uint8_t       ring            : 1;  //1
  uint8_t       middle          : 1;  //2
  uint8_t       index           : 1;  //3
  uint16_t      thumbstick_x    : 10; //4
  uint16_t      thumbstick_y    : 10; //14
  uint16_t      thumbCurl       : 10; //24
  uint16_t      thumbSplay      : 10; //34
  int16_t       refQuaternion_w : 16; //44
  int16_t       refQuaternion_x : 16; //60
  int16_t       refQuaternion_y : 16; //76
  int16_t       refQuaternion_z : 16; //92
} serial_data_t;

union serialDataUnion {
  uint8_t chars_rxed_array[14];
  serial_data_t rxed_data;
} serial_data;

void printSerialData(){
  Serial.print(serial_data.rxed_data.pinky);
  Serial.print("\t");
  Serial.print(serial_data.rxed_data.ring);
  Serial.print("\t");
  Serial.print(serial_data.rxed_data.middle);
  Serial.print("\t");
  Serial.print(serial_data.rxed_data.index);
  Serial.print("\t");
  Serial.print(serial_data.rxed_data.thumbstick_x);
  Serial.print("\t");
  Serial.print(serial_data.rxed_data.thumbstick_y);
  Serial.print("\t");
  Serial.print(serial_data.rxed_data.thumbCurl);
  Serial.print("\t");
  Serial.print(serial_data.rxed_data.thumbSplay);
  Serial.print("\t");
  Serial.print(serial_data.rxed_data.refQuaternion_w);
  Serial.print("\t");
  Serial.print(serial_data.rxed_data.refQuaternion_x);
  Serial.print("\t");
  Serial.print(serial_data.rxed_data.refQuaternion_y);
  Serial.print("\t");
  Serial.println(serial_data.rxed_data.refQuaternion_z);
}

// Make a top level struct that packs the button data along with the rest of the controller analog values
typedef struct __attribute__( ( packed, aligned( 1 ) ) )
{
  uint16_t      thumb_curl        : 10;  //0
  uint16_t      index_curl        : 10;  //10
  uint16_t      middle_curl       : 10;  //20
  uint16_t      ring_curl         : 10;  //30
  uint16_t      pinky_curl        : 10;  //40
  uint16_t      thumb_splay       : 10;  //50
  uint16_t      index_splay       : 10;  //60
  uint16_t      middle_splay      : 10;  //70
  uint16_t      ring_splay        : 10;  //80
  uint16_t      pinky_splay       : 10;  //90
  uint16_t      thumbstick_x      : 10;  //100
  uint16_t      thumbstick_y      : 10;  //110
  uint8_t       thumbstick_click  : 1;   //120
  uint8_t       a                 : 1;   //121
  uint8_t       b                 : 1;   //122
  uint8_t       system            : 1;   //123
} controller_data_t;
controller_data_t controller_data;

uint8_t received_characters[32] = {};

int chars_rxed = 0;
int uart_successes = 0;
int last_received_char = 0;
long microsLastReceived = 0;
// RX interrupt handler
void on_uart_rx() {
  while (uart_is_readable(uart1)) {
    if(micros() - microsLastReceived > 5000){
      chars_rxed = 0;
    }
    received_characters[chars_rxed & 0x1F] = uart_getc(uart1);
    microsLastReceived = micros();
    chars_rxed++;
  }
  if(chars_rxed == 17){
    interpret_serial_data();
    chars_rxed = 0;
  }
}

int interpretErrorType = 0;
int interpretSum = 0;
void interpret_serial_data(){
  if((received_characters[0] != 0x55) || (received_characters[1] != 0x55)){
    interpretErrorType = 1;
  	return;
  }
  uint8_t sum = 0;

  for(int i = 0; i < 16; i++){
    sum += received_characters[i];
  }

  interpretSum = sum;

  if(sum != received_characters[16]){
    interpretErrorType = 2;
  	return;
  }

  // now that it's confirmed data was received ok, fill serial_data union with the actual data that was received
  for(int i = 0; i < 14; i++){
    serial_data.chars_rxed_array[i] = received_characters[i + 2];
  }

  // finally parse the union serial_data into controller_data that will be sent to the tracker
  controller_data.thumb_curl = serial_data.rxed_data.thumbCurl;
  controller_data.thumb_splay = serial_data.rxed_data.thumbSplay;
  controller_data.thumbstick_x = serial_data.rxed_data.thumbstick_x;
  controller_data.thumbstick_y = serial_data.rxed_data.thumbstick_y;
  controller_data.thumbstick_click = serial_data.rxed_data.index;
  controller_data.a = serial_data.rxed_data.middle;
  controller_data.b = serial_data.rxed_data.ring;
  controller_data.system = serial_data.rxed_data.pinky;
  relativeQuaternion.w = serial_data.rxed_data.refQuaternion_w / 32767.;
  relativeQuaternion.x = serial_data.rxed_data.refQuaternion_x / 32767.;
  relativeQuaternion.y = serial_data.rxed_data.refQuaternion_y / 32767.;
  relativeQuaternion.z = serial_data.rxed_data.refQuaternion_z / 32767.;
}

float toDegrees(float angleRadians){
  return angleRadians*180/3.14;
}

int computeSplayAxis(float angleDegrees, float angleDegreesMin, float angleDegreesMiddle, float angleDegreesMax){
  int splayAxis = 512;
  if(angleDegrees <= angleDegreesMiddle) splayAxis = 512+512.*(angleDegrees-angleDegreesMiddle)/(angleDegreesMiddle-angleDegreesMin);
  else splayAxis = 512+512.*(angleDegrees-angleDegreesMiddle)/(angleDegreesMax-angleDegreesMiddle);
  if(splayAxis > 1023) splayAxis = 1023;
  else if(splayAxis < 0) splayAxis = 0;
  return splayAxis;
}

// the setup function runs once when you press reset or power the board
void setup() {

  Serial.begin(115200);
  // while(!Serial) // can't use this here, otherwise it only works with usb connected!

  delay(2000);
  Serial.println("ImmersiveGloves starting...");
  delay(1000); // wait 1 seconds, USB2 waits 3 seconds, so that we are sure USB2 has UART aligned for now - probably will make this better later

  // init the communication to Tundra Tracker, setup the CS irq callback (this has to be at Top level in arduino it seems)
  tundra_tracker.init( );
  gpio_set_irq_enabled_with_callback( tundra_tracker.get_cs_pin(), GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, true, &csn_irq );

  // Initialize I2C
  _i2c_init(i2c0, 400000);             // Init I2C0 peripheral at 400kHz
  gpio_set_function(0, GPIO_FUNC_I2C); // set pin 0 as an I2C pin (SDA in this case)
  gpio_set_function(1, GPIO_FUNC_I2C); // set pin 1 as an I2C pin (SCL in this case)
  gpio_pull_up(0);                     // use internal pull up on pin 0
  gpio_pull_up(1);                     // use internal pull up on pin 1
  Serial.println("I2C0 configured");

  _i2c_init(i2c1, 400000);             // Init I2C1 peripheral at 400kHz
  gpio_set_function(2, GPIO_FUNC_I2C); // set pin 2 as an I2C pin (SDA in this case)
  gpio_set_function(3, GPIO_FUNC_I2C); // set pin 3 as an I2C pin (SCL in this case)
  gpio_pull_up(2);                     // use internal pull up on pin 2
  gpio_pull_up(3);                     // use internal pull up on pin 3
  Serial.println("I2C1 configured");

  delay(1000);                         // Give the IMUs time to boot up

  Serial.println("Initializing Index IMU");
  bnoIndex.begin(i2c1, 0x4A);
  Serial.println("Initializing Middle IMU");
  bnoMiddle.begin(i2c1, 0x4B);
  Serial.println("Initializing Ring IMU");
  bnoRing.begin(i2c0, 0x4A);
  Serial.println("Initializing Pinky IMU");
  bnoPinky.begin(i2c0, 0x4B);

  handQuaternion.w = sqrt(2)/2;
  handQuaternion.x = 0;
  handQuaternion.y = 0;
  handQuaternion.z = -sqrt(2)/2;

  // Set up our UART with the required speed.
  uart_init(uart1, 115200);
  // Set the TX and RX pins by using the function select on the GPIO
  // Set datasheet for more information on function select
  gpio_set_function(4, GPIO_FUNC_UART);
  gpio_set_function(5, GPIO_FUNC_UART);

  // Turn off FIFO's - we want to do this character by character
  uart_set_fifo_enabled(uart1, false);

  // Set up and enable the interrupt handlers
  irq_set_exclusive_handler(UART1_IRQ, on_uart_rx);
  irq_set_enabled(UART1_IRQ, true);

  // Now enable the UART to send interrupts - RX only
  uart_set_irq_enables(uart1, true, false);

  // Reference functions below:
  // uart_putc_raw(uart1, 'A');           // Send out a character without any conversions
  // uart_putc(uart1, 'B');               // Send out a character but do CR/LF conversions
  // uart_puts(uart1, " Hello, UART!\n"); // Send out a string, with CR/LF conversions
}

void printControllerData(){
  Serial.print(controller_data.thumb_curl);
  Serial.print("\t");
  Serial.print(controller_data.index_curl);
  Serial.print("\t");
  Serial.print(controller_data.middle_curl);
  Serial.print("\t");
  Serial.print(controller_data.ring_curl);
  Serial.print("\t");
  Serial.print(controller_data.pinky_curl);
  Serial.print("\t");
  Serial.print(controller_data.thumb_splay);
  Serial.print("\t");
  Serial.print(controller_data.index_splay);
  Serial.print("\t");
  Serial.print(controller_data.middle_splay);
  Serial.print("\t");
  Serial.print(controller_data.ring_splay);
  Serial.print("\t");
  Serial.print(controller_data.pinky_splay);
  Serial.print("\t");
  Serial.print(controller_data.thumbstick_x);
  Serial.print("\t");
  Serial.print(controller_data.thumbstick_y);
  Serial.print("\t");
  Serial.print(controller_data.thumbstick_click);
  Serial.print("\t");
  Serial.print(controller_data.a);
  Serial.print("\t");
  Serial.print(controller_data.b);
  Serial.print("\t");
  Serial.println(controller_data.system);
}

int indexCurl_angleDegrees, middleCurl_angleDegrees, ringCurl_angleDegrees, pinkyCurl_angleDegrees;
int indexSplay_angleDegrees, middleSplay_angleDegrees, ringSplay_angleDegrees, pinkySplay_angleDegrees;

bool joystickIsEnabled = false;
int joystick_x = 512;
int joystick_y = 512;
int thumb_axis = 0;
int thumb_splay_axis = 0;
int index_axis = 0;
int index_splay_axis = 0;
int middle_axis = 0;
int middle_splay_axis = 0;
int ring_axis = 0;
int ring_splay_axis = 0;
int pinky_axis = 0;
int pinky_splay_axis = 0;

int millisLast = 0;
bool ledState = true;

// the loop function runs over and over again forever
void loop() {
  if(interpretErrorType != 0){
    Serial.print(interpretErrorType);
    Serial.print(" ");
    Serial.println(interpretSum);
    interpretErrorType = 0;
  }
  // if(interpretSum != 0){
  //   // Serial.println(interpretSum);
  //   printSerialData();
  //   interpretSum = 0;
  // }



  // if(millis() - millisLast > 10){
  //   ledState = !ledState;
  //   // Serial.println(ledState);
  //   // gpio_put(25, ledState);
  //   digitalWrite(LED_BUILTIN, ledState);  // turn the LED on (HIGH is the voltage level)
  //   millisLast = millis();
  //   // for(int i = 0; i < 14; i++){
  //   //   Serial.print(serial_data.chars_rxed_array[i]);
  //   //   Serial.print("\t");
  //   // }
  //   // Serial.println("");

  //   // Serial.print(controller_data.thumb_curl);
  //   // Serial.print(",");
  //   // Serial.println(controller_data.thumb_splay);
  //   // relativeQuaternion.printMe();
  // }

  // Serial.println(millis() - millisLast);
  // millisLast = millis();

  bnoIndex.getReadings();
  bnoMiddle.getReadings();
  bnoRing.getReadings();
  bnoPinky.getReadings();

  if(bnoIndex.hasNewQuaternion){
    // Serial.println("index");
    float radAccuracy;
    uint8_t accuracy;
    bnoIndex.getQuat(indexQuaternion.x, indexQuaternion.y, indexQuaternion.z, indexQuaternion.w, radAccuracy, accuracy);                    // get the index IMU quaternion
    // indexQuaternion.printMe();
    indexQuaternion = quaternion_multiply(relativeQuaternion, indexQuaternion);                                                    // rotate the indexQuaternion to be in the coordinate frame where my calculations work
    Quaternion indexToHandQuaternion = quaternion_multiply(handQuaternion, quaternion_conjugate(indexQuaternion));                 // get the relative quaternion between the reference IMU quaternion and the index IMU quaternion
    // indexToHandQuaternion.printMe();

    float indexCurl_angleRadians = getCurl(indexToHandQuaternion);                                                                        // get the curl angle in radians from the quaternion calculated above
    indexCurl_angleDegrees = toDegrees(indexCurl_angleRadians);                                                                              // convert the curl angle to degrees
    if(60 < indexCurl_angleDegrees && indexCurl_angleDegrees < 180) indexCurl_angleDegrees = -180;
    else if(0 < indexCurl_angleDegrees && indexCurl_angleDegrees <= 60) indexCurl_angleDegrees = 0;
    index_axis = 1023. * (indexCurl_angleDegrees + 180)/180;

    Quaternion indexCurlQuaternion = quaternionFromAngle(indexCurl_angleRadians, 0);                                                      // create a quaternion that represents just the amount of curl in the x axis
    Quaternion indexDecurledQuaternion = quaternion_multiply(indexCurlQuaternion, indexQuaternion);                                // rotate the indexQuaternion by the curl angle in the x axis
    Quaternion indexDecurledToHandQuaternion = quaternion_multiply(handQuaternion, quaternion_conjugate(indexDecurledQuaternion)); // get the relative quaternion between the reference IMU quaternion and the quaternion representing the index IMU rotated back by the curl angle
    float indexSplay_angleRadians = getSplay(indexDecurledToHandQuaternion);                                                              // get the splay angle in radians from the quaternion calculated above
    indexSplay_angleDegrees = toDegrees(indexSplay_angleRadians);                                                                        // convert the splay angle to degrees
    // rotate everything 180 degrees
    if(indexSplay_angleDegrees > 0) indexSplay_angleDegrees = 180 - indexSplay_angleDegrees;
    else indexSplay_angleDegrees = - 180 - indexSplay_angleDegrees;
    // computeSplayAxis(float angleDegrees, float angleDegreesMin, float angleDegreesMiddle, float angleDegreesMax)
    index_splay_axis = computeSplayAxis(indexSplay_angleDegrees, -30, 5, 25);
    // Serial.print(indexSplay_angleDegrees);
    // Serial.print(" ");
    // Serial.println(index_splay_axis);

    // scale the splay inversely by how much curl there is -> high curl, low splay
    index_splay_axis -= 512;
    index_splay_axis = index_splay_axis * (1023 - index_axis)/1023.;
    index_splay_axis += 512;

    controller_data.index_curl = index_axis;
    controller_data.index_splay = index_splay_axis;
  }

  if(bnoMiddle.hasNewQuaternion){
    // Serial.println("middle");
    float radAccuracy;
    uint8_t accuracy;
    bnoMiddle.getQuat(middleQuaternion.x, middleQuaternion.y, middleQuaternion.z, middleQuaternion.w, radAccuracy, accuracy);                    // get the middle IMU quaternion
    //middleQuaternion.printMe();
    middleQuaternion = quaternion_multiply(relativeQuaternion, middleQuaternion);                                                    // rotate the middleQuaternion to be in the coordinate frame where my calculations work
    Quaternion middleToHandQuaternion = quaternion_multiply(handQuaternion, quaternion_conjugate(middleQuaternion));                 // get the relative quaternion between the reference IMU quaternion and the middle IMU quaternion
    // middleToHandQuaternion.printMe();

    float middleCurl_angleRadians = getCurl(middleToHandQuaternion);                                                                        // get the curl angle in radians from the quaternion calculated above
    middleCurl_angleDegrees = toDegrees(middleCurl_angleRadians);                                                                              // convert the curl angle to degrees
    if(60 < middleCurl_angleDegrees && middleCurl_angleDegrees < 180) middleCurl_angleDegrees = -180;
    else if(0 < middleCurl_angleDegrees && middleCurl_angleDegrees <= 60) middleCurl_angleDegrees = 0;
    middle_axis = 1023. * (middleCurl_angleDegrees + 180)/180;

    Quaternion middleCurlQuaternion = quaternionFromAngle(middleCurl_angleRadians, 0);                                                      // create a quaternion that represents just the amount of curl in the x axis
    Quaternion middleDecurledQuaternion = quaternion_multiply(middleCurlQuaternion, middleQuaternion);                                // rotate the middleQuaternion by the curl angle in the x axis
    Quaternion middleDecurledToHandQuaternion = quaternion_multiply(handQuaternion, quaternion_conjugate(middleDecurledQuaternion)); // get the relative quaternion between the reference IMU quaternion and the quaternion representing the middle IMU rotated back by the curl angle
    float middleSplay_angleRadians = getSplay(middleDecurledToHandQuaternion);                                                              // get the splay angle in radians from the quaternion calculated above
    middleSplay_angleDegrees = toDegrees(middleSplay_angleRadians);                                                                        // convert the splay angle to degrees
    // rotate everything 180 degrees
    if(middleSplay_angleDegrees > 0) middleSplay_angleDegrees = 180 - middleSplay_angleDegrees;
    else middleSplay_angleDegrees = - 180 - middleSplay_angleDegrees;
    // computeSplayAxis(float angleDegrees, float angleDegreesMin, float angleDegreesMiddle, float angleDegreesMax)
    middle_splay_axis = computeSplayAxis(middleSplay_angleDegrees, 5, 19, 30);
    // Serial.print(middleSplay_angleDegrees);
    // Serial.print(" ");
    // Serial.println(middle_splay_axis);    

    // scale the splay inversely by how much curl there is -> high curl, low splay
    middle_splay_axis -= 512;
    middle_splay_axis = middle_splay_axis * (1023 - middle_axis)/1023.;
    middle_splay_axis += 512;

    controller_data.middle_curl = middle_axis;
    controller_data.middle_splay = middle_splay_axis;
  }

  if(bnoRing.hasNewQuaternion){
    // Serial.println("ring");
    float radAccuracy;
    uint8_t accuracy;
    bnoRing.getQuat(ringQuaternion.x, ringQuaternion.y, ringQuaternion.z, ringQuaternion.w, radAccuracy, accuracy);                    // get the ring IMU quaternion
    //ringQuaternion.printMe();
    ringQuaternion = quaternion_multiply(relativeQuaternion, ringQuaternion);                                                    // rotate the ringQuaternion to be in the coordinate frame where my calculations work
    Quaternion ringToHandQuaternion = quaternion_multiply(handQuaternion, quaternion_conjugate(ringQuaternion));                 // get the relative quaternion between the reference IMU quaternion and the ring IMU quaternion
    // ringToHandQuaternion.printMe();

    float ringCurl_angleRadians = getCurl(ringToHandQuaternion);                                                                        // get the curl angle in radians from the quaternion calculated above
    ringCurl_angleDegrees = toDegrees(ringCurl_angleRadians);                                                                              // convert the curl angle to degrees
    if(60 < ringCurl_angleDegrees && ringCurl_angleDegrees < 180) ringCurl_angleDegrees = -180;
    else if(0 < ringCurl_angleDegrees && ringCurl_angleDegrees <= 60) ringCurl_angleDegrees = 0;
    ring_axis = 1023. * (ringCurl_angleDegrees + 180)/180;

    Quaternion ringCurlQuaternion = quaternionFromAngle(ringCurl_angleRadians, 0);                                                      // create a quaternion that represents just the amount of curl in the x axis
    Quaternion ringDecurledQuaternion = quaternion_multiply(ringCurlQuaternion, ringQuaternion);                                // rotate the ringQuaternion by the curl angle in the x axis
    Quaternion ringDecurledToHandQuaternion = quaternion_multiply(handQuaternion, quaternion_conjugate(ringDecurledQuaternion)); // get the relative quaternion between the reference IMU quaternion and the quaternion representing the ring IMU rotated back by the curl angle
    float ringSplay_angleRadians = getSplay(ringDecurledToHandQuaternion);                                                              // get the splay angle in radians from the quaternion calculated above
    ringSplay_angleDegrees = toDegrees(ringSplay_angleRadians);                                                                        // convert the splay angle to degrees
    // rotate everything 180 degrees
    if(ringSplay_angleDegrees > 0) ringSplay_angleDegrees = 180 - ringSplay_angleDegrees;
    else ringSplay_angleDegrees = - 180 - ringSplay_angleDegrees;
    // computeSplayAxis(float angleDegrees, float angleDegreesMin, float angleDegreesMiddle, float angleDegreesMax)
    ring_splay_axis = computeSplayAxis(ringSplay_angleDegrees, -25, -9, 0);
    // Serial.print(ringSplay_angleDegrees);
    // Serial.print(" ");
    // Serial.println(ring_splay_axis); 

    // scale the splay inversely by how much curl there is -> high curl, low splay
    ring_splay_axis -= 512;
    ring_splay_axis = ring_splay_axis * (1023 - ring_axis)/1023.;
    ring_splay_axis += 512;

    controller_data.ring_curl = ring_axis;
    controller_data.ring_splay = ring_splay_axis;
  }

  if(bnoPinky.hasNewQuaternion){
    // Serial.println("pinky");
    float radAccuracy;
    uint8_t accuracy;
    bnoPinky.getQuat(pinkyQuaternion.x, pinkyQuaternion.y, pinkyQuaternion.z, pinkyQuaternion.w, radAccuracy, accuracy);                    // get the pinky IMU quaternion
    // pinkyQuaternion.printMe();
    pinkyQuaternion = quaternion_multiply(relativeQuaternion, pinkyQuaternion);                                                    // rotate the pinkyQuaternion to be in the coordinate frame where my calculations work
    Quaternion pinkyToHandQuaternion = quaternion_multiply(handQuaternion, quaternion_conjugate(pinkyQuaternion));                 // get the relative quaternion between the reference IMU quaternion and the pinky IMU quaternion
    // pinkyToHandQuaternion.printMe();

    float pinkyCurl_angleRadians = getCurl(pinkyToHandQuaternion);                                                                        // get the curl angle in radians from the quaternion calculated above
    pinkyCurl_angleDegrees = toDegrees(pinkyCurl_angleRadians);                                                                              // convert the curl angle to degrees
    if(60 < pinkyCurl_angleDegrees && pinkyCurl_angleDegrees < 180) pinkyCurl_angleDegrees = -180;
    else if(0 < pinkyCurl_angleDegrees && pinkyCurl_angleDegrees <= 60) pinkyCurl_angleDegrees = 0;
    pinky_axis = 1023. * (pinkyCurl_angleDegrees + 180)/180;

    Quaternion pinkyCurlQuaternion = quaternionFromAngle(pinkyCurl_angleRadians, 0);                                                      // create a quaternion that represents just the amount of curl in the x axis
    Quaternion pinkyDecurledQuaternion = quaternion_multiply(pinkyCurlQuaternion, pinkyQuaternion);                                // rotate the pinkyQuaternion by the curl angle in the x axis
    Quaternion pinkyDecurledToHandQuaternion = quaternion_multiply(handQuaternion, quaternion_conjugate(pinkyDecurledQuaternion)); // get the relative quaternion between the reference IMU quaternion and the quaternion representing the pinky IMU rotated back by the curl angle
    float pinkySplay_angleRadians = getSplay(pinkyDecurledToHandQuaternion);                                                              // get the splay angle in radians from the quaternion calculated above
    pinkySplay_angleDegrees = toDegrees(pinkySplay_angleRadians);                                                                        // convert the splay angle to degrees
    // rotate everything 180 degrees
    if(pinkySplay_angleDegrees > 0) pinkySplay_angleDegrees = 180 - pinkySplay_angleDegrees;
    else pinkySplay_angleDegrees = - 180 - pinkySplay_angleDegrees;
    // computeSplayAxis(float angleDegrees, float angleDegreesMin, float angleDegreesMiddle, float angleDegreesMax)
    pinky_splay_axis = computeSplayAxis(pinkySplay_angleDegrees, -53, -27, -5);
    // Serial.print(pinkySplay_angleDegrees);
    // Serial.print(" ");
    // Serial.println(pinky_splay_axis); 

    // scale the splay inversely by how much curl there is -> high curl, low splay
    pinky_splay_axis -= 512;
    pinky_splay_axis = pinky_splay_axis * (1023 - pinky_axis)/1023.;
    pinky_splay_axis += 512;
    
    controller_data.pinky_curl = pinky_axis;
    controller_data.pinky_splay = pinky_splay_axis;
  }

  // printControllerData();

  // Flag will be true when the library is ready for new data
  if ( tundra_tracker.data_ready() )
  {
    // Copy our controller struct to the data packet
    tundra_tracker.send_data( &controller_data, sizeof(controller_data) );

    // House keeping function for the library that should be ran right after data is ready
    tundra_tracker.handle_rx_data( );
  }
}

// Callback for SPI Chipselect, just connect in the tmi irq function
void csn_irq( uint gpio, uint32_t event_mask )
{
  tundra_tracker.csn_irq( gpio, event_mask );
}