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
#include "touchpad_testapp_model.h"

#include "log.h"

namespace dgk
{
TouchpadTestappModel::TouchpadTestappModel(QObject *parent)
    : QObject{parent}, m_touchpad{std::make_unique<Touchpad>()}
{
}

void TouchpadTestappModel::init()
{
  m_touchpad->contactChanged().onReceive(
      this,
      [this](const Contacts &contacts)
      {
        std::for_each(m_contacts.begin(), m_contacts.end(),
                      [](QContact *contact) { delete contact; });
        m_contacts.clear();
        m_contacts.reserve(contacts.size());
        for (const Contact &contact : contacts)
        {
          // LOGI() << "Matt: contact: " << contact.uid;
          m_contacts.push_back(
              new QContact(contact.uid, contact.uid, contact.x, contact.y));
        }

        emit contactsChanged();
      });
}

QList<QContact *> TouchpadTestappModel::contacts() const { return m_contacts; }

} // namespace dgk