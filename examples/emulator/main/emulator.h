#ifndef _EMULATOR_H_
#define _EMULATOR_H_
#include <hap.h>

typedef struct {
    hap_cid_t cid;
    char *name;
    hap_serv_t * (*serv_create) (void);
} hap_emulator_profile_t;
int emulator_write(hap_write_data_t write_data[], int count,
        void *serv_priv, void *write_priv);
hap_emulator_profile_t * emulator_get_profile();
int emulator_get_state();
void emulator_get_characteristics(); 
void emulator_read_chars(int num, ...);
void emulator_write_char(int aid, int iid, char *sval);
void emulator_show_profiles();
void emulator_reset_to_factory();
void emulator_reset_pairings();
void emulator_switch_profile(int profile);
void emulator_set_wac();
void emulator_set_auth();
void emulator_reset_wifi_credentials();
void emulator_reboot_accessory();
void console_init();
void register_system();

#endif /* _EMULATOR_H_ */
