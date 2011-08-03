#include <linux/limits.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <dlfcn.h>

#include "simpleconfig.h"
#include "static_map.h"
#include "server_plugin.h"

#include "tests.h"


#define PLUGIN_PATH "../server"


static const backend_plugin_t *load_plugin(const char *name)
{
    void *handle;
    char fn[PATH_MAX];
    backend_plugin_t *(*modinfo)(void);

    snprintf(fn, PATH_MAX, "%s/%s.so", PLUGIN_PATH, name);

    handle = dlopen(fn, RTLD_NOW);
    if (!handle) {
        fprintf(stderr, "Failed to load plugin \"%s\".\n", fn);
        exit(1);
    }

    modinfo = dlsym(handle, BACKEND_INFO_STR);
    if (!modinfo) {
        fprintf(stderr, "Failed to find %s.\n", BACKEND_INFO_STR);
        exit(1);
    }

    return modinfo();
}

int main(int argc, char *argv[])
{
    const backend_plugin_t *plugin = load_plugin("libvirt-qpid");

    fprintf(stderr, "Loaded plugin at %p\n", plugin);

    return test_plugin(plugin, 49000) + test_plugin(plugin, 5672);
}

