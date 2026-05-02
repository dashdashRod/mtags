#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wayland-client.h>
#include "ext-workspace-v1-protocol.h"
#include "wlr-foreign-toplevel-management-unstable-v1-protocol.h"
#include "dwl-ipc-unstable-v2-protocol.h"


static struct ext_workspace_manager_v1 *ws_manager = NULL;
static struct zwlr_foreign_toplevel_manager_v1 *tl_manager = NULL;
static struct zdwl_ipc_manager_v2 *ipc_manager = NULL;

/* ---- workspace handle listener ---- */

static void ws_id(void *data, struct ext_workspace_handle_v1 *ws, const char *id) {
    printf("  workspace %p: id=%s\n", (void*)ws, id);
}
static void ws_name(void *data, struct ext_workspace_handle_v1 *ws, const char *name) {
    printf("  workspace %p: name=%s\n", (void*)ws, name);
}
static void ws_coordinates(void *data, struct ext_workspace_handle_v1 *ws, struct wl_array *coords) {}
static void ws_state(void *data, struct ext_workspace_handle_v1 *ws, uint32_t state) {
    printf("  workspace %p: state=", (void*)ws);
    if (state & 1) printf("active ");
    if (state & 2) printf("urgent ");
    if (state & 4) printf("hidden ");
    printf("\n");
}
static void ws_capabilities(void *data, struct ext_workspace_handle_v1 *ws, uint32_t caps) {}
static void ws_removed(void *data, struct ext_workspace_handle_v1 *ws) {}

static const struct ext_workspace_handle_v1_listener ws_listener = {
    .id = ws_id,
    .name = ws_name,
    .coordinates = ws_coordinates,
    .state = ws_state,
    .capabilities = ws_capabilities,
    .removed = ws_removed,
};

/* ---- workspace group listener ---- */

static void group_capabilities(void *data, struct ext_workspace_group_handle_v1 *g, uint32_t caps) {}
static void group_output_enter(void *data, struct ext_workspace_group_handle_v1 *g, struct wl_output *o) {}
static void group_output_leave(void *data, struct ext_workspace_group_handle_v1 *g, struct wl_output *o) {}
static void group_workspace_enter(void *data, struct ext_workspace_group_handle_v1 *g,
                                  struct ext_workspace_handle_v1 *ws) {
    printf("group %p: workspace %p entered\n", (void*)g, (void*)ws);
}
static void group_workspace_leave(void *data, struct ext_workspace_group_handle_v1 *g,
                                  struct ext_workspace_handle_v1 *ws) {}
static void group_removed(void *data, struct ext_workspace_group_handle_v1 *g) {}

static const struct ext_workspace_group_handle_v1_listener group_listener = {
    .capabilities = group_capabilities,
    .output_enter = group_output_enter,
    .output_leave = group_output_leave,
    .workspace_enter = group_workspace_enter,
    .workspace_leave = group_workspace_leave,
    .removed = group_removed,
};

/* ---- workspace manager listener ---- */

static void mgr_workspace_group(void *data, struct ext_workspace_manager_v1 *m,
                                struct ext_workspace_group_handle_v1 *g) {
    printf("new group: %p\n", (void*)g);
    ext_workspace_group_handle_v1_add_listener(g, &group_listener, NULL);
}
static void mgr_workspace(void *data, struct ext_workspace_manager_v1 *m,
                          struct ext_workspace_handle_v1 *ws) {
    printf("new workspace: %p\n", (void*)ws);
    ext_workspace_handle_v1_add_listener(ws, &ws_listener, NULL);
}
static void mgr_done(void *data, struct ext_workspace_manager_v1 *m) {
    printf("--- workspace state complete ---\n");
}
static void mgr_finished(void *data, struct ext_workspace_manager_v1 *m) {}

static const struct ext_workspace_manager_v1_listener mgr_listener = {
    .workspace_group = mgr_workspace_group,
    .workspace = mgr_workspace,
    .done = mgr_done,
    .finished = mgr_finished,
};

/* ---- toplevel handle listener ---- */

static void tl_title(void *data, struct zwlr_foreign_toplevel_handle_v1 *h, const char *title) {
    printf("  toplevel %p: title=\"%s\"\n", (void*)h, title);
}
static void tl_app_id(void *data, struct zwlr_foreign_toplevel_handle_v1 *h, const char *app_id) {
    printf("  toplevel %p: app_id=\"%s\"\n", (void*)h, app_id);
}
static void tl_output_enter(void *data, struct zwlr_foreign_toplevel_handle_v1 *h, struct wl_output *o) {
    printf("  toplevel %p: output_enter %p\n", (void*)h, (void*)o);
}
static void tl_output_leave(void *data, struct zwlr_foreign_toplevel_handle_v1 *h, struct wl_output *o) {
    printf("  toplevel %p: output_leave %p\n", (void*)h, (void*)o);
}
static void tl_state(void *data, struct zwlr_foreign_toplevel_handle_v1 *h, struct wl_array *state) {
    printf("  toplevel %p: state=[", (void*)h);
    uint32_t *s;
    wl_array_for_each(s, state) {
        switch (*s) {
            case ZWLR_FOREIGN_TOPLEVEL_HANDLE_V1_STATE_MAXIMIZED:  printf("maximized "); break;
            case ZWLR_FOREIGN_TOPLEVEL_HANDLE_V1_STATE_MINIMIZED:  printf("minimized "); break;
            case ZWLR_FOREIGN_TOPLEVEL_HANDLE_V1_STATE_ACTIVATED:  printf("activated "); break;
            case ZWLR_FOREIGN_TOPLEVEL_HANDLE_V1_STATE_FULLSCREEN: printf("fullscreen "); break;
        }
    }
    printf("]\n");
}
static void tl_done(void *data, struct zwlr_foreign_toplevel_handle_v1 *h) {
    printf("  toplevel %p: done\n", (void*)h);
}
static void tl_closed(void *data, struct zwlr_foreign_toplevel_handle_v1 *h) {
    printf("  toplevel %p: closed\n", (void*)h);
}
static void tl_parent(void *data, struct zwlr_foreign_toplevel_handle_v1 *h,
                      struct zwlr_foreign_toplevel_handle_v1 *parent) {}

static const struct zwlr_foreign_toplevel_handle_v1_listener tl_listener = {
    .title = tl_title,
    .app_id = tl_app_id,
    .output_enter = tl_output_enter,
    .output_leave = tl_output_leave,
    .state = tl_state,
    .done = tl_done,
    .closed = tl_closed,
    .parent = tl_parent,
};

/* ---- toplevel manager listener ---- */

static void tl_mgr_toplevel(void *data, struct zwlr_foreign_toplevel_manager_v1 *m,
                            struct zwlr_foreign_toplevel_handle_v1 *h) {
    printf("new toplevel: %p\n", (void*)h);
    zwlr_foreign_toplevel_handle_v1_add_listener(h, &tl_listener, NULL);
}
static void tl_mgr_finished(void *data, struct zwlr_foreign_toplevel_manager_v1 *m) {}

static const struct zwlr_foreign_toplevel_manager_v1_listener tl_mgr_listener = {
    .toplevel = tl_mgr_toplevel,
    .finished = tl_mgr_finished,
};

/* ---- dwl-ipc output listener ---- */

static void ipc_out_toggle_visibility(void *data, struct zdwl_ipc_output_v2 *o) {}
static void ipc_out_active(void *data, struct zdwl_ipc_output_v2 *o, uint32_t active) {}
static void ipc_out_tag(void *data, struct zdwl_ipc_output_v2 *o,
                        uint32_t tag, uint32_t state, uint32_t clients, uint32_t focused) {}
static void ipc_out_layout(void *data, struct zdwl_ipc_output_v2 *o, uint32_t layout) {}
static void ipc_out_title(void *data, struct zdwl_ipc_output_v2 *o, const char *title) {}
static void ipc_out_appid(void *data, struct zdwl_ipc_output_v2 *o, const char *appid) {}
static void ipc_out_layout_symbol(void *data, struct zdwl_ipc_output_v2 *o, const char *symbol) {}
static void ipc_out_frame(void *data, struct zdwl_ipc_output_v2 *o) {
    printf("  --- ipc frame ---\n");
}
static void ipc_out_fullscreen(void *data, struct zdwl_ipc_output_v2 *o, uint32_t fs) {}
static void ipc_out_floating(void *data, struct zdwl_ipc_output_v2 *o, uint32_t fl) {}
static void ipc_out_x(void *data, struct zdwl_ipc_output_v2 *o, int32_t x) {}
static void ipc_out_y(void *data, struct zdwl_ipc_output_v2 *o, int32_t y) {}
static void ipc_out_width(void *data, struct zdwl_ipc_output_v2 *o, int32_t w) {}
static void ipc_out_height(void *data, struct zdwl_ipc_output_v2 *o, int32_t h) {}
static void ipc_out_last_layer(void *data, struct zdwl_ipc_output_v2 *o, const char *l) {}
static void ipc_out_kb_layout(void *data, struct zdwl_ipc_output_v2 *o, const char *l) {}
static void ipc_out_keymode(void *data, struct zdwl_ipc_output_v2 *o, const char *m) {}
static void ipc_out_scalefactor(void *data, struct zdwl_ipc_output_v2 *o, uint32_t s) {}

static void ipc_out_client(void *data, struct zdwl_ipc_output_v2 *o,
                           const char *appid, const char *title, uint32_t tagmask) {
    printf("  ipc client: appid=\"%s\" title=\"%s\" tagmask=0x%x\n",
           appid, title, tagmask);
}

static const struct zdwl_ipc_output_v2_listener ipc_out_listener = {
    .toggle_visibility = ipc_out_toggle_visibility,
    .active = ipc_out_active,
    .tag = ipc_out_tag,
    .layout = ipc_out_layout,
    .title = ipc_out_title,
    .appid = ipc_out_appid,
    .layout_symbol = ipc_out_layout_symbol,
    .frame = ipc_out_frame,
    .fullscreen = ipc_out_fullscreen,
    .floating = ipc_out_floating,
    .x = ipc_out_x,
    .y = ipc_out_y,
    .width = ipc_out_width,
    .height = ipc_out_height,
    .last_layer = ipc_out_last_layer,
    .kb_layout = ipc_out_kb_layout,
    .keymode = ipc_out_keymode,
    .scalefactor = ipc_out_scalefactor,
    .client = ipc_out_client,
};

/* ---- dwl-ipc manager listener ---- */

static void ipc_mgr_tags(void *data, struct zdwl_ipc_manager_v2 *m, uint32_t amount) {
    printf("ipc: tags amount=%u\n", amount);
}
static void ipc_mgr_layout(void *data, struct zdwl_ipc_manager_v2 *m, const char *name) {}

static const struct zdwl_ipc_manager_v2_listener ipc_mgr_listener = {
    .tags = ipc_mgr_tags,
    .layout = ipc_mgr_layout,
};

/* ---- wl_output (we just need it to bind a per-output ipc handle) ---- */

static void wl_output_geometry(void *data, struct wl_output *o, int32_t x, int32_t y,
                               int32_t pw, int32_t ph, int32_t sub, const char *make,
                               const char *model, int32_t transform) {}
static void wl_output_mode(void *data, struct wl_output *o, uint32_t flags,
                           int32_t w, int32_t h, int32_t refresh) {}
static void wl_output_done(void *data, struct wl_output *o) {}
static void wl_output_scale(void *data, struct wl_output *o, int32_t factor) {}
static void wl_output_name(void *data, struct wl_output *o, const char *name) {}
static void wl_output_description(void *data, struct wl_output *o, const char *desc) {}

static const struct wl_output_listener wl_output_listener_struct = {
    .geometry = wl_output_geometry,
    .mode = wl_output_mode,
    .done = wl_output_done,
    .scale = wl_output_scale,
    .name = wl_output_name,
    .description = wl_output_description,
};



/* ---- registry ---- */

static struct wl_output *the_output = NULL;
static struct zdwl_ipc_output_v2 *ipc_output_handle = NULL;

static void try_create_ipc_output(void) {
    if (ipc_manager && the_output && !ipc_output_handle) {
        ipc_output_handle = zdwl_ipc_manager_v2_get_output(ipc_manager, the_output);
        zdwl_ipc_output_v2_add_listener(ipc_output_handle, &ipc_out_listener, NULL);
        printf("ipc_output: bound\n");
    }
}

static void on_global(void *data, struct wl_registry *r, uint32_t name,
                      const char *iface, uint32_t version) {
    if (strcmp(iface, ext_workspace_manager_v1_interface.name) == 0) {
        ws_manager = wl_registry_bind(r, name, &ext_workspace_manager_v1_interface, 1);
        ext_workspace_manager_v1_add_listener(ws_manager, &mgr_listener, NULL);
        printf("bound: %s v%u\n", iface, version);
    } else if (strcmp(iface, zwlr_foreign_toplevel_manager_v1_interface.name) == 0) {
        tl_manager = wl_registry_bind(r, name, &zwlr_foreign_toplevel_manager_v1_interface, 3);
        zwlr_foreign_toplevel_manager_v1_add_listener(tl_manager, &tl_mgr_listener, NULL);
        printf("bound: %s v%u\n", iface, version);
    } else if (strcmp(iface, zdwl_ipc_manager_v2_interface.name) == 0) {
        ipc_manager = wl_registry_bind(r, name, &zdwl_ipc_manager_v2_interface, 3);
        zdwl_ipc_manager_v2_add_listener(ipc_manager, &ipc_mgr_listener, NULL);
        printf("bound: %s v%u\n", iface, version);
        try_create_ipc_output();
    } else if (strcmp(iface, wl_output_interface.name) == 0 && !the_output) {
        the_output = wl_registry_bind(r, name, &wl_output_interface, 4);
        wl_output_add_listener(the_output, &wl_output_listener_struct, NULL);
        printf("bound: %s v%u\n", iface, version);
        try_create_ipc_output();
    }
}

static void on_global_remove(void *data, struct wl_registry *r, uint32_t name) {}

static const struct wl_registry_listener registry_listener = {
    .global = on_global,
    .global_remove = on_global_remove,
};

int main(void) {
    struct wl_display *display = wl_display_connect(NULL);
    if (!display) { fprintf(stderr, "no display\n"); return 1; }

    struct wl_registry *registry = wl_display_get_registry(display);
    wl_registry_add_listener(registry, &registry_listener, NULL);

    wl_display_roundtrip(display);  // registry
    wl_display_roundtrip(display);  // workspace_group / workspace creation, toplevel creation
    wl_display_roundtrip(display);  // per-workspace + per-toplevel events
    wl_display_roundtrip(display);  // catch any strangler frame
    wl_display_disconnect(display);
    return 0;
}
