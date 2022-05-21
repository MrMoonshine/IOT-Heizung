/**** LOCATIONS ON SERVER ****/
#define TEMP_SOLAR_LOC "solartemp"    //Solarpannel
#define TEMP_ROOM_LOC "room"          //Raumtemperatur
#define TEMP_BUFFER_LOC "white"       //buffer ausgang
#define TEMP_SOLAROUT_LOC "blue"      //vorlauf
#define TEMP_SOLARIN_LOC "red"        //rücklauf
#define TEMP_HEATING_LOC "brown"      //Heizpumpe vorlauf
#define TEMP_GREEN_LOC "green"        //redundant
#define TEMP_YELLOW_LOC "yellow"      //redundant

#define ANALTEMP_PIN 36 //Analog pin for solartemp
#define TEMP_URL "http://alpakagott/alpakaheizung/dbrec.php?"
#define SET_URL "http://alpakagott/alpakaheizung/dbsta.php?pump="
#define LOG_URL "http://alpakagott/alpakaheizung/dbrec.php?id="

/**** ADDRESSES ****/
/*DeviceAddress sen[] = {{40,157,37,201,11,0,0,164},//Room temperature [TO-92]
                       {40,170,82,197,78,20,1,111}, //Red                      
                       {40,170,122,245,78,20,1,225},//Green
                       {40,170,161,197,78,20,1,135},//Blue
                       {40,170,137,188,78,20,1,122},//White
                       {40,170,99,243,78,20,1,103},//Yellow
                       {40,170,147,228,78,20,1,127}//Brown
                       };*/
#define ADDR_TEMP_ROOM   {40,157,37,201,11,0,0,164}
#define ADDR_TEMP_RED    {40,170,82,197,78,20,1,111}
#define ADDR_TEMP_GREEN  {40,170,122,245,78,20,1,225}
#define ADDR_TEMP_BLUE   {40,170,161,197,78,20,1,135}
#define ADDR_TEMP_WHITE  {40,170,137,188,78,20,1,122}
#define ADDR_TEMP_YELLOW {40,170,99,243,78,20,1,103}
#define ADDR_TEMP_BROWN  {40,170,147,228,78,20,1,127}

#define TEMP_ROOM_ADDR ADDR_TEMP_ROOM
#define TEMP_SOLARIN_ADDR ADDR_TEMP_RED
#define TEMP_GREEN_ADDR ADDR_TEMP_GREEN
#define TEMP_SOLAROUT_ADDR ADDR_TEMP_BLUE
#define TEMP_BUFFER_ADDR ADDR_TEMP_WHITE
#define TEMP_YELLOW_ADDR ADDR_TEMP_YELLOW
#define TEMP_HEATING_ADDR ADDR_TEMP_BROWN

#define HEATPUMP 15
#define N_HEATPUMP "heatpump"
#define SOLARPUMP 2
#define N_SOLARPUMP "solarpump"
#define REDUNDANT 4
#define N_REDUNDANT "redundant"
#define BUFFERPUMP 16
#define N_BUFFERPUMP "bufferpump"
#define GROUNDPUMP_0 18
#define N_GROUNDPUMP_0 "groundpump0"
#define GROUNDPUMP_1 19
#define N_GROUNDPUMP_1 "groundpump1"

#define MIXER_OPEN 17
#define MIXER_CLOSE 5
#define MIXTIME (25*6) //(2.5*60) 2.5 minutes

#define SOLARBUFF_OFFSET 10 
#define SOLIO_OFFSET 3
#define BUFFER_OFFSET 10
#define ROOFTEMP_OFFSET 4.5
#define SOLARMODE_URL "http://alpakagott/alpakaheizung/states/solarmode"

//#define ANALTEMP_SOLAR_RV  4694.0
//#define ANALTEMP_SOLAR_VDD 3300.0

//Logging IDs
#define LOG_SOLAR_ON 0
#define LOG_SOLAR_OFF 1
#define LOG_MIXSTEP_OPEN 2
#define LOG_MIXSTEP_CLOSE 3
#define LOG_MEASSURE_FAILURE 4
#define LOG_CONNECTED 5
//6 is testlog
#define LOG_RESTART 7
