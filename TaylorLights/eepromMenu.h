/**
 * 
 *  EEPROM based configuration system
 * 
 *  @author Aaron S. Crandall <acrandal@gmail.com>
 *  @copyright 2020
 * 
 */


#ifndef __EEPROM_MENU
#define __EEPROM_MENU

#include <EEPROM.h>

#define EEPROM_ALLOCATE 256
#define EEPROM_CONFIGS_ADDR 0
#define USER_INTERRUPT_TIMEOUT_SECS 5


struct Configs {
  char ssid[30] = "wifi ssid";
  char password[30] = "wifi password";
  char mqtt_server[30] = "127.0.0.1";
  char device_location[30] = "unknown";
};


// ** Class encompassing EEPROM configuration load/edit/store **********************
class ESP8266_EEPROM_Configs
{

  private:
    Configs my_configs;

  public:

  ESP8266_EEPROM_Configs() {
    //Configs *my_configs = new Configs;
  }

  char* get_SSID() { return my_configs.ssid; }
  char* get_Password() { return my_configs.password; }
  char* get_MQTTServer() { return my_configs.mqtt_server; }
  char* get_DeviceLocation() { return my_configs.device_location; }

  // ** Run full configuration routine ****************************************************
  void init() {
    init_eeprom();
    load_current_config();
    display_configs();
    do_user_menu();
    display_configs();
  }

  // ** Initialize the EEPROM access *******************************************************
  void init_eeprom() {
    EEPROM.begin(EEPROM_ALLOCATE);          // Allocate EEPROM for use (max 512b)
  }
  
  // ** Load in the current configuration from EEPROM *************************************
  void load_current_config() {
    EEPROM.get(EEPROM_CONFIGS_ADDR, my_configs);
  }

  // ** Save current config to EEPROM *****************************************************
  void save_current_config() {
    EEPROM.put(EEPROM_CONFIGS_ADDR, my_configs);
    EEPROM.commit();
  }

  // ** Editor prompt **********************************************************
  void show_configs_editor_prompt() {
    Serial.println(" ******** Current configuration:");
    display_configs();
    Serial.println("(s) Save Configuration to EEPROM");
    Serial.println("(q) Quit configuration (no save)");
    Serial.println();
    Serial.println(" Enter choice: ");
  }

  // ** Get a user's input, with a pause *************************************************
  // ** If secs_pause is 0, then wait forever
  String get_user_menu_selection(int secs_pause) {
    String user_entry = "";
    int timeout_count = 0;
    while(user_entry == "" && (secs_pause == 0 || timeout_count < secs_pause) ) {
      user_entry = Serial.readString();
      user_entry.trim();
      timeout_count++;
    }
    return user_entry;
  }

  // ** Prompt to edit a single char* string in the configs ********************
  void edit_one_config(char* config_string) {
    Serial.print("Current value: ");
    Serial.println(config_string);
    Serial.print("Enter new config value: ");
    String user_selection = get_user_menu_selection(0);
    user_selection.toCharArray(config_string, 30);
    Serial.print("Config updated to: ");
    Serial.println(config_string);
  }

  // ** Configs EDIT menu ******************************************************
  void do_configs_editor() {
    bool done = false;
  
    while( !done ) {
      show_configs_editor_prompt();
      String user_selection = get_user_menu_selection(0);
  
      Serial.print("User entered: ");
      Serial.println(user_selection);
  
      if(user_selection.length() < 1) { return; }
  
      char* string_to_edit;
      switch (tolower(user_selection[0])) {
        case '1':
          Serial.println("Editing SSID.");
          string_to_edit = my_configs.ssid;
          edit_one_config(string_to_edit);
          break;
        case '2':
          Serial.println("Editing Wifi Password.");
          string_to_edit = my_configs.password;
          edit_one_config(string_to_edit);
          break;
        case '3':
          Serial.println("Editing MQTT Server IP.");
          string_to_edit = my_configs.mqtt_server;
          edit_one_config(string_to_edit);
          break;
        case '4':
          Serial.println("Editing Device location string.");
          string_to_edit = my_configs.device_location;
          edit_one_config(string_to_edit);
          break;
        case 's':
          Serial.print("Saving to EEPROM -- ");
          save_current_config();
          Serial.println("Save complete.");
          break;
        case 'q':
          Serial.println("Quitting to normal mode");
          done = true;
          break;
      }
    }
  }

  // ** Pause if need to edit by user ******************************************
  void do_user_menu() {
    String serial_input = "";
  
    for(int i = 0; i < 4; i++) {
      Serial.println("*");
    }
    Serial.println("************************************************");
    Serial.println("** Welcome to Crandall's House IoT Device     **");
    Serial.println("************************************************");
    Serial.println("");
    Serial.println("Enter anything + enter for configuration editing menu");
    Serial.println(" --> Dots are counting a 15 second timeout to start normally");
    serial_input = Serial.readString();
  
    for( int i = 0; i < USER_INTERRUPT_TIMEOUT_SECS; i++ ) {   // for user interrupt
      if( serial_input != "" ) { break; }
      Serial.print(".");
      serial_input = Serial.readString();
    }
    Serial.println("");
  
    if(serial_input != "") {      
      Serial.print("Entered string: ");
      Serial.print(serial_input);
      Serial.println(" --> Entering configs editor");
      do_configs_editor();
    } else {
      Serial.println("No user interrupt, beginning normal operations");
    }
  }

  // ** Display summary of current config *************************************************
  void display_configs() {
    Serial.println("********************* Current configs *********************");
    Serial.print("(1) SSID: ");
    Serial.println(my_configs.ssid);
    Serial.print("(2) Pass: ");
    Serial.println(my_configs.password);
    Serial.print("(3) MQTT Server: ");
    Serial.println(my_configs.mqtt_server);
    Serial.print("(4) Device Location: ");
    Serial.println(my_configs.device_location);
  }
};


#endif
