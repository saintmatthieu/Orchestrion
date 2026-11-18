/*
 * This file is part of Orchestrion.
 *
 * Copyright (C) 2026 Matthieu Hodgkinson
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 */
#include "PipeWireGraphRate.h"

#include <pipewire/extensions/metadata.h>
#include <pipewire/pipewire.h>
#include <spa/utils/result.h>

#include <cstdlib>
#include <cstring>
#include <string>

namespace dgk
{
namespace
{
struct Query
{
  pw_main_loop *loop = nullptr;
  pw_core *core = nullptr;
  pw_registry *registry = nullptr;
  pw_metadata *metadata = nullptr;
  spa_hook coreHook{};
  spa_hook registryHook{};
  spa_hook metadataHook{};
  int pendingSync = 0;
  int syncsLeft = 2; // globals listed, then the metadata's properties
  muse::audio::sample_rate_t rate = 0;
  muse::audio::sample_rate_t forcedRate = 0;
};

muse::audio::sample_rate_t toRate(const char *value)
{
  return value ? std::strtoull(value, nullptr, 10) : 0;
}

int onProperty(void *data, uint32_t subject, const char *key, const char *,
               const char *value)
{
  auto &q = *static_cast<Query *>(data);
  if (subject != PW_ID_CORE || !key)
    return 0;
  if (std::strcmp(key, "clock.rate") == 0)
    q.rate = toRate(value);
  else if (std::strcmp(key, "clock.force-rate") == 0)
    q.forcedRate = toRate(value);
  return 0;
}

const pw_metadata_events metadataEvents = []
{
  pw_metadata_events e{};
  e.version = PW_VERSION_METADATA_EVENTS;
  e.property = onProperty;
  return e;
}();

void onGlobal(void *data, uint32_t id, uint32_t, const char *type, uint32_t,
              const spa_dict *props)
{
  auto &q = *static_cast<Query *>(data);
  if (q.metadata || std::strcmp(type, PW_TYPE_INTERFACE_Metadata) != 0)
    return;
  const char *name =
      props ? spa_dict_lookup(props, PW_KEY_METADATA_NAME) : nullptr;
  if (!name || std::strcmp(name, "settings") != 0)
    return;
  q.metadata = static_cast<pw_metadata *>(pw_registry_bind(
      q.registry, id, PW_TYPE_INTERFACE_Metadata, PW_VERSION_METADATA, 0));
  if (q.metadata)
    pw_metadata_add_listener(q.metadata, &q.metadataHook, &metadataEvents, &q);
}

const pw_registry_events registryEvents = []
{
  pw_registry_events e{};
  e.version = PW_VERSION_REGISTRY_EVENTS;
  e.global = onGlobal;
  return e;
}();

void onDone(void *data, uint32_t id, int seq)
{
  auto &q = *static_cast<Query *>(data);
  if (id != PW_ID_CORE || seq != q.pendingSync)
    return;
  if (--q.syncsLeft > 0)
    q.pendingSync = pw_core_sync(q.core, PW_ID_CORE, q.pendingSync);
  else
    pw_main_loop_quit(q.loop);
}

void onError(void *data, uint32_t id, int, int, const char *)
{
  if (id == PW_ID_CORE)
    pw_main_loop_quit(static_cast<Query *>(data)->loop);
}

const pw_core_events coreEvents = []
{
  pw_core_events e{};
  e.version = PW_VERSION_CORE_EVENTS;
  e.done = onDone;
  e.error = onError;
  return e;
}();

void onTimeout(void *data, uint64_t)
{
  pw_main_loop_quit(static_cast<Query *>(data)->loop);
}
} // namespace

std::optional<muse::audio::sample_rate_t> pipeWireGraphRate()
{
  pw_init(nullptr, nullptr);

  Query q;
  q.loop = pw_main_loop_new(nullptr);
  if (!q.loop)
    return std::nullopt;
  pw_loop *const loop = pw_main_loop_get_loop(q.loop);
  pw_context *const context = pw_context_new(loop, nullptr, 0);
  if (context)
    q.core = pw_context_connect(context, nullptr, 0);
  if (q.core)
  {
    pw_core_add_listener(q.core, &q.coreHook, &coreEvents, &q);
    q.registry = pw_core_get_registry(q.core, PW_VERSION_REGISTRY, 0);
    pw_registry_add_listener(q.registry, &q.registryHook, &registryEvents, &q);
    q.pendingSync = pw_core_sync(q.core, PW_ID_CORE, 0);

    spa_source *const timer = pw_loop_add_timer(loop, onTimeout, &q);
    timespec timeout{.tv_sec = 0, .tv_nsec = 500'000'000};
    pw_loop_update_timer(loop, timer, &timeout, nullptr, false);
    pw_main_loop_run(q.loop);
    pw_loop_destroy_source(loop, timer);

    if (q.metadata)
    {
      spa_hook_remove(&q.metadataHook);
      pw_proxy_destroy(reinterpret_cast<pw_proxy *>(q.metadata));
    }
    spa_hook_remove(&q.registryHook);
    pw_proxy_destroy(reinterpret_cast<pw_proxy *>(q.registry));
    spa_hook_remove(&q.coreHook);
    pw_core_disconnect(q.core);
  }
  if (context)
    pw_context_destroy(context);
  pw_main_loop_destroy(q.loop);

  const muse::audio::sample_rate_t rate = q.forcedRate ? q.forcedRate : q.rate;
  return rate ? std::optional{rate} : std::nullopt;
}
} // namespace dgk
