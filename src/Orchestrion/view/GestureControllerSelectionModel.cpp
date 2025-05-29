#include "GestureControllerSelectionModel.h"
#include <QApplication>
#include <QKeyEvent>

namespace dgk
{
ControllerInfo::ControllerInfo(bool available, QString name)
    : isAvailable(available), shortName(std::move(name))
{
}

GestureControllerSelectionModel::GestureControllerSelectionModel(
    QObject *parent)
    : QAbstractListModel(parent)
{
  qApp->installEventFilter(this);
}

void GestureControllerSelectionModel::init()
{
  for (GestureControllerType type :
       gestureControllerSelector()->selectedControllers())
    if (std::find(m_selectedControllerQueue.begin(),
                  m_selectedControllerQueue.end(),
                  type) == m_selectedControllerQueue.end())
      m_selectedControllerQueue.push_back(type);
  emit selectedControllersInfoChanged();

  midiDeviceService()->availableDevicesChanged().onNotify(
      this, [this] { emit selectedControllersInfoChanged(); });
  midiDeviceService()->selectedDeviceChanged().onNotify(
      this, [this] { emit selectedControllersInfoChanged(); });
}

QString GestureControllerSelectionModel::itemName(int index) const
{
  if (index < 0 || index >= rowCount())
    return QString();

  const auto type = sortedControllers()[index];
  switch (type)
  {
  case GestureControllerType::MidiDevice:
    return tr("Midi Device");
  case GestureControllerType::Touchpad:
    return tr("Touchpad (press \"T\" to toggle)");
  case GestureControllerType::Swipe:
    return tr("Touchpad (Swipe mode)");
  case GestureControllerType::ComputerKeyboard:
    return tr("Computer Keyboard");
  default:
    return QString();
  }
}

QString
GestureControllerSelectionModel::itemShortName(GestureControllerType type) const
{
  switch (type)
  {
  case GestureControllerType::MidiDevice:
    return tr("M");
  case GestureControllerType::Touchpad:
    return tr("T");
  case GestureControllerType::Swipe:
    return tr("S");
  case GestureControllerType::ComputerKeyboard:
    return tr("C");
  default:
    return QString();
  }
}

QList<ControllerInfo>
GestureControllerSelectionModel::selectedControllersInfo() const
{
  QList<ControllerInfo> selectedControllers;
  for (GestureControllerType type : m_selectedControllerQueue)
  {
    const auto isAvailable = type != GestureControllerType::MidiDevice ||
                             midiDeviceService()->selectedDeviceIsAvailable();
    selectedControllers.append({isAvailable, itemShortName(type)});
  }
  return selectedControllers;
}

bool GestureControllerSelectionModel::eventFilter(QObject *watched,
                                                  QEvent *event)
{
  if (event->type() == QEvent::KeyPress)
  {
    const auto keyEvent = static_cast<QKeyEvent *>(event);
    if (keyEvent->key() == Qt::Key_T)
    {
      updateControllerIsSelected(
          static_cast<int>(GestureControllerType::Touchpad),
          !gestureControllerSelector()->selectedControllers().count(
              GestureControllerType::Touchpad));
      return true;
    }
  }
  return QAbstractListModel::eventFilter(watched, event);
}

void GestureControllerSelectionModel::updateControllerIsSelected(int index,
                                                                 bool checked)
{
  auto types = gestureControllerSelector()->selectedControllers();
  const GestureControllerType type = sortedControllers()[index];

  if (!!types.count(type) == checked)
    return;

  if (checked)
  {
    types.insert(type);
    if (std::find(m_selectedControllerQueue.begin(),
                  m_selectedControllerQueue.end(),
                  type) == m_selectedControllerQueue.end())
    {
      m_selectedControllerQueue.push_back(type);
    }
  }
  else
  {
    types.erase(type);
    const auto it = std::find(m_selectedControllerQueue.begin(),
                              m_selectedControllerQueue.end(), type);
    if (it != m_selectedControllerQueue.end())
      m_selectedControllerQueue.erase(it);
  }
  gestureControllerSelector()->setSelectedControllers(types);

  const QModelIndex modelIndex = this->index(index);
  emit dataChanged(modelIndex, modelIndex, {rControllerIsSelected});
  emit selectedControllersInfoChanged();
}

QHash<int, QByteArray> GestureControllerSelectionModel::roleNames() const
{
  QHash<int, QByteArray> roles;
  roles[rControllerIsSelected] = "controllerIsSelected";
  roles[rControllerIsAvailable] = "controllerIsAvailable";
  roles[rControllerName] = "controllerName";
  return roles;
}

QVariant GestureControllerSelectionModel::data(const QModelIndex &index,
                                               int role) const
{
  if (!index.isValid() || index.row() >= rowCount())
    return QVariant();

  switch (role)
  {
  case rControllerIsSelected:
  {
    const GestureControllerType type = sortedControllers()[index.row()];
    return !!gestureControllerSelector()->selectedControllers().count(type);
  }
  case rControllerName:
    return itemName(index.row());
  default:
    return QVariant();
  }
}

std::vector<GestureControllerType>
GestureControllerSelectionModel::sortedControllers() const
{
  const auto controllers = gestureControllerSelector()->functionalControllers();
  std::vector<GestureControllerType> sorted{GestureControllerType::MidiDevice};
  sorted.reserve(controllers.size());
  if (controllers.count(GestureControllerType::Touchpad))
    sorted.push_back(GestureControllerType::Touchpad);
  if (controllers.count(GestureControllerType::Swipe))
    sorted.push_back(GestureControllerType::Swipe);
  if (controllers.count(GestureControllerType::ComputerKeyboard))
    sorted.push_back(GestureControllerType::ComputerKeyboard);
  return sorted;
}

int GestureControllerSelectionModel::rowCount(const QModelIndex &parent) const
{
  Q_UNUSED(parent);
  return static_cast<int>(
      gestureControllerSelector()->functionalControllers().size());
}

} // namespace dgk