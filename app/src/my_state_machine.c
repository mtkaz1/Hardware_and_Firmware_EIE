/**
 * @file my_state_machine.c
 */

#include <zephyr/smf.h>
#include "LED.h"     
#include "my_state_machine.h"
#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>
#include <zephyr/devicetree.h>
#include <zephyr/sys/util.h>
#include "BTN.h"


#define SW0_NODE DT_ALIAS(sw0)
#define SW1_NODE DT_ALIAS(sw1)
#define SW2_NODE DT_ALIAS(sw2)
#define SW3_NODE DT_ALIAS(sw3)
#define EVENT_BTN_PRESS BIT(0)


#define LED0_NODE DT_ALIAS(led0)
#define LED1_NODE DT_ALIAS(led1)
#define LED2_NODE DT_ALIAS(led2)
#define LED3_NODE DT_ALIAS(led3)




/*--------------------------------------------------------------------------------------*
 * Function Prototypes
 *--------------------------------------------------------------------------------------*/
static const struct smf_state demo_states[];
static const struct gpio_dt_spec button = GPIO_DT_SPEC_GET_OR(SW0_NODE, gpios, {0});
static void S0_entry(void *o);
static enum smf_state_result S0_run(void *o);
static void S1_entry(void *o);
static enum smf_state_result S1_run(void *o);
static void S2_entry(void *o);
static enum smf_state_result S2_run(void *o);
static void S3_entry(void *o);
static enum smf_state_result S3_run(void *o);

/*--------------------------------------------------------------------------------------*
 * Typedefs
 *--------------------------------------------------------------------------------------*/

 typedef struct {
  bool btn0_click;
  bool btn1_click;
  bool btn2_click;
  bool btn3_click;
  bool standby;     // BTN0+BTN1 held for 3s (true for ONE tick)
} buttons_t;

static void read_buttons(buttons_t *b);  // <-- buttons_t not defined yet

enum demo_state { S0, S1, S2, S3};

#define MAX_STR_LEN 64
#define PULSE_MS 100
struct s_object {
        /* This must be first */
        struct smf_ctx ctx;

        /* Events */
        struct k_event smf_event;
        uint32_t events;

        buttons_t buttons;


         /* 8-bit entry */
        uint8_t current_byte;
        uint8_t bit_count;      // 0..8

        /* stored string */
        char str[MAX_STR_LEN];
        uint8_t str_len;        // 0..63

        /* standby return */
        enum demo_state prev_state;   
        
        /* standby PWM breathing */
        uint8_t duty;
        int8_t duty_dir;
        int64_t last_pwm_ms;

        int64_t led0_off_time;
        int64_t led1_off_time;
} s_obj;


/*--------------------------------------------------------------------------------------*
 * Local Variables
 *--------------------------------------------------------------------------------------*/
static const struct smf_state demo_states[] = {
    [S0]  = SMF_CREATE_STATE(S0_entry, S0_run, NULL, NULL, NULL),
    [S1]  = SMF_CREATE_STATE(S1_entry, S1_run, NULL, NULL, NULL),
    [S2]  = SMF_CREATE_STATE(S2_entry, S2_run, NULL, NULL, NULL),
    [S3]  = SMF_CREATE_STATE(S3_entry, S3_run, NULL, NULL, NULL),
};

/*--------------------------------------------------------------------------------------*
 * Public Functions
 *--------------------------------------------------------------------------------------*/
void state_machine_init() {
        s_obj.current_byte = 0;
        s_obj.bit_count = 0;
        s_obj.str_len = 0;
        s_obj.str[0] = '\0';
        s_obj.prev_state = S0;

        s_obj.duty = 0;
        s_obj.duty_dir = 2;
        s_obj.last_pwm_ms = 0;

        s_obj.led0_off_time = 0;
        s_obj.led1_off_time = 0;
        smf_set_initial(SMF_CTX(&s_obj), &demo_states[S0]);
}

static void read_buttons(buttons_t *b) {
        *b =  (buttons_t){0};

        b->btn0_click = BTN_check_clear_pressed(BTN0);
        b->btn1_click = BTN_check_clear_pressed(BTN1);
        b->btn2_click = BTN_check_clear_pressed(BTN2);
        b->btn3_click = BTN_check_clear_pressed(BTN3);

        // standby

        static int64_t start_time = 0;
        int64_t now = k_uptime_get();
        bool both = BTN_is_pressed(BTN0) && BTN_is_pressed(BTN1);

        if (!both) {
                start_time = 0;               // released -> reset
        } else if (start_time == 0) {
                start_time = now;             // started holding -> record time
        } else if (start_time > 0 && (now - start_time) >= 3000) {
                b->standby = true;            // trigger once
                start_time = -1;              // don't retrigger until release
        }
}

int state_machine_run() {
        read_buttons(&s_obj.buttons);

        int64_t now = k_uptime_get();
        if (s_obj.led0_off_time && now >= s_obj.led0_off_time) {
                LED_set(LED0, LED_OFF);
                s_obj.led0_off_time = 0;
        }
        if (s_obj.led1_off_time && now >= s_obj.led1_off_time) {
                LED_set(LED1, LED_OFF);
                s_obj.led1_off_time = 0;
        }
        return smf_run_state(SMF_CTX(&s_obj)); 
}

static void reset_bits(struct s_object *s) {
    s->current_byte = 0;
    s->bit_count = 0;
    printk("BITS RESET\n");
}

static void append_bit(struct s_object *s, uint8_t bit) {
        if (s->bit_count >= 8){
                printk("Already have 8 bits, press BTN3 to save or BTN2 to reset.\n");
                return;
        }
         

        s->current_byte = (uint8_t)((s->current_byte << 1) | (bit & 1));
        s->bit_count++;

        printk("BIT=%d  count=%d  current_byte=0x%02X\n",
                bit, s->bit_count, s->current_byte);

        if (s->bit_count == 8) {
                char c = (char)s->current_byte;
                printk("BYTE COMPLETE: 0x%02X  '%c'\n", s->current_byte,
                (c >= 32 && c <= 126) ? c : '.');
    }
}

static bool save_char(struct s_object *s) {
     if (s->bit_count != 8) {
        printk("SAVE CHAR FAILED: need 8 bits, have %d\n", s->bit_count);
        return false;
        }

    if (s->str_len >= (MAX_STR_LEN - 1)) {
        printk("String full (max %d)\n", MAX_STR_LEN - 1);
        return false;
    }


    char c = (char)s->current_byte;
    uint8_t byte = s->current_byte;

    s->str[s->str_len++] = c;
    s->str[s->str_len] = '\0';

    printk("SAVED CHAR: 0x%02X '%c'   STRING=\"%s\"\n",
           byte, (c >= 32 && c <= 126) ? c : '.', s->str);

    reset_bits(s);
    return true;
}

static void clear_string(struct s_object *s) {
    s->str_len = 0;
    s->str[0] = '\0';
    reset_bits(s);
    printk("String Cleared\n");
}
        


/*--------------------------------------------------------------------------------------*
 * State Implementations
 *--------------------------------------------------------------------------------------*/

/* ----------------------------- STATE S0: CHAR_ENTRY -----------------------------
   - BTN0 appends bit 0 (LED0 toggles)
   - BTN1 appends bit 1 (LED1 toggles)
   - BTN2 resets bits
   - BTN3 saves char -> S1
   - LED3 blinks 1 Hz
------------------------------------------------------------------------------- */


 static void S0_entry(void *o)
{
        (void)o;
        printk("S0: CHAR_ENTRY\n");
        LED_blink(LED3, LED_1HZ);
        reset_bits(&s_obj);
}

static enum smf_state_result S0_run(void *o)
{
        struct s_object *s = (struct s_object *)o;

        if (s->buttons.standby) {
                s->prev_state = S0;
                smf_set_state(SMF_CTX(s), &demo_states[S3]);
                return SMF_EVENT_HANDLED;
        }

        /* Change states on Button Press Event */
            if (s->buttons.btn0_click) {
        LED_set(LED0, LED_ON);
        s->led0_off_time = k_uptime_get() + PULSE_MS;
        append_bit(s, 0);
    }

    if (s->buttons.btn1_click) {
        LED_set(LED1, LED_ON);
        s->led1_off_time = k_uptime_get() + PULSE_MS;
        append_bit(s, 1);
    }

    if (s->buttons.btn2_click) {
        reset_bits(s);
    }

    if (s->buttons.btn3_click) {
        if (save_char(s)) {
            smf_set_state(SMF_CTX(s), &demo_states[S1]);
        }
    }

    return SMF_EVENT_HANDLED;
}

/* ----------------------------- STATE S1: STRING_EDIT -----------------------------
   - BTN0/BTN1 enter bits for next char (LED0/LED1 toggles)
   - BTN2 deletes entire string -> S0
   - BTN3:
       if bit_count == 8 -> save another char (keep building)
       else -> save entire string -> S2
   - LED3 blinks 4 Hz
------------------------------------------------------------------------------- */

static void S1_entry(void *o)
{
        (void)o;
        printk("S1: STRING_EDIT str=\"%s\"\n", s_obj.str);
        LED_blink(LED3, LED_4HZ);
}

static enum smf_state_result S1_run(void *o)
{
    struct s_object *s = (struct s_object *)o;

    if (s->buttons.standby) {
        s->prev_state = S1;
        smf_set_state(SMF_CTX(s), &demo_states[S3]);
        return SMF_EVENT_HANDLED;
    }

    if (s->buttons.btn2_click) {
        clear_string(s);
        smf_set_state(SMF_CTX(s), &demo_states[S0]);
        return SMF_EVENT_HANDLED;
    }

    if (s->buttons.btn0_click) {
        LED_set(LED0, LED_ON);
        s->led0_off_time = k_uptime_get() + PULSE_MS;
        append_bit(s, 0);
    }

    if (s->buttons.btn1_click) {
        LED_set(LED1, LED_ON);
        s->led1_off_time = k_uptime_get() + PULSE_MS;
        append_bit(s, 1);
    }

    if (s->buttons.btn3_click) {
        if (s->bit_count == 8) {
            (void)save_char(s);               // add another char
        } else {
            printk("STRING SAVED. Ready to send.\n");
            smf_set_state(SMF_CTX(s), &demo_states[S2]); // save whole string
        }
    }

    return SMF_EVENT_HANDLED;
}
/* ----------------------------- STATE S2: STRING_READY -----------------------------
   - BTN2 deletes string -> S0
   - BTN3 prints to serial
   - LED3 blinks 16 Hz
------------------------------------------------------------------------------- */
static void S2_entry(void *o)
{
        (void)o;
        printk("S2: STRING_READY\n");
        LED_blink(LED3, LED_16HZ);
}

static enum smf_state_result S2_run(void *o)
{
           struct s_object *s = (struct s_object *)o;

    if (s->buttons.standby) {
        s->prev_state = S2;
        smf_set_state(SMF_CTX(s), &demo_states[S3]);
        return SMF_EVENT_HANDLED;
    }

    if (s->buttons.btn2_click) {
        clear_string(s);
        smf_set_state(SMF_CTX(s), &demo_states[S0]);
        return SMF_EVENT_HANDLED;
    }

    if (s->buttons.btn3_click) {
        printk("SEND: %s\n", s->str);
    }

    return SMF_EVENT_HANDLED;

}

/* ----------------------------- STATE S3: STANDBY -----------------------------
   - All LEDs “breathe” with PWM
   - Any button click exits and returns to previous state
------------------------------------------------------------------------------- */

static void S3_entry(void *o)
{
        struct s_object *s = (struct s_object *)o;
        printk("S3: STANDBY (return to S%d)\n", (int)s->prev_state);

        s->duty = 0;
        s->duty_dir = 2;                // step size
        s->last_pwm_ms = k_uptime_get();
}

static enum smf_state_result S3_run(void *o)
{
    struct s_object *s = (struct s_object *)o;

    // exit standby on any click
    if (s->buttons.btn0_click || s->buttons.btn1_click || s->buttons.btn2_click || s->buttons.btn3_click) {
        // stop PWM / reset LEDs so prior state's blink looks correct immediately
        LED_set(LED0, LED_OFF);
        LED_set(LED1, LED_OFF);
        LED_set(LED2, LED_OFF);
        LED_set(LED3, LED_OFF);

        smf_set_state(SMF_CTX(s), &demo_states[s->prev_state]);
        return SMF_EVENT_HANDLED;
    }
    // PWM breathing
    // If LED_pwm expects 0..100, change max 255 -> 100.
    int64_t now = k_uptime_get();
    if ((now - s->last_pwm_ms) >= 10) {
        s->last_pwm_ms = now;

        LED_pwm(LED0, s->duty);
        LED_pwm(LED1, s->duty);
        LED_pwm(LED2, s->duty);
        LED_pwm(LED3, s->duty);

        if (s->duty >= 255) s->duty_dir = -2;
        if (s->duty <= 0)   s->duty_dir =  2;

        s->duty = (uint8_t)(s->duty + s->duty_dir);
    }

    return SMF_EVENT_HANDLED;

}
