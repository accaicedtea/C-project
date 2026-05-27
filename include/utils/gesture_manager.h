#ifndef GESTURE_MANAGER_H
#define GESTURE_MANAGER_H

#include <gtk/gtk.h>

/* ================================================================
 * Callback signatures
 * ================================================================ */
typedef void (*GestureCallback)(GtkWidget *widget, gpointer user_data);
typedef void (*PanCallback)(GtkWidget *widget, GtkPanDirection direction, gdouble offset,
                            gpointer user_data);
/** scale_delta: true multiplicative increment between consecutive events
 *  (always > 0; already converted from the cumulative GTK value). */
typedef void (*PinchCallback)(GtkWidget *widget, gdouble scale_delta, gpointer user_data);
/** angle_delta_rad: incremental rotation in radians (CCW positive). */
typedef void (*RotationCallback)(GtkWidget *widget, gdouble angle_delta_rad, gpointer user_data);

/* ================================================================
 * GestureContext – lightweight base for swipe / pan / pinch / rotation
 * ================================================================ */
typedef struct {
    GtkWidget *widget;
    GestureCallback callback;
    gpointer user_data;
    /* internal – used by the legacy single-gesture helpers */
    guint timeout_id;
    guint duration_ms;
    gboolean is_long_press;
    guint32 press_time;
} GestureContext;

/* ================================================================
 * TapGestureContext – unified tap / double-tap / long-press
 *
 * A single controller handles all three actions on the same widget.
 * Any callback may be NULL (the corresponding action is ignored).
 *
 * Disambiguation rule:
 *   • If both on_tap and on_double_tap are set, single-tap is fired
 *     after the system double-click timeout so it can be suppressed
 *     when a second click arrives in time.
 *   • If only on_tap is set it fires immediately (no latency).
 *   • Long-press is independent of click-count detection.
 * ================================================================ */
typedef struct _TapGestureContext TapGestureContext;
struct _TapGestureContext {
    GtkWidget *widget;
    GestureCallback on_tap;        /* single tap  – may be NULL */
    GestureCallback on_double_tap; /* double tap  – may be NULL */
    GestureCallback on_long_press; /* long press  – may be NULL */
    GestureCallback on_release;    /* finger lifted – may be NULL */
    gpointer user_data;
    guint long_press_ms; /* hold duration, default 500 */
    /* ---- private, do not touch ---- */
    guint _single_timer;
    guint _long_timer;
    gboolean _pressed;
};

/* ================================================================
 * TAP API  (replaces the old gesture_add_tap / gesture_add_double_tap /
 *           gesture_add_long_press trio)
 * ================================================================ */

/**
 * gesture_add_taps:
 * Attach any combination of single-tap, double-tap and long-press to
 * @widget using a single shared GtkGestureClick controller.
 *
 * @on_tap, @on_double_tap, @on_long_press  – any may be NULL.
 * @long_press_ms – hold duration in ms; pass 0 for the default (500 ms).
 *
 * Returns a TapGestureContext that the caller owns; free with
 * tap_gesture_free() when no longer needed.
 */
TapGestureContext *gesture_add_taps(GtkWidget *widget, GestureCallback on_tap,
                                    GestureCallback on_double_tap, GestureCallback on_long_press,
                                    guint long_press_ms, gpointer user_data);

/** Set release callback on existing tap gesture context */
void tap_gesture_set_release_callback(TapGestureContext *ctx, GestureCallback on_release);

/** Cancel pending timers and free the context. */
void tap_gesture_free(TapGestureContext *ctx);

/* ================================================================
 * SWIPE API
 * All four directions share a single GtkGestureSwipe controller per
 * widget; call any subset freely.
 * @velocity_threshold – minimum px/s to recognise the gesture (e.g. 80).
 * ================================================================ */
GestureContext *gesture_add_swipe_left(GtkWidget *widget, gdouble velocity_threshold,
                                       GestureCallback callback, gpointer user_data);
GestureContext *gesture_add_swipe_right(GtkWidget *widget, gdouble velocity_threshold,
                                        GestureCallback callback, gpointer user_data);
GestureContext *gesture_add_swipe_up(GtkWidget *widget, gdouble velocity_threshold,
                                     GestureCallback callback, gpointer user_data);
GestureContext *gesture_add_swipe_down(GtkWidget *widget, gdouble velocity_threshold,
                                       GestureCallback callback, gpointer user_data);

/* ================================================================
 * PAN / PINCH / ROTATION API
 * ================================================================ */
GestureContext *gesture_add_pan(GtkWidget *widget, GtkOrientation orientation, PanCallback callback,
                                gpointer user_data);

GestureContext *gesture_add_pinch(GtkWidget *widget, PinchCallback callback, gpointer user_data);

GestureContext *gesture_add_rotation(GtkWidget *widget, RotationCallback callback,
                                     gpointer user_data);

/* ================================================================
 * CLEANUP
 * ================================================================ */
void gesture_remove(GestureContext *context);
void gesture_remove_all(GtkWidget *widget);

#endif