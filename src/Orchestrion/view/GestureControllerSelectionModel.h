#pragma once

#include "ExternalDevices/IMidiDeviceService.h"
#include "GestureControllers/IGestureControllerSelector.h"

#include "async/asyncable.h"
#include "modularity/ioc.h"
#include <QAbstractListModel>

namespace dgk
{
class ControllerInfo
{
  Q_GADGET;
  Q_PROPERTY(bool isAvailable MEMBER isAvailable);
  Q_PROPERTY(QString shortName MEMBER shortName);

public:
  ControllerInfo() = default;
  ControllerInfo(bool available, QString name);

  bool isAvailable = false;
  QString shortName;
};

class GestureControllerSelectionModel : public QAbstractListModel,
                                        public muse::async::Asyncable,
                                        public muse::Injectable
{
  Q_OBJECT;

  Q_PROPERTY(QList<ControllerInfo> selectedControllersInfo READ
                 selectedControllersInfo NOTIFY selectedControllersInfoChanged);

  muse::Inject<IGestureControllerSelector> gestureControllerSelector;
  muse::Inject<IMidiDeviceService> midiDeviceService;

public:
  explicit GestureControllerSelectionModel(QObject *parent = nullptr);

  Q_INVOKABLE void init();
  Q_INVOKABLE void updateControllerIsSelected(int index, bool checked);

  QList<ControllerInfo> selectedControllersInfo() const;

signals:
  void selectedControllersInfoChanged();

private:
  enum RoleNames
  {
    rControllerIsSelected = Qt::UserRole + 1,
    rControllerIsAvailable = Qt::UserRole + 2,
    rControllerName = Qt::UserRole + 3,
  };

  std::vector<GestureControllerType> sortedControllers() const;

  int rowCount(const QModelIndex &parent = QModelIndex()) const override;
  QVariant data(const QModelIndex &index, int role) const override;
  QHash<int, QByteArray> roleNames() const override;

  bool eventFilter(QObject *watched, QEvent *event) override;

  QString itemName(int index) const;
  QString itemShortName(GestureControllerType) const;

  std::vector<GestureControllerType> m_selectedControllerQueue;
};
} // namespace dgk
