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

#include "OrchestrionCommon/OrchestrionIoc.h"

#include <types/ret.h>
#include <ui/imainwindow.h>
#include <ui/inavigationcontroller.h>

#include <QObject>
#include <QPointF>
#include <QVariantAnimation>
#include <functional>

class QQuickItem;

namespace dgk
{
/**
 * The pointer of demos (and scripts): it glides to a control, named by its
 * navigation path (e.g. Controls/Playback/Play), and clicks it.
 *
 * The app sees real mouse events: moves along the way, then a press and a
 * release, sent to the window. So the controls wake, the button glows under
 * it and its own click handler runs, as with a user's mouse - which itself
 * doesn't move. DemoCursorOverlay.qml draws the pointer from the properties.
 * Main window only, for now.
 */
class DemoCursor : public QObject, public dgk::Injectable
{
  Q_OBJECT

  Q_PROPERTY(bool shown READ shown NOTIFY shownChanged)
  /** In the main window's coordinates. */
  Q_PROPERTY(QPointF position READ position NOTIFY positionChanged)
  Q_PROPERTY(bool pressed READ pressed NOTIFY pressedChanged)

  dgk::Inject<muse::ui::INavigationController> navigationController{this};
  dgk::Inject<muse::ui::IMainWindow> mainWindow{this};

public:
  using Done = std::function<void(const muse::Ret &)>;

  explicit DemoCursor(QObject *parent = nullptr);

  /**
   * Glides to the control, then clicks it; `done` once released.
   */
  void click(const QString &controlPath, Done done);

  /**
   * Glides to the control, hovering it; `done` once there.
   */
  void moveTo(const QString &controlPath, Done done);

  void hide();

  bool shown() const;
  QPointF position() const;
  bool pressed() const;

signals:
  void shownChanged();
  void positionChanged();
  void pressedChanged();

private:
  QQuickItem *findItem(const QString &controlPath, muse::Ret &ret) const;
  void glideTo(QPointF target, std::function<void()> then);
  void sendMouse(QEvent::Type type, Qt::MouseButton button,
                 Qt::MouseButtons buttons);
  void setPosition(QPointF position);
  void setPressed(bool pressed);

  QVariantAnimation m_glide;
  std::function<void()> m_afterGlide;
  QPointF m_position;
  bool m_shown = false;
  bool m_pressed = false;
};
} // namespace dgk
