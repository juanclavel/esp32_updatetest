//// Funciones
//void *sas_thread(void *arg);
//int sas_inicia(string puerto,char direccion_sas,char* error,string desc_error);
//unsigned short sas_CRC(unsigned char *,int, unsigned short);
//void sas_escribe_puerto_sas(char num_txbytes,bool PARITY);
//void sas_pop_event(void);
//void sas_pop_handpay(void);
//void sas_procesa_comando_tipo_R(unsigned char);
//void sas_procesa_comando_tipo_S(unsigned char);
//void sas_traslada_ToBcd_salida(uint value, unsigned char posicion);
//void sas_anula_confirmacion_pendiente_long(void);
//void sas_push_event(char);
//void sas_set_fifo_events(char *);
//void sas_set_counter_address(unsigned int *,unsigned int *);
//void sas_set_pointer_address(int,void *);
//void sas_set_game_data(string game_id,string game_aditional_id,string base_porcent,string paytable_id,
//    string sas_version,string serial_number,char denomination, char max_bet, char progres_group,char * game_options);
//



// Tabla de tipos de comando, R=0, S=1, M=2, G=3, unknown =5
const char SASLongPollCommandsType[256]={
5,1,1,1,1,1,1,1,1,2,1,1,5,5,1,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,5,0,0,
0,1,5,5,5,5,5,5,5,5,0,0,0,2,1,2,
5,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,5,1,1,5,0,
1,0,2,2,0,0,0,0,1,5,5,5,5,5,5,5,
5,5,5,5,5,5,5,5,5,5,5,5,5,5,1,2,
0,1,1,1,1,1,1,5,5,5,5,1,1,1,0,1,
1,5,5,2,0,0,1,0,5,5,1,1,2,5,0,0,
0,5,5,5,1,2,2,2,2,2,2,5,5,5,5,5,
2,5,5,5,2,5,5,5,1,7,1,5,5,5,5,2,
1,0,0,0,2,2,5,5,5,5,5,5,5,5,5,5};


// SAS EVENTS
#define SAS_EV_NO_EXCEPTION_ACTIVITY    0x00
#define SAS_EV_ACK                      0x01
#define SAS_EV_SLOT_DOOR_OPENED         0x11
#define SAS_EV_SLOT_DOOR_CLOSED         0x12
#define SAS_EV_SLOT_DROP_OPENED         0x13
#define SAS_EV_SLOT_DROP_CLOSED         0x14
#define SAS_EV_SLOT_CARD_CAGE_OPENED    0x15
#define SAS_EV_SLOT_CARD_CAGE_CLOSED    0x16
#define SAS_EV_AC_POWER_APPLIED         0x17
#define SAS_EV_AC_POWER_LOST            0x18
#define SAS_EV_CASHBOX_DOOR_OPENED      0x19
#define SAS_EV_CASHBOX_DOOR_CLOSED      0x1a
#define SAS_EV_CASHBOX_REMOVED          0x1b
#define SAS_EV_CASHBOX_INSTALLED        0x1c
#define SAS_EV_BELLY_DOOR_OPENED        0x1d
#define SAS_EV_BELLY_DOOR_CLOSED        0x1e
#define SAS_EV_WAITING_USER_INPUT       0x1f
#define SAS_EV_GENERAL_TILT             0x20
#define SAS_EV_COIN_IN_TILT             0x21
#define SAS_EV_COIN_OUT_TILT            0x22
#define SAS_EV_HOPPER_EMPTY             0x23
#define SAS_EV_EXTRA_COIN_PAID          0x24;
#define SAS_EV_DIVERTER_MALFUNCTION     0x25
#define SAS_EV_CAHSBOX_FULL_DETECTED    0x27
#define SAS_EV_BILL_JAM_DETECTED        0x28
#define SAS_EV_BILL_ACCEPTOR_FAILURE    0x29
#define SAS_EV_REVERSE_BILL_DETECTED    0x2a
#define SAS_EV_BILL_REJECTED            0x2b
#define SAS_EV_COUNTERFEIT_BILL_DETECTED 0x2c
#define SAS_EV_REVERSE_COIN_IN_DETECTED 0x2d
#define SAS_EV_CMOS_RAM_ERROR_RECOV     0x31
#define SAS_EV_CMOS_RAM_ERROR_NO_RECOV  0x32
#define SAS_EV_CMOS_RAM_BAD_DEVICE      0x33
#define SAS_EV_EEPROM_DATA_ERROR        0x34
#define SAS_EV_EEPROM_BAD_DEVICE        0x35
#define SAS_EV_EEPROM_CHECKSUM_VERSION  0x36
#define SAS_EV_EEPROM_CHECKSUM_COMPARE  0x37
#define SAS_EV_EEPROM_PART_CHKSUM_VERS  0x38
#define SAS_EV_EEPROM_PART_CHKSUM_COMP  0x39
#define SAS_EV_MEMORY_ERROR_RESET       0x3A
#define SAS_EV_LOW_BACKUP_BATTERY       0x3B
#define SAS_EV_OPERATOR_CHANGED_OPTIONS 0x3C
#define SAS_EV_CASHOUT_TICKET_PRINTED   0x3D
#define SAS_EV_HANDPAY_VALIDATED        0x3E
#define SAS_EV_VALIDATION_ID_NOT_VAL    0x3F
#define SAS_EV_REEL_TILT_UNSPECIFIED    0x40
#define SAS_EV_REEL_1_TILT              0x41
#define SAS_EV_REEL_2_TILT              0x42
#define SAS_EV_REEL_3_TILT              0x43
#define SAS_EV_REEL_4_TILT              0x44
#define SAS_EV_REEL_5_TILT              0x45
#define SAS_EV_REEL_MECH_DISCONNECTED   0x46
#define SAS_EV_BILL_1_ACCEPTED          0x47
#define SAS_EV_BILL_5_ACCEPTED          0x48
#define SAS_EV_BILL_10_ACCEPTED         0x49
#define SAS_EV_BILL_20_ACCEPTED         0x4A
#define SAS_EV_BILL_50_ACCEPTED         0x4B
#define SAS_EV_BILL_100_ACCEPTED        0x4C
#define SAS_EV_BILL_2_ACCEPTED          0x4D
#define SAS_EV_BILL_500_ACCEPTED        0x4E
#define SAS_EV_BILL_ACCEPTED            0x4F
#define SAS_EV_BILL_200_ACCEPTED        0x50
#define SAS_EV_HANDPAY_IS_PENDING       0x51
#define SAS_EV_HANDPAY_WAS_RESET        0x52
#define SAS_EV_NO_PROGRESSIVE_DATA      0x53
#define SAS_EV_PROGRESSIVE_WIN          0x54
#define SAS_EV_HANDPAY_PLAYER_CANCELLED 0x55
#define SAS_EV_SAS_PROGRESSIVE_LEVEL_HIT 0x56
#define SAS_EV_SYSTEM_VALIDATION_REQUEST 0x57
#define SAS_EV_PRINTER_COM_ERROR        0x60
#define SAS_EV_PRINTER_PAPER_ERROR      0x61
#define SAS_EV_CASHOUT_WIN_REQUEST      0x65
#define SAS_EV_CASHOUT_BUTTON_PRESSED   0x66
#define SAS_EV_TICKET_INSERTED          0x67
#define SAS_EV_TICKET_TRANSFER_COMPLETE 0x68
#define SAS_EV_AFT_TRANSFER_COMPLETE    0x69
#define SAS_EV_AFT_REQUEST_FOR_HOST_CAHSOUT 0x6A
#define SAS_EV_AFT_REQUEST_FOR_HOST_WIN_CAHSOUT 0x6B
#define SAS_EV_AFT_REQUEST_TO_REGISTER  0x6C
#define SAS_EV_AFT_REGISTRATION_ACK     0X6D
#define SAS_EV_AFT_REGISTRATION_CANC    0X6E
#define SAS_EV_GAME_LOCKED              0X6F
#define SAS_EV_AFT_EXCEPTION_BUFFER_OVF 0X70
#define SAS_EV_CHANGE_LAMP_ON           0X71
#define SAS_EV_CHANGE_LAMP_OFF          0X72
#define SAS_EV_GAME_RESET_IN_PAYOUT     0X73
#define SAS_EV_PRINTER_PAPER_LOW        0x74
#define SAS_EV_PRINTER_PAPER_OFF        0x75
#define SAS_EV_PRINTER_PAPER_ON         0x76
#define SAS_EV_REPLACE_PRINTER_RIBBON   0x77
#define SAS_EV_PRINTER_CARRIER_JAMMED   0x78
#define SAS_EV_COIN_IN_LOCKOUT_MALFCT   0x79
#define SAS_EV_SOFT_METERS_RESET        0x7A
#define SAS_EV_BILLS_VALID_TOTAL_RESET  0x7B
#define SAS_EV_LEGACY_BONUS_PAY         0x7C
#define SAS_EV_GAME_STARTED             0x7E
#define SAS_EV_GAME_ENDED               0x7F
#define SAS_EV_HOPPER_FULL_DETECTED     0x80
#define SAS_EV_HOPPER_LEVEL_LOW         0x81
#define SAS_EV_ENTER_TO_DISPLAY_METERS  0x82
#define SAS_EV_EXIT_TO_DISPLAY_METERS   0x83
#define SAS_EV_ENTER_TO_SELF_TEST       0x84
#define SAS_EV_EXIT_TO_SELF_TEST        0x85
#define SAS_EV_GAME_IS_OUT_SERVICE      0x86
#define SAS_EV_PLAYER_REQUEST_CARDS     0x87
#define SAS_EV_REEL_N_HAS_STOPPED       0X88
#define SAS_EV_COIN_CREDIT_WAGERED      0X89
#define SAS_EV_GAME_RECALL_ENTRY        0X8A
#define SAS_EV_CARD_HELD_NOHELD         0X8B
#define SAS_EV_GAME_SELECTED            0X8C
#define SAS_EV_COMPONENT_LIST_CHANGED   0X8E
#define SAS_EV_AUTHENTICATION_COMPLET   0X8F
#define SAS_EV_POWER_OFF_CARD_CAGE      0X98
#define SAS_EV_POWER_OFF_SLOT_DOOR      0X99
#define SAS_EV_POWER_OFF_CASBOX_DOOR    0X9A
#define SAS_EV_POWER_OFF_DROP_DOR       0X9B
#define SAS_EV_METER_CHANGE_PENDING     0XA0
#define SAS_EV_METER_CHANGE_CANCELLED   0XA1
#define SAS_EV_GAMES_DENOM_CHANGED      0XA2

enum SasCounters {
SAS_TOTAL_COIN_IN,
SAS_TOTAL_COIN_OUT,
SAS_TOTAL_DROP,
SAS_TOTAL_JACKPOT,
SAS_GAMES_PLAYED,
SAS_GAMES_WON,
SAS_GAMES_LOST,
SAS_GAMES_SINCE_POWER_RESET,
SAS_DOOR_OPEN,
SAS_POWER_UP,
SAS_CREDITS,
SAS_CREDIT_VALUE,
SAS_TRUE_COINS_IN,
SAS_TRUE_COINS_OUT,
SAS_TOTAL_HAND_PAY,
SAS_TOTAL_CANCELLED,
SAS_DOLLARS_FROM_BILLS,
SAS_BILLS_X_DENO_1,
SAS_BILLS_X_DENO_2,
SAS_BILLS_X_DENO_3,
SAS_BILLS_X_DENO_4,
SAS_BILLS_X_DENO_5,
SAS_BILLS_X_DENO_6,
SAS_BILLS_X_DENO_7,
SAS_BILLS_X_DENO_8,
SAS_BILLS_X_DENO_9,
SAS_BILLS_X_DENO_10,
SAS_BILLS_X_DENO_11,
SAS_BILLS_X_DENO_12,
SAS_BILLS_X_DENO_13,
SAS_BILLS_X_DENO_14,
SAS_BILLS_X_DENO_15,
SAS_BILLS_X_DENO_16,
SAS_BILLS_X_DENO_17,
SAS_BILLS_X_DENO_18,
SAS_BILLS_X_DENO_19,
SAS_BILLS_X_DENO_20,
SAS_BILLS_X_DENO_21,
SAS_BILLS_X_DENO_22,
SAS_BILLS_X_DENO_23,
SAS_BILLS_X_DENO_24
};

// byte sasAFT_Key[]={0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x10,0x11,0x012,0x13,0x14,0x15,0x16,0x17,0x18,0x19,0x20};
// byte sasAFT_Asset[]={0x01,0x00, 0x00, 0x00};
// byte sasAFT_POS[]={0x01,0x00, 0x00, 0x00};
/*
    sas_game_id=&sas_def_game_id[0];
    sas_game_aditional_id=&sas_def_game_aditional_id[0];
    sas_denomination=&sas_def_denomination;
    sas_max_bet=&sas_def_max_bet;
    sas_progresive_group=&sas_def_progresive_group;
    sas_paytable_id=&sas_def_paytable_id[0];
    sas_base_percentage=&sas_def_paytable_id[0];
*/

