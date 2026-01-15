#include <gtk/gtk.h>

#include <gdk/gdkx.h>
#include <X11/Xlib.h>
#include <X11/Xatom.h>

void x11_overlay(GtkWidget *window)
{
    GdkDisplay *gdk_display = gdk_display_get_default();
    Display *dpy = GDK_DISPLAY_XDISPLAY(gdk_display);

    if (!GDK_IS_X11_DISPLAY(gdk_display))
    {
        g_error("Not running on X11!");
    }

    GdkWindow *gdk_win = gtk_widget_get_window(window);
    Window win = GDK_WINDOW_XID(gdk_win);

    Atom wm_state = XInternAtom(dpy, "_NET_WM_STATE", False);
    Atom above    = XInternAtom(dpy, "_NET_WM_STATE_ABOVE", False);

    XChangeProperty(
        dpy, win,
        wm_state,
        XA_ATOM,
        32,
        PropModeReplace,
        (unsigned char *)&above,
        1
    );
}

#include <string>
#include <fstream>

static const char* WINDOW_CFG = "window.cfg";
static const char* NOTE_TXT = "note.txt";

std::string load_from_file()
{
    std::ifstream file(NOTE_TXT);
    if (!file.is_open())
        return "";

    std::string content(
        (std::istreambuf_iterator<char>(file)),
        std::istreambuf_iterator<char>()
    );
    return content;
}

void load_window_geometry(GtkWidget* window)
{
    std::ifstream file("window.cfg");
    if (!file.is_open())
        return;

    int x, y, w, h;
    file >> x >> y >> w >> h;

    gtk_window_move(GTK_WINDOW(window), x, y);
    gtk_window_resize(GTK_WINDOW(window), w, h);
}

void note_text(GtkWidget *window, GtkWidget *text)
{
    GdkWindow *txt = gtk_widget_get_window(text);

    GtkTextBuffer *buf =
        gtk_text_view_get_buffer(GTK_TEXT_VIEW(text));

    GtkTextIter start, end;
    gtk_text_buffer_get_bounds(buf, &start, &end);

    char *content =
        gtk_text_buffer_get_text(buf, &start, &end, FALSE);

    std::ofstream file(NOTE_TXT);
    file << content;

    g_free(content);
}

void window_position(GtkWidget *window, GtkWidget *text)
{
    GdkWindow *win = gtk_widget_get_window(window);

    int wx, wy, ww, wh;
    GtkAllocation alloc;

    gdk_window_get_root_origin(win, &wx, &wy);
    gdk_window_get_geometry(win, NULL, NULL, &ww, &wh);

    std::ofstream file("window.cfg");
    file << wx << " " << wy << " " << ww << " " << wh;
}

static void on_delete(GtkWidget *window, gpointer user_data)
{
    GtkWidget *text = GTK_WIDGET(user_data);

    window_position(window, text);
    gtk_main_quit();
}

static void on_destroy(GtkWidget *window, gpointer user_data)
{
    GtkWidget *text = GTK_WIDGET(user_data);

    note_text(window, text);

    gtk_main_quit();
}

int main(int argc, char *argv[])
{
    setenv("GDK_BACKEND", "x11", 1);
    
    gtk_init(&argc, &argv);

    GtkWidget *window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_default_size(GTK_WINDOW(window), 200, 200);
    gtk_window_set_title(GTK_WINDOW(window), "Sticky Notes");
    gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER);
    gtk_window_set_skip_taskbar_hint(GTK_WINDOW(window), TRUE);
    gtk_window_set_skip_pager_hint(GTK_WINDOW(window), TRUE);
    gtk_window_stick(GTK_WINDOW(window));
    gtk_window_set_type_hint(GTK_WINDOW(window),
                             GDK_WINDOW_TYPE_HINT_UTILITY);

    GtkCssProvider *css = gtk_css_provider_new();
    gtk_css_provider_load_from_data(css,
        "window, textview, text { background-color: #fff59d; }",
        -1, NULL);

    gtk_style_context_add_provider_for_screen(
        gdk_screen_get_default(),
        GTK_STYLE_PROVIDER(css),
        GTK_STYLE_PROVIDER_PRIORITY_APPLICATION
    );

    GtkWidget *text = gtk_text_view_new();
    gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(text), GTK_WRAP_WORD);
    gtk_text_view_set_left_margin(GTK_TEXT_VIEW(text), 8);
    gtk_text_view_set_right_margin(GTK_TEXT_VIEW(text), 8);

    std::string loaded = load_from_file();
    if (!loaded.empty())
    {
        GtkTextBuffer* buffer =
        gtk_text_view_get_buffer(GTK_TEXT_VIEW(text));

        gtk_text_buffer_set_text(
            buffer,
            loaded.c_str(),
            -1
        );
    }

    gtk_container_add(GTK_CONTAINER(window), text);

    g_signal_connect(window, "delete-event",
                     G_CALLBACK(on_delete), NULL);

    g_signal_connect(window, "destroy",
                     G_CALLBACK(on_destroy), text);

    gtk_widget_show_all(window);

    x11_overlay(window);

    load_window_geometry(window);

    gtk_main();
}