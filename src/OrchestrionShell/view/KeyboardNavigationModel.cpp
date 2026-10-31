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
#include "KeyboardNavigationModel.h"

#include <QCoreApplication>
#include <QKeyEvent>

namespace dgk
{
KeyboardNavigationModel::KeyboardNavigationModel(QObject *parent)
    : QObject(parent)
{
}

void KeyboardNavigationModel::load()
{
  const auto onChange = [this]
  {
    if (isNavigating())
      emit navigated();
  };
  navigationController()->navigationChanged().onNotify(this, onChange);
  // The first key moves the focus before turning the highlight on.
  navigationController()->highlightChanged().onNotify(this, onChange);

  qApp->installEventFilter(this);
}

bool KeyboardNavigationModel::isNavigating() const
{
  return navigationController()->isHighlight() &&
         navigationController()->activeControl();
}

bool KeyboardNavigationModel::eventFilter(QObject *watched, QEvent *event)
{
  // Not bound as shortcuts: a shortcut takes its key from whatever has the
  // focus, such as the score shelf's search field.
  if (event->type() != QEvent::KeyPress || !isNavigating())
    return QObject::eventFilter(watched, event);
  switch (static_cast<QKeyEvent *>(event)->key())
  {
  case Qt::Key_Escape:
    release();
    return true;
  case Qt::Key_Return:
  case Qt::Key_Enter:
    // Like Space (shortcuts.json's NAVIGATION scope)
    navigationController()->activeControl()->trigger();
    return true;
  default:
    return QObject::eventFilter(watched, event);
  }
}

void KeyboardNavigationModel::release()
{
  if (!navigationController()->isHighlight())
    return;
  // Not resetNavigation(), which makes the first control active again: the
  // next Tab would then go on from it, rather than start with it.
  for (muse::ui::INavigation *const nav :
       std::initializer_list<muse::ui::INavigation *>{
           navigationController()->activeControl(),
           navigationController()->activePanel(),
           navigationController()->activeSection()})
    if (nav)
      nav->setActive(false);
  navigationController()->setIsHighlight(false);
}
} // namespace dgk
