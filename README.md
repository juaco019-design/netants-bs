# NetAnts Reborn 2.0 - Clon limpio y modernizado

## ¿Qué es?
Reescritura completa desde cero de **NetAnts 1.25** (gestor de descargas clásico de 1998-2004), eliminando:
- Código obsoleto del dump RecStudio (`L00403710`, `GetModuleFileNameA`, `comctl32.dll` checks, `OleInitialize` etc.)
- Ads, spyware y banners del original
- WinInet / MFC obsoleto

Y conservando/clonando:
- Interfaz y UX: toolbar, categorías a la izquierda, lista con 8 columnas, statusbar animado
- Opciones: proxy, conexiones, velocidad, carpeta por defecto
- Lógica de gestor: multi-segmento (hasta 16), resume, cola, FTP+HTTP

## Stack moderno (pedido)
- **C++17**, **wxWidgets 3.2** (UI nativa Windows)
- **libcurl** + **cpr** (HTTP/FTP, multi-handle, resume, Range)
- **nlohmann/json** (archivos .job en JSON legible, reemplaza .ant binario)
- **CMake** + **GCC MinGW-w64** en Windows, flags `-O3 -flto -march=x86-64 -s -static-libgcc`

## Funcionalidades enumeradas (clonadas y modernizadas)

### 1. Interfaz clonada
- [x] Ventana principal 1024x600, icono hormiga XPM
- [x] Toolbar: Nuevo, Iniciar, Pausar, Borrar, Propiedades, Opciones (iconos 16x16)
- [x] Splitter: Tree categorías (Todas, Descargando, Completadas, En cola, Fallidas, HTTP, FTP)
- [x] ListCtrl columnas: Archivo, Tamaño, Descargado, %, Velocidad, Tiempo restante, Estado, URL
- [x] Animación hormigas llevando hoja (8 frames, wxTimer 150ms) + progress bar con hormiga

### 2. Diálogos clonados
- [x] Agregar URL: validación http/https/ftp, carpeta con browse, selector de segmentos (1-16)
- [x] Opciones: General (concurrentes, segmentos, límite KB/s, carpeta) + Proxy (host, puerto, user/pass)

### 3. Gestor de descargas HTTP/FTP (core modernizado)
- [x] `DownloadJob`: id corto, url, filename extraído, saveDir, totalSize via HEAD, segmented .part files, merge final
- [x] Soporte HTTP Range y FTP REST (libcurl `CURLOPT_RANGE`, `CURLOPT_RESUME_FROM_LARGE`)
- [x] Velocidad instantánea y media, ETA, progreso 0..1
- [x] Pausa (abort via progress callback return 1), Resume (usa tamaño .part existente)
- [x] Multi-handle `curl_multi_perform` + `curl_multi_wait` (no busy loop)
- [x] `DownloadManager`: cola, thread worker único con `maxConcurrent=3`, notificaciones UI thread-safe via `wxCallAfter`

### 4. Persistencia .job JSON
- Estructura `.job`:
```json
{
  "id": "a1b2c3d4",
  "url": "https://...",
  "fileName": "archivo.zip",
  "saveDir": "C:\\Users\\...\\Downloads",
  "totalSize": 1234567,
  "downloaded": 1234,
  "status": 1,
  "numSegments": 4,
  "segments": [{"start":0,"end":308641,"downloaded":0,"tempFile":"...part0"}]
}
```
- Guardado atómico (.tmp -> rename), carga al iniciar, recuperación de crash (Downloading -> Queued)

### 5. Optimización GCC Windows
- CMake: `-O3 -flto=auto -march=x86-64 -mtune=generic -static-libgcc -static-libstdc++ -s -Wl,--subsystem,windows`
- Debug: `-O0 -g3 --subsystem,console`
- Libs: `ws2_32 wsock32 wininet winmm comctl32 ole32`

## Errores removidos del dump original
- `L00403710` entry obsoleta con `GetModuleFileNameA` + `LoadLibrary comctl32.dll` + `DllGetVersion` manual -> reemplazado por `wxApp::OnInit` + `curl_global_init`
- `L0041B22A`, `L0041E794` checks de versión Windows 9x/NT -> eliminado, wxWidgets ya abstrae
- `FindWindowA "NA_MainWin"` single-instance via `OpenEventA "NetAnts_Running"` -> no necesario, wxSingleInstanceChecker si se quiere
- `OleInitialize` sin `CoInitializeEx` -> libcurl no necesita OLE
- Gotos, registros `_t41`, `__ebx` del decompiler -> reescritura OO limpia

## Compilación en Windows (MSYS2 MinGW)

```bash
# 1. Instala MSYS2, abre MINGW64 shell
pacman -S mingw-w64-x86_64-gcc mingw-w64-x86_64-cmake mingw-w64-x86_64-wxWidgets3.2 \
          mingw-w64-x86_64-curl mingw-w64-x86_64-cpr mingw-w64-x86_64-nlohmann-json

# 2. Compila
mkdir build && cd build
cmake .. -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release
mingw32-make -j4

# 3. Ejecuta
./NetAntsReborn.exe
# o
./NetAntsReborn.exe -- para debug: cmake -DCMAKE_BUILD_TYPE=Debug
```

Alternativa vcpkg:
```
vcpkg install wxwidgets curl cpr nlohmann-json --triplet x64-mingw-static
cmake .. -DCMAKE_TOOLCHAIN_FILE=[vcpkg]/scripts/buildsystems/vcpkg.cmake -DVCPKG_TARGET_TRIPLET=x64-mingw-static
```

## Estructura
```
src/
  App.h/cpp         - wxApp, curl_global_init
  MainFrame.h/cpp   - UI clon
  DownloadJob.h/cpp - core HTTP/FTP multi-segmento
  DownloadManager.h/cpp - cola y worker
  JobStorage.h/cpp  - .job JSON
  AddUrlDialog.h/cpp - dialogo nuevo
  OptionsDialog.h/cpp - opciones proxy
  Icons.h           - XPM hormiga + toolbar
  Animations.h      - frames hormiga + progress
  Utils.h/cpp       - formateo, paths, validacion
```

## Próximos pasos (no incluidos para mantener clon fiel)
- Drag&Drop de URLs, monitor clipboard
- Scheduler, apagado al terminar
- Categorías personalizadas, integración navegador
- Gráfico velocidad

Licencia: MIT - reescritura limpia, sin código del binario original (solo ideas/interfaz, que no es copyrightable en US bajo scènes à faire para gestor).
