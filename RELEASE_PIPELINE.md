# Release Pipeline

La pipeline di GitHub Actions gestisce automaticamente il build e il packaging per tutte le piattaforme.

## 🚀 Come usare

1. **Crea una Release su GitHub**:
   - Vai su GitHub → Releases
   - Clicca "Draft a new release"
   - Crea un tag (es. `v1.0.0`)
   - Clicca "Publish release"

2. **La pipeline farà automaticamente**:
   - ✅ Build su Linux → Flatpak (`.flatpak`)
   - ✅ Build su macOS → DMG universale (`arm64` + `x86_64`)
   - ✅ Cross-compile su Linux → Windows x86_64 e ARM64 (`.exe`)
   - ✅ Genera installer Inno Setup (`gtkapp-setup.exe`)
   - ✅ Carica tutti gli artefatti sulla Release

## 📦 Artefatti prodotti

| Piattaforma | File | Descrizione |
|-------------|------|-------------|
| Linux | `gtkapp.flatpak` | Flatpak con sandboxing GNOME 46 |
| macOS | `GtkApp.dmg` | DMG universale (Apple Silicon + Intel) |
| Windows | `gtkapp-setup.exe` | Installer che auto-rileva architettura (x64/ARM64) |

## ⚙️ Personalizzazione

### Flatpak
Modifica `flatpak/io.github.tuouser.GtkApp.json`:
```json
{
    "app-id": "io.github.TUOUSER.GtkApp",
    ...
}
```

### macOS (Info.plist)
Crea `flatpak/Info.plist` per il bundle .app:
```xml
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
    <key>CFBundleExecutable</key>
    <string>gtkapp</string>
    <key>CFBundleIdentifier</key>
    <string>io.github.tuouser.gtkapp</string>
    <key>CFBundleVersion</key>
    <string>1.0</string>
</dict>
</plist>
```

### Windows (Inno Setup)
Personalizza `setup.iss`:
- Cambia `MyAppName`, `MyAppPublisher`, `MyAppURL`
- Aggiungi file aggiuntivi o modifica cartelle di installazione

## 📋 Requisiti

- Repository GitHub pubblico
- Secrets: `GITHUB_TOKEN` (automatico)
- Nessuna configurazione aggiuntiva necessaria!

## 🔗 File correlati

- `.github/workflows/release.yml` - Workflow principale
- `setup.iss` - Configurazione Inno Setup per Windows
- `scripts/collect-windows-dlls.sh` - Script di raccolta DLL
- `flatpak/io.github.tuouser.GtkApp.json` - Manifest Flatpak

## 🐛 Troubleshooting

**Errore: "ISCC non trovato"**
- Verifica che Inno Setup 6 sia installato su Windows Runner
- Il job `windows-installer` lo installa automaticamente con `choco`

**Errore durante cross-compile Windows**
- Assicurati che i pacchetti MinGW siano disponibili su Ubuntu
- Verifica: `sudo apt install x86_64-w64-mingw32-gcc`

**DMG non crea correttamente**
- Verifica che `flatpak/Info.plist` esista
- Verifica che `flatpak/icon.icns` esista (file icone macOS)

---

**La pipeline è pronta!** Una volta pubblicata una Release, vedrai tutti gli artefatti comparire automaticamente.
