#pragma once

#include "../Touchpad.h"

#include <async/asyncable.h>

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