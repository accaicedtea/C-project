

#include "utils/gesture_manager.h"
#include <math.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

/* ================================================================
 * TAP / DOUBLE-TAP / LONG-PRESS  (unified TapGestureContext)
 * ================================================================ */

static gboolean on_single_tap_timer(gpointer user_data) {
    TapGestureContext *ctx = user_data;
    ctx->_single_timer = 0;
    if (ctx->on_tap)
        ctx->on_tap(ctx->widget, ctx->user_data);
    return G_SOURCE_REMOVE;
}

static gboolean on_long_press_timer(gpointer user_data) {
    TapGestureContext *ctx = user_data;
    ctx->_long_timer = 0;
    /* Fire only if the finger/button is still held */
    if (ctx->_pressed && ctx->on_long_press)
        ctx->on_long_press(ctx->widget, ctx->user_data);
    return G_SOURCE_REMOVE;
}

static void on_taps_pressed(GtkGestureClick *gesture, gint n_press, gdouble x, gdouble y,
                            gpointer user_data) {
    (void)gesture;
    (void)x;
    (void)y;
    TapGestureContext *ctx = user_data;

    /* Always cancel a stale long-press timer first */
    if (ctx->_long_timer) {
        g_source_remove(ctx->_long_timer);
        ctx->_long_timer = 0;
    }

    if (n_press == 1) {
        ctx->_pressed = TRUE;

        /* ---- Long press ---- */
        if (ctx->on_long_press)
            ctx->_long_timer = g_timeout_add(ctx->long_press_ms, on_long_press_timer, ctx);

        /* ---- Single tap ---- */
        if (ctx->on_tap) {
            if (ctx->on_double_tap) {
                /*
                 * Another click might be on its way: delay the single-tap
                 * callback by the system double-click window so the timer can
                 * be cancelled if n_press == 2 arrives first.
                 */
                if (ctx->_single_timer) {
                    g_source_remove(ctx->_single_timer);
                    ctx->_single_timer = 0;
                }
                guint dbl_ms = 400;
                GdkDisplay *dpy = gtk_widget_get_display(ctx->widget);
                g_object_get(gtk_settings_get_for_display(dpy), "gtk-double-click-time", &dbl_ms,
                             NULL);
                /* Add a small margin so the n_press==2 signal arrives first */
                ctx->_single_timer = g_timeout_add(dbl_ms + 30, on_single_tap_timer, ctx);
            } else {
                /* No double-tap registered → fire immediately, zero latency */
                ctx->on_tap(ctx->widget, ctx->user_data);
            }
        }

    } else if (n_press == 2) {
        /* Second click arrived in time → suppress the delayed single-tap */
        if (ctx->_single_timer) {
            g_source_remove(ctx->_single_timer);
            ctx->_single_timer = 0;
        }
        if (ctx->on_double_tap)
            ctx->on_double_tap(ctx->widget, ctx->user_data);
    }
}

static void on_taps_released(GtkGestureClick *gesture, gint n_press, gdouble x, gdouble y,
                             gpointer user_data) {
    (void)gesture;
    (void)n_press;
    (void)x;
    (void)y;
    TapGestureContext *ctx = user_data;
    ctx->_pressed = FALSE;
    /* Finger lifted before long-press timeout → cancel it */
    if (ctx->_long_timer) {
        g_source_remove(ctx->_long_timer);
        ctx->_long_timer = 0;
    }
    /* Trigger release callback if set */
    if (ctx->on_release) {
        ctx->on_release(ctx->widget, ctx->user_data);
    }
}

TapGestureContext *gesture_add_taps(GtkWidget *widget, GestureCallback on_tap,
                                    GestureCallback on_double_tap, GestureCallback on_long_press,
                                    guint long_press_ms, gpointer user_data) {
    TapGestureContext *ctx = g_new0(TapGestureContext, 1);
    ctx->widget = widget;
    ctx->on_tap = on_tap;
    ctx->on_double_tap = on_double_tap;
    ctx->on_long_press = on_long_press;
    ctx->user_data = user_data;
    ctx->long_press_ms = (long_press_ms > 0) ? long_press_ms : 500;

    GtkGesture *click = gtk_gesture_click_new();
    /* Non-exclusive so it coexists with swipe / pan on the same widget */
    gtk_gesture_single_set_exclusive(GTK_GESTURE_SINGLE(click), FALSE);
    gtk_widget_add_controller(widget, GTK_EVENT_CONTROLLER(click));

    g_signal_connect(click, "pressed", G_CALLBACK(on_taps_pressed), ctx);
    g_signal_connect(click, "released", G_CALLBACK(on_taps_released), ctx);

    g_object_set_data(G_OBJECT(widget), "taps-context", ctx);
    g_object_set_data(G_OBJECT(widget), "taps-gesture", click);
    return ctx;
}

void tap_gesture_free(TapGestureContext *ctx) {
    if (!ctx)
        return;
    if (ctx->_single_timer)
        g_source_remove(ctx->_single_timer);
    if (ctx->_long_timer)
        g_source_remove(ctx->_long_timer);
    g_free(ctx);
}

void tap_gesture_set_release_callback(TapGestureContext *ctx, GestureCallback on_release) {
    if (ctx) {
        ctx->on_release = on_release;
    }
}

/* ================================================================
 * SWIPE  (four directions share one GtkGestureSwipe per widget)
 * ================================================================ */

typedef struct {
    GtkWidget *widget;
    GestureContext *left_ctx;
    GestureContext *right_ctx;
    GestureContext *up_ctx;
    GestureContext *down_ctx;
    gdouble velocity_threshold;
} SwipeContextSet;

static void on_swipe(GtkGestureSwipe *gesture, gdouble vx, gdouble vy, gpointer user_data) {
    (void)gesture;
    SwipeContextSet *set = user_data;
    if (!set)
        return;

    gdouble thr = set->velocity_threshold;
    gdouble abs_x = fabs(vx);
    gdouble abs_y = fabs(vy);

    if (abs_x >= abs_y) { /* horizontal dominant */
        if (vx < -thr && set->left_ctx && set->left_ctx->callback)
            set->left_ctx->callback(set->widget, set->left_ctx->user_data);
        else if (vx > thr && set->right_ctx && set->right_ctx->callback)
            set->right_ctx->callback(set->widget, set->right_ctx->user_data);
    } else { /* vertical dominant */
        if (vy < -thr && set->up_ctx && set->up_ctx->callback)
            set->up_ctx->callback(set->widget, set->up_ctx->user_data);
        else if (vy > thr && set->down_ctx && set->down_ctx->callback)
            set->down_ctx->callback(set->widget, set->down_ctx->user_data);
    }
}

static SwipeContextSet *ensure_swipe_set(GtkWidget *widget, gdouble velocity_threshold) {
    SwipeContextSet *set = g_object_get_data(G_OBJECT(widget), "swipe-set");
    if (!set) {
        set = g_new0(SwipeContextSet, 1);
        set->widget = widget;
        set->velocity_threshold = velocity_threshold;

        GtkGesture *g = gtk_gesture_swipe_new();
        gtk_event_controller_set_propagation_phase(GTK_EVENT_CONTROLLER(g), GTK_PHASE_CAPTURE);
        gtk_widget_add_controller(widget, GTK_EVENT_CONTROLLER(g));
        g_signal_connect(g, "swipe", G_CALLBACK(on_swipe), set);

        g_object_set_data(G_OBJECT(widget), "swipe-set", set);
        g_object_set_data(G_OBJECT(widget), "swipe-gesture", g);
    }
    return set;
}

#define MAKE_SWIPE_FN(dir, field, key)                                                             \
    GestureContext *gesture_add_swipe_##dir(GtkWidget *widget, gdouble velocity_threshold,         \
                                            GestureCallback callback, gpointer user_data) {        \
        GestureContext *ctx = g_new0(GestureContext, 1);                                           \
        ctx->widget = widget;                                                                      \
        ctx->callback = callback;                                                                  \
        ctx->user_data = user_data;                                                                \
        SwipeContextSet *set = ensure_swipe_set(widget, velocity_threshold);                       \
        set->field = ctx;                                                                          \
        g_object_set_data(G_OBJECT(widget), key, ctx);                                             \
        return ctx;                                                                                \
    }

MAKE_SWIPE_FN(left, left_ctx, "swipe-left-context")
MAKE_SWIPE_FN(right, right_ctx, "swipe-right-context")
MAKE_SWIPE_FN(up, up_ctx, "swipe-up-context")
MAKE_SWIPE_FN(down, down_ctx, "swipe-down-context")

/* ================================================================
 * PAN
 * ================================================================ */

typedef struct {
    PanCallback callback;
    GestureContext *base_ctx;
} PanData;

static void on_pan_pan(GtkGesturePan *gesture, GtkPanDirection direction, gdouble offset,
                       gpointer user_data) {
    (void)gesture;
    PanData *d = user_data;
    if (d && d->callback && d->base_ctx)
        d->callback(d->base_ctx->widget, direction, offset, d->base_ctx->user_data);
}

GestureContext *gesture_add_pan(GtkWidget *widget, GtkOrientation orientation, PanCallback callback,
                                gpointer user_data) {
    GestureContext *ctx = g_new0(GestureContext, 1);
    ctx->widget = widget;
    ctx->user_data = user_data;

    PanData *d = g_new0(PanData, 1);
    d->callback = callback;
    d->base_ctx = ctx;

    GtkGesture *g = gtk_gesture_pan_new(orientation);
    gtk_event_controller_set_propagation_phase(GTK_EVENT_CONTROLLER(g), GTK_PHASE_CAPTURE);
    gtk_widget_add_controller(widget, GTK_EVENT_CONTROLLER(g));
    g_signal_connect(g, "pan", G_CALLBACK(on_pan_pan), d);

    g_object_set_data(G_OBJECT(widget), "pan-gesture", g);
    g_object_set_data(G_OBJECT(widget), "pan-context", ctx);
    g_object_set_data(G_OBJECT(widget), "pan-data", d);
    return ctx;
}

/* ================================================================
 * PINCH / ZOOM
 *
 * GTK4 GtkGestureZoom::scale-changed delivers a CUMULATIVE scale
 * factor from the gesture start, not an incremental one.  We track
 * the previous value and emit the true per-event delta so callers
 * can accumulate zoom with a simple multiplication.
 * ================================================================ */

typedef struct {
    PinchCallback callback;
    GestureContext *base_ctx;
    gdouble last_scale; /* cumulative scale at previous event */
} PinchData;

static void on_pinch_begin(GtkGesture *gesture, GdkEventSequence *seq, gpointer user_data) {
    (void)gesture;
    (void)seq;
    ((PinchData *)user_data)->last_scale = 1.0;
}

static void on_pinch_scale_changed(GtkGestureZoom *gesture, gdouble scale, gpointer user_data) {
    (void)gesture;
    PinchData *d = user_data;

    /* Compute true multiplicative increment */
    gdouble delta = (d->last_scale > 1e-9) ? scale / d->last_scale : 1.0;
    d->last_scale = scale;

    if (d->callback && d->base_ctx)
        d->callback(d->base_ctx->widget, delta, d->base_ctx->user_data);
}

GestureContext *gesture_add_pinch(GtkWidget *widget, PinchCallback callback, gpointer user_data) {
    GestureContext *ctx = g_new0(GestureContext, 1);
    ctx->widget = widget;
    ctx->user_data = user_data;

    PinchData *d = g_new0(PinchData, 1);
    d->callback = callback;
    d->base_ctx = ctx;
    d->last_scale = 1.0;

    GtkGesture *g = gtk_gesture_zoom_new();
    gtk_widget_add_controller(widget, GTK_EVENT_CONTROLLER(g));
    g_signal_connect(g, "begin", G_CALLBACK(on_pinch_begin), d);
    g_signal_connect(g, "scale-changed", G_CALLBACK(on_pinch_scale_changed), d);

    g_object_set_data(G_OBJECT(widget), "pinch-gesture", g);
    g_object_set_data(G_OBJECT(widget), "pinch-context", ctx);
    g_object_set_data(G_OBJECT(widget), "pinch-data", d);
    return ctx;
}

/* ================================================================
 * ROTATION
 *
 * GTK4 GtkGestureRotate::angle-changed(angle, angle_delta) – both
 * values are in RADIANS.  angle_delta is already the per-event
 * increment; we forward it directly.
 * ================================================================ */

/*
 * Minimum cumulative rotation (from gesture start) before we engage.
 * Below this the user is probably just resting fingers or doing a
 * pan/swipe — any rotation signal is noise.  ≈ 8°.
 */
#define ROTATION_ENGAGE_RAD 0.14

/*
 * Per-event dead zone: deltas smaller than this are discarded even
 * after engagement.  Absorbs trackpad micro-jitter.  ≈ 0.6°.
 */
#define ROTATION_DEAD_RAD 0.10

typedef struct {
    RotationCallback callback;
    GestureContext *base_ctx;
    gboolean engaged;  /* TRUE once the engage threshold is met */
    gdouble ref_angle; /* cumulative angle at the last emission  */
} RotationData;

/* Reset state at the start of every new gesture sequence */
static void on_rotation_begin(GtkGesture *gesture, GdkEventSequence *seq, gpointer user_data) {
    (void)gesture;
    (void)seq;
    RotationData *d = user_data;
    d->engaged = FALSE;
    d->ref_angle = 0.0;
}

static void on_rotation_angle_changed(GtkGestureRotate *gesture,
                                      gdouble angle,       /* cumulative from start */
                                      gdouble angle_delta, /* GTK incremental (unused) */
                                      gpointer user_data) {
    (void)gesture;
    (void)angle_delta;
    RotationData *d = user_data;

    /*
     * Phase 1 – waiting for intent.
     * Eat all events until the cumulative angle clearly exceeds the
     * engage threshold.  This prevents single-finger drags and
     * accidental touches from spinning the canvas.
     */
    if (!d->engaged) {
        if (fabs(angle) < ROTATION_ENGAGE_RAD)
            return;
        /* Threshold crossed: start tracking from the current position */
        d->engaged = TRUE;
        d->ref_angle = angle;
        return; /* skip this first frame so next delta starts clean */
    }

    /*
     * Phase 2 – engaged.
     * Compute our own delta relative to the last emitted reference.
     * Using the cumulative `angle` (rather than GTK's `angle_delta`)
     * lets us control exactly when the reference is updated, keeping
     * the dead zone consistent regardless of event rate.
     */
    gdouble delta = angle - d->ref_angle;

    if (fabs(delta) < ROTATION_DEAD_RAD)
        return; /* per-event dead zone */

    d->ref_angle = angle; /* commit the new reference */

    if (d->callback && d->base_ctx)
        d->callback(d->base_ctx->widget, delta, d->base_ctx->user_data);
}

GestureContext *gesture_add_rotation(GtkWidget *widget, RotationCallback callback,
                                     gpointer user_data) {
    GestureContext *ctx = g_new0(GestureContext, 1);
    ctx->widget = widget;
    ctx->user_data = user_data;

    RotationData *d = g_new0(RotationData, 1);
    d->callback = callback;
    d->base_ctx = ctx;
    d->engaged = FALSE;
    d->ref_angle = 0.0;

    GtkGesture *g = gtk_gesture_rotate_new();
    gtk_widget_add_controller(widget, GTK_EVENT_CONTROLLER(g));
    g_signal_connect(g, "begin", G_CALLBACK(on_rotation_begin), d);
    g_signal_connect(g, "angle-changed", G_CALLBACK(on_rotation_angle_changed), d);

    g_object_set_data(G_OBJECT(widget), "rotation-gesture", g);
    g_object_set_data(G_OBJECT(widget), "rotation-context", ctx);
    g_object_set_data(G_OBJECT(widget), "rotation-data", d);
    return ctx;
}

/* ================================================================
 * CLEANUP
 * ================================================================ */

void gesture_remove(GestureContext *ctx) {
    if (!ctx)
        return;
    if (ctx->timeout_id > 0)
        g_source_remove(ctx->timeout_id);
    g_free(ctx);
}

void gesture_remove_all(GtkWidget *widget) {
    if (!GTK_IS_WIDGET(widget))
        return;

    /* TapGestureContext */
    TapGestureContext *taps = g_object_get_data(G_OBJECT(widget), "taps-context");
    if (taps)
        tap_gesture_free(taps);

    /* GestureContext slots */
    static const char *ctx_keys[] = {
        "swipe-left-context", "swipe-right-context", "swipe-up-context", "swipe-down-context",
        "pan-context",        "pinch-context",       "rotation-context", NULL};
    for (int i = 0; ctx_keys[i]; i++) {
        GestureContext *c = g_object_get_data(G_OBJECT(widget), ctx_keys[i]);
        if (c)
            gesture_remove(c);
    }

    /* Auxiliary structs */
    PanData *pd = g_object_get_data(G_OBJECT(widget), "pan-data");
    if (pd)
        g_free(pd);
    PinchData *pi = g_object_get_data(G_OBJECT(widget), "pinch-data");
    if (pi)
        g_free(pi);
    RotationData *rd = g_object_get_data(G_OBJECT(widget), "rotation-data");
    if (rd)
        g_free(rd);
    SwipeContextSet *ss = g_object_get_data(G_OBJECT(widget), "swipe-set");
    if (ss)
        g_free(ss);
}