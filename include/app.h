#include "gtk/gtk.h"
#include "signal_handlers.h"

/**
 * #include <gtk/gtk.h>

// Global builder
GtkBuilder *builder = NULL;

// Signal handlers
void on_submit_clicked(GtkButton *button, gpointer user_data) {
    GtkEntry *name_entry = GTK_ENTRY(gtk_builder_get_object(builder, "name_entry"));
    GtkEntry *email_entry = GTK_ENTRY(gtk_builder_get_object(builder, "email_entry"));
    GtkSpinButton *age_spin = GTK_SPIN_BUTTON(gtk_builder_get_object(builder, "age_spin"));
    GtkComboBoxText *gender_combo = GTK_COMBO_BOX_TEXT(gtk_builder_get_object(builder, "gender_combo"));
    GtkCheckButton *terms_check = GTK_CHECK_BUTTON(gtk_builder_get_object(builder, "terms_check"));
    GtkLabel *status_label = GTK_LABEL(gtk_builder_get_object(builder, "status_label"));
    
    const gchar *name = gtk_entry_get_text(name_entry);
    const gchar *email = gtk_entry_get_text(email_entry);
    gint age = gtk_spin_button_get_value_as_int(age_spin);
    gchar *gender = gtk_combo_box_text_get_active_text(gender_combo);
    gboolean terms_accepted = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(terms_check));
    
    // Validation
    if (strlen(name) == 0) {
        gtk_label_set_text(status_label, "❌ Please enter your name");
        return;
    }
    
    if (strlen(email) == 0) {
        gtk_label_set_text(status_label, "❌ Please enter your email");
        return;
    }
    
    if (!terms_accepted) {
        gtk_label_set_text(status_label, "❌ Please accept terms and conditions");
        return;
    }
    
    // Print data
    g_print("\n=== Registration Data ===\n");
    g_print("Name: %s\n", name);
    g_print("Email: %s\n", email);
    g_print("Age: %d\n", age);
    g_print("Gender: %s\n", gender ? gender : "Not selected");
    g_print("========================\n\n");
    
    gchar *success_msg = g_strdup_printf("✓ Registration successful! Welcome, %s", name);
    gtk_label_set_text(status_label, success_msg);
    g_free(success_msg);
    g_free(gender);
}

void on_clear_clicked(GtkButton *button, gpointer user_data) {
    GtkEntry *name_entry = GTK_ENTRY(gtk_builder_get_object(builder, "name_entry"));
    GtkEntry *email_entry = GTK_ENTRY(gtk_builder_get_object(builder, "email_entry"));
    GtkSpinButton *age_spin = GTK_SPIN_BUTTON(gtk_builder_get_object(builder, "age_spin"));
    GtkComboBoxText *gender_combo = GTK_COMBO_BOX_TEXT(gtk_builder_get_object(builder, "gender_combo"));
    GtkCheckButton *terms_check = GTK_CHECK_BUTTON(gtk_builder_get_object(builder, "terms_check"));
    GtkLabel *status_label = GTK_LABEL(gtk_builder_get_object(builder, "status_label"));
    
    gtk_entry_set_text(name_entry, "");
    gtk_entry_set_text(email_entry, "");
    gtk_spin_button_set_value(age_spin, 18);
    gtk_combo_box_set_active(GTK_COMBO_BOX(gender_combo), -1);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(terms_check), FALSE);
    gtk_label_set_text(status_label, "Form cleared");
    
    g_print("Form cleared\n");
}

void on_cancel_clicked(GtkButton *button, gpointer user_data) {
    g_print("Operation cancelled\n");
    GtkApplication *app = GTK_APPLICATION(user_data);
    g_application_quit(G_APPLICATION(app));
}

static void load_css() {
    GtkCssProvider *provider;
    GdkDisplay *display;
    GdkScreen *screen;
    
    provider = gtk_css_provider_new();
    display = gdk_display_get_default();
    screen = gdk_display_get_default_screen(display);
    
    gtk_style_context_add_provider_for_screen(
        screen,
        GTK_STYLE_PROVIDER(provider),
        GTK_STYLE_PROVIDER_PRIORITY_APPLICATION
    );
    
    GError *error = NULL;
    gtk_css_provider_load_from_path(provider, "style.css", &error);
    
    if (error) {
        g_printerr("Error loading CSS: %s\n", error->message);
        g_clear_error(&error);
    }
    
    g_object_unref(provider);
}

static void activate(GtkApplication *app, gpointer user_data) {
    GtkWidget *window;
    GError *error = NULL;
    
    // Load CSS first
    load_css();
    
    // Create builder
    builder = gtk_builder_new();
    
    // Load UI from Glade file
    if (!gtk_builder_add_from_file(builder, "interface.glade", &error)) {
        g_printerr("Error loading Glade file: %s\n", error->message);
        g_clear_error(&error);
        return;
    }
    
    // Get main window
    window = GTK_WIDGET(gtk_builder_get_object(builder, "main_window"));
    
    if (!window) {
        g_printerr("Error: Could not find main_window in Glade file\n");
        return;
    }
    
    // Set the window's application
    gtk_window_set_application(GTK_WINDOW(window), app);
    
    // Connect signals - pass app to handlers that need it
    GtkButton *cancel_btn = GTK_BUTTON(gtk_builder_get_object(builder, "cancel_button"));
    g_signal_connect(cancel_btn, "clicked", G_CALLBACK(on_cancel_clicked), app);
    
    // Connect other signals
    gtk_builder_connect_signals(builder, NULL);
    
    // Show window
    gtk_widget_show_all(window);
}

int main(int argc, char **argv) {
    GtkApplication *app;
    int status;
    
    app = gtk_application_new("com.example.gladeapp", G_APPLICATION_DEFAULT_FLAGS);
    g_signal_connect(app, "activate", G_CALLBACK(activate), NULL);
    status = g_application_run(G_APPLICATION(app), argc, argv);
    g_object_unref(app);
    
    return status;
}
 */

 /**
  * <?xml version="1.0" encoding="UTF-8"?>
<interface>
  <requires lib="gtk+" version="3.20"/>
  
  <object class="GtkApplicationWindow" id="main_window">
    <property name="can_focus">False</property>
    <property name="title" translatable="yes">User Registration</property>
    <property name="default_width">600</property>
    <property name="default_height">400</property>
    
    <child>
      <object class="GtkBox" id="main_box">
        <property name="visible">True</property>
        <property name="orientation">vertical</property>
        
        <child>
          <object class="GtkLabel" id="header_label">
            <property name="visible">True</property>
            <property name="label">User Registration</property>
            <property name="height_request">60</property>
            <style>
              <class name="header"/>
            </style>
          </object>
        </child>
        
        <child>
          <object class="GtkBox" id="card_box">
            <property name="visible">True</property>
            <property name="orientation">vertical</property>
            <property name="spacing">15</property>
            <property name="margin">20</property>
            <style>
              <class name="card"/>
            </style>
            
            <child>
              <object class="GtkEntry" id="name_entry">
                <property name="visible">True</property>
                <property name="placeholder_text">Enter your name...</property>
                <property name="margin">10</property>
              </object>
            </child>
            
            <child>
              <object class="GtkEntry" id="email_entry">
                <property name="visible">True</property>
                <property name="placeholder_text">Enter your email...</property>
                <property name="margin">10</property>
              </object>
            </child>
            
            <child>
              <object class="GtkBox">
                <property name="visible">True</property>
                <property name="orientation">horizontal</property>
                <property name="spacing">10</property>
                <property name="margin">10</property>
                
                <child>
                  <object class="GtkLabel">
                    <property name="visible">True</property>
                    <property name="label">Age:</property>
                  </object>
                </child>
                
                <child>
                  <object class="GtkSpinButton" id="age_spin">
                    <property name="visible">True</property>
                    <property name="adjustment">age_adjustment</property>
                  </object>
                </child>
              </object>
            </child>
            
            <child>
              <object class="GtkBox">
                <property name="visible">True</property>
                <property name="orientation">horizontal</property>
                <property name="spacing">10</property>
                <property name="margin">10</property>
                
                <child>
                  <object class="GtkLabel">
                    <property name="visible">True</property>
                    <property name="label">Gender:</property>
                  </object>
                </child>
                
                <child>
                  <object class="GtkComboBoxText" id="gender_combo">
                    <property name="visible">True</property>
                    <items>
                      <item>Male</item>
                      <item>Female</item>
                      <item>Other</item>
                    </items>
                  </object>
                </child>
              </object>
            </child>
            
            <child>
              <object class="GtkCheckButton" id="terms_check">
                <property name="visible">True</property>
                <property name="label">I agree to the terms and conditions</property>
                <property name="margin">10</property>
              </object>
            </child>
            
            <child>
              <object class="GtkLabel" id="status_label">
                <property name="visible">True</property>
                <property name="label"></property>
                <property name="margin">10</property>
                <style>
                  <class name="status"/>
                </style>
              </object>
            </child>
            
            <child>
              <object class="GtkBox" id="button_box">
                <property name="visible">True</property>
                <property name="orientation">horizontal</property>
                <property name="spacing">10</property>
                <property name="halign">center</property>
                <property name="margin">20</property>
                
                <child>
                  <object class="GtkButton" id="submit_button">
                    <property name="visible">True</property>
                    <property name="label">Submit</property>
                    <property name="width_request">120</property>
                    <signal name="clicked" handler="on_submit_clicked"/>
                    <style>
                      <class name="success"/>
                    </style>
                  </object>
                </child>
                
                <child>
                  <object class="GtkButton" id="clear_button">
                    <property name="visible">True</property>
                    <property name="label">Clear</property>
                    <property name="width_request">120</property>
                    <signal name="clicked" handler="on_clear_clicked"/>
                    <style>
                      <class name="warning"/>
                    </style>
                  </object>
                </child>
                
                <child>
                  <object class="GtkButton" id="cancel_button">
                    <property name="visible">True</property>
                    <property name="label">Cancel</property>
                    <property name="width_request">120</property>
                    <style>
                      <class name="danger"/>
                    </style>
                  </object>
                </child>
              </object>
            </child>
          </object>
        </child>
      </object>
    </child>
  </object>
  
  <object class="GtkAdjustment" id="age_adjustment">
    <property name="lower">1</property>
    <property name="upper">150</property>
    <property name="value">18</property>
    <property name="step_increment">1</property>
    <property name="page_increment">10</property>
  </object>
</interface>
  */

/**
 * <?xml version="1.0" encoding="UTF-8"?>
<interface>
  <requires lib="gtk+" version="3.20"/>
  
  <!-- Main Window -->
  <object class="GtkApplicationWindow" id="main_window">
    <property name="title">Main Window</property>
    <property name="default_width">500</property>
    <property name="default_height">400</property>
    <child>
      <object class="GtkBox">
        <property name="visible">True</property>
        <property name="orientation">vertical</property>
        <property name="spacing">20</property>
        <property name="margin">30</property>
        
        <child>
          <object class="GtkLabel">
            <property name="visible">True</property>
            <property name="label">Main Application Window</property>
            <property name="justify">center</property>
            <style>
              <class name="header"/>
            </style>
          </object>
        </child>
        
        <child>
          <object class="GtkButton" id="open_settings_button">
            <property name="visible">True</property>
            <property name="label">Open Settings</property>
            <signal name="clicked" handler="on_open_settings"/>
            <style>
              <class name="success"/>
            </style>
          </object>
        </child>
        
        <child>
          <object class="GtkButton" id="open_about_button">
            <property name="visible">True</property>
            <property name="label">About</property>
            <signal name="clicked" handler="on_open_about"/>
            <style>
              <class name="warning"/>
            </style>
          </object>
        </child>
      </object>
    </child>
  </object>
  
  <!-- Settings Window -->
  <object class="GtkWindow" id="settings_window">
    <property name="title">Settings</property>
    <property name="default_width">400</property>
    <property name="default_height">300</property>
    <property name="modal">True</property>
    <child>
      <object class="GtkBox">
        <property name="visible">True</property>
        <property name="orientation">vertical</property>
        <property name="spacing">15</property>
        <property name="margin">20</property>
        
        <child>
          <object class="GtkLabel">
            <property name="visible">True</property>
            <property name="label">Application Settings</property>
            <style>
              <class name="header"/>
            </style>
          </object>
        </child>
        
        <child>
          <object class="GtkCheckButton" id="dark_mode_check">
            <property name="visible">True</property>
            <property name="label">Enable Dark Mode</property>
          </object>
        </child>
        
        <child>
          <object class="GtkCheckButton" id="notifications_check">
            <property name="visible">True</property>
            <property name="label">Enable Notifications</property>
          </object>
        </child>
        
        <child>
          <object class="GtkButton" id="close_settings_button">
            <property name="visible">True</property>
            <property name="label">Close</property>
            <signal name="clicked" handler="on_close_settings"/>
            <style>
              <class name="danger"/>
            </style>
          </object>
        </child>
      </object>
    </child>
  </object>
</interface>
 */

/**
 * window {
    background: #ecf0f1;
}

.header {
    background: linear-gradient(to right, #667eea, #764ba2);
    color: white;
    font-size: 28px;
    font-weight: bold;
    padding: 20px;
}

.card {
    background: white;
    border-radius: 15px;
    box-shadow: 0 4px 6px rgba(0,0,0,0.1);
    margin: 20px;
    padding: 20px;
}

button.success {
    background: #2ecc71;
    color: white;
    border-radius: 8px;
    padding: 12px 24px;
    font-size: 14px;
    border: none;
    min-width: 120px;
}

button.success:hover {
    background: #27ae60;
}

button.warning {
    background: #f39c12;
    color: white;
    border-radius: 8px;
    padding: 12px 24px;
    font-size: 14px;
    border: none;
    min-width: 120px;
}

button.warning:hover {
    background: #e67e22;
}

button.danger {
    background: #e74c3c;
    color: white;
    border-radius: 8px;
    padding: 12px 24px;
    font-size: 14px;
    border: none;
    min-width: 120px;
}

button.danger:hover {
    background: #c0392b;
}

entry {
    border: 2px solid #3498db;
    border-radius: 5px;
    padding: 10px;
    font-size: 14px;
    min-height: 40px;
}

entry:focus {
    border-color: #2980b9;
}

.status {
    color: #2ecc71;
    font-size: 14px;
    font-weight: bold;
}

combobox button {
    border: 2px solid #3498db;
    border-radius: 5px;
    padding: 10px;
}

spinbutton {
    border: 2px solid #3498db;
    border-radius: 5px;
}

checkbutton {
    font-size: 14px;
}
 */