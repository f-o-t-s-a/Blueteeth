#include <gtk/gtk.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Définition d'une structure pour stocker les informations de chaque utilisateur
typedef struct {
    int priorite;             // 0=normal ; 1=famille ; 2=PMR
    int nombre_de_sigesV;     // Nombre de sièges demandés
    int est_servi;            // 0 si non servi, 1 si servi
} Utilisateur;

// Variables globales pour l'interface et la logique
#define MAX_UTILISATEURS 3
Utilisateur utilisateurs[MAX_UTILISATEURS];
int utilisateur_courant = 0;
int taille_taxi = 4; // Taille maximale du taxi
int position_allocation = 0; // Position pour l'affichage des sièges alloués

// Widgets GTK+
GtkWindow *main_window;
GtkLabel *status_label;
GtkLabel *seats_display_label;
GtkComboBoxText *priorite_combo;
GtkSpinButton *seats_spinbutton;
GtkButton *add_user_button;
GtkButton *allocate_button;
GtkTextView *log_view;
GtkTextBuffer *log_buffer;
GtkWidget *seat_images[4]; // Pour afficher l'état des sièges
GtkWidget *seat_grid; // Grille pour les images des sièges

// Fonctions d'aide pour l'interface
void append_to_log(const char *text) {
    GtkTextIter iter;
    gtk_text_buffer_get_end_iter(log_buffer, &iter);
    gtk_text_buffer_insert(log_buffer, &iter, text, -1);
    gtk_text_buffer_insert(log_buffer, &iter, "\n", -1);
}

void update_seats_display() {
    char buffer[100];
    sprintf(buffer, "Sièges disponibles: %d", taille_taxi);
    gtk_label_set_text(status_label, buffer);

    // Mettre à jour l'affichage visuel des sièges
    for (int i = 0; i < 4; i++) {
        if (i < position_allocation) {
            // Siège alloué
            gtk_image_set_from_icon_name(GTK_IMAGE(seat_images[i]), "emblem-ok-symbolic", GTK_ICON_SIZE_LARGE_TOOLBAR);
        } else {
            // Siège libre
            gtk_image_set_from_icon_name(GTK_IMAGE(seat_images[i]), "checkbox-symbolic", GTK_ICON_SIZE_LARGE_TOOLBAR);
        }
    }
}

// Handler pour le bouton "Enregistrer la Demande"
void on_add_user_clicked(GtkButton *button, gpointer user_data) {
    if (utilisateur_courant >= MAX_UTILISATEURS) {
        append_to_log("Max. utilisateurs atteints. Ne peut plus enregistrer.");
        gtk_widget_set_sensitive(GTK_WIDGET(add_user_button), FALSE);
        return;
    }

    int priorite = gtk_combo_box_get_active(GTK_COMBO_BOX(priorite_combo));
    int nombre_de_sigesV = gtk_spin_button_get_value_as_int(seats_spinbutton);

    if (nombre_de_sigesV <= 0 || nombre_de_sigesV > 4) {
        append_to_log("Erreur: Nombre de sièges demandé invalide (1-4).");
        return;
    }

    utilisateurs[utilisateur_courant].priorite = priorite;
    utilisateurs[utilisateur_courant].nombre_de_sigesV = nombre_de_sigesV;
    utilisateurs[utilisateur_courant].est_servi = 0;

    char log_entry[200];
    sprintf(log_entry, "Utilisateur %d enregistré: Priorité %d, Sièges %d",
            utilisateur_courant + 1, priorite, nombre_de_sigesV);
    append_to_log(log_entry);

    utilisateur_courant++;

    if (utilisateur_courant >= MAX_UTILISATEURS) {
        append_to_log("Nombre maximum d'utilisateurs enregistré.");
        gtk_widget_set_sensitive(GTK_WIDGET(add_user_button), FALSE);
        gtk_widget_set_sensitive(GTK_WIDGET(allocate_button), TRUE);
    } else {
        sprintf(log_entry, "Enregistrez encore %d utilisateur(s).", MAX_UTILISATEURS - utilisateur_courant);
        append_to_log(log_entry);
    }
}

// Handler pour le bouton "Lancer l'Allocation"
void on_allocate_clicked(GtkButton *button, gpointer user_data) {
    append_to_log("\n--- DEBUT DE L'ALLOCATION DES SIÈGES PAR PRIORITÉ ---\n");
    gtk_widget_set_sensitive(GTK_WIDGET(allocate_button), FALSE);

    // Boucle pour servir les utilisateurs par ordre de priorité (2=PMR, 1=Famille, 0=Normal)
    for (int p = 2; p >= 0; p--) {
        for (int u = 0; u < utilisateur_courant; u++) {
            if (utilisateurs[u].priorite == p && utilisateurs[u].est_servi == 0) {
                int demande = utilisateurs[u].nombre_de_sigesV;
                char log_entry[200];

                if (demande > taille_taxi) {
                    sprintf(log_entry, "Utilisateur %d (Priorité %d): Demande de %d sièges ne peut être satisfaite (reste %d).",
                            u + 1, p, demande, taille_taxi);
                    append_to_log(log_entry);
                    append_to_log("Désolé, votre demande dépasse la capacité actuelle.");
                } else {
                    sprintf(log_entry, "*** Allocation pour Utilisateur %d (Priorité %d) : Demande de %d sièges. ***",
                            u + 1, p, demande);
                    append_to_log(log_entry);

                    for(int i = 0; i < demande; i++) {
                        sprintf(log_entry, "   -> Siège %d alloué.", position_allocation + 1);
                        append_to_log(log_entry);
                        position_allocation++;
                    }

                    taille_taxi -= demande;
                    utilisateurs[u].est_servi = 1;

                    sprintf(log_entry, "Il reste %d siège(s) disponible(s).", taille_taxi);
                    append_to_log(log_entry);

                    update_seats_display();

                    if (taille_taxi == 0) {
                        append_to_log("\n!!! VÉHICULE PLEIN -> DÉPART IMMÉDIAT DU TAXI. !!!\n");
                        gtk_widget_set_sensitive(GTK_WIDGET(add_user_button), FALSE);
                        gtk_widget_set_sensitive(GTK_WIDGET(allocate_button), FALSE);
                        return;
                    }
                }
            }
        }
    }

    append_to_log("\n--- FIN DE L'ALLOCATION ---");
    if (taille_taxi > 0) {
        char log_entry[100];
        sprintf(log_entry, "Le taxi n'est pas plein. Il reste %d siège(s) disponible(s).", taille_taxi);
        append_to_log(log_entry);
    }
}

// Fonction principale de l'application GTK+
void activate(GtkApplication *app, gpointer user_data) {
    main_window = GTK_WINDOW(gtk_application_window_new(app));
    gtk_window_set_title(main_window, "Gestion des Sièges Taxi");
    gtk_window_set_default_size(main_window, 600, 700);

    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_container_add(GTK_CONTAINER(main_window), vbox);  // GTK 3
    gtk_box_set_homogeneous(GTK_BOX(vbox), FALSE);

    // --- Section d'enregistrement des utilisateurs ---
    GtkWidget *frame_entry = gtk_frame_new("Enregistrement des Demandes");
    gtk_box_pack_start(GTK_BOX(vbox), frame_entry, FALSE, FALSE, 0);  // GTK 3

    GtkWidget *grid_entry = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(grid_entry), 5);
    gtk_grid_set_column_spacing(GTK_GRID(grid_entry), 5);
    gtk_container_add(GTK_CONTAINER(frame_entry), grid_entry);  // GTK 3
    gtk_widget_set_margin_start(grid_entry, 10);
    gtk_widget_set_margin_end(grid_entry, 10);
    gtk_widget_set_margin_top(grid_entry, 10);
    gtk_widget_set_margin_bottom(grid_entry, 10);

    // Priorité
    gtk_grid_attach(GTK_GRID(grid_entry), gtk_label_new("Priorité :"), 0, 0, 1, 1);
    priorite_combo = GTK_COMBO_BOX_TEXT(gtk_combo_box_text_new());
    gtk_combo_box_text_append_text(priorite_combo, "0: Normal");
    gtk_combo_box_text_append_text(priorite_combo, "1: Famille");
    gtk_combo_box_text_append_text(priorite_combo, "2: PMR (Personne à Mobilité Réduite)");
    gtk_combo_box_set_active(GTK_COMBO_BOX(priorite_combo), 0);
    gtk_grid_attach(GTK_GRID(grid_entry), GTK_WIDGET(priorite_combo), 1, 0, 1, 1);

    // Nombre de sièges
    gtk_grid_attach(GTK_GRID(grid_entry), gtk_label_new("Sièges demandés (1-4) :"), 0, 1, 1, 1);
    seats_spinbutton = GTK_SPIN_BUTTON(gtk_spin_button_new_with_range(1, 4, 1));
    gtk_spin_button_set_value(seats_spinbutton, 1);
    gtk_grid_attach(GTK_GRID(grid_entry), GTK_WIDGET(seats_spinbutton), 1, 1, 1, 1);

    // Bouton d'ajout
    add_user_button = GTK_BUTTON(gtk_button_new_with_label("Enregistrer la Demande"));
    g_signal_connect(add_user_button, "clicked", G_CALLBACK(on_add_user_clicked), NULL);
    gtk_grid_attach(GTK_GRID(grid_entry), GTK_WIDGET(add_user_button), 0, 2, 2, 1);

    // Bouton d'allocation
    allocate_button = GTK_BUTTON(gtk_button_new_with_label("Lancer l'Allocation"));
    g_signal_connect(allocate_button, "clicked", G_CALLBACK(on_allocate_clicked), NULL);
    gtk_widget_set_sensitive(GTK_WIDGET(allocate_button), FALSE);
    gtk_box_pack_start(GTK_BOX(vbox), GTK_WIDGET(allocate_button), FALSE, FALSE, 0);  // GTK 3

    // --- Section Affichage des sièges du taxi ---
    GtkWidget *frame_seats = gtk_frame_new("État actuel du Taxi");
    gtk_box_pack_start(GTK_BOX(vbox), frame_seats, FALSE, FALSE, 0);  // GTK 3

    GtkWidget *vbox_seats = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_container_add(GTK_CONTAINER(frame_seats), vbox_seats);  // GTK 3

    status_label = GTK_LABEL(gtk_label_new("Sièges disponibles: 4"));
    gtk_box_pack_start(GTK_BOX(vbox_seats), GTK_WIDGET(status_label), FALSE, FALSE, 0);  // GTK 3

    // Grille pour les images des sièges
    seat_grid = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(seat_grid), 5);
    gtk_grid_set_column_spacing(GTK_GRID(seat_grid), 5);
    gtk_box_pack_start(GTK_BOX(vbox_seats), seat_grid, FALSE, FALSE, 0);  // GTK 3

    for (int i = 0; i < 4; i++) {
        seat_images[i] = gtk_image_new_from_icon_name("checkbox-symbolic", GTK_ICON_SIZE_LARGE_TOOLBAR);
        gtk_grid_attach(GTK_GRID(seat_grid), seat_images[i], i, 0, 1, 1);
    }
    update_seats_display();

    // --- Section Log de l'application ---
    GtkWidget *frame_log = gtk_frame_new("Journal d'Allocation");
    gtk_box_pack_start(GTK_BOX(vbox), frame_log, TRUE, TRUE, 0);  // GTK 3

    log_view = GTK_TEXT_VIEW(gtk_text_view_new());
    gtk_text_view_set_editable(log_view, FALSE);
    gtk_text_view_set_cursor_visible(log_view, FALSE);
    log_buffer = gtk_text_view_get_buffer(log_view);

    GtkWidget *scrolled_window = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(scrolled_window), GTK_SHADOW_IN);  // GTK 3
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window),
                                   GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_container_add(GTK_CONTAINER(scrolled_window), GTK_WIDGET(log_view));  // GTK 3
    gtk_container_add(GTK_CONTAINER(frame_log), scrolled_window);  // GTK 3

    append_to_log("Bienvenue dans le gestionnaire de sièges de taxi !");
    append_to_log("Enregistrez jusqu'à 3 utilisateurs.");

    gtk_widget_show_all(GTK_WIDGET(main_window));  // GTK 3
}

int main(int argc, char **argv) {
    GtkApplication *app;
    int status;

    app = gtk_application_new("org.gtk.taxi_allocator", G_APPLICATION_FLAGS_NONE);  // Updated flag
    g_signal_connect(app, "activate", G_CALLBACK(activate), NULL);
    status = g_application_run(G_APPLICATION(app), argc, argv);
    g_object_unref(app);

    return status;
}
