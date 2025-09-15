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

#include "../Touchpad.h"

#include <global/async/asyncable.h>

#include <QList>
#include <QObject>

namespace dgk
{
class QContact : public QObject
{
  Q_OBJECT

  Q_PROPERTY(int id MEMBER id CONSTANT)
  Q_PROPERTY(int uid MEMBER uid CONSTANT)
  Q_PROPERTY(float x MEMBER x CONSTANT)
  Q_PROPERTY(float y MEMBER y CONSTANT)

public:
  QContact(int id, int uid, float x, float y, QObject *parent = nullptr)
      : QObject(parent), id(id), uid(uid), x(x), y(y)
  {
  }

  const int id;
  const int uid;
  const float x;
  const float y;
};

class TouchpadTestappModel : public QObject, public muse::async::Asyncable
{
  Q_OBJECT

  Q_PROPERTY(QList<QContact *> contacts READ contacts NOTIFY contactsChanged)

public:
  explicit TouchpadTestappModel(QObject *parent = nullptr);

  Q_INVOKABLE void init();
  QList<QContact *> contacts() const;

signals:
  void contactsChanged();

private:
  const std::unique_ptr<Touchpad> m_touchpad;
  QList<QContact *> m_contacts;
};
} // namespace dgk