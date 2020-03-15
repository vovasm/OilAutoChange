#include <Wire.h>
#include <LiquidCrystal.h>
LiquidCrystal lcd(4, 5, 6, 7, 8, 9);  

const float calibrationFactor = 4.5; //Калибровка счетчиков 4.5 = 450 импульсов/литр
const byte StartPWM = 60;            //Минимальный ШИМ запуска насоса 0-255
const int Tik = 100;                 //Интервал обновления экрана и пересчета в мс.
volatile byte pulseCount1;  
volatile byte pulseCount2;  
unsigned long TotalMl1;
unsigned long TotalMl2;
unsigned long oldTime;
unsigned long CurMillis;

//для floatToString
char charBuf[8]; 
static byte addon_letters[16];

void init_rus(const char* letters_use ){ // добавляет к шрифтам дисплея русские буквы
  // в качестве параметра можно использовать буквы: "БГДЖЗИЙЛПУФЦЧШЩЬЪЫЭЮЯ"
  // до 16 букв, но рекомендуется не более 8
  // таблица букв
  static byte letters[][8]   = {
    { B11111, B10000, B10000, B11111, B10001, B10001, B11111, B00000 },//Б
    { B11111, B10000, B10000, B10000, B10000, B10000, B10000, B00000 },//Г
    { B01111, B01001, B01001, B01001, B01001, B11111, B10001, B00000 },//Д
    { B10101, B10101, B10101, B01110, B10101, B10101, B10101, B00000 },//Ж
    { B01110, B10001, B00001, B00110, B00001, B10001, B01110, B00000 },//З
    { B10001, B10001, B10011, B10101, B11001, B10001, B10001, B00000 },//И
    { B10101, B10101, B10011, B10101, B11001, B10001, B10001, B00000 },//Й
    { B00111, B01001, B10001, B10001, B10001, B10001, B10001, B00000 },//Л
    { B11111, B10001, B10001, B10001, B10001, B10001, B10001, B00000 },//П
    { B10001, B10001, B10001, B01111, B00001, B10001, B01110, B00000 },//У
    { B01110, B10101, B10101, B10101, B01110, B00100, B00100, B00000 },//Ф
    { B10001, B10001, B10001, B10001, B10001, B10001, B11111, B00001 },//Ц
    { B10001, B10001, B10001, B01111, B00001, B00001, B00001, B00000 },//Ч
    { B10101, B10101, B10101, B10101, B10101, B10101, B11111, B00000 },//Ш
    { B10101, B10101, B10101, B10101, B10101, B10101, B11111, B00001 },//Щ
    { B10000, B10000, B10000, B11110, B10001, B10001, B11110, B00000 },//Ь
    { B11000, B01000, B01110, B01001, B01001, B01001, B01110, B00000 },//Ъ
    { B10001, B10001, B10001, B11101, B10101, B10101, B11101, B00000 },//Ы
    { B11110, B00001, B00001, B01111, B00001, B00001, B11110, B00000 },//Э
    { B10111, B10101, B10101, B11101, B10101, B10101, B10111, B00000 },//Ю
    { B01111, B10001, B10001, B01111, B10001, B10001, B10001, B00000 },//Я
  };
  static char chars[] = {'Б', 'Г', 'Д', 'Ж', 'З', 'И', 'Й', 'Л', 'П', 'У', 'Ф', 'Ц', 'Ч', 'Ш', 'Щ', 'Ь', 'Ъ', 'Ы', 'Э', 'Ю', 'Я'};
  static byte empty[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };
  int index = 0, cl = sizeof(chars) / sizeof(char), i, j, symb;
  memset(addon_letters, 0, sizeof(addon_letters));
  for ( j = 0; j < (int)strlen(letters_use) && j < 16; j++ )
    lcd.createChar(j, empty);

  for ( j = 0; j < (int)strlen(letters_use) && j < 16; j++ )
  {
    symb = -1;
    for ( i = 0; i < cl; i++ ) if ( chars[i] == letters_use[j] ) {
        symb = i;
        addon_letters[index] = letters_use[j];
        break;
      }
    if ( symb != -1 ) {
      lcd.createChar(index, letters[symb]);
      index++;
    }
  }
}

void print_rus(char *str) {  // Печать на LCD русскими буквами с автозаменой кирилицы на латиницу
  static char rus_letters[] = {'А', 'В', 'Е', 'Ё', 'К', 'М', 'Н', 'О', 'Р', 'С', 'Т', 'Х'};
  static char trans_letters[] = {'A', 'B', 'E', 'E', 'K', 'M', 'H', 'O', 'P', 'C', 'T', 'X'};
  int lcount = sizeof(rus_letters) / sizeof(char), i, j;
  for ( i = 0; i < (int)strlen(str); i++ )
  {
    if ( byte(str[i]) == 208 ) continue; // 208 игнорируем
    int found = 0;
    for (j = 0; j < 16; j++) if ( addon_letters[j] != 0 && byte(str[i]) == byte(addon_letters[j]) ) {
        lcd.write(j);
        found = 1;
        break;
      }
    if (!found) for (j = 0; j < lcount; j++) if ( byte(str[i]) == byte(rus_letters[j]) ) {
          lcd.write(trans_letters[j]);
          found = 1;
          break;
        }
    if (!found) lcd.write(byte(str[i]));
  }
}

void print_rus(int x, int y, char *str) { // Печать на LCD русскими буквами по указанным координатам
  lcd.setCursor(x, y);
  print_rus(str);
}

char * floatToString(char * outstr, double val, byte precision, byte widthp){ //буфер под результат, число, точность (после запятой), минимальная длина
  char temp[16];
  byte i;

  //обсчёт округления 
  double roundingFactor = 0.5;
  unsigned long mult = 1;
  for (i = 0; i < precision; i++)
  {
    roundingFactor /= 10.0;
    mult *= 10;
  }
  
  temp[0]='\0';
  outstr[0]='\0';

  if(val < 0.0){
    strcpy(outstr,"-\0");
    val = -val;
  }

  val += roundingFactor;

  strcat(outstr, itoa(int(val),temp,10));  // целая часть
  if( precision > 0) {
    strcat(outstr, ".\0"); // дробная
    unsigned long frac;
    unsigned long mult = 1;
    byte padding = precision -1;
    while(precision--)
      mult *=10;

    if(val >= 0)
      frac = (val - int(val)) * mult;
    else
      frac = (int(val)- val ) * mult;
    unsigned long frac1 = frac;

    while(frac1 /= 10)
      padding--;

    while(padding--)
      strcat(outstr,"0\0");

    strcat(outstr,itoa(frac,temp,10));
  }
  
  return outstr;
}

void setup(){

  lcd.begin(16, 2); 
  init_rus("БДЛИ");                     // добавляем буквы до 16 лучше до 8
  //               12345678901234567890
  
  print_rus(0, 0, (char*)"СБРОС: ");
  print_rus(0, 1, (char*)"ДОЛИВ: ");
   
  pinMode(2, INPUT);
  pinMode(3, INPUT);
  digitalWrite(2,HIGH);
  digitalWrite(3,HIGH);

  pinMode(10, OUTPUT);
  pinMode(13, OUTPUT);
  attachInterrupt(0, pulseCounter1, FALLING); // пин D2
  attachInterrupt(1, pulseCounter2, FALLING); // пин D3
}

void loop(){
   if((millis() - oldTime) > Tik)
  { 
    CurMillis = millis() - oldTime;
    oldTime = millis();
    TotalMl1 += 1000 / CurMillis * pulseCount1 / calibrationFactor / 60 * Tik;
    TotalMl2 += 1000 / CurMillis * pulseCount2 / calibrationFactor / 60 * Tik;
    pulseCount1 = 0;
    pulseCount2 = 0;

    print_rus(7, 0, floatToString(charBuf,TotalMl1,0,8));
    print_rus(7, 1, floatToString(charBuf,TotalMl2,0,8));
    if (TotalMl1 > TotalMl2) {
        digitalWrite(13,HIGH);
        int razn = TotalMl1 - TotalMl2;
        int intens = min(razn + StartPWM,255);
        //print_rus(11, 0, left(strcat(razn,"    ")),5);
        print_rus(12, 1, strcat(floatToString(charBuf,intens/2.55,0,3),"%  "));
        analogWrite(10,intens);
      } else {
        digitalWrite(13,LOW);
        analogWrite(10,0);
        print_rus(12, 1, "  0%");
      };
  }
}

void pulseCounter1(){
  pulseCount1++;
}

void pulseCounter2(){
  pulseCount2++;
}
