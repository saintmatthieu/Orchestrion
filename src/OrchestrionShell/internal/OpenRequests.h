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
#pragma once

#include "OrchestrionShell/IOpenRequests.h"

namespace dgk
{
class OpenRequests : public IOpenRequests
{
public:
  void submit(const QString &linkOrPath) override;
  bool hasPending() const override;
  std::optional<OpenRequest> take() override;
  void setReady() override;
  muse::async::Notification requested() const override;

private:
  std::optional<OpenRequest> m_pending;
  bool m_ready = false;
  muse::async::Notification m_requested;
};
} // namespace dgk
