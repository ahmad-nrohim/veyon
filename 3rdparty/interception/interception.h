#pragma once

#ifdef __cplusplus
extern "C" {
#endif

typedef void *InterceptionContext;
typedef int InterceptionDevice;
typedef int InterceptionFilter;
typedef int InterceptionPrecedence;
typedef int (*InterceptionPredicate)(InterceptionDevice device);

typedef struct
{
    unsigned short state;
    unsigned short flags;
    short rolling;
    int x;
    int y;
    unsigned int information;
} InterceptionMouseStroke;

typedef struct
{
    unsigned short code;
    unsigned short state;
    unsigned int information;
} InterceptionKeyStroke;

typedef char InterceptionStroke[sizeof(InterceptionMouseStroke)];

InterceptionContext interception_create_context(void);
void interception_destroy_context(InterceptionContext context);
InterceptionPrecedence interception_get_precedence(InterceptionContext context, InterceptionDevice device);
void interception_set_precedence(InterceptionContext context, InterceptionDevice device, InterceptionPrecedence precedence);
InterceptionFilter interception_get_filter(InterceptionContext context, InterceptionDevice device);
void interception_set_filter(InterceptionContext context, InterceptionPredicate predicate, InterceptionFilter filter);
InterceptionDevice interception_wait(InterceptionContext context);
InterceptionDevice interception_wait_with_timeout(InterceptionContext context, unsigned long milliseconds);
int interception_send(InterceptionContext context, InterceptionDevice device, const InterceptionStroke *stroke, unsigned int nstroke);
int interception_receive(InterceptionContext context, InterceptionDevice device, InterceptionStroke *stroke, unsigned int nstroke);
unsigned int interception_get_hardware_id(InterceptionContext context, InterceptionDevice device, void *hardware_id_buffer, unsigned int buffer_size);
int interception_is_invalid(InterceptionDevice device);
int interception_is_keyboard(InterceptionDevice device);
int interception_is_mouse(InterceptionDevice device);

#ifdef __cplusplus
}
#endif
