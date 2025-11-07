/**
 * @file my_state_machine.c
 */

#include <zephyr/smf.h>
#include "LED.h"     
#include "my_state_machine.h"
#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>
#define DT_ALIAS(sw0)
#define SW0_NODE
#define EVENT_BTN_PRESS BIT(0)

/*--------------------------------------------------------------------------------------*
 * Function Prototypes
 *--------------------------------------------------------------------------------------*/
static const struct smf_state demo_states[];
static void led_on_state_entry(void *o);
static enum smf_state_result led_on_state_run(void *o);
static void led_off_state_entry(void *o);
static enum smf_state_result led_off_state_run(void *o);
static const struct gpio_dt_spec button =
        GPIO_DT_SPEC_GET_OR(SW0_NODE, gpios, {0});
static struct gpio_callback button_cb_data;

/*--------------------------------------------------------------------------------------*
 * Typedefs
 *--------------------------------------------------------------------------------------*/
enum led_state_machine_states {
    LED_ON_STATE,
    LED_OFF_STATE
};

enum demo_state { S0, S1, S2, S3, };

struct s_object {
        /* This must be first */
        struct smf_ctx ctx;

        /* Events */
        struct k_event smf_event;
        int32_t events;

        /* Other state specific data add here */
} s_obj;

typedef struct {
    // Context variable used by Zephyr to track state machine state. Must be first.
    struct smf_ctx ctx;
    uint16_t count;
} led_state_object_t;

/*--------------------------------------------------------------------------------------*
 * Local Variables
 *--------------------------------------------------------------------------------------*/
static const struct smf_state led_states[] = {
    [LED_ON_STATE]  = SMF_CREATE_STATE(led_on_state_entry, led_on_state_run, NULL, NULL, NULL),
    [LED_OFF_STATE] = SMF_CREATE_STATE(led_off_state_entry, led_off_state_run, NULL, NULL, NULL)
};

static led_state_object_t led_state_object;

/*--------------------------------------------------------------------------------------*
 * Public Functions
 *--------------------------------------------------------------------------------------*/
void state_machine_init() {
    led_state_object.count = 0;
    smf_set_initial(SMF_CTX(&led_state_object), &led_states[LED_ON_STATE]);
}

int state_machine_run() {
    return smf_run_state(SMF_CTX(&led_state_object));
}

/*--------------------------------------------------------------------------------------*
 * State Implementations
 *--------------------------------------------------------------------------------------*/
static void s0_entry(void *o)
{
        printk("STATE0\n");
}

static void s0_run(void *o)
{
        struct s_object *s = (struct s_object *)o;

        /* Change states on Button Press Event */
        if (s->events & EVENT_BTN_PRESS) {
                smf_set_state(SMF_CTX(&s_obj), &demo_states[S1]);
        }
        return SMF_EVENT_HANDLED;
}

/* State S1 */
static void s1_entry(void *o)
{
        printk("STATE1\n");
}

static void s1_run(void *o)
{
        struct s_object *s = (struct s_object *)o;

        /* Change states on Button Press Event */
        if (s->events & EVENT_BTN_PRESS) {
                smf_set_state(SMF_CTX(&s_obj), &demo_states[S0]);
        }
        return SMF_EVENT_HANDLED;
}
/* State S2 */
static void s2_entry(void *o)
{
        printk("STATE2\n");
}

static void s2_run(void *o)
{
        struct s_object *s = (struct s_object *)o;

        /* Change states on Button Press Event */
        if (s->events & EVENT_BTN_PRESS) {
                smf_set_state(SMF_CTX(&s_obj), &demo_states[S1]);
        }
        return SMF_EVENT_HANDLED;
}

/* State S3 */
static void s3_entry(void *o)
{
        printk("STATE3\n");
}

static void s3_run(void *o)
{
        struct s_object *s = (struct s_object *)o;

        /* Change states on Button Press Event */
        if (s->events & EVENT_BTN_PRESS) {
                smf_set_state(SMF_CTX(&s_obj), &demo_states[S0]);
        }
        return SMF_EVENT_HANDLED;
}
/* State S4 */
static void s4_entry(void *o)
{
        printk("STATE4\n");
}

static void s4_run(void *o)
{
        struct s_object *s = (struct s_object *)o;

        /* Change states on Button Press Event */
        if (s->events & EVENT_BTN_PRESS) {
                smf_set_state(SMF_CTX(&s_obj), &demo_states[S0]);
        }
        return SMF_EVENT_HANDLED;
}

