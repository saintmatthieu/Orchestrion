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