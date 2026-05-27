# GTK4 Gesture Examples

This directory contains example code demonstrating how to use the gesture helper library in real applications.

## Files

### gesture_demo.c - 🎯 INTERACTIVE GESTURE SHOWCASE

**A fully functional demo application that shows all gestures in action with visual feedback!**

#### Features:

- ✅ **Live gesture recognition** - See gestures detected in real-time
- ✅ **Visual feedback** - Widget colors change on gesture detection
- ✅ **Event logging** - All gestures logged with timestamp and details
- ✅ **5 gesture types** demonstrated with separate demo areas
- ✅ **Interactive canvas** - Pinch to zoom, two-finger rotation
- ✅ **Counters** - Track total number of each gesture type
- ✅ **All platforms** - Works on desktop (trackpad) and mobile (touchscreen)

#### How to Run:

```bash
cd /home/acca/Scrivania/C-project

# Option 1: Using build script
chmod +x build_gesture_demo.sh
./build_gesture_demo.sh

# Option 2: Manual build
gcc -Wall -Wextra -std=c11 \
  $(pkg-config --cflags gtk4 glib-2.0) \
  -I./include -c examples/gesture_demo.c -o build/gesture_demo.o

gcc -o build/gesture_demo \
  build/gesture_demo.o \
  build/utils/gesture_manager.o \
  $(pkg-config --libs gtk4 glib-2.0)

./build/gesture_demo
```

#### Demo Sections:

1. **🎯 TAP GESTURES**
   - Single Tap: Click once, counter increments
   - Double Tap: Quick double-click, widget pulses
   - Long Press: Hold 500ms, widget highlights

2. **👈 SWIPE GESTURES**
   - Swipe any direction (left, right, up, down)
   - Fast flick motion required
   - Directional feedback in real-time log

3. **👆 PAN GESTURE**
   - Click and drag slowly (vertical)
   - Continuous offset tracking
   - Useful for scrolling simulation

4. **🔍 PINCH & ROTATE**
   - Two-finger pinch = zoom in/out
   - Two-finger rotate = rotation
   - Visual canvas responds in real-time

5. **📋 EVENT LOG**
   - All gestures logged with timestamp
   - Shows gesture type and details
   - Last 20 events visible

#### On Desktop (Trackpad):

```
- Click = Single tap
- Double-click = Double tap
- 2-finger drag = Pan
- 2-finger swipe = Swipe
- 2-finger pinch = Pinch zoom
- 2-finger rotate = Rotation
```

#### On Mobile/Touchscreen:

```
- Tap = Single tap
- Double-tap = Double tap
- Hold 500ms = Long press
- Fast swipe = Swipe gesture
- Slow drag = Pan gesture
- 2-finger pinch = Pinch zoom
- 2-finger rotate = Rotation
```

### gesture_example.c

Complete code examples showing:

- **Gesture-enabled Listbox**: Single tap, long press, swipe delete
- **Image Viewer**: Pinch to zoom, two-finger rotation, double-tap reset
- **Text Input**: Long press select-all, double-tap word selection
- **Best Practices**: Proper cleanup and resource management

## How to Use These Examples

### 1. Listbox with Touch Gestures

```c
#include "utils/gesture_manager.h"

// Create gesture context
GestureExampleContext *ctx = g_new0(GestureExampleContext, 1);
ctx->model = your_model;

// Setup gestures
setup_listbox_gestures(your_listbox, ctx);

// Setup row gestures when adding rows
setup_row_gestures(row, ctx);

// Cleanup on destroy
gesture_remove_all(GTK_WIDGET(your_listbox));
```

### 2. Image Viewer with Pinch/Rotate

```c
ImageViewerContext *ctx = g_new0(ImageViewerContext, 1);
setup_image_viewer_gestures(canvas, ctx);

// Drawing code would use ctx->zoom_level and ctx->rotation_angle
```

### 3. Enhanced Text Input

```c
TextInputContext *ctx = g_new0(TextInputContext, 1);
setup_entry_gestures(entry, ctx);
```

## Integration with Your Project

### Step 1: Include the Header

```c
#include "utils/gesture_manager.h"
```

### Step 2: Add Gestures During Widget Setup

```c
void app_view_new(AppView *view) {
    // ... existing setup code ...

    // Add gesture support
    gesture_add_tap(view->add_button, on_add_button_tap, view);
    gesture_add_swipe_left(view->users_list, 100.0, on_list_delete, view);
}
```

### Step 3: Cleanup on Destroy

```c
void app_view_destroy(AppView *view) {
    // Remove all gestures
    gesture_remove_all(GTK_WIDGET(view->add_button));
    gesture_remove_all(GTK_WIDGET(view->users_list));

    // ... rest of cleanup ...
}
```

## Common Patterns

### Pattern 1: Action on Tap

```c
static void on_item_tap(GtkWidget *widget, gpointer user_data) {
    // Handle tap
}

gesture_add_tap(item_widget, on_item_tap, context);
```

### Pattern 2: Delete on Swipe

```c
static void on_swipe_delete(GtkWidget *widget, gpointer user_data) {
    // Delete item
}

gesture_add_swipe_left(item_widget, 100.0, on_swipe_delete, context);
```

### Pattern 3: Context Menu on Long Press

```c
static void on_long_press_menu(GtkWidget *widget, gpointer user_data) {
    // Show popover or context menu
}

gesture_add_long_press(widget, 500, on_long_press_menu, context);
```

### Pattern 4: Zoom with Pinch

```c
static void on_pinch_zoom(GtkWidget *widget, gdouble scale, gpointer data) {
    AppContext *ctx = (AppContext *)data;
    ctx->zoom *= scale;
    gtk_widget_queue_draw(widget);
}

gesture_add_pinch(canvas, on_pinch_zoom, context);
```

## Testing Gestures

### On Desktop with Trackpad

- Tap = Single click
- Double-tap = Double click
- Swipe = Two-finger trackpad swipe
- Pan = Two-finger trackpad drag
- Pinch = Two-finger pinch gesture
- Rotate = Two-finger rotation (if supported)

### On Touchscreen Device

- Tap = Single finger tap
- Double-tap = Quick double tap
- Swipe = Fast finger drag (creates velocity)
- Pan = Slow finger drag with continuous updates
- Pinch = Two-finger pinch gesture
- Rotate = Two-finger rotation

### Debug Output

```bash
# Enable debugging
G_MESSAGES_DEBUG=all ./build/gtkapp

# Or run with GTK debug flags
GTK_DEBUG=events ./build/gtkapp
```

## Performance Tips

1. **Reuse contexts**: Don't create new gesture handlers for every row
2. **Cleanup properly**: Always remove gestures before widget destroy
3. **Threshold tuning**: Adjust velocity thresholds based on your needs
4. **Test on real hardware**: Trackpad behavior differs from touchscreen

## Troubleshooting

### Gestures not triggering

- Ensure widget has `can-focus` property set
- Check that widget has proper event mask
- Verify callback signature matches expected type

### Conflicts between gestures

- Don't mix swipe and pan on same widget
- Use pan for continuous tracking, swipe for quick flicks
- Keep gesture thresholds appropriate

### Resource leaks

- Always call `gesture_remove_all()` before widget destroy
- Check for orphaned contexts in memory profiler

## References

- [GTK4 Gesture Documentation](https://docs.gtk.org/gtk4/class.Gesture.html)
- [Event Handling in GTK4](https://docs.gtk.org/gtk4/input_handling.html)
- [MVC Architecture with Gestures](../GESTURES.md)

## License

These examples are part of the C-GTK4 project and follow the same license.
