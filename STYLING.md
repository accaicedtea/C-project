# GTK4 CSS Styling Guide

## 📚 Introduzione

GTK4 usa **CSS** per lo styling. I file CSS vengono caricati da `ui/style.css` e applicati a tutta l'applicazione automaticamente.

## 📂 Struttura Styling

```
ui/
├── main_window.ui         # Layout e struttura UI
├── style.css              # Stili CSS globali
└── themes/                # (Opzionale) Temi alternativi
    ├── dark.css
    └── light.css
```

## 🎨 Personalizzare Stili

### Opzione 1: Modificare `ui/style.css`

Apri `ui/style.css` e modifica gli stili direttamente:

```css
/* Cambia colore pulsante */
button {
  background-color: #ff6b6b; /* Rosso */
  color: #ffffff; /* Testo bianco */
  padding: 10px 20px;
  border-radius: 6px;
}

button:hover {
  background-color: #ff5252; /* Rosso più scuro */
}
```

### Opzione 2: Usare Variabili Tematiche

GTK4 supporta variabili CSS. Modifica all'inizio di `style.css`:

```css
@define-color primary_color #4c7ff0;
@define-color secondary_color #357abd;
@define-color accent_color #FF6B6B;

button {
  background-color: @primary_color;
}

button:hover {
  background-color: @secondary_color;
}
```

### Opzione 3: Tema Scuro

Crea `ui/theme_dark.css`:

```css
@define-color theme_bg_color #1e1e1e;
@define-color theme_fg_color #ffffff;
@define-color theme_base_color #2d2d2d;
@define-color borders #404040;

button {
  background-color: #0d47a1;
  color: #ffffff;
}
```

Carica in `src/view/app_view.c`:

```c
// Cambia:
gtk_css_provider_load_from_path(provider, "ui/style.css", &error);
// In:
gtk_css_provider_load_from_path(provider, "ui/theme_dark.css", &error);
```

## 🎯 Selettori CSS Comuni

| Selettore              | Cosa Stila            |
| ---------------------- | --------------------- |
| `button`               | Tutti i pulsanti      |
| `entry`                | Campi di testo        |
| `label`                | Etichette di testo    |
| `listbox`              | Liste                 |
| `listbox row`          | Righe della lista     |
| `listbox row:selected` | Riga selezionata      |
| `entry:focus`          | Entry quando attivo   |
| `button:hover`         | Pulsante al mouseover |
| `button:active`        | Pulsante premuto      |

## 📝 Proprietà CSS Comuni

```css
/* Dimensioni */
min-width: 100px;
min-height: 36px;
padding: 10px 15px;
margin: 5px;

/* Colori */
background-color: #ff6b6b;
color: #ffffff;
border-color: #333333;

/* Bordi */
border: 2px solid #ccc;
border-radius: 4px;
border-top: 1px solid red;

/* Testo */
font-size: 14pt;
font-weight: bold;
font-family: "Sans Serif";
text-decoration: underline;

/* Effetti */
opacity: 0.8;
box-shadow: 0 2px 4px rgba(0, 0, 0, 0.1);
```

## 🛠️ Esempio: Personalizzare Pulsante "Add"

### Nel file `main_window.ui`:

```xml
<object class="GtkButton" id="add_button">
  <property name="label">Add</property>
  <style>
    <class name="add-btn"/>
  </style>
</object>
```

### Nel file `ui/style.css`:

```css
button.add-btn {
  background-color: #27ae60; /* Verde */
  color: white;
  font-weight: bold;
  padding: 10px 20px;
  border-radius: 6px;
}

button.add-btn:hover {
  background-color: #229954; /* Verde scuro */
}

button.add-btn:active {
  background-color: #1e8449; /* Verde ancora più scuro */
}
```

## 🎨 Temi Predefiniti GTK4

Usa variabili di tema predefinite in GTK4:

```css
@define-color theme_bg_color #ffffff; /* Sfondo finestra */
@define-color theme_fg_color #000000; /* Testo principale */
@define-color theme_base_color #f5f5f5; /* Sfondo input */
@define-color theme_text_color #333333; /* Testo input */
@define-color theme_selected_bg_color #0d47a1; /* Sfondo selezionato */
@define-color theme_selected_fg_color #ffffff; /* Testo selezionato */
@define-color success_color #4caf50; /* Verde */
@define-color warning_color #ff9800; /* Arancione */
@define-color error_color #f44336; /* Rosso */
```

## 💡 Consigli Pratici

### 1. Test Veloce

Modifica `ui/style.css` e salva → L'app ricarica gli stili automaticamente

### 2. Dark Mode

Aggiungi un toggle per cambiare tema:

```c
void toggle_dark_mode(AppView *view) {
  GtkCssProvider *provider = gtk_css_provider_new();
  gtk_css_provider_load_from_path(
    provider,
    is_dark ? "ui/style.css" : "ui/dark.css",
    NULL
  );
}
```

### 3. Stato Hover / Focus

```css
button:hover {
  opacity: 0.8;
  /* oppure */
  transform: scale(1.02);
}

entry:focus {
  border: 2px solid @theme_selected_bg_color;
  box-shadow: 0 0 0 2px rgba(76, 127, 240, 0.1);
}
```

### 4. Classi Custom

Nel `.ui` file:

```xml
<style>
  <class name="my-custom-class"/>
</style>
```

Nel `style.css`:

```css
label.my-custom-class {
  font-weight: bold;
  color: #ff6b6b;
}
```

## 🔧 Debugging Stili

### Visualizzare stili applicati:

```bash
# Abilita debug mode
GTK_DEBUG=interactive make run
```

### Controllare CSS nel file:

```bash
# Valida sintassi CSS
csslint ui/style.css
```

## 📚 Risorse

- [GTK4 CSS Documentation](https://docs.gtk.org/gtk4/css-overview.html)
- [GTK4 CSS Properties](https://docs.gtk.org/gtk4/css-properties.html)
- [CSS Color Reference](https://www.w3schools.com/cssref/css_colors.asp)

## 🚀 Esempi Completi

### Tema Moderno Minimalista

```css
@define-color primary #2196F3;
@define-color secondary #F57C00;
@define-color background #FAFAFA;
@define-color surface #FFFFFF;
@define-color text #212121;

* {
  padding: 0;
  margin: 0;
}

button {
  background-color: @primary;
  color: white;
  padding: 12px 24px;
  border-radius: 8px;
  font-weight: 500;
}

entry {
  padding: 10px 12px;
  border: 1px solid #e0e0e0;
  border-radius: 6px;
}
```

### Tema Corporate Scuro

```css
@define-color bg #1A1A1A;
@define-color fg #EEEEEE;
@define-color accent #0091AD;

window {
  background-color: @bg;
  color: @fg;
}

button {
  background-color: @accent;
  color: @bg;
  font-weight: bold;
}
```

---

**Nota:** Ricarica con `make run` per vedere i cambiamenti. Se non funziona, controlla che il path di `style.css` sia corretto! 🎨
