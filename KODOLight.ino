#include <TuyaWifi.h>
#include <SoftwareSerial.h>
#include <FastLED.h>

#define DATA_PIN    6
#define LED_TYPE    WS2812
#define COLOR_ORDER GRB
#define NUM_LEDS    60

uint32_t BRIGHTNESS = 80;  //亮度 
uint32_t GradualChange;    //渐变控制 
uint8_t gHue = 0;          // 特性使用的旋转“基色”
//colour字符串装换为数字
unsigned int H = 0X00;
unsigned int S = 0X00;
unsigned int V = 0X00;
unsigned int dp_Number_Colour[12] = {0};
unsigned int dp_Number_Scene[28] = {0};

CRGB leds[NUM_LEDS];
//HSV方法定义颜色  
CHSV myHSVcolor(0,0,0);    //HSV方法定义颜色  myHSVcolor（色调，饱和度，明亮度）

TuyaWifi my_device;
SoftwareSerial DebugSerial(8,9);  //RX为D8，TX为D9

/* Current LED status */
unsigned char led_state = 0;
/* Connect network button pin */
int key_pin = 7;

/* Data point define */
#define DPID_Switch    20   //开关
#define DPID_WorkMode  21   //模式
#define DPID_Bright    22   //亮度
#define DPID_Temp      23   //色温
#define DPID_Colour    24   //彩光
#define DPID_Scene     25   //场景
#define DPID_Countdown 26   //倒计时
#define DPID_Music     27   //音乐律动
#define DPID_Control   28   //调节
#define DPID_SleepMode       31   //入睡
#define DPID_WakeupMode      32   //唤醒
#define DPID_PowerMemory     33   //断电记忆
#define DPID_DoNotDisturb    34   //勿扰模式
#define DPID_DreamlightScene      51   //幻彩情景
#define DPID_DreamlightmicMusic   52   //幻彩本地音乐律动
#define DPID_LightpixelNumberSet  53   //点数/长度设置

/* Current device DP values */
unsigned char dp_bool_Switch = 0;
unsigned char dp_enum_WorkMode = 0;
long dp_value_Bright = 0;
long dp_value_Temp = 0;
unsigned char dp_string_Colour[12] = {"000000000000"};
unsigned char dp_string_Scene[28] = {"0000000000000000000000000000"};
unsigned char dp_string_Control[21] = {"000000000000000000000"};

/* Stores all DPs and their types. PS: array[][0]:dpid, array[][1]:dp type. 
 *                                     dp type(TuyaDefs.h) : DP_TYPE_RAW, DP_TYPE_BOOL, DP_TYPE_VALUE, DP_TYPE_STRING, DP_TYPE_ENUM, DP_TYPE_BITMAP
*/
unsigned char dp_array[][2] =
{
  {DPID_Switch, DP_TYPE_BOOL},
  {DPID_WorkMode, DP_TYPE_ENUM},
  {DPID_Bright, DP_TYPE_VALUE},
  {DPID_Temp, DP_TYPE_VALUE},
  {DPID_Colour, DP_TYPE_STRING},
  {DPID_Scene, DP_TYPE_STRING},
  {DPID_Control, DP_TYPE_STRING},
};

unsigned char pid[] = {"qwzxnhzfs18w68b7"};
unsigned char mcu_ver[] = {"1.0.0"};

/* last time */
unsigned long last_time = 0;

//FastLED相关初始化
void PixelsInit()
{
  delay(1000);  
  // 告诉FastLED关于RGB LED的配置
  FastLED.addLeds<LED_TYPE,DATA_PIN,COLOR_ORDER>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);
  // 设置主亮度控制
  FastLED.setBrightness(BRIGHTNESS);
}

// FastLED的内置彩虹特效
void rainbow() 
{
  fill_rainbow( leds, NUM_LEDS, gHue, 7);
}

void setup() 
{
  // Serial.begin(9600);
  Serial.begin(9600);
  DebugSerial.begin(9600);

  //Initialize led port, turn off led.
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);

  //Initialize networking keys.
  pinMode(key_pin, INPUT_PULLUP);

  PixelsInit();

  //Enter the PID and MCU software version
  my_device.init(pid, mcu_ver);
  //incoming all DPs and their types array, DP numbers
  my_device.set_dp_cmd_total(dp_array, 7);
  //register DP download processing callback function
  my_device.dp_process_func_register(dp_process);
  //register upload all DP callback function
  my_device.dp_update_all_func_register(dp_update_all);

  last_time = millis();
}

void loop() 
{
  my_device.uart_service();

  //Enter the connection network mode when Pin7 is pressed.
  if (digitalRead(key_pin) == LOW) {
    delay(80);
    if (digitalRead(key_pin) == LOW) {
      my_device.mcu_set_wifi_mode(SMART_CONFIG);
    }
  }
  /* LED blinks when network is being connected */
  if ((my_device.mcu_get_wifi_work_state() != WIFI_LOW_POWER) && (my_device.mcu_get_wifi_work_state() != WIFI_CONN_CLOUD) && (my_device.mcu_get_wifi_work_state() != WIFI_SATE_UNKNOW)) {
    if (millis()- last_time >= 500) {
      last_time = millis();

      if (led_state == LOW) {
        led_state = HIGH;
      } else {
        led_state = LOW;
      }
      digitalWrite(LED_BUILTIN, led_state);
    }
  }
  delay(10);
}

//====================================//
unsigned int Char_Hex(unsigned char a)
{
  if(a == '0') return 0x00;
  if(a == '1') return 0x01;
  if(a == '2') return 0x02;
  if(a == '3') return 0x03;
  if(a == '4') return 0x04;
  if(a == '5') return 0x05;
  if(a == '6') return 0x06;
  if(a == '7') return 0x07;
  if(a == '8') return 0x08;
  if(a == '9') return 0x09;
  if(a == 'a') return 0x0a;
  if(a == 'b') return 0x0b;
  if(a == 'c') return 0x0c;
  if(a == 'd') return 0x0d;
  if(a == 'e') return 0x0e;
  if(a == 'f') return 0x0f;
}

void Colour_HSV(unsigned char *a)
{ 
  H = (Char_Hex(a[0])<<12) + (Char_Hex(a[1])<<8) + (Char_Hex(a[2])<<4) + Char_Hex(a[3]);
  S = (Char_Hex(a[4])<<12) + (Char_Hex(a[5])<<8) + (Char_Hex(a[6])<<4) + Char_Hex(a[7]);
  V = (Char_Hex(a[8])<<12) + (Char_Hex(a[9])<<8) + (Char_Hex(a[10])<<4) + Char_Hex(a[11]);
  H = map(H, 0, 360, 0, 255);
  S = map(S, 0, 1000, 0, 255);
  V = map(V, 0, 1000, 0, 255);
}
//====================================//

/**
 * @description: DP download callback function.
 * @param {unsigned char} dpid
 * @param {const unsigned char} value
 * @param {unsigned short} length
 * @return {unsigned char}
 */
unsigned char dp_process(unsigned char dpid,const unsigned char value[], unsigned short length)
{
  switch (dpid) {
      case DPID_Switch:
          DebugSerial.println("Bool type:");
          dp_bool_Switch = my_device.mcu_get_dp_download_data(dpid, value, length);
          DebugSerial.println(dp_bool_Switch);
          //自定义开关控制//
          if(dp_bool_Switch == 0)
          {
            for(uint32_t i = 0 ; i < 60; i++) leds[i] = 0x000000;
            FastLED.show();  // 发送'LEDS'阵列到实际的LED灯带
            delay(10);
          }
          else
          {
           for(uint32_t i = 0 ; i < 60; i++) leds[i] = 0xFFFFFF;
            FastLED.show();
            delay(10);
          }
          /* After processing the download DP command, the current status should be reported. */
          my_device.mcu_dp_update(DPID_Switch, dp_bool_Switch, 1);
       break;

       case DPID_WorkMode:
          DebugSerial.println("Enum type:");
          dp_enum_WorkMode = my_device.mcu_get_dp_download_data(dpid, value, length);
          DebugSerial.println(dp_enum_WorkMode);
          //自定义模式设置//
          if(dp_enum_WorkMode == 0)      //白光模式-只亮白灯
          {
            for(uint32_t i = 0 ; i < 60; i++) leds[i] = 0xFFFFFF;
            FastLED.show();
          } 
          else if(dp_enum_WorkMode == 3) //音乐模式
          {
            rainbow();   
            FastLED.show();
          }
          /* After processing the download DP command, the current status should be reported. */
          my_device.mcu_dp_update(DPID_WorkMode, dp_enum_WorkMode, 1);
       break;
            
       case DPID_Bright:
          DebugSerial.println("Value type:");
          dp_value_Bright = my_device.mcu_get_dp_download_data(DPID_Bright, value, length);
          DebugSerial.println(dp_value_Bright);
          //自定义亮度设置//
          BRIGHTNESS = map(dp_value_Bright, 0, 1000, 0, 255);
          FastLED.setBrightness(BRIGHTNESS);
          FastLED.show();
          /* After processing the download DP command, the current status should be reported. */
          my_device.mcu_dp_update(DPID_Bright, dp_value_Bright, 1);
       break;
       
       case DPID_Temp:
          DebugSerial.println("Value type:");
          dp_value_Temp = my_device.mcu_get_dp_download_data(DPID_Temp, value, length);
          DebugSerial.println(dp_value_Temp);
          /* After processing the download DP command, the current status should be reported. */
          my_device.mcu_dp_update(DPID_Temp, dp_value_Temp, 1);
       break;

       case DPID_Colour:
          DebugSerial.println("String type:");
          /*  */
          for (unsigned int i=0; i<length; i++) {
              dp_string_Colour[i] = value[i];
              DebugSerial.write(dp_string_Colour[i]);
          }
          DebugSerial.println("");
          //自定义颜色控制//
          Colour_HSV(dp_string_Colour);
          myHSVcolor.h = H;
          myHSVcolor.s = S;
          myHSVcolor.v = BRIGHTNESS;
          fill_solid(leds, NUM_LEDS, myHSVcolor);   
          FastLED.show();        
          /* After processing the download DP command, the current status should be reported. */
          my_device.mcu_dp_update(DPID_Colour, dp_string_Colour, length);
       break;

       case DPID_Scene:
          DebugSerial.println("String type:");
          /*  */
          for (unsigned int i=0; i<length; i++) {
              dp_string_Scene[i] = value[i];
              DebugSerial.write(dp_string_Scene[i]);
          }
          DebugSerial.println("");
          /* After processing the download DP command, the current status should be reported. */
          my_device.mcu_dp_update(DPID_Scene, dp_string_Scene, length);
       break;

       case DPID_Control:
          DebugSerial.println("String type:");
          /*  */
          for (unsigned int i=0; i<length; i++) {
              dp_string_Control[i] = value[i];
              DebugSerial.write(dp_string_Control[i]);
          }
          DebugSerial.println("");
          
          for (unsigned int i=0; i<12; i++) {
              dp_string_Colour[i] = dp_string_Control[i+1];
              DebugSerial.write(dp_string_Colour[i]);
          }
          //自定义颜色控制//
          Colour_HSV(dp_string_Colour);
          myHSVcolor.h = H;
          myHSVcolor.s = S;
          myHSVcolor.v = BRIGHTNESS;
          fill_solid(leds, NUM_LEDS, myHSVcolor);   
          FastLED.show();        
          /* After processing the download DP command, the current status should be reported. */
          my_device.mcu_dp_update(DPID_Colour, dp_string_Colour, length);
       break;

       default:
          break;
  }
  return SUCCESS;
}

/**
 * @description: Upload all DP status of the current device.
 * @param {*}
 * @return {*}
 */
void dp_update_all(void)
{
  my_device.mcu_dp_update(DPID_Switch, dp_bool_Switch, 1);
  my_device.mcu_dp_update(DPID_WorkMode, dp_enum_WorkMode, 1);
  my_device.mcu_dp_update(DPID_Bright, dp_value_Bright, 1);
  my_device.mcu_dp_update(DPID_Temp, dp_value_Temp, 1);
  my_device.mcu_dp_update(DPID_Colour, dp_string_Colour, (sizeof(dp_string_Colour) / sizeof(dp_string_Colour[0])));
  my_device.mcu_dp_update(DPID_Scene, dp_string_Scene, (sizeof(dp_string_Scene) / sizeof(dp_string_Scene[0])));
}
