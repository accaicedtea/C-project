/**
 * GTK4 Gesture Demo – corrected & improved
 *
 * Changes vs. previous iteration:
 *  1. Tap section: ONE widget carries all three gestures via gesture_add_taps().
 *     Single-tap, double-tap and long-press coexist without exclusion.
 *  2. Pan section: replaced the opaque text box with a GtkDrawingArea that
 *     draws a track + draggable handle so feedback is immediately visible.
 *     begin/end signals reset the baseline so every new drag starts cleanly.
 *  3. Pinch: the gesture manager now emits true incremental delta (not the
 *     cumulative GTK value).  The demo applies a log-space dampening factor
 *     of 0.45 for comfortable finger sensitivity.
 *  4. Rotation: delta_rad is already incremental from the gesture manager;
 *     dampened to 35% here so coarse finger movement maps to fine rotation.
 */

#include "utils/animations.h"
#include "utils/gesture_manager.h" /* adjust path to match your project layout */
#include <gtk/gtk.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

/* ================================================================
 * Application state
 * ================================================================ */

typedef struct {
    GtkApplicationWindow *window;

    /* Labels */
    GtkLabel *event_log;
    GtkLabel *tap_counter;
    GtkLabel *tap_last; /* "last: single / double / long" */
    GtkLabel *swipe_counter;
    GtkLabel *pan_counter;
    GtkLabel *pinch_counter;
    GtkLabel *rotation_counter;

    /* Canvas (pinch + rotation) */
    GtkDrawingArea *canvas;
    gdouble canvas_zoom;
    gdouble canvas_rotation;

    /* Pan handle */
    GtkDrawingArea *pan_canvas;
    gdouble pan_handle_y; /* 0 = top, 1 = bottom */
    gdouble pan_base_y;   /* position captured at gesture begin */

    /* Counters */
    guint tap_count, double_tap_count, long_press_count;
    guint swipe_count, pan_count, pinch_count, rotation_count;

    /* Log ring buffer */
    GString *log_buf;

    /* Animation contexts */
    GtkWidget *tap_area; /* For shake animation on long press */
    AnimationContext *current_animation;
    AnimationContext *double_tap_animation;
    AnimationContext *swipe_animation;
} DemoApp;

/* ================================================================
 * Logging
 * ================================================================ */

static void log_event(DemoApp *app, const gchar *tag, const gchar *detail) {
    time_t now = time(NULL);
    struct tm *tm = localtime(&now);
    gchar ts[10];
    strftime(ts, sizeof ts, "%H:%M:%S", tm);

    gchar *line = g_strdup_printf("[%s] %-11s %s", ts, tag, detail ? detail : "");
    if (app->log_buf->len > 0)
        g_string_prepend(app->log_buf, "\n");
    g_string_prepend(app->log_buf, line);
    g_free(line);

    /* Keep at most 20 lines */
    gchar *s = app->log_buf->str;
    int n = 0;
    for (gchar *p = s; *p; p++)
        if (*p == '\n' && ++n == 20) {
            g_string_truncate(app->log_buf, (gsize)((p + 1) - s));
            break;
        }
    gtk_label_set_text(app->event_log, app->log_buf->str);
}

/* ================================================================
 * Swipe revealers
 * ================================================================ */

typedef struct {
    GtkRevealer *rev_left, *rev_right, *rev_up, *rev_down;
    GtkLabel *lbl_left, *lbl_right, *lbl_up, *lbl_down;
} SwipePanels;

static SwipePanels g_panels;
static guint g_hide_timer = 0;

static gboolean hide_panels(gpointer ud) {
    (void)ud;
    gtk_revealer_set_reveal_child(g_panels.rev_left, FALSE);
    gtk_revealer_set_reveal_child(g_panels.rev_right, FALSE);
    gtk_revealer_set_reveal_child(g_panels.rev_up, FALSE);
    gtk_revealer_set_reveal_child(g_panels.rev_down, FALSE);
    g_hide_timer = 0;
    return G_SOURCE_REMOVE;
}

static void show_swipe_panel(const gchar *dir, const gchar *text, const gchar *css_class) {
    if (g_hide_timer) {
        g_source_remove(g_hide_timer);
        g_hide_timer = 0;
    }

    gtk_revealer_set_reveal_child(g_panels.rev_left, FALSE);
    gtk_revealer_set_reveal_child(g_panels.rev_right, FALSE);
    gtk_revealer_set_reveal_child(g_panels.rev_up, FALSE);
    gtk_revealer_set_reveal_child(g_panels.rev_down, FALSE);

    GtkRevealer *rev = NULL;
    GtkLabel *lbl = NULL;
    if (!g_strcmp0(dir, "left")) {
        rev = g_panels.rev_left;
        lbl = g_panels.lbl_left;
    } else if (!g_strcmp0(dir, "right")) {
        rev = g_panels.rev_right;
        lbl = g_panels.lbl_right;
    } else if (!g_strcmp0(dir, "up")) {
        rev = g_panels.rev_up;
        lbl = g_panels.lbl_up;
    } else {
        rev = g_panels.rev_down;
        lbl = g_panels.lbl_down;
    }

    gtk_label_set_text(lbl, text);
    static const gchar *all[] = {"swipe-panel-left", "swipe-panel-right", "swipe-panel-up",
                                 "swipe-panel-down", NULL};
    for (int i = 0; all[i]; i++)
        gtk_widget_remove_css_class(GTK_WIDGET(lbl), all[i]);
    gtk_widget_add_css_class(GTK_WIDGET(lbl), css_class);
    gtk_revealer_set_reveal_child(rev, TRUE);
    g_hide_timer = g_timeout_add(900, hide_panels, NULL);
}

static GtkRevealer *make_panel_revealer(GtkRevealerTransitionType trans, GtkAlign halign,
                                        GtkAlign valign, GtkLabel **out_lbl) {
    GtkRevealer *rev = GTK_REVEALER(gtk_revealer_new());
    gtk_revealer_set_transition_type(rev, trans);
    gtk_revealer_set_transition_duration(rev, 200);
    gtk_revealer_set_reveal_child(rev, FALSE);
    gtk_widget_set_halign(GTK_WIDGET(rev), halign);
    gtk_widget_set_valign(GTK_WIDGET(rev), valign);
    gtk_widget_set_can_target(GTK_WIDGET(rev), FALSE);

    GtkLabel *lbl = GTK_LABEL(gtk_label_new(""));
    gtk_widget_add_css_class(GTK_WIDGET(lbl), "swipe-panel");
    gtk_widget_set_size_request(GTK_WIDGET(lbl), 180, 52);
    gtk_revealer_set_child(rev, GTK_WIDGET(lbl));
    if (out_lbl)
        *out_lbl = lbl;
    return rev;
}

/* ================================================================
 * TAP callbacks   (all wired to the same widget via gesture_add_taps)
 * ================================================================ */

static void on_single_tap(GtkWidget *w, gpointer ud) {
    (void)w;
    DemoApp *app = ud;
    app->tap_count++;
    gchar *s = g_strdup_printf("🎯 Taps: %u  •  Double: %u  •  Long: %u", app->tap_count,
                               app->double_tap_count, app->long_press_count);
    gtk_label_set_text(app->tap_counter, s);
    g_free(s);
    gtk_label_set_text(app->tap_last, "Last detected: Single tap 👆");
    log_event(app, "TAP", "single");

    /* Pulse animation on tap */
    if (app->tap_area && GTK_IS_WIDGET(app->tap_area)) {
        animation_pulse_start(app->tap_area, 400);
    }
}

static void on_double_tap(GtkWidget *w, gpointer ud) {
    (void)w;
    DemoApp *app = ud;
    app->double_tap_count++;
    gchar *s = g_strdup_printf("🎯 Taps: %u  •  Double: %u  •  Long: %u", app->tap_count,
                               app->double_tap_count, app->long_press_count);
    gtk_label_set_text(app->tap_counter, s);
    g_free(s);
    gtk_label_set_text(app->tap_last, "Last detected: Double tap 👆👆");
    log_event(app, "DOUBLE TAP", "double");

    /* Bounce animation on double tap */
    if (app->tap_area && GTK_IS_WIDGET(app->tap_area)) {
        animation_bounce_start(app->tap_area, 600, 0.9);
    }
}

static void on_long_press(GtkWidget *w, gpointer ud) {
    (void)w;
    DemoApp *app = ud;
    app->long_press_count++;
    gchar *s = g_strdup_printf("🎯 Taps: %u  •  Double: %u  •  Long: %u", app->tap_count,
                               app->double_tap_count, app->long_press_count);
    gtk_label_set_text(app->tap_counter, s);
    g_free(s);
    gtk_label_set_text(app->tap_last, "Last detected: Long press ✊ (500 ms)");
    log_event(app, "LONG PRESS", "500 ms");

    /* Start exponential shake animation on the tap area widget */
    if (app->tap_area && GTK_IS_WIDGET(app->tap_area)) {
        if (app->current_animation) {
            animation_shake_stop(app->current_animation);
            app->current_animation = NULL;
        }
        /* Start shake: 2000ms duration, 0.8 intensity, 2.5 exponent for aggressive shake */
        app->current_animation = animation_shake_start(app->tap_area, 2000, 0.8, 2.5);
    }
}

static void on_finger_released(GtkWidget *w, gpointer ud) {
    (void)w;
    DemoApp *app = ud;

    /* Show glow effect when finger is lifted */
    if (app->tap_area && GTK_IS_WIDGET(app->tap_area)) {
        animation_glow_start(app->tap_area, 300, 0.7);
    }
}

/* ================================================================
 * SWIPE callbacks
 * ================================================================ */

static void on_swipe_left(GtkWidget *w, gpointer ud) {
    (void)w;
    DemoApp *app = ud;
    app->swipe_count++;
    gchar *s = g_strdup_printf("👈 Swipes: %u", app->swipe_count);
    gtk_label_set_text(app->swipe_counter, s);
    g_free(s);
    log_event(app, "SWIPE", "← LEFT");
    show_swipe_panel("left", "← SWIPE LEFT", "swipe-panel-left");

    /* Slide animation on swipe left */
    if (app->tap_area && GTK_IS_WIDGET(app->tap_area)) {
        animation_slide_start(app->tap_area, SLIDE_LEFT, 80, 500);
    }
}
static void on_swipe_right(GtkWidget *w, gpointer ud) {
    (void)w;
    DemoApp *app = ud;
    app->swipe_count++;
    gchar *s = g_strdup_printf("👉 Swipes: %u", app->swipe_count);
    gtk_label_set_text(app->swipe_counter, s);
    g_free(s);
    log_event(app, "SWIPE", "→ RIGHT");
    show_swipe_panel("right", "SWIPE RIGHT →", "swipe-panel-right");

    /* Slide animation on swipe right */
    if (app->tap_area && GTK_IS_WIDGET(app->tap_area)) {
        animation_slide_start(app->tap_area, SLIDE_RIGHT, 80, 500);
    }
}

static void on_swipe_up(GtkWidget *w, gpointer ud) {
    (void)w;
    DemoApp *app = ud;
    app->swipe_count++;
    gchar *s = g_strdup_printf("⬆ Swipes: %u", app->swipe_count);
    gtk_label_set_text(app->swipe_counter, s);
    g_free(s);
    log_event(app, "SWIPE", "↑ UP");
    show_swipe_panel("up", "↑  SWIPE UP", "swipe-panel-up");

    /* Slide animation on swipe up */
    if (app->tap_area && GTK_IS_WIDGET(app->tap_area)) {
        animation_slide_start(app->tap_area, SLIDE_UP, 60, 500);
    }
}
static void on_swipe_down(GtkWidget *w, gpointer ud) {
    (void)w;
    DemoApp *app = ud;
    app->swipe_count++;
    gchar *s = g_strdup_printf("⬇ Swipes: %u", app->swipe_count);
    gtk_label_set_text(app->swipe_counter, s);
    g_free(s);
    log_event(app, "SWIPE", "↓ DOWN");
    show_swipe_panel("down", "↓  SWIPE DOWN", "swipe-panel-down");

    /* Slide animation on swipe down */
    if (app->tap_area && GTK_IS_WIDGET(app->tap_area)) {
        animation_slide_start(app->tap_area, SLIDE_DOWN, 60, 500);
    }
}

/* ================================================================
 * PAN callbacks + drawing
 * ================================================================ */

/* Called at gesture start – snapshot the handle's current position */
static void on_pan_begin(GtkGesture *gesture, GdkEventSequence *seq, gpointer ud) {
    (void)gesture;
    (void)seq;
    DemoApp *app = ud;
    app->pan_base_y = app->pan_handle_y;
}

/* Called at gesture end – commit final position as new baseline */
static void on_pan_end(GtkGesture *gesture, GdkEventSequence *seq, gpointer ud) {
    (void)gesture;
    (void)seq;
    DemoApp *app = ud;
    app->pan_base_y = app->pan_handle_y;
}

static void on_pan(GtkWidget *w, GtkPanDirection dir, gdouble offset, gpointer ud) {
    DemoApp *app = ud;
    app->pan_count++;

    /* offset is cumulative pixels from gesture start, always positive.
       Normalise by the usable height of the canvas (subtract padding). */
    int h = gtk_widget_get_height(w);
    if (h < 10)
        h = 200;
    gdouble norm = offset / (gdouble)(h - 40); /* 20 px padding each side */

    if (dir == GTK_PAN_DIRECTION_UP)
        app->pan_handle_y = CLAMP(app->pan_base_y - norm, 0.0, 1.0);
    else
        app->pan_handle_y = CLAMP(app->pan_base_y + norm, 0.0, 1.0);

    gchar *s = g_strdup_printf("↕ Pans: %u", app->pan_count);
    gtk_label_set_text(app->pan_counter, s);
    g_free(s);

    s = g_strdup_printf("%s  %.0f%%", (dir == GTK_PAN_DIRECTION_UP) ? "↑ UP" : "↓ DOWN",
                        app->pan_handle_y * 100.0);
    log_event(app, "PAN", s);
    g_free(s);

    gtk_widget_queue_draw(w);
}

/* Draw the pan track + draggable handle */
static void on_pan_draw(GtkDrawingArea *da, cairo_t *cr, int w, int h, gpointer ud) {
    (void)da;
    DemoApp *app = ud;

    /* Background */
    cairo_set_source_rgb(cr, 0.93, 0.98, 0.93);
    cairo_paint(cr);

    const int PAD = 20;
    const int cx = w / 2;
    const int track_top = PAD;
    const int track_bot = h - PAD;
    const int track_h = track_bot - track_top;

    /* ---- Track ---- */
    cairo_set_source_rgba(cr, 0.55, 0.75, 0.55, 0.5);
    cairo_set_line_width(cr, 6.0);
    cairo_set_line_cap(cr, CAIRO_LINE_CAP_ROUND);
    cairo_move_to(cr, cx, track_top);
    cairo_line_to(cr, cx, track_bot);
    cairo_stroke(cr);

    /* ---- Arrow hints ---- */
    cairo_set_source_rgba(cr, 0.3, 0.55, 0.3, 0.7);
    cairo_select_font_face(cr, "Sans", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
    cairo_set_font_size(cr, 15);

    cairo_move_to(cr, cx - 8, track_top - 2);
    cairo_show_text(cr, "↑");
    cairo_move_to(cr, cx - 8, track_bot + 16);
    cairo_show_text(cr, "↓");

    /* ---- Drag hint text (only when at rest in the middle-ish) ---- */
    if (app->pan_handle_y > 0.35 && app->pan_handle_y < 0.65) {
        cairo_set_source_rgba(cr, 0.3, 0.55, 0.3, 0.4);
        cairo_set_font_size(cr, 12);
        const char *hint = "drag up / down";
        cairo_text_extents_t te;
        cairo_text_extents(cr, hint, &te);
        cairo_move_to(cr, cx - te.width / 2 - te.x_bearing, h / 2 + 40);
        cairo_show_text(cr, hint);
    }

    /* ---- Handle ---- */
    gdouble hy = track_top + track_h * app->pan_handle_y;
    const double RADIUS = 18.0;

    /* shadow */
    cairo_set_source_rgba(cr, 0, 0, 0, 0.12);
    cairo_arc(cr, cx + 2, hy + 3, RADIUS, 0, 2 * G_PI);
    cairo_fill(cr);

    /* fill */
    cairo_pattern_t *pat = cairo_pattern_create_radial(cx - 4, hy - 4, 2, cx, hy, RADIUS);
    cairo_pattern_add_color_stop_rgb(pat, 0, 0.55, 0.88, 0.56);
    cairo_pattern_add_color_stop_rgb(pat, 1, 0.18, 0.65, 0.22);
    cairo_set_source(cr, pat);
    cairo_pattern_destroy(pat);
    cairo_arc(cr, cx, hy, RADIUS, 0, 2 * G_PI);
    cairo_fill(cr);

    /* rim */
    cairo_set_source_rgb(cr, 0.13, 0.50, 0.16);
    cairo_set_line_width(cr, 1.5);
    cairo_arc(cr, cx, hy, RADIUS, 0, 2 * G_PI);
    cairo_stroke(cr);

    /* percentage label next to handle */
    gchar *pct = g_strdup_printf("%.0f %%", app->pan_handle_y * 100.0);
    cairo_set_source_rgb(cr, 0.10, 0.40, 0.12);
    cairo_set_font_size(cr, 13);
    cairo_move_to(cr, cx + RADIUS + 8, hy + 5);
    cairo_show_text(cr, pct);
    g_free(pct);
}

/* ================================================================
 * PINCH + ROTATION callbacks
 *
 * Sensitivity tuning:
 *   Pinch  – log-space dampening factor 0.45 (human finger spread is
 *             coarse; halving the exponent gives comfortable control).
 *   Rotate – linear scale 0.35 (same reasoning).
 * ================================================================ */

static void on_pinch(GtkWidget *w, gdouble scale_delta, gpointer ud) {
    DemoApp *app = ud;

    /*
     * scale_delta is the true multiplicative increment between events.
     * Apply log-space dampening:  damped = delta ^ 0.45
     * This keeps direction correct while reducing the range.
     */
    gdouble damped = (scale_delta > 1e-9) ? pow(scale_delta, 0.45) : 1.0;
    app->canvas_zoom = CLAMP(app->canvas_zoom * damped, 0.10, 12.0);

    app->pinch_count++;
    gchar *s = g_strdup_printf("🔍 Pinches: %u  zoom %.2f×", app->pinch_count, app->canvas_zoom);
    gtk_label_set_text(app->pinch_counter, s);
    g_free(s);
    gtk_widget_queue_draw(w);
}

static void on_rotation(GtkWidget *w, gdouble delta_rad, gpointer ud) {
    DemoApp *app = ud;

    /*
     * Rotation dampening for human fingers / trackpad:
     *   DAMPEN  0.12  → only ~7° of canvas movement per 60° of finger sweep
     *   Dead zone 2°  → absorbs trackpad jitter and accidental micro-rotations
     */
    const gdouble DAMPEN = 0.7;
    const gdouble DEAD_ZONE = 0.035; /* radians ≈ 2° */

    gdouble damped = delta_rad * DAMPEN;
    if (fabs(damped) < DEAD_ZONE * DAMPEN)
        return;

    app->rotation_count++;
    app->canvas_rotation += damped;
    app->canvas_rotation = fmod(app->canvas_rotation + 4 * G_PI, 2 * G_PI);

    gchar *s = g_strdup_printf("🔄 Rotations: %u  %.0f°", app->rotation_count,
                               app->canvas_rotation * 180.0 / G_PI);
    gtk_label_set_text(app->rotation_counter, s);
    g_free(s);
    gtk_widget_queue_draw(w);
}

/* ================================================================
 * Canvas draw (pinch / rotation visualiser)
 * ================================================================ */

static void on_canvas_draw(GtkDrawingArea *da, cairo_t *cr, int w, int h, gpointer ud) {
    (void)da;
    DemoApp *app = ud;

    cairo_set_source_rgb(cr, 0.97, 0.97, 1.00);
    cairo_paint(cr);

    cairo_translate(cr, w / 2.0, h / 2.0);
    cairo_rotate(cr, app->canvas_rotation);
    cairo_scale(cr, app->canvas_zoom, app->canvas_zoom);

    /* Grid */
    cairo_set_source_rgba(cr, 0.75, 0.75, 0.85, 0.4);
    cairo_set_line_width(cr, 0.7);
    for (int i = -3; i <= 3; i++) {
        cairo_move_to(cr, i * 25, -90);
        cairo_line_to(cr, i * 25, 90);
        cairo_move_to(cr, -90, i * 25);
        cairo_line_to(cr, 90, i * 25);
    }
    cairo_stroke(cr);

    /* Outer ring */
    cairo_set_source_rgb(cr, 0.20, 0.50, 0.90);
    cairo_set_line_width(cr, 2.5);
    cairo_arc(cr, 0, 0, 55, 0, 2 * G_PI);
    cairo_stroke(cr);

    /* Tick marks */
    cairo_set_line_width(cr, 1.5);
    for (int i = 0; i < 12; i++) {
        double a = i * G_PI / 6.0;
        double r1 = (i % 3 == 0) ? 44 : 48;
        cairo_move_to(cr, r1 * cos(a), r1 * sin(a));
        cairo_line_to(cr, 55 * cos(a), 55 * sin(a));
    }
    cairo_stroke(cr);

    /* Direction arrow (red) */
    cairo_set_source_rgb(cr, 0.88, 0.18, 0.18);
    cairo_set_line_width(cr, 3.5);
    cairo_move_to(cr, 0, 0);
    cairo_line_to(cr, 50, 0);
    cairo_stroke(cr);

    cairo_move_to(cr, 50, 0);
    cairo_line_to(cr, 41, -6);
    cairo_line_to(cr, 41, 6);
    cairo_close_path(cr);
    cairo_fill(cr);

    /* Centre dot */
    cairo_set_source_rgb(cr, 0.30, 0.30, 0.60);
    cairo_arc(cr, 0, 0, 4, 0, 2 * G_PI);
    cairo_fill(cr);

    /* Zoom & angle label (in screen space, so reset transform first) */
    cairo_identity_matrix(cr);
    gchar *info = g_strdup_printf("zoom %.2f×  angle %.0f°", app->canvas_zoom,
                                  app->canvas_rotation * 180.0 / G_PI);
    cairo_set_source_rgba(cr, 0.3, 0.3, 0.5, 0.75);
    cairo_select_font_face(cr, "Monospace", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
    cairo_set_font_size(cr, 12);
    cairo_move_to(cr, 8, h - 8);
    cairo_show_text(cr, info);
    g_free(info);
}

/* ================================================================
 * Widget helpers
 * ================================================================ */

static GtkWidget *make_tap_area(const gchar *label_text, int w, int h, const gchar *css_class) {
    GtkBox *box = GTK_BOX(gtk_box_new(GTK_ORIENTATION_VERTICAL, 0));
    gtk_widget_set_size_request(GTK_WIDGET(box), w, h);
    gtk_widget_add_css_class(GTK_WIDGET(box), css_class);
    gtk_widget_set_halign(GTK_WIDGET(box), GTK_ALIGN_FILL);
    gtk_widget_set_valign(GTK_WIDGET(box), GTK_ALIGN_CENTER);
    gtk_widget_set_focusable(GTK_WIDGET(box), TRUE);
    gtk_widget_set_can_target(GTK_WIDGET(box), TRUE);

    GtkLabel *lbl = GTK_LABEL(gtk_label_new(label_text));
    gtk_label_set_justify(lbl, GTK_JUSTIFY_CENTER);
    gtk_widget_set_halign(GTK_WIDGET(lbl), GTK_ALIGN_CENTER);
    gtk_widget_set_valign(GTK_WIDGET(lbl), GTK_ALIGN_CENTER);
    gtk_widget_set_hexpand(GTK_WIDGET(lbl), TRUE);
    gtk_widget_set_vexpand(GTK_WIDGET(lbl), TRUE);
    gtk_box_append(box, GTK_WIDGET(lbl));
    return GTK_WIDGET(box);
}

static GtkBox *make_section(const gchar *title, const gchar *hint) {
    GtkBox *box = GTK_BOX(gtk_box_new(GTK_ORIENTATION_VERTICAL, 8));
    gtk_widget_set_margin_top(GTK_WIDGET(box), 16);
    gtk_widget_set_margin_start(GTK_WIDGET(box), 16);
    gtk_widget_set_margin_end(GTK_WIDGET(box), 16);
    gtk_widget_set_margin_bottom(GTK_WIDGET(box), 8);

    GtkLabel *t = GTK_LABEL(gtk_label_new(title));
    gtk_widget_set_halign(GTK_WIDGET(t), GTK_ALIGN_START);
    gtk_widget_add_css_class(GTK_WIDGET(t), "section-title");
    gtk_box_append(box, GTK_WIDGET(t));

    if (hint) {
        GtkLabel *h = GTK_LABEL(gtk_label_new(hint));
        gtk_widget_set_halign(GTK_WIDGET(h), GTK_ALIGN_START);
        gtk_label_set_wrap(h, TRUE);
        gtk_widget_add_css_class(GTK_WIDGET(h), "dim-label");
        gtk_box_append(box, GTK_WIDGET(h));
    }
    return box;
}

/* ================================================================
 * CSS
 * ================================================================ */

static const gchar *APP_CSS =
    /* Tap area */
    ".tap-area {"
    "  background: #e3f2fd;"
    "  border: 2px solid #90caf9;"
    "  border-radius: 12px;"
    "  font-size: 14px;"
    "  font-weight: bold;"
    "  color: #1565c0;"
    "}"
    ".tap-area:focus { outline: none; }"

    /* Swipe area */
    ".swipe-area {"
    "  background: #fce4ec;"
    "  border: 2px solid #f48fb1;"
    "  border-radius: 12px;"
    "  font-size: 15px;"
    "  font-weight: bold;"
    "  color: #880e4f;"
    "}"

    /* Canvas */
    ".demo-canvas {"
    "  background: white;"
    "  border: 1px solid #c0c0d0;"
    "  border-radius: 8px;"
    "}"
    ".pan-canvas {"
    "  border: 2px dashed #66bb6a;"
    "  border-radius: 12px;"
    "}"

    /* Swipe panels */
    ".swipe-panel {"
    "  border-radius: 10px;"
    "  padding: 10px 22px;"
    "  font-size: 15px;"
    "  font-weight: bold;"
    "  color: white;"
    "  margin: 12px;"
    "}"
    ".swipe-panel-left  { background: #e53935; }"
    ".swipe-panel-right { background: #1e88e5; }"
    ".swipe-panel-up    { background: #43a047; }"
    ".swipe-panel-down  { background: #fb8c00; }"

    /* Tap result label */
    ".tap-result {"
    "  font-size: 14px;"
    "  font-weight: bold;"
    "  color: #1a237e;"
    "  padding: 6px 0;"
    "}"

    /* Misc */
    ".log-label     { font-family: monospace; font-size: 12px; }"
    ".section-title { font-weight: bold; font-size: 15px; }"
    ".dim-label     { color: alpha(currentColor, 0.55); font-size: 12px; }"
    ".counter-label { font-size: 13px; font-weight: bold; }"
    ".title-1       { font-size: 22px; font-weight: bold; }";

/* ================================================================
 * UI setup
 * ================================================================ */

static void setup_ui(DemoApp *app) {
    gtk_window_set_title(GTK_WINDOW(app->window), "GTK4 Gesture Demo");
    gtk_window_set_default_size(GTK_WINDOW(app->window), 880, 1020);

    /* CSS */
    GtkCssProvider *css = gtk_css_provider_new();
    gtk_css_provider_load_from_string(css, APP_CSS);
    gtk_style_context_add_provider_for_display(gdk_display_get_default(), GTK_STYLE_PROVIDER(css),
                                               GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);

    /* Root */
    GtkBox *root = GTK_BOX(gtk_box_new(GTK_ORIENTATION_VERTICAL, 0));
    gtk_window_set_child(GTK_WINDOW(app->window), GTK_WIDGET(root));

    /* Header */
    {
        GtkBox *hdr = GTK_BOX(gtk_box_new(GTK_ORIENTATION_VERTICAL, 4));
        gtk_widget_set_margin_top(GTK_WIDGET(hdr), 16);
        gtk_widget_set_margin_start(GTK_WIDGET(hdr), 16);
        gtk_widget_set_margin_end(GTK_WIDGET(hdr), 16);
        GtkLabel *ht = GTK_LABEL(gtk_label_new("Touch Gesture Showcase"));
        gtk_widget_add_css_class(GTK_WIDGET(ht), "title-1");
        gtk_box_append(hdr, GTK_WIDGET(ht));
        GtkLabel *hs = GTK_LABEL(gtk_label_new("Tap / double-tap / long-press share one widget. "
                                               "Pan has a live draggable handle."));
        gtk_widget_add_css_class(GTK_WIDGET(hs), "dim-label");
        gtk_box_append(hdr, GTK_WIDGET(hs));
        gtk_box_append(root, GTK_WIDGET(hdr));
    }

    /* Scrollable content */
    GtkScrolledWindow *sc = GTK_SCROLLED_WINDOW(gtk_scrolled_window_new());
    gtk_scrolled_window_set_policy(sc, GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
    gtk_widget_set_vexpand(GTK_WIDGET(sc), TRUE);
    gtk_box_append(root, GTK_WIDGET(sc));

    GtkBox *content = GTK_BOX(gtk_box_new(GTK_ORIENTATION_VERTICAL, 0));
    gtk_scrolled_window_set_child(sc, GTK_WIDGET(content));

    /* ======================================================= *
     * TAP SECTION                                             *
     * All three gestures registered on the same widget.       *
     * ======================================================= */
    {
        GtkBox *sec = make_section(
            "🎯 TAP GESTURES", "One area handles single-tap, double-tap and long-press together "
                               "via gesture_add_taps().  Single-tap is delayed by the system "
                               "double-click timeout so it doesn't fire during a double-tap.");

        GtkWidget *area = make_tap_area("Tap · Double-tap · Long press\n(all on the same widget)",
                                        -1, 110, "tap-area");
        gtk_widget_set_hexpand(area, TRUE);
        gtk_widget_set_margin_start(area, 8);
        gtk_widget_set_margin_end(area, 8);

        /* Store tap area reference for animations */
        app->tap_area = area;
        app->current_animation = NULL;

        /* *** Unified call – replaces the old trio of gesture_add_* calls *** */
        TapGestureContext *tap_ctx = gesture_add_taps(
            area, on_single_tap, on_double_tap, on_long_press, 500, /* long-press threshold ms */
            app);
        /* Set release callback for visual feedback when finger is lifted */
        tap_gesture_set_release_callback(tap_ctx, on_finger_released);

        gtk_box_append(sec, area);

        app->tap_last = GTK_LABEL(gtk_label_new("Last detected: —"));
        gtk_widget_add_css_class(GTK_WIDGET(app->tap_last), "tap-result");
        gtk_widget_set_halign(GTK_WIDGET(app->tap_last), GTK_ALIGN_CENTER);
        gtk_widget_set_margin_top(GTK_WIDGET(app->tap_last), 4);
        gtk_box_append(sec, GTK_WIDGET(app->tap_last));

        app->tap_counter = GTK_LABEL(gtk_label_new("🎯 Taps: 0  •  Double: 0  •  Long: 0"));
        gtk_widget_add_css_class(GTK_WIDGET(app->tap_counter), "counter-label");
        gtk_widget_set_margin_start(GTK_WIDGET(app->tap_counter), 8);
        gtk_box_append(sec, GTK_WIDGET(app->tap_counter));

        gtk_box_append(content, GTK_WIDGET(sec));
    }

    /* ======================================================= *
     * SWIPE SECTION                                           *
     * ======================================================= */
    {
        GtkBox *sec = make_section("👈 SWIPE GESTURES",
                                   "Flick quickly in any direction. A coloured badge slides in "
                                   "from the matching edge.");

        GtkOverlay *ov = GTK_OVERLAY(gtk_overlay_new());
        gtk_widget_set_overflow(GTK_WIDGET(ov), GTK_OVERFLOW_HIDDEN);
        gtk_widget_set_margin_start(GTK_WIDGET(ov), 8);
        gtk_widget_set_margin_end(GTK_WIDGET(ov), 8);

        GtkWidget *sw = make_tap_area("⟵   Swipe in any direction   ⟶", -1, 130, "swipe-area");
        gtk_widget_set_hexpand(sw, TRUE);
        gtk_overlay_set_child(ov, sw);

        gesture_add_swipe_left(sw, 80.0, on_swipe_left, app);
        gesture_add_swipe_right(sw, 80.0, on_swipe_right, app);
        gesture_add_swipe_up(sw, 80.0, on_swipe_up, app);
        gesture_add_swipe_down(sw, 80.0, on_swipe_down, app);

        g_panels.rev_left =
            make_panel_revealer(GTK_REVEALER_TRANSITION_TYPE_SLIDE_LEFT, GTK_ALIGN_END,
                                GTK_ALIGN_CENTER, &g_panels.lbl_left);
        gtk_overlay_add_overlay(ov, GTK_WIDGET(g_panels.rev_left));

        g_panels.rev_right =
            make_panel_revealer(GTK_REVEALER_TRANSITION_TYPE_SLIDE_RIGHT, GTK_ALIGN_START,
                                GTK_ALIGN_CENTER, &g_panels.lbl_right);
        gtk_overlay_add_overlay(ov, GTK_WIDGET(g_panels.rev_right));

        g_panels.rev_up = make_panel_revealer(GTK_REVEALER_TRANSITION_TYPE_SLIDE_UP,
                                              GTK_ALIGN_CENTER, GTK_ALIGN_END, &g_panels.lbl_up);
        gtk_overlay_add_overlay(ov, GTK_WIDGET(g_panels.rev_up));

        g_panels.rev_down =
            make_panel_revealer(GTK_REVEALER_TRANSITION_TYPE_SLIDE_DOWN, GTK_ALIGN_CENTER,
                                GTK_ALIGN_START, &g_panels.lbl_down);
        gtk_overlay_add_overlay(ov, GTK_WIDGET(g_panels.rev_down));

        gtk_box_append(sec, GTK_WIDGET(ov));

        app->swipe_counter = GTK_LABEL(gtk_label_new("👉 Swipes: 0"));
        gtk_widget_add_css_class(GTK_WIDGET(app->swipe_counter), "counter-label");
        gtk_widget_set_margin_start(GTK_WIDGET(app->swipe_counter), 8);
        gtk_box_append(sec, GTK_WIDGET(app->swipe_counter));

        gtk_box_append(content, GTK_WIDGET(sec));
    }

    /* ======================================================= *
     * PAN SECTION                                             *
     * DrawingArea with a draggable handle on a vertical track *
     * ======================================================= */
    {
        GtkBox *sec =
            make_section("↕ PAN GESTURE", "Drag the green handle up or down. "
                                          "The position percentage updates live while you drag.");

        app->pan_canvas = GTK_DRAWING_AREA(gtk_drawing_area_new());
        gtk_widget_set_size_request(GTK_WIDGET(app->pan_canvas), -1, 180);
        gtk_widget_add_css_class(GTK_WIDGET(app->pan_canvas), "pan-canvas");
        gtk_widget_set_hexpand(GTK_WIDGET(app->pan_canvas), TRUE);
        gtk_widget_set_margin_start(GTK_WIDGET(app->pan_canvas), 8);
        gtk_widget_set_margin_end(GTK_WIDGET(app->pan_canvas), 8);
        gtk_drawing_area_set_draw_func(app->pan_canvas, on_pan_draw, app, NULL);

        /* Attach vertical pan */
        gesture_add_pan(GTK_WIDGET(app->pan_canvas), GTK_ORIENTATION_VERTICAL, on_pan, app);

        /* Hook begin/end on the underlying GtkGesturePan to track baseline */
        GtkGesture *pan_gesture = g_object_get_data(G_OBJECT(app->pan_canvas), "pan-gesture");
        g_signal_connect(pan_gesture, "begin", G_CALLBACK(on_pan_begin), app);
        g_signal_connect(pan_gesture, "end", G_CALLBACK(on_pan_end), app);

        gtk_box_append(sec, GTK_WIDGET(app->pan_canvas));

        app->pan_counter = GTK_LABEL(gtk_label_new("↕ Pans: 0"));
        gtk_widget_add_css_class(GTK_WIDGET(app->pan_counter), "counter-label");
        gtk_widget_set_margin_start(GTK_WIDGET(app->pan_counter), 8);
        gtk_box_append(sec, GTK_WIDGET(app->pan_counter));

        gtk_box_append(content, GTK_WIDGET(sec));
    }

    /* ======================================================= *
     * PINCH + ROTATION SECTION                               *
     * ======================================================= */
    {
        GtkBox *sec = make_section("🔍 PINCH & ROTATE",
                                   "Two-finger gestures on the canvas (trackpad: pinch-to-zoom / "
                                   "two-finger twist). Sensitivity is dampened for comfortable "
                                   "finger movement.");

        app->canvas = GTK_DRAWING_AREA(gtk_drawing_area_new());
        gtk_widget_set_size_request(GTK_WIDGET(app->canvas), -1, 300);
        gtk_widget_add_css_class(GTK_WIDGET(app->canvas), "demo-canvas");
        gtk_widget_set_hexpand(GTK_WIDGET(app->canvas), TRUE);
        gtk_widget_set_margin_start(GTK_WIDGET(app->canvas), 8);
        gtk_widget_set_margin_end(GTK_WIDGET(app->canvas), 8);
        gtk_drawing_area_set_draw_func(app->canvas, on_canvas_draw, app, NULL);

        gesture_add_pinch(GTK_WIDGET(app->canvas), on_pinch, app);
        gesture_add_rotation(GTK_WIDGET(app->canvas), on_rotation, app);

        gtk_box_append(sec, GTK_WIDGET(app->canvas));

        GtkBox *row = GTK_BOX(gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 20));
        gtk_widget_set_margin_start(GTK_WIDGET(row), 8);

        app->pinch_counter = GTK_LABEL(gtk_label_new("🔍 Pinches: 0  zoom 1.00×"));
        app->rotation_counter = GTK_LABEL(gtk_label_new("🔄 Rotations: 0  0°"));
        gtk_widget_add_css_class(GTK_WIDGET(app->pinch_counter), "counter-label");
        gtk_widget_add_css_class(GTK_WIDGET(app->rotation_counter), "counter-label");
        gtk_box_append(row, GTK_WIDGET(app->pinch_counter));
        gtk_box_append(row, GTK_WIDGET(app->rotation_counter));
        gtk_box_append(sec, GTK_WIDGET(row));

        gtk_box_append(content, GTK_WIDGET(sec));
    }

    /* ======================================================= *
     * EVENT LOG                                               *
     * ======================================================= */
    {
        GtkBox *sec = make_section("📋 EVENT LOG", "Real-time gesture log");

        GtkScrolledWindow *log_sc = GTK_SCROLLED_WINDOW(gtk_scrolled_window_new());
        gtk_widget_set_size_request(GTK_WIDGET(log_sc), -1, 180);
        gtk_scrolled_window_set_policy(log_sc, GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

        app->event_log = GTK_LABEL(gtk_label_new("Ready…"));
        gtk_widget_add_css_class(GTK_WIDGET(app->event_log), "log-label");
        gtk_label_set_wrap(app->event_log, FALSE);
        gtk_label_set_selectable(app->event_log, TRUE);
        gtk_widget_set_halign(GTK_WIDGET(app->event_log), GTK_ALIGN_START);
        gtk_widget_set_valign(GTK_WIDGET(app->event_log), GTK_ALIGN_START);
        gtk_widget_set_margin_top(GTK_WIDGET(app->event_log), 6);
        gtk_widget_set_margin_start(GTK_WIDGET(app->event_log), 8);
        gtk_scrolled_window_set_child(log_sc, GTK_WIDGET(app->event_log));
        gtk_box_append(sec, GTK_WIDGET(log_sc));
        gtk_box_append(content, GTK_WIDGET(sec));
    }

    /* Footer */
    {
        GtkLabel *ft =
            GTK_LABEL(gtk_label_new("💡 Trackpad: two-finger pinch/twist for canvas gestures. "
                                    "Touch screen: full multitouch support."));
        gtk_widget_add_css_class(GTK_WIDGET(ft), "dim-label");
        gtk_label_set_wrap(ft, TRUE);
        gtk_widget_set_margin_top(GTK_WIDGET(ft), 12);
        gtk_widget_set_margin_start(GTK_WIDGET(ft), 16);
        gtk_widget_set_margin_end(GTK_WIDGET(ft), 16);
        gtk_widget_set_margin_bottom(GTK_WIDGET(ft), 16);
        gtk_box_append(root, GTK_WIDGET(ft));
    }

    gtk_window_present(GTK_WINDOW(app->window));
}

/* ================================================================
 * Application entry point
 * ================================================================ */

static void on_activate(GtkApplication *gapp, gpointer ud) {
    (void)ud;
    DemoApp *demo = g_new0(DemoApp, 1);
    demo->log_buf = g_string_new("");
    demo->canvas_zoom = 1.0;
    demo->canvas_rotation = 0.0;
    demo->pan_handle_y = 0.5;
    demo->pan_base_y = 0.5;
    demo->window = GTK_APPLICATION_WINDOW(gtk_application_window_new(gapp));
    setup_ui(demo);
    log_event(demo, "APP", "Started — ready for gestures!");
}

int main(int argc, char *argv[]) {
    GtkApplication *app = gtk_application_new("org.gtk.gesture_demo", G_APPLICATION_DEFAULT_FLAGS);
    g_signal_connect(app, "activate", G_CALLBACK(on_activate), NULL);
    int r = g_application_run(G_APPLICATION(app), argc, argv);
    g_object_unref(app);
    return r;
}