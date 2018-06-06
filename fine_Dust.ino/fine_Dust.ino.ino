#include <SoftwareSerial.h>       //Software Serial Port
#include <Wire.h>                        // library for the i2c communication
#include <LiquidCrystal_I2C.h>        // library for LCD 1602 I2C
#include <DHT.h>                       // library for temparature&humidity sensor


#define RxD 2
#define TxD 3
#define PM25 0
#define PM10 1

#define MASTER 1              //change this macro to define the Bluetooth as Master or not 
SoftwareSerial blueToothSerial(RxD,TxD);  //the software serial port 
LiquidCrystal_I2C lcd(0x3F,16,2);     // Access address: 0x3F or 0x27


int redPin = 11;
int greenPin = 10;
int bluePin = 9;
int dustPin = 5;
float dust_value = 0;  // fine dust value 
float dustDensityug=0;

char recv_str[100]; //char array to get message

int pin = A1;  // 핀설정
DHT dht11(pin,DHT11);

unsigned long duration;

unsigned long starttime;

unsigned long recvtime;
unsigned long endtime;
unsigned long curtime;


unsigned long sampletime_ms = 30000;// Time to measure concentration

float pcsPerCF = 0;  //Initialize CF to 0 per particle

unsigned long lowpulseoccupancy = 0;


float ratio = 0;


float concentration = 0;

int flag = 0;





void setup(){
  
    
  Serial.begin(9600);   // begin serial monitor
  
  pinMode(RxD, INPUT);
  pinMode(TxD, OUTPUT);
  pinMode(dustPin,INPUT); // fine dust sensor input
  
  pinMode(4, OUTPUT);
  pinMode(6,OUTPUT);  //Output to control arduino fan
  pinMode(redPin, OUTPUT);//  to change color depending on fine dust density
  pinMode(greenPin, OUTPUT);
  pinMode(bluePin, OUTPUT); 
   lcd.init();                      // LCD initiialize
  // Print a message to the LCD.
  lcd.backlight();                // backlight on
  lcd.setCursor(0,0);             
  lcd.print("Chung-ang univ.");
  lcd.setCursor(0,1);            
  lcd.print("Capstone project");
   digitalWrite(6,HIGH);
  /*for (int i = 1; i <= 60; i++)
  {
    delay(1000); // 1s
    Serial.print(i);
    Serial.println(" s (wait 60s for DSM501 to warm up)");

    lcd.setCursor(0,0);             
    lcd.print("waiting for    ");
    lcd.setCursor(0,1);            
    lcd.print("sensor warm up    ");
  }*/

  
  Serial.println("start!");
  
  digitalWrite(6,HIGH);
  
    
    blueToothSerial.begin(9600);    //bluetooth module setup
    Serial.println("\r\npower on!");
    setupBlueToothConnection();     //initialize Bluetooth
    //this block is waiting for connection was established.
      lcd.setCursor(0,0);             
      lcd.print("Waiting for             ");
      lcd.setCursor(0,1);            
      lcd.print("Bluetooth              ");
    while(1)
    {
      
        if(recvMsg(100) == 0)
        {
            if(strcmp((char *)recv_str, (char *)"Start") == 0)//if got message, arduino starts up
            {
                Serial.println("connected\r\n");
                lcd.setCursor(0,0);
                lcd.print("Chung-ang univ.       ");
                lcd.setCursor(0,1);            
                lcd.print("Capstone project          ");
  

                starttime = millis(); 
                break;
            }
        }
        delay(200);
        
    }
    
    
    

}

void loop(){
  duration = pulseIn(dustPin, LOW);
  

  lowpulseoccupancy += duration;
  
  

  
  
  if ((millis()-starttime) > sampletime_ms)//Checking if it is time to sample
  {
    ratio = lowpulseoccupancy/(sampletime_ms*10.0);
    
    concentration = 1.1*pow(ratio,3)-3.8*pow(ratio,2)+520*ratio+0.62;
    
    pcsPerCF = concentration*100;
    
    dustDensityug = pcsPerCF/13000;
    
   
    Serial.print(dustDensityug);
    Serial.println("ug/m3");
   
   // Reading temperature or humidity takes about 250 milliseconds!
  // Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
  float humi = dht11.readHumidity();
  // Read temperature as Celsius (the default)
  float tempa = dht11.readTemperature();
  // Read temperature as Fahrenheit (isFahrenheit = true)
 

  // Check if any reads failed and exit early (to try again).
  if (isnan(humi) || isnan(tempa) ) {
    Serial.println("Failed to read from DHT sensor!");
    return;
  }
  Serial.print("Humidity: ");
  Serial.print(humi);
  Serial.print(" %\t");
  Serial.print("Temperature: ");
  Serial.print(tempa);
  Serial.print(" *C ");
  Serial.print("\n");

  

   #if MASTER  //central role
    //in master mode, the bluetooth send message periodically. 
    delay(400);
    Serial.println("send humidity and temparature\n");
     
    blueToothSerial.print("[humidity:");
    blueToothSerial.print(humi);
    blueToothSerial.print("/\n");
    delay(100);
    blueToothSerial.print("temperature:");
    blueToothSerial.print(tempa);
    blueToothSerial.print("/\n");
    delay(100);
    blueToothSerial.print("dustdensity(ug):");
    blueToothSerial.print(dustDensityug);
    blueToothSerial.print("/]\n");
    
    delay(400);
      #else   //peripheral role
    delay(200);
    //the slave role only send message when received one.
    if(recvMsg(1000) == 0)
    {
        Serial.print("recv: ");
        Serial.print((char *)recv_str);
        Serial.println("");
        Serial.println("send: ACK");
        blueToothSerial.print("ACK");  //return back message
    }
    #endif
   
    
     
    lcd.clear();
    lcd.print("humi:");
    lcd.print(humi);
    lcd.setCursor(7,0);
    lcd.print("%");
    lcd.print("temp:");
    lcd.print(tempa);
    lcd.setCursor(15,0);
    lcd.print("C");
    lcd.write(byte(0));
    // code for write dust value in LCD monitor
    lcd.write(byte(1));
    lcd.setCursor(0,1);
    lcd.print("Dust: ");
    lcd.print(dustDensityug);
    lcd.setCursor(11,1);
    lcd.write(byte(0));
    lcd.print("g/m");
    lcd.write(byte(1));
    
     if(dustDensityug <= 30.0){       // for fine dust density under 30 , print good 
     setColor(0, 255, 0);
     Serial.print("   ");
     Serial.println("Good");
     if(flag==0){
     digitalWrite(6,HIGH);
     }
     else{
      
     }
  }else if(30.0 < dustDensityug && dustDensityug <= 80.0){      // for fine dust density over 30 , print normal
    setColor(0, 0 , 255);
     Serial.print("   ");
     Serial.println("Normal"); 
      if(flag==0){
     digitalWrite(6,LOW);
     }
     else{
      
     }   
  }else if (80.0 < dustDensityug && dustDensityug <= 150.0){    // for fine dust density over 80 , print bad
     setColor(255, 255 , 0);
     Serial.print("   ");
     Serial.println("Bad");
      if(flag==0){
     digitalWrite(6,LOW);
     }
     else{
      
     }           
  }else{                                                     // for fine dust density over 150 , print warning
     setColor(255, 0, 0);
     Serial.print("   ");
     Serial.println("Warning");
      if(flag==0){
     digitalWrite(6,LOW);
     }
     else{
      
     }   
  }
  
    
  
    

    lowpulseoccupancy = 0;
   

    starttime = millis();
    
   
    

  }


/*   
  if(dustDensityug <= 30.0){       // for fine dust density under 30 , print good 
     setColor(0, 255, 0);
     Serial.print("   ");
     Serial.println("Good");
     digitalWrite(6,HIGH);
  }else if(30.0 < dustDensityug && dustDensityug <= 80.0){      // for fine dust density over 30 , print normal
    setColor(0, 0 , 255);
     Serial.print("   ");
     Serial.println("Normal"); 
     digitalWrite(6,LOW);   
  }else if (80.0 < dustDensityug && dustDensityug <= 150.0){    // for fine dust density over 80 , print bad
     setColor(255, 255 , 0);
     Serial.print("   ");
     Serial.println("Bad");
     digitalWrite(6,LOW);           
  }else{                                                     // for fine dust density over 150 , print warning
     setColor(255, 0, 0);
     Serial.print("   ");
     Serial.println("Warning");
     digitalWrite(6,LOW);   
  }
  */

 
  //get any message to print
    if(recvMsg(25) == 0)
    {
        Serial.print("recv: ");
        Serial.print((char *)recv_str);
           if(strcmp((char *)recv_str, (char *)"F/OFF") == 0)//if got message, arduino starts up
            {
                digitalWrite(6,HIGH);
                flag = 1;
                
            
            }
            if(strcmp((char *)recv_str, (char *)"F/ON") == 0)//if got message, arduino starts up
            {
                digitalWrite(6,LOW);
                flag =1;
                
            }
            if(strcmp((char *)recv_str, (char *)"Usual") == 0)//if got message, arduino starts up
            {
                flag =0;
                
            }

             if(strcmp((char *)recv_str, (char *)"warning") == 0)//if got message, arduino starts up
            {
                dustDensityug = 150;
                
            }
        
        if(strcmp((char *)recv_str, (char *)"Stop") == 0){
          digitalWrite(6,HIGH);
          
          lcd.setCursor(0,0);             
          lcd.print("-----Pauses----------");
          lcd.setCursor(0,1);            
          lcd.print("waiting start----------");
          setColor(0,0,0);
          while(1){

             if(recvMsg(100) == 0)
        {
            if(strcmp((char *)recv_str, (char *)"Start") == 0)//if got message, arduino starts up
            {
                lcd.setCursor(0,0);             
                lcd.print("Chung-ang univ.      ");
                lcd.setCursor(0,1);            
                lcd.print("Capstone project          ");
                break;
                
            }
        }
        delay(200);
            
          }
          lowpulseoccupancy = 0;
          starttime = millis();

          
        }
        Serial.println("");
    }
    
   

}
void setColor(int red, int green, int blue)
{
  analogWrite(redPin, red);
  analogWrite(greenPin, green);
  analogWrite(bluePin, blue); 
}


int strcmp(char *a, char *b)
{
    unsigned int ptr = 0;
    while(a[ptr] != '\0')
    {
        if(a[ptr] != b[ptr]) return -1;
        ptr++;
    }
    return 0;
}

//configure the Bluetooth through AT commands
int setupBlueToothConnection()
{
    #if MASTER
    Serial.println("this is MASTER\r\n");
    #else
    Serial.println("this is SLAVE\r\n");
    #endif

    Serial.print("Setting up Bluetooth link\r\n");
    delay(2000);//wait for module restart

    //wait until Bluetooth was found
    while(1)
    {
        if(sendBlueToothCommand("AT") == 0)
        {
            if(strcmp((char *)recv_str, (char *)"OK") == 0)
            {
                Serial.println("Bluetooth exists\r\n");
                break;
            }
        }
        delay(500);
    }

    //configure the Bluetooth
    sendBlueToothCommand("AT+DEFAULT");      //restore factory configurations, what parameters?
    delay(2000);

    //set role according to the macro
    sendBlueToothCommand("AT+NAME?");

    #if MASTER
    sendBlueToothCommand("AT+ROLEM");     //set to master mode
    #else
    sendBlueToothCommand("AT+ROLES");     //set to slave mode
    #endif
    sendBlueToothCommand("AT+RESTART");     //restart module to take effect

  #if ATCOMMAND_NAME_AUTH
    sendBlueToothCommand("AT+AUTH1");     //enable authentication
    sendBlueToothCommand("AT+PASS123456");    //set password
    sendBlueToothCommand("AT+NOTI1");     //set password  
  sendBlueToothCommand("AT+NAMEZETAHM-10");
  #endif
  
    delay(2000);//wait for module restart

    //check if the Bluetooth always exists
    if(sendBlueToothCommand("AT") == 0)
    {
        if(strcmp((char *)recv_str, (char *)"OK") == 0)
        {
            Serial.print("Setup complete\r\n\r\n");
            return 0;;
        }
    }

    return -1;
}

//send command to Bluetooth and return if there is a response received
int sendBlueToothCommand(char command[])
{
    Serial.print("send: ");
    Serial.print(command);
    Serial.println("");
#if NLCR
    blueToothSerial.println(command);
#else
    blueToothSerial.print(command);
#endif    
    delay(300);

    if(recvMsg(200) != 0) return -1;

    Serial.print("recv: ");
    Serial.print(recv_str);
    Serial.println("");
    return 0;
}

//receive message from Bluetooth with time out
int recvMsg(unsigned int timeout)
{
    //wait for feedback
    unsigned int time = 0;
    unsigned char num;
    unsigned char i;

    //waiting for the first character with time out
    recvtime = millis();
    i = 0;
    
    while(1)
    {
      
      delay(25);  
        if(blueToothSerial.available())
        {
            recv_str[i] = char(blueToothSerial.read());
            i++;
            break;
        }
        time++;
        if(time > (timeout / 25)){
         
          return -1;
        }
    }

    //read other characters from uart buffer to string
    while(blueToothSerial.available() && (i < 100))
    {                                              
        recv_str[i] = char(blueToothSerial.read());
        i++;
    }
#if NLCR    
    recv_str[i-2] = '\0';       //discard two character \n\r
#else
    recv_str[i] = '\0';
#endif
    return 0;
}

