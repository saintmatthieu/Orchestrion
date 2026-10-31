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
#include "TutorialOfferModel.h"

#include <global/settings.h>

#include <QFileInfo>
#include <chrono>

namespace dgk
{
namespace
{
// A moment to take in the score before the offer comes up.
constexpr auto delayAfterOpen = std::chrono::milliseconds{2500};

const muse::Settings::Key TUTORIAL_OFFER_DISMISSED("OrchestrionShell",
                                                   "TUTORIAL_OFFER_DISMISSED");

// The tutorial offered
constexpr auto gettingStarted = "GettingStarted";
} // namespace

TutorialOfferModel::TutorialOfferModel(QObject *parent) : QObject(parent)
{
  m_delay.setSingleShot(true);
  m_delay.setInterval(delayAfterOpen);
  connect(&m_delay, &QTimer::timeout, this, [this] { update(); });
}

void TutorialOfferModel::load()
{
  muse::settings()->setDefaultValue(TUTORIAL_OFFER_DISMISSED, muse::Val{false});
  m_dismissed = muse::settings()->value(TUTORIAL_OFFER_DISMISSED).toBool();
  if (m_dismissed)
    return;
  projectSession()->projectChanged().onNotify(this,
                                              [this] { m_delay.start(); });
  tutorials()->runningChanged().onNotify(this, [this] { update(); });
  if (projectSession()->project())
    m_delay.start();
}

void TutorialOfferModel::play()
{
  dismiss();
  const std::vector<ITutorials::Tutorial> &all = tutorials()->tutorials();
  for (const ITutorials::Tutorial &tutorial : all)
    if (QFileInfo{tutorial.path}.completeBaseName() == gettingStarted)
    {
      tutorials()->run(tutorial.path);
      return;
    }
  if (!all.empty())
    tutorials()->run(all.front().path);
}

void TutorialOfferModel::dismiss()
{
  m_delay.stop();
  if (!m_dismissed)
  {
    m_dismissed = true;
    muse::settings()->setSharedValue(TUTORIAL_OFFER_DISMISSED, muse::Val{true});
  }
  setShown(false);
}

void TutorialOfferModel::update()
{
  setShown(!m_dismissed && projectSession()->project() &&
           !tutorials()->running() && !tutorials()->tutorials().empty());
}

bool TutorialOfferModel::shown() const { return m_shown; }

void TutorialOfferModel::setShown(bool shown)
{
  if (shown == m_shown)
    return;
  m_shown = shown;
  emit shownChanged();
}
} // namespace dgk
