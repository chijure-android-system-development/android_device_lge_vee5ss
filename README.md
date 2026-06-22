# CyanogenMod 10 para LG L5 II (vee5ss)

Device tree para portar CyanogenMod 10 / Android 4.1.2 Jelly Bean al
LG-E450g, codename `vee5ss`, basado en MediaTek MT6575.

Este arbol esta en desarrollo y fue depurado de forma iterativa con ADB,
TWRP y backups del stock ROM. No asumir que una compilacion limpia reproduce
el telefono si antes se hicieron `adb push` manuales: comparar siempre los
blobs y el ramdisk contra el dispositivo que bootea.

## Hardware

| Campo | Valor |
| --- | --- |
| Dispositivo | LG L5 II / LG-E450g |
| Codename | `vee5ss` |
| SoC | MediaTek MT6575 |
| CPU | ARMv7-A Cortex-A9, single core |
| RAM | 512 MB aprox. |
| Pantalla | WVGA 480x800, hdpi |
| Android stock base | 4.1.2 JB, JZO54K / E450g10b |
| Kernel | prebuilt, cargado desde `device/lge/vee5ss/kernel` |
| Boot image | formato MTK custom mediante `mtkbootimg` |

## Kernel en el arbol

Solo hay un kernel prebuilt en el device tree:

| Archivo | Uso |
| --- | --- |
| `device/lge/vee5ss/kernel` | **Kernel activo del build**. `BoardConfig.mk` lo referencia via `TARGET_PREBUILT_KERNEL`. Ya incluye header MTK `KERNEL`, y aparece byte a byte dentro de `out/target/product/vee5ss/boot.img` en el offset del kernel. |

El estado verificado del port usa `device/lge/vee5ss/kernel` como prebuilt activo.

## Particiones

Las particiones fueron verificadas desde `/proc/dumchar_info`. Los nodos como
`/dev/bootimg` y `/dev/recovery` son interfaces virtuales del driver MTK eMMC.

| Montaje | Block device | FS | Tamano |
| --- | --- | --- | --- |
| `/boot` | `/dev/bootimg` | emmc | 6 MB |
| `/recovery` | `/dev/recovery` | emmc | 7 MB |
| `/system` | `/dev/block/mmcblk0p8` | ext4 | 1 GB |
| `/cache` | `/dev/block/mmcblk0p9` | ext4 | stock |
| `/data` | `/dev/block/mmcblk0p10` | ext4 | ~2 GB |
| `/external_sd` | `/dev/block/mmcblk1p1` | vfat | SD externa |
| `/nvram` | `/dev/nvram` | emmc | stock |
| `/uboot` | `/dev/uboot` | emmc | stock |

Flasheo directo del boot en este equipo:

```bash
adb push out/target/product/vee5ss/boot.img /cache/boot.img
adb shell "dd if=/cache/boot.img of=/dev/bootimg bs=4096; sync"
adb reboot
```

Aunque `/proc/dumchar_info` reporta `bootimg` en `mmcblk0` offset
`0x1200000` (`bs=512 seek=36864`), durante la depuracion ese metodo no cambio
el ramdisk que realmente arranco. El metodo fiable verificado fue escribir al
nodo MTK `/dev/bootimg`.

---

## Estado actual (2026-06-22)

| Subsistema | Estado |
| --- | --- |
| Boot Android | ✅ `sys.boot_completed=1` confirmado |
| GPU / SurfaceFlinger | ✅ Funciona con blobs PowerVR SGX531 |
| `/dev/pvrsrvkm` | ✅ `0666`, cargado via insmod en init.rc |
| RIL (rild) | ✅ Corre, socket `/dev/socket/rild` activo |
| `liblgpclient_jni.so` | ✅ Stub reconstruido con exports LGE correctos |
| Carga firmware modem | ✅ `modem.img` + `DSP_ROM` cargados por CCCI |
| CCCI módulos | ✅ `ccci_plat.ko` + `ccci.ko` + `ccmni.ko` stock |
| `ccci_fsd` / `ccci_rpcd` | ✅ Corriendo, NVRAM y RPC respondidos |
| `ccci_mdinit` | ⚠️ Race condition con timer del kernel (ver abajo) |
| `gsm0710muxd` | ⏳ No arranca hasta resolver ccci_mdinit |
| `/dev/radio/ptty*` | ⏳ No existen hasta que corre gsm0710muxd |
| `gsm.sim.state` | ⏳ UNKNOWN |
| MicroSD externa | ✅ Montada en `/storage/sdcard1` via vold.fstab |
| Almacenamiento interno | ✅ FUSE daemon corriendo, `/storage/sdcard0` montado |
| Settings > Storage | ✅ Abre sin crash |
| Brillo LCD | ✅ `lights.default.so` + `libproxyhal.so` stock cargados |
| LEDs notificación (RGB) | ✅ LP5521 R/G/B via HAL, blink con timer trigger |
| boot.img reconstruido | ✅ Flasheado con mtkbootimg |

---

## Historial de problemas resueltos

### 1. OTA inicial faltando APKs core
**Síntoma:** OTA de 99 MB sin Launcher, Settings, Phone ni ~44 APKs esenciales.  
**Causa:** `device.mk` no heredaba `generic_no_telephony.mk`.  
**Fix:** Agregar al `device.mk`.

### 2. Kernel panic al cargar pvrsrvkm
**Síntoma:** Panic al arrancar; diagnóstico inicial incorrecto de "doble carga".  
**Causa real:** El `boot.img` había sido empaquetado con Python (gzip con `os=255`
vs `os=3`); el kernel lo rechazaba generando pánico VFS, no pvrsrvkm.  
**Fix:** Usar siempre `mtkbootimg` para empaquetar; nunca Python.

### 3. SurfaceFlinger heap crash (`deadbaad`)
**Síntoma:** `libdvm` crash con dirección `deadbaad`.  
**Causa:** `pvrsrvkm.ko` había sido eliminado del vendor tree por error.  
**Fix:** Restaurar `proprietary/lib/modules/pvrsrvkm.ko` desde backup stock.

### 4. `EGL_BAD_DISPLAY` / `EGL_BAD_PARAMETER`
**Síntoma:** SurfaceFlinger no iniciaba; blobs EGL no encontrados.  
**Causa:** El árbol `/system/vendor/` completo faltaba del OTA.  
**Fix:** Agregar todos los blobs GPU PowerVR a `vee5ss-vendor-blobs.mk`.

### 5. `android.phone` crash loop
**Síntoma:** `com.android.phone` se reiniciaba constantemente.  
**Causa:** `ro.telephony.ril_class=MediaTekRIL` apuntaba a una clase Java
inexistente en CM10.  
**Fix:** Reemplazar con `rild.libpath=/system/lib/mtk-ril.so` en `build.prop`.

### 6. `rild` SIGSEGV en `0x00000000`
**Síntoma:** rild crasheaba al intentar llamar a funciones de `liblgpclient_jni.so`.  
**Causa:** El stub de `liblgpclient_jni.so` no exportaba `LGE_FacReadNetworkCode`
ni `LGE_FacReadNetworkCodeListNum`.  
**Fix:** Reconstruir stub con los symbols correctos.

### 7. Modem WDT (MD_WDT_STA=8000) a los 32 segundos
**Síntoma:** El modem lanzaba Watchdog Timer ~32s después de `MD_INIT_START_BOOT`.  
**Causa:** Sin `ccci_mdinit` corriendo, nadie lee `/dev/ccci_sys_rx` para completar
el handshake CCCI. El modem espera respuesta del AP, no llega, y dispara WDT.  
**Fix:** Agregar `ccci_mdinit`, `ccci_fsd`, `ccci_rpcd`, `gsm0710muxd` al vendor
tree e `init.mt6575.rc`.

### 8. `ccci_plat.ko` / `ccci.ko` con "Unknown symbol"
**Síntoma:** Los módulos del vendor tree fallaban al cargar:
`Unknown symbol __stack_chk_guard`, `kmem_cache_alloc_trace`, etc.  
**Causa:** Los `.ko` del vendor árbol eran incompatibles con el kernel del telefono.  
**Fix:** Reemplazar con los módulos stock extraídos del backup del dispositivo,
ubicados en `/home/chijure/e450-build/system_lib_modules_backup/`.

### 9. `ccci_fsd` y `ccci_mdinit` muertos por SIGHUP
**Síntoma:** Los daemons CCCI morían al cerrar la sesión adb.  
**Causa:** `nohup` no sirve porque los binarios reestablecen `SIGHUP` a `SIG_DFL`
al arrancar, revirtiendo el `SIG_IGN` de nohup.  
**Fix:** Usar `setsid` en lugar de `nohup`; crea una nueva sesión de proceso
completamente inmune al SIGHUP del shell padre.

### 10. Kernel panic por `rmmod ccci`
**Síntoma:** Kernel panic al hacer `rmmod ccci.ko` con el modem activo.  
**Causa:** Con el modem en estado WDT-reset o activo, el módulo CCCI tiene estado
hardware activo; el rmmod corrompe esa máquina de estados.  
**Regla permanente:** NUNCA hacer `rmmod ccci*` con el modem activo o tras un WDT.
El único reset seguro de CCCI+modem es reiniciar el dispositivo.

### 11. MicroSD no montaba (`No such file or directory`)
**Síntoma:** Vold montaba `/dev/block/mmcblk1p1` en `/mnt/secure/staging` (OK),
luego fallaba: `Failed to move mount /mnt/secure/staging -> /mnt/sdcard (No such
file or directory)`. El contenido de la SD nunca aparecía.

**Causas encadenadas (tres independientes):**

1. **`/storage/` nunca creado.** El ramdisk no incluye el directorio `/storage/`.
   `init.mt6575.rc` en `on init` hacía `mkdir /storage/emulated` y
   `mkdir /storage/sdcard1`, pero el rootfs montado como `ro` en ese punto.
   El resultado: ambas llamadas fallan silenciosamente, `/storage/` nunca existe,
   y los symlinks `/mnt/sdcard → /storage/emulated/legacy` y
   `/mnt/external_sd → /storage/sdcard1` apuntan a paths inexistentes.

2. **Vold usa `AutoVolume("sdcard", "/mnt/sdcard")` por defecto.** El binario vold
   de CM10 para MT6575 NO lee `fstab.mt6575` para configurar volúmenes; busca
   `/etc/vold.fstab` y si no lo encuentra (línea 184 de `system/vold/main.cpp`)
   crea un `AutoVolume` hardcodeado con label `"sdcard"` y mount point `"/mnt/sdcard"`.
   Esto hace que vold intente mover el mount al symlink `/mnt/sdcard`, que resuelve
   a `/storage/emulated/legacy` — path inexistente (causa 1).

3. **El daemon `sdcard` (FUSE) es `class late_start`.** No arranca hasta que para la
   animación de boot. Aunque `/storage/` existiera, `ccci_fsd` necesita la estructura
   FUSE antes de que el modem la pida; el ordering es incorrecto en arranques
   con la SD ya presente.

**Path sysfs de la SD:** `/devices/platform/mtk-sd.1/mmc_host/mmc1`
(verificado con `cat /sys/block/mmcblk1/device/uevent` y `readlink -f
/sys/block/mmcblk1/device`).

**Fix — dos cambios en fuente + un archivo nuevo:**

_1. `init.mt6575.rc` — agregar `mkdir /storage` antes de los subdirectorios:_
```
on init
    mkdir /mnt/shell/emulated 0700 shell shell
    mkdir /storage 0755 root root          ← NUEVO
    mkdir /storage/emulated 0555 root root
    mkdir /storage/sdcard1 0775 system system
```

_2. `device/lge/vee5ss/vold.fstab` — archivo nuevo:_
```
dev_mount sdcard1 /storage/sdcard1 auto /devices/platform/mtk-sd.1/mmc_host/mmc1
```

_3. `device.mk` — añadir a PRODUCT_COPY_FILES:_
```make
device/lge/vee5ss/vold.fstab:system/etc/vold.fstab
```

**Resultado:** Vold lee `vold.fstab`, crea un `DirectVolume` con label `sdcard1`,
monta la SD directamente en `/storage/sdcard1` (1.9 GB, 434 MB libres, confirmado
en vivo). El symlink `/mnt/external_sd → /storage/sdcard1` funciona correctamente.

### 12. Settings > Storage se cerraba (`getVolumeState` IllegalArgumentException)
**Síntoma:** Abrir Settings > Storage forzaba el cierre de la aplicación.  
**Causa:** `MountService.getVolumeState()` lanza `IllegalArgumentException` cuando
el path consultado no está en su mapa `mVolumeStates`. El mapa se construye desde
`storage_list.xml` (vía recursos de framework) y desde eventos de vold.
Sin un `storage_list.xml` de device, el AOSP por defecto usa `/mnt/sdcard` como
primary. Vold reporta `/storage/sdcard1`. Settings preguntaba por paths que
MountService no reconocía → excepción.

**Fix — `storage_list.xml` en overlay del device:**  
Nuevo archivo `overlay/frameworks/base/core/res/res/xml/storage_list.xml`:
```xml
<storage android:mountPoint="/storage/sdcard0"
         android:storageDescription="@string/storage_internal"
         android:emulated="true" android:mtpReserve="100" android:primary="true" />
<storage android:mountPoint="/storage/sdcard1"
         android:storageDescription="@string/storage_sd_card"
         android:removable="true" />
```
Con `emulated="true"`, MountService pre-popula `/storage/sdcard0` como
`MEDIA_MOUNTED` al arrancar, sin esperar evento de vold. El overlay requiere
que `DEVICE_PACKAGE_OVERLAYS := device/lge/vee5ss/overlay` esté en `device.mk`
(faltaba — añadido).

### 13. `statfs failed: ENOENT` — daemon sdcard crasheaba en cada boot
**Síntoma:** Aunque Settings > Storage ya no lanzaba `IllegalArgumentException`,
`StorageMeasurement` crasheaba con `statfs failed: ENOENT` al intentar medir
el espacio en `/storage/emulated/legacy`.  
**Causas encadenadas:**
1. El daemon sdcard se reiniciaba en bucle porque la llamada era:
   `/system/bin/sdcard -u 1023 -g 1023 -l /data/media /mnt/shell/emulated`
   pero el binario (`sdcard.c`, `MOUNT_POINT="/storage/sdcard0"`) usa la sintaxis
   **ICS-era**: `sdcard [-l -f] <path> <uid> <gid>` → "too many arguments".
2. El path de almacenamiento primario configurado (`/storage/emulated/legacy`)
   no coincidía con donde el daemon realmente monta (`/storage/sdcard0`).
3. El directorio `/mnt/shell/emulated` (padre del mount point en la llamada
   incorrecta) tampoco existía porque `/mnt/shell` no se creaba en init.rc.

**Fix — alineación completa de paths a `/storage/sdcard0`:**
- `init.mt6575.rc` `on init`: `mkdir /storage/sdcard0 0775 system system`
- `EXTERNAL_STORAGE` → `/storage/sdcard0` (antes `/storage/emulated/legacy`)
- Symlinks `/sdcard` y `/mnt/sdcard` → `/storage/sdcard0`
- Eliminar exports `EMULATED_STORAGE_SOURCE/TARGET` (no aplican en JB 4.1.2)
- Eliminar `mkdir /mnt/shell` y `/mnt/shell/emulated` (ya no se usan)
- Servicio sdcard: `sdcard /data/media 1023 1023` (sintaxis correcta)
- `storage_list.xml` mountPoint: `/storage/sdcard0`

**Resultado:** `init.svc.sdcard=running`, `/dev/fuse /storage/sdcard0` montado,
`/dev/block/mmcblk1p1 /storage/sdcard1` montado, Settings > Storage abre.

### 14. Brillo bloqueado en 0 (`lights.default.so` no cargaba)
**Síntoma:** La pantalla arrancaba con brillo 0. Settings > Display no tenía efecto.  
**Causa:** `lights.default.so` del stock MTK depende de `libproxyhal.so`, que no
estaba en el vendor tree. El error en logcat:
```
Cannot load library: libproxyhal.so not found
```
Sin HAL de luces, `LightService` no puede controlar el backlight LCD, los LEDs
RGB del LP5521 ni el backlight de los botones.

**Fix:** Extraer `libproxyhal.so` y `lights.default.so` del backup TWRP stock
(`system.ext4.win` de `/home/chijure/Documentos/2026-06-16--08-31-05_JZO54K/`)
y añadirlos al vendor tree. `libproxyhal.so` solo depende de `libbinder`,
`libutils`, `liblog`, `libc` — todas librerías estándar de CM10.

**Archivos añadidos:**
- `vendor/lge/vee5ss/proprietary/lib/libproxyhal.so`
- `vendor/lge/vee5ss/proprietary/lib/hw/lights.default.so` (ya estaba, ahora carga)

**Resultado:** `D/lights: set_led_state`, `blink_red`, `blink_green` en logcat.
Brillo ajustable desde Settings > Display. LEDs RGB (LP5521) funcionales con
blink por hardware via `timer` trigger en sysfs.

---

## Arquitectura CCCI (conocimiento critico)

### Protocolo de arranque del modem MT6575

El modem usa CCCI (Cross Core Communication Interface). El protocolo real
descubierto durante el port es:

```
ccci_mdinit
    │
    ├─ Espera "nvram restore ready" (10 s timeout → falla si no hay nvram_daemon)
    │
    ├─ Escribe "1" en /sys/class/BOOT/BOOT/boot/md
    │       │
    │       └─ Kernel: carga modem.img + DSP_ROM en RAM del modem
    │                  MPU protection activa
    │                  Modem empieza a ejecutar
    │
    ├─ Modem → Kernel: 0xFAF50002 (MD_INIT_START_BOOT)
    │       │
    │       └─ Kernel: inicia timer de espera NORMAL_BOOT_ID
    │                  llama ccci_send_run_time_data()
    │                  registra "wait for NORMAL_BOOT_ID"
    │
    ├─ ccci_rpcd responde llamadas RPC del modem (ej. ADC_RFTMP calibration)
    │       Duración: ~0.36 segundos
    │
    ├─ ccci_fsd responde solicitudes de archivos NVRAM del modem
    │       Archivos: NVD_CORE, NVD_CUST, NVD_DATA, CALIBRAT, IMPORTNT, NVD_IMEI
    │       Duración: ~9 segundos (muchos archivos, secuencial)
    │
    ├─ Modem → Kernel: NORMAL_BOOT_ID (modem listo para datos)
    │       │
    │       └─ Kernel: sets MD_STATE_READY → ttyC0 ahora acepta escrituras
    │
    ├─ ccci_mdinit: ctl.start gsm0710muxd
    │
    └─ gsm0710muxd: crea /dev/radio/pttyXXX → rild conecta → GSM/3G operativo
```

### Mensajes CCCI en ccci_sys_rx

| Valor | Nombre | Quien lo envía | Significado |
| --- | --- | --- | --- |
| `0xFAF50002` | `MD_INIT_START_BOOT` | Modem | Etapa 1 completa; enviar runtime data |
| `0xFAF50004` | `CCCI_MD_MSG_EXCEPTION` | Kernel | WDT / crash del modem |
| `0xFAF50007` | timeout notification | Kernel | Timer del kernel expiró |

**Nota importante:** `0xFAF50007` en `/dev/ccci_sys_rx` NO es el `NORMAL_BOOT_ID`
del modem; es la notificación del kernel a `ccci_mdinit` de que se acabó el tiempo.
El `NORMAL_BOOT_ID` lo procesa el kernel internamente y se refleja en el estado del
driver CCCI (`MD_STATE_READY`).

### Race condition del timer (problema en investigacion)

En la versión stock del `ccci.ko` extraído del dispositivo, `ccci_send_run_time_data()`
arranca un timer de **10 segundos** esperando que el modem complete su inicialización
y envíe `NORMAL_BOOT_ID`. El modem tarda **~10.006 segundos** en completar la carga
de NVRAM y enviar `NORMAL_BOOT_ID`. Resultado: el timer del kernel expira
**319 microsegundos antes** de que el kernel procese el `NORMAL_BOOT_ID`.

Cuando el timer expira primero, el kernel llama `md_boot_up_timeout_func()` que:
1. Escribe `0xFAF50007` en `ccci_sys_rx` (notifica a `ccci_mdinit`)
2. No actualiza `MD_STATE_READY`
3. Deja `ttyC0` en estado `md_boot_1` (sin aceptar escrituras)

Incluso si el `NORMAL_BOOT_ID` llega 319µs después, el driver usa `del_timer()`
(no `del_timer_sync()`), lo que permite que el callback del timer se ejecute
concurrentemente en otro CPU y gane la carrera.

### Fix aplicado: patch de ccci.ko (timer 10 s → 20 s)

Para dar margen al modem, se parcheó `ccci.ko` directamente:

**Instrucción ARM parchada** (en `ccci_send_run_time_data()`, offset `0x5c54`):

```
Antes:  e2811ffa  add r1, r1, #1000   @ 0x3e8  (10 s a HZ=100)
Despues: e2811e7d  add r1, r1, #2000   @ 0x7d0  (20 s a HZ=100)
```

**Script de patch** (reproducible):

```python
fname = 'vendor/lge/vee5ss/proprietary/lib/modules/ccci.ko'
with open(fname, 'rb') as f:
    data = bytearray(f.read())
offset = 0x5c88  # file offset = .text offset (0x34) + virtual addr (0x5c54)
old = bytes([0xFA, 0x1F, 0x81, 0xE2])  # add r1, r1, #1000
new = bytes([0x7D, 0x1E, 0x81, 0xE2])  # add r1, r1, #2000
assert data[offset:offset+4] == old
data[offset:offset+4] = new
with open(fname, 'wb') as f:
    f.write(data)
```

El `ccci.ko` en el vendor tree ya tiene este patch aplicado. El backup del
original sin parchear está en `proprietary/lib/modules/ccci.ko.orig`.

**Estado con el patch:** el modem completa RPC calibration (~0.36 s) pero
`ccci_fsd` no sirve suficientes archivos NVRAM en 20 s. Investigando.

---

## Modulos CCCI

Los módulos del vendor árbol original eran incompatibles. Los módulos actuales
en el árbol son copias exactas del dispositivo stock:

**Fuente:** `/home/chijure/e450-build/system_lib_modules_backup/`

| Módulo | Major | Función |
| --- | --- | --- |
| `ccci_plat.ko` | — | HAL de plataforma MT6575 para CCCI |
| `ccci.ko` | 178, 183, 184 | Driver CCCI principal + control del modem |
| `ccmni.ko` | — | Interfaz de red del modem (datos móviles) |

Orden de carga obligatorio: `ccci_plat.ko` → `ccci.ko` → `ccmni.ko`

---

## Nodos CCCI

El driver CCCI usa `cdev_add()`, NO `misc_register()`, por lo tanto
**udev/ueventd NO crea los nodos automáticamente**. Deben crearse con `mknod`.

```
/dev/ccci_fs          c 178 0    # ccci_fsd — lectura de archivos NVRAM
/dev/ccci_rpc         c 178 1    # ccci_rpcd — llamadas RPC del modem
/dev/ccci_sys_rx      c 184 2    # ccci_mdinit — mensajes kernel→modem RX
/dev/ccci_sys_tx      c 184 3    # ccci_mdinit — mensajes AP→modem TX
/dev/ccci_pcm_rx      c 184 4    # audio PCM
/dev/ccci_pcm_tx      c 184 5
/dev/ccci_ipc_1220_0  c 183 0    # IPC entre CPUs
/dev/ccci_uem_rx      c 184 18   # UEM (gestión energía)
/dev/ccci_uem_tx      c 184 19
/dev/ccci_md_log_rx   c 184 42   # log del modem
/dev/ccci_md_log_tx   c 184 43
/dev/ttyC0            c 169 0    # AT commands (gsm0710muxd)
/dev/ttyC1            c 169 1
/dev/ttyC2            c 169 2
```

Todos los nodos: `chmod 0660`, `chown radio radio`.
`/sys/class/BOOT/BOOT/boot/md`: `chown radio radio`.

---

## Daemonizacion correcta de los daemons CCCI

`nohup` NO funciona con los binarios CCCI: estos llaman a `signal(SIGHUP, SIG_DFL)`
al arrancar, revirtiendo el `SIG_IGN` que pone nohup.

Usar `setsid` para crear una nueva sesión de proceso:

```sh
setsid /system/bin/ccci_rpcd  >/data/local/tmp/ccci_rpcd.log  2>&1 </dev/null &
setsid /system/bin/ccci_fsd   >/data/local/tmp/ccci_fsd.log   2>&1 </dev/null &
setsid /system/bin/ccci_mdinit >/data/local/tmp/ccci_mdinit.log 2>&1 </dev/null &
```

Con `setsid`, el proceso se convierte en líder de sesión y es inmune al SIGHUP
del shell padre. `ps` mostrará `ppid=1` (adoptado por init).

---

## Script de inicializacion manual del modem

El archivo `/data/local/tmp/modem_init.sh` en el dispositivo permite reproducir
el arranque del modem sin un boot.img reconstruido:

```sh
#!/system/bin/sh
# modem_init.sh — boot MT6575 modem manualmente (pruebas)

# 1. Permisos pvrsrvkm (ueventd los resetea a 0600 en cada boot)
chmod 0666 /dev/pvrsrvkm

# 2. Cargar modulos stock desde /data/local/tmp
insmod /data/local/tmp/ccci_plat.ko
insmod /data/local/tmp/ccci.ko     # version con patch timer 20s
insmod /system/lib/modules/ccmni.ko

# 3. Crear nodos CCCI
mknod /dev/ccci_fs c 178 0;   chmod 0660 /dev/ccci_fs;   chown 1001 /dev/ccci_fs
mknod /dev/ccci_rpc c 178 1;  chmod 0660 /dev/ccci_rpc;  chown 1001 /dev/ccci_rpc
mknod /dev/ccci_sys_rx c 184 2; chmod 0660 /dev/ccci_sys_rx; chown 1001 /dev/ccci_sys_rx
mknod /dev/ccci_sys_tx c 184 3; chmod 0660 /dev/ccci_sys_tx; chown 1001 /dev/ccci_sys_tx
mknod /dev/ccci_pcm_rx c 184 4; chmod 0660 /dev/ccci_pcm_rx; chown 1001 /dev/ccci_pcm_rx
mknod /dev/ccci_pcm_tx c 184 5; chmod 0660 /dev/ccci_pcm_tx; chown 1001 /dev/ccci_pcm_tx
mknod /dev/ccci_ipc_1220_0 c 183 0; chmod 0660 /dev/ccci_ipc_1220_0; chown 1001 /dev/ccci_ipc_1220_0
mknod /dev/ccci_uem_rx c 184 18; chmod 0660 /dev/ccci_uem_rx; chown 1001 /dev/ccci_uem_rx
mknod /dev/ccci_uem_tx c 184 19; chmod 0660 /dev/ccci_uem_tx; chown 1001 /dev/ccci_uem_tx
mknod /dev/ccci_md_log_rx c 184 42; chmod 0660 /dev/ccci_md_log_rx; chown 1001 /dev/ccci_md_log_rx
mknod /dev/ccci_md_log_tx c 184 43; chmod 0660 /dev/ccci_md_log_tx; chown 1001 /dev/ccci_md_log_tx
mkdir -p /dev/radio; chmod 0770 /dev/radio; chown 1001:1001 /dev/radio
mkdir -p /data/nvram/md
chown radio radio /sys/class/BOOT/BOOT/boot/md

# 4. Daemons CCCI (setsid para inmunidad a SIGHUP)
setsid /system/bin/ccci_rpcd  >/data/local/tmp/ccci_rpcd.log  2>&1 </dev/null &
sleep 1
setsid /system/bin/ccci_fsd   >/data/local/tmp/ccci_fsd.log   2>&1 </dev/null &
sleep 1
setsid /system/bin/ccci_mdinit >/data/local/tmp/ccci_mdinit.log 2>&1 </dev/null &
```

---

## Reglas de seguridad

1. **NUNCA `rmmod ccci*` con el modem activo o tras un WDT.**
   Produce kernel panic. El único reset seguro es `adb reboot`.

2. **No parchear `boot.img` con Python.** El gzip de Python usa `os=255`; el
   kernel MT6575 requiere `os=3`. El resultado es pánico VFS al arrancar.

3. **No escribir particiones sin confirmar el formato MTK.** Verificar que el
   archivo fue creado con `mtkbootimg` antes de cualquier `dd`.

4. **Conservar backups antes de builds limpias:**
   ```bash
   adb shell "su -c 'dd if=/dev/bootimg of=/sdcard/boot-working.img'"
   adb pull /sdcard/boot-working.img .
   adb pull /system/lib/modules/ccci.ko ./ccci.ko.phone-working
   ```

---

## Advertencia de reproducibilidad

El telefono que boozeaba durante la depuracion no coincidia exactamente con
las fuentes en el momento de la comprobacion:

- El telefono tenia un `system/lib/modules/ccci.ko` distinto al blob local
  (el blob del arbol ahora tiene el patch del timer, el del telefono no).
- Los `init*.rc` vivos en el ramdisk del telefono son mas viejos que los de
  `device/lge/vee5ss/rootdir/` — no incluyen las definiciones de servicios
  para `ccci_mdinit`, `ccci_fsd`, `ccci_rpcd`, ni `gsm0710muxd`.
- El ZIP previo en `out/target/product/vee5ss` estaba desfasado respecto a
  `vee5ss-vendor-blobs.mk`; una build limpia incluiria mas archivos.

Antes de borrar `out/` hacer siempre el backup mencionado arriba.

---

## Pendiente

### Inmediato: completar la inicializacion del modem

La prueba con `ccci.ko` parcheado (20 s) muestra que:
- El modem completa la calibracion RPC (`ADC_RFTMP`) en ~0.36 s ✅
- La carga de NVRAM via `ccci_fsd` no completa en 20 s ⚠️

Líneas de investigación:
1. **¿Por qué ccci_fsd no sirve archivos NVRAM?** — el daemon corre
   (`file open by ccci_fsd` en dmesg) pero no hay actividad de archivos en logcat.
2. **Archivos NVRAM faltantes en el backup:** `NVD_IMEI/ST6TA001`, `NVD_IMEI/ST6TB001`,
   `IMPORTNT/ST33A004`, `IMPORTNT/ST33B004` — estos son slots dual-SIM que el
   modem pide aunque el E450g es single-SIM. `ccci_fsd` responde con `-9` (ENOENT).
3. **Arrancar `gsm0710muxd` manualmente** una vez que el modem llegue a
   `MD_STATE_READY` y verificar que `/dev/radio/ptty*` aparecen.

### Reconstruir boot.img

El `init.mt6575.rc` actual en el árbol contiene los cambios necesarios:
- `mkdir /storage 0755 root root` + `mkdir /storage/sdcard0` en `on init`
- `EXTERNAL_STORAGE=/storage/sdcard0`, symlinks `/sdcard` y `/mnt/sdcard` → sdcard0
- Nodos CCCI via `mknod` en `on boot`
- Servicios `ccci_rpcd`, `ccci_fsd`, `ccci_mdinit`, `gsm0710muxd` declarados
  con `class core` y `oneshot`
- Servicio `sdcard /data/media 1023 1023` con sintaxis ICS correcta
- `chmod 0666 /dev/pvrsrvkm` permanente en `ueventd.mt6575.rc`
- `vold.fstab` en `device/lge/vee5ss/` → `system/etc/vold.fstab` (fix SD externa)

Cuando se toca `init.mt6575.rc` **y** el overlay de framework:
```bash
cd /home/chijure/cm10
. build/envsetup.sh
lunch cm_vee5ss-eng
rm -f out/target/product/vee5ss/boot.img out/target/product/vee5ss/ramdisk.img \
       out/target/product/vee5ss/root/init.mt6575.rc \
       out/target/product/vee5ss/system/framework/framework-res.apk \
       out/target/product/vee5ss/obj/APPS/framework-res_intermediates/package.apk
make bootimage framework-res
```

Comando para reconstruir solo el boot.img:

```bash
cd /home/chijure/cm10
. build/envsetup.sh
lunch cm_vee5ss-eng
rm -f out/target/product/vee5ss/boot.img out/target/product/vee5ss/ramdisk.img
make bootimage
```

Flashear en el dispositivo:

```bash
adb push out/target/product/vee5ss/boot.img /cache/boot.img
adb shell "dd if=/cache/boot.img of=/dev/bootimg bs=4096; sync"
adb reboot
```

Validar despues del reboot:

```bash
adb shell md5sum /init.mt6575.rc /ueventd.mt6575.rc
adb shell ls -l /dev/pvrsrvkm
```

No usar `dd` directo a `/dev/block/mmcblk0 bs=512 seek=36864` como metodo
principal. En este dispositivo ese comando no actualizo el ramdisk que termino
arrancando; el metodo verificado es escribir al nodo MTK `/dev/bootimg`.

---

## Archivos del device tree

| Archivo | Proposito |
| --- | --- |
| `AndroidProducts.mk` | registra el producto CM |
| `cm.mk` | product makefile principal |
| `device.mk` | paquetes, props y `PRODUCT_COPY_FILES` del dispositivo |
| `BoardConfig.mk` | plataforma MT6575, particiones, kernel y boot image |
| `bootimg.mk` | receta custom para `mtkbootimg` |
| `kernel` | kernel prebuilt activo usado por `TARGET_PREBUILT_KERNEL` |
| `rootdir/init.mt6575.rc` | init principal: insmod, mknod CCCI, servicios, paths de storage |
| `rootdir/init.mt6575.usb.rc` | USB, MTP y ADB |
| `rootdir/ueventd.mt6575.rc` | permisos de nodos `/dev` |
| `rootdir/fstab.mt6575` | montajes de Android |
| `vold.fstab` | config de vold — `sdcard1` → `/storage/sdcard1` via sysfs path MTK |
| `rootdir/recovery.fstab` | particiones para recovery |
| `overlay/frameworks/base/core/res/res/xml/storage_list.xml` | define volúmenes para MountService y Settings > Storage |
| `overlay/frameworks/base/core/res/res/values/config.xml` | config WiFi y animación |
| `mtkbootimg/` | host tool para boot images MTK |

---

## Vendor tree

Los blobs propietarios viven en:

```text
vendor/lge/vee5ss/proprietary/
```

El producto hereda:

```make
$(call inherit-product-if-exists, vendor/lge/vee5ss/vee5ss-vendor-blobs.mk)
```

Blobs criticos declarados por `vee5ss-vendor-blobs.mk`:

- RIL: `lib/mtk-ril.so`, `lib/librilmtk.so`, `lib/libutilrilmtk.so`
- GPU: `vendor/bin/pvrsrvctl`, `vendor/lib/egl/*`,
  `vendor/lib/hw/gralloc.mt6575.so`, `vendor/lib/libsrv_*.so`
- Kernel modules: `pvrsrvkm.ko`, `mtklfb.ko`, `ccci_plat.ko`, `ccci.ko`
  (patched), `ccmni.ko`, `wlan.ko`, módulos WMT/STP
- Modem userspace: `ccci_mdinit`, `ccci_fsd`, `ccci_rpcd`, `gsm0710muxd`
- Firmware modem: `etc/firmware/modem.img` (5085360 bytes, 2013/03/22, MT6575_S01),
  `etc/firmware/DSP_ROM` (688960 bytes, 2013/01/08, MAUI.11AMDW1222_TC1_SP.W12.48)
- NVRAM/LG: `libnvram.so`, `libcustom_nvram.so`,
  `libnvram_daemon_callback.so`, `liblgpart.so`, `liblgpclient_jni.so`

---

## Boot image MTK

No parchear `boot.img` con scripts Python ni reempaquetarlo con el `mkbootimg`
AOSP normal. En este equipo eso produjo panics de VFS por imagen invalida.

Usar siempre el flujo del arbol:

```make
BOARD_CUSTOM_BOOTIMG    := true
BOARD_CUSTOM_MKBOOTIMG  := $(HOST_OUT_EXECUTABLES)/mtkbootimg$(HOST_EXECUTABLE_SUFFIX)
BOARD_CUSTOM_BOOTIMG_MK := device/lge/vee5ss/bootimg.mk
```

---

## Build

Las compilaciones se hacen dentro del contenedor Docker `cm10-builder`,
con `/home/chijure/cm10` montado como `/home/builder/cm10`.

Build completo:

```bash
docker exec -d cm10-builder bash -c \
  'cd /home/builder/cm10 && source build/envsetup.sh && breakfast vee5ss && make -j$(nproc) bacon > /tmp/bacon_build.log 2>&1'
docker exec cm10-builder tail -50 /tmp/bacon_build.log
```

Build local equivalente:

```bash
cd /home/chijure/cm10
. build/envsetup.sh
lunch cm_vee5ss-eng
make -j$(nproc) bacon
```

Solo boot image:

```bash
cd /home/chijure/cm10
. build/envsetup.sh
lunch cm_vee5ss-eng
rm -f out/target/product/vee5ss/boot.img out/target/product/vee5ss/ramdisk.img
make bootimage
```

---

## Flash con TWRP

Procedimiento usado durante el port:

```bash
adb push out/target/product/vee5ss/cm-10-*-UNOFFICIAL-vee5ss.zip /cache/OTA.zip
adb shell twrp install /cache/OTA.zip
adb push out/target/product/vee5ss/boot.img /cache/boot.img
adb shell "dd if=/cache/boot.img of=/dev/bootimg bs=4096; sync"
adb reboot
```

No escribir particiones sin confirmar que el archivo corresponde al formato MTK
correcto y que existe un backup recuperable.

Validacion post-flash del ramdisk:

```bash
adb shell md5sum /init.mt6575.rc /ueventd.mt6575.rc
adb shell ls -l /dev/pvrsrvkm
```

Para el boot con el fix de GPU, los hashes esperados fueron:

```text
55334b5b8b0d768b3a348ab9f919ec10  /init.mt6575.rc
b972e890de1867b3061bc02a29f0faa9  /ueventd.mt6575.rc
crw-rw-rw- root root 228, 0 /dev/pvrsrvkm
```

---

## Debug rapido

```bash
# Estado general
adb wait-for-device
adb shell getprop sys.boot_completed
adb shell dmesg | grep -iE 'pvr|sgx|ccci|modem|wdt|ril|fatal|panic'
adb logcat -b main -b system -b radio

# CCCI / modem
adb shell ls -l /dev/pvrsrvkm /dev/ccci* /dev/ttyC* /dev/radio 2>/dev/null
adb shell ps | grep -E 'rild|ccci|gsm0710|pvrsrv'
adb shell getprop gsm.sim.state
adb shell dmesg | grep -E 'NORMAL_BOOT|BOOT_ID|Time out|md_boot|ttyC.*not ready'

# Verificar blobs del telefono contra el arbol local
adb shell md5sum /system/lib/mtk-ril.so \
                 /system/vendor/bin/pvrsrvctl \
                 /system/lib/modules/ccci.ko
md5sum vendor/lge/vee5ss/proprietary/lib/mtk-ril.so \
       vendor/lge/vee5ss/proprietary/vendor/bin/pvrsrvctl \
       vendor/lge/vee5ss/proprietary/lib/modules/ccci.ko
```

---

## Kernel stock/custom relacionado

Notas utiles desde `/home/chijure/e450-build/fuentes/CLAUDE.md`:

- `pvrsrvkm.ko` compilado funciona si el kernel se construye con
  `TARGET_BUILD_VARIANT=user`; con variant `eng` se rompe el ABI del ioctl PVR.
- `mtklfb.ko` debe ser compatible con el ABI del MT6575.
- En stock, `/system/lib/modules/wlan.ko` puede ser el mismo binario que
  `wlan_mt6620.ko`.
- Botones: BACK y MENU salen por `mtk-tpd-kpd`; HOME fisico por `mtk-kpd`.

Estas notas son del trabajo de kernel y no reemplazan la validacion del userspace
CM10, pero explican por que algunos modulos compilados pueden bootear mientras
otros deben mantenerse como blobs stock.

---

## Fuentes de referencia locales

```text
/home/chijure/.claude/projects/-home-chijure-cm10/memory/
/home/chijure/e450-build/fuentes/CLAUDE.md
/home/chijure/twrp-4.4/device/lge/vee5ss/
/home/chijure/Documentos/2026-06-16--08-31-05_JZO54K/   # backup stock TWRP
/home/chijure/e450-build/system_lib_modules_backup/      # modulos .ko stock
/home/chijure/Documentos/ccci_extract/                   # binarios CCCI stock
```
