/**
 * Example: Complete Gesture Handling in GTK4 with MVC
 * 
 * This file demonstrates how to integrate touch gestures
 * into the MVC application with proper event handling.
 */

#include <gtk/gtk.h>
#include "utils/gesture_manager.h"
#include "model/app_model.h"

/* Example: Simple gesture-enabled listbox */

typedef struct {
  GtkListBox *listbox;
  AppModel *model;
} GestureExampleContext;

/* Callback for single tap on list item */
static void on_list_item_tap(GtkWidget *widget, gpointer user_data) {
  GestureExampleContext *ctx = (GestureExampleContext *)user_data;
  
  g_message("List item tapped!");
  
  /* You could emit a signal or update selection here */
  gtk_list_box_select_child(ctx->listbox, GTK_LIST_BOX_ROW(widget));
}

/* Callback for long press on list item (context menu) */
static void on_list_item_long_press(GtkWidget *widget, gpointer user_data) {
  GestureExampleContext *ctx = (GestureExampleContext *)user_data;
  
  g_message("Long press detected - showing context menu");
  
  /* Could show a popover or context menu here */
}

/* Callback for swipe left (delete action) */
static void on_list_item_swipe_left(GtkWidget *widget, gpointer user_data) {
  GestureExampleContext *ctx = (GestureExampleContext *)user_data;
  
  g_message("Swipe left - delete item");
  
  /* Delete the item from model and refresh UI */
  gint row_index = gtk_list_box_row_get_index(GTK_LIST_BOX_ROW(widget));
  if (row_index >= 0 && ctx->model) {
    /* Example: app_model_delete_user(ctx->model, row_index); */
  }
}

/* Callback for pan/drag (scroll implementation) */
static void on_list_pan(GtkWidget *widget, GtkPanDirection direction,
                        gdouble offset, gpointer user_data) {
  GestureExampleContext *ctx = (GestureExampleContext *)user_data;
  G_GNUC_UNUSED gdouble unused_offset = offset;
  
  switch (direction) {
    case GTK_PAN_DIRECTION_UP:
      g_message("Pan up: %.1f pixels", offset);
      break;
    case GTK_PAN_DIRECTION_DOWN:
      g_message("Pan down: %.1f pixels", offset);
      break;
    default:
      break;
  }
}

/**
 * Setup gestures for a listbox row
 * Call this when creating each row widget
 */
void setup_row_gestures(GtkListBoxRow *row, 
                        GestureExampleContext *ctx) {
  GtkWidget *row_widget = GTK_WIDGET(row);
  
  /* Single tap - select item */
  gesture_add_tap(row_widget, on_list_item_tap, ctx);
  
  /* Long press - show context menu (800ms) */
  gesture_add_long_press(row_widget, 800, on_list_item_long_press, ctx);
  
  /* Swipe left - delete item */
  gesture_add_swipe_left(row_widget, 100.0, on_list_item_swipe_left, ctx);
}

/**
 * Setup gestures for the listbox container
 */
void setup_listbox_gestures(GtkListBox *listbox,
                            GestureExampleContext *ctx) {
  GtkWidget *listbox_widget = GTK_WIDGET(listbox);
  ctx->listbox = listbox;
  
  /* Pan vertically for smooth scrolling */
  gesture_add_pan(listbox_widget, GTK_ORIENTATION_VERTICAL,
                  (PanCallback)on_list_pan, ctx);
}

/**
 * Example: Gesture-enabled image viewer
 */
typedef struct {
  GtkDrawingArea *canvas;
  gdouble zoom_level;
  gdouble rotation_angle;
} ImageViewerContext;

static void on_image_pinch(GtkWidget *widget, gdouble scale,
                           gpointer user_data) {
  ImageViewerContext *ctx = (ImageViewerContext *)user_data;
  
  ctx->zoom_level *= scale;
  g_message("Zoom level: %.2f", ctx->zoom_level);
  
  /* Clamp zoom level */
  if (ctx->zoom_level < 0.1) ctx->zoom_level = 0.1;
  if (ctx->zoom_level > 10.0) ctx->zoom_level = 10.0;
  
  gtk_widget_queue_draw(widget);
}

static void on_image_rotate(GtkWidget *widget, gdouble angle,
                            gpointer user_data) {
  ImageViewerContext *ctx = (ImageViewerContext *)user_data;
  
  ctx->rotation_angle += angle;
  g_message("Rotation: %.2f degrees", ctx->rotation_angle * 180.0 / M_PI);
  
  gtk_widget_queue_draw(widget);
}

static void on_image_double_tap(GtkWidget *widget, gpointer user_data) {
  ImageViewerContext *ctx = (ImageViewerContext *)user_data;
  
  /* Reset zoom and rotation */
  ctx->zoom_level = 1.0;
  ctx->rotation_angle = 0.0;
  
  g_message("Image reset to original size");
  gtk_widget_queue_draw(widget);
}

void setup_image_viewer_gestures(GtkDrawingArea *canvas,
                                 ImageViewerContext *ctx) {
  GtkWidget *canvas_widget = GTK_WIDGET(canvas);
  ctx->canvas = canvas;
  ctx->zoom_level = 1.0;
  ctx->rotation_angle = 0.0;
  
  /* Pinch to zoom */
  gesture_add_pinch(canvas_widget, (PinchCallback)on_image_pinch, ctx);
  
  /* Two-finger rotation */
  gesture_add_rotation(canvas_widget, (RotationCallback)on_image_rotate, ctx);
  
  /* Double tap to reset */
  gesture_add_double_tap(canvas_widget, on_image_double_tap, ctx);
}

/**
 * Example: Text input with gesture enhancements
 */
typedef struct {
  GtkEntry *entry;
  gboolean is_editing;
} TextInputContext;

static void on_entry_long_press(GtkWidget *widget, gpointer user_data) {
  TextInputContext *ctx = (TextInputContext *)user_data;
  
  /* Select all text on long press */
  gtk_editable_select_region(GTK_EDITABLE(widget), 0, -1);
  g_message("Text selected (long press)");
}

static void on_entry_double_tap(GtkWidget *widget, gpointer user_data) {
  TextInputContext *ctx = (TextInputContext *)user_data;
  
  /* Select current word on double tap */
  g_message("Double tap on entry - could select word");
}

void setup_entry_gestures(GtkEntry *entry, TextInputContext *ctx) {
  GtkWidget *entry_widget = GTK_WIDGET(entry);
  ctx->entry = entry;
  
  /* Long press to select all */
  gesture_add_long_press(entry_widget, 500, on_entry_long_press, ctx);
  
  /* Double tap to select word */
  gesture_add_double_tap(entry_widget, on_entry_double_tap, ctx);
}

/**
 * Best practices example:
 * Complete cleanup in widget destroy callback
 */
static void on_widget_destroy(GtkWidget *widget, gpointer user_data) {
  /* Remove ALL gestures before widget is destroyed */
  gesture_remove_all(widget);
  
  g_message("All gestures removed from widget");
}

void register_gesture_cleanup(GtkWidget *widget) {
  g_signal_connect(widget, "destroy", G_CALLBACK(on_widget_destroy), NULL);
}

/* ============================================================ */

/**
 * Usage in main application:
 * 
 * In app_controller.c or app_view.c:
 * 
 * void app_view_init_gestures(AppView *view) {
 *     GestureExampleContext *ctx = g_new0(GestureExampleContext, 1);
 *     ctx->model = view->model;
 *     
 *     // Setup listbox gestures
 *     setup_listbox_gestures(view->users_list, ctx);
 *     
 *     // Setup gestures for each row as they're created
 *     // (in app_view_add_user_row function):
 *     setup_row_gestures(GTK_LIST_BOX_ROW(row), ctx);
 *     
 *     // Register cleanup
 *     register_gesture_cleanup(GTK_WIDGET(view->users_list));
 *     
 *     view->gesture_context = ctx;
 * }
 * 
 * void app_view_cleanup(AppView *view) {
 *     gesture_remove_all(GTK_WIDGET(view->users_list));
 *     if (view->gesture_context) {
 *         g_free(view->gesture_context);
 *         view->gesture_context = NULL;
 *     }
 * }
 */
