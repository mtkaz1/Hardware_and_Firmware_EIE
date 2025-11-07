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
static void S0_run(void *o);
static void S1_entry(void *o);
static void S1_run(void *o);
static void S2_entry(void *o);
static void S2_run(void *o);
static void S3_entry(void *o);
static void S3_run(void *o);

/*--------------------------------------------------------------------------------------*
 * Typedefs
 *--------------------------------------------------------------------------------------*/

enum demo_state { S0, S1, S2, S3};

struct s_object {
        /* This must be first */
        struct smf_ctx ctx;

        /* Events */
        struct k_event smf_event;
        int32_t events;

        /* Other state specific data add here */
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
    smf_set_initial(SMF_CTX(&s_obj), &led_states[S0]);
}

int state_machine_run() {
    return smf_run_state(SMF_CTX(&s_obj));
}

/*--------------------------------------------------------------------------------------*
 * State Implementations
 *--------------------------------------------------------------------------------------*/
static void s0_entry(void *o)
{
        printk("STATE0\n");
}

static void S0_run(void *o)
{
        struct s_object *s = (struct s_object *)o;

        /* Change states on Button Press Event */
        if (s->events & EVENT_BTN_PRESS) {
                smf_set_state(SMF_CTX(&s_obj), &demo_states[S1]);
        }
        return SMF_EVENT_HANDLED;
}

/* State S1 */
static void S1_entry(void *o)
{
        printk("STATE1\n");
}

static void S1_run(void *o)
{
        struct s_object *s = (struct s_object *)o;
        /* Change states on Button Press Event */
        if (s->events & EVENT_BTN_PRESS) {
                smf_set_state(SMF_CTX(&s_obj), &demo_states[S0]);
        }
        return SMF_EVENT_HANDLED;
}
/* State S2 */
static void S2_entry(void *o)
{
        printk("STATE2\n");
}

static void S2_run(void *o)
{
        struct s_object *s = (struct s_object *)o;

        /* Change states on Button Press Event */
        if (s->events & EVENT_BTN_PRESS) {
                smf_set_state(SMF_CTX(&s_obj), &demo_states[S1]);
        }
        return SMF_EVENT_HANDLED;
}

/* State S3 */
static void S3_entry(void *o)
{
        printk("STATE3\n");
}

static void S3_run(void *o)
{
        struct s_object *s = (struct s_object *)o;

        /* Change states on Button Press Event */
        if (s->events & EVENT_BTN_PRESS) {
                smf_set_state(SMF_CTX(&s_obj), &demo_states[S0]);
        }
        return SMF_EVENT_HANDLED;
}
