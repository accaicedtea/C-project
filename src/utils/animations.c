#include "utils/animations.h"
#include <glib.h>
#include <math.h>

/* ============================================================
 * Global animation context tracking
 * ============================================================ */

static GHashTable *active_animations = NULL;

static void ensure_animation_table(void) {
    if (!active_animations) {
        active_animations = g_hash_table_new(g_direct_hash, g_direct_equal);
    }
}

/* ============================================================
 * Easing functions - Smooth and natural motion curves
 * ============================================================ */

static gdouble ease_out_cubic(gdouble t) {
    gdouble f = t - 1.0;
    return f * f * f + 1.0;
}

static gdouble ease_out_elastic(gdouble t) {
    if (t == 0.0)
        return 0.0;
    if (t == 1.0)
        return 1.0;
    gdouble c4 = (2.0 * G_PI) / 3.0;
    return pow(2.0, -10.0 * t) * sin((t * 10.0 - 0.75) * c4) + 1.0;
}

static gdouble ease_in_out_cubic(gdouble t) {
    return (t < 0.5) ? 4.0 * t * t * t : 1.0 - pow(-2.0 * t + 2.0, 3.0) / 2.0;
}

/* ============================================================
 * Shake/Tremolo Animation Implementation
 * ============================================================ */

static gboolean on_shake_tick(GtkWidget *widget, GdkFrameClock *frame_clock, gpointer user_data) {
    AnimationContext *ctx = (AnimationContext *)user_data;

    if (!ctx || !ctx->is_active || !GTK_IS_WIDGET(widget)) {
        return G_SOURCE_REMOVE;
    }

    gint64 frame_time = gdk_frame_clock_get_frame_time(frame_clock);
    gdouble elapsed_ms = (frame_time - ctx->start_time) / 1000.0;

    if (elapsed_ms >= ctx->duration_ms) {
        ctx->is_active = FALSE;
        gtk_widget_set_margin_start(widget, 0);
        gtk_widget_set_margin_top(widget, 0);
        g_hash_table_remove(active_animations, widget);
        return G_SOURCE_REMOVE;
    }

    gdouble progress = elapsed_ms / ctx->duration_ms;
    /* Apply exponent to decay curve: higher exponent = longer shake persistence */
    gdouble decay = pow(1.0 - progress, ctx->exponent);
    /* Amplitude: 0.8 intensity × decay × 25px base = up to 20px shake */
    gdouble shake_amplitude = ctx->intensity * decay * 25.0;

    /* Oscillate at 20Hz base frequency */
    gdouble angle = elapsed_ms * 20.0 * G_PI / 500.0;
    gint shake_x = (gint)(shake_amplitude * sin(angle));
    gint shake_y = (gint)(shake_amplitude * cos(angle * 1.3));

    gtk_widget_set_margin_start(widget, shake_x);
    gtk_widget_set_margin_top(widget, shake_y);

    return G_SOURCE_CONTINUE;
}

AnimationContext *animation_shake_start(GtkWidget *widget, guint duration_ms, gdouble intensity,
                                        gdouble exponent) {
    if (!GTK_IS_WIDGET(widget))
        return NULL;

    ensure_animation_table();

    AnimationContext *existing = g_hash_table_lookup(active_animations, widget);
    if (existing) {
        animation_shake_stop(existing);
    }

    AnimationContext *ctx = g_new0(AnimationContext, 1);
    ctx->widget = widget;
    ctx->duration_ms = duration_ms;
    ctx->intensity = CLAMP(intensity, 0.0, 1.0);
    ctx->exponent = (exponent > 0.0) ? exponent : 2.0;
    ctx->is_active = TRUE;
    ctx->type = ANIM_SHAKE;

    GdkFrameClock *clock = gtk_widget_get_frame_clock(widget);
    if (clock) {
        ctx->start_time = gdk_frame_clock_get_frame_time(clock);
    } else {
        ctx->start_time = g_get_monotonic_time();
    }

    ctx->tick_id = gtk_widget_add_tick_callback(widget, (GtkTickCallback)on_shake_tick, ctx, NULL);

    g_hash_table_insert(active_animations, widget, ctx);

    return ctx;
}

void animation_shake_stop(AnimationContext *ctx) {
    if (!ctx)
        return;

    if (ctx->is_active && GTK_IS_WIDGET(ctx->widget)) {
        gtk_widget_remove_tick_callback(ctx->widget, ctx->tick_id);
        gtk_widget_set_margin_start(ctx->widget, 0);
        gtk_widget_set_margin_top(ctx->widget, 0);
        ctx->is_active = FALSE;
    }

    ensure_animation_table();
    if (ctx->widget) {
        g_hash_table_remove(active_animations, ctx->widget);
    }

    g_free(ctx);
}

/* ============================================================
 * Pulse/Fade Animation
 * ============================================================ */

static gboolean on_pulse_tick(GtkWidget *widget, GdkFrameClock *frame_clock, gpointer user_data) {
    AnimationContext *ctx = (AnimationContext *)user_data;

    if (!ctx || !ctx->is_active || !GTK_IS_WIDGET(widget)) {
        return G_SOURCE_REMOVE;
    }

    gint64 frame_time = gdk_frame_clock_get_frame_time(frame_clock);
    gdouble elapsed_ms = (frame_time - ctx->start_time) / 1000.0;

    if (elapsed_ms >= ctx->duration_ms) {
        ctx->is_active = FALSE;
        gtk_widget_set_opacity(widget, 1.0);
        g_hash_table_remove(active_animations, widget);
        return G_SOURCE_REMOVE;
    }

    gdouble progress = elapsed_ms / ctx->duration_ms;
    gdouble smooth = ease_in_out_cubic(progress);
    gdouble opacity = 0.75 + 0.25 * sin(smooth * G_PI * 2.0);

    gtk_widget_set_opacity(widget, opacity);

    return G_SOURCE_CONTINUE;
}

AnimationContext *animation_pulse_start(GtkWidget *widget, guint duration_ms) {
    if (!GTK_IS_WIDGET(widget))
        return NULL;

    ensure_animation_table();

    AnimationContext *existing = g_hash_table_lookup(active_animations, widget);
    if (existing) {
        animation_pulse_stop(existing);
    }

    AnimationContext *ctx = g_new0(AnimationContext, 1);
    ctx->widget = widget;
    ctx->duration_ms = duration_ms;
    ctx->intensity = 1.0;
    ctx->exponent = 1.0;
    ctx->is_active = TRUE;
    ctx->type = ANIM_PULSE;

    GdkFrameClock *clock = gtk_widget_get_frame_clock(widget);
    if (clock) {
        ctx->start_time = gdk_frame_clock_get_frame_time(clock);
    } else {
        ctx->start_time = g_get_monotonic_time();
    }

    ctx->tick_id = gtk_widget_add_tick_callback(widget, (GtkTickCallback)on_pulse_tick, ctx, NULL);

    g_hash_table_insert(active_animations, widget, ctx);

    return ctx;
}

void animation_pulse_stop(AnimationContext *ctx) {
    if (!ctx)
        return;

    if (ctx->is_active && GTK_IS_WIDGET(ctx->widget)) {
        gtk_widget_remove_tick_callback(ctx->widget, ctx->tick_id);
        gtk_widget_set_opacity(ctx->widget, 1.0);
        ctx->is_active = FALSE;
    }

    ensure_animation_table();
    if (ctx->widget) {
        g_hash_table_remove(active_animations, ctx->widget);
    }

    g_free(ctx);
}

/* ============================================================
 * Bounce Animation
 * ============================================================ */

static gboolean on_bounce_tick(GtkWidget *widget, GdkFrameClock *frame_clock, gpointer user_data) {
    AnimationContext *ctx = (AnimationContext *)user_data;

    if (!ctx || !ctx->is_active || !GTK_IS_WIDGET(widget)) {
        return G_SOURCE_REMOVE;
    }

    gint64 frame_time = gdk_frame_clock_get_frame_time(frame_clock);
    gdouble elapsed_ms = (frame_time - ctx->start_time) / 1000.0;

    if (elapsed_ms >= ctx->duration_ms) {
        ctx->is_active = FALSE;
        gtk_widget_set_opacity(widget, 1.0);
        g_hash_table_remove(active_animations, widget);
        return G_SOURCE_REMOVE;
    }

    gdouble progress = elapsed_ms / ctx->duration_ms;
    gdouble bounce_factor = ease_out_elastic(progress);
    gdouble opacity = 0.7 + (0.3 * bounce_factor);

    gtk_widget_set_opacity(widget, opacity);

    return G_SOURCE_CONTINUE;
}

AnimationContext *animation_bounce_start(GtkWidget *widget, guint duration_ms, gdouble intensity) {
    if (!GTK_IS_WIDGET(widget))
        return NULL;

    ensure_animation_table();

    AnimationContext *existing = g_hash_table_lookup(active_animations, widget);
    if (existing) {
        animation_bounce_stop(existing);
    }

    AnimationContext *ctx = g_new0(AnimationContext, 1);
    ctx->widget = widget;
    ctx->duration_ms = duration_ms;
    ctx->intensity = CLAMP(intensity, 0.0, 1.0);
    ctx->exponent = 1.0;
    ctx->is_active = TRUE;
    ctx->type = ANIM_BOUNCE;

    GdkFrameClock *clock = gtk_widget_get_frame_clock(widget);
    if (clock) {
        ctx->start_time = gdk_frame_clock_get_frame_time(clock);
    } else {
        ctx->start_time = g_get_monotonic_time();
    }

    ctx->tick_id = gtk_widget_add_tick_callback(widget, (GtkTickCallback)on_bounce_tick, ctx, NULL);

    g_hash_table_insert(active_animations, widget, ctx);

    return ctx;
}

void animation_bounce_stop(AnimationContext *ctx) {
    if (!ctx)
        return;

    if (ctx->is_active && GTK_IS_WIDGET(ctx->widget)) {
        gtk_widget_remove_tick_callback(ctx->widget, ctx->tick_id);
        gtk_widget_set_opacity(ctx->widget, 1.0);
        ctx->is_active = FALSE;
    }

    ensure_animation_table();
    if (ctx->widget) {
        g_hash_table_remove(active_animations, ctx->widget);
    }

    g_free(ctx);
}

/* ============================================================
 * Slide Animation
 * ============================================================ */

typedef struct {
    SlideDirection direction;
    gint distance_px;
    gint original_margin_start;
    gint original_margin_end;
    gint original_margin_top;
    gint original_margin_bottom;
} SlideData;

static gboolean on_slide_tick(GtkWidget *widget, GdkFrameClock *frame_clock, gpointer user_data) {
    AnimationContext *ctx = (AnimationContext *)user_data;
    SlideData *data = (SlideData *)ctx->extra_data;

    if (!ctx || !ctx->is_active || !GTK_IS_WIDGET(widget) || !data) {
        return G_SOURCE_REMOVE;
    }

    gint64 frame_time = gdk_frame_clock_get_frame_time(frame_clock);
    gdouble elapsed_ms = (frame_time - ctx->start_time) / 1000.0;

    if (elapsed_ms >= ctx->duration_ms) {
        ctx->is_active = FALSE;
        gtk_widget_set_margin_start(widget, data->original_margin_start);
        gtk_widget_set_margin_end(widget, data->original_margin_end);
        gtk_widget_set_margin_top(widget, data->original_margin_top);
        gtk_widget_set_margin_bottom(widget, data->original_margin_bottom);
        g_hash_table_remove(active_animations, widget);
        return G_SOURCE_REMOVE;
    }

    gdouble progress = ease_out_cubic(elapsed_ms / ctx->duration_ms);
    gint offset = (gint)(data->distance_px * progress);

    if (data->direction == SLIDE_LEFT) {
        gtk_widget_set_margin_start(widget, data->original_margin_start - offset);
    } else if (data->direction == SLIDE_RIGHT) {
        gtk_widget_set_margin_start(widget, data->original_margin_start + offset);
    } else if (data->direction == SLIDE_UP) {
        gtk_widget_set_margin_top(widget, data->original_margin_top - offset);
    } else if (data->direction == SLIDE_DOWN) {
        gtk_widget_set_margin_top(widget, data->original_margin_top + offset);
    }

    return G_SOURCE_CONTINUE;
}

AnimationContext *animation_slide_start(GtkWidget *widget, SlideDirection direction,
                                        gint distance_px, guint duration_ms) {
    if (!GTK_IS_WIDGET(widget))
        return NULL;

    ensure_animation_table();

    AnimationContext *existing = g_hash_table_lookup(active_animations, widget);
    if (existing) {
        animation_slide_stop(existing);
    }

    AnimationContext *ctx = g_new0(AnimationContext, 1);
    SlideData *data = g_new0(SlideData, 1);

    data->direction = direction;
    data->distance_px = distance_px;
    data->original_margin_start = gtk_widget_get_margin_start(widget);
    data->original_margin_end = gtk_widget_get_margin_end(widget);
    data->original_margin_top = gtk_widget_get_margin_top(widget);
    data->original_margin_bottom = gtk_widget_get_margin_bottom(widget);

    ctx->widget = widget;
    ctx->duration_ms = duration_ms;
    ctx->intensity = 1.0;
    ctx->exponent = 1.0;
    ctx->is_active = TRUE;
    ctx->type = ANIM_SLIDE;
    ctx->extra_data = data;

    GdkFrameClock *clock = gtk_widget_get_frame_clock(widget);
    if (clock) {
        ctx->start_time = gdk_frame_clock_get_frame_time(clock);
    } else {
        ctx->start_time = g_get_monotonic_time();
    }

    ctx->tick_id = gtk_widget_add_tick_callback(widget, (GtkTickCallback)on_slide_tick, ctx, NULL);

    g_hash_table_insert(active_animations, widget, ctx);

    return ctx;
}

void animation_slide_stop(AnimationContext *ctx) {
    if (!ctx)
        return;

    if (ctx->is_active && GTK_IS_WIDGET(ctx->widget)) {
        gtk_widget_remove_tick_callback(ctx->widget, ctx->tick_id);
        SlideData *data = (SlideData *)ctx->extra_data;
        if (data) {
            gtk_widget_set_margin_start(ctx->widget, data->original_margin_start);
            gtk_widget_set_margin_end(ctx->widget, data->original_margin_end);
            gtk_widget_set_margin_top(ctx->widget, data->original_margin_top);
            gtk_widget_set_margin_bottom(ctx->widget, data->original_margin_bottom);
        }
        ctx->is_active = FALSE;
    }

    ensure_animation_table();
    if (ctx->widget) {
        g_hash_table_remove(active_animations, ctx->widget);
    }

    if (ctx->extra_data)
        g_free(ctx->extra_data);
    g_free(ctx);
}

/* ============================================================
 * Rotate Animation
 * ============================================================ */

static gboolean on_rotate_tick(GtkWidget *widget, GdkFrameClock *frame_clock, gpointer user_data) {
    AnimationContext *ctx = (AnimationContext *)user_data;

    if (!ctx || !ctx->is_active || !GTK_IS_WIDGET(widget)) {
        return G_SOURCE_REMOVE;
    }

    gint64 frame_time = gdk_frame_clock_get_frame_time(frame_clock);
    gdouble elapsed_ms = (frame_time - ctx->start_time) / 1000.0;

    if (elapsed_ms >= ctx->duration_ms) {
        ctx->is_active = FALSE;
        gtk_widget_set_opacity(widget, 1.0);
        g_hash_table_remove(active_animations, widget);
        return G_SOURCE_REMOVE;
    }

    gdouble progress = elapsed_ms / ctx->duration_ms;
    gdouble smooth = ease_out_cubic(progress);
    gdouble opacity = 0.8 + (0.2 * sin(smooth * G_PI));

    gtk_widget_set_opacity(widget, opacity);

    return G_SOURCE_CONTINUE;
}

AnimationContext *animation_rotate_start(GtkWidget *widget, guint duration_ms, gdouble angle_deg) {
    if (!GTK_IS_WIDGET(widget))
        return NULL;

    ensure_animation_table();

    AnimationContext *existing = g_hash_table_lookup(active_animations, widget);
    if (existing) {
        animation_rotate_stop(existing);
    }

    AnimationContext *ctx = g_new0(AnimationContext, 1);

    ctx->widget = widget;
    ctx->duration_ms = duration_ms;
    ctx->intensity = angle_deg;
    ctx->exponent = 1.0;
    ctx->is_active = TRUE;
    ctx->type = ANIM_ROTATE;
    ctx->extra_data = NULL;

    GdkFrameClock *clock = gtk_widget_get_frame_clock(widget);
    if (clock) {
        ctx->start_time = gdk_frame_clock_get_frame_time(clock);
    } else {
        ctx->start_time = g_get_monotonic_time();
    }

    ctx->tick_id = gtk_widget_add_tick_callback(widget, (GtkTickCallback)on_rotate_tick, ctx, NULL);

    g_hash_table_insert(active_animations, widget, ctx);

    return ctx;
}

void animation_rotate_stop(AnimationContext *ctx) {
    if (!ctx)
        return;

    if (ctx->is_active && GTK_IS_WIDGET(ctx->widget)) {
        gtk_widget_remove_tick_callback(ctx->widget, ctx->tick_id);
        gtk_widget_set_opacity(ctx->widget, 1.0);
        ctx->is_active = FALSE;
    }

    ensure_animation_table();
    if (ctx->widget) {
        g_hash_table_remove(active_animations, ctx->widget);
    }

    g_free(ctx);
}

/* ============================================================
 * Scale Animation
 * ============================================================ */

static gboolean on_scale_tick(GtkWidget *widget, GdkFrameClock *frame_clock, gpointer user_data) {
    AnimationContext *ctx = (AnimationContext *)user_data;

    if (!ctx || !ctx->is_active || !GTK_IS_WIDGET(widget)) {
        return G_SOURCE_REMOVE;
    }

    gint64 frame_time = gdk_frame_clock_get_frame_time(frame_clock);
    gdouble elapsed_ms = (frame_time - ctx->start_time) / 1000.0;

    if (elapsed_ms >= ctx->duration_ms) {
        ctx->is_active = FALSE;
        gtk_widget_set_opacity(widget, 1.0);
        g_hash_table_remove(active_animations, widget);
        return G_SOURCE_REMOVE;
    }

    gdouble progress = elapsed_ms / ctx->duration_ms;
    gdouble smooth = ease_in_out_cubic(progress);
    gdouble opacity = 0.7 + (0.3 * smooth);

    gtk_widget_set_opacity(widget, opacity);

    return G_SOURCE_CONTINUE;
}

AnimationContext *animation_scale_start(GtkWidget *widget, guint duration_ms, gdouble scale_factor,
                                        gdouble easing_exp) {
    if (!GTK_IS_WIDGET(widget))
        return NULL;

    ensure_animation_table();

    AnimationContext *existing = g_hash_table_lookup(active_animations, widget);
    if (existing) {
        animation_scale_stop(existing);
    }

    AnimationContext *ctx = g_new0(AnimationContext, 1);

    ctx->widget = widget;
    ctx->duration_ms = duration_ms;
    ctx->intensity = scale_factor;
    ctx->exponent = (easing_exp > 0.0) ? easing_exp : 2.0;
    ctx->is_active = TRUE;
    ctx->type = ANIM_SCALE;
    ctx->extra_data = NULL;

    GdkFrameClock *clock = gtk_widget_get_frame_clock(widget);
    if (clock) {
        ctx->start_time = gdk_frame_clock_get_frame_time(clock);
    } else {
        ctx->start_time = g_get_monotonic_time();
    }

    ctx->tick_id = gtk_widget_add_tick_callback(widget, (GtkTickCallback)on_scale_tick, ctx, NULL);

    g_hash_table_insert(active_animations, widget, ctx);

    return ctx;
}

void animation_scale_stop(AnimationContext *ctx) {
    if (!ctx)
        return;

    if (ctx->is_active && GTK_IS_WIDGET(ctx->widget)) {
        gtk_widget_remove_tick_callback(ctx->widget, ctx->tick_id);
        gtk_widget_set_opacity(ctx->widget, 1.0);
        ctx->is_active = FALSE;
    }

    ensure_animation_table();
    if (ctx->widget) {
        g_hash_table_remove(active_animations, ctx->widget);
    }

    g_free(ctx);
}

/* ============================================================
 * Glow Animation
 * ============================================================ */

static gboolean on_glow_tick(GtkWidget *widget, GdkFrameClock *frame_clock, gpointer user_data) {
    AnimationContext *ctx = (AnimationContext *)user_data;

    if (!ctx || !ctx->is_active || !GTK_IS_WIDGET(widget)) {
        return G_SOURCE_REMOVE;
    }

    gint64 frame_time = gdk_frame_clock_get_frame_time(frame_clock);
    gdouble elapsed_ms = (frame_time - ctx->start_time) / 1000.0;

    if (elapsed_ms >= ctx->duration_ms) {
        ctx->is_active = FALSE;
        gtk_widget_set_opacity(widget, 1.0);
        g_hash_table_remove(active_animations, widget);
        return G_SOURCE_REMOVE;
    }

    gdouble progress = elapsed_ms / ctx->duration_ms;
    gdouble smooth = ease_in_out_cubic(progress);
    gdouble glow = sin(smooth * G_PI) * ctx->intensity;
    gdouble opacity = 1.0 - (glow * 0.15);

    gtk_widget_set_opacity(widget, opacity);

    return G_SOURCE_CONTINUE;
}

AnimationContext *animation_glow_start(GtkWidget *widget, guint duration_ms, gdouble intensity) {
    if (!GTK_IS_WIDGET(widget))
        return NULL;

    ensure_animation_table();

    AnimationContext *existing = g_hash_table_lookup(active_animations, widget);
    if (existing) {
        animation_glow_stop(existing);
    }

    AnimationContext *ctx = g_new0(AnimationContext, 1);
    ctx->widget = widget;
    ctx->duration_ms = duration_ms;
    ctx->intensity = CLAMP(intensity, 0.0, 1.0);
    ctx->exponent = 1.0;
    ctx->is_active = TRUE;
    ctx->type = ANIM_GLOW;

    GdkFrameClock *clock = gtk_widget_get_frame_clock(widget);
    if (clock) {
        ctx->start_time = gdk_frame_clock_get_frame_time(clock);
    } else {
        ctx->start_time = g_get_monotonic_time();
    }

    ctx->tick_id = gtk_widget_add_tick_callback(widget, (GtkTickCallback)on_glow_tick, ctx, NULL);

    g_hash_table_insert(active_animations, widget, ctx);

    return ctx;
}

void animation_glow_stop(AnimationContext *ctx) {
    if (!ctx)
        return;

    if (ctx->is_active && GTK_IS_WIDGET(ctx->widget)) {
        gtk_widget_remove_tick_callback(ctx->widget, ctx->tick_id);
        gtk_widget_set_opacity(ctx->widget, 1.0);
        ctx->is_active = FALSE;
    }

    ensure_animation_table();
    if (ctx->widget) {
        g_hash_table_remove(active_animations, ctx->widget);
    }

    g_free(ctx);
}

/* ============================================================
 * Rainbow/Color Cycle Animation
 * ============================================================ */

static gboolean on_rainbow_tick(GtkWidget *widget, GdkFrameClock *frame_clock, gpointer user_data) {
    AnimationContext *ctx = (AnimationContext *)user_data;

    if (!ctx || !ctx->is_active || !GTK_IS_WIDGET(widget)) {
        return G_SOURCE_REMOVE;
    }

    gint64 frame_time = gdk_frame_clock_get_frame_time(frame_clock);
    gdouble elapsed_ms = (frame_time - ctx->start_time) / 1000.0;

    if (elapsed_ms >= ctx->duration_ms) {
        ctx->is_active = FALSE;
        gtk_widget_set_opacity(widget, 1.0);
        g_hash_table_remove(active_animations, widget);
        return G_SOURCE_REMOVE;
    }

    gdouble progress = elapsed_ms / ctx->duration_ms;
    gdouble smooth = ease_in_out_cubic(progress);
    gdouble opacity = 0.7 + (0.3 * sin(smooth * G_PI * 2.0));

    gtk_widget_set_opacity(widget, opacity);

    return G_SOURCE_CONTINUE;
}

AnimationContext *animation_rainbow_start(GtkWidget *widget, guint duration_ms, gdouble cycles) {
    if (!GTK_IS_WIDGET(widget))
        return NULL;

    ensure_animation_table();

    AnimationContext *existing = g_hash_table_lookup(active_animations, widget);
    if (existing) {
        animation_rainbow_stop(existing);
    }

    AnimationContext *ctx = g_new0(AnimationContext, 1);

    ctx->widget = widget;
    ctx->duration_ms = duration_ms;
    ctx->intensity = (cycles > 0.0) ? cycles : 1.0;
    ctx->exponent = 1.0;
    ctx->is_active = TRUE;
    ctx->type = ANIM_RAINBOW;
    ctx->extra_data = NULL;

    GdkFrameClock *clock = gtk_widget_get_frame_clock(widget);
    if (clock) {
        ctx->start_time = gdk_frame_clock_get_frame_time(clock);
    } else {
        ctx->start_time = g_get_monotonic_time();
    }

    ctx->tick_id =
        gtk_widget_add_tick_callback(widget, (GtkTickCallback)on_rainbow_tick, ctx, NULL);

    g_hash_table_insert(active_animations, widget, ctx);

    return ctx;
}

void animation_rainbow_stop(AnimationContext *ctx) {
    if (!ctx)
        return;

    if (ctx->is_active && GTK_IS_WIDGET(ctx->widget)) {
        gtk_widget_remove_tick_callback(ctx->widget, ctx->tick_id);
        gtk_widget_set_opacity(ctx->widget, 1.0);
        ctx->is_active = FALSE;
    }

    ensure_animation_table();
    if (ctx->widget) {
        g_hash_table_remove(active_animations, ctx->widget);
    }

    g_free(ctx);
}

/* ============================================================
 * Generic Stop Function
 * ============================================================ */

void animation_stop(AnimationContext *ctx) {
    if (!ctx)
        return;

    switch (ctx->type) {
    case ANIM_SHAKE:
        animation_shake_stop(ctx);
        break;
    case ANIM_PULSE:
        animation_pulse_stop(ctx);
        break;
    case ANIM_BOUNCE:
        animation_bounce_stop(ctx);
        break;
    case ANIM_SLIDE:
        animation_slide_stop(ctx);
        break;
    case ANIM_ROTATE:
        animation_rotate_stop(ctx);
        break;
    case ANIM_SCALE:
        animation_scale_stop(ctx);
        break;
    case ANIM_GLOW:
        animation_glow_stop(ctx);
        break;
    case ANIM_RAINBOW:
        animation_rainbow_stop(ctx);
        break;
    default:
        if (GTK_IS_WIDGET(ctx->widget)) {
            gtk_widget_remove_tick_callback(ctx->widget, ctx->tick_id);
        }
        g_free(ctx->extra_data);
        g_free(ctx);
        break;
    }
}
