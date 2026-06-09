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

#include <async/channel.h>
#include <draw/types/geometry.h>
#include <modularity/imoduleinterface.h>
#include <notation/notationtypes.h>

#include <QString>

namespace dgk
{
class IOrchestrionNotationInteractionProcessor : MODULE_EXPORT_INTERFACE
{
  INTERFACE_ID(IOrchestrionNotationInteractionProcessor);

public:
  virtual ~IOrchestrionNotationInteractionProcessor() = default;

  virtual void onMousePressed(const muse::PointF &logicalPosition,
                              float hitWidth) = 0;
  virtual void onMouseMoved(const muse::PointF &logicalPosition,
                            float hitWidth) = 0;
  virtual muse::async::Channel<const mu::notation::EngravingItem *>
  itemClicked() const = 0;

  //! Returns the note under the given position, or nullptr if there is none
  //! (used by the developer note-labeling feature).
  virtual const mu::notation::EngravingItem *
  hitNoteAt(const muse::PointF &logicalPosition, float hitWidth) const = 0;

  //! Current free-text label (Fingering text) of the given note, or empty.
  virtual QString noteLabel(const mu::notation::EngravingItem *note) const = 0;

  //! Adds/replaces (empty text removes) the note's free-text label, stored as
  //! a Fingering so it persists in the score and exports to MusicXML.
  virtual void setNoteLabel(const mu::notation::EngravingItem *note,
                            const QString &text) = 0;
};
} // namespace dgk