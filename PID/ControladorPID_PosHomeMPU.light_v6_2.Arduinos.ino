
/*________________________________________________
Código PID sensores  de retroalimentacion HMC5883 MPU6050 con Pantalla LCD
Sensores de retroalimentacion HMC5883 | MPU6050
    Posición inicio (home) agregado 

         Librería MPU6050_light
  Hecho por: Alonso Damian Muñoz Rico ICE
    Trabajando correctamente 15/10/22
__________________________________________________*/
#include <Separador.h> //Librería anexa
//#include <MPU6050_tockn.h>  //incluye librería MPU6050

#include <MPU6050_light.h> //incluye librería MPU6050
#include <MechaQMC5883.h> // incluye libreria para magnetometro QMC5883L
#include <Wire.h> //Inicializa comunicacion I2C
#include <EEPROM.h>
//#include <LiquidCrystal_I2C.h> //Pantalla LC D
#include <Tone.h>
#include <PID_v1.h>

int notes[] = { NOTE_A4,
                NOTE_B4,
                NOTE_C4,
                NOTE_D4,
                NOTE_E4,
                NOTE_F4,
                NOTE_G4 };
//LiquidCrystal_I2C lcd(0x27,20,4); //Direccion y pines de salida LCD


//MPU6050 mpu6050(Wire); //Se inicializa la conexion I2C para el sensor MPU6050_knock
MPU6050 mpu1(Wire); //Se inicializa la conexion I2C para el sensor MPU6050_light
MPU6050 mpu2(Wire); //Se inicializa la conexion I2C para el sensor MPU6050_light

unsigned long timer = 0;

//String inputString = "";
//bool stringComplete = false; 
bool home_pos=false;
String azimuthTemp = ""; //Crea una variable temporal donde se guarda el valor del Azimut para la conversion a valor numérico
String elevationTemp = ""; //Crea una variable temporal donde se guarda el valor del Elevación para la conversion a valor numérico
float azimuth ; //Se inicializa el valor de az leido en 0
float elevation ;
//float azimuth; //Se inicializa el valor de az
//float elevation;
int eeAddress=0;
float geografico=0;



Separador s; //Crea un objeto de la clase Separador
MechaQMC5883 qmc;   // crea objeto

#define PUL 7 //Pin para la señal de pulso motor 1
#define DIR 6 //define Direction pin motor 1
#define EN 9 //define Enable Pin motor 1
#define PUL2 5 //Pin para la señal de pulso motor 2
#define DIR2 4 //define Direction pin motor 2
#define EN2 12 //define Enable Pin motor 2

//Variables auxiliares
float posicionAz = 0;
float posicionInicialAz = 0;
float posicionEl = 0;
float posicionInicialEl = 0;
float hist_I[10]={0,0,0,0,0,0,0,0,0,0};
float hist_I2[10]={0,0,0,0,0,0,0,0,0,0};
//Variables para el uso de potenciometro como método de posicionamiento
//int valPot1;
//float angPot1 = 0; 
//int valPot2;
//float angPot2 = 0; //Variables para el uso de Potenciometros

float angHMC = 0;
float angMPU = 0;
int period = 50; //Refresh rate period of the loop is 50ms
float last_z,z;
String inputString = "";         // a String to hold incoming data
bool stringComplete = false;  // whether the string is complete

bool ret=false;
float theta=1|0;
float rho=0;

//float errorPosicion1Prev, errorPosicion1;
//float errorPosicion2Prev, errorPosicion2;

//Valores de las constantes del comportamiento del controlador PID
double Setpoint, Input, Output;
double Setpoint2, Input2, Output2;
//Azimuth
double kp=8; // Se queda en 8.9 para un periodo maximo de 64ms 15 8
double ki=0.2; //0.2      0.2
double kd=0.5; //3100    0.5
//Elevación
double kp2=8; // Se queda en 8.9 para un periodo maximo de 64ms 10 8 
double ki2=0.2; //0.2       0.5
double kd2=0.05; //3100    0.005
PID myPID(&Input, &Output, & Setpoint, kp, ki, kd, DIRECT); //az
PID myPID2(&Input2, &Output2, & Setpoint2, kp2, ki2, kd2, DIRECT); //el
//float PID_p, PID_i, PID_d, PID_total; //Variables auxiliares
//float PID_p2, PID_i2, PID_d2, PID_total2; //Variables auxiliares

//El modelo que se utiliza para el control PID es el siguiente:
//PID = (kp*error)+(PID_i+ki*error)+(kd*(error-errorPrevio)/tiempo)

void PHome(); //Declaración de función posición de inicio(home) ET.  
//void PhomeElevacion(); //Declaracion de funcion de regreso para valores negativos satelite

#define I2C_SLAVE_ADDR 0x27
long data = 100;
long response = 0;
Tone notePlayer[2];

void setup() {
//Serial.begin(9600);
Serial.begin(9600);
  mpu1.setAddress(0x68);
  mpu2.setAddress(0x69);
  notePlayer[0].begin(7);
  notePlayer[1].begin(5);

  //PID inicialización
  myPID.SetMode(AUTOMATIC);
  myPID2.SetMode(AUTOMATIC);

  ////////////
   Wire.begin();
// Comenzamos la transmisión al dispositivo 1
    Wire.beginTransmission(1);
    
//// motor de azimut
//pinMode (PUL, OUTPUT); //Define la salida del pulso como salida
pinMode (DIR, OUTPUT); //Define la salida de direccion como salida
pinMode (EN, OUTPUT); //Define la salida de enable como salida

// motor de elevacion
//pinMode (PUL2, OUTPUT);
pinMode (DIR2, OUTPUT);
pinMode (EN2, OUTPUT);


  Serial.println("Calibrando sensores");
  delay(5000);
 
  //geografico=52;
  mpu1.begin();
  mpu2.begin();
  Serial.println("Iniciando Posicion home & PID...");   // inicializa objeto MPU6050 librería light
  Serial.println(F("Calculando offsets gyro, no mover MPU6050..."));
  delay(100);
  
  //mpu.setGyroOffsets(-3.06, 0.79, 0.04);
  mpu1.calcGyroOffsets();
  mpu2.calcGyroOffsets();
  // Esto es para la calibracion
  mpu1.update();
  mpu2.update();
  z=360-mpu2.getAngleZ();
  PHome(); //Llamar función Posiscion home a ejecutar antes 
           //Antes de ejectutar void loop.
                    
  Serial.println("Estacion terrena en posición home correcta"); 

  }
  
void loop() {
  
//  if(!home_pos){
//    if(!ret){
//      if(theta<350){
//        theta+=0.01;  
//      }else if(theta>=350){
//        ret=true;
//      }
//    }else{
//      if(theta>10){
//        theta-=0.1;
//      }else if(theta<=10){
//        ret=false;
//      }
//    }
//    //rho=5+abs(40*sin(0.5*theta));
//    //azimuth=theta;
//    //elevation=rho;
//  }

  
  
  mpu1.update();
  mpu2.update();
  //int x,y,z;      // variables de los 3 ejes
  //float acimut,geografico;  // variables para acimut magnetico y geografico
  //float declinacion = +5.2;    // declinacion desde pagina: http://www.magnetic-declination.com/
  //qmc.read(&x,&y,&z,&acimut);   // lectura de los ejes y acimut magnetico
//
  //geografico = acimut + declinacion;  // acimut geografico como suma del magnetico y declinacion
  EEPROM.get(eeAddress,geografico);
  last_z=z;
  z=360-mpu2.getAngleZ();
  geografico+=z-last_z;
  //geografico=100;
  //EEPROM.put(eeAddress,geografico);
  while(geografico < 0){      // si es un valor negativo
  geografico+=360;// suma 360 y vuelve a asignar a variable
  }  while(geografico>360){
    geografico-=360;
  }
  //geografico=360-geografico;
  //geografico=161;
  EEPROM.put(eeAddress,geografico);
  angHMC = geografico; //Valor calculado 
  
  angMPU = 90-filtro_MPU(); //angulo Y del giroscopio libreria light

if(Serial.available()){
    stringComplete=false;
//dividirString(); 

}

  
  /*Serial.println("");
  Serial.println("-------Trabajando control PID-------");
  Serial.println("");

  
if (Serial.available()) //Verifica si se está obteniendo respuesta de entrada del puerto serial
{
dividirString(); //Divide la cadena en dos variables numéricas de Az y El recibidas mediante Orbitron (Vease manual de Orbitron anexo)
}
*/
//En las siguientes lineas se hace el proceso matemático para el control PID para azimut
//____PID para azimuth______

//---------------------------PID------------------------------//
  //Azimuth 
  azimuth = 20;
  elevation = 50;
  //azimuth = 100;
  //elevation = 85;
  
  Input = angHMC; //retroalimenación MPU AZIMUTH
  Setpoint = azimuth; // el valor al que debe llegar  
  //Setpoint = 90; // valor de preuba 
  myPID.Compute();
  double  f_az = Output;
  //double  f_az = Output*3.18;
  //Serial.print("Frecuencia en azimuth  ");
  //Serial.println(f_az);

//    if(Output >= 255 ){
//      Output = 255;
//    }else if (Output <= 0 ){
//        Output = 0;
//      }
      
//      if(f_az >= 255 ){
//      Output = 255;
//    }else if (f_az <= 0 ){
//        Output = 0;
//      }

  //Elevación
   
  Input2 = angMPU;
  Setpoint2 = elevation; 
  myPID2.Compute();
  double f_el = Output2;
  //double f_el = Output2*5.1;

//  if(Output2 >=255 ){
//      Output2 = 255;
//    }else if (Output2 <= 0 ){
//        Output2 = 0;
//      }

//      if(f_el >= 255 ){
//      Output = 255;
//    }else if (f_el <= 0 ){
//        Output = 0;
//      }

      

  //float error_az = azimuth - angHMC;
  //float error_el = elevation - angMPU;
  
  //float faz = error_az*3.18;
  //float fel = error_El*6.375; 
  
  //Serial.print("Frecuencia en elevación  ");
  //Serial.println(f_el);

//PID ALONSO

//----------------------Movimiento--------------------------------//

//if(angHMC>=8 && angHMC<=12   && (angMPU>=13.5 && angMPU<=15.5)){
  home_pos=false;
//}
if(f_az>=255.0)f_az=255.0;
if(f_el>=255.0)f_el=255.0;
if(azimuth<5.0)azimuth=5.0;
if(azimuth>350.0)azimuth=350.0;
if(elevation<5.0)elevation=5.0;
if(elevation>90.0)elevation=90.0;
if(azimuth<350 && azimuth>10){
//Condicion para Azimut
//--------------------Azimuth------------------------//
  digitalWrite(EN,LOW);
  if(angHMC<azimuth-1){ //Se define una tolerancia de 1 grado -1 grado
    digitalWrite(DIR,LOW); //movimiento sentido horario
    notePlayer[0].play((int)200);
    //notePlayer[0].play((int)f_az);
  }else if(angHMC>azimuth+1){ //+1 grado
    digitalWrite(DIR,HIGH); //movimiento sentido antihorario
    notePlayer[0].play((int)200);
    //notePlayer[0].play((int)f_az);
  }else{
    digitalWrite(EN,HIGH);
    notePlayer[0].stop();
  }
}else{
  digitalWrite(EN,HIGH);
  notePlayer[0].stop();
  
}
//tone(PUL,400);
//tone(PUL2,400);

//Condicion para elevación

//--------------------Elevación------------------------//

if(elevation<=90 && elevation>-1){
//Condicion para Azimut
//--------------------Elevació------------------------//
  digitalWrite(EN2,LOW);
  if(angMPU<elevation-1 ){ //Se define una tolerancia de 1 grado -1
    digitalWrite(DIR2,LOW); //movimiento sentido horario
    notePlayer[1].play((int)200);
    //notePlayer[1].play((int)f_el);
  }else if(angMPU>elevation+1){ //+1 
    digitalWrite(DIR2,HIGH); //movimiento sentido antihorario
    notePlayer[1].play((int)200);
    //notePlayer[1].play((int)f_el);
  }else{
    digitalWrite(EN2,HIGH);
    notePlayer[1].stop();
  }
}else{
  digitalWrite(EN2,HIGH);
  notePlayer[1].stop();
}
    
    

//Serial.print("Targets: ");
//Serial.print("[AZ]: ");
//Serial.print(azimuth);
//Serial.print(" [EL]: ");
//Serial.print(elevation);
//Serial.print("  Current : ");
//Serial.print("[AZ]: ");
Serial.print(angHMC);
Serial.print(",");
//Serial.print(z);
//Serial.print(",");
//Serial.print(" [EL]: ");
Serial.print(angMPU);
Serial.print(",");
float error_az = azimuth - angHMC;
float error_el = elevation - angMPU;
//Serial.print("   [AZ]: ");
Serial.print(error_az);
Serial.print(",");
Serial.print(millis());
Serial.print(",");
//Serial.print(" [EL]: ");
Serial.print(error_el);
Serial.print(",");
Serial.print(millis());
Serial.print(",");
Serial.print(Output);
Serial.print(",");
Serial.print(Output2);
Serial.print(",");
Serial.print(f_az);
Serial.print(",");
Serial.println(f_el);

//Serial.println();


}

void dividirString(){
if(!home_pos){  
  String datosSerial = Serial.readString(); //Lee los datos del puerto serial
  azimuthTemp = s.separa(datosSerial,',',0); //Utiliza la librería Separador y asigna el primer elemento separado con una coma al azimut
  elevationTemp = s.separa(datosSerial,',',1); //Utiliza la librería Separador y asigna el segundo elemento a la elevación
  //Transforma la cadena a dato numérico para ser tratado en el proceso
  azimuth = azimuthTemp.toFloat();
  elevation = elevationTemp.toFloat();
  if(azimuth<5.0)azimuth=5.0;
if(azimuth>355.0)azimuth=355.0;
if(elevation<5.0)elevation=5.0;
//if(elevation>60.0)elevation=60.0;
}else{
  azimuth=10;
  elevation=25;
}
stringComplete=true;
  if(azimuth<5.0)azimuth=5.0;
if(azimuth>355.0)azimuth=355.0;
if(elevation<5.0)elevation=5.0;
if(elevation>60.0)elevation=60.0;
}
//Ejecuta función home
void PHome(){
  home_pos=true;
  azimuth=10;
  elevation=15.5;

}
  //Función para llamar sensor HMC5883 a ejectutar 
 
//void serialEvent(){
  //stringComplete=false;
//dividirString(); 

//}
float filtro_MPU(){
  float sum=0;
  for(int i=0; i<20;i++){
    mpu1.update();
    sum+=mpu1.getAngleY();
  }
  return sum/20;
}
