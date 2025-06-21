#include "GestureControllerSelectionModel.h"
#include <QApplication>
#include <QKeyEvent>

namespace dgk
{
ControllerInfo::ControllerInfo(bool isWorking, QString icon,
                               GestureControllerType type)
    : isWorking(isWorking), icon(std::move(icon)), type(type)
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
  emitSignals();

  midiDeviceService()->availableDevicesChanged().onNotify(this, [this]
                                                          { emitSignals(); });
  midiDeviceService()->selectedDeviceChanged().onNotify(this, [this]
                                                        { emitSignals(); });

  gestureControllerSelector()->selectedControllersChanged().onNotify(
      this,
      [this]
      {
        const auto types = gestureControllerSelector()->selectedControllers();

        // Remove controllers that are not in the current selection
        m_selectedControllerQueue.erase(
            std::remove_if(m_selectedControllerQueue.begin(),
                           m_selectedControllerQueue.end(),
                           [&types](GestureControllerType type)
                           { return !types.count(type); }),
            m_selectedControllerQueue.end());

        // Add controllers that are in the current selection but not in the
        // queue
        for (GestureControllerType type : types)
          if (std::find(m_selectedControllerQueue.begin(),
                        m_selectedControllerQueue.end(),
                        type) == m_selectedControllerQueue.end())
            m_selectedControllerQueue.push_back(type);

        emitSignals();
      });
}

void GestureControllerSelectionModel::emitSignals()
{
  for (auto i = 0; i < rowCount(); ++i)
  {
    const QModelIndex modelIndex = index(i);
    emit dataChanged(modelIndex, modelIndex, {rControllerIsWorking});
  }
  emit selectedControllersInfoChanged();
  emit hasWarningChanged();
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

QString GestureControllerSelectionModel::iconDir() const
{
  return QString{"file:///"} +
         globalConfiguration()->appDataPath().toQString() +
         "icons/controllers/";
}

QString
GestureControllerSelectionModel::itemIcon(GestureControllerType type) const
{
  switch (type)
  {
  case GestureControllerType::MidiDevice:
    return iconDir() + "piano_24dp_1F1F1F_FILL0_wght400_GRAD0_opsz24.svg";
  case GestureControllerType::Touchpad:
    return iconDir() +
           "trackpad_input_3_24dp_1F1F1F_FILL0_wght400_GRAD0_opsz24.svg";
  case GestureControllerType::Swipe:
    return iconDir() +
           "swipe_vertical_24dp_1F1F1F_FILL0_wght400_GRAD0_opsz24.svg";
  case GestureControllerType::ComputerKeyboard:
    return iconDir() +
           "keyboard_alt_24dp_1F1F1F_FILL0_wght400_GRAD0_opsz24.svg";
  default:
    return {};
  }
}

QString GestureControllerSelectionModel::warningIcon() const
{
  return iconDir() + "warning_24dp_1F1F1F_FILL0_wght400_GRAD0_opsz24.svg";
}

bool GestureControllerSelectionModel::hasWarning() const
{
  const auto info = selectedControllersInfo();
  return !std::any_of(info.begin(), info.end(),
                      [](const ControllerInfo &controller)
                      { return controller.isWorking; });
}

QList<ControllerInfo>
GestureControllerSelectionModel::selectedControllersInfo() const
{
  QList<ControllerInfo> selectedControllers;
  for (GestureControllerType type : m_selectedControllerQueue)
    selectedControllers.append(
        {isControllerWorking(type), itemIcon(type), type});
  return selectedControllers;
}

bool GestureControllerSelectionModel::isControllerWorking(
    GestureControllerType type) const
{
  if (type != GestureControllerType::MidiDevice)
    return true;
  if (const auto midiDevice = midiDeviceService()->selectedDevice())
    return midiDeviceService()->isAvailable(*midiDevice) &&
           !midiDeviceService()->isNoDevice(*midiDevice);
  return false;
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
  emitSignals();
}

QHash<int, QByteArray> GestureControllerSelectionModel::roleNames() const
{
  QHash<int, QByteArray> roles;
  roles[rControllerIsSelected] = "controllerIsSelected";
  roles[rControllerIsWorking] = "controllerIsWorking";
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
  case rControllerIsWorking:
    return isControllerWorking(index.row());
  default:
    return QVariant();
  }
}

bool GestureControllerSelectionModel::isControllerWorking(int index) const
{
  const auto types = sortedControllers();
  if (index < 0 || index >= types.size())
  {
    assert(false);
    return false;
  }
  return isControllerWorking(types.at(index));
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