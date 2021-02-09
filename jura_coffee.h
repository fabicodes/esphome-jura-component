#include "esphome.h"

class JuraCoffee: public PollingComponent, public UARTDevice {
    Sensor * xsensor1 { nullptr }; // num_espresso
    Sensor * xsensor2 { nullptr }; // num_coffee
    Sensor * xsensor3 { nullptr }; // num_big_coffee
    Sensor * xsensor4 { nullptr }; // num_double_coffee
    Sensor * xsensor5 { nullptr }; // num_powder_coffee
    Sensor * xsensor6 { nullptr }; // num_unknown1
    Sensor * xsensor7 { nullptr }; // num_unknown2
    Sensor * xsensor8 { nullptr }; // num_unknown3
    BinarySensor * xsensor9 { nullptr }; // power_status
    BinarySensor * xsensor10 { nullptr }; // err_tray
    BinarySensor * xsensor11 { nullptr }; // err_tank
    BinarySensor * xsensor12 { nullptr }; // err_grounds
    BinarySensor * xsensor13 { nullptr }; // rinsing_status
    BinarySensor * xsensor14 { nullptr }; // sel_big_coffee
    BinarySensor * xsensor15 { nullptr }; // sel_coffee
    BinarySensor * xsensor16 { nullptr }; // sel_espresso
    BinarySensor * xsensor17 { nullptr }; // sel_mild
    BinarySensor * xsensor18 { nullptr }; // sel_normal
    BinarySensor * xsensor19 { nullptr }; // sel_strong

    public:
        JuraCoffee(UARTComponent * parent, Sensor * sensor1, Sensor * sensor2, Sensor * sensor3, Sensor * sensor4, Sensor * sensor5, Sensor * sensor6, Sensor * sensor7, Sensor * sensor8, BinarySensor * sensor9, BinarySensor * sensor10, BinarySensor * sensor11, BinarySensor * sensor12, BinarySensor * sensor13, BinarySensor * sensor14, BinarySensor * sensor15, BinarySensor * sensor16, BinarySensor * sensor17, BinarySensor * sensor18, BinarySensor * sensor19): UARTDevice(parent), xsensor1(sensor1), xsensor2(sensor2), xsensor3(sensor3), xsensor4(sensor4), xsensor5(sensor5), xsensor6(sensor6), xsensor7(sensor7), xsensor8(sensor8), xsensor9(sensor9), xsensor10(sensor10), xsensor11(sensor11), xsensor12(sensor12), xsensor13(sensor13), xsensor14(sensor14), xsensor15(sensor15), xsensor16(sensor16), xsensor17(sensor17), xsensor18(sensor18), xsensor19(sensor19) {}

    long num_espresso, num_coffee, num_big_coffee, num_double_coffee, num_powder_coffee, num_unknown1, num_unknown2, num_unknown3;
    bool power_status, err_tray, err_tank, err_grounds, rinsing_status, sel_big_coffee, sel_coffee, sel_espresso, sel_mild, sel_normal, sel_strong;
    int man_update = 0;
    int refresh_state = 0;
    const char * known_commands[11] = {
        "AN:01", // Action Power On
        "AN:02", // Action Power Off
        "FA:02", // Button Big Coffee
        "FA:03", // Button Coffee
        "FA:04", // Button Espresso
        "FA:05", // Button Start
        "FA:06", // Button Mild
        "FA:07", // Button Normal
        "FA:08", // Button Strong
        "FA:09", // Button Flush
        0
    };

    // Jura communication function taken in entirety from cmd2jura.ino, found at https://github.com/hn/jura-coffee-machine
    String cmd2jura(String outbytes) {
        if (outbytes == "AN:0A") {
            ESP_LOGD("main", "Malicious Command blocked");
            return "";
        }
        int i = 0;
        while (known_commands[i]) {
            if (strcmp(known_commands[i], outbytes.c_str()) == 0) {
                refresh_state = 1;
                break;
            }
            i++;
        }
        String inbytes = "";
        int w = 0;

        while (available()) {
            read();
        }

        outbytes += "\r\n";
        for (int i = 0; i < outbytes.length(); i++) {
            for (int s = 0; s < 8; s += 2) {
                char rawbyte = 255;
                bitWrite(rawbyte, 2, bitRead(outbytes.charAt(i), s + 0));
                bitWrite(rawbyte, 5, bitRead(outbytes.charAt(i), s + 1));
                write(rawbyte);
            }
            delay(8);
        }

        int s = 0;
        char inbyte = 0;
        while (!inbytes.endsWith("\r\n")) {
            if (available()) {
                byte rawbyte = read();
                bitWrite(inbyte, s + 0, bitRead(rawbyte, 2));
                bitWrite(inbyte, s + 1, bitRead(rawbyte, 5));
                if ((s += 2) >= 8) {
                    s = 0;
                    inbytes += inbyte;
                }
            } else {
                delay(10);
            }
            if (w++ > 500) {
                return "";
            }
        }

        return inbytes.substring(0, inbytes.length() - 2);
    }

    void setup() override {
        this -> set_update_interval(2500); // 1000 = 1 sec
    }

    void loop() override {}

    void update() override {
        String result, hexString, substring;
        byte hex_to_byte;
        ESP_LOGD("main", "Refresh state: %d", refresh_state);
        switch (refresh_state) {
        case 0:
            // Fetch our line of EEPROM
            result = cmd2jura("RT:0000");
            num_espresso = strtol(result.substring(3, 7).c_str(), NULL, 16);
            num_coffee = strtol(result.substring(7, 11).c_str(), NULL, 16);
            num_big_coffee = strtol(result.substring(11, 15).c_str(), NULL, 16);
            num_double_coffee = strtol(result.substring(15, 19).c_str(), NULL, 16);
            num_powder_coffee = strtol(result.substring(27, 31).c_str(), NULL, 16);
            num_unknown1 = strtol(result.substring(35, 39).c_str(), NULL, 16);
            num_unknown2 = strtol(result.substring(39, 44).c_str(), NULL, 16);
            num_unknown3 = strtol(result.substring(44, 47).c_str(), NULL, 16);

            if (xsensor1 != nullptr) xsensor1 -> publish_state(num_espresso);
            if (xsensor2 != nullptr) xsensor2 -> publish_state(num_coffee);
            if (xsensor3 != nullptr) xsensor3 -> publish_state(num_big_coffee);
            if (xsensor4 != nullptr) xsensor4 -> publish_state(num_double_coffee);
            if (xsensor5 != nullptr) xsensor5 -> publish_state(num_powder_coffee);
            if (xsensor6 != nullptr) xsensor6 -> publish_state(num_unknown1);
            if (xsensor7 != nullptr) xsensor7 -> publish_state(num_unknown2);
            if (xsensor8 != nullptr) xsensor8 -> publish_state(num_unknown3);
            ESP_LOGD("main", "num_espresso: %d, num_coffee: %d, num_big_coffee: %d, num_double_coffee: %d, num_powder_coffee: %d, num_unknown1: %d, num_unknown2: %d, num_unknown3: %d", num_espresso, num_coffee, num_big_coffee, num_double_coffee, num_powder_coffee, num_unknown1, num_unknown2, num_unknown3);
            refresh_state++;
            break;
        case 1:
            // Tray & water tank status
            // Much gratitude to https://www.instructables.com/id/IoT-Enabled-Coffee-Machine/ for figuring out how these bits are stored
            hex_to_byte = strtol(cmd2jura("IC:").substring(3, 5).c_str(), NULL, 16);
            err_tray = !bitRead(hex_to_byte, 4);
            if (xsensor10 != nullptr) xsensor10 -> publish_state(err_tray);
            ESP_LOGD("main", "err_tray: %d", err_tray);
            refresh_state++;
            break;
        case 2:
            // Grounds Status
            // Bekannte Werte: 0x0004 (OK), 0x0024 (Trester), 0x000C (Wasser)
            hex_to_byte = strtol(cmd2jura("RR:03").substring(5).c_str(), NULL, 16);
            err_tank = bitRead(hex_to_byte, 3);
            err_grounds = bitRead(hex_to_byte, 5);
            if (xsensor11 != nullptr) xsensor11 -> publish_state(err_tank);
            if (xsensor12 != nullptr) xsensor12 -> publish_state(err_grounds);
            ESP_LOGD("main", "err_tank: %d, err_grounds: %d", err_tank, err_grounds);
            refresh_state++;
            break;
        case 3:
            // Power Status
            power_status = cmd2jura("RR:13") == "rr:0100";
            if (xsensor9 != nullptr) xsensor9 -> publish_state(power_status);
            ESP_LOGD("main", "power_status: %d", power_status);
            refresh_state++;
            break;
        case 4:
            // Rinse Status
            rinsing_status = cmd2jura("RR:62") == "rr:0101";
            if (xsensor13 != nullptr) xsensor13 -> publish_state(rinsing_status);
            ESP_LOGD("main", "rinsing_status: %d", rinsing_status);
            refresh_state++;
            break;
        case 5:
            // Selection
            hex_to_byte = strtol(cmd2jura("RR:5D").substring(5).c_str(), NULL, 16);
            sel_big_coffee = bitRead(hex_to_byte, 1);
            sel_coffee = bitRead(hex_to_byte, 2);
            sel_espresso = bitRead(hex_to_byte, 3);
            sel_mild = bitRead(hex_to_byte, 5);
            sel_normal = bitRead(hex_to_byte, 6);
            sel_strong = bitRead(hex_to_byte, 7);
            if (xsensor14 != nullptr) xsensor14 -> publish_state(sel_big_coffee);
            if (xsensor15 != nullptr) xsensor15 -> publish_state(sel_coffee);
            if (xsensor16 != nullptr) xsensor16 -> publish_state(sel_espresso);
            if (xsensor17 != nullptr) xsensor17 -> publish_state(sel_mild);
            if (xsensor18 != nullptr) xsensor18 -> publish_state(sel_normal);
            if (xsensor19 != nullptr) xsensor19 -> publish_state(sel_strong);
            ESP_LOGD("main", "sel_big_coffee: %d, sel_coffee: %d, sel_espresso: %d, sel_mild: %d, sel_normal: %d, sel_strong: %d", sel_big_coffee, sel_coffee, sel_espresso, sel_mild, sel_normal, sel_strong);
            refresh_state++;
            break;
        default:
            refresh_state = 0;
            break;
        }
    }
};