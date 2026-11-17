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

#include "IModifiableItemRegistry.h"
#include "IOrchestrionSequencer.h"
#include "IOrchestrionSequencerConfiguration.h"
#include "OrchestrionSynthesis/ITrackChannelMapper.h"
#include "OrchestrionTypes.h"
#include "ScoreAnimation/ISegmentRegistry.h"
#include "internal/OrchestrionSequencer.h"
#include "internal/PlayThread.h"
#include "playback/iplaybackcontroller.h"
#include <QCoreApplication>
#include <QMetaObject>
#include <QThread>
#include <memory>
#include <modularity/ioc.h>
#include <vector>

#include "OrchestrionCommon/OrchestrionIoc.h"
namespace mu::notation
{
class IMasterNotation;
}

namespace dgk
{
/**
 * Owns `object`, which is destroyed on the main thread, whichever thread lets
 * go of it last.
 */
template <typename T> std::shared_ptr<T> DestroyedOnMainThread(T *object)
{
  return std::shared_ptr<T>(
      object,
      [](T *object)
      {
        QCoreApplication *const app = QCoreApplication::instance();
        if (!app || QThread::currentThread() == app->thread())
          delete object;
        else
          QMetaObject::invokeMethod(
              app, [object] { delete object; }, Qt::QueuedConnection);
      });
}

struct NotationProducts
{
  const std::shared_ptr<OrchestrionSequencer> sequencer;
  const IModifiableItemRegistryPtr modifiableItemRegistry;
  const Staff rightHand;
  const Staff leftHand;
};

class OrchestrionSequencerFactory : public dgk::Injectable
{
  dgk::Inject<ISegmentRegistry> segmentRegistry{this};
  dgk::Inject<ITrackChannelMapper> mapper{this};
  dgk::Inject<IOrchestrionSequencerConfiguration> sequencerConfig{this};

public:
  /** The sequencer is to play on `playThread`. */
  NotationProducts
  CreateSequencer(mu::notation::IMasterNotation &masterNotation,
                  PlayThread &playThread);
};
} // namespace dgk