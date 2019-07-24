/*!
 * @file veml6075_fulltest.ino
 *
 * A complete test of the library API with various settings available
 * 
 * Designed specifically to work with the VEML6075 sensor from Adafruit
 * ----> https://www.adafruit.com/products/3964
 *
 * These sensors use I2C to communicate, 2 pins (SCL+SDA) are required
 * to interface with the breakout.
 *
 * Adafruit invests time and resources providing this open source code,
 * please support Adafruit and open-source hardware by purchasing
 * products from Adafruit!
 *
 * Written by Limor Fried/Ladyada for Adafruit Industries.  
 *
 * MIT license, all text here must be included in any redistribution.
 *
 */
 
#include <Wire.h>
#include <SPI.h>
#include <Arduino.h>
#include "Adafruit_VEML6075.h"
#include "Adafruit_BLE.h"
#include "Adafruit_BluefruitLE_SPI.h"
#include "Adafruit_BluefruitLE_UART.h"
#include "BluefruitConfig.h"

#if SOFTWARE_SERIAL_AVAILABLE
  #include <SoftwareSerial.h>
#endif

/*=========================================================================
    APPLICATION SETTINGS

    FACTORYRESET_ENABLE       Perform a factory reset when running this sketch
   
                              Enabling this will put your Bluefruit LE module
                              in a 'known good' state and clear any config
                              data set in previous sketches or projects, so
                              running this at least once is a good idea.
   
                              When deploying your project, however, you will
                              want to disable factory reset by setting this
                              value to 0.  If you are making changes to your
                              Bluefruit LE device via AT commands, and those
                              changes aren't persisting across resets, this
                              is the reason why.  Factory reset will erase
                              the non-volatile memory where config data is
                              stored, setting it back to factory default
                              values.
       
                              Some sketches that require you to bond to a
                              central device (HID mouse, keyboard, etc.)
                              won't work at all with this feature enabled
                              since the factory reset will clear all of the
                              bonding data stored on the chip, meaning the
                              central device won't be able to reconnect.
    MINIMUM_FIRMWARE_VERSION  Minimum firmware version to have some new features
    MODE_LED_BEHAVIOUR        LED activity, valid options are
                              "DISABLE" or "MODE" or "BLEUART" or
                              "HWUART"  or "SPI"  or "MANUAL"
    -----------------------------------------------------------------------*/
    #define FACTORYRESET_ENABLE         1
    #define MINIMUM_FIRMWARE_VERSION    "0.6.6"
    #define MODE_LED_BEHAVIOUR          "MODE"
    #define DEBUG                       1
/*=========================================================================*/

// Create the hardware serial bluefruit object
Adafruit_BluefruitLE_UART ble(Serial1, BLUEFRUIT_UART_MODE_PIN);

// Create uv sensor object
Adafruit_VEML6075 uv = Adafruit_VEML6075();

// A small helper
void error(const __FlashStringHelper*err) {
  Serial.println(err);
  while (1);
}

int skin_type  = 1;
int SPF = 1;
float MED = 30.0;
float uv_inst = 0.0;
float uv_total = 0.0;
float uv_percent = 0.0;


/**************************************************************************/
/*!
    @brief sets up the UV sensor coefficients and integration contant
*/
/**************************************************************************/
void setup_UV(void) {
  /* Initialise the module */
  Serial.print(F("Initialising the VEML6075 sensor: "));
  Serial.println(F("---------------------------------------"));
  if (! uv.begin()) {
    Serial.println("Failed to communicate with VEML6075 sensor, check wiring?");
  }
  Serial.println("Found VEML6075 sensor");

  // Set the integration constant
  uv.setIntegrationTime(VEML6075_100MS);
  // Get the integration constant and print it!
  Serial.print("Integration time set to ");
  switch (uv.getIntegrationTime()) {
    case VEML6075_50MS: Serial.print("50"); break;
    case VEML6075_100MS: Serial.print("100"); break;
    case VEML6075_200MS: Serial.print("200"); break;
    case VEML6075_400MS: Serial.print("400"); break;
    case VEML6075_800MS: Serial.print("800"); break;
  }
  Serial.println("ms");

  // Set the high dynamic mode
  uv.setHighDynamic(false);
  // Get the mode
  if (uv.getHighDynamic()) {
    Serial.println("High dynamic reading mode");
  } else {
    Serial.println("Normal dynamic reading mode");
  }

  // Set the mode
  uv.setForcedMode(false);
  // Get the mode
  if (uv.getForcedMode()) {
    Serial.println("Forced reading mode");
  } else {
    Serial.println("Continuous reading mode");
  }

  // Set the calibration coefficients
  uv.setCoefficients(2.22, 1.33,  // UVA_A and UVA_B coefficients
                     2.95, 1.74,  // UVB_C and UVB_D coefficients
                     0.001461, 0.002591); // UVA and UVB responses
}

/**************************************************************************/
/*!
    @brief  Sets up the HW an the BLE module
*/
/**************************************************************************/
void setup_BLE(void)
{
  /* Initialise the module */
  Serial.print(F("Initialising the Bluefruit LE module: "));

  if ( !ble.begin(VERBOSE_MODE) )
  {
    error(F("Couldn't find Bluefruit, make sure it's in CoMmanD mode & check wiring?"));
  }
  Serial.println( F("OK!") );

  if ( FACTORYRESET_ENABLE )
  {
    /* Perform a factory reset to make sure everything is in a known state */
    Serial.println(F("Performing a factory reset: "));
    if ( ! ble.factoryReset() ){
      error(F("Couldn't factory reset"));
    }
  }

  /* Disable command echo from Bluefruit */
  ble.echo(false);

  Serial.println("Requesting Bluefruit info:");
  /* Print Bluefruit information */
  ble.info();

  Serial.println(F("Please use Adafruit Bluefruit LE app to connect in UART mode"));
  Serial.println(F("Then Enter characters to send to Bluefruit"));
  Serial.println();

  ble.verbose(false);  // debug info is a little annoying after this point!

  /* Wait for connection */
  while (! ble.isConnected()) {
      delay(500);
  }

  Serial.println(F("******************************"));

  // LED Activity command is only supported from 0.6.6
  if ( ble.isVersionAtLeast(MINIMUM_FIRMWARE_VERSION) )
  {
    // Change Mode LED Activity
    Serial.println(F("Change LED activity to " MODE_LED_BEHAVIOUR));
    ble.sendCommandCheckOK("AT+HWModeLED=" MODE_LED_BEHAVIOUR);
  }

  // Set module to DATA mode
  Serial.println( F("Switching to DATA mode!") );
  ble.setMode(BLUEFRUIT_MODE_DATA);

  Serial.println(F("******************************"));
}

/**************************************************************************/
/*!
    @brief  Calls UV and BLuetooth setup functions (this function is called
            automatically on startup)
*/
/**************************************************************************/
void setup(void) {
  while (!Serial);  // comment out if not debugging/ not using the serial port (i.e. running off battery)
  delay(500);
  Serial.begin(115200);
  setup_UV();
  setup_BLE();


  // Check for incoming characters from Bluefruit
  Serial.println("Enter Skin Type (1-6):");

   /* Wait for connection */
  while (!ble.available()) {}
  ble.readline();
  ble.flush();
  ble.println("SETUP");
  ble.println("Enter Skin Type (1-6):");
  while(1)
  {
    ble.readline();
    if (isdigit(ble.buffer[0])) {
      skin_type = atoi(ble.buffer);
      Serial.print("Skin type entered: "); Serial.println(skin_type);
      ble.print("Skin type entered: "); ble.println(skin_type);
      break;
    }
  }

  Serial.println(F("Enter SPF:"));
  ble.println("Enter SPF:");
  while(1)
  {
    ble.readline();
    if (isdigit(ble.buffer[0])) {
      SPF = atoi(ble.buffer);
      Serial.print("SPF enetered: "); Serial.println(SPF);
      ble.print(F("SPF enetered: ")); ble.println(SPF);
      break;
    }
  }

  // Set the MED for the skin type
  switch (skin_type) {
    case 1: MED = 150; break;
    case 2: MED = 250; break;
    case 3: MED = 300; break;
    case 4: MED = 400; break;
    case 5: MED = 600; break;
    case 6: MED = 900; break;
  }

}


/**************************************************************************/
/*!
    @brief  Constantly read UV readings and poll for new command or response data
*/
/**************************************************************************/
void loop() {

  // Read UV and calculate values to send to bluetooth
  uv_inst = uv.readUVI()*25;
  uv_total = uv_total + uv_inst;
  uv_adjusted = (uv_total/(SPF*1000));
  uv_percent = (uv_total/(MED*SPF*1000))*100;
  time_to_burn = (MED - uv_adjusted)/(uv_inst*60);
  Serial.print("UV Index reading: "); Serial.println(uv.readUVI());
  Serial.print("Current UV Energy: "); Serial.println(uv_inst);
  Serial.print("Total UV absorbed: "); Serial.println(uv_total); 
  Serial.print("Percent of UV absorbed before burn: "); Serial.println(uv_percent);
  
  // Send UV percentage to bluetooth
  ble.print("Total UV absorbed: "); ble.println(uv_total);
  ble.print("Percent of UV absorbed before burn: "); ble.println(uv_percent);
  ble.print("Time to burn: "); ble.println(time_to_burn);
  

  if(uv_percent >= 100)
  {
    Serial.println("WARNING: maximum UV absorption exceeded");
    // Send warning to bluetooth
    ble.println("MAX UV EXCEEDED");
  }
 
  delay(1000);
}
