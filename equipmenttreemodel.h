#ifndef EQUIPMENTTREEMODEL_H
#define EQUIPMENTTREEMODEL_H

#include <QAbstractItemModel>
#include <QString>

class Equipment;
namespace SubstationLayout {
struct Layout;
}

class EquipmentTreeModel : public QAbstractItemModel
{
    Q_OBJECT

public:
    explicit EquipmentTreeModel(QObject *parent = nullptr);
    ~EquipmentTreeModel() override;

    void setLayout(const SubstationLayout::Layout &layout);
    void updateLayout(const SubstationLayout::Layout &layout);
    void retranslate();
    Equipment *equipmentForIndex(const QModelIndex &index) const;
    QModelIndex indexForEquipment(Equipment *equipment) const;
    Equipment *equipmentByName(const QString &name) const;

    QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const override;
    QModelIndex parent(const QModelIndex &index) const override;
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;

private:
    Equipment *itemFromIndex(const QModelIndex &index) const;
    void buildTree(const SubstationLayout::Layout &layout);
    QModelIndex indexForEquipmentRecursive(Equipment *current, Equipment *target, const QModelIndex &currentIndex) const;
    Equipment *equipmentByNameRecursive(Equipment *current, const QString &name) const;

    Equipment *m_rootItem;
};

#endif // EQUIPMENTTREEMODEL_H
