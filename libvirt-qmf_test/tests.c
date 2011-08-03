#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include "simpleconfig.h"
#include "static_map.h"
#include "server_plugin.h"

#include "tests.h"


#define TEST_DOMAIN "libvirt-qpid-dev"


typedef int (*test_fn)(const fence_callbacks_t *, const backend_context_t);

struct test {
    test_fn func;
    const char *name;
};

#define TEST_LIST static const struct test tests[] =
#define TEST(func) {func, #func},


static int running = 0;


static int test_on(const fence_callbacks_t *cb, const backend_context_t ctx)
{
    running = 1;
    return cb->on(TEST_DOMAIN, NULL, 0, ctx);
}

static int test_on_fail(const fence_callbacks_t *cb, const backend_context_t ctx)
{
    return !cb->on("bogus", NULL, 0, ctx);
}

static int test_reboot(const fence_callbacks_t *cb, const backend_context_t ctx)
{
    return cb->reboot(TEST_DOMAIN, NULL, 0, ctx);
}

static int test_off(const fence_callbacks_t *cb, const backend_context_t ctx)
{
    running = 0;
    return cb->off(TEST_DOMAIN, NULL, 0, ctx);
}

struct host {
    char *hostname;
    char *uuid;
    int running;
};

struct hostlist {
    size_t len;
    struct host *hosts;
};

static int host_list_append(const char *name,
        const char *uuid,
        int state,
        void *arg)
{
    struct hostlist *list = arg;
    struct host *newhosts = realloc(list->hosts, sizeof(struct host) * ++list->len);

    if (newhosts) {
        struct host *h = &newhosts[list->len - 1];
        list->hosts = newhosts;

        h->hostname = strdup(name);
        h->uuid = strdup(uuid);
        h->running = state;
    }

    return 0;
}

static void host_list_free(struct hostlist *hostlist)
{
    size_t i;

    for (i = 0; i < hostlist->len; i++) {
        struct host *h = &hostlist->hosts[i];

        free(h->hostname);
        free(h->uuid);
    }

    free(hostlist->hosts);
    hostlist->len = 0;
    hostlist->hosts = NULL;
}

static int test_list(const fence_callbacks_t *cb, const backend_context_t ctx)
{
    size_t i;
    int result;
    struct hostlist hostlist = {0, NULL};

    result = cb->hostlist(&host_list_append, &hostlist, ctx);

    if (result) {
        goto exit;
    }

    result = 1;
    for (i = 0; i < hostlist.len; i++) {
        struct host *h = &hostlist.hosts[i];
        if (strcmp(h->hostname, TEST_DOMAIN) == 0) {
            printf("    Found host uuid=%s; %s\n",
                    h->uuid,
                    h->running ? "RUNNING" : "SHUTOFF");
            if (h->running == running) {
                result = 0;
            }
            break;
        }
    }

exit:
    host_list_free(&hostlist);
    return result;
}


TEST_LIST {
    TEST(test_list)
    TEST(test_on)
    TEST(test_list)
    TEST(test_reboot)
    TEST(test_list)
    TEST(test_off)
    TEST(test_list)
    TEST(test_on_fail)
};


static uint16_t conf_port = 0;

int get_conf(void *obj, const char *key, char *value, size_t valuesz)
{
    if (conf_port && strcmp(key, "backends/libvirt-qpid/@port") == 0) {
        snprintf(value, valuesz, "%hu", conf_port);
        return 0;
    }

    return 1; // otherwise use defaults
}

int test_plugin(const backend_plugin_t *plugin, uint16_t port)
{
    int i;
    int errors = 0;
    backend_context_t context;
    config_object_t conf = {&get_conf};

    printf("Testing plugin %s (%s) on port %hu...\n",
            plugin->name, plugin->version, port);

    conf_port = port;
    plugin->init(&context, &conf);

    for (i = 0; i < (sizeof(tests) / sizeof(*tests)); i++) {
        const struct test *t = tests + i;
        printf("  running %s\n", t->name);
        if (t->func(plugin->callbacks, context)) {
            errors++;
            puts("    FAIL\n");
        } else {
            puts("    PASS\n");
        }
    }

    printf("%d test%s completed with %d error%s.\n\n",
            i, i == 1 ? "" : "s",
            errors, errors == 1 ? "" : "s");

    plugin->cleanup(context);

    return errors;
}

