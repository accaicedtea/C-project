# GTK4 C Application - MVC Architecture

Un'applicazione C con interfaccia GTK4 organizzata secondo l'architettura **Model-View-Controller (MVC)** con UI design separato in file `.ui`.

## 📁 Struttura del Progetto

```
C-project/
├── include/                    # Header files
│   ├── model/
│   │   └── app_model.h        # Struct e funzioni per i dati
│   ├── view/
│   │   └── app_view.h         # Interfaccia GTK4
│   ├── controller/
│   │   └── app_controller.h   # Logica applicazione
│   └── utils/
│       └── db_connection.h    # Connessione PostgreSQL
│
├── src/                        # Source files (specchio di include/)
│   ├── model/
│   │   └── app_model.c
│   ├── view/
│   │   └── app_view.c
│   ├── controller/
│   │   └── app_controller.c
│   ├── utils/
│   │   └── db_connection.c
│   └── main.c                  # Entry point
│
├── ui/                         # File UI (GtkBuilder XML)
│   └── main_window.ui
├── resources/                  # Risorse (icone, etc.)
├── build/                      # Build output (generato)
├── .vscode/                    # Configurazione VSCode
│   ├── c_cpp_properties.json   # Include paths per IntelliSense
│   ├── launch.json             # Debug configuration
│   ├── settings.json           # Workspace settings
│   └── tasks.json              # Build tasks
├── Makefile                    # Build automation
├── .gitignore
└── README.md
```

## 🏗️ Architettura MVC

### **Model** (`include/model/` + `src/model/`)

Gestisce i dati dell'applicazione, indipendente da GTK.

**File:** `app_model.h` / `app_model.c`

```c
typedef struct {
  gint id;
  gchar *name;
  gchar *email;
} User;

typedef struct {
  GPtrArray *users;
} AppModel;
```

**Funzioni:**

- `AppModel *app_model_new()` - Crea nuovo modello
- `void app_model_add_user()` - Aggiunge utente
- `GPtrArray *app_model_get_users()` - Ottiene lista utenti
- `void app_model_remove_user()` - Rimuove utente

### **View** (`include/view/` + `src/view/`)

Interfaccia grafica con GTK4, carica UI da file `.ui`.

**File:** `app_view.h` / `app_view.c`

```c
typedef struct {
  GtkApplicationWindow *main_window;
  GtkEntry *name_entry;
  GtkEntry *email_entry;
  GtkButton *add_button;
  GtkListBox *users_list;
} AppView;
```

**Funzioni:**

- `AppView *app_view_new()` - Carica UI da file
- `void app_view_display_users()` - Aggiorna lista UI
- `gchar *app_view_get_name_input()` - Legge input

### **Controller** (`include/controller/` + `src/controller/`)

Coordinamento tra Model e View, logica applicazione.

**File:** `app_controller.h` / `app_controller.c`

```c
typedef struct {
  AppModel *model;
  AppView *view;
  DBConnection *db;
} AppController;
```

**Funzioni:**

- `AppController *app_controller_new()` - Inizializza MVC
- `void app_controller_add_user()` - Gestisce aggiunta utente
- `void app_controller_refresh_ui()` - Aggiorna interfaccia

### **Utils** (`include/utils/` + `src/utils/`)

Funzioni di supporto, connessione database, gesture handling per touchscreen.

**File:**

- `db_connection.h` / `db_connection.c` - Connessione PostgreSQL
- `gesture_manager.h` / `gesture_manager.c` - Touch gesture handlers (tap, swipe, pan, pinch, rotate)

**Gestures supportati:**

- **Tap**: Single tap, double tap, long press
- **Swipe**: Left, right, up, down (con velocity detection)
- **Pan/Drag**: Horizontal e vertical panning
- **Pinch**: Zoom in/out con due dita
- **Rotate**: Rotazione con due dita

Vedi [GESTURES.md](GESTURES.md) per dettagli completi e [examples/gesture_example.c](examples/gesture_example.c) per esempi.

## 🛠️ Compilazione e Esecuzione

### Prerequisiti

```bash
# Ubuntu/Debian
sudo apt-get install -y build-essential pkg-config libgtk-4-dev libpq-dev

# Fedora
sudo dnf install -y gcc pkg-config gtk4-devel libpq-devel

# Arch
sudo pacman -S base-devel gtk4 postgresql-libs
```

### Comandi Make

| Comando       | Descrizione                    |
| ------------- | ------------------------------ |
| `make build`  | Compila l'applicazione         |
| `make run`    | Compila ed esegue              |
| `make clean`  | Rimuove build artifacts        |
| `make format` | Formatta codice (clang-format) |
| `make lint`   | Analizza codice (clang-tidy)   |
| `make test`   | Esegue test base               |
| `make help`   | Mostra aiuto                   |

### Esempio

```bash
cd /home/acca/Scrivania/C-project

# Compilare
make build
# Output: Build successful: build/gtkapp

# Eseguire
make run

# Ricompilare pulito
make clean && make build

# Formattare codice
make format
```

## 📝 File UI (.ui)

L'interfaccia è definita in `ui/main_window.ui` in formato XML (GtkBuilder):

```xml
<?xml version="1.0" encoding="UTF-8"?>
<interface>
  <object class="GtkApplicationWindow" id="main_window">
    <property name="title">GTK App</property>
    <property name="default-width">600</property>
    <property name="default-height">400</property>
  </object>
</interface>
```

Viene caricata in `app_view.c`:

```c
GtkBuilder *builder = gtk_builder_new();
gtk_builder_add_from_file(builder, "ui/main_window.ui", &error);
view->main_window = GTK_APPLICATION_WINDOW(
  gtk_builder_get_object(builder, "main_window")
);
```

## 🔧 Configurazione VSCode

Il progetto include configurazione completa per VSCode:

### Include Paths (c_cpp_properties.json)

```json
"includePath": [
  "${workspaceFolder}/include",
  "${workspaceFolder}/include/model",
  "${workspaceFolder}/include/view",
  "${workspaceFolder}/include/controller",
  "${workspaceFolder}/include/utils",
  "/usr/include/gtk-4.0",
  "/usr/include/glib-2.0",
  "/usr/include/postgresql"
]
```

### IntelliSense

Gli include sono automaticamente risolti:

```c
#include "model/app_model.h"       ✓ Trovato
#include "view/app_view.h"         ✓ Trovato
#include "controller/app_controller.h" ✓ Trovato
#include "utils/db_connection.h"   ✓ Trovato
```

### Debug (F5)

Configurato automaticamente in `launch.json`:

```json
{
  "version": "0.2.0",
  "configurations": [
    {
      "name": "Debug GtkApp",
      "type": "cppdbg",
      "request": "launch",
      "program": "${workspaceFolder}/build/gtkapp",
      "preLaunchTask": "build"
    }
  ]
}
```

## 🐛 Debug

```bash
# Compilare con simboli debug (-g)
make build

# Lanciare nel debugger VSCode
# F5 oppure: Ctrl+Shift+D → "Debug GtkApp"
```

## 📦 Include Pattern

Il Makefile aggiunge `-Iinclude` al compilatore, permettendo:

```c
// In src/controller/app_controller.c
#include "controller/app_controller.h"
#include "model/app_model.h"
#include "view/app_view.h"
#include "utils/db_connection.h"
```

**Percorsi relativi:**

- Root include: `${workspaceFolder}/include`
- Sottodirectory: risolte automaticamente

## 🎯 Flusso di Esecuzione

```
main.c: main()
    ↓
main.c: on_activate() [signal]
    ↓
app_controller.c: app_controller_new()
    ├─→ app_model.c: app_model_new()
    ├─→ app_view.c: app_view_new()
    │   └─→ Carica ui/main_window.ui
    └─→ db_connection.c: db_connection_new()
    ↓
app_controller.c: app_controller_refresh_ui()
    ├─→ app_model_get_users()
    └─→ app_view_display_users()
    ↓
main_window presentata all'utente
```

## 🚀 Estensioni

### Aggiungere un nuovo modulo

```bash
# 1. Creare struttura
mkdir -p include/mio_modulo src/mio_modulo

# 2. Creare files
touch include/mio_modulo/mio_modulo.h
touch src/mio_modulo/mio_modulo.c

# 3. Aggiungere include nel controller
# In include/controller/app_controller.h:
#include "mio_modulo/mio_modulo.h"

# 4. Compilare
make clean && make build
```

## 🔗 Database PostgreSQL (Opzionale)

### Setup Database

```bash
sudo -u postgres psql
```

```sql
CREATE DATABASE gtkapp;
\c gtkapp

CREATE TABLE users (
    id SERIAL PRIMARY KEY,
    name VARCHAR(255) NOT NULL,
    email VARCHAR(255) NOT NULL,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);
```

### Connessione in app_controller.c

```c
AppController *app_controller_new(GtkApplication *app) {
  AppController *controller = g_malloc(sizeof(AppController));

  // Connessione database
  controller->db = db_connection_new(
    "localhost",     // host
    "postgres",      // user
    "password",      // password (modificare)
    "gtkapp"         // database
  );

  // ...resto codice...
}
```

## 📝 Convenzioni

- **Header guards:** `#ifndef APP_MODULE_H` / `#define APP_MODULE_H`
- **Naming:** `app_function_name()` (snake_case)
- **Struct:** `typedef struct { ... } ModuleName;`
- **Static functions:** `static void on_event(...);` (callback interni)

## 📚 Risorse

- [GTK4 Docs](https://docs.gtk.org/gtk4/)
- [GLib Reference](https://developer.gnome.org/glib/)
- [PostgreSQL libpq](https://www.postgresql.org/docs/current/libpq.html)
- [GtkBuilder XML](https://docs.gtk.org/gtk4/class.Builder.html)

## 📄 Licenza

MIT

---

**Creato:** Maggio 2026  
**Template:** GTK4 MVC C Application
