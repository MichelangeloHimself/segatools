#include <windows.h>

#include <stddef.h>
#include <stdlib.h>

#include "amex/amex.h"
#include "amex/config.h"

#include "chunihook/jvs.h"
#include "chunihook/slider.h"

#include "chuniio/chuniio.h"

#include "hook/process.h"

#include "hooklib/clock.h"
#include "hooklib/gfx.h"
#include "hooklib/serial.h"
#include "hooklib/spike.h"

#include "platform/hwmon.h"
#include "platform/nusec.h"

#include "util/dprintf.h"

static process_entry_t chuni_startup;

static DWORD CALLBACK chuni_pre_startup(void)
{
    struct amex_config amex_cfg;
    HMODULE d3dc;

    dprintf("--- Begin chuni_pre_startup ---\n");

    /* Pin the D3D shader compiler. This makes startup much faster. */

    d3dc = LoadLibraryW(L"D3DCompiler_43.dll");

    if (d3dc != NULL) {
        dprintf("Pinned shader compiler, hMod=%p\n", d3dc);
    } else {
        dprintf("Failed to load shader compiler!\n");
    }

    /* Hook Win32 APIs */

    clock_hook_init();
    gfx_hook_init();
    serial_hook_init();

    /* Initialize platform API emulation */

    hwmon_hook_init();
    nusec_hook_init();

    /* Initialize AMEX emulation */

    amex_config_load(&amex_cfg, L".\\segatools.ini");
    amex_hook_init(&amex_cfg);

    /* Initialize Chunithm board emulation */

    if (amex_cfg.jvs.enable) {
        chunithm_jvs_init();
    }

    slider_hook_init();

    /* Initialize debug helpers */

    spike_hook_init("chunispike.txt");
    gfx_set_windowed();

    dprintf("---  End  chuni_pre_startup ---\n");

    /* Initialize IO DLL */

    chuni_io_init();

    /* Jump to EXE start address */

    return chuni_startup();
}

BOOL WINAPI DllMain(HMODULE mod, DWORD cause, void *ctx)
{
    HRESULT hr;

    if (cause != DLL_PROCESS_ATTACH) {
        return TRUE;
    }

    hr = process_hijack_startup(chuni_pre_startup, &chuni_startup);

    if (!SUCCEEDED(hr)) {
        dprintf("Failed to hijack process startup: %x\n", (int) hr);
    }

    return SUCCEEDED(hr);
}
