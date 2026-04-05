/* test application that demonstrates _NET_WM_FULLSCREEN_MONITORS 
 *
 * how to run it:
 * G_MESSAGES_DEBUG=all ./wm_fsm_test
 */

#include <gtk/gtk.h>
#include <gdk/gdkx.h>
#include <string.h>


static GtkWidget* window;

static gboolean print_final_size(gpointer data)
{
   (void)data;
   gint w, h;
   gtk_window_get_size(GTK_WINDOW(window), &w, &h);
   g_debug("final window size: %dx%d", w, h);
   return FALSE;
}

static gboolean on_configure_after_unfullscreen(GtkWidget *widget, GdkEventConfigure *event, gpointer data)
{
   (void)event;
   (void)data;
   g_signal_handlers_disconnect_by_func(widget, on_configure_after_unfullscreen, data);
   g_idle_add(print_final_size, NULL);
   return FALSE;
}

static gboolean fullscreen(gpointer data)
{
   (void)data;
   g_debug("fullscreen()");
   gtk_window_fullscreen(GTK_WINDOW(window));
   return FALSE;
}

static gboolean on_window_state_event(GtkWidget *widget, GdkEventWindowState *event, gpointer data)
{
   (void)widget;
   (void)data;
   if ((event->changed_mask & GDK_WINDOW_STATE_FULLSCREEN) &&
       !(event->new_window_state & GDK_WINDOW_STATE_FULLSCREEN)) {
      /* Force geometry back */
      gtk_window_move(GTK_WINDOW(window), 0, 0);
      gtk_window_resize(GTK_WINDOW(window), 800, 600);
      g_signal_connect(window, "configure-event", G_CALLBACK(on_configure_after_unfullscreen), NULL);
   }
   return FALSE;
}

static gboolean unfullscreen(gpointer data)
{
   (void)data;
   g_debug("unfullscreen()");
   gtk_window_unfullscreen(GTK_WINDOW(window));
   return FALSE;
}

static gboolean switch_monitors(gpointer data)
{
   static const long monitors[4][4] = {
      {0, 0, 0, 1},
      {0, 0, 0, 0},
      {1, 1, 1, 1},
      {0, 0, 0, 1}
   };

   static const char* desc[4] = {
       "Window should be covering both heads 1 and 2\n",
       "Window should be covering just the first head\n",
       "Window should be covering just the second head\n",
       "Window should be covering both heads 1 and 2\n"
   };


   guint index = GPOINTER_TO_UINT(data);

   g_debug("%s", desc[index]);

   GdkDisplay *display = gdk_display_get_default();
   GdkWindow *gwin = gtk_widget_get_window(window);
   if (!gwin) {
      g_warning("switch_monitors: window not realized yet");
      return FALSE;
   }
   XClientMessageEvent xclient;

   memset(&xclient, 0, sizeof(xclient));
   xclient.type = ClientMessage;
   xclient.window = GDK_WINDOW_XID(gwin);
   xclient.message_type = gdk_x11_get_xatom_by_name_for_display(display, "_NET_WM_FULLSCREEN_MONITORS");
   xclient.format = 32;
   xclient.data.l[0] = monitors[index][0];
   xclient.data.l[1] = monitors[index][1];
   xclient.data.l[2] = monitors[index][2];
   xclient.data.l[3] = monitors[index][3];
   xclient.data.l[4] = 1;

   XSendEvent(GDK_WINDOW_XDISPLAY(gwin),
              GDK_WINDOW_XID(gdk_get_default_root_window()),
              False,
              SubstructureRedirectMask | SubstructureNotifyMask,
              (XEvent *) &xclient);

   return FALSE;
}

static gboolean quit(gpointer data)
{
   (void)data;
   gtk_main_quit();
   return FALSE;
}

int main(int argc, char** argv)
{
   gtk_init(&argc, &argv);

   window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
   gtk_window_set_default_size(GTK_WINDOW(window), 800, 600);
   gtk_widget_show(window);
   g_signal_connect(window, "window-state-event", G_CALLBACK(on_window_state_event), NULL);

   g_timeout_add(1000, (GSourceFunc) fullscreen, NULL);
   g_timeout_add(5000, (GSourceFunc) switch_monitors, GUINT_TO_POINTER(0));
   g_timeout_add(10000, (GSourceFunc) switch_monitors, GUINT_TO_POINTER(1));
   g_timeout_add(15000, (GSourceFunc) switch_monitors, GUINT_TO_POINTER(2));
   g_timeout_add(20000, (GSourceFunc) switch_monitors, GUINT_TO_POINTER(3));
   g_timeout_add(25000, (GSourceFunc) unfullscreen, NULL);
   g_timeout_add(30000, (GSourceFunc) quit, NULL);

   gtk_main();
}
