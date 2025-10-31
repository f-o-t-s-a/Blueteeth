#include "gtk/gtk.h"

GtkBuilder* builder;

static void load_css() {
	GtkCssProvider* provider;
	GdkDisplay* display;
	GdkScreen* screen;

	provider = gtk_css_provider_new();
	display = gdk_display_get_default();
	screen = gdk_display_get_default_screen(display);

	gtk_style_context_add_provider_for_screen(
		screen,
		GTK_STYLE_PROVIDER(provider),
		GTK_STYLE_PROVIDER_PRIORITY_APPLICATION
	);

	gtk_css_provider_load_from_path(provider, "./css/styles.css", NULL);
	g_object_unref(provider);
}

static void on_button_clicked(GtkWidget* button, gpointer data) {
	g_print("Turn on to Connect !!..");
}

static void activate(GtkApplication* app, gpointer user_data) {
	GtkWidget* window;
	GError* error = NULL;

	load_css();

	builder = gtk_builder_new();
	if (!gtk_builder_add_from_file(builder, "./assets/main.glade", &error)) {
		g_warning("Could not load UI file: %s ..", error->message);
		g_clear_error(&error);
		return;
	}

	window = GTK_WIDGET(gtk_builder_get_object(builder, "wnd_primary"));
	gtk_window_set_application(GTK_WINDOW(window), app);
	gtk_builder_connect_signals(builder, NULL);

	gtk_widget_show_all(window);
}

int main(int argc, char** argv) {
	GtkApplication* app;
	int status;

	app = gtk_application_new("com.example.app", G_APPLICATION_DEFAULT_FLAGS);
	g_signal_connect(app, "activate", G_CALLBACK(activate), NULL);
	status = g_application_run(G_APPLICATION(app), argc, argv);
	g_object_unref(app);

	return status;
}
