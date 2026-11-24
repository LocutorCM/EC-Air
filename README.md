# EC Air

Excitador de aire de alta frecuencia (estilo Fresh Air) para macOS. Formatos: **AU** (Logic Pro), **VST3** y Standalone.

- **Mid Air**: realza 3–9 kHz y genera armónicos por encima de 9 kHz.
- **High Air**: realza >10 kHz y añade armónicos de 12 kHz+.
- **Link**: mueve ambas perillas a la vez. **Trim**: ±12 dB de salida. **Bypass**.
- Procesado con sobremuestreo 2x (sin aliasing). Latencia informada al host.

## Compilar en GitHub
Sube el proyecto: la pestaña **Actions** compila sola; descarga `EC-Air-macOS` desde el resultado del workflow.

## Instalar
Descomprime y copia:
- `EC Air.component` → `~/Library/Audio/Plug-Ins/Components/`
- `EC Air.vst3` → `~/Library/Audio/Plug-Ins/VST3/`

Si macOS lo bloquea: `xattr -cr "EC Air.component" "EC Air.vst3"`.
Reinicia Logic Pro: aparece en *Audio FX → EC → EC Air*.
Validar: `auval -v aufx Ecar Ecaa`

## Compilar en local
```
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```
