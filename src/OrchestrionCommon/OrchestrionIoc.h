/*
 * This file is part of Orchestrion.
 *
 * Copyright (C) 2024 Matthieu Hodgkinson
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
#pragma once

#include <functional>
#include <memory>
#include <modularity/imodulesetup.h>
#include <modularity/ioc.h>

namespace dgk
{
/**
 * The application's IoC context.
 *
 * MuseScore's framework scopes most of its services to a "context" (one per
 * window); Orchestrion is a single-window application with exactly one
 * context. It is created by the framework when the application sets it up
 * (see OrchestrionApp), and recorded here so that Orchestrion's own services,
 * which are constructed before that happens, can resolve context-scoped
 * MuseScore services lazily.
 */
inline muse::modularity::ContextPtr &iocContextStorage()
{
  static muse::modularity::ContextPtr ctx;
  return ctx;
}

inline muse::modularity::ContextPtr iocContext() { return iocContextStorage(); }

inline void setIocContext(const muse::modularity::ContextPtr &ctx)
{
  iocContextStorage() = ctx;
}

/**
 * A context getter for muse::Contextable that resolves the application
 * context on first use, so it may be handed out before the context exists.
 */
inline muse::Contextable::GetContext lazyIocContext()
{
  return [] { return iocContext(); };
}

/**
 * Base class for Orchestrion's injectable classes: a muse::Contextable bound
 * (lazily) to the application's single IoC context. Interfaces not found in
 * the context are resolved from the global IoC.
 */
class Injectable : public muse::Contextable
{
public:
  Injectable() : muse::Contextable(lazyIocContext()) {}
};

/**
 * Injection of a service by interface: MuseScore's framework requires
 * GlobalInject for global interfaces and ContextInject for context-scoped
 * ones; this picks the right one from the interface's declaration, so that
 * Orchestrion's classes can declare `dgk::Inject<I> name{this};` uniformly.
 */
template <class I, bool IsGlobal = I::modularity_isGlobalInterface()>
class Inject;

template <class I> class Inject<I, true> : public muse::GlobalInject<I>
{
public:
  Inject(const muse::Contextable * = nullptr) {}
};

template <class I> class Inject<I, false> : public muse::ContextInject<I>
{
public:
  Inject(const muse::Contextable *inj) : muse::ContextInject<I>(inj) {}
};

/**
 * An IContextSetup that records the application context and forwards the
 * per-context lifecycle hooks to a module's callbacks. Orchestrion's modules
 * use it to run their initialisation in the context phase, where MuseScore's
 * context-scoped services (playback, notation, ...) are available.
 */
class ModuleContextSetup : public muse::modularity::IContextSetup
{
public:
  struct Hooks
  {
    std::function<void()> registerExports;
    std::function<void()> resolveImports;
    std::function<void(const muse::IApplication::RunMode &)> onPreInit;
    std::function<void(const muse::IApplication::RunMode &)> onInit;
    std::function<void(const muse::IApplication::RunMode &)> onAllInited;
    std::function<void()> onDeinit;
  };

  ModuleContextSetup(const muse::modularity::ContextPtr &ctx, Hooks hooks)
      : muse::modularity::IContextSetup(ctx), m_hooks(std::move(hooks))
  {
    setIocContext(ctx);
  }

  void registerExports() override
  {
    if (m_hooks.registerExports)
      m_hooks.registerExports();
  }
  void resolveImports() override
  {
    if (m_hooks.resolveImports)
      m_hooks.resolveImports();
  }
  void onPreInit(const muse::IApplication::RunMode &mode) override
  {
    if (m_hooks.onPreInit)
      m_hooks.onPreInit(mode);
  }
  void onInit(const muse::IApplication::RunMode &mode) override
  {
    if (m_hooks.onInit)
      m_hooks.onInit(mode);
  }
  void onAllInited(const muse::IApplication::RunMode &mode) override
  {
    if (m_hooks.onAllInited)
      m_hooks.onAllInited(mode);
  }
  void onDeinit() override
  {
    if (m_hooks.onDeinit)
      m_hooks.onDeinit();
  }

private:
  const Hooks m_hooks;
};
} // namespace dgk
