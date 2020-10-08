#include <esp_log.h>
#include <hap_apple_servs.h>
#include <hap_apple_chars.h>
#include <string.h>
#include "emulator.h"
#include "nvs.h"

int hap_factory_keystore_set(const char *name_space, const char *key, const uint8_t *val, size_t val_size);
int hap_keystore_set(const char *name_space, const char *key, const uint8_t *val, const size_t val_len);
int hap_keystore_get(const char *name_space, const char *key, uint8_t *val, size_t *val_size);

int hap_keystore_get_u32(const char *name_space, const char  *key, uint32_t *buffer, char *entity_name)
{
    int ret = HAP_FAIL;
    nvs_handle handle;
    esp_err_t err = nvs_open(name_space, NVS_READONLY, &handle);
    if (err != ESP_OK) {
        ESP_LOGI("hap_keystore", "Error (%d) opening NVS handle for %s!", err, entity_name);
        return HAP_FAIL;
    } else {
        err = nvs_get_u32(handle, key, buffer);
        if (err == ESP_OK) {
            ret = HAP_SUCCESS;
        } else {
            ESP_LOGI("hap_keystore", "Error getting %s data!", entity_name);
        }
        nvs_close(handle);
    }
    return ret;
}

int hap_keystore_set_u32(const char *name_space, const char *key, uint32_t buffer, char *entity_name)
{
    int ret = HAP_FAIL;
    nvs_handle handle;
    esp_err_t err = nvs_open(name_space, NVS_READWRITE, &handle);
    if (err != ESP_OK) {
        ESP_LOGI("hap_keystore", "Error (%d) opening NVS handle for %s!", err, entity_name);
        return HAP_FAIL;
    } else {
        err = nvs_set_u32(handle, key, buffer);
        if (err != ESP_OK) {
            ESP_LOGI("hap_keystore", "Error loading %s data!", entity_name);
            ret = HAP_FAIL;
        } else {
            nvs_commit(handle);
            ret = HAP_SUCCESS;
        }
        nvs_close(handle);
    }
    return ret;
}

static int profile_num;

static int hex_compare(unsigned char *buf, int buf_len)
{
    char auth_data[] = { 0x01, 0x02, 0x03 };
    if(strlen(auth_data) == buf_len)
        return memcmp(buf, auth_data ,buf_len);
    return -1;
}

/* Char: Target Door State with tw(Timed Write) & aa(Addtional Authorization) */
static hap_char_t *hap_char_target_door_state_create_with_tw_aa(uint8_t targ_door_state)
{
    hap_char_t *hc = hap_char_uint8_create(HAP_CHAR_UUID_TARGET_DOOR_STATE,
                                           HAP_CHAR_PERM_PR | HAP_CHAR_PERM_PW | HAP_CHAR_PERM_EV | HAP_CHAR_PERM_AA | HAP_CHAR_PERM_TW, targ_door_state);
    if (!hc) {
        return NULL;
    }

    hap_char_int_set_constraints(hc, 0, 4, 1);

    return hc;
}

/* Timed Write(tw)and Additional Auth(aa) Info simulation for Target Door state characteristic in Garage door opener */
static hap_serv_t *hap_serv_garage_door_opener_create_with_tw_aa(uint8_t curr_door_state, uint8_t targ_door_state, bool obstr_detect)
{
    hap_serv_t *hs = hap_serv_create(HAP_SERV_UUID_GARAGE_DOOR_OPENER);
    if (!hs) {
        return NULL;
    }
    if (hap_serv_add_char(hs, hap_char_current_door_state_create(curr_door_state)) != HAP_SUCCESS) {
        goto err;
    }
    if (hap_serv_add_char(hs, hap_char_target_door_state_create_with_tw_aa(targ_door_state)) != HAP_SUCCESS) {
        goto err;
    }
    if (hap_serv_add_char(hs, hap_char_obstruction_detect_create(obstr_detect)) != HAP_SUCCESS) {
        goto err;
    }
    return hs;
err:
    hap_serv_delete(hs);
    return NULL;
}

/* Services Emulation Block */
static hap_serv_t *hap_fan_create()
{
    hap_serv_t *hs = hap_serv_fan_create(false);
    hap_serv_add_char(hs, hap_char_rotation_direction_create(0));
    return hs;
}

static hap_serv_t *hap_switch_create()
{
    return hap_serv_switch_create(false);
}

static hap_serv_t *hap_lightbulb_create()
{
    hap_serv_t *hs = hap_serv_lightbulb_create(false);
    hap_serv_add_char(hs, hap_char_brightness_create(50));
    hap_serv_add_char(hs, hap_char_saturation_create(50));
    hap_serv_add_char(hs, hap_char_hue_create(50));
    return hs;
}

static hap_serv_t *hap_garage_door_opener_create()
{
    return hap_serv_garage_door_opener_create_with_tw_aa(0, 0 ,false);
}

static hap_serv_t *hap_lock_create()
{
    return hap_serv_lock_mechanism_create(0, 0);
}

static hap_serv_t *hap_lock_management_create()
{
    return hap_serv_lock_management_create(NULL, NULL);
}

static hap_serv_t *hap_outlet_create()
{
    return hap_serv_outlet_create(false, true);
}
#ifndef CONFIG_IDF_TARGET_ESP8266
static hap_serv_t *hap_thermostat_create()
{
    return hap_serv_thermostat_create(0, 0, 20.0, 25.0, 0);
}

static hap_serv_t *hap_leak_sensor_create()
{
    return hap_serv_leak_sensor_create(0);
}

static hap_serv_t *hap_air_quality_sensor_create()
{
    return hap_serv_air_quality_sensor_create(0);
}

static hap_serv_t *hap_carbon_monoxide_sensor_create()
{
    return hap_serv_carbon_monoxide_sensor_create(0);
}

static hap_serv_t *hap_contact_sensor_create()
{
    return hap_serv_contact_sensor_create(0);
}

static hap_serv_t *hap_humidity_sensor_create()
{
    return hap_serv_humidity_sensor_create(20.0);
}

static hap_serv_t *hap_light_sensor_create()
{
    return hap_serv_light_sensor_create(100.0);
}

static hap_serv_t *hap_motion_sensor_create()
{
    return hap_serv_motion_sensor_create(0);
}

static hap_serv_t *hap_occupancy_sensor_create()
{
    return hap_serv_occupancy_sensor_create(0);
}

static hap_serv_t *hap_temperature_sensor_create()
{
    return hap_serv_temperature_sensor_create(20.0);
}

static hap_serv_t *hap_smoke_sensor_create()
{
    return hap_serv_smoke_sensor_create(0);
}

static hap_serv_t *hap_door_create()
{
    return hap_serv_door_create(0, 10, 0);
}

static hap_serv_t *hap_window_create()
{
    return hap_serv_window_create(0, 10, 0);
}

static hap_serv_t *hap_window_covering_create()
{
    return hap_serv_window_covering_create(0, 10, 0);
}

static hap_serv_t *hap_security_system_create()
{
    return hap_serv_security_system_create(0, 0);
}

static hap_serv_t *hap_programmable_switch_create()
{
    return hap_serv_stateless_programmable_switch_create(0);
}

static hap_serv_t *hap_air_purifier_create()
{
    return hap_serv_air_purifier_create(0, 0, 0);
}

static hap_serv_t *hap_air_heater_create()
{
    hap_serv_t *hs = hap_serv_heater_cooler_create(0, 20.0, 1, 1);
    hap_char_t *hc = hap_serv_get_char_by_uuid(hs, HAP_CHAR_UUID_CURRENT_HEATER_COOLER_STATE);
    hap_char_int_set_constraints(hc, 0, 2, 1);

    hc = hap_serv_get_char_by_uuid(hs, HAP_CHAR_UUID_TARGET_HEATER_COOLER_STATE);
    hap_char_int_set_constraints(hc, 1, 1, 1);

    hap_serv_add_char(hs, hap_char_heating_threshold_temperature_create(15.0));
    return hs;
}

static hap_serv_t *hap_air_conditioner_create()
{
    hap_serv_t *hs = hap_serv_heater_cooler_create(0, 20.0, 0, 0);
    hap_serv_add_char(hs, hap_char_heating_threshold_temperature_create(15.0));
    hap_serv_add_char(hs, hap_char_cooling_threshold_temperature_create(25.0));
    return hs;
}

static hap_serv_t *hap_humidifier_create()
{
    return hap_serv_humidifier_dehumidifier_create(0, 10, 0, 0);
}

static hap_serv_t *hap_dehumidifier_create()
{
    return hap_serv_humidifier_dehumidifier_create(0, 10, 0, 0);
}

static hap_serv_t *hap_battery_service_create()
{
    return hap_serv_battery_service_create(20, 0, 0);
}

static hap_serv_t *hap_carbon_dioxide_sensor_create()
{
    return hap_serv_carbon_dioxide_sensor_create(0);
}
#endif /* !CONFIG_IDF_TARGET_ESP8266 */
/* List of services to be emulated */
static hap_emulator_profile_t profiles[] = {
    {HAP_CID_FAN, "Fan", hap_fan_create},
    {HAP_CID_GARAGE_DOOR_OPENER, "Garage-Door-Opener", hap_garage_door_opener_create},
    {HAP_CID_LIGHTING, "Lightbulb", hap_lightbulb_create},
    {HAP_CID_LOCK, "Lock", hap_lock_create},
    {HAP_CID_LOCK, "Lock-Management", hap_lock_management_create},
    {HAP_CID_OUTLET, "Outlet", hap_outlet_create},
    {HAP_CID_SWITCH, "Switch", hap_switch_create},
#ifndef CONFIG_IDF_TARGET_ESP8266
    {HAP_CID_THERMOSTAT, "Thermostat", hap_thermostat_create},
    {HAP_CID_SENSOR, "Leak-Sensor", hap_leak_sensor_create},
    {HAP_CID_SENSOR, "Air-quality-Sensor", hap_air_quality_sensor_create},
    {HAP_CID_SENSOR, "CO-Sensor", hap_carbon_monoxide_sensor_create},
    {HAP_CID_SENSOR, "Contact-Sensor", hap_contact_sensor_create},
    {HAP_CID_SENSOR, "Humidity-Sensor", hap_humidity_sensor_create},
    {HAP_CID_SENSOR, "Light-Sensor", hap_light_sensor_create},
    {HAP_CID_SENSOR, "Motion-Sensor", hap_motion_sensor_create},
    {HAP_CID_SENSOR, "Occupancy-Sensor", hap_occupancy_sensor_create},
    {HAP_CID_SENSOR, "Temperature-Sensor", hap_temperature_sensor_create},
    {HAP_CID_SENSOR, "Smoke-Sensor", hap_smoke_sensor_create},
    {HAP_CID_SENSOR, "CO2-Sensor", hap_carbon_dioxide_sensor_create},
    {HAP_CID_SECURITY_SYSTEM, "Security-System", hap_security_system_create},
    {HAP_CID_DOOR, "Door", hap_door_create},
    {HAP_CID_WINDOW, "Window", hap_window_create},
    {HAP_CID_WINDOW_COVERING, "Window-Covering", hap_window_covering_create},
    {HAP_CID_PROGRAMMABLE_SWITCH, "Programmable-Switch", hap_programmable_switch_create},
    {HAP_CID_AIR_PURIFIER, "Air-Purifier", hap_air_purifier_create},
    {HAP_CID_HEATER, "Air-Heater", hap_air_heater_create},
    {HAP_CID_AIR_CONDITIONER, "Air-Conditioner", hap_air_conditioner_create},
    {HAP_CID_HUMIDIFIER, "Humidifier", hap_humidifier_create},
    {HAP_CID_DEHUMIDIFIER, "Dehumidifier", hap_dehumidifier_create},
    {HAP_CID_OTHER, "Battery", hap_battery_service_create},
#endif /* !CONFIG_IDF_TARGET_ESP8266 */
};

/* Profile, Characteristics read, maniplation block */
static int profile_cnt = sizeof(profiles)/sizeof(hap_emulator_profile_t);

hap_emulator_profile_t *emulator_get_profile()
{
    size_t size = sizeof(profile_num);
    if (hap_factory_keystore_get("emulator", "profile", (uint8_t *)&profile_num, &size) != HAP_SUCCESS) {
        hap_factory_keystore_set("emulator", "profile", (uint8_t *)&profile_num, size);
    }

    return &profiles[profile_num];
}

int emulator_get_state()
{
    uint8_t state;
    size_t size = sizeof(state);
    if(hap_keystore_get("emulator", "state", (uint8_t *)&state, &size) == HAP_SUCCESS && state == 1) {
        return HAP_SUCCESS;
    } else
        return HAP_FAIL;
}

void hap_set_value(char * uuid, hap_val_t *val) {
    
    hap_serv_t *hs = hap_acc_get_first_serv(hap_get_first_acc());
        while(!hap_serv_get_char_by_uuid(hs, uuid))
            hs = hap_serv_get_next(hs);
        if(hs)
            hap_char_update_val(hap_serv_get_char_by_uuid(hs, uuid), val); 
}

void emulator_get_characteristics()
{   
    uint8_t brightness;
    size_t size = sizeof(brightness);
    if(hap_keystore_get("emulator", "brightness", (uint8_t *)&brightness, &size) == HAP_SUCCESS)
    {
        hap_val_t val;
        val.i = brightness;
        hap_set_value(HAP_CHAR_UUID_BRIGHTNESS, &val);
    }    
    
    uint32_t hue;
    if(hap_keystore_get_u32("emulator", "hue", &hue, "hue") == HAP_SUCCESS) {
        hap_val_t val;
        val.u = hue;
        hap_set_value(HAP_CHAR_UUID_HUE, &val);
    }
    
    uint32_t saturation;
    if(hap_keystore_get_u32("emulator", "saturation", &saturation, "saturation") == HAP_SUCCESS) {

        hap_val_t val;
        val.u = saturation;
        hap_set_value(HAP_CHAR_UUID_SATURATION, &val);
    }

}

void emulator_show_profiles()
{
    int i;
    printf("List of homekit profiles.\n");
    for (i = 0; i < profile_cnt; i++) {
        printf("%d: %s\n", i, profiles[i].name);
    }
}

int emulator_set_profile(int profile)
{
    if (profile >= profile_cnt || profile < 0) {
        printf("Invalid profile %d\n", profile);
        return -1;
    }
    profile_num = profile;
    hap_factory_keystore_set("emulator", "profile", (uint8_t *)&profile_num, sizeof(profile_num));
    return 0;
}

static char val[260];
static char * emulator_print_value(hap_char_t *hc, const hap_val_t *cval)
{
    uint16_t perm = hap_char_get_perm(hc);
    if (perm & HAP_CHAR_PERM_PR) {
        hap_char_format_t format = hap_char_get_format(hc);
	    switch (format) {
		    case HAP_CHAR_FORMAT_BOOL : {
                snprintf(val, sizeof(val), "%s", cval->b ? "true":"false");
    			break;
		    }
		    case HAP_CHAR_FORMAT_UINT8:
		    case HAP_CHAR_FORMAT_UINT16:
		    case HAP_CHAR_FORMAT_UINT32:
		    case HAP_CHAR_FORMAT_INT:
                snprintf(val, sizeof(val), "%d", cval->i);
                break;
    		case HAP_CHAR_FORMAT_FLOAT :
                snprintf(val, sizeof(val), "%f", cval->f);
		    	break;
    		case HAP_CHAR_FORMAT_STRING :
                if (cval->s) {
                    snprintf(val, sizeof(val), "%s", cval->s);
                } else {
                    snprintf(val, sizeof(val), "null");
                }
			    break;
            default :
                snprintf(val, sizeof(val), "unsupported");
		}
    } else {
        snprintf(val, sizeof(val), "null");
    }
    return val;
}

int emulator_write(hap_write_data_t write_data[], int count,
        void *serv_priv, void *write_priv)
{
    int i;
    hap_write_data_t *write;
    for (i = 0; i < count; i++) {
        write = &write_data[i];
        uint16_t perm = hap_char_get_perm(write->hc);
        if(perm & HAP_CHAR_PERM_AA && hex_compare(write->auth_data.data, write->auth_data.len)) {
            printf("Invalid Auth data\n");
            continue;
        }
        int iid = hap_char_get_iid(write->hc);
        int aid = hap_acc_get_aid(hap_serv_get_parent(hap_char_get_parent(write->hc)));
        printf("Write aid = %d, iid = %d, val = %s\n", aid, iid, emulator_print_value(write->hc, &(write->val)));
        hap_char_update_val(write->hc, &(write->val));
        const char * uuid = hap_char_get_type_uuid(write->hc);
        if (!strcmp(uuid, HAP_CHAR_UUID_ON))
            hap_keystore_set("emulator", "state", (uint8_t *)&write->val.b, sizeof(uint8_t));
        if (!strcmp(uuid, HAP_CHAR_UUID_BRIGHTNESS))
            hap_keystore_set("emulator", "brightness", (uint8_t *)&write->val.i, sizeof(uint8_t));
        if (!strcmp(uuid, HAP_CHAR_UUID_HUE))
            hap_keystore_set_u32("emulator", "hue", write->val.u, "hue");
        if (!strcmp(uuid, HAP_CHAR_UUID_SATURATION))
            hap_keystore_set_u32("emulator", "saturation", write->val.u, "saturation");
        *(write->status) = HAP_STATUS_SUCCESS;
    }
    return HAP_SUCCESS;
}

static void emulator_print_char(hap_char_t *hc)
{
    const hap_val_t *cval = hap_char_get_val(hc);
    printf("C %d %s %s\n", hap_char_get_iid(hc),
            hap_char_get_type_uuid(hc), emulator_print_value(hc, cval));
}

static void emulator_print_serv(hap_serv_t *hs)
{
    printf("S %d %s\n", hap_serv_get_iid(hs), hap_serv_get_type_uuid(hs));
    hap_char_t *hc;
    for (hc = hap_serv_get_first_char(hs); hc; hc = hap_char_get_next(hc)) {
        emulator_print_char(hc);
    }
}

static void emulator_print_acc(hap_acc_t *ha)
{
    printf("A %d\n", hap_acc_get_aid(ha));
    hap_serv_t *hs;
    for (hs = hap_acc_get_first_serv(ha); hs; hs = hap_serv_get_next(hs)) {
        emulator_print_serv(hs);
    }
}

static void emulator_print_all()
{
    printf("Accessory database\n");
    hap_acc_t *ha;
    for (ha = hap_get_first_acc(); ha; ha = hap_acc_get_next(ha)) {
        emulator_print_acc(ha);
    }
}
void emulator_read_chars(int num, ...)
{
    va_list valist;
    va_start(valist, num);

    if (num == 0) {
        emulator_print_all();
    } else {
        hap_acc_t *ha = hap_acc_get_by_aid(va_arg(valist, int));
        if (!ha) {
            printf("Invalid aid\n");
        } else {
            if (num == 1) 
            emulator_print_acc(ha);
            else if (num == 2) {
                hap_char_t *hc = hap_acc_get_char_by_iid(ha, va_arg(valist, int));
                if (!hc) {
                    printf("Invalid iid\n");
                } else {
                    emulator_print_char(hc);
                }
            }
        }
    }
}

void emulator_write_char(int aid, int iid, char *sval)
{
    if (!sval) {
        printf("Invalid val\n");
        return;
    }
    hap_acc_t *ha = hap_acc_get_by_aid(aid);
    if (!ha) {
        printf("Invalid aid\n");
        return;
    }
    hap_char_t *hc = hap_acc_get_char_by_iid(ha, iid);
    if (!hc) {
        printf("Invalid iid\n");
        return;
    }
    hap_char_format_t format = hap_char_get_format(hc);
    hap_val_t val;
    switch (format) {
	    case HAP_CHAR_FORMAT_BOOL : {
            if (!strcmp(sval, "true")) {
                val.b = true;
            } else if (!strcmp(sval, "false")) {
                val.b = false;
            } else {
                printf("Invalid val %s\n", sval);
                return;
            }
            const char * uuid = hap_char_get_type_uuid(hc);
            if (!strcmp(uuid, HAP_CHAR_UUID_ON))
                hap_keystore_set("emulator", "state", (uint8_t *)&val.b, sizeof(uint8_t));
            hap_char_update_val(hc, &val);
   			break;
	    }
	    case HAP_CHAR_FORMAT_UINT8:
	    case HAP_CHAR_FORMAT_UINT16:
	    case HAP_CHAR_FORMAT_UINT32:
	    case HAP_CHAR_FORMAT_INT: {
            char *start = sval;
            char *end = sval + strlen(sval);
            char *endptr;
            int i = strtoul(start, &endptr, 10);
            if (endptr == end) {
                val.i = i;
                hap_char_update_val(hc, &val);
            } else {
                printf("Invalid val %s\n", sval);
                return;
            }
            break;
        }
   		case HAP_CHAR_FORMAT_FLOAT : {
            char *start = sval;
            char *end = sval + strlen(sval);
            char *endptr;
            float f = strtof(start, &endptr);
            if (endptr == end) {
                val.f = f;
                hap_char_update_val(hc, &val);
            } else {
                printf("Invalid val %s\n", sval);
                return;
            }
          	break;
        }
   		case HAP_CHAR_FORMAT_STRING :
            val.s = sval;
            hap_char_update_val(hc, &val);
       	    break;
        default :
            printf("Format not supported\n"); 
    }
}

void emulator_reset_to_factory() 
{
    hap_reset_to_factory();
}

void emulator_reset_pairings() 
{
    hap_reset_pairings();
}

void emulator_switch_profile(int profile) 
{
    int ret = emulator_set_profile(profile);
    if(ret == -1) {
    } else {
        hap_reboot_accessory();
    }
}

void emulator_set_wac(int method)
{
    if(method == 1 || method ==2) {
        hap_factory_keystore_set("wac", "method", (uint8_t *)&method, sizeof(method));
        hap_reboot_accessory();
    }
    else
        printf("Invalid WAC Id.");
}

void emulator_set_auth(int method)
{
    int wac = 2;
    if(method == 1 || method ==2) {
        hap_factory_keystore_set("auth", "method", (uint8_t *)&method, sizeof(method));
        printf("Enforcing WAC2\n");
        hap_factory_keystore_set("wac", "method", (uint8_t *)&wac, sizeof(wac));
        hap_reboot_accessory();
    }
    else
        printf("Invalid Auth Id.");
}

void emulator_reset_wifi_credentials()
{
    hap_reset_network();
}

void emulator_reboot_accessory()
{
    hap_reboot_accessory();
}
