#ifndef ANIMATIONS_H
#define ANIMATIONS_H

#include <gtk/gtk.h>

/* ============================================================
 * Animation Types
 * ============================================================ */

typedef enum {
    ANIM_SHAKE,
    ANIM_PULSE,
    ANIM_BOUNCE,
    ANIM_SLIDE,
    ANIM_ROTATE,
    ANIM_SCALE,
    ANIM_GLOW,
    ANIM_RAINBOW
} AnimationType;

/* ============================================================
 * Animation Context & Callbacks
 * ============================================================ */

typedef struct {
    GtkWidget *widget;
    gdouble start_time;
    gdouble duration_ms;
    guint tick_id;
    gboolean is_active;
    gdouble intensity;   /* 0.0 to 1.0 */
    gdouble exponent;    /* curvature: 1.0=linear, 2.0=quadratic, etc */
    AnimationType type;  /* animation type */
    gpointer extra_data; /* type-specific data (angles, offsets, etc) */
} AnimationContext;

/* ============================================================
 * Shake/Tremolo Animation Functions
 * ============================================================ */

/**
 * Start a shake/tremolo animation on a widget
 * @param widget: Target widget to shake
 * @param duration_ms: Duration in milliseconds
 * @param intensity: Shake intensity 0.0-1.0 (controls pixel offset)
 * @param exponent: Animation curve (1.0=linear, 2.0=quadratic/expo)
 * @return: Animation context (don't free manually)
 */
AnimationContext *animation_shake_start(GtkWidget *widget, guint duration_ms, gdouble intensity,
                                        gdouble exponent);

/**
 * Stop a shake animation
 * @param ctx: Animation context
 */
void animation_shake_stop(AnimationContext *ctx);

/* ============================================================
 * Pulse/Fade Animation
 * ============================================================ */

/**
 * Start a pulsing fade animation
 * @param widget: Target widget
 * @param duration_ms: Duration in milliseconds
 * @return: Animation context
 */
AnimationContext *animation_pulse_start(GtkWidget *widget, guint duration_ms);

/**
 * Stop a pulse animation
 * @param ctx: Animation context
 */
void animation_pulse_stop(AnimationContext *ctx);

/* ============================================================
 * Bounce Animation (elastic effect)
 * ============================================================ */

/**
 * Start a bounce animation (scale up/down elastically)
 * @param widget: Target widget
 * @param duration_ms: Duration in milliseconds
 * @param intensity: Bounce height 0.0-1.0
 * @return: Animation context
 */
AnimationContext *animation_bounce_start(GtkWidget *widget, guint duration_ms, gdouble intensity);

/**
 * Stop bounce animation
 * @param ctx: Animation context
 */
void animation_bounce_stop(AnimationContext *ctx);

/* ============================================================
 * Slide Animation (movement)
 * ============================================================ */

typedef enum { SLIDE_LEFT, SLIDE_RIGHT, SLIDE_UP, SLIDE_DOWN } SlideDirection;

/**
 * Start a slide animation (widget slides in/out)
 * @param widget: Target widget
 * @param direction: Slide direction
 * @param distance_px: Max slide distance in pixels
 * @param duration_ms: Duration in milliseconds
 * @return: Animation context
 */
AnimationContext *animation_slide_start(GtkWidget *widget, SlideDirection direction,
                                        gint distance_px, guint duration_ms);

/**
 * Stop slide animation
 * @param ctx: Animation context
 */
void animation_slide_stop(AnimationContext *ctx);

/* ============================================================
 * Rotate Animation
 * ============================================================ */

/**
 * Start a rotation animation
 * @param widget: Target widget
 * @param duration_ms: Duration in milliseconds
 * @param angle_deg: Total rotation angle in degrees (360 = full spin)
 * @return: Animation context
 */
AnimationContext *animation_rotate_start(GtkWidget *widget, guint duration_ms, gdouble angle_deg);

/**
 * Stop rotation animation
 * @param ctx: Animation context
 */
void animation_rotate_stop(AnimationContext *ctx);

/* ============================================================
 * Scale Animation (zoom in/out)
 * ============================================================ */

/**
 * Start a scale/zoom animation
 * @param widget: Target widget
 * @param duration_ms: Duration in milliseconds
 * @param scale_factor: Target scale (2.0 = 2x size, 0.5 = half size)
 * @param easing_exp: Easing exponent (2.0 = smooth)
 * @return: Animation context
 */
AnimationContext *animation_scale_start(GtkWidget *widget, guint duration_ms, gdouble scale_factor,
                                        gdouble easing_exp);

/**
 * Stop scale animation
 * @param ctx: Animation context
 */
void animation_scale_stop(AnimationContext *ctx);

/* ============================================================
 * Glow Animation (luminous effect)
 * ============================================================ */

/**
 * Start a glow animation (shadow/luminescence effect)
 * @param widget: Target widget
 * @param duration_ms: Duration in milliseconds
 * @param intensity: Glow intensity 0.0-1.0
 * @return: Animation context
 */
AnimationContext *animation_glow_start(GtkWidget *widget, guint duration_ms, gdouble intensity);

/**
 * Stop glow animation
 * @param ctx: Animation context
 */
void animation_glow_stop(AnimationContext *ctx);

/* ============================================================
 * Rainbow/Color Cycle Animation
 * ============================================================ */

/**
 * Start a rainbow color cycling animation
 * @param widget: Target widget
 * @param duration_ms: Duration for one full cycle in milliseconds
 * @param cycles: Number of complete cycles (1.0 = one rotation through hues)
 * @return: Animation context
 */
AnimationContext *animation_rainbow_start(GtkWidget *widget, guint duration_ms, gdouble cycles);

/**
 * Stop rainbow animation
 * @param ctx: Animation context
 */
void animation_rainbow_stop(AnimationContext *ctx);

/* ============================================================
 * Generic Stop Function (for any animation type)
 * ============================================================ */

/**
 * Stop any active animation on a widget
 * @param ctx: Animation context of any type
 */
void animation_stop(AnimationContext *ctx);

#endif