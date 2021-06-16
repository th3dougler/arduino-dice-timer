#include <basicMPU6050.h>
//-- Input parameters:

// Gyro settings:
#define LP_FILTER 6  // Low pass filter.           Value from 0 to 6
#define GYRO_SENS 0  // Gyro sensitivity.          Value from 0 to 3
#define ACCEL_SENS 0 // Accelerometer sensitivity. Value from 0 to 3

//buzzer output pin
#define BUZZ_PIN 10
#define LED_PIN 9

//seconds to countdown before starting timer:
#define TIMER_INTERVAL 1000
#define STABILITY_INTERVAL 500

#define COUNTDOWN_INITIAL 5
#define ORIENTATION_SENSITIVITY_HIGH 0.2
#define ORIENTATION_SENSITIVITY_LOW -0.2

//face/timer associations:
const int timerStartValues[6] = {-1, 30, 120, 180, 10, 60};

// Accelerometer offset: // Use these values to calibrate the accelerometer. The sensor should output 1.0g if held level.
constexpr int AX_OFFSET = 450;  // Use these values to calibrate the accelerometer. The sensor should output 1.0g if held level.
constexpr int AY_OFFSET = -150; // These values are unlikely to be zero.
constexpr int AZ_OFFSET = -4450;

// Output scale:
constexpr float AX_SCALE = 2; // Multiplier for accelerometer outputs. Use this to calibrate the sensor. If unknown set to 1.
constexpr float AY_SCALE = 2;
constexpr float AZ_SCALE = 2;

basicMPU6050<LP_FILTER, GYRO_SENS, ACCEL_SENS,
             AX_OFFSET, AY_OFFSET, AZ_OFFSET,
             &AX_SCALE, &AY_SCALE, &AZ_SCALE>
    imu;

/* Global variables */
bool isStable = false;
int stabilityCountdown = COUNTDOWN_INITIAL;
float pX = 0;
float pY = 0;
float pZ = 0;

int previousTime = millis();
int timerCountdownValue = -1;

void setup()
{
  // Set registers
  imu.setup();
  // Start console
  Serial.begin(38400);
  pinMode(BUZZ_PIN, OUTPUT);
  pinMode(LED_PIN, OUTPUT);
}

/* 
Template of faces:
0:  X: 0   Y: 0  Z: 1
1:  X: 0   Y: 0  Z: -1
2:  X: 0   Y: 1  Z: 0
3:  X: 0   Y: -1 Z: 0
4:  X: 1   Y: 0  Z: 0
5:  X: -1  Y: 0  Z: 0
 */
int getFace()

{
  if (imu.az() > 0.9)
    return 0;
  else if (imu.az() < -0.9)
    return 1;
  else if (imu.ay() > 0.9)
    return 2;
  else if (imu.ay() < -0.9)
    return 3;
  else if (imu.ax() > 0.9)
    return 4;
  else if (imu.ax() < -0.9)
    return 5;
}

/* 
  Compare current to previous readings, countdown if they are within margins defined by ORIENTATION_SENSITIVITY
 */
bool checkStability()
{
  //if values are unstable, reset count
  Serial.print("Stability: ");
  Serial.println(stabilityCountdown);

  if (pX - imu.ax() > ORIENTATION_SENSITIVITY_HIGH || pX - imu.ax() < ORIENTATION_SENSITIVITY_LOW ||
      pY - imu.ay() > ORIENTATION_SENSITIVITY_HIGH || pY - imu.ay() < ORIENTATION_SENSITIVITY_LOW ||
      pZ - imu.az() > ORIENTATION_SENSITIVITY_HIGH || pZ - imu.az() < ORIENTATION_SENSITIVITY_LOW)
  {
    pX = imu.ax();
    pY = imu.ay();
    pZ = imu.az();
    stabilityCountdown = COUNTDOWN_INITIAL;
  }
  else
  {
    stabilityCountdown--;
    if (stabilityCountdown <= 0)
    {
      stabilityCountdown = COUNTDOWN_INITIAL;
      return true;
    }
    else
      return false;
  }
}
/* led: 0 = only buzzer, 1= both, 2=only led  */
void buzzer(int num, int onDelay, int offDelay, int led = 0)
{
  for (int i = 0; i < num; i++)
  {
    switch (led)
    {
    case 1:
      digitalWrite(LED_PIN, HIGH);
      digitalWrite(BUZZ_PIN, HIGH);
      delay(onDelay);
      digitalWrite(LED_PIN, LOW);
      digitalWrite(BUZZ_PIN, LOW);
      delay(offDelay);
      break;
    case 2:
      digitalWrite(BUZZ_PIN, HIGH);
      delay(onDelay);
      digitalWrite(BUZZ_PIN, LOW);
      delay(offDelay);
      break;
    default:
      digitalWrite(LED_PIN, HIGH);
      delay(onDelay);
      digitalWrite(LED_PIN, LOW);
      delay(offDelay);
      break;
    }
  }
}

/* 
  Timer loop, executed every 1000ms when isStable == true;
 */
void timerLoop()
{
  if (getFace() == 0)
  {
    Serial.println("0face");
    timerCountdownValue = -1;
    isStable = false;
    return;
  }
  else if (timerCountdownValue == -1)
  {
    if (getFace() == 0)
    {
      Serial.println("No Timer Selected...");
      timerCountdownValue = -1;
      isStable = false;
      return;
    }
    buzzer(3, 100, 50, 1);
    timerCountdownValue = timerStartValues[getFace()] - 1;
  }
  else if (timerCountdownValue != 0)
    timerCountdownValue--;
  else
  {
    buzzer(3, 333, 333, 1);
    timerCountdownValue = -1;
    isStable = false;
  }
  Serial.print("timerCountdownValue: ");
  Serial.println(timerCountdownValue);
}

void render()
{
  Serial.print("isstable: ");
  if (isStable)
    Serial.println("true");
  else
    Serial.println("false");
}
bool toggleLED = false;
void flash()
{
  toggleLED = !toggleLED;
  if (toggleLED)
    digitalWrite(LED_PIN, HIGH);
  else
    digitalWrite(LED_PIN, LOW);
}
void loop()
{
  int currentTime = millis();
  if (!isStable)
  {

    /* check for stability every .5 seconds, till stability countdown is satisfied */
    if (currentTime - previousTime >= STABILITY_INTERVAL)
    {
      flash();
      previousTime = currentTime;
      isStable = checkStability();
      render();
    }
  }
  else
  {

    if (currentTime - previousTime >= TIMER_INTERVAL)
    {
      flash();
      previousTime = currentTime;
      timerLoop();
      render();
    }
  }
}